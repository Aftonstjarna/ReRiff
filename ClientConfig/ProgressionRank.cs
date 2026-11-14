using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

namespace ReRiff {
    internal static class ProgressionRank {
        internal const string OriginalHex="7eb10d00041f0a28810e00062d19021628361000066c691b320d7eb10d00041f0a1728800e00067eb10d00041f0b28810e00062d1a021628361000066c691f0a320d7eb10d00041f0b1728800e00067eb10d00041f0c28810e00062d1a021628361000066c691f0f320d7eb10d00041f0c1728800e00067eb10d00041f0d28810e00062d1a021628361000066c691f14320d7eb10d00041f0d1728800e00062a";
        internal delegate void Check(IntPtr driver);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Callback(IntPtr driver);
        private static readonly Callback NativeCallback=Dispatch;
        private static Check originalCheck;
        private static ProgressionView.ProfileLevel nativeLevel;
        private static Func<IntPtr> currentDriver;
        private static bool validating,errorLogged;
        private static int probeCalls,lastLevel=-1;
        private static IntPtr probeDriver;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionRank(IntPtr callback,int validated);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern IntPtr ReRiffGetProgressionRankThunk();
        internal static byte[] OriginalBody(){return Enumerable.Range(0,OriginalHex.Length/2).Select(i=>Convert.ToByte(OriginalHex.Substring(i*2,2),16)).ToArray();}
        internal static float LevelFor(ProgressionStore.State progress,IntPtr driver,IntPtr xp,ProgressionView.ProfileLevel calculate){
            if(progress==null||progress.PrestigeCount==0)return calculate(driver,xp);
            // Original Rank checks stop at level 20. Keep their native float
            // input bounded even when the permanent identity exceeds level 40.
            return (float)Math.Min(40m,ProgressionView.PermanentLevel(driver,progress.MainXp,calculate));
        }
        private static float SelectLevel(IntPtr driver,IntPtr xp){
            try {
                var state=ProgressionCheckpoints.Ready&&ProgressionNative.Bound!=null?ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp()):null;
                float level=LevelFor(state,driver,xp,nativeLevel);
                if(state!=null&&state.PrestigeCount>0&&(int)level!=lastLevel){lastLevel=(int)level;NativeReelBridge.Log("Rank achievement level: permanent_level="+lastLevel+" cycle_xp="+state.CycleXp+" prestige="+state.PrestigeCount);}
                return level;
            }catch(Exception error){Error(error);return nativeLevel(driver,xp);}
        }
        private static void Error(Exception error){if(!errorLogged){errorLogged=true;NativeReelBridge.Log("Permanent Rank achievement check unavailable: "+error);}}
        private static void Dispatch(IntPtr driver){
            if(validating){probeDriver=driver;probeCalls++;return;}
            try{
                if(!ProgressionCheckpoints.Ready||ProgressionNative.Bound==null)return;
                var progress=ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp());
                if(progress==null||progress.PrestigeCount==0)return;
                IntPtr target=currentDriver();if(target==IntPtr.Zero)return;
                NativeReelBridge.Log("Post-race Rank recheck: prestige="+progress.PrestigeCount);
                originalCheck(target);
            }catch(Exception error){Error(error);}
        }
        // Re-emit the exact original checker, substituting only its four calls
        // to the level getter. Original thresholds, existing-flag tests and
        // native setAchieved calls remain in the recovered instruction stream.
        internal static Check Clone(byte[] bytes,Func<int,MethodInfo> method,Func<int,FieldInfo> field,MethodInfo select){
            if(bytes==null||!bytes.SequenceEqual(OriginalBody()))throw new IOException("Original Rank checker bytes differ");
            var dynamic=new DynamicMethod("ReRiff_original_Rank_checker",typeof(void),new[]{typeof(IntPtr)},typeof(ProgressionRank).Module,true);
            var il=dynamic.GetILGenerator();var labels=new Dictionary<int,Label>();
            var operations=new List<Tuple<int,OpCode,int>>();int at=0,levelCalls=0;
            var opcodes=typeof(OpCodes).GetFields(BindingFlags.Public|BindingFlags.Static).Where(f=>f.FieldType==typeof(OpCode)).Select(f=>(OpCode)f.GetValue(null)).Where(o=>o.Size==1).ToDictionary(o=>(byte)o.Value);
            while(at<bytes.Length){int offset=at;OpCode op;if(!opcodes.TryGetValue(bytes[at++],out op))throw new IOException("Unreviewed Rank opcode");int value=0;
                switch(op.OperandType){
                    case OperandType.InlineNone:break;
                    case OperandType.ShortInlineI:value=unchecked((sbyte)bytes[at++]);break;
                    case OperandType.InlineField:case OperandType.InlineMethod:value=BitConverter.ToInt32(bytes,at);at+=4;break;
                    case OperandType.ShortInlineBrTarget:value=at+1+unchecked((sbyte)bytes[at]);at++;if(!labels.ContainsKey(value))labels.Add(value,il.DefineLabel());break;
                    default:throw new IOException("Unreviewed Rank operand");
                }operations.Add(Tuple.Create(offset,op,value));
            }
            foreach(int target in labels.Keys)if(!operations.Any(o=>o.Item1==target))throw new IOException("Invalid Rank branch target");
            foreach(var operation in operations){Label label;if(labels.TryGetValue(operation.Item1,out label))il.MarkLabel(label);var op=operation.Item2;int value=operation.Item3;
                switch(op.OperandType){
                    case OperandType.InlineNone:il.Emit(op);break;
                    case OperandType.ShortInlineI:il.Emit(op,(sbyte)value);break;
                    case OperandType.InlineField:if(value!=0x04000db1)throw new IOException("Unexpected Rank singleton");il.Emit(op,field(value));break;
                    case OperandType.InlineMethod:
                        if(value==0x06001036){il.Emit(OpCodes.Conv_I);il.Emit(OpCodes.Call,select);levelCalls++;}
                        else{if(value!=0x06000e81&&value!=0x06000e80)throw new IOException("Unexpected Rank method");il.Emit(op,method(value));}break;
                    case OperandType.ShortInlineBrTarget:il.Emit(op,labels[value]);break;
                }
            }
            if(levelCalls!=4)throw new IOException("Original Rank level calls differ");return (Check)dynamic.CreateDelegate(typeof(Check));
        }
        internal static Check Thunk(IntPtr target){
            if(target==IntPtr.Zero)throw new IOException("Missing Rank thunk");
            var dynamic=new DynamicMethod("ReRiff_Rank_thunk_ABI",typeof(void),new[]{typeof(IntPtr)},typeof(ProgressionRank).Module,true);var il=dynamic.GetILGenerator();
            il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Conv_I);il.Emit(OpCodes.Ldc_I4_0);il.Emit(OpCodes.Conv_I);il.Emit(OpCodes.Ldc_I4,target.ToInt32());il.Emit(OpCodes.Conv_I);
            il.EmitCalli(OpCodes.Calli,CallingConvention.Cdecl,typeof(void),new[]{typeof(IntPtr),typeof(IntPtr)});il.Emit(OpCodes.Ret);return (Check)dynamic.CreateDelegate(typeof(Check));
        }
        internal static void ValidateBridge(){
            IntPtr pointer=Marshal.GetFunctionPointerForDelegate(NativeCallback);
            if(ReRiffConfigureProgressionRank(pointer,0)!=1)throw new IOException("Rank callback registration refused");
            try{validating=true;probeCalls=0;probeDriver=IntPtr.Zero;Thunk(ReRiffGetProgressionRankThunk())(new IntPtr(0x1234));if(probeCalls!=1||probeDriver!=new IntPtr(0x1234))throw new IOException("Rank thunk ABI differs");}
            finally{validating=false;}
            if(ReRiffConfigureProgressionRank(pointer,1)!=1)throw new IOException("Rank callback validation refused");
        }
        internal static void Configure(Module game){
            var check=game.ResolveMethod(0x06001039) as MethodInfo;var level=game.ResolveMethod(0x06001036) as MethodInfo;
            var has=game.ResolveMethod(0x06000e81) as MethodInfo;var award=game.ResolveMethod(0x06000e80) as MethodInfo;var manager=game.ResolveField(0x04000db1);
            if(check==null||check.Name!="gaDriverLevel.checkLevelsAchievements"||!check.IsStatic||check.ReturnType!=typeof(void)||check.GetParameters().Length!=1
                ||level==null||level.Name!="gaDriverLevel.getDriverProfileLevel"||has==null||has.Name!="gaAchievements.isAchieved"||award==null||award.Name!="gaAchievements.setAchieved"
                ||manager==null||manager.Name!="?ms_pInstance@gaAchievements@@0PAV1@A")throw new IOException("Original Rank bindings differ");
            var body=check.GetMethodBody();if(body==null||body.LocalVariables.Count!=0||body.ExceptionHandlingClauses.Count!=0||body.MaxStackSize!=3)throw new IOException("Original Rank method layout differs");
            nativeLevel=(ProgressionView.ProfileLevel)NativeReelBridge.CreateCall(level,typeof(ProgressionView.ProfileLevel),false);
            originalCheck=Clone(body.GetILAsByteArray(),token=>(MethodInfo)game.ResolveMethod(token),token=>game.ResolveField(token),typeof(ProgressionRank).GetMethod("SelectLevel",BindingFlags.NonPublic|BindingFlags.Static));
            currentDriver=ProgressionNative.Singleton(game,0x04000f11,"?ms_pInstance@gaDriverLevel@@0PAV1@A");
            ValidateBridge();NativeReelBridge.Log("Original Rank thresholds and award path retained; completed races recheck permanent main level after prestige.");
        }
    }
}
