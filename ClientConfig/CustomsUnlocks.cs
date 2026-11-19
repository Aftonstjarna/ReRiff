// Extend the original track-summary music check; award through unlockCar.
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using System.Text;
namespace ReRiff {
    internal static class CustomsUnlocks {
        internal sealed class Rule {internal string Car,Artist,Title;}
        internal static List<Rule> Rules=Defaults();
        static bool custom;
        internal static List<Rule> Defaults(){return new List<Rule>{new Rule{Car="pl1",Artist="The Police",Title=""},new Rule{Car="f1r",Artist="Fleetwood Mac",Title="Chain"}};}
        internal sealed class Node {internal string Name,Text;internal uint Type;internal List<Node> Children;}
        internal static List<Rule> Parse(byte[] b){
            if(b==null||b.Length<20||b.Length>4000000||BitConverter.ToUInt32(b,0)!=0x46434647||BitConverter.ToUInt32(b,4)!=3)throw new IOException("Invalid unlock CFG");
            uint count=BitConverter.ToUInt32(b,12),roots=BitConverter.ToUInt32(b,16);long poolLong=20L+count*32L;if(count==0||count>100000||roots>count||poolLong+BitConverter.ToUInt32(b,8)!=b.Length)throw new IOException("Invalid unlock CFG bounds");int pool=(int)poolLong;
            Func<uint,string> text=delegate(uint off){if(off>=b.Length-pool)throw new IOException("Invalid CFG string");int start=pool+(int)off,end=start;while(end<b.Length&&b[end]!=0)end++;if(end==b.Length)throw new IOException("Unterminated CFG string");return new UTF8Encoding(false,true).GetString(b,start,end-start);};
            var visited=new HashSet<uint>();Func<uint,int,Node> read=null;read=delegate(uint index,int depth){
                if(index>=count||depth>100||!visited.Add(index))throw new IOException("Invalid CFG tree");int p=20+(int)index*32;uint value=BitConverter.ToUInt32(b,p+8),type=BitConverter.ToUInt32(b,p+20),size=BitConverter.ToUInt32(b,p+24);
                if(BitConverter.ToUInt32(b,p+4)!=0||BitConverter.ToUInt32(b,p+12)!=0||BitConverter.ToUInt32(b,p+28)!=0||type>2||(type!=2&&size!=0))throw new IOException("Invalid CFG record");
                var n=new Node{Name=text(BitConverter.ToUInt32(b,p)),Type=type,Children=new List<Node>()};uint hash=5387;foreach(byte c in Encoding.UTF8.GetBytes(n.Name))hash=unchecked(hash*33+c);if(hash!=BitConverter.ToUInt32(b,p+16))throw new IOException("Invalid CFG name hash");
                if(type==0)n.Text=text(value);else if(type==1){float f=BitConverter.ToSingle(b,p+8);if(float.IsNaN(f)||float.IsInfinity(f))throw new IOException("Invalid CFG number");}else{if(value>count||size>count-value)throw new IOException("Invalid CFG child range");for(uint i=0;i<size;i++)n.Children.Add(read(value+i,depth+1));}return n;
            };
            Node names=null,unlocks=null;for(uint i=0;i<roots;i++){var n=read(i,0);if(n.Name=="carNames")names=n;if(n.Name=="carUnlocks")unlocks=n;}if(visited.Count!=count||names==null||unlocks==null||names.Type!=2||unlocks.Type!=2)throw new IOException("Missing unlock tables");
            var rules=new List<Rule>();var cars=new HashSet<string>(StringComparer.Ordinal);
            for(int i=0;i<unlocks.Children.Count;i++){var n=unlocks.Children[i];if(n.Type!=2)continue;if(i>=names.Children.Count||names.Children[i].Type!=0)throw new IOException("Missing unlock car ID");string car=names.Children[i].Text;if(car.Length!=3||!cars.Add(car))throw new IOException("Invalid unlock car ID");string artist=null,title="";
                foreach(var f in n.Children){if(f.Name=="artistUnlock"){if(f.Type!=0)throw new IOException("Invalid artist unlock");artist=f.Text;}if(f.Name=="songUnlock"){if(f.Type!=0)throw new IOException("Invalid title unlock");title=f.Text;}}
                if(artist!=null){if(string.IsNullOrWhiteSpace(artist)||artist.Length>200||title.Length>200)throw new IOException("Invalid music unlock text");rules.Add(new Rule{Car=car,Artist=artist,Title=title});}
            }return rules;
        }
        internal static void Load(byte[] config){var next=config==null?Defaults():Parse(config);Rules=next;custom=config!=null;}
        internal static void LoadPlan(List<ModArchiveCache.Plan> plans){
            byte[] marker=ModArchiveCache.Resource(plans,"scripts/config/customs_unlocks.txt");
            if(marker==null){Load(null);return;}
            string[] lines=Encoding.ASCII.GetString(marker).TrimEnd('\n').Split('\n');
            if(lines.Length!=3||lines[0]!="reriff-customs-unlocks-v1")throw new IOException("Unknown Customs unlock manifest");
            byte[] cfg=ModArchiveCache.Resource(plans,"scripts/config/unlocks_pc.cfg"),lua=ModArchiveCache.Resource(plans,"scripts/config/unlocks_pc.luac");
            using(var hash=System.Security.Cryptography.SHA256.Create()){
                if(cfg==null||lua==null||BitConverter.ToString(hash.ComputeHash(cfg)).Replace("-","").ToLowerInvariant()!=lines[1]||BitConverter.ToString(hash.ComputeHash(lua)).Replace("-","").ToLowerInvariant()!=lines[2])throw new IOException("Customs unlock files were overridden or changed independently");
            }
            Load(cfg);
        }
        internal static bool Matches(Rule r,string artist,string title){return artist.IndexOf(r.Artist,StringComparison.OrdinalIgnoreCase)>=0&&(r.Title.Length==0||title.IndexOf(r.Title,StringComparison.OrdinalIgnoreCase)>=0);}
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]delegate void Check(IntPtr artist,IntPtr title);
        delegate int CarIndex(IntPtr self,IntPtr name);delegate void Unlock(IntPtr self,int car);
        static CarIndex carIndex;static Unlock unlock;static Func<IntPtr> paint,manager;
        static readonly Check callback=Run;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]static extern int ReRiffConfigureCustomsUnlocks(IntPtr callback);
        static Func<IntPtr> Singleton(Module game,int token,string name){var field=game.ResolveField(token);if(field.Name!=name||!field.IsStatic||!field.FieldType.IsPointer)throw new IOException("Wrong native unlock singleton");var d=new DynamicMethod("Customs_"+token,typeof(IntPtr),Type.EmptyTypes,game,true);var il=d.GetILGenerator();il.Emit(OpCodes.Ldsfld,field);il.Emit(OpCodes.Conv_I);il.Emit(OpCodes.Ret);return (Func<IntPtr>)d.CreateDelegate(typeof(Func<IntPtr>));}
        internal static void Configure(Module game){
            var lookup=(MethodInfo)game.ResolveMethod(0x06001391);var award=(MethodInfo)game.ResolveMethod(0x060012d2);if(lookup.Name!="gaVehiclePaintManager.getCarIndexFromVisName"||award.Name!="gaUnlockManager.unlockCar")throw new IOException("Wrong native unlock methods");
            carIndex=(CarIndex)NativeReelBridge.CreateCall(lookup,typeof(CarIndex),false);unlock=(Unlock)NativeReelBridge.CreateCall(award,typeof(Unlock),false);paint=Singleton(game,0x0400147d,"?ms_pInstance@gaVehiclePaintManager@@0PAV1@A");manager=Singleton(game,0x04001342,"?ms_pInstance@gaUnlockManager@@0PAV1@A");
            if(ReRiffConfigureCustomsUnlocks(Marshal.GetFunctionPointerForDelegate(callback))!=1)throw new IOException("Customs unlock hook unavailable");
        }
        static string NativeString(IntPtr address){var b=ProgressionNative.Read(address,24);uint length=BitConverter.ToUInt32(b,16),capacity=BitConverter.ToUInt32(b,20);if(length>65535||length>capacity)throw new IOException("Invalid native song metadata");byte[] text=capacity<16?b:ProgressionNative.Read(new IntPtr(BitConverter.ToInt32(b,0)),(int)length);return Encoding.UTF8.GetString(text,0,(int)length);}
        static void Run(IntPtr artistAddress,IntPtr titleAddress){try{
            string artist=NativeString(artistAddress),title=NativeString(titleAddress);var p=paint();var m=manager();if(p==IntPtr.Zero||m==IntPtr.Zero)return;
            foreach(var rule in Rules)if(Matches(rule,artist,title)){var name=Marshal.StringToHGlobalAnsi(rule.Car);try{int i=carIndex(p,name);if(i>=0&&i<22)unlock(m,i);}finally{Marshal.FreeHGlobal(name);}if(!custom)break;}
        }catch(Exception e){NativeReelBridge.Log("Customs music unlock skipped: "+e.Message);}}
    }
}
