using System;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace ReRiff {
    internal static class Logging {
        internal static bool? Setting(string folder) {
            try {
                bool section=false;
                foreach(string raw in File.ReadAllLines(Path.Combine(folder,"server.ini"))) {
                    string line=raw.Trim();if(line.StartsWith("["))section=line.Equals("[diagnostics]",StringComparison.OrdinalIgnoreCase);
                    int eq=line.IndexOf('=');if(section&&eq>0&&line.Substring(0,eq).Trim().Equals("logging_enabled",StringComparison.OrdinalIgnoreCase)) {
                        string value=line.Substring(eq+1).Trim();return value=="1"||value.Equals("true",StringComparison.OrdinalIgnoreCase)||value.Equals("yes",StringComparison.OrdinalIgnoreCase);
                    }
                }
            }catch(IOException){}catch(UnauthorizedAccessException){}
            return null;
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
            if(!Enabled(folder))return;
            string logs=Path.Combine(folder,"logs");Directory.CreateDirectory(logs);File.AppendAllText(Path.Combine(logs,name),message);
        }
    }
}
