using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
namespace ReRiff {
    // Controller Select/View toggles the native HUD visibility through the
    // original gaHUDManager.toggleUI method.
    internal static class HudToggle {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void ToggleOriginal(IntPtr manager);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void SetAllVisible(IntPtr manager,bool visible,int owner,bool immediate);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Toggle();
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureHudToggle(IntPtr callback);
        private static Func<IntPtr> manager;
        private static ToggleOriginal original;
        private static SetAllVisible setAllVisible;
        private static FieldInfo showField;
        private static readonly Toggle callback=Observe;
        internal static void Configure(Module game){
            if(IntPtr.Size!=4)throw new PlatformNotSupportedException();
            var method=game.ResolveMethod(0x06000c53) as MethodInfo;
            if(method==null||method.Name!="gaHUDManager.toggleUI"||method.GetParameters().Length!=1)
                throw new IOException("Original HUD toggle differs");
            original=(ToggleOriginal)NativeReelBridge.CreateCall(method,typeof(ToggleOriginal),false);
            var all=game.GetMethods(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic)
                .SingleOrDefault(m=>m.Name=="gaHUDManager.setAllVisible");
            if(all==null||all.GetParameters().Length!=4)throw new IOException("Original HUD visibility differs");
            setAllVisible=(SetAllVisible)NativeReelBridge.CreateCall(all,typeof(SetAllVisible),false);
            showField=game.GetFields(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic)
                .SingleOrDefault(f=>f.Name=="?s_bShowUI@gaHUDItem@@1_NA");
            if(showField==null||showField.FieldType!=typeof(bool))throw new IOException("Original HUD show flag differs");
            manager=ProgressionNative.Singleton(game,0x0400039c,"?ms_pInstance@gaHUDManager@@0PAV1@A");
            if(ReRiffConfigureHudToggle(Marshal.GetFunctionPointerForDelegate(callback))!=1)
                throw new IOException("HUD toggle bridge refused");
        }
        private static void Observe(){
            IntPtr value=manager();
            if(value==IntPtr.Zero)return;
            original(value);
            bool visible=showField==null||(bool)showField.GetValue(null);
            /* toggleUI only covers HIDE value 12; include every HUD owner so
             * Select hides the complete HUD, not just the text elements. */
            setAllVisible(value,visible,0x7FFFFFFF,true);
        }
    }
}
