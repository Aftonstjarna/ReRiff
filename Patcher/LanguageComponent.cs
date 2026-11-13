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
    static string Under(string game, string name) { if (name.Length == 0 || name.Contains('\\') || name.Contains(':') || name.Split('/').Any(p => p == "" || p == "." || p == "..") || Path.IsPathRooted(name)) throw new IOException("Unsafe language component path."); string target = Path.GetFullPath(game); foreach (string part in name.Split('/')) { target = Path.Combine(target, part); if ((File.Exists(target) || Directory.Exists(target)) && (File.GetAttributes(target) & FileAttributes.ReparsePoint) != 0) throw new IOException("Language component paths must not be links."); } return target; }
    static bool FontPath(string name) => Regex.IsMatch(name, @"^vfs[1-9a-f][0-9a-f]?\.(dat|zip)$") || Regex.IsMatch(name, @"^fonts/noto-cjk/(jp|kr|sc|tc|hk)(64|96)?\.rrcf$") || name == "fonts/noto-cjk/OFL.txt" || Regex.IsMatch(name,@"^fonts/noto-latin/(latin(32|64|96)\.rrcf|OFL\.txt)$");
    internal static bool Present(string game,string pack="cjk") => File.Exists(Under(game, StateFor(pack)));
    internal static bool Enabled(string game,string pack="cjk")
    {
        string path = Path.Combine(game, "server.ini"); if (!File.Exists(path)) return false;
        string section = ""; bool? saved = null; bool legacy = false;
        foreach (string raw in File.ReadAllLines(path))
        {
            string line = raw.Trim(); if (line.StartsWith("[")) section = line.ToLowerInvariant(); int eq = line.IndexOf('='); if (eq < 1) continue;
            string key = line.Substring(0, eq).Trim(), value = line.Substring(eq + 1).Trim(); bool enabled = value == "1" || value.Equals("true", StringComparison.OrdinalIgnoreCase);
            if (section == "[languages]" && key.Equals(pack+"_enabled", StringComparison.OrdinalIgnoreCase)) saved = enabled;
            if (section == "[diagnostics]" && key.Equals("cjk_render_probe", StringComparison.OrdinalIgnoreCase)) legacy = enabled;
        }
        return saved ?? (pack=="cjk"&&legacy);
    }
    static byte[] Config(string game, bool enabled,string pack="cjk") { string path = Path.Combine(game, "server.ini"); var lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string> { "[server]", "external_url =", "fallback_url =", "timeout_seconds = 5", "networking_enabled = true", "show_server_popup = true", "", "[launcher]", "skip_launcher = false", "auto_update_on_skip = false" }; bool section = false, found = false; int languageAt = -1; for (int i = 0; i < lines.Count; i++) { string line = lines[i].Trim(); if (line.StartsWith("[")) section = line.Equals("[languages]", StringComparison.OrdinalIgnoreCase); if (section && line.StartsWith("[")) languageAt = i; int eq = line.IndexOf('='); if (section && eq > 0 && line.Substring(0, eq).Trim().Equals(pack+"_enabled", StringComparison.OrdinalIgnoreCase)) { lines[i] = pack+"_enabled = " + (enabled ? "true" : "false"); found = true; } } if (!found) { if (languageAt >= 0) lines.Insert(languageAt + 1, pack+"_enabled = " + (enabled ? "true" : "false")); else lines.AddRange(new[] { "", "[languages]", pack+"_enabled = " + (enabled ? "true" : "false") }); } return new UTF8Encoding(false).GetBytes(string.Join("\n", lines) + "\n"); }
    static Installed Existing(string game,string pack="cjk") { var old = Read<Installed>(File.ReadAllBytes(Under(game, StateFor(pack)))); if (old.schema != 1 || old.files == null || old.archives == null) throw new IOException("Unsupported CJK component state."); foreach (var entry in old.files) { if (!FontPath(entry.Key)) throw new IOException("Unexpected owned font path."); string path = Under(game, entry.Key); if (!File.Exists(path) || Hash(File.ReadAllBytes(path)) != entry.Value) throw new IOException("Installed CJK file changed or is missing; retained: " + entry.Key); } return old; }
    internal static Dictionary<string, byte[]> Plan(string game, string asset, string expected, bool enable,string pack="cjk")
    {
        var plan = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
        if (!enable) { plan["server.ini"] = Config(game, false,pack); return plan; }
        if (asset == "") { if (!Present(game,pack)) throw new IOException("Install the CJK font component first."); Existing(game,pack); plan["server.ini"] = Config(game, true,pack); return plan; }
        if (!Regex.IsMatch(expected, @"^[0-9a-fA-F]{64}$") || Hash(File.ReadAllBytes(asset)) != expected.ToLowerInvariant()) throw new IOException("CJK download checksum mismatch.");
        using (var zip = ZipFile.OpenRead(asset))
        {
            if (zip.Entries.Count > 64 || zip.Entries.Sum(e => e.Length) > 256L * 1024 * 1024) throw new IOException("CJK package is too large.");
            var entries = new Dictionary<string, ZipArchiveEntry>(StringComparer.OrdinalIgnoreCase);
            foreach (var entry in zip.Entries) { Under(game, entry.FullName); if (entries.ContainsKey(entry.FullName)) throw new IOException("Duplicate CJK package entry."); entries.Add(entry.FullName, entry); }
            ZipArchiveEntry metadata; if (!entries.TryGetValue("component.json", out metadata) || metadata.Length > 65536) throw new IOException("Missing CJK component manifest.");
            Func<ZipArchiveEntry, byte[]> bytes = e => { using (var input = e.Open()) using (var output = new MemoryStream()) { input.CopyTo(output); return output.ToArray(); } };
            var package = Read<Package>(bytes(metadata)); if ((package.schema != 1 && package.schema != 2) || package.files == null || package.legacy == null || package.files.Count + 1 != entries.Count) throw new IOException("Invalid CJK component manifest.");
            var other=Present(game,pack=="latin"?"cjk":"latin")?Existing(game,pack=="latin"?"cjk":"latin"):null;
            var old = Present(game,pack) ? Existing(game,pack) : other; var state = new Installed { version = package.version }; var used = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string[] labels = package.schema == 2 ? new[] { "combined" } : new[] { "base", "hd" };
            var retired = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (old != null) foreach (var file in old.files) if (file.Key.StartsWith("vfs")) retired.Add(file.Key);
            // Prototype pairs are owned only when both exact published hashes match.
            foreach (var prior in package.legacy) {
                if (prior.Value.Length != 2 || !Regex.IsMatch(prior.Key, @"^vfs[1-9a-f][0-9a-f]?$")) continue;
                string dat = Under(game, prior.Key + ".dat"), archive = Under(game, prior.Key + ".zip");
                if (File.Exists(dat) && File.Exists(archive) && Hash(File.ReadAllBytes(dat)) == prior.Value[0] && Hash(File.ReadAllBytes(archive)) == prior.Value[1]) { retired.Add(prior.Key + ".dat"); retired.Add(prior.Key + ".zip"); }
            }
            // Windows mounts volume numbers case-insensitively, including padded hex suffixes.
            var occupied = new HashSet<int>();
            foreach(string path in Directory.GetFiles(game)){
                var number=Regex.Match(Path.GetFileName(path),@"^vfs([0-9a-f]{1,7})\.(dat|zip)$",RegexOptions.IgnoreCase);
                int id;if(number.Success&&int.TryParse(number.Groups[1].Value,System.Globalization.NumberStyles.HexNumber,System.Globalization.CultureInfo.InvariantCulture,out id))occupied.Add(id);
            }
            foreach (string label in labels)
            {
                string stem = "";
                if (old != null) {
                    old.archives.TryGetValue(label, out stem);
                    if (string.IsNullOrEmpty(stem) && label == "combined") old.archives.TryGetValue("base", out stem);
                    if (string.IsNullOrEmpty(stem) && label == "base") old.archives.TryGetValue("combined", out stem);
                    if (!string.IsNullOrEmpty(stem) && (!old.files.ContainsKey(stem + ".dat") || !old.files.ContainsKey(stem + ".zip"))) throw new IOException("Incomplete CJK archive ownership record.");
                }
                if (string.IsNullOrEmpty(stem)) foreach (var previous in package.legacy)
                    {
                        if (previous.Value.Length != 2 || !Regex.IsMatch(previous.Key, @"^vfs[1-9a-f][0-9a-f]?$")) continue; string dat = Under(game, previous.Key + ".dat"), archive = Under(game, previous.Key + ".zip"); if (!used.Contains(previous.Key) && File.Exists(dat) && File.Exists(archive) && Hash(File.ReadAllBytes(dat)) == previous.Value[0] && Hash(File.ReadAllBytes(archive)) == previous.Value[1])
                        { // Identify the old volume by its resource group.
                            using (var prior = ZipFile.OpenRead(archive)) if (prior.Entries.Any(e => e.FullName.Contains(label == "hd" ? "/jp64/" : "/jp/"))) { stem = previous.Key; break; }
                        }
                    }
                if (string.IsNullOrEmpty(stem)) for (int id = 1; id <= 31; id++) { string candidate = "vfs" + id.ToString("x"); if (!occupied.Contains(id) && !used.Contains(candidate) && !File.Exists(Under(game, candidate + ".dat")) && !File.Exists(Under(game, candidate + ".zip"))) { stem = candidate; break; } }
                if (string.IsNullOrEmpty(stem) || !Regex.IsMatch(stem, @"^vfs[1-9a-f][0-9a-f]?$") || used.Contains(stem)) throw new IOException("No unused native archive slot for CJK fonts."); used.Add(stem); state.archives[label] = stem;
            }
            foreach (var file in package.files)
            {
                ZipArchiveEntry entry; if (!entries.TryGetValue(file.Key, out entry)) throw new IOException("Missing CJK component file."); byte[] data = bytes(entry); if (Hash(data) != file.Value) throw new IOException("CJK component file checksum mismatch: " + file.Key); string target = file.Key;
                if (file.Key.StartsWith("archives/")) { var match = Regex.Match(file.Key, @"^archives/(base|hd|combined)\.(dat|zip)$"); if (!match.Success || !state.archives.ContainsKey(match.Groups[1].Value)) throw new IOException("Unexpected font archive."); target = state.archives[match.Groups[1].Value] + "." + match.Groups[2].Value; }
                if (!FontPath(target)) throw new IOException("Unexpected CJK package path."); string existing = Under(game, target);
                if (File.Exists(existing) && !target.StartsWith("vfs") && (old == null || !old.files.ContainsKey(target)) && Hash(File.ReadAllBytes(existing)) != file.Value) throw new IOException("Unowned font file retained: " + target);
                plan[target] = data; state.files[target] = file.Value;
            }
            foreach (string label in labels) foreach (string ext in new[] { "dat", "zip" }) if (!plan.ContainsKey(state.archives[label] + "." + ext)) throw new IOException("Incomplete font archive pair.");
            foreach (string file in pack=="latin"?new[]{"latin32.rrcf","latin64.rrcf","latin96.rrcf","OFL.txt"}:new[]{"jp.rrcf","jp64.rrcf","jp96.rrcf","OFL.txt"}) if (!plan.ContainsKey("fonts/noto-"+pack+"/" + file)) throw new IOException("Incomplete CJK metrics/license.");
            foreach (string file in retired) if (!plan.ContainsKey(file)) plan[file] = null;
            if(other!=null){if(package.schema!=2)throw new IOException("Upgrade to the combined CJK package first.");MergeFontVolume(plan,game,other,pack,state);}
            plan[StateFor(pack)] = Json(state); plan["server.ini"] = Config(game, true,pack); return plan;
        }
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
    static void MergeFontVolume(Dictionary<string,byte[]> plan,string game,Installed other,string pack,Installed state) {
        string stem=state.archives["combined"],zipName=stem+".zip",datName=stem+".dat";byte[] data;
        // Stream textures: expanding every CJK page simultaneously can exhaust
        // the 32-bit Windows helper's address space.
        using(var input=new MemoryStream(plan[zipName]))using(var incomingZip=new ZipArchive(input,ZipArchiveMode.Read))
        using(var existingZip=ZipFile.OpenRead(Under(game,other.archives["combined"]+".zip"))) {
            var incoming=FontTextures(incomingZip);var files=FontTextures(existingZip);
            string prefix="graphics/fonts/reriff/"+(pack=="latin"?"latin":"cjk")+"/";
            foreach(string name in files.Keys.Where(n=>n.StartsWith(prefix,StringComparison.Ordinal)).ToArray())files.Remove(name);
            foreach(var entry in incoming)files.Add(entry.Key,entry.Value);
            using(var output=new MemoryStream()){
                using(var zip=new ZipArchive(output,ZipArchiveMode.Create,true))foreach(var file in files.OrderBy(f=>f.Key,StringComparer.Ordinal)){
                    var entry=zip.CreateEntry(file.Key,CompressionLevel.Optimal);entry.LastWriteTime=new DateTimeOffset(1980,1,1,0,0,0,TimeSpan.Zero);
                    using(var source=file.Value.Open())using(var target=entry.Open())source.CopyTo(target);
                }data=output.ToArray();
            }
        }
        byte[] index=FontIndex(data);plan[zipName]=data;plan[datName]=index;
        state.files[zipName]=Hash(data);state.files[datName]=Hash(index);
        other.archives["combined"]=stem;
        foreach(string name in other.files.Keys.Where(n=>n.StartsWith("vfs",StringComparison.Ordinal)).ToArray())other.files.Remove(name);
        other.files[zipName]=Hash(data);other.files[datName]=Hash(index);
        plan[StateFor(pack=="latin"?"cjk":"latin")]=Json(other);
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
            foreach(string pack in new[]{"cjk","latin"})if(Present(game,pack)) {
                var state=Existing(game,pack);
                foreach(string name in state.files.Keys.Concat(new[]{StateFor(pack)}).Distinct())Atomic(Under(shadow,name),File.ReadAllBytes(Under(game,name)));
            }
            if(File.Exists(Under(game,"server.ini")))Atomic(Under(shadow,"server.ini"),File.ReadAllBytes(Under(game,"server.ini")));
            // Preserve occupied mod slots in the shadow without reading their contents.
            foreach(string file in Directory.GetFiles(game,"*"))if(Regex.IsMatch(Path.GetFileName(file),@"(?i)^vfs[0-9a-f]+\.(dat|zip)$")&&!File.Exists(Path.Combine(shadow,Path.GetFileName(file))))File.WriteAllBytes(Path.Combine(shadow,Path.GetFileName(file)),new byte[0]);
            if(cjk.HasValue)foreach(var item in Plan(shadow,cjkAsset,cjkHash,cjk.Value)) {result[item.Key]=item.Value;if(item.Value==null)File.Delete(Under(shadow,item.Key));else Atomic(Under(shadow,item.Key),item.Value);}
            foreach(var item in Plan(shadow,latinAsset,latinHash,latin.Value,"latin"))result[item.Key]=item.Value;
            return result;
        } finally {Directory.Delete(shadow,true);}
    }
    static void Atomic(string path, byte[] data) { Directory.CreateDirectory(Path.GetDirectoryName(path)); string temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp"; try { File.WriteAllBytes(temp, data); if (File.Exists(path)) File.Replace(temp, path, null); else File.Move(temp, path); } finally { if (File.Exists(temp)) File.Delete(temp); } }
    internal static void Apply(string game, Dictionary<string, byte[]> plan) {
        MigrateBackups(game); bool development=Development(game);
        var before=new Dictionary<string,byte[]>();var changed=new List<string>();
        try {
            foreach(var entry in plan) {
                string path=Under(game,entry.Key);before[entry.Key]=File.Exists(path)?File.ReadAllBytes(path):null;
                if(before[entry.Key]!=null&&(entry.Value==null||Hash(before[entry.Key])!=Hash(entry.Value))) BackupPrevious(Under(game,".bak"),entry.Key,before[entry.Key],development);
                if(entry.Value==null)File.Delete(path);else Atomic(path,entry.Value);changed.Add(entry.Key);
            }
        } catch {
            foreach(string name in changed.AsEnumerable().Reverse()) {string path=Under(game,name);if(before[name]==null)File.Delete(path);else Atomic(path,before[name]);}
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
