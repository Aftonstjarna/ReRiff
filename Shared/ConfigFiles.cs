using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Linq;
using System.Security.Cryptography;

namespace ReRiff {
    // Section routing keeps old installs readable until migration completes.
    internal static class ConfigFiles {
        internal static string Name(string section) {
            switch(section.ToLowerInvariant()) {
                case "lastfm":case "graphics":case "languages":case "launcher":case "diagnostics":return "client.ini";
                case "progression":return "singleplayer.ini";
                default:return "server.ini";
            }
        }
        internal static string ReadPath(string folder,string section) {
            string preferred=Path.Combine(folder,Name(section));
            return File.Exists(preferred)?preferred:Path.Combine(folder,"server.ini");
        }
        internal static Dictionary<string,string> Read(string folder,string section) {
            var result=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
            foreach(string name in new[]{"server.ini",Name(section)}.Distinct()) {
                string path=Path.Combine(folder,name);if(!File.Exists(path))continue;
                bool active=false;
                foreach(string raw in File.ReadAllLines(path)) {
                    string line=raw.Trim();if(line.StartsWith("[")&&line.EndsWith("]")){active=line.Substring(1,line.Length-2).Trim().Equals(section,StringComparison.OrdinalIgnoreCase);continue;}
                    if(!active||line.StartsWith(";")||line.StartsWith("#"))continue;
                    int eq=line.IndexOf('=');if(eq>0)result[line.Substring(0,eq).Trim()]=line.Substring(eq+1).Trim();
                }
            }
            return result;
        }
        internal static void Atomic(string path,string text) {
            string temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try {File.WriteAllText(temp,text,new UnicodeEncoding(false,true));if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);}
            finally{if(File.Exists(temp))File.Delete(temp);}
        }
        private static List<string> Merge(IEnumerable<string> source,List<string> merged) {
                string section="";
                foreach(string raw in source) {
                    string line=raw.Trim();
                    if(line.StartsWith("[")&&line.EndsWith("]")) {
                        section=line.Substring(1,line.Length-2).Trim();
                        if(!merged.Any(x=>x.Trim().Equals(line,StringComparison.OrdinalIgnoreCase)))merged.Add(raw);
                        continue;
                    }
                    int eq=line.IndexOf('=');if(eq<=0||line.StartsWith(";")||line.StartsWith("#"))continue;
                    string key=line.Substring(0,eq).Trim();int at=-1,end=merged.Count;bool active=false,found=false;
                    for(int i=0;i<merged.Count;i++) {
                        string item=merged[i].Trim();
                        if(item.StartsWith("[")) {
                            if(active){end=i;break;}
                            active=item.Equals("["+section+"]",StringComparison.OrdinalIgnoreCase);if(active)at=i;
                        }else if(active){int e=item.IndexOf('=');if(e>0&&item.Substring(0,e).Trim().Equals(key,StringComparison.OrdinalIgnoreCase))found=true;}
                    }
                    if(!found&&at>=0)merged.Insert(end,raw);
                }
            return merged;
        }
        internal static string Document(string folder,string name) {
            string legacy=Path.Combine(folder,"server.ini"),path=Path.Combine(folder,name);var source=new List<string>();bool active=false;
            if(File.Exists(legacy))foreach(string raw in File.ReadAllLines(legacy)) {
                string line=raw.Trim();if(line.StartsWith("[")&&line.EndsWith("]"))active=Name(line.Substring(1,line.Length-2).Trim())==name;
                if(active)source.Add(raw);
            }
            var merged=Merge(source,File.Exists(path)?File.ReadAllLines(path).ToList():new List<string>());
            return String.Join(Environment.NewLine,merged)+Environment.NewLine;
        }
        private static void Backup(string folder,string name) {
            string backups=Path.Combine(folder,".bak/configs");Directory.CreateDirectory(backups);
            byte[] raw=File.ReadAllBytes(Path.Combine(folder,name));string hash;
            using(var sha=SHA256.Create())hash=BitConverter.ToString(sha.ComputeHash(raw)).Replace("-","").ToLowerInvariant();
            string saved=Path.Combine(backups,hash+"-"+name);if(!File.Exists(saved))File.WriteAllBytes(saved,raw);File.SetLastWriteTimeUtc(saved,DateTime.UtcNow);
            var owned=Directory.GetFiles(backups,"*-"+name).Where(path=>{
                if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)return false;
                string label=Path.GetFileName(path);if(label.Length!=65+name.Length)return false;
                using(var sha=SHA256.Create())return label.Substring(0,64)==BitConverter.ToString(sha.ComputeHash(File.ReadAllBytes(path))).Replace("-","").ToLowerInvariant();
            }).OrderByDescending(File.GetLastWriteTimeUtc).ThenBy(path=>path==saved?0:1).Skip(2).ToArray();
            foreach(string old in owned)File.Delete(old);
        }
        internal static void Migrate(string folder) {
            string legacy=Path.Combine(folder,"server.ini");if(!File.Exists(legacy))return;
            string[] lines=File.ReadAllLines(legacy);var keep=new List<string>();var groups=new Dictionary<string,List<string>>();string target="server.ini";
            foreach(string raw in lines) {
                string line=raw.Trim();if(line.StartsWith("[")&&line.EndsWith("]"))target=Name(line.Substring(1,line.Length-2).Trim());
                if(target=="server.ini")keep.Add(raw);else{if(!groups.ContainsKey(target))groups[target]=new List<string>();groups[target].Add(raw);}
            }
            if(groups.Count==0)return;
            Backup(folder,"server.ini");
            foreach(var group in groups) {
                string path=Path.Combine(folder,group.Key);
                string text=String.Join(Environment.NewLine,Merge(group.Value,File.Exists(path)?File.ReadAllLines(path).ToList():new List<string>()))+Environment.NewLine;
                if(File.Exists(path))Backup(folder,group.Key);
                Atomic(path,text);
            }
            // Remove old sections only after every destination is safely written.
            Atomic(legacy,String.Join(Environment.NewLine,keep)+Environment.NewLine);
        }
    }
}
