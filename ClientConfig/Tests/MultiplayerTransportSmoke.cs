using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using ReRiff;
namespace ReRiff {internal static class NativeReelBridge {internal static void Log(string value){}}}
class MultiplayerTransportSmoke {
    static void Check(bool value,string text){if(!value)throw new Exception(text);}
    static void Refused(Action action){try{action();}catch(IOException){return;}throw new Exception("Expected refusal");}
    static void Automatic(string[] args){
        string folder=Path.Combine(Path.GetTempPath(),"reriff-auto-"+Guid.NewGuid());Directory.CreateDirectory(folder);
        var seen=new Dictionary<string,object>{{"cycleXp",0},{"coins",0},{"achievements",new int[0]},{"equippedCar",5},{"equippedPack",2},{"ownedCars",new[]{5}},{"ownedPacks",new[]{47}}};
        int prompts=0;bool locked=args.Length>3;
        Func<string,bool,string> prompt=(url,retry)=>{prompts++;Check(locked,"Open server must not prompt");return retry?args[3]:"wrong-password";};
        try{
            Check(ProgressionRemote.Connect(folder,args[0]+"/legacy-missing-api",()=>seen,prompt)==null,"Legacy server without progression keeps existing behavior");
            Check(ProgressionRemote.Enabled(folder),"Automatic connection enabled without INI IDs");
            File.WriteAllText(Path.Combine(folder,"server.ini"),"[multiplayer_progression]\nmultiplayer_enabled=false\n");Check(!ProgressionRemote.Enabled(folder),"Existing explicit opt-out stays disabled");File.Delete(Path.Combine(folder,"server.ini"));
            var first=ProgressionRemote.Connect(folder,args[0],()=>seen,prompt);
            Check(first.ServerId==args[1]&&first.AccountId!=args[2],"Server discovered; independent new account");
            Check(first.Load()==null,"New automatic account is fresh");
            first.Commit(new byte[]{20,30},0,0,false);
            var second=ProgressionRemote.Connect(folder,args[0],()=>seen,prompt);
            Check(second.AccountId==first.AccountId&&second.Load().Progress.Generation==1,"Reconnect restores automatically remembered account");
            if(locked)Check(prompts==4,"Locked server prompts and retries incorrect password on each connection");
            string path=Directory.GetFiles(Path.Combine(folder,"localserver/accounts"),"*.json").Single();
            string saved=File.ReadAllText(path);
            Check(!locked||!saved.Contains(args[3]),"Server password is never persisted");
            var json=new System.Web.Script.Serialization.JavaScriptSerializer();
            var state=(Dictionary<string,object>)json.DeserializeObject(saved);
            // Simulate losing the first connect response after server allocation.
            state["accountId"]=null;File.WriteAllText(path,json.Serialize(state));
            Check(ProgressionRemote.Connect(folder,args[0],()=>seen,prompt).AccountId==first.AccountId,"Pending first connect retries same account");
            state["accountId"]=first.AccountId;state["serverId"]="aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";File.WriteAllText(path,json.Serialize(state));
            Refused(()=>ProgressionRemote.Connect(folder,args[0],()=>seen,prompt));
            string migration=Path.Combine(folder,"migration");Directory.CreateDirectory(migration);
            var candidate=ProgressionRemote.Connect(migration,args[0],()=>seen,prompt);
            string importPath=Directory.GetFiles(Path.Combine(migration,"localserver/accounts"),"*.json").Single();
            var importState=(Dictionary<string,object>)json.DeserializeObject(File.ReadAllText(importPath));importState["importOriginalOnce"]=true;File.WriteAllText(importPath,json.Serialize(importState));
            candidate=ProgressionRemote.Connect(migration,args[0],()=>seen,prompt);Check(candidate.ImportOriginal,"Only explicitly marked account imports");
            Check(candidate.Load()==null,"Import target must be empty");
            seen["cycleXp"]=61338574;seen["achievements"]=Enumerable.Range(0,39).Where(i=>i!=18).ToArray();seen["coins"]=2330;
            var imported=candidate.Import(new byte[]{40,50},61338574);
            Check(imported.Progress.MainXp==61338574&&imported.Progress.PrestigeCount==0,"Import preserves original earned XP without adding prestige");
            Check(!File.ReadAllText(importPath).Contains("importOriginalOnce"),"Successful import consumes its one-time request");
            candidate.PreparePrestige(1);
            seen["cycleXp"]=0;seen["achievements"]=new[]{18};seen["coins"]=0;
            Check(candidate.Commit(new byte[]{60,70},0,1,true).Progress.PrestigeCount==1,"Imported completed profile can prestige");
            File.WriteAllText(path,"invalid cache");
            bool failed=false;try{ProgressionRemote.Connect(folder,args[0],()=>seen,prompt);}catch(Exception){failed=true;}Check(failed,"Corrupt account must not silently create another account");
        }finally{Directory.Delete(folder,true);}
        Console.WriteLine("PASS: automatic identity, persisted/retried account, password handling, and changed-server refusal");
    }
    static void Main(string[] args){
        Automatic(args);if(args.Length>3)return;
        var config=new Dictionary<string,string>{{"multiplayer_url",args[0]},{"multiplayer_server_id",args[1]},{"multiplayer_account_id",args[2]},{"multiplayer_token",new string('a',64)}};
        var observed=new Dictionary<string,object>{{"cycleXp",0},{"coins",0},{"achievements",new int[0]},{"equippedCar",5},{"equippedPack",2},{"ownedCars",new[]{5}},{"ownedPacks",new[]{47}}};
        var client=new ProgressionRemote(args[0],config,()=>observed);
        Check(client.Load()==null,"Fresh server has no imported checkpoint");
        Check(ProgressionBalance.Current.IntervalDiscount&&ProgressionBalance.Current.Discount(10)==.20m,"Server balance applied to native economy");
        var first=client.Commit(new byte[]{1,2,3},0,0,false);
        Check(first.Progress.Generation==1&&first.Progress.PrestigeCount==0,"Initial server generation");
        observed["cycleXp"]=12000;observed["achievements"]=Enumerable.Range(0,39).Where(i=>i!=18).ToArray();observed["coins"]=1000;
        var earned=client.Commit(new byte[]{4,5,6},12000,1,false);
        Check(earned.Progress.MainXp==12000&&client.CanPrestige(earned.Progress),"Earned progress and eligibility");
        client.PreparePrestige(2);
        observed["cycleXp"]=0;observed["coins"]=0;observed["achievements"]=new[]{18};
        var prestige=client.Commit(new byte[]{7,8,9},0,2,true);
        Check(prestige.Progress.PrestigeCount==1&&prestige.Progress.MainXp==12000&&prestige.Progress.CycleXp==0,"Confirmed prestige across real HTTP transport");
        var restarted=new ProgressionRemote(args[0],config,()=>observed);
        var loaded=restarted.Load();Check(loaded.Progress.PrestigeCount==1&&loaded.NativeSave.SequenceEqual(new byte[]{7,8,9}),"Reconnect restores exact reset save");
        Check(ProgressionRewards.Coins(100,0,loaded.Progress.PrestigeCount,ProgressionBalance.Current.CoinRate,ProgressionBalance.Current.PrestigeGrowth,ProgressionBalance.Current.MultiplierCap)==200,"Post-prestige native coin quote");
        Refused(()=>client.Commit(new byte[]{0},0,1,false));
        config["multiplayer_token"]=new string('b',64);Refused(()=>new ProgressionRemote(args[0],config,()=>observed).Load());
        config["multiplayer_token"]=new string('a',64);config["multiplayer_account_id"]="aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";Refused(()=>new ProgressionRemote(args[0],config,()=>observed).Load());
        config["multiplayer_account_id"]=args[2];config["multiplayer_server_id"]="12345678-1234-1234-1234-123456789abc";Refused(()=>new ProgressionRemote(args[0],config,()=>observed).Load());
        Refused(()=>new ProgressionRemote("http://wrong.example",config,()=>observed));
        Console.WriteLine("PASS: authenticated real HTTP checkpoints, server balance, confirmed prestige, restart, doubled reward, stale-session and identity refusal");
    }
}
