using System;
using System.Runtime.InteropServices;
namespace ReRiff {
    internal static class NativeModCache {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate int Prepare(IntPtr root,uint used,IntPtr output,int capacity);
        static readonly Prepare Callback=Build;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        static extern int ReRiffConfigureModCache(IntPtr callback);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        static extern void ReRiffSetCarSlotsActive(int active);
        [DllImport("user32.dll",CharSet=CharSet.Unicode)]
        static extern int MessageBoxW(IntPtr owner,string text,string title,uint flags);
        internal static void Configure(){if(ReRiffConfigureModCache(Marshal.GetFunctionPointerForDelegate(Callback))!=1)throw new InvalidOperationException("Mod discovery already ran; cache adapter was not registered");}
        static int Build(IntPtr root,uint used,IntPtr output,int capacity){
            try{
                var plan=ModArchiveCache.Prepare(Marshal.PtrToStringUni(root),used);
                ReRiffSetCarSlotsActive(ModArchiveCache.CurrentCars!=null?1:0);
                if(plan.Count>capacity)throw new InvalidOperationException("Mod plan exceeds native capacity");
                for(int i=0;i<plan.Count;i++){
                    IntPtr record=new IntPtr(output.ToInt64()+i*524);Marshal.WriteInt32(record,plan[i].Id);
                    char[] path=(plan[i].Stem+"\0").ToCharArray();Marshal.Copy(path,0,new IntPtr(record.ToInt64()+4),path.Length);
                }
                NativeReelBridge.Log("Prepared "+plan.Count+" native mod volumes (loose caches and automatic overflow merge).");return plan.Count;
             }catch(Exception error){if(ModArchiveCache.CarSlotsRequested){NativeReelBridge.Log("Automatic car preparation refused before save access: "+error);MessageBoxW(IntPtr.Zero,"ReRiff could not safely prepare custom car slots. See logs/ReRiff-game.log.","ReRiff car slots",0x10);Environment.Exit(1);return 0;}NativeReelBridge.Log("Mod cache preparation failed; retaining legacy discovery: "+error);return -1;}
        }
    }
}
