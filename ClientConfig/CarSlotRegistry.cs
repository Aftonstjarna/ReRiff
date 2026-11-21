// Intentional ReRiff allocation policy; native slots and saves remain fixed at 22.
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
namespace ReRiff {
    internal static class CarSlotRegistry {
        internal static readonly string[] Stock={"alf","bui","cor","rs1","vau","fab","hon","cad","rol","mer","mot","pha","che","f1r","pl1"};
        internal sealed class Car {
            internal string Key,Package,Source,Base,Native;
            internal int Slot=-1;
            internal bool Active,Overwrite;
            internal Car Copy(){return (Car)MemberwiseClone();}
        }
        internal sealed class State {
            internal readonly List<Car> Cars=new List<Car>();
            internal string Revision;
        }
        internal static string Hash(string value){using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(Encoding.UTF8.GetBytes(value))).Replace("-","").ToLowerInvariant();}
        static void Check(bool condition,string message){if(!condition)throw new IOException("Car slots: "+message);}
        static bool Id(string value){if(value.Length!=3)return false;foreach(char c in value)if(!(c>='a'&&c<='z')&&!(c>='0'&&c<='9'))return false;return true;}
        internal static Dictionary<string,Dictionary<string,string>> Ini(string text){
            var result=new Dictionary<string,Dictionary<string,string>>(StringComparer.Ordinal);string section="mod";result[section]=new Dictionary<string,string>(StringComparer.Ordinal);
            foreach(string raw in text.Split('\n')){string line=raw.Trim();if(line.Length==0||line[0]==';'||line[0]=='#')continue;
                if(line[0]=='['&&line.EndsWith("]",StringComparison.Ordinal)){section=line.Substring(1,line.Length-2);Check(section.Length>0,"empty section");if(!result.ContainsKey(section))result[section]=new Dictionary<string,string>(StringComparer.Ordinal);continue;}
                int split=line.IndexOf('=');Check(split>0,"invalid INI line");string key=line.Substring(0,split).Trim(),value=line.Substring(split+1).Trim();
                Check(!result[section].ContainsKey(key),"duplicate key "+key);result[section][key]=value;
            }return result;
        }
        internal static List<Car> Package(string folder,string text){
            Check(text.Length<=65536,"origin.txt too large");var ini=Ini(text);string mode=null,identity=null;
            foreach(var section in ini.Values){string value;if(section.TryGetValue("overwrite",out value)){Check(mode==null,"duplicate overwrite flag");mode=value;}if(section.TryGetValue("id",out value)){Check(identity==null,"duplicate package id");identity=value;}}
            // Legacy origin-only packages retain their authored native layout.
            if(mode==null)return new List<Car>();Check(mode=="0"||mode=="1","overwrite must be 0 or 1");
            if(identity==null)identity=Path.GetFileName(folder);Check(SafeName(identity,128),"invalid package identity");Check(SafeName(Path.GetFileName(folder),255),"invalid package folder name");
            Dictionary<string,string> origins;Check(ini.TryGetValue("origins",out origins),"missing [origins]");var cars=new List<Car>();
            foreach(var item in origins){if(item.Key=="overwrite"||item.Key=="id")continue;Check(Id(item.Key)&&Array.IndexOf(Stock,item.Value)>=0,"invalid car/donor ID");
                cars.Add(new Car{Key=identity+"|"+item.Key,Package=Path.GetFileName(folder),Source=item.Key,Base=item.Value,Active=true,Overwrite=mode=="1"});}
            Check(cars.Count>0,"no donor mappings");cars.Sort((a,b)=>StringComparer.Ordinal.Compare(a.Key,b.Key));return cars;
        }
        internal static State Plan(State previous,IEnumerable<Car> requested){
            var next=new State();var old=new Dictionary<string,Car>(StringComparer.Ordinal);if(previous!=null)foreach(var car in previous.Cars){Check(!old.ContainsKey(car.Key),"duplicate registry identity");old.Add(car.Key,car);var copy=car.Copy();copy.Active=false;copy.Slot=-1;next.Cars.Add(copy);}
            var active=new List<Car>();var keys=new HashSet<string>(StringComparer.Ordinal);var donors=new HashSet<string>(StringComparer.Ordinal);
            foreach(var request in requested){Check(keys.Add(request.Key),"duplicate package identity "+request.Key);Check(Array.IndexOf(Stock,request.Base)>=0,"unknown donor");var car=request.Copy();car.Active=true;if(car.Overwrite)Check(donors.Add(car.Base),"multiple replacements for donor "+car.Base);next.Cars.RemoveAll(c=>c.Key==car.Key);next.Cars.Add(car);active.Add(car);}
            active.Sort((a,b)=>StringComparer.Ordinal.Compare(a.Key,b.Key));int count=0;foreach(var car in active)if(!car.Overwrite)count++;Check(count<=7,"only seven custom slots are available");
            // Compact active slots, retaining old relative order where possible.
            // Any movement is accompanied by identity-based save migration.
            var added=active.FindAll(c=>!c.Overwrite);added.Sort(delegate(Car a,Car b){Car x,y;int sx=old.TryGetValue(a.Key,out x)&&x.Slot>=15?x.Slot:22,sy=old.TryGetValue(b.Key,out y)&&y.Slot>=15?y.Slot:22;int order=sx.CompareTo(sy);return order!=0?order:StringComparer.Ordinal.Compare(a.Key,b.Key);});
            var used=new HashSet<string>(StringComparer.Ordinal);
            foreach(var car in active)if(car.Overwrite)used.Add(car.Base);
            for(int i=0;i<added.Count;i++){
                // Prefer the authored identity when it is not a stock car and is
                // unique: the stock localization already has its car token.
                string native=added[i].Source;
                if(Array.IndexOf(Stock,native)>=0||used.Contains(native)){native=null;for(int n=0;n<7;n++){string candidate="r"+n.ToString("D2");if(!used.Contains(candidate)){native=candidate;break;}}Check(native!=null,"only seven native identities are available");}
                added[i].Slot=15+i;added[i].Native=native;used.Add(native);
            }
            foreach(var car in active)if(car.Overwrite){car.Slot=Array.IndexOf(Stock,car.Base);car.Native=car.Base;}
            Check(next.Cars.Count<=4096,"too many registry cars");next.Cars.Sort((a,b)=>StringComparer.Ordinal.Compare(a.Key,b.Key));next.Revision=Hash(Encode(next,false));return next;
        }
        internal static string Encode(State state,bool revision=true){
            var b=new StringBuilder("[registry]\nschema=1\n");if(revision)b.Append("revision=").Append(state.Revision).Append('\n');
            foreach(var car in state.Cars){b.Append("\n[car_").Append(Hash(car.Key)).Append("]\nkey=").Append(car.Key).Append("\npackage=").Append(car.Package).Append("\nsource=").Append(car.Source).Append("\nbase=").Append(car.Base).Append("\nnative=").Append(car.Native??"").Append("\nslot=").Append(car.Slot).Append("\nactive=").Append(car.Active?1:0).Append("\noverwrite=").Append(car.Overwrite?1:0).Append('\n');}return b.ToString();
        }
        static bool SafeName(string value,int limit){
            if(value.Length==0||value.Length>limit||value=="."||value==".."||value!=value.Trim())return false;
            foreach(char c in value)if(Char.IsControl(c)||c=='|'||c=='='||c=='['||c==']'||c=='/'||c=='\\')return false;return true;
        }
        internal static State Decode(string text){
            Check(text!=null,"missing registry");
            var ini=Ini(text);Dictionary<string,string> header;Check(ini.TryGetValue("registry",out header)&&header.ContainsKey("schema")&&header["schema"]=="1","unsupported registry");var state=new State();
            var identities=new HashSet<string>(StringComparer.Ordinal);var slots=new HashSet<int>();
            foreach(var item in ini){if(!item.Key.StartsWith("car_",StringComparison.Ordinal))continue;var v=item.Value;
                Check(state.Cars.Count<4096,"too many registry cars");
                foreach(string field in new[]{"key","package","source","base","native","slot","active","overwrite"})Check(v.ContainsKey(field),"missing car field "+field);
                int slot;Check(Int32.TryParse(v["slot"],out slot),"invalid slot");
                Check((v["active"]=="0"||v["active"]=="1")&&(v["overwrite"]=="0"||v["overwrite"]=="1"),"invalid car flag");
                var car=new Car{Key=v["key"],Package=v["package"],Source=v["source"],Base=v["base"],Native=v["native"],Slot=slot,Active=v["active"]=="1",Overwrite=v["overwrite"]=="1"};
                int split=car.Key.IndexOf('|');
                Check(split>0&&SafeName(car.Key.Substring(0,split),128)&&car.Key.Substring(split+1)==car.Source&&Id(car.Source),"invalid registry identity");
                Check(SafeName(car.Package,255)&&item.Key=="car_"+Hash(car.Key)&&identities.Add(car.Key),"invalid or duplicate registry car");
                int donor=Array.IndexOf(Stock,car.Base);Check(donor>=0,"unknown registry donor");
                if(car.Active){
                    Check(slots.Add(slot),"duplicate active car slot");
                    if(car.Overwrite)Check(slot==donor&&car.Native==car.Base,"invalid donor replacement slot");
                    else Check(slot>=15&&slot<22&&(car.Native=="r"+(slot-15).ToString("D2")||(car.Native==car.Source&&Array.IndexOf(Stock,car.Source)<0)),"invalid custom car slot");
                }else{
                    Check(slot==-1,"inactive car occupies a slot");
                    // Plan retains the former native ID when an identity is retired.
                    if(car.Overwrite)Check(car.Native==car.Base,"invalid retired donor replacement");
                    else Check((car.Native.Length==3&&car.Native[0]=='r'&&car.Native[1]=='0'&&car.Native[2]>='0'&&car.Native[2]<='6')||(car.Native==car.Source&&Array.IndexOf(Stock,car.Source)<0),"invalid retired custom car ID");
                }
                state.Cars.Add(car);
            }
            state.Revision=Hash(Encode(state,false));Check(header.ContainsKey("revision")&&header["revision"]==state.Revision,"registry checksum mismatch");return state;
        }
    }
}
