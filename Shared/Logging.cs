using System;
using System.IO;
using System.Text;
using System.Threading;
using System.Collections.Generic;
using System.Globalization;

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
        private static readonly Dictionary<string,string> headers=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
        internal static string Clean(string value) {
            if(String.IsNullOrEmpty(value))return "unavailable";
            var text=new StringBuilder();foreach(char c in value){if(text.Length==256)break;text.Append(Char.IsControl(c)||c=='\u2028'||c=='\u2029'||c=='['||c==']'?' ':c);}return text.ToString();
        }
        internal static void SetAccountIdentity(string folder,string server,string account) {
            // This is a diagnostic snapshot, never a credential or login source.
            try {
                Guid parsed;if(!Guid.TryParse(server,out parsed)||!Guid.TryParse(account,out parsed))return;
                string file=Path.Combine(folder,"localserver/diagnostic-context.ini");
                Directory.CreateDirectory(Path.GetDirectoryName(file));
                headerUntil=DateTime.MinValue;
                ConfigFiles.Atomic(file,"[diagnostic]\nserver_id="+server+"\naccount_id="+account+"\nprocess_id="+System.Diagnostics.Process.GetCurrentProcess().Id+"\nprocess_started="+System.Diagnostics.Process.GetCurrentProcess().StartTime.ToUniversalTime().ToFileTimeUtc()+"\nplayer=unavailable\n");
            }catch(IOException){}catch(UnauthorizedAccessException){}
        }
        internal static string InstallationId(string folder,bool create=false) {
            try {
                string file=Path.Combine(folder,"localserver/diagnostic-client-id.txt");
                if(create)Directory.CreateDirectory(Path.GetDirectoryName(file));
                if(create&&!File.Exists(file))try{using(var stream=new FileStream(file,FileMode.CreateNew,FileAccess.Write,FileShare.Read)){byte[] bytes=Encoding.ASCII.GetBytes(Guid.NewGuid().ToString());stream.Write(bytes,0,bytes.Length);}}catch(IOException){}
                Guid id;if(new FileInfo(file).Length==36&&Guid.TryParse(File.ReadAllText(file),out id))return id.ToString();
            }catch(IOException){}catch(UnauthorizedAccessException){}
            return "unavailable";
        }
        private static string headerFolder,headerValue;
        private static DateTime headerUntil;
        internal static string Header(string folder,bool createClientId=false) {
            if(headerFolder==folder&&DateTime.UtcNow<headerUntil)return headerValue;
            string player="unavailable",account="unavailable",server="unavailable",version="unavailable",identity="last-known",owner="",started="";
            try {
                string path=Path.Combine(folder,"localserver/diagnostic-context.ini");
                if(File.Exists(path)&&new FileInfo(path).Length<8192)foreach(string raw in File.ReadAllLines(path)) {
                    int eq=raw.IndexOf('=');if(eq<1)continue;string key=raw.Substring(0,eq).Trim(),value=raw.Substring(eq+1).Trim();Guid id;
                    if(key=="player")player=Clean(value);
                    if(key=="account_id"&&Guid.TryParse(value,out id))account=id.ToString();
                    if(key=="server_id"&&Guid.TryParse(value,out id))server=id.ToString();
                    if(key=="process_id")owner=value;
                    if(key=="process_started")started=value;
                }
                path=Path.Combine(folder,"localserver/client-version.txt");if(File.Exists(path)&&new FileInfo(path).Length<256)version=Clean(File.ReadAllText(path).Trim());
            }catch(IOException){}catch(UnauthorizedAccessException){}
            if(owner==System.Diagnostics.Process.GetCurrentProcess().Id.ToString()&&started==System.Diagnostics.Process.GetCurrentProcess().StartTime.ToUniversalTime().ToFileTimeUtc().ToString())identity="current";
            headerFolder=folder;headerUntil=DateTime.UtcNow.AddSeconds(5);
            return headerValue="[diagnostic] client_id="+InstallationId(folder,createClientId)+"; player="+player+"; account_id="+account+"; server_id="+server+"; identity="+identity+"; installed_client="+version+"; platform="+Environment.OSVersion.Platform+"; process="+System.Diagnostics.Process.GetCurrentProcess().ProcessName+"; pid="+System.Diagnostics.Process.GetCurrentProcess().Id+"; bits="+(IntPtr.Size*8);
        }
        internal static string Stamp() { return "["+DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ",CultureInfo.InvariantCulture)+"] "; }
        internal static bool Stamped(string line) { return line.Length>=27&&line[0]=='['&&line[11]=='T'&&line[24]=='Z'&&line[25]==']'&&line[26]==' '; }
        internal static string Lines(string text,string component) {
            var result=new StringBuilder();
            using(var reader=new StringReader(text)) {string line;while((line=reader.ReadLine())!=null) {
                if(Stamped(line)&&component=="")result.Append(line);
                else result.Append(Stamp()).Append(component).Append(line);
                result.Append(Environment.NewLine);
            }}return result.ToString();
        }
        internal sealed class TimestampWriter : TextWriter {
            readonly TextWriter output;readonly StringBuilder pending=new StringBuilder();readonly object gate=new object();
            bool continuation;
            internal TimestampWriter(TextWriter output) {this.output=output;}
            public override Encoding Encoding {get{return output.Encoding;}}
            void Emit(bool newline) {
                string text=pending.ToString().TrimEnd('\r');pending.Length=0;
                if(!continuation&&!Stamped(text))output.Write(Stamp());
                output.Write(text);if(newline)output.WriteLine();continuation=!newline;
            }
            public override void Write(char value) {lock(gate){if(value=='\n')Emit(true);else {pending.Append(value);if(pending.Length>=65536)Emit(false);}}}
            public override void Write(string value) {if(value==null)return;lock(gate){foreach(char c in value)Write(c);}}
            public override void Flush() {lock(gate){if(pending.Length>0)Emit(false);output.Flush();}}
            protected override void Dispose(bool disposing) {if(disposing)Flush();base.Dispose(disposing);}
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
                    message=Lines(message,target!=name?"["+Category(name)+"] ":"");
                    string file=Path.Combine(logs,target),header=Header(folder,true),previous;
                    if(!headers.TryGetValue(file,out previous)||previous!=header||!File.Exists(file)||new FileInfo(file).Length==0){message=Lines(header,"")+message;headers[file]=header;}
                    File.AppendAllText(file,message,new UTF8Encoding(false));
                }finally{if(acquired)mutex.ReleaseMutex();}
            }
        }
    }
}
