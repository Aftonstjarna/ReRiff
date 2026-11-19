using System;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

public class CarFallbackSmoke {
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void Setter(IntPtr settings,int car);
    static void Check(bool good,string message){if(!good)throw new Exception(message);}
    public static int Main(string[] args){
        Check(IntPtr.Size==4,"32-bit CLR required");
        var original=Assembly.LoadFile(Path.GetFullPath(args[0])).GetType("Fixture.OriginalProgression",true).GetMethod("gaSaveGameChunkSettings.setChosenCar");
        var bridge=Assembly.LoadFrom(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"ReRiff.ClientConfig.dll")).GetType("ReRiff.NativeReelBridge",true);
        var setter=(Setter)bridge.GetMethod("CreateCall",BindingFlags.Static|BindingFlags.NonPublic).Invoke(null,new object[]{original,typeof(Setter),false});
        var thunk=new DynamicMethod("car_fallback_native_call",typeof(void),new[]{typeof(IntPtr),typeof(int)},typeof(CarFallbackSmoke));
        var il=thunk.GetILGenerator();il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldarg_1);
        il.Emit(OpCodes.Ldc_I4,Marshal.GetFunctionPointerForDelegate(setter).ToInt32());il.Emit(OpCodes.Conv_I);
        il.EmitCalli(OpCodes.Calli,CallingConvention.Cdecl,typeof(void),new[]{typeof(IntPtr),typeof(int)});il.Emit(OpCodes.Ret);
        var native=(Setter)thunk.CreateDelegate(typeof(Setter));
        IntPtr settings=Marshal.AllocHGlobal(328);
        try{
            byte[] before=new byte[328];for(int i=0;i<before.Length;i++)before[i]=(byte)(i%251);before[4]=0;before[5]=1;
            Array.Copy(BitConverter.GetBytes(15),0,before,324,4);Marshal.Copy(before,0,settings,328);
            native(settings,0);
            byte[] after=new byte[328];Marshal.Copy(settings,after,0,328);
            before[4]=1;Array.Copy(BitConverter.GetBytes(0),0,before,324,4);
            for(int i=0;i<before.Length;i++)Check(before[i]==after[i],"Original setter changed unrelated settings byte "+i);
            Marshal.WriteByte(settings,4,0);native(settings,0);Check(Marshal.ReadByte(settings,4)==0,"Unchanged selection dirtied the save");
            Console.WriteLine("PASS original setter through production call adapter and native Cdecl thunk: selection and dirty flag only, all 22 paint slots retained, repeat is idempotent");
            GC.KeepAlive(setter);return 0;
        }finally{Marshal.FreeHGlobal(settings);}
    }
}
