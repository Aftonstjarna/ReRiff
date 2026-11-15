using System;
using System.Reflection.Emit;
using System.Reflection;
using System.IO;
using System.Runtime.InteropServices;
class ProgressionLevelQuoteSmoke {
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate int Quote(int level);
    [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl)] static extern int ReRiffConfigureProgressionLevelQuote(IntPtr quote);
    [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl)] static extern IntPtr ReRiffGetProgressionLevelQuoteThunk();
    static int factor,seen;
    static int Reward(int level){seen=level;return 125*factor;}
    static int Main(){
        Quote callback=Reward;
        if(ReRiffConfigureProgressionLevelQuote(Marshal.GetFunctionPointerForDelegate(callback))!=1)throw new Exception("Registration failed");
        var method=new DynamicMethod("LevelMessageArgument",typeof(int),new[]{typeof(int)},typeof(ProgressionLevelQuoteSmoke).Module,true);
        var il=method.GetILGenerator();il.DeclareLocal(typeof(int));
        il.Emit(OpCodes.Ldarg_0); // Original first formatting argument: level.
        il.Emit(OpCodes.Dup);il.Emit(OpCodes.Ldc_I4,ReRiffGetProgressionLevelQuoteThunk().ToInt32());il.Emit(OpCodes.Conv_I);
        il.EmitCalli(OpCodes.Calli,CallingConvention.Cdecl,typeof(int),new[]{typeof(int)});
        // Preserve both original formatter arguments: level followed by coins.
        il.Emit(OpCodes.Stloc_0);il.Emit(OpCodes.Ldc_I4,10000);il.Emit(OpCodes.Mul);il.Emit(OpCodes.Ldloc_0);il.Emit(OpCodes.Add);il.Emit(OpCodes.Ret);
        var format=(Quote)method.CreateDelegate(typeof(Quote));
        for(factor=0;factor<=3;factor++)for(int level=1;level<=40;level++)if(format(level)!=level*10000+125*factor||seen!=level)throw new Exception("Native quote argument/return ABI mismatch");
        var assembly=Assembly.LoadFrom(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"ReRiff.ClientConfig.dll"));
        var balanceType=assembly.GetType("ReRiff.ProgressionBalance",true);var rewards=assembly.GetType("ReRiff.ProgressionRewards",true);
        var flags=BindingFlags.NonPublic|BindingFlags.Static|BindingFlags.Instance;
        var current=balanceType.GetField("Current",flags);object previous=current.GetValue(null);
        try {
            foreach(decimal rate in new[]{0m,.375m,1m,2m})foreach(uint prestige in new uint[]{0,1,2,9}) {
                object balance=Activator.CreateInstance(balanceType,true);balanceType.GetField("CoinRate",flags).SetValue(balance,rate);
                balanceType.GetField("PrestigeGrowth",flags).SetValue(balance,.5m);balanceType.GetField("MultiplierCap",flags).SetValue(balance,(decimal?)2m);current.SetValue(null,balance);
                int message=(int)rewards.GetMethod("CoinBonus",flags).Invoke(null,new object[]{101,prestige});
                int versus=(int)rewards.GetMethod("GhostBonus",flags).Invoke(null,new object[]{101,prestige});
                int[] payment=(int[])rewards.GetMethod("RaceCoins",flags).Invoke(null,new object[]{0,101,0,prestige});
                if(message!=versus||message!=payment[1]||message!=(int)Decimal.Floor(101m*rate*Math.Min(1m+prestige*.5m,2m)))throw new Exception("Reward messages disagree with payment policy");
            }
        }finally{current.SetValue(null,previous);}
        GC.KeepAlive(callback);
        Console.WriteLine("PASS: 160 level-up formatter cases plus shared rate, fractional rounding, cap and zero-rate payment/message policies");return 0;
    }
}
