// Archive names in component ownership records are logical VFS names. The
// marker switches their physical directory without changing IDs or hashes.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Globalization;
using System.Runtime.Serialization.Json;
internal static class ModsVfs {
    internal const string Marker="mods/.reriff-vfs";
    internal static void DisabledFolder(string game,Dictionary<string,byte[]> plan) {
        const string marker="mods/.disabled/README.txt";
        Safe(game,marker);
        if(!File.Exists(Path.Combine(game,marker)))plan[marker]=Encoding.UTF8.GetBytes("Mods in this folder are not loaded. Move a mod folder back into mods to enable it. Use the launcher's Languages checkboxes for font packs.\n");
    }
    static bool Archive(string name) {return Regex.IsMatch(name,@"^vfs[0-9a-f]+\.(dat|zip)$",RegexOptions.IgnoreCase);}
    internal static string Resolve(string game,string name) {
        return Archive(name)&&File.Exists(Path.Combine(game,Marker))?"mods/"+name:name;
    }
    internal static IEnumerable<string> Files(string game) {
        return Directory.GetFileSystemEntries(game).Concat(Directory.Exists(Path.Combine(game,"mods"))?Directory.GetFileSystemEntries(Path.Combine(game,"mods")):new string[0]);
    }
    static void Safe(string game,string name) {
        for(string p=Path.Combine(game,name);p!=null&&p!=Path.GetDirectoryName(Path.GetFullPath(game));p=Path.GetDirectoryName(p))
            if((File.Exists(p)||Directory.Exists(p))&&(File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("VFS migration refuses linked path: "+name);
    }
    internal static Dictionary<string,byte[]> Plan(string game,Dictionary<string,byte[]> incoming) {
        Safe(game,"mods");var result=new Dictionary<string,byte[]>(incoming,StringComparer.OrdinalIgnoreCase);
        var found=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
        var ids=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
        foreach(string path in Files(game)) {
            string name=Path.GetFileName(path);bool inMods=Path.GetDirectoryName(Path.GetFullPath(path))==Path.GetFullPath(Path.Combine(game,"mods"));
            if(!Regex.IsMatch(name,@"^vfs.*\.(dat|zip)$",RegexOptions.IgnoreCase))continue;
            if(name.Equals("vfs.dat",StringComparison.OrdinalIgnoreCase)||name.Equals("vfs.zip",StringComparison.OrdinalIgnoreCase)){
                if(inMods)throw new IOException("Stock VFS names are not allowed in mods.");continue;
            }
            var m=Regex.Match(name,@"^vfs([0-9a-f]+)\.(dat|zip)$",RegexOptions.IgnoreCase);int id;
            if(!m.Success||!int.TryParse(m.Groups[1].Value,NumberStyles.HexNumber,CultureInfo.InvariantCulture,out id)||id<1||id>31)
                throw new IOException("Unsupported mod archive name retained: "+name);
            string key=id.ToString("x")+"."+m.Groups[2].Value.ToLowerInvariant(),previous;
            if(ids.TryGetValue(key,out previous)&&previous!=name)throw new IOException("Conflicting VFS volume aliases: "+previous+" / "+name);
            ids[key]=name;string relative=(inMods?"mods/":"")+name;Safe(game,relative);
            if(!File.Exists(path))throw new IOException("Expected archive file: "+relative);
            byte[] data=File.ReadAllBytes(path),other;
            if(found.TryGetValue(name,out other)&&!data.SequenceEqual(other))throw new IOException("Root/mods archive conflict: "+name);
            found[name]=data;
            if(!inMods)result[name]=null;
        }
        foreach(var item in found)if(!incoming.ContainsKey("mods/"+item.Key))result["mods/"+item.Key]=item.Value;
        foreach(var item in incoming)if(Archive(item.Key)){result["mods/"+item.Key]=item.Value;if(File.Exists(Path.Combine(game,item.Key)))result[item.Key]=null;else result.Remove(item.Key);}
        var final=new HashSet<string>(found.Keys,StringComparer.OrdinalIgnoreCase);
        foreach(var item in result.Where(p=>p.Key.StartsWith("mods/",StringComparison.OrdinalIgnoreCase)&&Archive(Path.GetFileName(p.Key)))){
            string name=Path.GetFileName(item.Key);if(item.Value==null)final.Remove(name);else final.Add(name);
        }
        var finalIds=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach(string name in final){
            var m=Regex.Match(name,@"^vfs([0-9a-f]+)\.(dat|zip)$",RegexOptions.IgnoreCase);int id;
            if(!m.Success||!int.TryParse(m.Groups[1].Value,NumberStyles.HexNumber,CultureInfo.InvariantCulture,out id)||id<1||id>31||!finalIds.Add(id.ToString("x")+"."+m.Groups[2].Value))throw new IOException("Conflicting or unsupported planned VFS name: "+name);
        }
        foreach(string name in final){string pair=Path.ChangeExtension(name,name.EndsWith(".dat",StringComparison.OrdinalIgnoreCase)?"zip":"dat");if(!final.Contains(pair))throw new IOException("Incomplete mod archive pair: "+name);}
        DisabledFolder(game,result);
        result[Marker]=Encoding.UTF8.GetBytes("ReRiff VFS directory version 1\n");return result;
    }
    internal static int Run(string[] args){
        try {
            if(args.Length!=4||args[0]!="--game-dir"||args[2]!="--stage")throw new ArgumentException("Expected game directory and private staging directory.");
            string game=args[1],stage=args[3];var input=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
            string removed=Path.Combine(stage,".reriff-mods-removals.json");
            if(File.Exists(removed))using(var stream=File.OpenRead(removed))foreach(var name in (string[])new DataContractJsonSerializer(typeof(string[])).ReadObject(stream))input[name]=null;
            foreach(string file in Directory.GetFiles(stage,"*",SearchOption.AllDirectories))if(file!=removed)input[file.Substring(stage.Length+1).Replace('\\','/')]=File.ReadAllBytes(file);
            var plan=Plan(game,input);
            foreach(string file in Directory.GetFiles(stage,"*",SearchOption.AllDirectories))File.Delete(file);
            foreach(var item in plan)if(item.Value!=null){string file=Path.Combine(stage,item.Key);Directory.CreateDirectory(Path.GetDirectoryName(file));File.WriteAllBytes(file,item.Value);}
            using(var stream=File.Create(removed))new DataContractJsonSerializer(typeof(string[])).WriteObject(stream,plan.Where(p=>p.Value==null).Select(p=>p.Key).ToArray());
            return 0;
        }catch(Exception error){Console.Error.WriteLine("Mods migration refused: "+error.Message);return 1;}
    }
}
