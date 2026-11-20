using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace ReRiff {
    internal static class CarFallback {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void Setter(IntPtr settings,int car);
        private static Setter setter;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureCarFallback(IntPtr callback);
        internal static void Configure(Module game){
            MethodInfo method=game.ResolveMethod(0x0600127c) as MethodInfo;
            if(method==null||method.Name!="gaSaveGameChunkSettings.setChosenCar"||!method.IsStatic
                ||method.ReturnType!=typeof(void)||method.GetParameters().Length!=2
                ||BitConverter.ToString(method.GetMethodBody().GetILAsByteArray()).Replace("-","")!="03022044010000584A2E0E022044010000580354021A5817522A")
                throw new IOException("Original selected-car setter differs; fallback refused");
            setter=(Setter)NativeReelBridge.CreateCall(method,typeof(Setter),false);
            if(ReRiffConfigureCarFallback(Marshal.GetFunctionPointerForDelegate(setter))!=1)
                throw new IOException("Car fallback registration refused");
            NativeReelBridge.Log("Local missing-car fallback bound to original selected-car setter; package origins retained in car-origins.ini.");
        }
    }
}
