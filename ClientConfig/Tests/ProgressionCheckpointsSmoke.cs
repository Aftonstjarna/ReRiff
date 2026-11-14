using System;
using System.IO;
using System.Linq;
using ReRiff;

namespace ReRiff {internal static class NativeReelBridge {internal static void Log(string text){}}}
class ProgressionCheckpointsSmoke {
    static void Check(bool value,string text){if(!value)throw new Exception(text);}
    static void Main(){
        string root=Path.Combine(Path.GetTempPath(),"reriff-native-checkpoints-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try {
            string path=Path.Combine(root,"reriff-sp-fixture.bin");byte[] initial={1,2,3},race={4,5,6},interrupted={9,8,7};
            File.WriteAllBytes(path,initial);ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,1234);
            string bank=Path.Combine(root,Path.Combine("ReRiff-progression","reriff-sp-fixture"));
            var store=new ProgressionStore(bank,"singleplayer|reriff-sp-fixture");
            Check(store.Read().Progress.MainXp==1234&&store.Read().Progress.PrestigeCount==0,"Initial native load");
            File.WriteAllBytes(path,race);ProgressionCheckpoints.Process(path,2,1300);
            Check(store.Read().Progress.MainXp==1300&&store.Read().Progress.CycleXp==1300,"Native XP observation");
            Check(store.Read().NativeSave.SequenceEqual(race),"Native save snapshot");
            ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,1300);
            Check(store.Read().Progress.Generation==2,"Restart does not reimport or advance generation");
            ProgressionCheckpoints.Process(path,2,1300);Check(store.Read().Progress.Generation==2,"Unchanged native save is not duplicated");
            File.WriteAllBytes(path,interrupted);ProgressionCheckpoints.Process(path,0,0);
            Check(File.ReadAllBytes(path).SequenceEqual(race),"Restore only committed state after interruption");
            Check(File.ReadAllBytes(path+".recovery").SequenceEqual(interrupted),"Preserve uncommitted bytes for recovery");
            File.Delete(path);ProgressionCheckpoints.Process(path,0,0);
            Check(File.ReadAllBytes(path).SequenceEqual(race),"Missing working file restores from checkpoint without cloud reimport");
            ProgressionCheckpoints.Process(path,1,1300);
            bool refused=false;try{ProgressionCheckpoints.Process(path,2,0);}catch(IOException){refused=true;}
            Check(refused&&store.Read().Progress.PrestigeCount==0,"No automatic prestige/reset from changed XP");
            // The real original level table ends at 1 billion XP (level 40).
            // A prestige cycle must keep contributing to the permanent total
            // across that boundary, including totals beyond signed native XP.
            string cappedPath=Path.Combine(root,"reriff-sp-capped.bin");
            var cappedStore=new ProgressionStore(Path.Combine(root,Path.Combine("ReRiff-progression","reriff-sp-capped")),"singleplayer|reriff-sp-capped");
            string revision=store.Read().Progress.BalanceRevision;
            var capped=cappedStore.Initialize(initial,1000000000,1000000000,revision);
            capped=cappedStore.Commit(race,new ProgressionStore.State(capped.Progress.Generation+1,1000000000,0,1,revision),capped.Progress.Generation);
            File.WriteAllBytes(cappedPath,race);
            ProgressionCheckpoints.Process(cappedPath,0,0);ProgressionCheckpoints.Process(cappedPath,1,0);
            var preview=ProgressionCheckpoints.DisplayProgress(250000);
            Check(preview.MainXp==1000250000&&preview.CycleXp==250000&&preview.PrestigeCount==1,"Prestige XP previews above the main level cap");
            Check(cappedStore.Read().Progress.MainXp==1000000000,"Display preview does not credit XP twice");
            File.WriteAllBytes(cappedPath,interrupted);ProgressionCheckpoints.Process(cappedPath,2,250000);
            Check(cappedStore.Read().Progress.MainXp==1000250000,"Prestige XP commits above the main level cap");
            ProgressionCheckpoints.Process(cappedPath,2,2000000000);
            Check(cappedStore.Read().Progress.MainXp==3000000000UL,"Permanent XP exceeds signed native XP without losing prestige gains");
            ulong generation=cappedStore.Read().Progress.Generation;
            ProgressionCheckpoints.Process(cappedPath,0,0);ProgressionCheckpoints.Process(cappedPath,1,2000000000);
            ProgressionCheckpoints.Process(cappedPath,2,2000000000);
            capped=cappedStore.Read();
            Check(capped.Progress.Generation==generation&&capped.Progress.MainXp==3000000000UL&&capped.Progress.CycleXp==2000000000&&capped.Progress.PrestigeCount==1,"Capped main-level XP survives restart and repeated save exactly once");
            Console.WriteLine("PASS: native checkpoints, permanent prestige XP beyond the main level cap and signed native range, restart reuse, interrupted recovery and no automatic reset");
        }finally{Directory.Delete(root,true);}
    }
}
