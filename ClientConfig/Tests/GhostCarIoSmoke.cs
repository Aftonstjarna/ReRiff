using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using ReRiff;
namespace ReRiff {internal static class NativeReelBridge {internal static void Log(string message){Console.WriteLine(message);}}}
namespace ReRiff {internal static partial class ModArchiveCache {internal static CarSlotRegistry.State CurrentCars;}}
class GhostCarIoSmoke {
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static void Text(BinaryWriter writer,string text){byte[] b=Encoding.ASCII.GetBytes(text);writer.Write(b.Length);writer.Write(b);}
    static byte[] Sample(string car="dod",int pack=1){using(var s=new MemoryStream()){using(var w=new BinaryWriter(s)){w.Write(18);w.Write(1);w.Write(120f);Text(w,"123");w.Write(100f);Text(w,"abc");Text(w,car);w.Write(pack);w.Write(80);w.Write(new byte[80]);return s.ToArray();}}}
    static int Main(){
        string root=Path.Combine(Path.GetTempPath(),"reriff-ghost-io-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        string archive=Path.Combine(root,"mods/zz-pulse-clone-test/vfs.zip");Directory.CreateDirectory(Path.GetDirectoryName(archive));File.WriteAllBytes(archive,new byte[]{1,2,3});
        File.WriteAllText(Path.Combine(root,"client.ini"),"[diagnostics]\ncar_ghost_test=1\n");
        ModArchiveCache.CurrentCars=CarSlotRegistry.Plan(null,CarSlotRegistry.Package("donor-mod","overwrite=0\n[origins]\nr00=cor"));
        byte[] identity=GhostCarTestFormat.Digest(new byte[]{1,2,3},3),raw=Sample();
        var type=typeof(GhostCarTest);type.GetField("folder",BindingFlags.Static|BindingFlags.NonPublic).SetValue(null,root);
        var run=type.GetMethod("Run",BindingFlags.Static|BindingFlags.NonPublic);
        IntPtr directory=Marshal.StringToHGlobalUni(root),name=Marshal.StringToHGlobalAnsi("abc-123.ghost"),patch=Marshal.AllocHGlobal(16);
        Func<int,int,int> invoke=(phase,installed)=>(int)run.Invoke(null,new object[]{directory,name,phase,installed,patch});
        string path=Path.Combine(root,"abc-123.ghost");File.WriteAllBytes(path,raw);
        try{
            Check(invoke(1,1)==1,"Publication failed");byte[] wire=File.ReadAllBytes(path);
            Check(File.ReadAllBytes(path+".reriff-car-original").SequenceEqual(raw),"Original not backed up");
            Check(GhostCarTestFormat.ReadHeader(wire).Car=="alf","Nonstock wire car");
            Check(wire.Length==raw.Length,"Identity trailer added");
            Check(invoke(0,0)==0&&invoke(0,1)==0,"Custom read overlay enabled");
            Check(File.ReadAllBytes(path).SequenceEqual(wire),"Canonical read changed replay");
            Check(invoke(1,1)==0,"Already shared ghost rewritten");
            File.WriteAllBytes(path,raw);Check(invoke(0,0)==0&&GhostCarTestFormat.ReadHeader(File.ReadAllBytes(path)).Car=="alf","Old cached ghost not normalized before read");
            File.WriteAllBytes(path,raw);
            File.WriteAllBytes(archive,new byte[]{1,2,3});File.WriteAllText(path+".reriff-car-new","keep");
            Check(invoke(1,1)==-1&&File.ReadAllBytes(path).SequenceEqual(raw)&&File.ReadAllText(path+".reriff-car-new")=="keep","Existing staging file overwritten");
            File.Delete(path+".reriff-car-new");
            File.WriteAllText(Path.Combine(root,"client.ini"),"[diagnostics]\ncar_ghost_test=0\n");
            string customPath=Path.Combine(root,"def-123.ghost");File.WriteAllBytes(customPath,Sample("r00",4));
            IntPtr customName=Marshal.StringToHGlobalAnsi("def-123.ghost");
            try{
                Func<int,int> customInvoke=phase=>(int)run.Invoke(null,new object[]{directory,customName,phase,1,patch});
                byte[] customOriginal=File.ReadAllBytes(customPath);
                Check(customInvoke(0)==-1&&File.ReadAllBytes(customPath).SequenceEqual(customOriginal)&&!File.Exists(customPath+".reriff-car-original"),"Cached runtime-car read was normalized using a potentially reassigned slot");
                Check(customInvoke(1)==1,"Allocated-car publication failed without diagnostic opt-in");
                var donor=GhostCarTestFormat.ReadHeader(File.ReadAllBytes(customPath));Check(donor.Car=="cor"&&donor.Pack==0,"Runtime car did not persist stock donor header");
                Check(File.Exists(customPath+".reriff-car-original"),"Runtime-car original was not backed up");
                byte[] published=File.ReadAllBytes(customPath);Check(customInvoke(0)==0&&File.ReadAllBytes(customPath).SequenceEqual(published),"Published stock-donor ghost was not reusable for reads");
                string unknownPath=Path.Combine(root,"ghi-123.ghost");byte[] unknown=Sample("r06",2);File.WriteAllBytes(unknownPath,unknown);
                IntPtr unknownName=Marshal.StringToHGlobalAnsi("ghi-123.ghost");
                try{Func<int,int> unknownInvoke=phase=>(int)run.Invoke(null,new object[]{directory,unknownName,phase,1,patch});Check(unknownInvoke(1)==-1&&File.ReadAllBytes(unknownPath).SequenceEqual(unknown),"Unmapped runtime ID was published or modified");}
                finally{Marshal.FreeHGlobal(unknownName);}
            }finally{Marshal.FreeHGlobal(customName);}
            Console.WriteLine("PASS legacy fallback, phase-gated custom donor publication, cached runtime-ID read refusal, stock reread, backup, idempotency and write-failure preservation");return 0;
        }finally{Marshal.FreeHGlobal(directory);Marshal.FreeHGlobal(name);Marshal.FreeHGlobal(patch);Directory.Delete(root,true);}
    }
}
