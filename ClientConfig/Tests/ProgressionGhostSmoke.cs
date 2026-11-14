using System;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using ReRiff;
namespace ReRiff {
    public class ClientConfig {}
    internal static class Logging {internal static void Append(string folder,string file,string text){}}
    internal static class ProgressionDomain {internal static bool Enabled(string folder){throw new NotSupportedException("Ghost fixture does not prepare real saves");}}
}
class ProgressionGhostSmoke {
    private static IntPtr achievements;
    private static int awards;
    private static bool Has(IntPtr manager,int id){Check(manager==achievements&&id==32,"Original achievement ID and manager");return Marshal.ReadByte(manager,id)!=0;}
    private static void Award(IntPtr manager,int id,bool value){Check(manager==achievements&&id==32&&value,"Only award missing Friend or Foe");awards++;Marshal.WriteByte(manager,id,1);}
    private static IntPtr GetScoreManager(IntPtr player){return Marshal.ReadIntPtr(player,608);}
    private static float GetScore(IntPtr score){byte[] raw=new byte[4];Marshal.Copy(IntPtr.Add(score,4),raw,0,4);return (float)Math.Floor(BitConverter.ToSingle(raw,0));}
    private static void Check(bool value,string text){if(!value)throw new Exception(text);}
    private static byte[] Read(IntPtr p,int count){byte[] b=new byte[count];Marshal.Copy(p,b,0,count);return b;}
    private static void Float(IntPtr p,int offset,float value){Marshal.Copy(BitConverter.GetBytes(value),0,IntPtr.Add(p,offset),4);}
    static int Main(string[] args){
        Check(IntPtr.Size==4,"Run in a 32-bit CLR");IntPtr config=Marshal.AllocHGlobal(312),players=Marshal.AllocHGlobal(52),list=Marshal.AllocHGlobal(4),player=Marshal.AllocHGlobal(612),score=Marshal.AllocHGlobal(8);achievements=Marshal.AllocHGlobal(52);
        try {
            foreach(var item in new[]{Tuple.Create(config,312),Tuple.Create(players,52),Tuple.Create(player,612),Tuple.Create(score,8)})Marshal.Copy(new byte[item.Item2],0,item.Item1,item.Item2);
            Marshal.WriteIntPtr(players,48,list);Marshal.WriteIntPtr(list,player);Marshal.WriteIntPtr(player,608,score);
            GhostAchievement.ScoreManager getManager=GetScoreManager;GhostAchievement.Score getScore=GetScore;
            if(args.Length==1){var type=Assembly.LoadFile(System.IO.Path.GetFullPath(args[0])).GetType("Fixture.OriginalProgression",true);getManager=(GhostAchievement.ScoreManager)NativeReelBridge.CreateCall(type.GetMethod("gaPlayerLocal.getScoreManager"),typeof(GhostAchievement.ScoreManager),false);getScore=(GhostAchievement.Score)NativeReelBridge.CreateCall(type.GetMethod("gaScoreManager.getScore"),typeof(GhostAchievement.Score),false);}
            int reads=0;var adapter=new GhostAchievement(()=>{reads++;return config;},()=>players,()=>achievements,getManager,getScore,Has,Award);int cases=0;
            float[][] scores={new[]{99f,100f},new[]{100f,100f},new[]{101f,100f},new[]{100.9f,100f},new[]{101.9f,101.5f},new[]{Single.NaN,100f},new[]{101f,Single.NaN},new[]{Single.PositiveInfinity,100f},new[]{Single.PositiveInfinity,Single.PositiveInfinity},new[]{0f,0f}};
            foreach(bool sp in new[]{false,true})foreach(int mode in new[]{0,1,2,3,4})foreach(bool existing in new[]{false,true})foreach(var pair in scores){
                Marshal.Copy(new byte[52],0,achievements,52);Marshal.WriteByte(achievements,18,1);Marshal.WriteByte(achievements,32,existing?(byte)1:(byte)0);awards=0;
                Marshal.WriteInt32(config,304,mode);Float(config,308,pair[1]);Float(score,4,pair[0]);byte[] before=Read(achievements,52),configBefore=Read(config,312),playerBefore=Read(player,612),scoreBefore=Read(score,8);
                bool expected=sp&&(mode==2||mode==3)&&!existing&&(float)Math.Floor(pair[0])>pair[1];int previousReads=reads;bool earned=adapter.CheckResult(sp);
                Check(earned==expected&&awards==(expected?1:0),"Native win requirement, rounding, mode and singleplayer scope");
                if(!sp)Check(previousReads==reads,"Multiplayer bypass does not inspect or change game state");
                byte[] after=Read(achievements,52);before[32]=existing||expected?(byte)1:(byte)0;Check(before.SequenceEqual(after),"Carried-over completion and all other flags retained");
                Check(configBefore.SequenceEqual(Read(config,312))&&playerBefore.SequenceEqual(Read(player,612))&&scoreBefore.SequenceEqual(Read(score,8)),"No race/score mutation");
                Check(!adapter.CheckResult(sp)&&awards==(expected?1:0),"Repeated results cannot duplicate the award");cases++;
            }
            ProgressionGhost.ValidateBridge();Console.WriteLine("PASS: {0} ghost cases preserve carried-over Friend or Foe, require a strict rounded-score win against a normal ghost, skip multiplayer/no-ghost/friend/loss/tie/NaN, and validate callback ABI; original score fixture={1}",cases,args.Length==1);return 0;
        }finally{foreach(var p in new[]{config,players,list,player,score,achievements})Marshal.FreeHGlobal(p);}
    }
}
