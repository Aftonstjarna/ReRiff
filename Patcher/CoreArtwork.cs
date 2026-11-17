// Owned ReRiff artwork volumes. Planning is read-only; setup commits the pair
// and its ownership record through its existing transactional install path.
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text.RegularExpressions;
using System.Globalization;

internal static class CoreArtwork
{
    const string ModStem="mods/reriff-assets/vfs";
    const string StatePath="localserver/artwork/core.json";
    [DataContract] internal sealed class State {
        [DataMember] public int schema=1;
        [DataMember] public string stem="";
        [DataMember] public Dictionary<string,string> files=new Dictionary<string,string>();
    }
    static string Safe(string root,string name) { name=ModsVfs.Resolve(root,name);
        string full=Path.GetFullPath(Path.Combine(root,name));root=Path.GetFullPath(root);
        if(!full.StartsWith(root+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new IOException("Unsafe artwork path.");
        for(string part=full;part!=null;part=Path.GetDirectoryName(part)) {
            if((File.Exists(part)||Directory.Exists(part))&&(File.GetAttributes(part)&FileAttributes.ReparsePoint)!=0)throw new IOException("Artwork path is a link: "+part);
            if(part==root)break;
        }
        return full;
    }
    static DataContractJsonSerializer Serializer() { return new DataContractJsonSerializer(typeof(State),new DataContractJsonSerializerSettings{UseSimpleDictionaryFormat=true}); }
    static bool Stem(string stem) { return Enumerable.Range(1,31).Any(n=>stem=="vfs"+n.ToString("x")); }
    internal static Dictionary<string,byte[]> Plan(string game,string dat,string datHash,string zip,string zipHash,IEnumerable<string> reserved) {
        byte[] index=File.ReadAllBytes(dat),archive=File.ReadAllBytes(zip);
        if(index.Length>1048576||archive.Length>1048576||LanguageComponent.Hash(index)!=datHash||LanguageComponent.Hash(archive)!=zipHash)throw new IOException("Core artwork integrity check failed.");
        using(var stream=new MemoryStream(archive))using(var source=new ZipArchive(stream,ZipArchiveMode.Read)) {
            if(source.Entries.Count!=1||source.Entries[0].FullName!="graphics/reriff/achievements/batchtextures.brres"||source.Entries[0].Length>1048576)throw new IOException("Unexpected core artwork contents.");
        }
        var plan=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
        var occupied=new HashSet<string>(reserved,StringComparer.OrdinalIgnoreCase);
        var owned=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        string stateFile=Safe(game,StatePath);State previous=null;
        if(File.Exists(stateFile)) {
            using(var stream=File.OpenRead(stateFile))previous=(State)Serializer().ReadObject(stream);
            if(previous==null||previous.files==null||previous.files.Count!=2||
                !(previous.schema==1&&Stem(previous.stem)||previous.schema==2&&previous.stem==ModStem))throw new IOException("Invalid core artwork ownership record.");
            foreach(string suffix in new[]{".dat",".zip"}) {
                string name=previous.stem+suffix,path=Safe(game,name);string expected;
                if(!previous.files.TryGetValue(name,out expected))throw new IOException("Incomplete artwork ownership record.");
                // Accept a manually moved pair only when its recorded hashes still match.
                if(!File.Exists(path)&&previous.schema==1)path=Safe(game,ModStem+suffix);
                if(!File.Exists(path)||LanguageComponent.Hash(File.ReadAllBytes(path))!=expected)throw new IOException("Owned artwork changed or missing: "+name);
                string relative=path.Substring(Path.GetFullPath(game).Length+1).Replace('\\','/');owned.Add(relative);
                if(relative!=ModStem+suffix)plan[relative]=null;
            }
        }else {
            // Retire only exact published pairs; unrelated numbered mods stay untouched.
            foreach(int id in Enumerable.Range(1,31)) {
                string stem="vfs"+id.ToString("x"),a=Safe(game,stem+".dat"),b=Safe(game,stem+".zip");
                if(occupied.Contains(stem+".dat")||occupied.Contains(stem+".zip"))continue;
                if(File.Exists(a)&&File.Exists(b)&&LanguageComponent.Hash(File.ReadAllBytes(a))==datHash&&LanguageComponent.Hash(File.ReadAllBytes(b))==zipHash) {
                    plan[a.Substring(Path.GetFullPath(game).Length+1).Replace('\\','/')]=null;
                    plan[b.Substring(Path.GetFullPath(game).Length+1).Replace('\\','/')]=null;
                }
            }
        }
        foreach(var file in new[]{new KeyValuePair<string,byte[]>(ModStem+".dat",index),new KeyValuePair<string,byte[]>(ModStem+".zip",archive)}) {
            string path=Safe(game,file.Key);
            if(occupied.Contains(file.Key))throw new IOException("Artwork conflicts with another planned component.");
            if(Directory.Exists(path)||File.Exists(path)&&!owned.Contains(file.Key)&&LanguageComponent.Hash(File.ReadAllBytes(path))!=LanguageComponent.Hash(file.Value))throw new IOException("Unowned artwork retained: "+file.Key);
            plan[file.Key]=file.Value;
        }
        var state=new State{schema=2,stem=ModStem};state.files[ModStem+".dat"]=datHash;state.files[ModStem+".zip"]=zipHash;
        byte[] record;using(var stream=new MemoryStream()){Serializer().WriteObject(stream,state);record=stream.ToArray();}
        plan[StatePath]=record;return plan;
    }
    internal static int Run(string[] args) {
        try {
            var options=new Dictionary<string,string>();for(int i=0;i<args.Length;i+=2){if(i+1==args.Length)throw new ArgumentException("Expected option value.");options.Add(args[i],args[i+1]);}
            string reserved;options.TryGetValue("--reserved",out reserved);
            var plan=Plan(options["--game-dir"],options["--dat"],options["--dat-sha256"],options["--zip"],options["--zip-sha256"],(reserved??"").Split(','));
            var removed=new Dictionary<string,string>();
            foreach(var item in plan){
                if(item.Value==null){removed[item.Key]=LanguageComponent.Hash(File.ReadAllBytes(Safe(options["--game-dir"],item.Key)));continue;}
                string path=Safe(options["--stage"],item.Key);Directory.CreateDirectory(Path.GetDirectoryName(path));File.WriteAllBytes(path,item.Value);
            }
            if(removed.Count!=0)using(var stream=File.Create(Safe(options["--stage"],".reriff-artwork-removals.json")))new DataContractJsonSerializer(typeof(Dictionary<string,string>),new DataContractJsonSerializerSettings{UseSimpleDictionaryFormat=true}).WriteObject(stream,removed);
            return 0;
        }catch(Exception error){Console.Error.WriteLine("Core artwork preflight failed: "+error.Message);return 1;}
    }
}
