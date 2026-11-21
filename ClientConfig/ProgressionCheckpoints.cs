using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

namespace ReRiff {
    // Experimental checkpoint integration. Only the confirmed Credits callback
    // starts a reset; ordinary native saves retain independent permanent XP.
    internal interface IProgressionRemote {
        string Revision {get;}
        ProgressionStore.Checkpoint Load();
        ProgressionStore.Checkpoint Commit(byte[] native,int cycleXp,ulong generation,bool prestige);
        bool CanPrestige(ProgressionStore.State state);
        void PreparePrestige(ulong generation);
    }
    internal interface IProgressionImport {
        bool ImportOriginal {get;}
        ProgressionStore.Checkpoint Import(byte[] native,int cycleXp);
    }
    internal static class ProgressionCheckpoints {
        internal static IProgressionRemote Remote;
        internal static Action SaveInitial;
        internal static Action<string> PrepareCarSave,ObserveCarLoad,AfterCarLoad;
        internal static bool CanPrestige {get{return Ready&&(Remote==null||Remote.CanPrestige(checkpoint.Progress));}}
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int NativeEvent(IntPtr path,int phase,int nativeXp);
        private static readonly NativeEvent Callback=Observe;
        internal static IntPtr CallbackPointer {get{return Marshal.GetFunctionPointerForDelegate(Callback);}}
        private static ProgressionStore store;
        private static ProgressionStore.Checkpoint checkpoint;
        private static string currentPath;
        private static ProgressionStore.State pendingPrestige;
        internal static bool Ready {get{return (store!=null||Remote!=null)&&checkpoint!=null&&pendingPrestige==null;}}
        internal static ulong Generation {get{if(!Ready)throw new IOException("Progression checkpoint is not ready");return checkpoint.Progress.Generation;}}
        internal static string PurchaseTestRequest {get{return !Ready?null:Path.Combine(Path.Combine(Path.GetDirectoryName(currentPath),Path.Combine("ReRiff-progression",Path.GetFileNameWithoutExtension(currentPath))),"purchase-test.once");}}
        internal static ProgressionStore.State DisplayProgress(int nativeCycleXp){
            if(!Ready||nativeCycleXp<checkpoint.Progress.CycleXp||nativeCycleXp>2000000000)return null;
            var old=checkpoint.Progress;
            return new ProgressionStore.State(old.Generation,checked(old.MainXp+(ulong)(nativeCycleXp-old.CycleXp)),nativeCycleXp,old.PrestigeCount,old.BalanceRevision);
        }
        // Called only by the confirmed native Credits action.
        // The supplied operations must use
        // original native saving/reset/setter methods on the game thread.
        // Any exception after mutation requires stopping the game: restart
        // restores the last whole checkpoint, not partially reset RAM.
        internal static void CommitConfirmedPrestige(Func<bool> eligible,Action saveBefore,Action resetAndRetain,Action saveAfter){
            if(pendingPrestige!=null)throw new InvalidOperationException("Prestige transaction already active");
            if(eligible==null||saveBefore==null||resetAndRetain==null||saveAfter==null)throw new ArgumentNullException("Native prestige operations required");
            if(checkpoint==null||!CanPrestige||!eligible())throw new InvalidOperationException("Prestige is not eligible or the save is not ready");
            saveBefore(); // Preserve current race/garage changes through native serialization.
            if(Remote!=null)Remote.PreparePrestige(checkpoint.Progress.Generation);
            if(!CanPrestige||!eligible())throw new InvalidOperationException("Prestige eligibility changed before reset");
            var old=checkpoint.Progress;
            if(!File.ReadAllBytes(currentPath).SequenceEqual(checkpoint.NativeSave))throw new IOException("Native save did not reach its checkpoint before prestige");
            var next=new ProgressionStore.State(checked(old.Generation+1),old.MainXp,0,checked(old.PrestigeCount+1),old.BalanceRevision);
            try {
                pendingPrestige=next;
                resetAndRetain();saveAfter();
                if(checkpoint.Progress.Generation!=next.Generation||checkpoint.Progress.PrestigeCount!=next.PrestigeCount
                    ||checkpoint.Progress.CycleXp!=0)throw new IOException("Native prestige save did not commit; restart to recover");
            }finally{pendingPrestige=null;}
        }
        private static string NativeRevision { get { return Remote==null?Revision():Remote.Revision; } }
        private static string Revision(){using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes("ReRiff default singleplayer coin-pack discounts v1; scoped race and creator rewards v1; post-race permanent Rank recheck v1; singleplayer normal-ghost Friend or Foe v1|"+ProgressionBalance.Current.Revision))).Replace("-","").ToLowerInvariant();}
        private static void Atomic(string path,byte[] bytes){
            string temporary=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try {
                using(var stream=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None)){
                    stream.Write(bytes,0,bytes.Length);stream.Flush(true);
                }
                if(File.Exists(path))File.Replace(temporary,path,null);else File.Move(temporary,path);
            }finally{if(File.Exists(temporary))File.Delete(temporary);}
        }
        private static int Observe(IntPtr path,int phase,int nativeXp){
            try {string file=Marshal.PtrToStringUni(path);
                if(phase==2&&PrepareCarSave!=null)PrepareCarSave(file);
                Process(file,phase,nativeXp);
                if((phase==0||phase==1)&&ObserveCarLoad!=null)ObserveCarLoad(file);
                if(phase==1&&AfterCarLoad!=null)AfterCarLoad(file);
                return 1;}
            catch(Exception error){NativeReelBridge.Log("Progression checkpoint refused: "+error);return 0;}
        }
        internal static void Process(string path,int phase,int nativeXp){
            path=Path.GetFullPath(path);
            if(phase==0){
                if(pendingPrestige!=null)throw new IOException("Cannot reload a domain during prestige");
                string name=Path.GetFileNameWithoutExtension(path);
                store=Remote==null?new ProgressionStore(Path.Combine(Path.GetDirectoryName(path),Path.Combine("ReRiff-progression",name)),"singleplayer|"+name):null;
                checkpoint=null;
                checkpoint=Remote==null?store.Read():Remote.Load();currentPath=path;
                // A local multiplayer file is only a working cache. A new
                // server profile must never import it or the original save.
                if(Remote!=null&&checkpoint==null&&File.Exists(path)){
                    Atomic(path+".recovery",File.ReadAllBytes(path));File.Delete(path);
                }
                if(checkpoint!=null){
                    byte[] working=File.Exists(path)?File.ReadAllBytes(path):null;
                    if(working==null||!working.SequenceEqual(checkpoint.NativeSave)){
                        // Preserve the uncommitted working copy before restoring
                        // the last whole save/counter checkpoint. One recovery
                        // file is replaced on a later recovery, not accumulated.
                        if(working!=null)Atomic(path+".recovery",working);Atomic(path,checkpoint.NativeSave);
                        NativeReelBridge.Log("Progression restored its last committed save/counter generation; working copy preserved for recovery.");
                    }
                }
                return;
            }
            if((store==null&&Remote==null)||path!=currentPath)throw new IOException("Progression checkpoint domain was not prepared");
            if(nativeXp<0||nativeXp>2000000000)throw new IOException("Native cycle XP out of range");
            if(phase==3){
                if(Remote==null||checkpoint!=null||nativeXp!=0||SaveInitial==null)throw new IOException("Unexpected server initialization event");
                SaveInitial();
                if(checkpoint==null)throw new IOException("Original save did not initialize the server checkpoint");
                return;
            }
            byte[] bytes=File.ReadAllBytes(path);
            if(Remote!=null){
                if(phase!=1&&phase!=2)throw new ArgumentException("Unknown native checkpoint phase");
                if(checkpoint==null){
                    var import=Remote as IProgressionImport;
                    if(phase==1&&import!=null&&import.ImportOriginal)checkpoint=import.Import(bytes,nativeXp);
                    else {
                        if(nativeXp!=0)throw new IOException("A new server profile cannot import existing XP");
                        checkpoint=Remote.Commit(bytes,nativeXp,0,false);
                    }
                }else if(phase==1){
                    if(checkpoint.Progress.CycleXp!=nativeXp)throw new IOException("Native XP differs from server checkpoint");
                }else{
                    var previous=checkpoint.Progress;
                    if(pendingPrestige==null&&nativeXp==previous.CycleXp&&bytes.SequenceEqual(checkpoint.NativeSave)&&previous.BalanceRevision==NativeRevision)return;
                    var committed=Remote.Commit(bytes,nativeXp,previous.Generation,pendingPrestige!=null);
                    if(committed.Progress.PrestigeCount!=(pendingPrestige==null?previous.PrestigeCount:pendingPrestige.PrestigeCount))throw new IOException("Server prestige count differs");
                    checkpoint=committed;
                }
            }else if(phase==1){
                if(checkpoint==null)checkpoint=store.Initialize(bytes,(ulong)nativeXp,nativeXp,NativeRevision);
                if(checkpoint.Progress.CycleXp!=nativeXp)throw new IOException("Native XP differs from committed progression");
            }else if(phase==2){
                if(checkpoint==null)throw new IOException("Native load must precede a progression checkpoint");
                var previous=checkpoint.Progress;
                if(pendingPrestige!=null){
                    if(nativeXp!=0||pendingPrestige.Generation!=checked(previous.Generation+1))throw new IOException("Prestige save must contain zero cycle XP and one new generation");
                    checkpoint=store.Commit(bytes,pendingPrestige,previous.Generation);
                }else{
                if(nativeXp<previous.CycleXp)throw new IOException("Unexpected cycle XP reset; prestige is not enabled");
                if(nativeXp==previous.CycleXp&&bytes.SequenceEqual(checkpoint.NativeSave)&&previous.BalanceRevision==NativeRevision)return;
                var next=new ProgressionStore.State(checked(previous.Generation+1),checked(previous.MainXp+(ulong)(nativeXp-previous.CycleXp)),nativeXp,previous.PrestigeCount,NativeRevision);
                checkpoint=store.Commit(bytes,next,previous.Generation);
                }
            }else throw new ArgumentException("Unknown native checkpoint phase");
            var value=checkpoint.Progress;
            NativeReelBridge.Log("Progression checkpoint: generation="+value.Generation+" permanent_xp="+value.MainXp+" cycle_xp="+value.CycleXp+" prestige="+value.PrestigeCount);
        }
    }
}
