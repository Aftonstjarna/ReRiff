using System;using System.IO;using System.Text;using System.Collections.Generic;
class ConfigSplitSmoke {
 static void Check(bool value,string text){if(!value)throw new Exception(text);}
 static void Main(){string root=Path.Combine(Path.GetTempPath(),"reriff-split-"+Guid.NewGuid());Directory.CreateDirectory(root);try{
  string original="; keep unknown settings\n[server]\nexternal_url=https://example.invalid\n[graphics]\nui_scale=55\n[diagnostics]\nlogging_enabled=true\nlog_unicode=false\n[languages]\ncjk_enabled=true\n[launcher]\nskip_launcher=false\n[progression]\nisolate_singleplayer=1\ntrace_enabled=1\n[other]\nvalue=untouched\n";
  File.WriteAllText(Path.Combine(root,"server.ini"),original,Encoding.Unicode);
  File.WriteAllText(Path.Combine(root,"client.ini"),"[graphics]\nui_scale=50\n",Encoding.UTF8);
  Check(ReRiff.ConfigFiles.Read(root,"diagnostics")["log_unicode"]=="false","legacy per-key fallback");
  ReRiff.ConfigFiles.Migrate(root);
  Check(ReRiff.ConfigFiles.Read(root,"graphics")["ui_scale"]=="50","explicit new value wins");
  Check(ReRiff.ConfigFiles.Read(root,"languages")["cjk_enabled"]=="true","language preserved");
  Check(ReRiff.ConfigFiles.Read(root,"progression")["isolate_singleplayer"]=="1","isolation preserved");
  Check(!File.ReadAllText(Path.Combine(root,"server.ini")).Contains("[graphics]"),"legacy section removed");
  Check(File.ReadAllText(Path.Combine(root,"server.ini")).Contains("value=untouched"),"unknown settings preserved");
  string before=File.ReadAllText(Path.Combine(root,"client.ini"));ReRiff.ConfigFiles.Migrate(root);Check(before==File.ReadAllText(Path.Combine(root,"client.ini")),"idempotent migration");
  ReRiff.ProgressionBalance.Load(root);var defaults=ReRiff.ProgressionBalance.Current;
  Check(defaults.Discount(33)==.25m,"default discount");Check(ReRiff.ProgressionRewards.RaceCoins(80,0,0,1)[0]==160,"default multiplier");
  var custom=ReRiff.ProgressionBalance.Parse(new Dictionary<string,string>{{"coin_reward_rate","1.5"},{"prestige_multiplier_growth","0.5"},{"prestige_multiplier_cap","2"},{"paint_price_scale","2"},{"discount_level_30","0.28"}});
  ReRiff.ProgressionBalance.Current=custom;Check(ReRiff.ProgressionRewards.RaceCoins(100,10,0,4)[0]==300,"custom capped award");
  Check(ReRiff.ProgressionRewards.PaintPrice(500,33,custom.PaintScale)==720,"custom paint price");
  Check(defaults.Revision!=custom.Revision,"revision tracks balance");
  foreach(string value in new[]{"-1","NaN","1e99"}){bool rejected=false;try{ReRiff.ProgressionBalance.Parse(new Dictionary<string,string>{{"coin_reward_rate",value}});}catch(InvalidDataException){rejected=true;}Check(rejected,"invalid number rejected");}
  for(int i=0;i<4;i++){File.WriteAllText(Path.Combine(root,"server.ini"),"[server]\nexternal_url=https://example.invalid\n[graphics]\nui_scale="+i+"\n");ReRiff.ConfigFiles.Migrate(root);}
  Check(Directory.GetFiles(Path.Combine(root,".bak/configs"),"*-server.ini").Length<=2,"only two server config backups");
  Check(Directory.GetFiles(Path.Combine(root,".bak/configs"),"*-client.ini").Length<=2,"only two client config backups");
  Console.WriteLine("PASS: split migration, precedence, legacy fallback, progression retention, configurable awards/prices and invalid settings");
 }finally{ReRiff.ProgressionBalance.Current=new ReRiff.ProgressionBalance();Directory.Delete(root,true);}}
}
