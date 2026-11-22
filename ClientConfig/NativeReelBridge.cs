// Transport only: expose the original Game.exe IL methods to the native menu.
// No game assembly is shipped or referenced at build time.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using System.Security.Cryptography;

namespace ReRiff {
    internal static class NativeReelBridge {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate IntPtr Construct(IntPtr reel, IntPtr parent, float spacing, int direction);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Add(IntPtr reel, IntPtr option, IntPtr screen);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Restrict(IntPtr reel, int visible);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Tick(IntPtr reel);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Move(IntPtr reel, int first, IntPtr time);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int Selection(IntPtr reel);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Scrollbar(IntPtr reel, float x, float top, float bottom, int pcScale, IntPtr width);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Overlay(IntPtr manager,int state);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int AlphabetJump(IntPtr menu,int letter);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void CullingVisible(IntPtr visual,bool visible);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigurePickerAlphabet(IntPtr callback);
        private static readonly List<Delegate> Roots = new List<Delegate>();
        private static CullingVisible cullingOriginal;
        private static readonly CullingVisible cullingCallback=ObserveCulling;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureCullingVisible(IntPtr callback);
        private static void ObserveCulling(IntPtr visual,bool visible){
            try{if(cullingOriginal!=null&&visual!=IntPtr.Zero)cullingOriginal(visual,visible);}catch{}
        }
        private static bool attempted;
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureNativeReel([In] IntPtr[] methods, int count);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureNativeGraphics([In] IntPtr[] methods, int count);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureNativeUiScale([In] IntPtr[] methods, int count);

        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureNativePrompt([In] IntPtr[] methods,int count);

        // addOption consumes an SSP passed by address in IL. Its C ABI adapter
        // accepts an owning object reference and supplies the temporary SSP.
        internal static Delegate CreateCall(MethodInfo target, Type delegateType, bool owningSsp) {
            MethodInfo invoke=delegateType.GetMethod("Invoke");
            ParameterInfo[] parameters=invoke.GetParameters();
            Type[] signature=new Type[parameters.Length];
            for(int i=0;i<signature.Length;i++)signature[i]=parameters[i].ParameterType;
            DynamicMethod method=new DynamicMethod("ReRiff_"+target.Name,invoke.ReturnType,signature,target.Module,true);
            ILGenerator il=method.GetILGenerator();
            LocalBuilder ssp=owningSsp?il.DeclareLocal(typeof(IntPtr)):null;
            for(int i=0;i<signature.Length;i++) {
                il.Emit(OpCodes.Ldarg,(short)i);
                if(owningSsp&&i==1) { il.Emit(OpCodes.Stloc,ssp);il.Emit(OpCodes.Ldloca,ssp);il.Emit(OpCodes.Conv_U); }
            }
            il.Emit(OpCodes.Call,target);
            if(invoke.ReturnType==typeof(IntPtr))il.Emit(OpCodes.Conv_I);
            il.Emit(OpCodes.Ret);
            return method.CreateDelegate(delegateType);
        }

