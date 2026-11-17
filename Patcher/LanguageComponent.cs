// Shared optional-font lifecycle for Windows setup/launcher and the native Linux host.
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

internal static class LanguageComponent
{
    static string StateFor(string pack) { if(pack!="cjk"&&pack!="latin")throw new ArgumentException("Unknown font pack.");return "localserver/languages/"+pack+".json"; }
    [DataContract]
    internal sealed class Package
    {
        [DataMember] public int schema = 1;
        [DataMember] public string version = "";
        [DataMember] public Dictionary<string, string> files = new Dictionary<string, string>();
        [DataMember] public Dictionary<string, string[]> legacy = new Dictionary<string, string[]>();
    }
    [DataContract]
    internal sealed class Installed
    {
        [DataMember] public int schema = 1;
        [DataMember] public string version = "";
        [DataMember] public Dictionary<string, string> files = new Dictionary<string, string>();
        [DataMember] public Dictionary<string, string> archives = new Dictionary<string, string>();
    }
    static T Read<T>(byte[] bytes) { using (var stream = new MemoryStream(bytes)) return (T)new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { UseSimpleDictionaryFormat = true }).ReadObject(stream); }
    static byte[] Json<T>(T value) { using (var stream = new MemoryStream()) { new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { UseSimpleDictionaryFormat = true }).WriteObject(stream, value); return stream.ToArray(); } }
    internal static string Hash(byte[] data) { using (var hash = SHA256.Create()) return string.Concat(hash.ComputeHash(data).Select(b => b.ToString("x2"))); }
    internal static bool Development(string game) {
        string path=Under(game,"localserver/client-channel.ini");
        return File.Exists(path)&&Regex.IsMatch(File.ReadAllText(path),@"(?im)^\s*channel\s*=\s*experimental\s*$");
    }
    internal static void PruneConfigHistory(string backups,string latest=null) {
        string folder=Under(backups,"configs");if(!Directory.Exists(folder))return;
        var groups=new Dictionary<string,List<string>>(StringComparer.OrdinalIgnoreCase);
        foreach(string path in Directory.GetFiles(folder)) {
            if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)continue;
            var match=Regex.Match(Path.GetFileName(path),@"^([0-9a-f]{64})-(.+)$");if(!match.Success||Hash(File.ReadAllBytes(path))!=match.Groups[1].Value)continue;
            string name=match.Groups[2].Value;if(!groups.ContainsKey(name))groups[name]=new List<string>();groups[name].Add(path);
        }
        foreach(var group in groups.Values)foreach(string path in group.OrderByDescending(p=>p==latest).ThenByDescending(File.GetLastWriteTimeUtc).ThenByDescending(p=>p,StringComparer.Ordinal).Skip(2))File.Delete(path);
    }
    internal static void BackupPrevious(string backups,string name,byte[] data,bool development,DateTime? modified=null) {
        string extension=Path.GetExtension(name).ToLowerInvariant();
        bool config=new[]{".ini",".cfg",".conf",".toml",".config",".json",".xml",".yaml",".yml"}.Contains(extension);
        if(!config&&!development)return;
        string path=Under(backups,(config?"configs/":"versions/")+Hash(data)+"-"+Path.GetFileName(name));DateTime previous=File.Exists(path)?File.GetLastWriteTimeUtc(path):DateTime.MinValue;if(!File.Exists(path))Atomic(path,data);
        if(config) {DateTime stamp=modified??DateTime.UtcNow;if(modified.HasValue&&previous>stamp)stamp=previous;File.SetLastWriteTimeUtc(path,stamp);PruneConfigHistory(backups,modified.HasValue?null:path);}
    }
    internal static void MigrateConfigHistory(string backups) {
        string versions=Under(backups,"versions");
        if(Directory.Exists(versions)) {
            foreach(string path in Directory.GetFiles(versions)) {
                if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)continue;
                var match=Regex.Match(Path.GetFileName(path),@"^([0-9a-f]{64})-(.+\.(ini|cfg|conf|toml|config|json|xml|yaml|yml))$",RegexOptions.IgnoreCase);if(!match.Success)continue;
                byte[] raw=File.ReadAllBytes(path);if(Hash(raw)!=match.Groups[1].Value.ToLowerInvariant())continue;
                BackupPrevious(backups,match.Groups[2].Value,raw,false,File.GetLastWriteTimeUtc(path));File.Delete(path);
            }
            if(Directory.GetFileSystemEntries(versions).Length==0)Directory.Delete(versions);
        }
        PruneConfigHistory(backups);
    }
    internal static void PruneReleaseHistory(string backups) {
        string versions=Under(backups,"versions");if(!Directory.Exists(versions))return;
        var originals=new HashSet<string>{"2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2","4e2f65147faec13ae5333337e3c7cd9e25028f3119fe817e059b1d2eab678522","21757cde7fa348089fe6398f50bc225371e38208b556df47c18a18259cf86ccb","d661606db8658adb54d5be18b3e3a3a6f07c8e738f2d78556ece42355f8e62e4","554b77df210e85252f20e6e611a1debefff5f64756bcc35e04072b754e4e795a","abfedd473b3f4a9597bbdc90d20f4b6f696bb2ebb937a03177461df695430ad6","62d1beb2cc5536f42952af06e6bbb54f8cbf3f9655bad55092b402d544e7f6ee","f0904bb0198f81bef4c4520c3ad15d6e3ba2dc3a7d69ebbce6d3ebf715908a1d"};
        foreach(string path in Directory.GetFiles(versions)) {
            if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)continue;
            var match=Regex.Match(Path.GetFileName(path),@"^([0-9a-f]{64})-(.+)$");if(!match.Success)continue;
            byte[] raw=File.ReadAllBytes(path);if(Hash(raw)!=match.Groups[1].Value)continue;
            if(originals.Contains(match.Groups[1].Value)) {string saved=Under(backups,"originals/"+Path.GetFileName(path));if(!File.Exists(saved))Atomic(saved,raw);}
            else BackupPrevious(backups,match.Groups[2].Value,raw,false,File.GetLastWriteTimeUtc(path));
            File.Delete(path);
        }
        if(Directory.GetFileSystemEntries(versions).Length==0)Directory.Delete(versions);
    }
    internal static void MigrateBackups(string game) {
        string old = Under(game,"bak"), target = Under(game,".bak");
        if (Directory.Exists(old)) MergeBackups(old,target);
    }
    static void MergeBackups(string old, string target) {
        if (!Directory.Exists(target)) { Directory.Move(old,target); return; }
        foreach (string entry in Directory.GetFileSystemEntries(old)) {
            string next = Path.Combine(target,Path.GetFileName(entry));
            if ((File.GetAttributes(entry)&FileAttributes.ReparsePoint)!=0 || ((File.Exists(next)||Directory.Exists(next))&&(File.GetAttributes(next)&FileAttributes.ReparsePoint)!=0)) throw new IOException("Refusing linked backup entry.");
            if (Directory.Exists(entry)&&Directory.Exists(next)) MergeBackups(entry,next);
            else if (!File.Exists(next)&&!Directory.Exists(next)) { if (Directory.Exists(entry)) Directory.Move(entry,next); else File.Move(entry,next); }
            else if (File.Exists(entry)&&File.Exists(next)&&Hash(File.ReadAllBytes(entry))==Hash(File.ReadAllBytes(next))) File.Delete(entry);
            else { string saved=next+".legacy-"+Guid.NewGuid().ToString("N");if(Directory.Exists(entry))Directory.Move(entry,saved);else File.Move(entry,saved); }
        }
        Directory.Delete(old);
    }
    static string Under(string game, string name) { name=ModsVfs.Resolve(game,name); if (name.Length == 0 || name.Contains('\\') || name.Contains(':') || name.Split('/').Any(p => p == "" || p == "." || p == "..") || Path.IsPathRooted(name)) throw new IOException("Unsafe language component path."); string target = Path.GetFullPath(game); foreach (string part in name.Split('/')) { target = Path.Combine(target, part); if ((File.Exists(target) || Directory.Exists(target)) && (File.GetAttributes(target) & FileAttributes.ReparsePoint) != 0) throw new IOException("Language component paths must not be links."); } return target; }
    static bool FontPath(string name) => Regex.IsMatch(name,@"^mods/noto-(cjk|latin)-font/vfs\.(dat|zip)$") || (name.StartsWith("mods/noto-cjk-font/noto-cjk/") && FontPath("fonts/"+name.Substring("mods/noto-cjk-font/".Length))) || (name.StartsWith("mods/noto-latin-font/noto-latin/") && FontPath("fonts/"+name.Substring("mods/noto-latin-font/".Length))) || Regex.IsMatch(name, @"^vfs[1-9a-f][0-9a-f]?\.(dat|zip)$") || Regex.IsMatch(name, @"^fonts/noto-cjk/(jp|kr|sc|tc|hk)(64|96)?\.rrcf$") || name == "fonts/noto-cjk/OFL.txt" || Regex.IsMatch(name,@"^fonts/noto-latin/(latin(32|64|96)\.rrcf|OFL\.txt)$");
    internal static bool Present(string game,string pack="cjk") => File.Exists(Under(game, StateFor(pack)));
    internal static bool Enabled(string game,string pack="cjk")
    {
        string path = ReRiff.ConfigFiles.ReadPath(game,"languages"); if (!File.Exists(path)) return false;
        string section = ""; bool? saved = null; bool legacy = false;
        foreach (string raw in ReRiff.ConfigFiles.Document(game,"client.ini").Replace("\r\n","\n").Split('\n'))
        {
            string line = raw.Trim(); if (line.StartsWith("[")) section = line.ToLowerInvariant(); int eq = line.IndexOf('='); if (eq < 1) continue;
            string key = line.Substring(0, eq).Trim(), value = line.Substring(eq + 1).Trim(); bool enabled = value == "1" || value.Equals("true", StringComparison.OrdinalIgnoreCase);
            if (section == "[languages]" && key.Equals(pack+"_enabled", StringComparison.OrdinalIgnoreCase)) saved = enabled;
            if (section == "[diagnostics]" && key.Equals("cjk_render_probe", StringComparison.OrdinalIgnoreCase)) legacy = enabled;
        }
        return saved ?? (pack=="cjk"&&legacy);
    }
    static byte[] Config(string game, bool enabled,string pack="cjk") { var lines = ReRiff.ConfigFiles.Document(game,"client.ini").Replace("\r\n","\n").Split('\n').ToList(); bool section = false, found = false; int languageAt = -1; for (int i = 0; i < lines.Count; i++) { string line = lines[i].Trim(); if (line.StartsWith("[")) section = line.Equals("[languages]", StringComparison.OrdinalIgnoreCase); if (section && line.StartsWith("[")) languageAt = i; int eq = line.IndexOf('='); if (section && eq > 0 && line.Substring(0, eq).Trim().Equals(pack+"_enabled", StringComparison.OrdinalIgnoreCase)) { lines[i] = pack+"_enabled = " + (enabled ? "true" : "false"); found = true; } } if (!found) { if (languageAt >= 0) lines.Insert(languageAt + 1, pack+"_enabled = " + (enabled ? "true" : "false")); else lines.AddRange(new[] { "", "[languages]", pack+"_enabled = " + (enabled ? "true" : "false") }); } return new UTF8Encoding(false).GetBytes(string.Join("\n", lines) + "\n"); }
    static string ModRoot(string pack) => "mods/noto-"+pack+"-font/";
    static string OwnedPath(string game,string name,string hash) {
        string path=Under(game,name);
        if(File.Exists(path))return path;
        // Adopt the user's earlier manually arranged folder only with exact ownership hashes.
        string candidate=name.StartsWith("vfs")?ModRoot("cjk")+"vfs"+Path.GetExtension(name):
            name.StartsWith("fonts/noto-")?"mods/noto-"+(name.Contains("noto-latin/")?"latin":"cjk")+"-font/"+name.Substring(6):name;
        string moved=Under(game,candidate);
        return File.Exists(moved)&&Hash(File.ReadAllBytes(moved))==hash?moved:path;
    }
    static Installed Existing(string game,string pack="cjk") {
        var old=Read<Installed>(File.ReadAllBytes(Under(game,StateFor(pack))));
        if((old.schema!=1&&old.schema!=2)||old.files==null||old.archives==null)throw new IOException("Unsupported font component state.");
        foreach(var entry in old.files) {
            if(!FontPath(entry.Key))throw new IOException("Unexpected owned font path.");
            string path=OwnedPath(game,entry.Key,entry.Value);
            if(!File.Exists(path)||Hash(File.ReadAllBytes(path))!=entry.Value)throw new IOException("Installed font file changed or is missing; retained: "+entry.Key);
        }return old;
    }
    static byte[] TextureZip(IEnumerable<byte[]> sources,string pack) {
        using(var output=new MemoryStream()) {
            var seen=new HashSet<string>(StringComparer.Ordinal);
            using(var target=new ZipArchive(output,ZipArchiveMode.Create,true))foreach(byte[] source in sources)
                using(var input=new MemoryStream(source))using(var zip=new ZipArchive(input,ZipArchiveMode.Read))
                    foreach(var file in FontTextures(zip).OrderBy(f=>f.Key,StringComparer.Ordinal)) {
                        if(!file.Key.StartsWith("graphics/fonts/reriff/"+pack+"/",StringComparison.Ordinal))continue;
                        if(!seen.Add(file.Key))throw new IOException("Duplicate font texture.");
                        var entry=target.CreateEntry(file.Key,CompressionLevel.Optimal);entry.LastWriteTime=new DateTimeOffset(1980,1,1,0,0,0,TimeSpan.Zero);
                        using(var from=file.Value.Open())using(var to=entry.Open())from.CopyTo(to);
                    }
            if(seen.Count==0)throw new IOException("Font archive contains no "+pack+" textures.");
            return output.ToArray();
        }
    }
    static void StoreMod(Dictionary<string,byte[]> plan,string pack,string version,byte[] archive,Dictionary<string,byte[]> metrics) {
        string root=ModRoot(pack);var state=new Installed{schema=2,version=version};state.archives["combined"]=root+"vfs";
        plan[root+"vfs.zip"]=archive;plan[root+"vfs.dat"]=FontIndex(archive);
        foreach(var file in metrics)plan[root+"noto-"+pack+"/"+file.Key]=file.Value;
        foreach(var file in plan.Where(f=>f.Key.StartsWith(root,StringComparison.Ordinal)&&f.Value!=null))state.files[file.Key]=Hash(file.Value);
        plan[StateFor(pack)]=Json(state);
    }
    static Dictionary<string,byte[]> MigrateFonts(string game) {
        var plan=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
        var migrated=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
        foreach(string pack in new[]{"cjk","latin"})if(Present(game,pack)) {
            var old=Existing(game,pack);if(old.schema==2)continue;
            var archives=old.archives.Values.Distinct().Select(stem=>File.ReadAllBytes(OwnedPath(game,stem+".zip",old.files[stem+".zip"]))).ToArray();
            var metrics=new Dictionary<string,byte[]>();
            foreach(var file in old.files.Where(f=>f.Key.StartsWith("fonts/noto-"+pack+"/",StringComparison.Ordinal)))metrics[Path.GetFileName(file.Key)]=File.ReadAllBytes(OwnedPath(game,file.Key,file.Value));
            foreach(var file in old.files) {
                string actual=OwnedPath(game,file.Key,file.Value).Substring(Path.GetFullPath(game).Length+1).Replace('\\','/');
                plan[actual]=null;
                // Retire exact duplicate loose metrics, but retain unrelated or changed files.
                string ordinary=Under(game,file.Key);
                if(File.Exists(ordinary)&&Hash(File.ReadAllBytes(ordinary))==file.Value)plan[ordinary.Substring(Path.GetFullPath(game).Length+1).Replace('\\','/')]=null;
            }
            StoreMod(migrated,pack,old.version,TextureZip(archives,pack),metrics);
        }
        foreach(var file in migrated)plan[file.Key]=file.Value;
        return plan;
    }
    internal static Dictionary<string, byte[]> Plan(string game,string asset,string expected,bool enable,string pack="cjk") {
        StateFor(pack);var plan=MigrateFonts(game);
        if(enable&&asset!="") {
            if(!Regex.IsMatch(expected,@"^[0-9a-fA-F]{64}$")||Hash(File.ReadAllBytes(asset))!=expected.ToLowerInvariant())throw new IOException("Font download checksum mismatch.");
            using(var zip=ZipFile.OpenRead(asset)) {
                if(zip.Entries.Count>64||zip.Entries.Sum(e=>e.Length)>256L*1024*1024)throw new IOException("Font package is too large.");
                var files=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
                foreach(var entry in zip.Entries) {
                    Under(game,entry.FullName);
                    using(var input=entry.Open())using(var output=new MemoryStream()){input.CopyTo(output);files.Add(entry.FullName,output.ToArray());}
                }
                byte[] metadata;if(!files.TryGetValue("component.json",out metadata)||metadata.Length>65536)throw new IOException("Missing font manifest.");
                var package=Read<Package>(metadata);
                if((package.schema!=1&&package.schema!=2)||package.files==null||package.files.Count+1!=files.Count)throw new IOException("Invalid font manifest.");
                var archives=new List<byte[]>();var metrics=new Dictionary<string,byte[]>();
                foreach(var file in package.files) {
                    byte[] data;if(!files.TryGetValue(file.Key,out data)||Hash(data)!=file.Value)throw new IOException("Font checksum mismatch: "+file.Key);
                    if(Regex.IsMatch(file.Key,@"^archives/(combined|base|hd)\.(zip|dat)$")) {
                        if(file.Key.EndsWith(".zip")) {
                            if(!files.ContainsKey(Path.ChangeExtension(file.Key,"dat")))throw new IOException("Incomplete font archive pair.");
                            archives.Add(data);
                        }
                    }else if(file.Key.StartsWith("fonts/noto-"+pack+"/",StringComparison.Ordinal)&&FontPath(file.Key))metrics.Add(Path.GetFileName(file.Key),data);
                    else throw new IOException("Unexpected font package path.");
                }
                foreach(string name in pack=="latin"?new[]{"latin32.rrcf","latin64.rrcf","latin96.rrcf","OFL.txt"}:new[]{"jp.rrcf","jp64.rrcf","jp96.rrcf","OFL.txt"})if(!metrics.ContainsKey(name))throw new IOException("Incomplete font metrics/license.");
                var updated=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);StoreMod(updated,pack,package.version,TextureZip(archives,pack),metrics);
                var old=Present(game,pack)?Existing(game,pack):null;
                foreach(var file in updated.Where(f=>f.Key.StartsWith(ModRoot(pack),StringComparison.Ordinal))) {
                    string path=Under(game,file.Key);
                    if(File.Exists(path)&&!plan.ContainsKey(file.Key)&&(old==null||!old.files.ContainsKey(file.Key))&&Hash(File.ReadAllBytes(path))!=Hash(file.Value))throw new IOException("Unowned font file retained: "+file.Key);
                }
                if(old!=null&&old.schema==2)foreach(string name in old.files.Keys)if(!updated.ContainsKey(name))plan[name]=null;
                foreach(var file in updated)plan[file.Key]=file.Value;
            }
        }else if(enable&&!Present(game,pack))throw new IOException("Install the font component first.");
        // Check every destination, including migration-only operations, before any writes.
        foreach(var file in plan.Where(f=>f.Value!=null&&f.Key.StartsWith("mods/",StringComparison.Ordinal))) {
            string path=Under(game,file.Key);
            if(!File.Exists(path))continue;
            bool owned=false;
            foreach(string owner in new[]{"cjk","latin"})if(Present(game,owner)) {
                var state=Read<Installed>(File.ReadAllBytes(Under(game,StateFor(owner))));
                if(state.files.Any(f=>OwnedPath(game,f.Key,f.Value)==path&&Hash(File.ReadAllBytes(path))==f.Value))owned=true;
            }
            if(!owned&&Hash(File.ReadAllBytes(path))!=Hash(file.Value))throw new IOException("Unowned mod file retained: "+file.Key);
        }
        plan["client.ini"]=Config(game,enable,pack);
        return plan;
    }
    // Rebuild only an owned font volume; original/mod volumes are never edited.
    static Dictionary<string, ZipArchiveEntry> FontTextures(ZipArchive zip) {
        var files=new Dictionary<string,ZipArchiveEntry>(StringComparer.Ordinal);
        if(zip.Entries.Count>64||zip.Entries.Sum(e=>e.Length)>2048L*1024*1024)throw new IOException("Font archive exceeds limits.");
        foreach(var entry in zip.Entries) {
            if(!Regex.IsMatch(entry.FullName,@"^graphics/fonts/reriff/(cjk|latin)/[a-z0-9/]+/batchtextures\.brres$")||files.ContainsKey(entry.FullName))throw new IOException("Unexpected font texture path.");
            files.Add(entry.FullName,entry);
        }return files;
    }
    static byte[] FontIndex(byte[] zip) {
        Func<int,uint> u32=p=>BitConverter.ToUInt32(zip,p);Func<int,ushort> u16=p=>BitConverter.ToUInt16(zip,p);
        int end=zip.Length-22;if(end<0||u32(end)!=0x06054b50)throw new IOException("Unexpected font ZIP footer.");
        int count=u16(end+10),cursor=(int)u32(end+16),origin=cursor;uint crc=0;
        var pool=new MemoryStream();var nodes=new List<ushort[]>();var paths=new Dictionary<string,int>();var children=new Dictionary<string,List<int>>();var records=new List<uint[]>();
        for(int i=0;i<count;i++) {
            if(u32(cursor)!=0x02014b50)throw new IOException("Invalid font ZIP directory.");
            int size=u16(cursor+28);string name=Encoding.ASCII.GetString(zip,cursor+46,size);string parent="";string[] parts=name.Split('/');
            for(int depth=0;depth<parts.Length;depth++) {
                string path=string.Join("/",parts.Take(depth+1));int node;
                if(!paths.TryGetValue(path,out node)) {
                    if(pool.Length+parts[depth].Length+1>65535||nodes.Count>=65535)throw new IOException("Font VFS tree exceeds bounds.");
                    node=nodes.Count;paths[path]=node;nodes.Add(new[]{(ushort)pool.Length,(ushort)65535,(ushort)65535,(ushort)(depth==parts.Length-1?i:0x3fff)});
                    byte[] text=Encoding.ASCII.GetBytes(parts[depth]+"\0");pool.Write(text,0,text.Length);
                    if(!children.ContainsKey(parent))children[parent]=new List<int>();children[parent].Add(node);
                }parent=path;
            }
            records.Add(new uint[]{2,u16(cursor+10),u32(cursor+20),u32(cursor+24),(uint)size,u32(cursor+42),(uint)(cursor-origin)});
            crc=unchecked(crc+u32(cursor+16));cursor+=46+size+u16(cursor+30)+u16(cursor+32);
        }
        foreach(var group in children){if(group.Key!="")nodes[paths[group.Key]][2]=(ushort)group.Value[0];for(int i=1;i<group.Value.Count;i++)nodes[group.Value[i-1]][1]=(ushort)group.Value[i];}
        using(var output=new MemoryStream())using(var writer=new BinaryWriter(output)) {
            foreach(uint value in new uint[]{0x0300ffee,(uint)nodes.Count,(uint)count,(uint)pool.Length,crc})writer.Write(value);
            foreach(ushort value in new ushort[]{0,65535,(ushort)children[""][0],65535})writer.Write(value);
            foreach(var node in nodes)foreach(ushort value in node)writer.Write(value);
            foreach(var record in records)foreach(uint value in record)writer.Write(value);writer.Write(pool.ToArray());return output.ToArray();
        }
    }
    internal static Dictionary<string,byte[]> Selections(string game,string cjkAsset,string cjkHash,bool? cjk,string latinAsset,string latinHash,bool? latin) {
        var result=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
        if(latin==null)return cjk.HasValue?Plan(game,cjkAsset,cjkHash,cjk.Value):result;
        string shadow=Path.Combine(Path.GetTempPath(),"reriff-fonts-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(shadow);
        try {
            if(File.Exists(Under(game,ModsVfs.Marker)))Atomic(Under(shadow,ModsVfs.Marker),File.ReadAllBytes(Under(game,ModsVfs.Marker)));
            foreach(string pack in new[]{"cjk","latin"})if(Present(game,pack)) {
                var state=Existing(game,pack);
                foreach(string name in state.files.Keys.Concat(new[]{StateFor(pack)}).Distinct()) {
                    string source=state.files.ContainsKey(name)?OwnedPath(game,name,state.files[name]):Under(game,name);
                    string relative=source.Substring(Path.GetFullPath(game).Length+1).Replace('\\','/');
                    Atomic(Under(shadow,relative),File.ReadAllBytes(source));
                }
            }
            foreach(string pack in new[]{"cjk","latin"}) {
                string folder=Under(game,ModRoot(pack).TrimEnd('/'));
                if(Directory.Exists(folder))foreach(string file in Directory.GetFiles(folder,"*",SearchOption.AllDirectories)) {
                    string name=file.Substring(Path.GetFullPath(game).Length+1).Replace('\\','/');
                    Atomic(Under(shadow,name),File.ReadAllBytes(Under(game,name)));
                }
            }
            foreach(string config in new[]{"server.ini","client.ini"})if(File.Exists(Under(game,config)))Atomic(Under(shadow,config),File.ReadAllBytes(Under(game,config)));
            if(cjk.HasValue)foreach(var item in Plan(shadow,cjkAsset,cjkHash,cjk.Value)) {result[item.Key]=item.Value;if(item.Value==null)File.Delete(Under(shadow,item.Key));else Atomic(Under(shadow,item.Key),item.Value);}
            foreach(var item in Plan(shadow,latinAsset,latinHash,latin.Value,"latin"))result[item.Key]=item.Value;
            return result;
        } finally {Directory.Delete(shadow,true);}
    }
    static void Atomic(string path, byte[] data) { Directory.CreateDirectory(Path.GetDirectoryName(path)); string temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp"; try { File.WriteAllBytes(temp, data); if (File.Exists(path)) File.Replace(temp, path, null); else File.Move(temp, path); } finally { if (File.Exists(temp)) File.Delete(temp); } }
    internal static void Apply(string game, Dictionary<string, byte[]> plan) {
        MigrateBackups(game); bool development=Development(game);
        var before=new Dictionary<string,byte[]>();var changed=new List<string>();
        var paths=plan.Keys.ToDictionary(name=>name,name=>Under(game,name));
        try {
            foreach(var entry in plan) {
                string path=paths[entry.Key];before[entry.Key]=File.Exists(path)?File.ReadAllBytes(path):null;
                if(before[entry.Key]!=null&&(entry.Value==null||Hash(before[entry.Key])!=Hash(entry.Value))) BackupPrevious(Under(game,".bak"),entry.Key,before[entry.Key],development);
                if(entry.Value==null)File.Delete(path);else Atomic(path,entry.Value);changed.Add(entry.Key);
            }
        } catch {
            foreach(string name in changed.AsEnumerable().Reverse()) {string path=paths[name];if(before[name]==null)File.Delete(path);else Atomic(path,before[name]);}
            throw;
        }
        MigrateConfigHistory(Under(game,".bak"));
        if(!development)PruneReleaseHistory(Under(game,".bak"));
    }
    static byte[] Download(string url, int maximum) { ServicePointManager.SecurityProtocol = (SecurityProtocolType)3072; var request = WebRequest.Create(url); request.Timeout = 30000; using (var response = request.GetResponse()) using (var input = response.GetResponseStream()) using (var output = new MemoryStream()) { byte[] block = new byte[65536]; int count; while ((count = input.Read(block, 0, block.Length)) > 0) { if (output.Length + count > maximum) throw new IOException("Language download is too large."); output.Write(block, 0, count); } return output.ToArray(); } }
    internal static int Run(string[] args)
    {
        string result = "", game = "", temporary = "",pack="cjk"; bool ok = false; string message = ""; try
        {
            if (args.Length == 0) throw new ArgumentException("Language action required."); var options = new Dictionary<string, string>(); for (int i = 1; i < args.Length; i += 2) { if (i + 1 >= args.Length || options.ContainsKey(args[i])) throw new ArgumentException("Invalid language option."); options.Add(args[i], args[i + 1]); }
            game = Path.GetFullPath(options["--game-dir"]); if (options.ContainsKey("--result")) result = Path.GetFullPath(options["--result"]); string action = args[0];pack=options.ContainsKey("--pack")?options["--pack"]:"cjk";StateFor(pack);
            if (action != "status")
            {
                string asset = options.ContainsKey("--asset") ? options["--asset"] : "", expected = options.ContainsKey("--sha256") ? options["--sha256"] : ""; bool enable = action != "disable" && (!options.ContainsKey("--enable") || options["--enable"] == "true");
                if (action != "install" && action != "disable" && action != "stage") throw new ArgumentException("Unknown language action.");
                if (action == "install" && enable && asset == "" && !Present(game,pack))
                {
                    string version = File.ReadAllText(Under(game, "localserver/client-version.txt")).Trim(); if (!Regex.IsMatch(version, @"^\d+\.\d+\.\d+(?:-beta\.\d+)?$")) throw new IOException("Invalid client version.");
                    string assetName=pack=="latin"?"ReRiff-Latin.zip":"ReRiff-CJK.zip";
                    string origin = "https://github.com/Aftonstjarna/ReRiff/releases/download/v" + version + "/"; string sums = Encoding.UTF8.GetString(Download(origin + "SHA256SUMS", 65536)); var match = Regex.Match(sums, @"(?m)^([0-9a-fA-F]{64})\s+"+Regex.Escape(assetName)+@"\s*$"); if (!match.Success) throw new IOException("This release has no verified CJK component."); expected = match.Groups[1].Value; temporary = Path.GetTempFileName(); File.WriteAllBytes(temporary, Download(origin + assetName, 256 * 1024 * 1024)); asset = temporary;
                }
                var plan = options.ContainsKey("--latin-enable")?Selections(game,asset,expected,enable,options.ContainsKey("--latin-asset")?options["--latin-asset"]:"",options.ContainsKey("--latin-sha256")?options["--latin-sha256"]:"",options["--latin-enable"]=="true"):Plan(game, asset, expected, enable,pack);
                if (action == "stage") { string stage = options["--stage"]; var removals = new Dictionary<string,string>(); foreach (var file in plan) { if (file.Value == null) { removals[file.Key] = Hash(File.ReadAllBytes(Under(game,file.Key))); continue; } string path = Under(stage, file.Key); Directory.CreateDirectory(Path.GetDirectoryName(path)); File.WriteAllBytes(path, file.Value); } if (removals.Count != 0) Atomic(Under(stage,".reriff-language-removals.json"),Json(removals)); }
                else Apply(game, plan);
                message = enable ? pack+" fonts installed and enabled. Restart the game to apply." : pack+" fonts disabled. Restart the game to apply. Installed fonts are retained.";
            }
            else message = Present(game,pack) ? pack+" font component is installed." : pack+" font component is not installed.";
            ok = true;
        }
        catch (Exception error) { message = error.Message; Console.Error.WriteLine(error); if (game != ""&&ReRiff.Logging.Enabled(game)) try { ReRiff.Logging.Append(game,"ReRiff-languages.log",error+"\n"); } catch (IOException) { } }
        finally { if (temporary != "" && File.Exists(temporary)) File.Delete(temporary); if (result != "") { bool present = false, enabled = false; try { if (game != "") { present = Present(game,pack); enabled = Enabled(game,pack); } } catch (IOException) { } string content = "[result]\nok=" + (ok ? "true" : "false") + "\ninstalled=" + (present ? "true" : "false") + "\nenabled=" + (enabled ? "true" : "false") + "\nmessage=" + message.Replace('\n', ' ').Replace('\r', ' ') + "\n"; Atomic(result, Encoding.Unicode.GetPreamble().Concat(Encoding.Unicode.GetBytes(content)).ToArray()); } }
        return ok ? 0 : 1;
    }
}
