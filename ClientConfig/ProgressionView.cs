using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace ReRiff {
    internal static class ProgressionView {
        internal delegate float ProfileLevel(IntPtr driver,IntPtr xp);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int Summary(IntPtr text,int capacity,IntPtr factor,int cycleXp);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void Fill(IntPtr menu,float factor);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int LocalTest();
        private static readonly Summary Display=Render;
        private static readonly LocalTest PurchaseTest=ProgressionPurchaseTest.Run;
        private static Fill fill;
        private static ProfileLevel nativeLevel;
        private static Func<IntPtr> driver;
        private static bool errorLogged;
        private static string lastIdentity;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionView([In] IntPtr[] callbacks,int count);
        internal static string Identity(int level,uint prestige){
            if(level<0)throw new ArgumentOutOfRangeException("level");
            // The native view replaces this layout marker with difficulty sprite 89;
            // xN is omitted for the first prestige. No font pack is required.
            return level.ToString(CultureInfo.InvariantCulture)+(prestige==0?"":" *"+(prestige==1?"":"x"+prestige.ToString(CultureInfo.InvariantCulture)));
        }
        internal static int MainLevel(ProgressionStore.State progress){
            if(progress==null||nativeLevel==null||driver==null)throw new InvalidOperationException("Permanent level is not ready");
            return (int)PermanentLevel(driver(),progress.MainXp,nativeLevel);
        }
        internal static decimal PermanentLevel(IntPtr currentDriver,ulong xp,ProfileLevel calculate){
            byte[] vector=ProgressionNative.Read(currentDriver,12);
            uint first=BitConverter.ToUInt32(vector,0),last=BitConverter.ToUInt32(vector,4),end=BitConverter.ToUInt32(vector,8);
            if(first==0||last<first||end<last||(last-first)%4!=0||(last-first)/4<2||(last-first)/4>128)throw new IOException("Invalid native level table");
            int count=(int)((last-first)/4);if(count!=41)throw new IOException("Original main level table length differs");
            byte[] thresholds=ProgressionNative.Read(new IntPtr(unchecked((int)first)),count*4);
            int previous=-1;
            for(int i=0;i<count;i++){
                int value=BitConverter.ToInt32(thresholds,i*4);
                if(value<0||value<=previous)throw new IOException("Native level thresholds are not increasing");previous=value;
            }
            if(xp>=(ulong)previous)return ProgressionBalance.Current.ExtendedMainLevel(xp,count-1,previous,previous-BitConverter.ToInt32(thresholds,(count-2)*4));
            IntPtr explicitXp=Marshal.AllocHGlobal(4);
            try {
                Marshal.WriteInt32(explicitXp,(int)Math.Min(xp,(ulong)previous));
                float result=calculate(currentDriver,explicitXp);
                if(Single.IsNaN(result)||Single.IsInfinity(result)||result<0||result>40)throw new IOException("Invalid original profile level");
                return (decimal)(double)result;
            }finally{Marshal.FreeHGlobal(explicitXp);}
        }
        private static int Render(IntPtr text,int capacity,IntPtr factor,int cycleXp){
            try {
                var progress=ProgressionCheckpoints.DisplayProgress(cycleXp);
                if(progress==null||capacity<2||text==IntPtr.Zero||factor==IntPtr.Zero||nativeLevel==null||driver==null)return 0;
                decimal level=PermanentLevel(driver(),progress.MainXp,nativeLevel);
                string identity=Identity((int)level,progress.PrestigeCount);
                if(identity.Length>=capacity)return 0;
                Marshal.Copy((identity+"\0").ToCharArray(),0,text,identity.Length+1);
                Marshal.Copy(new[]{ProgressionBalance.Current.MainLevelFill(level)},0,factor,1);
                if(lastIdentity!=identity){lastIdentity=identity;NativeReelBridge.Log("Permanent-level Profile display: main_level="+(int)level+" prestige="+progress.PrestigeCount+" native_cycle_xp="+cycleXp+" permanent_xp="+progress.MainXp);}
                return 1;
            }catch(Exception error){if(!errorLogged){errorLogged=true;NativeReelBridge.Log("Permanent-level display unavailable: "+error);}return 0;}
        }
        internal static void Configure(Module game){
            var level=game.ResolveMethod(0x06001036) as MethodInfo;
            var bar=game.ResolveMethod(0x060013ec) as MethodInfo;
            if(level==null||level.Name!="gaDriverLevel.getDriverProfileLevel"||bar==null||bar.Name!="gaDriverProfileMenu.setFillbarFactor")throw new IOException("Original level display methods differ");
            nativeLevel=(ProfileLevel)NativeReelBridge.CreateCall(level,typeof(ProfileLevel),false);
            fill=(Fill)NativeReelBridge.CreateCall(bar,typeof(Fill),false);
            driver=ProgressionNative.Singleton(game,0x04000f11,"?ms_pInstance@gaDriverLevel@@0PAV1@A");
            try {
                if(ReRiffConfigureProgressionView(new[]{Marshal.GetFunctionPointerForDelegate(Display),Marshal.GetFunctionPointerForDelegate(fill),Marshal.GetFunctionPointerForDelegate(PurchaseTest)},3)!=1)throw new IOException("Native profile display registration failed");
            }catch(EntryPointNotFoundException){NativeReelBridge.Log("Native profile display requires the matching experimental menu DLL.");}
        }
    }
}
