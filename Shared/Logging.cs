using System;
using System.IO;
using System.Text;
using System.Threading;
using System.Collections.Generic;

namespace ReRiff {
    internal static class Logging {
        internal static bool? Setting(string folder) {
            try {string value;if(!ConfigFiles.Read(folder,"diagnostics").TryGetValue("logging_enabled",out value))return null;
            return value=="1"||value.Equals("true",StringComparison.OrdinalIgnoreCase)||value.Equals("yes",StringComparison.OrdinalIgnoreCase);
            }catch(IOException){}catch(UnauthorizedAccessException){}return null;
        }
        internal static bool Enabled(string folder) {
            bool? saved=Setting(folder);if(saved.HasValue)return saved.Value;
            try {
                string channel=Path.Combine(folder,"localserver/client-channel.ini");
                if(File.Exists(channel))foreach(string raw in File.ReadAllLines(channel)) {
                    string line=raw.Trim();int eq=line.IndexOf('=');if(eq>0&&line.Substring(0,eq).Trim().Equals("channel",StringComparison.OrdinalIgnoreCase))return line.Substring(eq+1).Trim().Equals("experimental",StringComparison.OrdinalIgnoreCase);
                }
                string version=Path.Combine(folder,"localserver/client-version.txt");return File.Exists(version)&&File.ReadAllText(version).Contains("-");
            }catch(IOException){}catch(UnauthorizedAccessException){}
            return false;
        }
        internal static string Category(string name) {
            string category=Path.GetFileNameWithoutExtension(name).ToLowerInvariant().Replace("reriff-", "").Replace('-', '_');
            if(category.StartsWith("localserver"))category="localserver";
            if(category=="python_setup")category="python";
            if(category.StartsWith("installer"))category="installer";
            return category;
        }
        internal static string Group(string category) {
            if(category=="client"||category=="network"||category=="news")return "connections";
            if(category=="engine_menu"||category=="singleplayer")return "game";
            if(category=="unicode"||category=="metadata")return "text";
            return category;
        }
        internal static string FileName(string name) {
            string category=Category(name),group=Group(category);
            return group!=category?"ReRiff-"+group+".log":name;
        }
        internal static bool Enabled(string folder,string name) {
            if(!Enabled(folder))return false;
            string category=Category(name),group=Group(category);
            try {string value;var settings=ConfigFiles.Read(folder,"diagnostics");
                if(settings.TryGetValue("log_"+group,out value)||settings.TryGetValue("log_"+category,out value))
                    return value=="1"||value.Equals("true",StringComparison.OrdinalIgnoreCase)||value.Equals("yes",StringComparison.OrdinalIgnoreCase);
            }catch(IOException){}catch(UnauthorizedAccessException){}
            return true;
        }
        internal static byte[] Config(byte[] input,bool enabled) {
            Encoding encoding=new UTF8Encoding(false);int skip=0;
            if(input.Length>=2&&input[0]==255&&input[1]==254){encoding=Encoding.Unicode;skip=2;}
            else if(input.Length>=2&&input[0]==254&&input[1]==255){encoding=Encoding.BigEndianUnicode;skip=2;}
            else if(input.Length>=3&&input[0]==239&&input[1]==187&&input[2]==191){encoding=new UTF8Encoding(true);skip=3;}
            string text=encoding.GetString(input,skip,input.Length-skip);string newline=text.Contains("\r\n")?"\r\n":"\n";
            var lines=new List<string>(text.Replace("\r\n","\n").Split('\n'));
            bool section=false,found=false;int at=-1;
            for(int i=0;i<lines.Count;i++) {
                string line=lines[i].Trim();if(line.StartsWith("[")){section=line.Equals("[diagnostics]",StringComparison.OrdinalIgnoreCase);if(section)at=i;}
                int eq=line.IndexOf('=');if(section&&eq>0&&line.Substring(0,eq).Trim().Equals("logging_enabled",StringComparison.OrdinalIgnoreCase)){lines[i]="logging_enabled = "+(enabled?"true":"false");found=true;}
            }
            if(!found){if(at>=0)lines.Insert(at+1,"logging_enabled = "+(enabled?"true":"false"));else lines.AddRange(new[]{"","[diagnostics]","logging_enabled = "+(enabled?"true":"false")});}
            byte[] body=encoding.GetBytes(string.Join("\n",lines.ToArray()).TrimEnd('\n').Replace("\n",newline)+newline),preamble=encoding.GetPreamble(),output=new byte[preamble.Length+body.Length];
            Buffer.BlockCopy(preamble,0,output,0,preamble.Length);Buffer.BlockCopy(body,0,output,preamble.Length,body.Length);return output;
        }
        internal static void Append(string folder,string name,string message) {
            if(!Enabled(folder,name))return;
            using(var mutex=new Mutex(false,@"Local\ReRiff.Diagnostics")) {
                bool acquired=false;
                try {
                    try {acquired=mutex.WaitOne(1000);}catch(AbandonedMutexException){acquired=true;}
                    if(!acquired)return;
                    string logs=Path.Combine(folder,"logs");Directory.CreateDirectory(logs);
                    string target=FileName(name);
                    if(target!=name)message="["+Category(name)+"] "+message;
                    File.AppendAllText(Path.Combine(logs,target),message,new UTF8Encoding(false));
                }finally{if(acquired)mutex.ReleaseMutex();}
            }
        }
    }
}
