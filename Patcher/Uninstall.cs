using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Text.RegularExpressions;

internal static class Uninstall
{
    const string ReceiptPath="localserver/uninstall-state.json";
    [DataContract] internal sealed class Receipt {
        [DataMember] public int schema=1;
        [DataMember] public string backup_dir="";
        [DataMember] public Dictionary<string,string> files=new Dictionary<string,string>();
        [DataMember] public Dictionary<string,string> originals=new Dictionary<string,string>();
    }
    [DataContract] sealed class Component {
        [DataMember] public Dictionary<string,string> files=new Dictionary<string,string>();
    }
    static T Read<T>(string path) {using(var f=File.OpenRead(path))return (T)new DataContractJsonSerializer(typeof(T),new DataContractJsonSerializerSettings{UseSimpleDictionaryFormat=true}).ReadObject(f);}
    static string Hash(string path){return LanguageComponent.Hash(File.ReadAllBytes(path));}
    static string Safe(string root,string name) {
        if(string.IsNullOrEmpty(name)||name.Contains("\\")||name.StartsWith("/")||name.Split('/').Any(p=>p=="."||p==".."||p==""||p.Contains(":")))throw new IOException("Unsafe uninstall path: "+name);
        root=Path.GetFullPath(root);string current=root;
        if((File.GetAttributes(root)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked game folder retained.");
        foreach(string part in name.Split('/')){current=Path.Combine(current,part);if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked path retained: "+name);}
        return current;
    }
    internal static bool Managed(string name) {
        // Positive allow-list: saves, INIs, localserver and unrelated mods can
        // never become deletion targets, even through an invalid receipt.
        if(name.EndsWith(".ini",StringComparison.OrdinalIgnoreCase))return false;
        return Regex.IsMatch(name,@"^ReRiff[.-][A-Za-z0-9_.-]+\.(dll|exe|sh)$",RegexOptions.IgnoreCase)
            || Regex.IsMatch(name,@"^(mods/(reriff-assets|noto-cjk-font|noto-latin-font)/|fonts/noto-(cjk|latin)/)[A-Za-z0-9_./-]+$")
            || Regex.IsMatch(name,@"^(mods/)?vfs[1-9a-f][0-9a-f]?\.(dat|zip)$");
    }
    internal static byte[] MakeReceipt(string game,Dictionary<string,byte[]> plan,Dictionary<string,string> originals,string backups) {
        string prior=Safe(game,ReceiptPath);Receipt receipt=File.Exists(prior)?Read<Receipt>(prior):new Receipt();
        if(receipt.schema!=1||receipt.files==null)throw new IOException("Invalid uninstall ownership record.");
        receipt.backup_dir=backups;receipt.originals=originals;
        foreach(var pair in plan)if(Managed(pair.Key)||originals.ContainsKey(pair.Key)) {
            if(pair.Value==null)receipt.files.Remove(pair.Key);else receipt.files[pair.Key]=LanguageComponent.Hash(pair.Value);
        }
        foreach(var pair in originals)if(!plan.ContainsKey(pair.Key)&&File.Exists(Safe(game,pair.Key)))receipt.files[pair.Key]=Hash(Safe(game,pair.Key));
        using(var m=new MemoryStream()){new DataContractJsonSerializer(typeof(Receipt),new DataContractJsonSerializerSettings{UseSimpleDictionaryFormat=true}).WriteObject(m,receipt);return m.ToArray();}
    }
    internal static void Apply(string game,bool dryRun) {
        game=Path.GetFullPath(game);Safe(game,"Game.exe");
        var receipt=Read<Receipt>(Safe(game,ReceiptPath));
        if(receipt.schema!=1||receipt.files==null||receipt.originals==null||receipt.originals.Count!=2)throw new IOException("Missing or unsupported uninstall record. Reinstall the current ReRiff package first.");
        // Fixed vanilla reference: a receipt cannot authorize restoring arbitrary
        // paths or replacing the original executable.
        var vanilla=new Dictionary<string,string>{
            {"ParseOctane.dll","d661606db8658adb54d5be18b3e3a3a6f07c8e738f2d78556ece42355f8e62e4"},
            {"Parse.NetFx45.dll","4e2f65147faec13ae5333337e3c7cd9e25028f3119fe817e059b1d2eab678522"}};
        foreach(var v in vanilla)if(!receipt.originals.ContainsKey(v.Key)||receipt.originals[v.Key]!=v.Value)throw new IOException("Unrecognized original DLL reference.");
        var componentRecords=new List<string>();
        foreach(string name in new[]{"localserver/artwork/core.json","localserver/languages/cjk.json","localserver/languages/latin.json"}) {
            string path=Safe(game,name);if(!File.Exists(path))continue;
            componentRecords.Add(name);
            var component=Read<Component>(path);if(component.files==null)throw new IOException("Invalid component ownership: "+name);
            foreach(var item in component.files){if(!Managed(item.Key))throw new IOException("Unsupported owned component path: "+item.Key);receipt.files[item.Key]=item.Value;}
        }
        var restore=new Dictionary<string,byte[]>();var remove=new List<string>();
        foreach(var v in vanilla) {
            string target=Safe(game,v.Key);
            if(File.Exists(target)&&Hash(target)==v.Value)continue;
            string recorded;if(File.Exists(target)&&(!receipt.files.TryGetValue(v.Key,out recorded)||Hash(target)!=recorded))throw new IOException("Modified game DLL retained: "+v.Key);
            string backup=Safe(Path.GetFullPath(receipt.backup_dir),"originals/"+v.Key);
            if(!File.Exists(backup)||Hash(backup)!=v.Value)throw new IOException("Verified original backup missing: "+v.Key);
            restore[v.Key]=File.ReadAllBytes(backup);
        }
        foreach(var item in receipt.files) {
            if(vanilla.ContainsKey(item.Key))continue;
            if(!Managed(item.Key))throw new IOException("Protected path in uninstall record: "+item.Key);
            string target=Safe(game,item.Key);
            if(Directory.Exists(target))throw new IOException("Directory retained: "+item.Key);
            if(!File.Exists(target))continue;
            if(Hash(target)!=item.Value)throw new IOException("Modified ReRiff file retained: "+item.Key);
            remove.Add(item.Key);
        }
        // Retire only install ownership metadata, never localserver user data.
        // Otherwise a later reinstall mistakes removed fonts for modified files.
        remove.AddRange(componentRecords);
        // Detect game instances before changing anything. Native Linux hosts see
        // Wine's executable in /proc; Windows uses the normal process name.
        foreach(var p in System.Diagnostics.Process.GetProcesses())using(p){try{if(p.ProcessName.Equals("Game",StringComparison.OrdinalIgnoreCase)||p.ProcessName.Equals("Game.exe",StringComparison.OrdinalIgnoreCase))throw new IOException("Close Riff Racer before uninstalling.");}catch(InvalidOperationException){}}
        if(dryRun){Console.WriteLine("Validated uninstall: "+restore.Count+" DLL restorations, "+remove.Count+" owned files.");return;}
        string quarantine=Safe(game,".bak/uninstall-"+DateTime.UtcNow.ToString("yyyyMMddTHHmmss")+"-"+Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(quarantine);
        var moved=new List<string>();var written=new List<string>();
        try {
            foreach(string name in remove.Concat(restore.Keys)) {
                string source=Safe(game,name);if(!File.Exists(source))continue;
                string destination=Safe(quarantine,name);Directory.CreateDirectory(Path.GetDirectoryName(destination));
                try{File.Move(source,destination);moved.Add(name);}
                catch {
                    // Some runtimes fall back to copy+unlink and may leave a
                    // destination behind when unlinking a read-only parent fails.
                    if(File.Exists(destination)) {
                        if(!File.Exists(source))moved.Add(name);
                        else if(Hash(source)==Hash(destination))File.Delete(destination);
                    }
                    throw;
                }
            }
            foreach(var item in restore){string target=Safe(game,item.Key);using(var f=new FileStream(target,FileMode.CreateNew,FileAccess.Write,FileShare.None)){written.Add(item.Key);f.Write(item.Value,0,item.Value.Length);}}
        } catch {
            foreach(string name in written)File.Delete(Safe(game,name));
            foreach(string name in moved.AsEnumerable().Reverse())File.Move(Safe(quarantine,name),Safe(game,name));
            throw;
        }
        foreach(string name in remove) {
            string parent=Path.GetDirectoryName(Safe(game,name));
            try{while(parent!=game&&Directory.Exists(parent)&&!Directory.EnumerateFileSystemEntries(parent).Any()){Directory.Delete(parent);parent=Path.GetDirectoryName(parent);}}catch(IOException){}catch(UnauthorizedAccessException){}
        }
        Console.WriteLine("ReRiff removed. User data, localserver, singleplayer saves, INIs, logs and backups preserved. Remove ReRiff from Steam launch options. Recovery files: "+quarantine);
    }
    static void Result(string path,string text){string temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";File.WriteAllText(temp,text,Encoding.Unicode);File.Move(temp,path);}
    internal static int Run(string[] args) {
        string game=null,result=null;bool dry=false;
        try {
            for(int i=0;i<args.Length;i++){if(args[i]=="--dry-run")dry=true;else if(args[i]=="--game-dir"&&i+1<args.Length)game=args[++i];else if(args[i]=="--result"&&i+1<args.Length)result=args[++i];else throw new ArgumentException("Unknown uninstall option.");}
            if(game==null)throw new ArgumentException("--game-dir required.");
            Apply(game,dry);
            if(result!=null)Result(result,"[result]\nok=true\nmessage=ReRiff removed. User data and settings kept. Remove ReRiff from Steam launch options.\n");
            return 0;
        } catch(Exception error){Console.Error.WriteLine(error.Message);if(result!=null)Result(result,"[result]\nok=false\nmessage="+error.Message.Replace('\r',' ').Replace('\n',' ')+"\n");return 1;}
    }
}
