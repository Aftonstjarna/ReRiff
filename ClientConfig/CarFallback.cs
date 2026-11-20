using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace ReRiff {
    internal static class CarFallback {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void Setter(IntPtr settings,int car);
        private static Setter setter;
        private delegate void PaintSetter(IntPtr settings,int car,int pack);
        private static PaintSetter paintSetter;
        internal static void Paint(IntPtr settings,int car,int pack){if(paintSetter==null)throw new IOException("Native paint setter unavailable");paintSetter(settings,car,pack);}
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureCarFallback(IntPtr callback);
        internal static void Select(IntPtr settings,int car){if(setter==null)throw new IOException("Native car setter unavailable");setter(settings,car);}
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int Migrate(IntPtr save);
        private static readonly Migrate migrate=Tick;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)] private static extern int ReRiffConfigureCarMigration(IntPtr callback);
        private static int Tick(IntPtr save){try{CarSaveRuntime.Tick(save);return 1;}catch(Exception e){NativeReelBridge.Log("Car identity migration refused: "+e);return 0;}}
        internal static void Configure(Module game){
            MethodInfo method=game.ResolveMethod(0x0600127c) as MethodInfo;
            if(method==null||method.Name!="gaSaveGameChunkSettings.setChosenCar"||!method.IsStatic
                ||method.ReturnType!=typeof(void)||method.GetParameters().Length!=2
                ||BitConverter.ToString(method.GetMethodBody().GetILAsByteArray()).Replace("-","")!="03022044010000584A2E0E022044010000580354021A5817522A")
                throw new IOException("Original selected-car setter differs; fallback refused");
            setter=(Setter)NativeReelBridge.CreateCall(method,typeof(Setter),false);
            MethodInfo paint=game.ResolveMethod(0x0600127b) as MethodInfo;
            if(paint==null||paint.Name!="gaSaveGameChunkSettings.setChosenPackIndexForCar"||!paint.IsStatic
                ||paint.ReturnType!=typeof(void)||paint.GetParameters().Length!=3)throw new IOException("Original paint setter differs");
            paintSetter=(PaintSetter)NativeReelBridge.CreateCall(paint,typeof(PaintSetter),false);
            if(ReRiffConfigureCarFallback(Marshal.GetFunctionPointerForDelegate(setter))!=1)
                throw new IOException("Car fallback registration refused");
            if(ReRiffConfigureCarMigration(Marshal.GetFunctionPointerForDelegate(migrate))!=1)throw new IOException("Car migration callback unavailable");
            NativeReelBridge.Log("Local missing-car fallback bound to original selected-car setter; package origins retained in car-origins.ini.");
        }
    }
}
