using System;
using System.IO;
using System.Linq;
using ReRiff;

class ProgressionStoreSmoke {
    static int checks;
    static void Check(bool value,string label){checks++;if(!value)throw new Exception(label);}
    static void Refuses(Action action,string label){bool failed=false;try{action();}catch(IOException){failed=true;}Check(failed,label);}
    static readonly string Revision=new string('a',64),ChangedRevision=new string('b',64);
    static void Main(){
        string root=Path.Combine(Path.GetTempPath(),"reriff-checkpoints-"+Guid.NewGuid().ToString("N"));
        try {
            string path=Path.Combine(root,"singleplayer");var store=new ProgressionStore(path,"singleplayer|synthetic-account");
            Check(store.Read()==null&&!Directory.Exists(path),"Absent state must not create files");
            byte[] original={1,2,3,4},race={5,6,7,8},reset={9,10,11,12};
            var first=store.Initialize(original,1234,1234,Revision);
            Check(first.Progress.Generation==1&&first.Progress.MainXp==1234&&first.Progress.PrestigeCount==0,"Initial import");
            var existing=store.Initialize(new byte[]{99},0,0,Revision);
            Check(existing.NativeSave.SequenceEqual(original)&&existing.Progress.MainXp==1234,"Import is one-time");
            var updated=store.Commit(race,new ProgressionStore.State(2,1300,1300,0,Revision),1);
            Check(updated.Progress.Generation==2&&store.Read().NativeSave.SequenceEqual(race),"Normal checkpoint");
            Refuses(()=>store.Commit(reset,new ProgressionStore.State(2,1400,1400,0,Revision),1),"Stale writer refused");
            Refuses(()=>store.Commit(reset,new ProgressionStore.State(3,1299,1300,0,Revision),2),"Permanent XP rollback refused");
            Refuses(()=>store.Commit(reset,new ProgressionStore.State(3,1400,0,0,Revision),2),"Cycle reset without prestige refused");
            Refuses(()=>store.Commit(reset,new ProgressionStore.State(3,1400,1,1,Revision),2),"Prestige must reset cycle XP");
            Refuses(()=>store.Commit(reset,new ProgressionStore.State(3,1400,0,2,Revision),2),"Skipped prestige count refused");
            store.BeforeAtomicCommit=stage=>{if(stage=="manifest")throw new IOException("Simulated crash before commit");};
            Refuses(()=>store.Commit(reset,new ProgressionStore.State(3,1300,0,1,Revision),2),"Interrupted reset refused");
            var afterCrash=store.Read();
            Check(afterCrash.Progress.PrestigeCount==0&&afterCrash.Progress.CycleXp==1300&&afterCrash.NativeSave.SequenceEqual(race),"Interrupted reset retains old save and counters together");
            Check(!Directory.GetFiles(path,"*.tmp").Any(),"Interrupted commit cleans temporary files");
            store.BeforeAtomicCommit=null;
            var prestige=store.Commit(reset,new ProgressionStore.State(3,1300,0,1,Revision),2);
            Check(prestige.Progress.PrestigeCount==1&&prestige.Progress.MainXp==1300&&prestige.Progress.CycleXp==0,"Committed reset retains main XP");
            var reopened=new ProgressionStore(path,"singleplayer|synthetic-account").Read();
            Check(reopened.Progress.Generation==3&&reopened.NativeSave.SequenceEqual(reset),"Restart loads committed generation");
            store.Commit(original,new ProgressionStore.State(4,1300,0,1,ChangedRevision),3);
            Check(store.Read().Progress.PrestigeCount==1&&store.Read().Progress.BalanceRevision==ChangedRevision,"Balance revision preserves progression");
            Check(Directory.GetFiles(path,"save-*.bin").Length==2,"Snapshot storage remains bounded");
            Refuses(()=>new ProgressionStore(path,"server|another-account").Read(),"Cross-domain load refused");
            byte[] manifest=File.ReadAllBytes(Path.Combine(path,"state.bin"));
            File.WriteAllBytes(Path.Combine(path,"state.bin"),new byte[]{0});
            Refuses(()=>store.Read(),"Corrupt manifest refused without resetting");
            File.WriteAllBytes(Path.Combine(path,"state.bin"),manifest);
            File.WriteAllBytes(Path.Combine(path,"save-1.bin"),new byte[]{0});
            Refuses(()=>store.Read(),"Corrupt committed snapshot refused");
            File.WriteAllBytes(Path.Combine(path,"save-1.bin"),original);
            File.Delete(Path.Combine(path,"state.bin"));
            Refuses(()=>store.Initialize(original,0,0,Revision),"Missing manifest cannot silently reimport over snapshots");
            Console.WriteLine("PASS: "+checks+" checkpoint checks; atomic save/counter generations, interrupted-reset recovery, one-time import, domain separation, CAS and corruption refusal");
        } finally {if(Directory.Exists(root))Directory.Delete(root,true);}
    }
}
