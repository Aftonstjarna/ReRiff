using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Collections.Generic;
namespace ReRiff {
    internal static class GhostCarTest {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int Process(IntPtr root,IntPtr name,int phase,int installed,IntPtr patch);
        private static readonly Process Callback=Run;
        private static string folder;private static readonly HashSet<string> reported=new HashSet<string>(StringComparer.Ordinal);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)] private static extern int ReRiffConfigureGhostCarTest(IntPtr callback);
        internal static void Configure(){
            folder=AppDomain.CurrentDomain.BaseDirectory;if(!GhostCarTestFormat.Enabled(folder))return;
            if(ReRiffConfigureGhostCarTest(Marshal.GetFunctionPointerForDelegate(Callback))!=1)throw new IOException("Ghost car test registration failed");
            NativeReelBridge.Log("Ghost car fallback armed: stock Pulse replay representation only; custom model sharing deferred.");
        }
        private static int Run(IntPtr rootPointer,IntPtr namePointer,int phase,int installed,IntPtr patch){
            try{
                string name=Marshal.PtrToStringAnsi(namePointer),root=Marshal.PtrToStringUni(rootPointer);
                if(name==null||root==null||!Regex.IsMatch(name,@"^[A-Za-z0-9]{1,64}-[0-9]{1,20}\.ghost$"))return 0;
                string path=Path.Combine(root,name);if(!File.Exists(path)||new FileInfo(path).Length>GhostCarTestFormat.MaxSize)return 0;
                byte[] bytes=File.ReadAllBytes(path);
                if(phase!=0&&phase!=1)return 0;
                byte[] wire=GhostCarTestFormat.Publish(bytes,null);if(ReferenceEquals(wire,bytes))return 0;
                string temp=path+".reriff-car-new",backup=path+".reriff-car-original";
                using(var stream=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){stream.Write(wire,0,wire.Length);stream.Flush(true);}
                try {File.Replace(temp,path,File.Exists(backup)?null:backup);}
                finally {if(File.Exists(temp))File.Delete(temp);}
                NativeReelBridge.Log("Ghost car fallback: stock Pulse header saved without custom identity; original recording backed up.");
                // Before-open canonicalization also covers old cached test files.
                // Never hand a custom-car overlay to the native read hook.
                return phase==1?1:0;
            }catch(Exception error){
                string key=error.GetType().Name+": "+error.Message;
                lock(reported){if(!reported.Add(key))return -1;}
                NativeReelBridge.Log("Ghost car test refused: "+key);return -1;
            }
        }
    }
}
