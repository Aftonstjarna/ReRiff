using System;
using System.IO;
using System.Linq;
using ReRiff;
namespace ReRiff {internal static class NativeReelBridge {internal static void Log(string value){}}}
class MultiplayerCheckpointsSmoke {
    sealed class Server:IProgressionRemote {
        internal ProgressionStore.Checkpoint Saved;
        internal bool Offline,LostAcknowledgement;
        internal int Writes,Resets;
        public string Revision {get{return new string('b',64);}}
        public void PreparePrestige(ulong generation){if(Offline||Saved.Progress.Generation!=generation)throw new IOException("Server unavailable or changed");}
        public bool CanPrestige(ProgressionStore.State state){return state.PrestigeCount<2;}
        public ProgressionStore.Checkpoint Load(){if(Offline)throw new IOException("offline");return Saved;}
        public ProgressionStore.Checkpoint Commit(byte[] bytes,int xp,ulong expected,bool prestige){
            if(Offline)throw new IOException("offline");
            var old=Saved==null?new ProgressionStore.State(0,0,0,0,Revision):Saved.Progress;
            if(expected!=old.Generation)throw new IOException("stale session");
            Writes++;if(prestige)Resets++;
            Saved=new ProgressionStore.Checkpoint(new ProgressionStore.State(expected+1,old.MainXp+(prestige?0UL:(ulong)(xp-old.CycleXp)),xp,old.PrestigeCount+(prestige?1u:0u),Revision),(byte[])bytes.Clone());
            if(LostAcknowledgement){LostAcknowledgement=false;throw new IOException("connection lost after commit");}
            return Saved;
        }
    }
    static void Check(bool value,string text){if(!value)throw new Exception(text);}
    static void Refused(Action action){try{action();}catch(IOException){return;}throw new Exception("Expected refusal");}
    static void Main(){
        string root=Path.Combine(Path.GetTempPath(),"reriff-mp-"+Guid.NewGuid());Directory.CreateDirectory(root);
        try {
            string path=Path.Combine(root,"server-cache.bin");var server=new Server();ProgressionCheckpoints.Remote=server;
            File.WriteAllBytes(path,new byte[]{99});ProgressionCheckpoints.Process(path,0,0);
            Check(!File.Exists(path)&&File.ReadAllBytes(path+".recovery")[0]==99,"New server never imports a local cache");
            int initialSaves=0;
            ProgressionCheckpoints.SaveInitial=()=>{initialSaves++;File.WriteAllBytes(path,new byte[]{1});ProgressionCheckpoints.Process(path,2,0);};
            ProgressionCheckpoints.Process(path,3,0);Check(initialSaves==1,"Fresh server profile serialized through the native save callback");
            Check(ProgressionCheckpoints.Ready&&server.Saved.Progress.Generation==1,"First native save initializes server profile");
            File.WriteAllBytes(path,new byte[]{2});ProgressionCheckpoints.Process(path,2,12000);
            Check(server.Saved.Progress.MainXp==12000,"Server owns permanent progress");
            int writes=server.Writes;ProgressionCheckpoints.Process(path,2,12000);Check(server.Writes==writes,"Unchanged checkpoint is not uploaded");
            int resets=0;Action before=()=>ProgressionCheckpoints.Process(path,2,12000);
            Action reset=()=>{resets++;File.WriteAllBytes(path,new byte[]{3});};
            Action after=()=>ProgressionCheckpoints.Process(path,2,0);
            server.Offline=true;
            Refused(()=>ProgressionCheckpoints.CommitConfirmedPrestige(()=>true,before,reset,after));Check(resets==0,"Unavailable server cannot mutate native progress");
            server.Offline=false;ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,12000);
            Check(File.ReadAllBytes(path)[0]==2,"Reconnect restores server save, ignoring unsent cache");
            server.LostAcknowledgement=true;
            Refused(()=>ProgressionCheckpoints.CommitConfirmedPrestige(()=>true,before,reset,after));
            Check(server.Resets==1&&server.Saved.Progress.PrestigeCount==1,"Reset committed despite lost acknowledgement");
            ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,0);
            Check(ProgressionCheckpoints.DisplayProgress(0).PrestigeCount==1&&File.ReadAllBytes(path)[0]==3,"Restart recovers committed reset without another reset");
            var other=new Server();ProgressionCheckpoints.Remote=other;ProgressionCheckpoints.Process(path,0,0);
            Check(!File.Exists(path)&&server.Saved.Progress.PrestigeCount==1,"Server change cannot import previous server data");
            Check(!Directory.Exists(Path.Combine(root,"ReRiff-progression")),"No local authority manifest for multiplayer");
            Console.WriteLine("PASS: remote initialization, no imports, ordinary saves, pre-reset disconnect, lost-ack recovery and server separation");
        }finally{ProgressionCheckpoints.Remote=null;Directory.Delete(root,true);}
    }
}
