using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
namespace ReRiff {
    // Controller Select/View toggles the native HUD visibility through the
    // original gaHUDManager.toggleUI method.
    internal static class HudToggle {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void ToggleOriginal(IntPtr manager);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Toggle();
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureHudToggle(IntPtr callback);
        private static Func<IntPtr> manager;
        private static ToggleOriginal original;
        private static readonly Toggle callback=Observe;
        internal static void Configure(Module game){
            if(IntPtr.Size!=4)throw new PlatformNotSupportedException();
            var method=game.ResolveMethod(0x06000c53) as MethodInfo;
            if(method==null||method.Name!="gaHUDManager.toggleUI"||method.GetParameters().Length!=1)
                throw new IOException("Original HUD toggle differs");
            original=(ToggleOriginal)NativeReelBridge.CreateCall(method,typeof(ToggleOriginal),false);
            manager=ProgressionNative.Singleton(game,0x0400039c,"?ms_pInstance@gaHUDManager@@0PAV1@A");
            if(ReRiffConfigureHudToggle(Marshal.GetFunctionPointerForDelegate(callback))!=1)
                throw new IOException("HUD toggle bridge refused");
        }
        private static void Observe(){
            IntPtr value=manager();
            if(value!=IntPtr.Zero)original(value);
        }
    }
}
