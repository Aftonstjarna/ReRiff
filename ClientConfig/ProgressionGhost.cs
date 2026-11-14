using System;
using System.IO;
using System.Globalization;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

namespace ReRiff {
    // Extend the original winning-ghost result, using original score rounding
    // and achievement awards. No existing completion is cleared or rewritten.
    internal sealed class GhostAchievement {
        internal delegate IntPtr ScoreManager(IntPtr player);
        internal delegate float Score(IntPtr manager);
        internal delegate bool Has(IntPtr manager,int id);
        internal delegate void Award(IntPtr manager,int id,bool value);
        private readonly Func<IntPtr> config,players,achievements;
        private readonly ScoreManager scoreManager;
        private readonly Score score;
        private readonly Has has;
        private readonly Award award;
        private readonly Action<string> trace;
        internal GhostAchievement(Func<IntPtr> config,Func<IntPtr> players,Func<IntPtr> achievements,ScoreManager scoreManager,Score score,Has has,Award award,Action<string> trace=null){
            this.config=config;this.players=players;this.achievements=achievements;this.scoreManager=scoreManager;this.score=score;this.has=has;this.award=award;this.trace=trace;
        }
        internal bool CheckResult(bool singleplayer){
            if(!singleplayer){Trace("singleplayer=0 skipped=multiplayer");return false;}
            byte[] race=ProgressionNative.Read(config(),312);
            int kind=BitConverter.ToInt32(race,304);
            if(kind!=2&&kind!=3){Trace("singleplayer=1 ghost_kind="+kind+" skipped=not-normal-ghost");return false;}
            IntPtr manager=achievements();
            if(has(manager,32)){Trace("singleplayer=1 ghost_kind="+kind+" already_earned=1 preserved=1");return false;}
            byte[] playerManager=ProgressionNative.Read(players(),52);
            IntPtr first=new IntPtr(BitConverter.ToInt32(playerManager,48));
            if(first==IntPtr.Zero){Trace("skipped=no-player-list");return false;}
            IntPtr player=new IntPtr(BitConverter.ToInt32(ProgressionNative.Read(first,4),0));
            if(player==IntPtr.Zero){Trace("skipped=no-local-player");return false;}
            IntPtr scoring=scoreManager(player);if(scoring==IntPtr.Zero){Trace("skipped=no-score-manager");return false;}
            float result=score(scoring),target=BitConverter.ToSingle(race,308);
            // As in original ble.un, a loss, tie or unordered NaN does not win.
            Trace("singleplayer=1 ghost_kind="+kind+" player_score="+result.ToString("R",CultureInfo.InvariantCulture)+" ghost_score="+target.ToString("R",CultureInfo.InvariantCulture)+" won="+(result>target?1:0));
            if(!(result>target))return false;
            award(manager,32,true);return true;
        }
        private void Trace(string text){if(trace!=null)trace("Ghost achievement result: "+text);}
    }
    internal static class ProgressionGhost {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Callback();
        private static readonly Callback NativeCallback=Dispatch;
        private static GhostAchievement adapter;
        private static volatile bool singleplayer;
        private static bool validating,errorLogged;
        private static int probeCalls;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureGhostAchievement(IntPtr callback,int validated);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern IntPtr ReRiffGetGhostAchievementThunk();
        internal static void SetSingleplayer(bool enabled){singleplayer=enabled;NativeReelBridge.Log("Ghost achievement mode: singleplayer="+(enabled?1:0));}
        private static void Dispatch(){
            if(validating){probeCalls++;return;}
            if(!singleplayer||adapter==null)return;
            try{if(adapter.CheckResult(true))NativeReelBridge.Log("Friend or Foe awarded: singleplayer normal ghost beaten; original achievement path.");}
            catch(Exception error){if(!errorLogged){errorLogged=true;NativeReelBridge.Log("Singleplayer ghost achievement check unavailable: "+error);}}
        }
        internal static void ValidateBridge(){
            IntPtr pointer=Marshal.GetFunctionPointerForDelegate(NativeCallback);
            if(ReRiffConfigureGhostAchievement(pointer,0)!=1)throw new IOException("Ghost achievement callback registration refused");
            IntPtr target=ReRiffGetGhostAchievementThunk();if(target==IntPtr.Zero)throw new IOException("Ghost achievement thunk missing");
            var dynamic=new DynamicMethod("ReRiff_ghost_achievement_ABI",typeof(void),Type.EmptyTypes,typeof(ProgressionGhost).Module,true);var il=dynamic.GetILGenerator();
            il.Emit(OpCodes.Ldc_I4,target.ToInt32());il.Emit(OpCodes.Conv_I);il.EmitCalli(OpCodes.Calli,CallingConvention.Cdecl,typeof(void),Type.EmptyTypes);il.Emit(OpCodes.Ret);
            try{validating=true;probeCalls=0;((Callback)dynamic.CreateDelegate(typeof(Callback)))();if(probeCalls!=1)throw new IOException("Ghost achievement thunk ABI differs");}
            finally{validating=false;}
            if(ReRiffConfigureGhostAchievement(pointer,1)!=1)throw new IOException("Ghost achievement callback validation refused");
        }
        private static T Bind<T>(Module game,int token,string name) where T:class {
            var method=game.ResolveMethod(token) as MethodInfo;
            if(method==null||!method.IsStatic||method.Name!=name)throw new IOException("Original ghost achievement binding differs: "+name);
            return NativeReelBridge.CreateCall(method,typeof(T),false) as T;
        }
        internal static void Configure(Module game){
            adapter=new GhostAchievement(
                ProgressionNative.Singleton(game,0x040010b6,"?ms_pInstance@gaRaceConfig@@0PAV1@A"),
                ProgressionNative.Singleton(game,0x040005a0,"?ms_instance@gaPlayerManager@@1PAV1@A"),
                ProgressionNative.Singleton(game,0x04000db1,"?ms_pInstance@gaAchievements@@0PAV1@A"),
                Bind<GhostAchievement.ScoreManager>(game,0x060005d4,"gaPlayerLocal.getScoreManager"),
                Bind<GhostAchievement.Score>(game,0x060007ac,"gaScoreManager.getScore"),
                Bind<GhostAchievement.Has>(game,0x06000e81,"gaAchievements.isAchieved"),
                Bind<GhostAchievement.Award>(game,0x06000e80,"gaAchievements.setAchieved"),NativeReelBridge.Log);
            ValidateBridge();NativeReelBridge.Log("Singleplayer Friend or Foe extension bound; existing achievements retained, normal ghost win required.");
        }
    }
}
