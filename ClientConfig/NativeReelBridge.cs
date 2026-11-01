// Transport only: expose the original Game.exe IL methods to the native menu.
// No game assembly is shipped or referenced at build time.
using System;
using System.Collections.Generic;
using System.IO;
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
        private static readonly List<Delegate> Roots = new List<Delegate>();
        private static bool attempted;
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureNativeReel([In] IntPtr[] methods, int count);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureNativeGraphics([In] IntPtr[] methods, int count);

        // addOption consumes an SSP passed by address in IL. Its C ABI adapter
        // accepts an owning object reference and supplies the temporary SSP.
        private static Delegate CreateCall(MethodInfo target, Type delegateType, bool owningSsp) {
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

        internal static void Configure() {
            if(attempted)return;attempted=true;
            try {
                Module game=null;
                foreach(Assembly assembly in AppDomain.CurrentDomain.GetAssemblies()) {
                    if(assembly.IsDynamic)continue;
                    if(string.Equals(Path.GetFileName(assembly.Location),"Game.exe",StringComparison.OrdinalIgnoreCase)) {game=assembly.ManifestModule;break;}
                }
                if(game==null)return;
                using(var hash=SHA256.Create())using(var file=File.OpenRead(game.FullyQualifiedName)) {
                    string value=BitConverter.ToString(hash.ComputeHash(file)).Replace("-","").ToLowerInvariant();
                    if(value!="2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2")return;
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
            } catch(DllNotFoundException) { }
              catch(EntryPointNotFoundException) { }
              catch(BadImageFormatException) { }
              catch(Exception error) {
                try {File.AppendAllText(Path.Combine(Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location),"ReRiff-engine-menu.log"),"Native file-picker bridge: "+error+Environment.NewLine);}catch(Exception) { }
            }
        }
    }
}
