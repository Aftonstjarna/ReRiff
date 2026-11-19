using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Web.Script.Serialization;
using ReRiff;
class ListenBrainzSmoke {
    static int checks;
    static void Check(bool condition,string message){checks++;if(!condition)throw new Exception(message);}
    static int Main(string[] args){
        const string token="12345678-1234-1234-1234-123456789abc";
        var config=new ListenBrainzConfig{Token=token,Enabled=true};
        Check(config.Ready&&config.Percent==80,"valid independent configuration");
        foreach(string invalid in new[]{"","bad",token+"\r\n","00000000-0000-0000-0000-000000000000"})Check(!ListenBrainzConfig.ValidToken(invalid),"invalid token refused");
        var track=new LastFmTrack{Artist="L'Arc~en~Ciel",Title="雪 & Light",Duration=200,Started=1700000000,Album="Album"};
        string json=ListenBrainzApi.Payload(track,false);var root=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(json);
        Check((string)root["listen_type"]=="single"&&json.Contains("1700000000")&&json.Contains("artist_name")&&json.Contains("release_name"),"listen payload");
        Check(!json.Contains(token)&&!json.Contains("session_key"),"no credentials in payload");
        json=ListenBrainzApi.Payload(track,true);Check(json.Contains("playing_now")&&!json.Contains("listened_at"),"playing now omits timestamp");
        var headers=new WebHeaderCollection();Check(ListenBrainzApi.Wait(headers)==1,"minimum spacing");
        headers["X-RateLimit-Remaining"]="0";headers["X-RateLimit-Reset-In"]="120";Check(ListenBrainzApi.Wait(headers)==120,"quota reset respected");
        headers["Retry-After"]="180";Check(ListenBrainzApi.Wait(headers)==180,"longer retry after respected");
        var playback=new LastFmPlayback(80);playback.Start(track,0,0);playback.Observe(0,0,true,false);
        for(int i=1;i<160;i++)Check(!playback.Observe(i,i,true,false),"not before threshold");
        Check(playback.Observe(160,160,true,false)&&!playback.Observe(161,161,true,true),"one listen at 80 percent");
        string folder=Path.Combine(Path.GetTempPath(),"reriff-lb-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(folder);
        try {
            File.WriteAllText(Path.Combine(folder,"client.ini"),"[listenbrainz]\nenabled=true\nuser_token="+token+"\nscrobble_percent=90\n[lastfm]\nenabled=false\n");
            var loaded=ListenBrainzConfig.Load(folder);Check(loaded.Ready&&loaded.Percent==90&&!LastFmConfig.Load(folder).Ready,"independent settings routed to client INI");
            string path=Path.Combine(folder,"listenbrainz-queue.json"),account=ListenBrainzService.Account(config);
            var box=new LastFmOutbox(path,account);box.Add(track);box.Add(track);box.Flush();Check(box.Count==1,"queue deduplication");
            Check(!File.ReadAllText(path).Contains(token),"queue does not store token");
            box=new LastFmOutbox(path,account);Check(box.Count==1,"offline queue recovered");
            Check(new LastFmOutbox(path,"different-token").Count==0,"another token cannot replay queue");
            var delivery=new ListenBrainzDelivery();DateTime now=new DateTime(2026,11,18);int calls=0;
            Func<ListenBrainzConfig,LastFmTrack,bool,int> limited=(c,t,p)=>{calls++;throw new ListenBrainzFailure(429,120);};
            delivery.Send(box,config,null,now,limited);Check(calls==1&&box.Count==1&&delivery.Next==now.AddSeconds(120),"429 retains listen and honors wait");
            delivery.Send(box,config,null,now.AddSeconds(119),limited);Check(calls==1,"no premature retry");
            delivery.Send(box,config,null,now.AddSeconds(120),(c,t,p)=>{calls++;Check(!p&&t.Started==track.Started,"retry retains original start time");return 1;});
            Check(box.Count==0&&new LastFmOutbox(path,account).Count==0,"acknowledged listen removed durably");
            box.Add(track);delivery=new ListenBrainzDelivery();delivery.Send(box,config,track,now,(c,t,p)=>{Check(p,"now playing request");return 30;});
            Check(box.Count==1&&delivery.Next==now.AddSeconds(30),"now playing neither consumes queue nor ignores quota");
            delivery=new ListenBrainzDelivery();delivery.Send(box,config,null,now,(c,t,p)=>{throw new ListenBrainzFailure(401);});
            Check(delivery.Stopped&&box.Count==1,"invalid token stops without losing pending history");
            delivery=new ListenBrainzDelivery();delivery.Send(box,config,null,now,(c,t,p)=>{throw new ListenBrainzFailure(400);});Check(box.Count==0,"bad metadata does not block following listens");
            box.Add(track);delivery=new ListenBrainzDelivery();delivery.Send(box,config,null,now,(c,t,p)=>{throw new ListenBrainzFailure(503);});Check(!delivery.Stopped&&box.Count==1&&delivery.Next>now,"server failure backs off");
            if(args.Length==1){var protectedConfig=ListenBrainzConfig.Load(args[0]);Check(protectedConfig.Ready&&protectedConfig.Token==token,"launcher DPAPI token readable by client");}
        }finally{Directory.Delete(folder,true);}
        Console.WriteLine("PASS: ListenBrainz {0} payload, policy, queue, retry and configuration checks (no network submissions)",checks);return 0;
    }
}
namespace ReRiff {internal static class NativeReelBridge {internal static void Log(string message){}}}
