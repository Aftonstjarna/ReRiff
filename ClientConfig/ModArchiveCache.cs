// ReRiff owns cache construction only. The original engine mounts and reads VFS.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Security.Cryptography;

namespace ReRiff {
    internal static partial class ModArchiveCache {
        const string Version="reriff-loose-cache-v1";
        internal sealed class Entry {
            internal string Name, Source, SourceHash;
            internal byte[] Data;
            internal long Offset;
            internal uint Size, Packed, Crc;
            internal ushort Method;
        }
        internal sealed class Plan { internal string Stem; internal int Id; }
        static void Regular(string path) {
            if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked mod path: "+path);
        }
        static string Digest(string file) {
            Regular(file);
            using(var hash=SHA256.Create())using(var stream=File.OpenRead(file))return Hex(hash.ComputeHash(stream));
        }
        static string Hex(byte[] bytes){return BitConverter.ToString(bytes).Replace("-","").ToLowerInvariant();}
        static string Key(string value){using(var hash=SHA256.Create())return Hex(hash.ComputeHash(Encoding.UTF8.GetBytes(value)));}
        static void Name(string name) {
            if(name.Length==0||name.Length>65535)throw new IOException("Invalid native resource name");
            foreach(char c in name)if(c<32||c>126||c=='\\'||c==':'||char.IsUpper(c))throw new IOException("Resource names must be lowercase ASCII: "+name);
            foreach(string part in name.Split('/'))if(part==""||part=="."||part=="..")throw new IOException("Invalid resource path: "+name);
        }
        static readonly uint[] CrcTable=MakeCrcTable();
        static uint[] MakeCrcTable(){var table=new uint[256];for(uint i=0;i<256;i++){uint v=i;for(int bit=0;bit<8;bit++)v=(v>>1)^((v&1)!=0?0xedb88320u:0u);table[i]=v;}return table;}
        static uint Crc(Stream input,long count) {
            uint crc=0xffffffff;byte[] buffer=new byte[65536];
            while(count>0){int n=input.Read(buffer,0,(int)Math.Min(count,buffer.Length));if(n==0)throw new EndOfStreamException();count-=n;
                for(int i=0;i<n;i++){crc=(crc>>8)^CrcTable[(crc^buffer[i])&255];}}
            return ~crc;
        }
        static void Walk(string root,string relative,List<Entry> entries,StringBuilder fingerprint) {
            string directory=Path.Combine(root,relative);Regular(directory);
            string[] paths=Directory.GetFileSystemEntries(directory);Array.Sort(paths,StringComparer.Ordinal);
            foreach(string path in paths){Regular(path);string actual=(relative.Length==0?"":relative+"/")+Path.GetFileName(path);
                /* Cache names are lowercase; stock VFS indexes are lowercase even
                 * when the containing ZIP preserves mixed-case file names. */
                string name=actual.ToLowerInvariant();Name(name);
                if(Directory.Exists(path)){Walk(root,actual,entries,fingerprint);continue;}
                long size=new FileInfo(path).Length;if(size>uint.MaxValue)throw new IOException("ZIP64 mod assets are unsupported");
                string hash=Digest(path);
                fingerprint.Append(name).Append('\0').Append(hash).Append('\n');
                entries.Add(new Entry{Name=name,Source=path,Size=(uint)size,Packed=(uint)size,SourceHash=hash});
            }
        }
        static ushort U16(byte[] b,int p){return BitConverter.ToUInt16(b,p);}
        static uint U32(byte[] b,int p){return BitConverter.ToUInt32(b,p);}
        // Read only resources exposed by the native index; ZIP extras are not mods.
        static List<Entry> ReadPair(string stem,bool original=false) {
            Regular(stem+".dat");Regular(stem+".zip");if(new FileInfo(stem+".dat").Length>1048576)throw new IOException("Native VFS index exceeds bounds");byte[] dat=File.ReadAllBytes(stem+".dat");
            if(dat.Length<28||U32(dat,0)!=0x0300ffee)throw new IOException("Invalid VFS index");
            uint nc=U32(dat,4),fc=U32(dat,8),pool=U32(dat,12);
            if(nc==0||nc>65535||fc==0||fc>=0x3fff||pool>65535||28L+nc*8L+fc*28L+pool!=dat.Length)throw new IOException("Invalid VFS bounds");
            int record=28+(int)nc*8,strings=record+(int)fc*28;
            var names=new string[fc];var visited=new HashSet<int>();var pending=new Queue<KeyValuePair<int,string>>();
            pending.Enqueue(new KeyValuePair<int,string>(U16(dat,24),""));
            while(pending.Count>0){var item=pending.Dequeue();int node=item.Key;if(node==65535)continue;
                if(node>=nc||!visited.Add(node))throw new IOException("Invalid VFS tree");int p=28+node*8;int off=U16(dat,p);
                if(off>=pool)throw new IOException("Invalid VFS name offset");int end=strings+off;while(end<dat.Length&&dat[end]!=0)end++;
                if(end==dat.Length)throw new IOException("Unterminated VFS name");
                for(int i=strings+off;i<end;i++)if(dat[i]>127)throw new IOException("Non-ASCII VFS name");
                string part=Encoding.ASCII.GetString(dat,strings+off,end-strings-off);Name(part);if(part.IndexOf('/')>=0)throw new IOException("Invalid VFS node name");
                string name=item.Value+part;int fi=U16(dat,p+6),child=U16(dat,p+4);
                if(fi==0x3fff)pending.Enqueue(new KeyValuePair<int,string>(child,name+"/"));
                else {if(fi>=fc||names[fi]!=null||child!=65535)throw new IOException("Invalid VFS file node");names[fi]=name;}
                pending.Enqueue(new KeyValuePair<int,string>(U16(dat,p+2),item.Value));
            }
            if(visited.Count!=nc)throw new IOException("Unreachable VFS nodes");
            var entries=new List<Entry>();var unique=new HashSet<string>(StringComparer.Ordinal);uint total=0;
            using(var f=File.OpenRead(stem+".zip"))using(var r=new BinaryReader(f)){
                int tailLength=(int)Math.Min(f.Length,65557);f.Position=f.Length-tailLength;byte[] tail=r.ReadBytes(tailLength);int footer=-1;
                for(int t=tail.Length-22;t>=0;t--)if(U32(tail,t)==0x06054b50&&t+22+U16(tail,t+20)==tail.Length){footer=t;break;}
                if(footer<0||U16(tail,footer+4)!=0||U16(tail,footer+6)!=0||U16(tail,footer+8)!=U16(tail,footer+10))throw new IOException("Invalid ZIP directory");
                long central=U32(tail,footer+16),centralSize=U32(tail,footer+12);
                if(central+centralSize!=f.Length-tailLength+footer)throw new IOException("Invalid ZIP directory bounds");
                for(int i=0;i<fc;i++){
                string name=names[i];if(name==null||!unique.Add(name))throw new IOException("Duplicate/missing VFS resource");
                int p=record+i*28;uint method=U32(dat,p+4),packed=U32(dat,p+8),size=U32(dat,p+12),len=U32(dat,p+16),offset=U32(dat,p+20);
                if(U32(dat,p)!=2||(method!=0&&method!=8)||len!=name.Length||offset+30L>f.Length)throw new IOException("Unsupported VFS record");
                long cpOffset=central+U32(dat,p+24);if(cpOffset<central||cpOffset+46>central+centralSize)throw new IOException("Invalid indexed ZIP directory offset");
                f.Position=cpOffset;byte[] directory=r.ReadBytes(46);
                if(U32(directory,0)!=0x02014b50||U16(directory,10)!=method||U32(directory,20)!=packed||U32(directory,24)!=size||U32(directory,42)!=offset||U16(directory,28)!=len)throw new IOException("ZIP directory/index mismatch");
                uint expectedCrc=U32(directory,16);ushort expectedFlags=U16(directory,8);
                if(cpOffset+46+len+U16(directory,30)+U16(directory,32)>central+centralSize)throw new IOException("ZIP directory record overflow");
                if(!String.Equals(Encoding.ASCII.GetString(r.ReadBytes((int)len)),name,original?StringComparison.OrdinalIgnoreCase:StringComparison.Ordinal))throw new IOException("ZIP directory name mismatch");
                f.Position=offset;if(r.ReadUInt32()!=0x04034b50)throw new IOException("Invalid ZIP local header");
                r.ReadUInt16();ushort flags=r.ReadUInt16(),zipMethod=r.ReadUInt16();r.ReadUInt32();uint crc=r.ReadUInt32(),cp=r.ReadUInt32(),sz=r.ReadUInt32();ushort nl=r.ReadUInt16(),extra=r.ReadUInt16();
                if((flags&~(method==8||original?0x80e:0x808))!=0||flags!=expectedFlags||zipMethod!=method||nl!=len||((flags&8)==0&&(cp!=packed||sz!=size||crc!=expectedCrc)))throw new IOException("ZIP/index mismatch");
                byte[] nb=r.ReadBytes(nl);if(nb.Length!=nl||!String.Equals(Encoding.ASCII.GetString(nb),name,original?StringComparison.OrdinalIgnoreCase:StringComparison.Ordinal))throw new IOException("ZIP name/index mismatch");
                long start=f.Position+extra;if(start+packed>central)throw new IOException("Truncated ZIP data");
                crc=expectedCrc;
                total=unchecked(total+crc);entries.Add(new Entry{Name=name,Source=stem+".zip",Offset=start,Size=size,Packed=packed,Crc=crc,Method=(ushort)method});
            }
            }
            if(total!=U32(dat,16))throw new IOException("VFS checksum mismatch");return entries;
        }
        internal static byte[] Resource(List<Plan> plans,string name){
            Entry found=null;foreach(var plan in plans)foreach(var e in ReadPair(plan.Stem))if(e.Name==name)found=e;
            if(found==null)return null;if(found.Size>4000000||found.Packed>4000000)throw new IOException("Mod config exceeds bounds");
            using(var packed=new MemoryStream()){Copy(found,packed);packed.Position=0;byte[] result;
                if(found.Method==0)result=packed.ToArray();else using(var unzip=new System.IO.Compression.DeflateStream(packed,System.IO.Compression.CompressionMode.Decompress))using(var output=new MemoryStream()){
                    byte[] buffer=new byte[4096];int n;while((n=unzip.Read(buffer,0,buffer.Length))>0){if(output.Length+n>found.Size)throw new IOException("Config decompression exceeds bounds");output.Write(buffer,0,n);}result=output.ToArray();}
                if(result.Length!=found.Size)throw new IOException("Config size mismatch");using(var data=new MemoryStream(result))if(Crc(data,result.Length)!=found.Crc)throw new IOException("Config CRC mismatch");return result;
            }
        }
        sealed class Node {internal ushort Name,Next=65535,Child=65535,File=0x3fff;}
        static void Copy(Entry e,Stream output) {
            if(e.Data!=null){output.Write(e.Data,0,e.Data.Length);return;}
            Regular(e.Source);using(var input=File.OpenRead(e.Source)){input.Position=e.Offset;long left=e.Packed;byte[] bytes=new byte[65536];while(left>0){int n=input.Read(bytes,0,(int)Math.Min(left,bytes.Length));if(n==0)throw new EndOfStreamException();output.Write(bytes,0,n);left-=n;}}
        }
        static uint Position(Stream s){if(s.Position>uint.MaxValue)throw new IOException("ZIP64 mod archives are unsupported");return (uint)s.Position;}
        static void WritePair(string stem,List<Entry> entries) {
            entries.Sort(delegate(Entry a,Entry b){return StringComparer.Ordinal.Compare(a.Name,b.Name);});
            if(entries.Count==0||entries.Count>=0x3fff)throw new IOException("Native archive file limit exceeded");
            var nodes=new List<Node>();var paths=new Dictionary<string,int>(StringComparer.Ordinal);var children=new Dictionary<string,List<int>>(StringComparer.Ordinal);var pool=new MemoryStream();
            for(int file=0;file<entries.Count;file++){string[] parts=entries[file].Name.Split('/');string parent="";
                for(int depth=0;depth<parts.Length;depth++){string path=parent+parts[depth];int index;
                    if(!paths.TryGetValue(path,out index)){
                        byte[] name=Encoding.ASCII.GetBytes(parts[depth]);if(pool.Length+name.Length+1>65535||nodes.Count>=65535)throw new IOException("Native archive tree limit exceeded");
                        index=nodes.Count;paths.Add(path,index);nodes.Add(new Node{Name=(ushort)pool.Length,File=depth==parts.Length-1?(ushort)file:(ushort)0x3fff});pool.Write(name,0,name.Length);pool.WriteByte(0);
                        List<int> siblings;if(!children.TryGetValue(parent,out siblings)){siblings=new List<int>();children.Add(parent,siblings);}siblings.Add(index);
                    }else if(nodes[index].File!=0x3fff||depth==parts.Length-1)throw new IOException("Mod resource file/directory conflict");
                    parent=path+"/";
                }
            }
            foreach(var pair in children){if(pair.Key!="")nodes[paths[pair.Key.TrimEnd('/')]].Child=(ushort)pair.Value[0];for(int i=1;i<pair.Value.Count;i++)nodes[pair.Value[i-1]].Next=(ushort)pair.Value[i];}
            var offsets=new List<uint>();var centrals=new List<uint>();uint total=0;
            using(var f=new FileStream(stem+".zip",FileMode.CreateNew,FileAccess.Write))using(var w=new BinaryWriter(f)){
                foreach(var e in entries){if(e.Data!=null){using(var source=new MemoryStream(e.Data))e.Crc=Crc(source,e.Size);}else if(e.SourceHash!=null){using(var source=File.OpenRead(e.Source))e.Crc=Crc(source,e.Size);}
                    offsets.Add(Position(f));byte[] name=Encoding.ASCII.GetBytes(e.Name);w.Write(0x04034b50u);w.Write((ushort)20);w.Write((ushort)0);w.Write(e.Method);w.Write((ushort)0);w.Write((ushort)33);w.Write(e.Crc);w.Write(e.Packed);w.Write(e.Size);w.Write((ushort)name.Length);w.Write((ushort)0);w.Write(name);Copy(e,f);if(e.SourceHash!=null&&Digest(e.Source)!=e.SourceHash)throw new IOException("Mod changed during cache generation; retry after edits finish");total=unchecked(total+e.Crc);}
                uint central=Position(f);for(int i=0;i<entries.Count;i++){var e=entries[i];byte[] name=Encoding.ASCII.GetBytes(e.Name);centrals.Add(Position(f)-central);w.Write(0x02014b50u);w.Write((ushort)20);w.Write((ushort)20);w.Write((ushort)0);w.Write(e.Method);w.Write((ushort)0);w.Write((ushort)33);w.Write(e.Crc);w.Write(e.Packed);w.Write(e.Size);w.Write((ushort)name.Length);w.Write((ushort)0);w.Write((ushort)0);w.Write((ushort)0);w.Write((ushort)0);w.Write(0u);w.Write(offsets[i]);w.Write(name);}
                uint length=Position(f)-central;w.Write(0x06054b50u);w.Write((ushort)0);w.Write((ushort)0);w.Write((ushort)entries.Count);w.Write((ushort)entries.Count);w.Write(length);w.Write(central);w.Write((ushort)0);
            }
            using(var f=new FileStream(stem+".dat",FileMode.CreateNew,FileAccess.Write))using(var w=new BinaryWriter(f)){
                w.Write(0x0300ffeeu);w.Write((uint)nodes.Count);w.Write((uint)entries.Count);w.Write((uint)pool.Length);w.Write(total);w.Write((ushort)0);w.Write((ushort)65535);w.Write((ushort)children[""][0]);w.Write((ushort)65535);
                foreach(var n in nodes){w.Write(n.Name);w.Write(n.Next);w.Write(n.Child);w.Write(n.File);}
                for(int i=0;i<entries.Count;i++){var e=entries[i];w.Write(2u);w.Write((uint)e.Method);w.Write(e.Packed);w.Write(e.Size);w.Write((uint)e.Name.Length);w.Write(offsets[i]);w.Write(centrals[i]);}w.Write(pool.ToArray());
            }
        }
        static string Cache(string folder,string prefix,string fingerprint,List<Entry> entries) {
            string stem=Path.Combine(folder,prefix+Key(Version+"\n"+fingerprint));
            string receipt=stem+".receipt";
            var candidates=new List<string>();candidates.Add(receipt);
            candidates.AddRange(Directory.GetFiles(folder,Path.GetFileName(stem)+"-*.receipt"));
            foreach(string candidate in candidates){
                string existing=candidate.Substring(0,candidate.Length-8);
                if(!File.Exists(candidate)||!File.Exists(existing+".dat")||!File.Exists(existing+".zip"))continue;
                Regular(candidate);if(new FileInfo(candidate).Length>256)continue;if(File.ReadAllText(candidate)==Version+"\n"+Digest(existing+".dat")+"\n"+Digest(existing+".zip"))return existing;
            }
            // Never overwrite unknown files, an in-use cache, or an incomplete pair.
            if(File.Exists(receipt)||File.Exists(stem+".dat")||File.Exists(stem+".zip"))stem+="-"+Guid.NewGuid().ToString("N");
            string temporary=stem+"-building-"+Guid.NewGuid().ToString("N");
            try{WritePair(temporary,entries);File.Move(temporary+".zip",stem+".zip");File.Move(temporary+".dat",stem+".dat");using(var receiptFile=new FileStream(stem+".receipt",FileMode.CreateNew,FileAccess.Write))using(var writer=new StreamWriter(receiptFile))writer.Write(Version+"\n"+Digest(stem+".dat")+"\n"+Digest(stem+".zip"));}
            finally{foreach(string extension in new[]{".dat",".zip"})if(File.Exists(temporary+extension))File.Delete(temporary+extension);}
            return stem;
        }
        // The native scanner passes the mods directory with a trailing separator.
        // GetDirectoryName would then return the mods directory itself, so car
        // packages must resolve the game root from a trimmed path.
        static string Full(string path) {
            path=Path.GetFullPath(path);string trimmed=path.TrimEnd(Path.DirectorySeparatorChar,Path.AltDirectorySeparatorChar);
            return trimmed.Length==0||trimmed.EndsWith(":",StringComparison.Ordinal)?path:trimmed;
        }
        internal static List<Plan> Prepare(string root,uint used) {
            root=Full(root);Regular(root);
            using(var gate=new FileStream(Path.Combine(root,".car-slots.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None))return PrepareCore(root,used);
        }
        static List<Plan> PrepareCore(string root,uint used) {
            root=Full(root);Regular(root);var slots=new List<int>();for(int id=1;id<32;id++)if((used&(1u<<id))==0)slots.Add(id);
            var stems=new List<string>();string[] folders=Directory.GetDirectories(root);Array.Sort(folders,StringComparer.Ordinal);
            var carPlan=PlanCars(root,folders);
            foreach(string folder in folders){if(carPlan!=null&&carPlan.Folders.Contains(folder))continue;if(Path.GetFileName(folder).StartsWith(".",StringComparison.Ordinal)||(File.GetAttributes(folder)&FileAttributes.ReparsePoint)!=0)continue;
                string assets=Path.Combine(folder,"assets"),legacy=Path.Combine(folder,"vfs");
                bool loose=Directory.Exists(assets),pair=File.Exists(legacy+".dat")&&File.Exists(legacy+".zip");
                if(loose&&pair)throw new IOException("Mod contains both loose assets and VFS: "+folder);
                if(loose){var entries=new List<Entry>();var key=new StringBuilder();Walk(assets,"",entries,key);var seen=new HashSet<string>(StringComparer.Ordinal);foreach(var entry in entries)if(!seen.Add(entry.Name))throw new IOException("Duplicate mod resource name: "+entry.Name);stems.Add(Cache(folder,".reriff-cache-",key.ToString(),entries));}
                else if(pair){Regular(legacy+".dat");Regular(legacy+".zip");if(Directory.GetFiles(folder,"vfs*.dat").Length!=1)throw new IOException("Multiple VFS indexes in mod: "+folder);stems.Add(legacy);}
            }
            if(carPlan!=null&&carPlan.Entries.Count>0)stems.Add(Cache(root,".reriff-cars-",carPlan.Fingerprint,carPlan.Entries));
            if(stems.Count>slots.Count){
                if(slots.Count==0)throw new IOException("Numbered/mounted volumes occupy all 31 native slots");
                int first=slots.Count-1;var merged=new SortedDictionary<string,Entry>(StringComparer.Ordinal);var key=new StringBuilder();
                for(int i=first;i<stems.Count;i++){key.Append(stems[i]).Append('\0').Append(Digest(stems[i]+".dat")).Append(Digest(stems[i]+".zip")).Append('\n');foreach(var e in ReadPair(stems[i]))merged[e.Name]=e;}
                // Deliberately directly under mods/, never inside an individual mod.
                string combined=Cache(root,".reriff-merged-",key.ToString(),new List<Entry>(merged.Values));stems.RemoveRange(first,stems.Count-first);stems.Add(combined);
            }
            var plans=new List<Plan>();for(int i=0;i<stems.Count;i++){if(stems[i].Length>=250)throw new IOException("Native archive path exceeds MAX_PATH");plans.Add(new Plan{Stem=stems[i],Id=slots[i]});}
            if(carPlan!=null)CommitCars(root,carPlan);return plans;
        }
    }
}
