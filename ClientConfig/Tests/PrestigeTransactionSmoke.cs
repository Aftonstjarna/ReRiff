using System;
using System.IO;
using System.Linq;
using ReRiff;

namespace ReRiff {internal static class NativeReelBridge {internal static void Log(string text){}}}
class PrestigeTransactionSmoke {
    static void Check(bool value,string text){if(!value)throw new Exception(text);}
    static bool Refused(Action operation){try{operation();return false;}catch(InvalidOperationException){return true;}catch(IOException){return true;}}
    static void Main(){
        string root=Path.Combine(Path.GetTempPath(),"reriff-prestige-transaction-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try {
            string path=Path.Combine(root,"reriff-sp-fixture.bin");byte[] initial={1,2,3},reset={4,5,6},race={7,8,9};
            File.WriteAllBytes(path,initial);ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,1234);
            var store=new ProgressionStore(Path.Combine(root,Path.Combine("ReRiff-progression","reriff-sp-fixture")),"singleplayer|reriff-sp-fixture");
            int saves=0,resets=0;
            Action before=delegate{saves++;ProgressionCheckpoints.Process(path,2,1234);};
            Action mutate=delegate{resets++;File.WriteAllBytes(path,reset);};
            Action after=delegate{ProgressionCheckpoints.Process(path,2,0);};
            Check(Refused(delegate{ProgressionCheckpoints.CommitConfirmedPrestige(()=>false,before,mutate,after);})&&saves==0&&resets==0,"Ineligible cannot save or reset");
            int queries=0;
            Check(Refused(delegate{ProgressionCheckpoints.CommitConfirmedPrestige(()=>++queries==1,before,mutate,after);})&&resets==0,"Recheck after pre-reset save");
            Action interrupted=delegate{mutate();throw new IOException("Injected interruption after native reset");};
            Check(Refused(delegate{ProgressionCheckpoints.CommitConfirmedPrestige(()=>true,before,interrupted,after);}),"Interrupted native reset fails");
            Check(store.Read().Progress.PrestigeCount==0&&store.Read().NativeSave.SequenceEqual(initial),"Interrupted reset preserves committed native save and count");
            ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,1234);
            Check(File.ReadAllBytes(path).SequenceEqual(initial),"Restart recovers pre-reset state");
            Check(Refused(delegate{ProgressionCheckpoints.CommitConfirmedPrestige(()=>true,before,mutate,()=>{});}),"Skipped native save cannot report success");
            Check(store.Read().Progress.PrestigeCount==0,"Skipped native save does not advance count");
            ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,1234);
            ProgressionCheckpoints.CommitConfirmedPrestige(()=>true,before,mutate,after);
            var committed=store.Read();Check(committed.Progress.MainXp==1234&&committed.Progress.CycleXp==0&&committed.Progress.PrestigeCount==1,"Reset preserves main XP, zeroes cycle and increments count");
            Check(committed.NativeSave.SequenceEqual(reset),"Native reset bytes and counters commit together");
            ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,0);
            File.WriteAllBytes(path,race);ProgressionCheckpoints.Process(path,2,50);
            committed=store.Read();Check(committed.Progress.MainXp==1284&&committed.Progress.CycleXp==50&&committed.Progress.PrestigeCount==1,"Post-prestige race advances permanent and cycle XP independently");
            Check(Refused(delegate{ProgressionCheckpoints.Process(path,2,0);})&&store.Read().Progress.PrestigeCount==1,"Later XP decrease cannot auto-prestige");
            Console.WriteLine("PASS: confirmed prestige transaction, eligibility recheck, interruption/skipped-save recovery, atomic counters, restart and post-prestige XP; synthetic native operations only");
        }finally{Directory.Delete(root,true);}
    }
}
