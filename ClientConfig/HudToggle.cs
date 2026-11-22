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
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void ForceVisible(bool visible);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int VisibleQuery();
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureHudToggle(IntPtr callback);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureHudSetVisible(IntPtr callback);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureHudVisibleQuery(IntPtr callback);
        private static Func<IntPtr> manager;
        private static ToggleOriginal original;
        private static SetAllVisible setAllVisible;
        private static FieldInfo showField;
        private static readonly Toggle callback=Observe;
        private static readonly ForceVisible forceCallback=ForceSetVisible;
        private static readonly VisibleQuery queryCallback=QueryVisible;
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
            if(ReRiffConfigureHudSetVisible(Marshal.GetFunctionPointerForDelegate(forceCallback))!=1)
                throw new IOException("HUD visibility bridge refused");
            if(ReRiffConfigureHudVisibleQuery(Marshal.GetFunctionPointerForDelegate(queryCallback))!=1)
                throw new IOException("HUD visibility query refused");
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
        private static void ForceSetVisible(bool visible){
            IntPtr value=manager();
            if(value==IntPtr.Zero)return;
            bool current=showField==null||(bool)showField.GetValue(null);
            if(current!=visible)original(value);
            /* Photo mode uses an explicit state so it does not depend on the
             * host's intermittent HUD flag being readable at toggle time. */
            setAllVisible(value,visible,0x7FFFFFFF,true);
        }
        private static int QueryVisible(){
            return showField==null||(bool)showField.GetValue(null)?1:0;
        }
    }
}
