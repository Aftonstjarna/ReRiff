using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using ReRiff;
namespace ReRiff {
    public class ClientConfig {}
    internal static class Logging {internal static void Append(string folder,string file,string text){}}
    internal static class ProgressionDomain {internal static bool Enabled(string folder){throw new NotSupportedException("Rank fixture does not prepare real saves");}}
}
class ProgressionRankSmoke {
    public static IntPtr Achievements;
    private static IntPtr progression;
    private static ProgressionStore.State progress;
    private static ProgressionView.ProfileLevel calculate;
    private static readonly List<int> awards=new List<int>();
    private static bool Has(IntPtr manager,int id){Check(manager==Achievements,"Original manager forwarded");return Marshal.ReadByte(manager,id)!=0;}
    private static void Award(IntPtr manager,int id,bool value){Check(manager==Achievements&&value,"Original award arguments");Check(id>=10&&id<=13,"Only original Rank IDs awarded");Marshal.WriteByte(manager,id,1);awards.Add(id);}
    private static float Select(IntPtr driver,IntPtr xp){return ProgressionRank.LevelFor(progress,driver,xp,calculate);}
    private static float Calculate(IntPtr driver,IntPtr xp){return Math.Min(40,(xp==IntPtr.Zero?Marshal.ReadInt32(progression,280):Marshal.ReadInt32(xp))/1000f);}
    private static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static int Main(string[] args){
        Check(IntPtr.Size==4,"Run in a 32-bit CLR");IntPtr root=Marshal.AllocHGlobal(64),driver=Marshal.AllocHGlobal(12),thresholds=Marshal.AllocHGlobal(41*4);
        Achievements=Marshal.AllocHGlobal(52);progression=Marshal.AllocHGlobal(296);
        try {
            Marshal.Copy(new byte[64],0,root,64);Marshal.Copy(new byte[296],0,progression,296);Marshal.WriteIntPtr(root,32,progression);
            for(int i=0;i<41;i++)Marshal.WriteInt32(thresholds,i*4,i*1000);
            Marshal.WriteIntPtr(driver,thresholds);Marshal.WriteIntPtr(driver,4,IntPtr.Add(thresholds,41*4));Marshal.WriteIntPtr(driver,8,IntPtr.Add(thresholds,41*4));
            calculate=Calculate;
            if(args.Length==1){var type=Assembly.LoadFile(Path.GetFullPath(args[0])).GetType("Fixture.OriginalProgression",true);type.GetField("SaveRoot").SetValue(null,root);calculate=(ProgressionView.ProfileLevel)NativeReelBridge.CreateCall(type.GetMethod("gaDriverLevel.getDriverProfileLevel"),typeof(ProgressionView.ProfileLevel),false);}
            var flags=BindingFlags.Public|BindingFlags.NonPublic|BindingFlags.Static;
            var check=ProgressionRank.Clone(ProgressionRank.OriginalBody(),token=>typeof(ProgressionRankSmoke).GetMethod(token==0x06000e81?"Has":"Award",flags),token=>typeof(ProgressionRankSmoke).GetField("Achievements",flags),typeof(ProgressionRankSmoke).GetMethod("Select",flags));
            int scenarios=0;
            foreach(int mainLevel in new[]{0,4,5,9,10,14,15,19,20,33,40})foreach(uint count in new uint[]{0,1,2})foreach(int cycleLevel in new[]{0,6,20}){
                Marshal.Copy(new byte[52],0,Achievements,52);Marshal.WriteByte(Achievements,18,1);awards.Clear();Marshal.WriteInt32(progression,280,cycleLevel*1000);Marshal.WriteInt32(progression,284,360);
                progress=new ProgressionStore.State(1,(ulong)(mainLevel*1000),cycleLevel*1000,count,new string('0',64));
                byte[] before=new byte[296];Marshal.Copy(progression,before,0,296);check(driver);
                int expected=count==0?cycleLevel:mainLevel;var wanted=new[]{5,10,15,20}.Select((level,index)=>new{level,index}).Where(x=>expected>=x.level).Select(x=>10+x.index).ToArray();
                Check(awards.SequenceEqual(wanted),"Original Rank thresholds with isolated permanent/cycle choice");check(driver);Check(awards.Count==wanted.Length,"No duplicate Rank awards");
                byte[] after=new byte[296];Marshal.Copy(progression,after,0,296);Check(before.SequenceEqual(after),"Level selection never swaps cycle XP, coins or ownership");Check(Marshal.ReadByte(Achievements,18)==1,"First Prestige marker preserved");scenarios++;
            }
            progress=null;Marshal.WriteInt32(progression,280,6000);Marshal.Copy(new byte[52],0,Achievements,52);awards.Clear();check(driver);Check(awards.SequenceEqual(new[]{10}),"No isolated checkpoint retains native behavior");
            byte[] changed=ProgressionRank.OriginalBody();changed[0]^=1;bool refused=false;try{ProgressionRank.Clone(changed,null,null,null);}catch(IOException){refused=true;}Check(refused,"Changed original checker refused");
            ProgressionRank.ValidateBridge();
            Console.WriteLine("PASS: {0} Rank cases preserve original IDs/thresholds and duplicate guards, use permanent level only after prestige, leave cycle XP/coins/ownership unchanged, and validate native ABI; original level fixture={1}",scenarios,args.Length==1);
            return 0;
        }finally{foreach(var pointer in new[]{root,driver,thresholds,Achievements,progression})Marshal.FreeHGlobal(pointer);}
    }
}
