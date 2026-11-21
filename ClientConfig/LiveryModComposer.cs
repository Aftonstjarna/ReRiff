using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
namespace ReRiff {
    // Merge livery-only packages at load time. Each package ships a complete
    // paint table with one selected pack changed; the loader takes only that
    // pack and applies every package's delta to the effective base. The stock
    // archive is used to discover the package's intended delta, so composed
    // custom-car names and other mods' changes are never reverted.
    internal static partial class ModArchiveCache {
        const string LiveryMetadata="livery-maker.json";
        sealed class LiveryPackage {
            internal string Folder,Car;internal int Pack;
            internal Dictionary<string,byte[]> Assets;
            internal Dictionary<string,string> Resources;
        }
        sealed class LiveryPlan {internal HashSet<string> Folders=new HashSet<string>(StringComparer.Ordinal);internal List<Entry> Entries=new List<Entry>();internal string Fingerprint;}
        static bool LiveryFolder(string folder){return File.Exists(Path.Combine(folder,LiveryMetadata));}
        static string DigestBytes(byte[] data){using(var hash=SHA256.Create())return Hex(hash.ComputeHash(data));}
        static Dictionary<string,object> LiveryObject(object value,string label){
            var map=value as Dictionary<string,object>;if(map==null)throw new IOException("Livery metadata is missing "+label);return map;
        }
        static string LiveryString(Dictionary<string,object> map,string key){
            object value;if(!map.TryGetValue(key,out value))throw new IOException("Livery metadata is missing "+key);
            var text=value as string;if(String.IsNullOrEmpty(text))throw new IOException("Livery metadata has an empty "+key);return text;
        }
        static bool CarId(string value){
            if(value==null||value.Length!=3)return false;foreach(char c in value)if(!(c>='a'&&c<='z')&&!(c>='0'&&c<='9'))return false;return true;
        }
        static LiveryPackage ReadLivery(string folder){
            if(File.Exists(Path.Combine(folder,"origin.txt")))throw new IOException("Mod has both car origin and livery metadata: "+folder);
            string metadata=Path.Combine(folder,LiveryMetadata);Regular(metadata);
            if(new FileInfo(metadata).Length>65536)throw new IOException("Livery metadata too large: "+folder);
            var root=LiveryObject(LiveryJson.Parse(File.ReadAllText(metadata)),"document");
            string tool=LiveryString(root,"tool");
            if(!tool.StartsWith("reriff-livery-maker-",StringComparison.Ordinal))throw new IOException("Not a ReRiff livery mod: "+folder);
            string car=LiveryString(root,"selected_car").ToLowerInvariant();
            if(!CarId(car))throw new IOException("Invalid livery car identity in "+folder);
            object packValue;if(!root.TryGetValue("selected_pack_index",out packValue))throw new IOException("Livery metadata is missing selected_pack_index");
            int pack;try{pack=Convert.ToInt32(packValue);}catch(Exception){throw new IOException("Invalid livery paint index in "+folder);}
            if(pack<0||pack>99)throw new IOException("Invalid livery paint index in "+folder);
            var resources=new Dictionary<string,string>(StringComparer.Ordinal);
            object listValue;
            if(root.TryGetValue("resources",out listValue)){
                var list=listValue as List<object>;if(list==null)throw new IOException("Invalid livery resource list in "+folder);
                foreach(var item in list){
                    var entry=LiveryObject(item,"resource");string path=LiveryString(entry,"path").ToLowerInvariant();Name(path);
                    string sha=LiveryString(entry,"sha256").ToLowerInvariant();
                    if(sha.Length!=64||sha.Any(c=>!(c>='0'&&c<='9')&&!(c>='a'&&c<='f')))throw new IOException("Invalid livery resource digest in "+folder);
                    resources[path]=sha;
                }
            }
            string assets=Path.Combine(folder,"assets");
            if(!Directory.Exists(assets))throw new IOException("Livery mod has no loose assets: "+folder);
            var walked=new List<Entry>();Walk(assets,"",walked,new StringBuilder());
            var map=new Dictionary<string,byte[]>(StringComparer.Ordinal);long total=0;
            foreach(var entry in walked){
                if(map.ContainsKey(entry.Name))throw new IOException("Duplicate livery resource name: "+entry.Name);
                total+=entry.Size;if(total>256L*1024*1024)throw new IOException("Livery package exceeds bounds: "+folder);
                map.Add(entry.Name,CarBytes(entry));
            }
            foreach(var asset in map){
                string sha;if(!resources.TryGetValue(asset.Key,out sha))throw new IOException("Unverified livery resource: "+asset.Key+" in "+folder);
                if(DigestBytes(asset.Value)!=sha)throw new IOException("Livery resource changed: "+asset.Key+" in "+folder);
            }
            foreach(var resource in resources)if(!map.ContainsKey(resource.Key))throw new IOException("Livery resource missing: "+resource.Key+" in "+folder);
            return new LiveryPackage{Folder=folder,Car=car,Pack=pack,Assets=map,Resources=resources};
        }
        static Dictionary<string,Entry> LiveryBase(string root,string[] folders,CarsPlan cars){
            string game=Path.GetDirectoryName(root);
            var effective=ReadPair(Path.Combine(game,"vfs"),true).ToDictionary(e=>e.Name,StringComparer.Ordinal);
            foreach(string folder in folders){
                if(Path.GetFileName(folder).StartsWith(".",StringComparison.Ordinal)||(File.GetAttributes(folder)&FileAttributes.ReparsePoint)!=0)continue;
                if(LiveryFolder(folder)||(cars!=null&&cars.Folders.Contains(folder)))continue;
                var extra=new List<Entry>();string assets=Path.Combine(folder,"assets"),legacy=Path.Combine(folder,"vfs");
                if(Directory.Exists(assets))Walk(assets,"",extra,new StringBuilder());
                else if(File.Exists(legacy+".dat")&&File.Exists(legacy+".zip"))extra=ReadPair(legacy);
                foreach(var entry in extra)if(entry.Name.StartsWith("scripts/config/",StringComparison.Ordinal)||entry.Name.StartsWith("strings/",StringComparison.Ordinal))effective[entry.Name]=entry;
            }
            if(cars!=null)foreach(var entry in cars.Entries)effective[entry.Name]=entry;
            return effective;
        }
        static Entry LiveryEntry(string name,byte[] data){Name(name);return new Entry{Name=name,Data=data,Size=(uint)data.Length,Packed=(uint)data.Length};}
        sealed class LiveryReader {
            readonly Dictionary<string,Entry> entries;readonly Dictionary<string,Entry> stock;readonly Dictionary<string,byte[]> memo=new Dictionary<string,byte[]>(StringComparer.Ordinal),original=new Dictionary<string,byte[]>(StringComparer.Ordinal);
            internal LiveryReader(Dictionary<string,Entry> effective,Dictionary<string,Entry> stock){entries=effective;this.stock=stock;}
            internal byte[] Base(string name){
                byte[] data;if(memo.TryGetValue(name,out data))return data;Entry entry;if(!entries.TryGetValue(name,out entry))return null;
                data=entry.Data??CarBytes(entry);memo[name]=data;return data;
            }
            internal byte[] Stock(string name){
                byte[] data;if(original.TryGetValue(name,out data))return data;Entry entry;if(!stock.TryGetValue(name,out entry))return null;
                data=entry.Data??CarBytes(entry);original[name]=data;return data;
            }
        }
        static bool SameNode(NativeCarConfig.Node left,NativeCarConfig.Node right){
            if(ReferenceEquals(left,right))return true;if(left==null||right==null)return false;
            if(left.Name!=right.Name||left.Type!=right.Type)return false;
            if(left.Type==0)return left.Text==right.Text;
            if(left.Type==1)return left.Number==right.Number;
            if(left.Children.Count!=right.Children.Count)return false;
            for(int i=0;i<left.Children.Count;i++)if(!SameNode(left.Children[i],right.Children[i]))return false;
            return true;
        }
        static bool ComposedCar(CarsPlan cars,string car){
            if(cars==null)return false;foreach(var item in cars.After.Cars)if(item.Active&&item.Native==car)return true;return false;
        }
        static bool MergeLiveryConfig(string stem,List<LiveryPackage> packages,CarsPlan cars,LiveryReader reader,Dictionary<string,Entry> output,StringBuilder key){
            string path="scripts/config/"+stem+".cfg";byte[] baseBytes=reader.Base(path);if(baseBytes==null)return false;
            byte[] stockBytes=reader.Stock(path);if(stockBytes==null)throw new IOException("Stock paint config missing: "+path);
            var roots=NativeCarConfig.Read(baseBytes);var paints=NativeCarConfig.Find(roots,"Paints");if(paints==null)throw new IOException("Paint table missing from "+path);
            var stockPaints=NativeCarConfig.Find(NativeCarConfig.Read(stockBytes),"Paints");if(stockPaints==null)throw new IOException("Stock paint table missing");
            bool changed=false;
            foreach(var package in packages){
                byte[] config;if(!package.Assets.TryGetValue(path,out config))continue;
                var packageTree=NativeCarConfig.Read(config);var packagePaints=NativeCarConfig.Find(packageTree,"Paints");
                if(packagePaints==null)throw new IOException("Paint table missing from "+package.Folder);
                var target=NativeCarConfig.Find(paints.Children,package.Car);var source=NativeCarConfig.Find(packagePaints.Children,package.Car);
                if(target==null)throw new IOException("Livery car "+package.Car+" is missing from the composed paint table: "+package.Folder);
                if(source==null)throw new IOException("Livery metadata car "+package.Car+" is missing from its paint table: "+package.Folder);
                if(package.Pack>=target.Children.Count)throw new IOException("Livery paint index outside composed car: "+package.Folder);
                if(package.Pack>=source.Children.Count)throw new IOException("Livery paint index outside package car: "+package.Folder);
                if(!ComposedCar(cars,package.Car)){
                    var stockCar=NativeCarConfig.Find(stockPaints.Children,package.Car);
                    if(stockCar==null)throw new IOException("Unknown stock livery car: "+package.Car+" in "+package.Folder);
                    if(source.Children.Count!=stockCar.Children.Count)throw new IOException("Livery package adds or removes paints instead of changing one: "+package.Folder);
                    if(package.Pack>=stockCar.Children.Count)throw new IOException("Livery paint index outside stock car: "+package.Folder);
                    for(int i=0;i<source.Children.Count;i++)if(i!=package.Pack&&!SameNode(source.Children[i],stockCar.Children[i]))
                        throw new IOException("Livery package changes more than paint "+(package.Pack+1)+": "+package.Folder);
                    // A texture-only package may ship an unchanged table; it is
                    // still valid when its decal replaces a stock paint texture.
                    if(SameNode(source.Children[package.Pack],stockCar.Children[package.Pack]))continue;
                }
                target.Children[package.Pack]=source.Children[package.Pack].Copy();changed=true;
            }
            if(!changed)return false;
            CarModComposer.NormalizeLivery(paints);
            output[path]=LiveryEntry(path,NativeCarConfig.Write(roots));
            string lua="scripts/config/"+stem+".luac";output[lua]=LiveryEntry(lua,NativeCarConfig.Lua(roots,4));
            string lua64="scripts/config/"+stem+".luac64";output[lua64]=LiveryEntry(lua64,NativeCarConfig.Lua(roots,8));
            key.Append("base:").Append(path).Append(':').Append(DigestBytes(baseBytes)).Append('\n');
            return true;
        }
        static List<string> LiveryStrings(byte[] data){
            if(data==null||data.Length<8||BitConverter.ToUInt32(data,0)!=3||BitConverter.ToUInt32(data,4)!=2)throw new IOException("Unsupported livery string bank");
            var result=new List<string>();int at=8;
            while(at<data.Length){
                if(at+4>data.Length)throw new IOException("Truncated livery string bank");uint count=BitConverter.ToUInt32(data,at);at+=4;
                if(count>100000||at+(long)count*2>data.Length)throw new IOException("Invalid livery string bank");result.Add(Encoding.Unicode.GetString(data,at,(int)count*2));at+=(int)count*2;
            }
            return result;
        }
        static byte[] LiveryBank(List<string> strings){
            using(var memory=new MemoryStream())using(var writer=new BinaryWriter(memory)){
                writer.Write(3u);writer.Write(2u);foreach(string text in strings){byte[] data=Encoding.Unicode.GetBytes(text);writer.Write((uint)data.Length/2);writer.Write(data);}return memory.ToArray();
            }
        }
        static bool MergeLiveryStrings(List<LiveryPackage> packages,LiveryReader reader,Dictionary<string,Entry> output,StringBuilder key){
            var changes=new Dictionary<string,Dictionary<int,KeyValuePair<string,string>>>(StringComparer.Ordinal);bool changed=false;
            foreach(var package in packages){
                foreach(var asset in package.Assets){
                    string path=asset.Key;
                    if(path=="strings/tokens.dat")throw new IOException("Livery packages cannot change token order: "+package.Folder);
                    if(!path.StartsWith("strings/",StringComparison.Ordinal)||!path.EndsWith(".str",StringComparison.Ordinal))continue;
                    byte[] baseBytes=reader.Base(path);if(baseBytes==null)throw new IOException("Base string bank missing: "+path+" for "+package.Folder);
                    byte[] stockBytes=reader.Stock(path);if(stockBytes==null)throw new IOException("Stock string bank missing: "+path);
                    var baseStrings=LiveryStrings(baseBytes);var stockStrings=LiveryStrings(stockBytes);var packageStrings=LiveryStrings(asset.Value);
                    if(baseStrings.Count!=stockStrings.Count)throw new IOException("String token order differs from stock: "+path);
                    if(packageStrings.Count!=stockStrings.Count)throw new IOException("Livery string bank differs from stock: "+path+" in "+package.Folder);
                    Dictionary<int,KeyValuePair<string,string>> edits;if(!changes.TryGetValue(path,out edits)){edits=new Dictionary<int,KeyValuePair<string,string>>();changes.Add(path,edits);}
                    for(int i=0;i<packageStrings.Count;i++){
                        if(packageStrings[i]==stockStrings[i])continue;
                        KeyValuePair<string,string> existing;
                        if(edits.TryGetValue(i,out existing)){
                            if(existing.Key!=packageStrings[i])throw new IOException("Two livery mods change the same name: "+path+" index "+i+": "+existing.Value+" and "+package.Folder);
                        }else edits.Add(i,new KeyValuePair<string,string>(packageStrings[i],package.Folder));
                    }
                }
            }
            foreach(var path in changes){
                if(path.Value.Count==0)continue;
                byte[] baseBytes=reader.Base(path.Key);var strings=LiveryStrings(baseBytes);
                foreach(var edit in path.Value)strings[edit.Key]=edit.Value.Key;
                output[path.Key]=LiveryEntry(path.Key,LiveryBank(strings));
                key.Append("base:").Append(path.Key).Append(':').Append(DigestBytes(baseBytes)).Append('\n');changed=true;
            }
            return changed;
        }
        static void MergeLiveryResources(List<LiveryPackage> packages,Dictionary<string,Entry> output){
            foreach(var package in packages)foreach(var asset in package.Assets){
                string path=asset.Key;if(path.StartsWith("scripts/config/",StringComparison.Ordinal)||path.StartsWith("strings/",StringComparison.Ordinal))continue;
                Entry existing;if(output.TryGetValue(path,out existing)){
                    if(!existing.Data.SequenceEqual(asset.Value))throw new IOException("Two livery mods provide different "+path);
                    continue;
                }
                output[path]=LiveryEntry(path,asset.Value);
            }
        }
        // Small JSON reader for livery-maker.json. The file is our own tool's
        // output; this avoids a .NET Framework-only serializer in the loader
        // and in cross-target regression tests.
        sealed class LiveryJson {
            readonly string text;int at;
            LiveryJson(string value){text=value;}
            internal static object Parse(string value){
                if(value==null||value.Length>65536)throw new IOException("Invalid livery metadata size");
                var parser=new LiveryJson(value);object result=parser.Value(0);parser.Skip();
                if(parser.at!=value.Length)throw new IOException("Trailing livery metadata");return result;
            }
            void Skip(){while(at<text.Length&&(text[at]==' '||text[at]=='\t'||text[at]=='\r'||text[at]=='\n'))at++;}
            IOException Bad(){return new IOException("Invalid livery metadata at "+at);}
            object Value(int depth){
                if(depth>16)throw Bad();Skip();if(at>=text.Length)throw Bad();char c=text[at];
                if(c=='{')return Object(depth+1);
                if(c=='[')return Array(depth+1);
                if(c=='"')return String();
                if(c=='t'&&at+4<=text.Length&&text.Substring(at,4)=="true"){at+=4;return true;}
                if(c=='f'&&at+5<=text.Length&&text.Substring(at,5)=="false"){at+=5;return false;}
                if(c=='n'&&at+4<=text.Length&&text.Substring(at,4)=="null"){at+=4;return null;}
                if(c=='-'||(c>='0'&&c<='9'))return Number();
                throw Bad();
            }
            Dictionary<string,object> Object(int depth){
                var result=new Dictionary<string,object>(StringComparer.Ordinal);at++;Skip();
                if(at<text.Length&&text[at]=='}'){at++;return result;}
                while(true){
                    Skip();if(at>=text.Length||text[at]!='"')throw Bad();string key=String();Skip();
                    if(at>=text.Length||text[at]!=':')throw Bad();at++;
                    if(result.ContainsKey(key))throw new IOException("Duplicate livery metadata key");result.Add(key,Value(depth));Skip();
                    if(at>=text.Length)throw Bad();char c=text[at++];if(c=='}')return result;if(c!=',')throw Bad();
                }
            }
            List<object> Array(int depth){
                var result=new List<object>();at++;Skip();
                if(at<text.Length&&text[at]==']'){at++;return result;}
                while(true){
                    result.Add(Value(depth));Skip();if(at>=text.Length)throw Bad();char c=text[at++];
                    if(c==']')return result;if(c!=',')throw Bad();
                }
            }
            string String(){
                if(at>=text.Length||text[at]!='"')throw Bad();at++;var result=new StringBuilder();
                while(at<text.Length){char c=text[at++];
                    if(c=='"')return result.ToString();
                    if(c!='\\'){result.Append(c);continue;}
                    if(at>=text.Length)throw Bad();char escape=text[at++];
                    if(escape=='"'||escape=='\\'||escape=='/')result.Append(escape);
                    else if(escape=='b')result.Append('\b');
                    else if(escape=='f')result.Append('\f');
                    else if(escape=='n')result.Append('\n');
                    else if(escape=='r')result.Append('\r');
                    else if(escape=='t')result.Append('\t');
                    else if(escape=='u'){
                        if(at+4>text.Length)throw Bad();int code;
                        if(!Int32.TryParse(text.Substring(at,4),System.Globalization.NumberStyles.HexNumber,System.Globalization.CultureInfo.InvariantCulture,out code))throw Bad();
                        result.Append((char)code);at+=4;
                    }else throw Bad();
                }
                throw Bad();
            }
            object Number(){
                int start=at;if(at<text.Length&&text[at]=='-')at++;
                while(at<text.Length&&((text[at]>='0'&&text[at]<='9')||text[at]=='.'||text[at]=='e'||text[at]=='E'||text[at]=='+'||text[at]=='-'))at++;
                double value;
                if(!Double.TryParse(text.Substring(start,at-start),System.Globalization.NumberStyles.Float,System.Globalization.CultureInfo.InvariantCulture,out value))throw Bad();
                return value;
            }
        }
        static LiveryPlan PlanLiveries(string root,string[] folders,CarsPlan cars){
            var packages=new List<LiveryPackage>();
            foreach(string folder in folders){
                if(Path.GetFileName(folder).StartsWith(".",StringComparison.Ordinal)||(File.GetAttributes(folder)&FileAttributes.ReparsePoint)!=0)continue;
                if(!LiveryFolder(folder))continue;packages.Add(ReadLivery(folder));
            }
            if(packages.Count==0)return null;
            var targets=new Dictionary<string,string>(StringComparer.Ordinal);
            foreach(var package in packages){
                string target=package.Car+"|"+package.Pack,other;
                if(targets.TryGetValue(target,out other))throw new IOException("Two livery mods target "+package.Car+" paint "+(package.Pack+1)+": "+other+" and "+package.Folder);
                targets.Add(target,package.Folder);
            }
            string game=Path.GetDirectoryName(root);
            var effective=LiveryBase(root,folders,cars);
            var stock=ReadPair(Path.Combine(game,"vfs"),true).ToDictionary(e=>e.Name,StringComparer.Ordinal);
            var reader=new LiveryReader(effective,stock);var plan=new LiveryPlan();var entries=new Dictionary<string,Entry>(StringComparer.Ordinal);
            var key=new StringBuilder("livery-merge-v1\n");key.Append("cars:").Append(cars==null?"":cars.Fingerprint).Append('\n');
            foreach(var package in packages){
                plan.Folders.Add(package.Folder);key.Append(package.Folder).Append('\n');
                foreach(var asset in package.Assets.OrderBy(item=>item.Key,StringComparer.Ordinal))key.Append(asset.Key).Append(':').Append(DigestBytes(asset.Value)).Append('\n');
            }
            MergeLiveryConfig("paint",packages,cars,reader,entries,key);
            MergeLiveryConfig("paint_pc",packages,cars,reader,entries,key);
            MergeLiveryStrings(packages,reader,entries,key);
            MergeLiveryResources(packages,entries);
            if(entries.Count==0)throw new IOException("Livery mods produced no resources");
            plan.Entries=entries.Values.ToList();plan.Fingerprint=key.ToString();return plan;
        }
    }
}
