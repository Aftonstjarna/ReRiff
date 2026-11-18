using System;
using System.Collections.Generic;
using System.IO;
using ReRiff;
class LastFmSmoke {
    static int checks;
    static void Check(bool value,string name){if(!value)throw new Exception(name);checks++;}
    static LastFmTrack Track(double duration){return new LastFmTrack{Artist="Artist",Title="Song",Duration=duration,Started=123};}
    static LastFmPlayback Play(int percent,double duration,int seconds){var p=new LastFmPlayback(percent);p.Start(Track(duration),0,0);p.Observe(0,0,true,false);for(int i=1;i<=seconds;i++)p.Observe(i,i,true,false);return p;}
    static int Main(string[] commandLine){
        Check(LastFmConfig.ReadCredential("dpapi:00")=="","invalid protected credential fails closed");
        Check(LastFmConfig.ReadCredential("dpapi:zz")=="","malformed protected credential fails closed");
        if(commandLine.Length==1){var native=LastFmConfig.Load(commandLine[0]);Check(native.Ready&&native.Key==new string('a',32)&&native.Secret==new string('b',32)&&native.Session==new string('Z',32),"native DPAPI credentials readable by game");}
        Check(LastFmConfig.ValidToken("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ-_"),"opaque URL-safe auth tokens");
        Check(!LastFmConfig.ValidToken("bad&token"),"malformed token rejected");
        Check(new LastFmConfig{Enabled=true,Key=new string('a',32),Secret=new string('b',32),Session=new string('Z',32),User="user"}.Ready,"opaque session accepted");
        var parameters=new Dictionary<string,string>{{"method","auth.getToken"},{"api_key","test"},{"format","json"}};
        Check(LastFmApi.Signature(parameters,"secret")=="0bf56d6ed6962e4fd19bdf818c538e65","signature vector");
        parameters["callback"]="ignored";parameters["api_sig"]="ignored";Check(LastFmApi.Signature(parameters,"secret")=="0bf56d6ed6962e4fd19bdf818c538e65","excluded signature fields");
        Check(LastFmApi.Encode(new Dictionary<string,string>{{"track","雪 & +"}})=="track=%E9%9B%AA%20%26%20%2B","UTF8 form encoding");
        var p=Play(80,100,79);Check(!p.Observe(79,79,true,false),"before threshold");Check(p.Observe(80,80,true,false),"80 percent");Check(!p.Observe(81,81,true,true),"no duplicate on completion");
        p=Play(80,100,60);Check(p.Observe(60,60,false,true),"completed race before 80");
        p=Play(1,100,49);Check(!p.Observe(49,49,false,true),"minimum half track");Check(p.Observe(50,50,true,true)==false,"resume does not credit paused interval");Check(p.Observe(51,51,true,true),"minimum reached after resume");
        p=Play(100,1000,239);Check(!p.Observe(239,239,false,true),"four minute gate");p.Observe(240,240,true,false);Check(p.Observe(241,241,true,true),"completed long track after four minutes");
        p=Play(80,30,30);Check(!p.Observe(30,30,false,true),"short tracks ignored");
        p=Play(80,100,10);p.Observe(10,100,false,false);p.Observe(10,101,true,false);p.Observe(11,102,true,false);Check(p.Listened==11,"pause excluded");
        p.Observe(90,103,true,false);Check(p.Listened==11,"seek excluded");p.Observe(91,203,true,false);Check(p.Listened==11,"stall excluded");
        p.Start(Track(100),0,204);Check(p.Listened==0,"restart resets listening");
        Check(!new LastFmTrack{Artist="unknown artist",Title="Song",Duration=100}.Valid,"missing metadata");
        var t=Track(100);t.Artist="雪";t.Title="曲";Check(t.Valid&&t.Parameters("session",true)["timestamp"]=="123","tagged Unicode metadata and original timestamp");
        string folder=Path.Combine(Path.GetTempPath(),"reriff-lastfm-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(folder);
        try{
            string file=Path.Combine(folder,"client.ini");File.WriteAllText(file,"[launcher]\nskip=true\n[lastfm]\napi_key = preserved\nshared_secret = preserved-secret\nenabled = false\n");
            LastFmConfig.Save(folder,new Dictionary<string,string>{{"enabled","true"},{"scrobble_percent","90"}});
            var c=LastFmConfig.Load(folder);Check(c.Enabled&&c.Percent==90&&c.Key=="preserved"&&c.Secret=="preserved-secret","preserve credentials while saving");Check(File.ReadAllText(file).Contains("skip=true"),"preserve other sections");
            File.WriteAllText(file,File.ReadAllText(file)+"\n[lastfm]\nenabled = false\n[graphics]\nwidth=1920\n",System.Text.Encoding.Unicode);LastFmConfig.Save(folder,new Dictionary<string,string>{{"enabled","true"}});Check(LastFmConfig.Load(folder).Enabled,"duplicate section keys replaced");Check(File.ReadAllText(file).Contains("width=1920"),"following section preserved");
            var cfg=new LastFmConfig{Key="app",Session="session",Secret="secret"};
            string queue=Path.Combine(folder,"queue.json"),owner=LastFmOutbox.Account(cfg);var box=new LastFmOutbox(queue,owner);
            var queued=Track(100);box.Add(queued);box.Add(queued);box.Flush();Check(box.Count==1,"queue deduplicates same playback");
            Check(!File.ReadAllText(queue).Contains("session")&&!File.ReadAllText(queue).Contains("secret"),"queue excludes credentials");
            box=new LastFmOutbox(queue,owner);Check(box.Count==1,"queue survives restart");Check(new LastFmOutbox(queue,"another-account").Count==0,"account isolation");
            DateTime now=new DateTime(2026,1,1,0,0,0,DateTimeKind.Utc);int requests=0;
            Func<LastFmConfig,string,IDictionary<string,string>,Dictionary<string,object>> offline=(config,method,args)=>{requests++;throw new LastFmFailure(11);};
            box.Deliver(cfg,now,offline);Check(box.Count==1&&box.NextAttempt==now.AddSeconds(15),"temporary failure retained with backoff");
            box.Deliver(cfg,now.AddSeconds(14),offline);Check(requests==1,"no early retries");box.Deliver(cfg,now.AddSeconds(15),offline);Check(requests==2&&box.NextAttempt==now.AddSeconds(45),"backoff increases");
            Func<LastFmConfig,string,IDictionary<string,string>,Dictionary<string,object>> accepted=(config,method,args)=>{
                Check(method=="track.scrobble"&&args["timestamp"]=="123","retry keeps original timestamp");
                return new Dictionary<string,object>{{"scrobbles",new Dictionary<string,object>{{"@attr",new Dictionary<string,object>{{"accepted","1"},{"ignored","0"}}}}}};
            };
            box.Deliver(cfg,now.AddSeconds(45),accepted);Check(box.Count==0&&new LastFmOutbox(queue,owner).Count==0,"success removes durable entry");
            box.Add(queued);box.Deliver(cfg,now.AddSeconds(48),(config,method,args)=>{throw new LastFmFailure(9);});Check(box.Stopped&&box.Count==1,"invalid session stops requests without losing queue");
            Check(ConfigFiles.Name("lastfm")=="client.ini","client config route");
            LastFmConfig.Save(folder,new Dictionary<string,string>{{"scrobble_percent","101"}});Check(LastFmConfig.Load(folder).Percent==80,"invalid threshold defaults");
        }finally{Directory.Delete(folder,true);}
        Console.WriteLine("Last.fm: "+checks+" checks passed.");return 0;
    }
}

namespace ReRiff { internal static class NativeReelBridge {internal static void Log(string s){} } }