        internal static void Log(string message) {
            try {Logging.Append(Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location),"ReRiff-engine-menu.log",message+Environment.NewLine);}catch(Exception) { }
        }
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,CharSet=CharSet.Unicode,ExactSpelling=true)]
        private static extern void ReRiffShowError(string title,string text);
        [DllImport("user32.dll",CharSet=CharSet.Unicode,ExactSpelling=true)]
        private static extern int MessageBoxW(IntPtr owner,string text,string title,uint flags);
        // Prefer the selectable native dialog; fall back for an older helper DLL.
        internal static void ShowError(string title,string text){
            try{ReRiffShowError(title,text);}
            catch(EntryPointNotFoundException){MessageBoxW(IntPtr.Zero,text,title,0x10);}
            catch(DllNotFoundException){MessageBoxW(IntPtr.Zero,text,title,0x10);}
        }
        internal static void Configure() {
            if(attempted)return;attempted=true;
            try {
                Module game=null;
                foreach(Assembly assembly in AppDomain.CurrentDomain.GetAssemblies()) {
                    if(assembly.IsDynamic)continue;
                    if(string.Equals(Path.GetFileName(assembly.Location),"Game.exe",StringComparison.OrdinalIgnoreCase)) {game=assembly.ManifestModule;break;}
                }
                if(game==null){Log("Original Game.exe module not found; native bridge unavailable.");return;}
                using(var hash=SHA256.Create())using(var file=File.OpenRead(game.FullyQualifiedName)) {
                    string value=BitConverter.ToString(hash.ComputeHash(file)).Replace("-","").ToLowerInvariant();
                    if(value!="2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2"){Log("Unsupported Game.exe SHA256="+value);return;}
                }
                int[] tokens={0x06001138,0x0600113b,0x0600113c,0x0600113d,0x0600113e,0x06001140,0x06001142};
                string[] names={"gaMenuReel.{ctor}","gaMenuReel.addOption","gaMenuReel.restrictScrolling","gaMenuReel.tick","gaMenuReel.moveToSelection","gaMenuReel.getCurrentSelection","gaMenuReel.addScrollbar"};
                Type[] types={typeof(Construct),typeof(Add),typeof(Restrict),typeof(Tick),typeof(Move),typeof(Selection),typeof(Scrollbar)};
                IntPtr[] pointers=new IntPtr[tokens.Length];
                for(int i=0;i<tokens.Length;i++) {
                    MethodInfo original=game.ResolveMethod(tokens[i]) as MethodInfo;
                    if(original==null||original.Name!=names[i])throw new InvalidOperationException("Unexpected original reel method");
                    Delegate callback=CreateCall(original,types[i],i==1);
                    Roots.Add(callback);pointers[i]=Marshal.GetFunctionPointerForDelegate(callback);
                }
                ReRiffConfigureNativeReel(pointers,pointers.Length);
                int[] graphicsTokens={0x060010dc,0x060012a5};
                string[] graphicsNames={"gaGame.onResolutionAdjusted","gaStateManager.rescaleMenus"};
                IntPtr[] graphicsPointers=new IntPtr[graphicsTokens.Length];
                for(int i=0;i<graphicsTokens.Length;i++){
                    MethodInfo original=game.ResolveMethod(graphicsTokens[i]) as MethodInfo;
                    if(original==null||original.Name!=graphicsNames[i])throw new InvalidOperationException("Unexpected original graphics method");
                    Delegate callback=CreateCall(original,typeof(Tick),false);
                    Roots.Add(callback);graphicsPointers[i]=Marshal.GetFunctionPointerForDelegate(callback);
                }
                ReRiffConfigureNativeGraphics(graphicsPointers,graphicsPointers.Length);
                MethodInfo fontSetup=game.ResolveMethod(0x060012c0) as MethodInfo;
                if(fontSetup==null||fontSetup.Name!="gaUI.setupFontScale")throw new InvalidOperationException("Unexpected original font-scale method");
                Delegate fontCallback=CreateCall(fontSetup,typeof(Tick),false);Roots.Add(fontCallback);
                ReRiffConfigureNativeUiScale(new IntPtr[]{Marshal.GetFunctionPointerForDelegate(fontCallback)},1);
                int[] overlayTokens={0x0600129b,0x0600129c};
                string[] overlayNames={"gaStateManager.setOverlayState","gaStateManager.popOverlayState"};
                IntPtr[] overlayPointers=new IntPtr[2];
                for(int i=0;i<2;i++){
                    MethodInfo original=game.ResolveMethod(overlayTokens[i]) as MethodInfo;
                    if(original==null||original.Name!=overlayNames[i])throw new InvalidOperationException("Unexpected original overlay method");
                    Delegate callback=CreateCall(original,typeof(Overlay),false);Roots.Add(callback);overlayPointers[i]=Marshal.GetFunctionPointerForDelegate(callback);
                }
                ReRiffConfigureNativePrompt(overlayPointers,2);
                try {LastFmNative.Configure(game);}
                catch {Log("Scrobbling playback binding unavailable.");}
                try {NativeModCache.Configure();}
                catch(Exception error){Log("Mod cache adapter unavailable: "+error);}
                try {CarFallback.Configure(game);CarSaveRuntime.Configure();}
                catch(Exception error){Log("Local car fallback unavailable: "+error);}
                try {GhostCarTest.Configure();}
                catch(Exception error){Log("Ghost car test unavailable: "+error);}
                try {NativePickerSearch.Configure(game);}
                catch(Exception error){Log("Picker search unavailable: "+error);}
                try {
                    var visible=game.GetMethods(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic)
                        .SingleOrDefault(m=>m.Name=="elVisual.setVisible");
                    if(visible==null||visible.GetParameters().Length!=2)throw new IOException("Original prop visibility signature differs");
                    cullingOriginal=(CullingVisible)CreateCall(visible,typeof(CullingVisible),false);Roots.Add(cullingOriginal);
                    if(ReRiffConfigureCullingVisible(Marshal.GetFunctionPointerForDelegate(cullingCallback))!=1)throw new IOException("Culling visibility registration refused");
                    Log("Original prop visibility bound for culling disable.");
                }catch(Exception error){Log("Culling prop visibility unavailable: "+error);}
                try {CustomsUnlocks.Configure(game);}catch(Exception e){Log("Customs unlock setup failed: "+e.Message);}
                try {HudToggle.Configure(game);}catch(Exception error){Log("HUD Select toggle unavailable: "+error);}
                try {ProgressionNative.Configure(game);}
                catch(Exception error){Log("Original progression adapter unavailable; live prestige remains disabled: "+error);}
                try {ProgressionView.Configure(game);}
                catch(Exception error){Log("Permanent-level display binding unavailable: "+error);}
                try {ProgressionGhost.Configure(game);}
                catch(Exception error){Log("Singleplayer ghost achievement binding unavailable: "+error);}
                try {ProgressionRank.Configure(game);}
                catch(Exception error){Log("Permanent Rank binding unavailable; original Rank checks retained: "+error);}
                try {ProgressionEconomy.Configure(game);}
                catch(Exception error){Log("Original economy trace unavailable; original amounts retained: "+error);}
                try {
                    var jump=game.ResolveMethod(0x06001053) as MethodInfo;
                    if(jump==null||jump.Name!="gaFileSelectInterface.jumpToSelection"||jump.ReturnType!=typeof(bool)||jump.GetParameters().Length!=2||jump.GetParameters()[1].ParameterType!=typeof(sbyte))throw new IOException("Native picker jump signature differs");
                    var body=jump.GetMethodBody();if(body==null)throw new IOException("Native picker jump body missing");
                    using(var hash=SHA256.Create()) {
                        string value=BitConverter.ToString(hash.ComputeHash(body.GetILAsByteArray())).Replace("-","").ToLowerInvariant();
                        if(value!="27868fc02f3bdb306399a688ccbc08628148349407a3a5f98702affd6d8a6545")throw new IOException("Native picker jump bytes differ");
                    }
                    Delegate callback=CreateCall(jump,typeof(AlphabetJump),false);Roots.Add(callback);
                    if(ReRiffConfigurePickerAlphabet(Marshal.GetFunctionPointerForDelegate(callback))!=1)throw new IOException("Picker alphabet registration refused");
                    Log("Original picker A-Z jump bound; controller alphabet integration ready.");
                }catch(Exception error){Log("Controller alphabet shortcut unavailable; original picker unchanged: "+error);}

            } catch(DllNotFoundException) { }
              catch(EntryPointNotFoundException) { }
              catch(BadImageFormatException) { }
              catch(Exception error) {
                Log("Native file-picker bridge: "+error);
            }
        }
    }
}
