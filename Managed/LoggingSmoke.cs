using System;using System.IO;using System.Text;
class LoggingSmoke {
 static void Check(bool v){if(!v)throw new Exception("Logging assertion failed");}
 static int Main(){string root=Path.Combine(Path.GetTempPath(),"reriff-logging-"+Guid.NewGuid());Directory.CreateDirectory(root);try{
 Check(!ReRiff.Logging.Enabled(root));ReRiff.Logging.Append(root,"client.log","off");Check(!Directory.Exists(Path.Combine(root,"logs")));
 Directory.CreateDirectory(Path.Combine(root,"localserver"));File.WriteAllText(Path.Combine(root,"localserver/client-channel.ini"),"[client]\nchannel=experimental\n");Check(ReRiff.Logging.Enabled(root));ReRiff.Logging.Append(root,"client.log","on");string log=Path.Combine(root,"logs/ReRiff-connections.log");Check(File.ReadAllText(log)=="[client] on");
 byte[] config=ReRiff.Logging.Config(Encoding.UTF8.GetBytes("[server]\nexternal_url = https://example.invalid\n"),false);File.WriteAllBytes(Path.Combine(root,"server.ini"),config);Check(!ReRiff.Logging.Enabled(root));ReRiff.Logging.Append(root,"client.log","off");Check(File.ReadAllText(log)=="[client] on");Check(Encoding.UTF8.GetString(config).Contains("https://example.invalid"));
 File.WriteAllText(Path.Combine(root,"server.ini"),"[diagnostics]\nlogging_enabled=true\nlog_unicode=false\nlog_localserver=false\nlog_engine_menu=true\n");
 ReRiff.Logging.Append(root,"ReRiff-unicode.log","off");Check(!File.Exists(Path.Combine(root,"logs/ReRiff-unicode.log")));
 Check(!ReRiff.Logging.Enabled(root,"ReRiff-localserver-helper.log"));Check(ReRiff.Logging.Enabled(root,"ReRiff-engine-menu.log"));
 Check(ReRiff.Logging.Enabled(root,"ReRiff-client.log"));
 File.WriteAllBytes(Path.Combine(root,"server.ini"),ReRiff.Logging.Config(File.ReadAllBytes(Path.Combine(root,"server.ini")),false));
 Check(!ReRiff.Logging.Enabled(root,"ReRiff-engine-menu.log"));
 File.WriteAllBytes(Path.Combine(root,"server.ini"),ReRiff.Logging.Config(File.ReadAllBytes(Path.Combine(root,"server.ini")),true));
 Check(!ReRiff.Logging.Enabled(root,"ReRiff-unicode.log"));Check(ReRiff.Logging.Enabled(root,"ReRiff-engine-menu.log"));
 File.WriteAllText(Path.Combine(root,"client.ini"),"[diagnostics]\nlogging_enabled=true\nlog_connections=false\nlog_game=true\nlog_text=true\n");
 Check(!ReRiff.Logging.Enabled(root,"ReRiff-network.log")&&!ReRiff.Logging.Enabled(root,"ReRiff-news.log"));
 Check(ReRiff.Logging.Enabled(root,"ReRiff-unicode.log"));
 ReRiff.Logging.Append(root,"ReRiff-engine-menu.log","native bridge\n");ReRiff.Logging.Append(root,"ReRiff-singleplayer.log","save\n");
 string grouped=File.ReadAllText(Path.Combine(root,"logs/ReRiff-game.log"));Check(grouped.Contains("[engine_menu] native bridge")&&grouped.Contains("[singleplayer] save"));
 Check(!File.Exists(Path.Combine(root,"logs/ReRiff-singleplayer.log")));
 ReRiff.Logging.Append(root,"ReRiff-metadata.log","metadata\n");Check(File.ReadAllText(Path.Combine(root,"logs/ReRiff-text.log")).Contains("[metadata]"));
 byte[] unicode=Encoding.Unicode.GetPreamble(),body=Encoding.Unicode.GetBytes("[server]\r\nname=日本語\r\n");byte[] input=new byte[unicode.Length+body.Length];Buffer.BlockCopy(unicode,0,input,0,unicode.Length);Buffer.BlockCopy(body,0,input,unicode.Length,body.Length);byte[] updated=ReRiff.Logging.Config(input,false);Check(updated[0]==255&&updated[1]==254&&Encoding.Unicode.GetString(updated).Contains("name=日本語\r\n"));
 Console.WriteLine("PASS: stable/beta defaults, explicit override, no disabled logs folder, preserved existing logs and UTF-16 config");return 0;
 }finally{Directory.Delete(root,true);}}
}
