using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

namespace ReRiff {
    // Extend original getters/coin handling. Singleplayer coin-pack discounts
    // use the shared native getter; scoped native Results/Creator entry hooks
    // keep earned rewards consistent with their original display fields.
    internal static class ProgressionEconomy {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int Price(IntPtr paint,int car,int pack);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void Coins(IntPtr progression,int delta);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int GenerationQuote(int amount);
        private static readonly GenerationQuote GenerationCallback=QuoteGeneration;
        private static readonly GenerationQuote GhostQuoteCallback=QuoteGhostBonus;
        private static readonly GenerationQuote LevelQuoteCallback=QuoteLevelBonus;
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int LevelReward(IntPtr driver,int level);
        private static LevelReward originalLevelReward;
        private static Func<IntPtr> levelDriver;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionLevelQuote(IntPtr quote);
        private static int QuoteLevelBonus(int level){
            int original=0;
            try {
                original=originalLevelReward(levelDriver(),level);
                if(original<0||!ProgressionCheckpoints.Ready||ProgressionNative.Bound==null)return original;
                var progress=ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp());
                if(progress==null)return original;
                int quote=ProgressionRewards.CoinBonus(original,progress.PrestigeCount);
                NativeReelBridge.Log("Prestige level-up bonus display: level="+level+" original="+original+" quoted="+quote+" prestige="+progress.PrestigeCount);
                return quote;
            }catch(Exception error){Error(error);return original;}
        }
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionRewards(IntPtr quote);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionGhostQuote(IntPtr quote);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern IntPtr ReRiffGetProgressionRaceContext();
        private static int QuoteGeneration(int amount){
            try {
                if(amount<0||!ProgressionCheckpoints.Ready||ProgressionNative.Bound==null)return amount;
                var progress=ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp());
                if(progress==null)return amount;
                int quote=ProgressionRewards.Coins(amount,ProgressionNative.Bound.CoinBalance(),progress.PrestigeCount,ProgressionBalance.Current.CreatorRate,ProgressionBalance.Current.PrestigeGrowth,ProgressionBalance.Current.MultiplierCap);
                NativeReelBridge.Log("Prestige generation quote: original="+amount+" quoted="+quote+" prestige="+progress.PrestigeCount);
                return quote;
            }catch(Exception error){Error(error);return amount;}
        }
        private static int QuoteGhostBonus(int amount){
            try {
                if(amount<0||!ProgressionCheckpoints.Ready||ProgressionNative.Bound==null)return amount;
                var progress=ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp());
                if(progress==null)return amount;
                int quote=ProgressionRewards.GhostBonus(amount,progress.PrestigeCount);
                NativeReelBridge.Log("Prestige ghost bonus display: original="+amount+" quoted="+quote+" prestige="+progress.PrestigeCount);
                return quote;
            }catch(Exception error){Error(error);return amount;}
        }
        private static readonly Price PriceCallback=ObservePrice;
        private static readonly Coins CoinCallback=ObserveCoins;
        private static Price originalPrice;
        private static Price originalIap;
        private static Coins originalCoins;
        private static Module originalModule;
        private static bool validating,errorLogged;
        private static int coinTraces;
        private static readonly HashSet<string> priceTraces=new HashSet<string>();
        internal const string PriceBody="0420b00000005a031f0c5a02584a581f24584a2a";
        internal const string IapBody="0420b00000005a031f0c5a02584a581f28584a2a";
        internal const string CoinBody="02201c0100005802201c010000584a03585402201c010000584a0a02201c010000580616320a02201c010000584a2b011654021a5817522a";
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionEconomy([In] IntPtr[] callbacks,int count,int validated);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern IntPtr ReRiffGetProgressionEconomyThunk(int which);
        private static byte[] Hex(string value){var bytes=new byte[value.Length/2];for(int i=0;i<bytes.Length;i++)bytes[i]=Convert.ToByte(value.Substring(i*2,2),16);return bytes;}
        internal static Delegate Clone(MethodInfo source,Type signature,string expected){
            var body=source.GetMethodBody();byte[] bytes=body==null?null:body.GetILAsByteArray();
            if(bytes==null||!bytes.SequenceEqual(Hex(expected))||body.ExceptionHandlingClauses.Count!=0
                ||body.LocalVariables.Any(v=>v.LocalType!=typeof(int)))throw new IOException("Original economy IL differs; wrapper refused");
            var invoke=signature.GetMethod("Invoke");
            var method=new DynamicMethod("ReRiff_original_"+source.Name,invoke.ReturnType,invoke.GetParameters().Select(p=>p.ParameterType).ToArray(),source.Module,true){InitLocals=body.InitLocals};
            // Wine Mono does not implement DynamicILInfo.SetCode. Re-emit the
            // reviewed instruction subset through the existing bridge's IL API.
            // These bodies contain no metadata references, EH or two-byte ops.
            var il=method.GetILGenerator();foreach(var local in body.LocalVariables)il.DeclareLocal(local.LocalType,local.IsPinned);
            var opcodes=typeof(OpCodes).GetFields(BindingFlags.Public|BindingFlags.Static).Where(f=>f.FieldType==typeof(OpCode)).Select(f=>(OpCode)f.GetValue(null)).Where(o=>o.Size==1).ToDictionary(o=>(byte)o.Value);
            var operations=new List<Tuple<int,OpCode,object>>();var labels=new Dictionary<int,Label>();int at=0;
            while(at<bytes.Length){
                int offset=at;OpCode op;if(!opcodes.TryGetValue(bytes[at++],out op))throw new IOException("Unreviewed original opcode");object operand=null;
                switch(op.OperandType){
                    case OperandType.InlineNone:break;
                    case OperandType.ShortInlineI:operand=unchecked((sbyte)bytes[at++]);break;
                    case OperandType.InlineI:operand=BitConverter.ToInt32(bytes,at);at+=4;break;
                    case OperandType.ShortInlineBrTarget:
                        int target=at+1+unchecked((sbyte)bytes[at]);at++;operand=target;
                        if(!labels.ContainsKey(target))labels.Add(target,il.DefineLabel());break;
                    default:throw new IOException("Unreviewed original operand kind");
                }operations.Add(Tuple.Create(offset,op,operand));
            }
            foreach(int target in labels.Keys)if(!operations.Any(o=>o.Item1==target))throw new IOException("Original branch target is not an instruction");
            foreach(var operation in operations){
                if(il.ILOffset!=operation.Item1)throw new IOException("Original economy instruction offsets changed");
                Label label;if(labels.TryGetValue(operation.Item1,out label))il.MarkLabel(label);
                switch(operation.Item2.OperandType){
                    case OperandType.InlineNone:il.Emit(operation.Item2);break;
                    case OperandType.ShortInlineI:il.Emit(operation.Item2,(sbyte)operation.Item3);break;
                    case OperandType.InlineI:il.Emit(operation.Item2,(int)operation.Item3);break;
                    case OperandType.ShortInlineBrTarget:il.Emit(operation.Item2,labels[(int)operation.Item3]);break;
                }
            }
            if(il.ILOffset!=bytes.Length)throw new IOException("Original economy code length changed");
            return method.CreateDelegate(signature);
        }
        internal static Delegate Thunk(IntPtr target,bool price){
            if(target==IntPtr.Zero)throw new IOException("Native economy thunk missing");
            Type type=price?typeof(Price):typeof(Coins);var invoke=type.GetMethod("Invoke");
            var method=new DynamicMethod("ReRiff_validate_economy_thunk",invoke.ReturnType,invoke.GetParameters().Select(p=>p.ParameterType).ToArray(),typeof(ProgressionEconomy).Module,true);
            var il=method.GetILGenerator();
            if(price){il.Emit(OpCodes.Ldarga_S,(byte)0);il.Emit(OpCodes.Conv_U);}else{il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Conv_I);}
            il.Emit(OpCodes.Ldarg_1);il.Emit(OpCodes.Conv_I);if(price)il.Emit(OpCodes.Ldarg_2);
            il.Emit(OpCodes.Ldc_I4,target.ToInt32());il.Emit(OpCodes.Conv_I);
            il.EmitCalli(OpCodes.Calli,CallingConvention.Cdecl,typeof(void),price?new[]{typeof(IntPtr),typeof(IntPtr),typeof(int)}:new[]{typeof(IntPtr),typeof(IntPtr)});
            if(price){il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Conv_I4);}
            il.Emit(OpCodes.Ret);
            if(!price){while(il.ILOffset<55)il.Emit(OpCodes.Nop);il.Emit(OpCodes.Ret);}
            if(il.ILOffset!=(price?20:56))throw new IOException("Economy thunk code length differs");
            return method.CreateDelegate(type);
        }
        private static void Error(Exception error){if(!errorLogged){errorLogged=true;NativeReelBridge.Log("Economy trace policy unavailable; original amounts retained: "+error);}}
        private static int ObservePrice(IntPtr paint,int car,int pack){
            int raw=originalPrice(paint,car,pack);
            try {
                if(!validating&&ProgressionCheckpoints.Ready&&ProgressionNative.Bound!=null&&raw>=0){
                    var progress=ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp());int level=ProgressionView.MainLevel(progress);
                    int proposed=CoinPackPrice(raw,originalIap(paint,car,pack),level);string key=car+":"+pack+":"+raw+":"+proposed;
                    if(priceTraces.Count<512&&priceTraces.Add(key))NativeReelBridge.Log("Prestige price applied: car="+car+" pack="+pack+" original="+raw+" quoted="+proposed+" main_level="+level);
                    return proposed;
                }
            }catch(Exception error){Error(error);}return raw;
        }
        internal static int CoinPackPrice(int original,int iapIndex,int mainLevel){return original<0||iapIndex>=0?original:ProgressionRewards.PaintPrice(original,mainLevel,ProgressionBalance.Current.PaintScale);}
        internal static int TestCoinPackPrice(IntPtr paint,int car,int pack,int mainLevel){
            if(originalPrice==null||originalIap==null||originalIap(paint,car,pack)>=0)throw new InvalidOperationException("Purchase test requires a native coin pack");
            int raw=originalPrice(paint,car,pack);if(raw<=0)throw new InvalidOperationException("Purchase test requires a paid coin pack");
            return CoinPackPrice(raw,-1,mainLevel);
        }
        private static string Caller(){
            foreach(var frame in new StackTrace().GetFrames()??new StackFrame[0]){
                var method=frame.GetMethod();if(method==null||method.Module!=originalModule||method.Name=="gaSaveGameChunkProgression.incTokens"||method.Name.StartsWith("ReRiff_",StringComparison.Ordinal))continue;
                return method.Name;
            }return "unresolved";
        }
        internal static int ApplyRaceQuote(IntPtr menu,int delta,int current,uint prestige){
            byte[] fields=ProgressionNative.Read(IntPtr.Add(menu,388),12);
            int race=BitConverter.ToInt32(fields,0),bonus=BitConverter.ToInt32(fields,4),before=BitConverter.ToInt32(fields,8);
            if(race<0||bonus<0||(long)race+bonus!=delta||before!=current)throw new IOException("Native race award fields differ from payment");
            int[] quote=ProgressionRewards.RaceCoins(race,bonus,current,prestige);
            int total=checked(quote[0]+quote[1]);
            Marshal.WriteInt32(menu,388,quote[0]);Marshal.WriteInt32(menu,392,quote[1]);return total;
        }
        private static void ObserveCoins(IntPtr progression,int delta){
            int applied=delta;
            try {
                if(!validating&&ProgressionCheckpoints.Ready&&ProgressionNative.Bound!=null){
                    int current=BitConverter.ToInt32(ProgressionNative.Read(IntPtr.Add(progression,284),4),0);
                    var progress=ProgressionCheckpoints.DisplayProgress(ProgressionNative.Bound.CycleXp());
                    string source=Caller();IntPtr menu=ReRiffGetProgressionRaceContext();
                    if(progress!=null&&menu!=IntPtr.Zero&&source=="gaResultsTokens.enter"){
                        applied=ApplyRaceQuote(menu,delta,current,progress.PrestigeCount);
                        int race=Marshal.ReadInt32(menu,388),bonus=Marshal.ReadInt32(menu,392);
                        NativeReelBridge.Log("Prestige race award applied: original="+delta+" credited="+applied+" race="+race+" bonus="+bonus+" prestige="+progress.PrestigeCount);
                    }
                    // Creator's scoped native tuneable already supplies both
                    // its text and credit. Never multiply that credit again.
                    if(coinTraces++<64)NativeReelBridge.Log("Prestige coin dispatch: source="+source+" original_delta="+delta+" applied_delta="+applied+" balance_before="+current);
                }
            }catch(Exception error){Error(error);}
            originalCoins(progression,applied);
        }
        // Used by the isolated fixture test too. The same native thunks/calli
        // convention are exercised on allocated buffers before any game patch.
        internal static void BindAndValidate(MethodInfo price,MethodInfo coins,MethodInfo iap){
            originalPrice=(Price)Clone(price,typeof(Price),PriceBody);originalIap=(Price)Clone(iap,typeof(Price),IapBody);
            originalCoins=(Coins)Clone(coins,typeof(Coins),CoinBody);originalModule=price.Module;
            IntPtr[] callbacks={Marshal.GetFunctionPointerForDelegate(PriceCallback),Marshal.GetFunctionPointerForDelegate(CoinCallback)};
            if(ReRiffConfigureProgressionEconomy(callbacks,2,0)!=1)throw new IOException("Economy probe registration refused");
            IntPtr paint=Marshal.AllocHGlobal(12),pack=Marshal.AllocHGlobal(176),chunk=Marshal.AllocHGlobal(296);
            try {
                validating=true;Marshal.WriteIntPtr(paint,pack);Marshal.WriteInt32(pack,36,733);Marshal.WriteInt32(chunk,284,17);
                if(originalPrice(paint,0,0)!=733)throw new IOException("Original price IL clone differs");
                originalCoins(chunk,3);originalCoins(chunk,-7);
                if(Marshal.ReadInt32(chunk,284)!=13)throw new IOException("Original signed coin IL clone differs");
                Marshal.WriteInt32(chunk,284,17);
                var priceThunk=(Price)Thunk(ReRiffGetProgressionEconomyThunk(0),true);var coinsThunk=(Coins)Thunk(ReRiffGetProgressionEconomyThunk(1),false);
                if(priceThunk(paint,0,0)!=733)throw new IOException("Original price thunk ABI differs");
                coinsThunk(chunk,3);if(Marshal.ReadInt32(chunk,284)!=20)throw new IOException("Original coin thunk ABI differs");
                coinsThunk(chunk,-7);if(Marshal.ReadInt32(chunk,284)!=13)throw new IOException("Original debit thunk ABI differs");
            }finally{validating=false;Marshal.FreeHGlobal(paint);Marshal.FreeHGlobal(pack);Marshal.FreeHGlobal(chunk);}
            if(ReRiffConfigureProgressionEconomy(callbacks,2,1)!=1)throw new IOException("Economy probe validation refused");
            NativeReelBridge.Log("Original economy IL clones and native thunk ABI validated; singleplayer discounts and scoped race/creator rewards registered; prestige requires eligible Credits confirmation.");
        }
        internal static void Configure(Module game){
            string folder=Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location);
            if(!ProgressionDomain.Enabled(folder))return;
            var price=game.ResolveMethod(0x0600138d) as MethodInfo;var coins=game.ResolveMethod(0x06001263) as MethodInfo;
            var iap=game.ResolveMethod(0x0600138e) as MethodInfo;
            if(price==null||price.Name!="gaVehiclePaintManager.getPackPrice"||coins==null||coins.Name!="gaSaveGameChunkProgression.incTokens"||iap==null||iap.Name!="gaVehiclePaintManager.getPackIAPIndex")throw new IOException("Original economy methods differ");
            BindAndValidate(price,coins,iap);
            var ghostBase=game.ResolveField(0x04000747);
            var versus=game.ResolveMethod(0x06000769) as MethodInfo;
            byte[] versusIl=versus==null?null:versus.GetMethodBody().GetILAsByteArray();
            if(ghostBase.Name!="?s_tokensBeatOpponentGhost@gaResultsTokens@@2HB"||!ghostBase.IsStatic||ghostBase.IsLiteral||ghostBase.IsInitOnly
                ||ghostBase.FieldType!=typeof(int)||versus==null||versus.Name!="gaResultsVersus.initScreens"||versusIl==null||versusIl.Length<0x530
                ||!versusIl.Skip(0x52b).Take(5).SequenceEqual(new byte[]{0x7e,0x47,0x07,0,4}))throw new IOException("Original versus reward field/formatter differs");
            if(ReRiffConfigureProgressionGhostQuote(Marshal.GetFunctionPointerForDelegate(GhostQuoteCallback))!=1)throw new IOException("Native ghost display registration refused");
            var levelReward=game.ResolveMethod(0x06001034) as MethodInfo;
            var formatter=game.ResolveMethod(0x06000785) as MethodInfo;
            if(levelReward==null||levelReward.Name!="gaDriverLevel.getTokenRewardAtLevel"||!levelReward.GetMethodBody().GetILAsByteArray().SequenceEqual(Hex("031a5a021f0c584a584a2a"))
                ||formatter==null||formatter.Name!="gaResultsXP.tick")throw new IOException("Original level-up reward getter/formatter differs");
            originalLevelReward=(LevelReward)NativeReelBridge.CreateCall(levelReward,typeof(LevelReward),false);
            levelDriver=ProgressionNative.Singleton(game,0x04000f11,"?ms_pInstance@gaDriverLevel@@0PAV1@A");
            if(ReRiffConfigureProgressionLevelQuote(Marshal.GetFunctionPointerForDelegate(LevelQuoteCallback))!=1)throw new IOException("Native level-up display registration refused");
            if(ReRiffConfigureProgressionRewards(Marshal.GetFunctionPointerForDelegate(GenerationCallback))!=1)throw new IOException("Native reward context registration refused");
        }
    }
}
