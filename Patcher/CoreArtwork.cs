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
    const string StatePath="localserver/artwork/core.json";
    [DataContract] internal sealed class State {
        [DataMember] public int schema=1;
        [DataMember] public string stem="";
        [DataMember] public Dictionary<string,string> files=new Dictionary<string,string>();
    }
    static string Safe(string root,string name) {
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
        var occupied=new HashSet<string>(reserved,StringComparer.OrdinalIgnoreCase);
        // The engine treats padded and upper-case names as the same volume.
        foreach(string path in Directory.GetFileSystemEntries(game)) {
            string name=Path.GetFileName(path);var match=Regex.Match(name,@"^vfs([0-9a-f]+)\.(dat|zip)$",RegexOptions.IgnoreCase);
            int id;if(match.Success&&int.TryParse(match.Groups[1].Value,NumberStyles.HexNumber,CultureInfo.InvariantCulture,out id)&&id>0&&id<=31) {
                string normalized="vfs"+id.ToString("x")+"."+match.Groups[2].Value.ToLowerInvariant();
                if(name!=normalized)occupied.Add(normalized);
            }
        }
        string stateFile=Safe(game,StatePath),stem="";
        if(File.Exists(stateFile)) {
            State previous;using(var stream=File.OpenRead(stateFile))previous=(State)Serializer().ReadObject(stream);
            if(previous==null||previous.schema!=1||!Stem(previous.stem)||previous.files==null||previous.files.Count!=2)throw new IOException("Invalid core artwork ownership record.");
            stem=previous.stem;
            foreach(string suffix in new[]{".dat",".zip"}) {
                string name=stem+suffix,path=Safe(game,name);string expected;
                if(!previous.files.TryGetValue(name,out expected)||!File.Exists(path)||LanguageComponent.Hash(File.ReadAllBytes(path))!=expected)throw new IOException("Owned artwork changed or missing: "+name);
            }
        } else {
            // Adopt an identical prototype pair only; never replace an unowned file.
            var candidates=new[]{2}.Concat(Enumerable.Range(3,29)).Concat(new[]{1}).Select(n=>"vfs"+n.ToString("x")).ToArray();
            foreach(string candidate in candidates) {
                string a=Safe(game,candidate+".dat"),b=Safe(game,candidate+".zip");
                if(occupied.Contains(candidate+".dat")||occupied.Contains(candidate+".zip"))continue;
                if(File.Exists(a)&&File.Exists(b)&&LanguageComponent.Hash(File.ReadAllBytes(a))==datHash&&LanguageComponent.Hash(File.ReadAllBytes(b))==zipHash){stem=candidate;break;}
            }
            if(stem=="")foreach(string candidate in candidates) {
                string a=Safe(game,candidate+".dat"),b=Safe(game,candidate+".zip");
                if(!occupied.Contains(candidate+".dat")&&!occupied.Contains(candidate+".zip")&&!File.Exists(a)&&!File.Exists(b)&&!Directory.Exists(a)&&!Directory.Exists(b)){stem=candidate;break;}
            }
            if(stem=="")throw new IOException("No free native volume for ReRiff artwork.");
        }
        if(occupied.Contains(stem+".dat")||occupied.Contains(stem+".zip"))throw new IOException("Artwork conflicts with another planned component.");
        var state=new State{stem=stem};state.files[stem+".dat"]=datHash;state.files[stem+".zip"]=zipHash;
        byte[] record;using(var stream=new MemoryStream()){Serializer().WriteObject(stream,state);record=stream.ToArray();}
        return new Dictionary<string,byte[]>{{stem+".dat",index},{stem+".zip",archive},{StatePath,record}};
    }
    internal static int Run(string[] args) {
        try {
            var options=new Dictionary<string,string>();for(int i=0;i<args.Length;i+=2){if(i+1==args.Length)throw new ArgumentException("Expected option value.");options.Add(args[i],args[i+1]);}
            string reserved;options.TryGetValue("--reserved",out reserved);
            var plan=Plan(options["--game-dir"],options["--dat"],options["--dat-sha256"],options["--zip"],options["--zip-sha256"],(reserved??"").Split(','));
            foreach(var item in plan){string path=Safe(options["--stage"],item.Key);Directory.CreateDirectory(Path.GetDirectoryName(path));File.WriteAllBytes(path,item.Value);}return 0;
        }catch(Exception error){Console.Error.WriteLine("Core artwork preflight failed: "+error.Message);return 1;}
    }
}
