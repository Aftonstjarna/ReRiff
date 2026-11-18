using System;using System.IO;using System.Text;
class LoggingSmoke {
 static void Check(bool v){if(!v)throw new Exception("Logging assertion failed");}
 static int Main(){string root=Path.Combine(Path.GetTempPath(),"reriff-logging-"+Guid.NewGuid());Directory.CreateDirectory(root);try{
 Check(!ReRiff.Logging.Enabled(root));ReRiff.Logging.Append(root,"client.log","off");Check(!Directory.Exists(Path.Combine(root,"logs")));
 Directory.CreateDirectory(Path.Combine(root,"localserver"));File.WriteAllText(Path.Combine(root,"localserver/client-channel.ini"),"[client]\nchannel=experimental\n");Check(ReRiff.Logging.Enabled(root));ReRiff.Logging.Append(root,"client.log","on");string log=Path.Combine(root,"logs/ReRiff-connections.log");Check(File.ReadAllText(log).EndsWith("[client] on"+Environment.NewLine));
 Check(File.ReadAllText(log).Contains("[diagnostic] client_id="));
 ReRiff.Logging.SetAccountIdentity(root,"11111111-1111-1111-1111-111111111111","22222222-2222-2222-2222-222222222222");
 ReRiff.Logging.Append(root,"client.log","identified");
 Check(File.ReadAllText(log).Contains("account_id=22222222-2222-2222-2222-222222222222"));
 Check(File.ReadAllText(log).Contains("identity=current"));
 Check(!File.ReadAllText(Path.Combine(root,"localserver/diagnostic-context.ini")).Contains("token"));
 Check(ReRiff.Logging.InstallationId(root)==ReRiff.Logging.InstallationId(root));
 Check(!ReRiff.Logging.Clean("name\r\n[injected]").Contains("\n"));
 string retained=File.ReadAllText(log);
 byte[] config=ReRiff.Logging.Config(Encoding.UTF8.GetBytes("[server]\nexternal_url = https://example.invalid\n"),false);File.WriteAllBytes(Path.Combine(root,"server.ini"),config);Check(!ReRiff.Logging.Enabled(root));ReRiff.Logging.Append(root,"client.log","off");Check(File.ReadAllText(log)==retained);Check(Encoding.UTF8.GetString(config).Contains("https://example.invalid"));
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
 ReRiff.Logging.Append(root,"ReRiff-engine-menu.log","first\r\n日本語\n  exception continuation\n");
 foreach(string line in File.ReadAllLines(Path.Combine(root,"logs/ReRiff-game.log")))Check(ReRiff.Logging.Stamped(line));
 var sink=new StringWriter();using(var writer=new ReRiff.Logging.TimestampWriter(sink)){writer.Write("frag");writer.Flush();writer.Write("ment\r\nsecond\n");writer.Flush();}
 string[] lines=sink.ToString().Split(new[]{Environment.NewLine},StringSplitOptions.RemoveEmptyEntries);Check(lines.Length==2&&lines[0].Substring(27)=="fragment"&&lines[1].Substring(27)=="second");
 Check(ReRiff.Logging.Lines(lines[0]+"\n","")==lines[0]+Environment.NewLine);
 Console.WriteLine("PASS: stable/beta defaults, explicit override, no disabled logs folder, preserved existing logs and UTF-16 config");return 0;
 }finally{Directory.Delete(root,true);}}
}
