using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;

namespace ReRiff {
    internal sealed class LastFmQueue {
        public string Account {get;set;}
        public List<LastFmTrack> Tracks {get;set;}
    }
    internal sealed class LastFmOutbox {
        private readonly string path;private readonly LastFmQueue queue;
        private int delay=15;private bool dirty;
        internal DateTime NextAttempt=DateTime.MinValue;
        internal bool Stopped;
        internal int Count {get{return queue.Tracks.Count;}}
        internal LastFmTrack First {get{return Count==0?null:queue.Tracks[0];}}
        internal void RemoveFirst(){queue.Tracks.RemoveAt(0);dirty=true;Flush();}
        internal LastFmOutbox(string path,string account){
            this.path=path;queue=new LastFmQueue{Account=account,Tracks=new List<LastFmTrack>()};
            if(File.Exists(path)&&new FileInfo(path).Length<1024*1024){
                try{
                    var saved=new JavaScriptSerializer().Deserialize<LastFmQueue>(File.ReadAllText(path));
                    if(saved!=null&&saved.Account==account&&saved.Tracks!=null)queue.Tracks=saved.Tracks.Where(t=>t!=null&&t.Valid&&t.Started>0).Take(500).ToList();
                }catch{ /* Malformed local history cannot break gameplay. */ }
            }
        }
        internal static string Account(LastFmConfig config){using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(config.Key+"\n"+config.Session))).Replace("-","");}
        internal void Add(LastFmTrack track){
            if(!track.Valid||queue.Tracks.Any(t=>t.Started==track.Started&&t.Artist==track.Artist&&t.Title==track.Title))return;
            if(queue.Tracks.Count==500)queue.Tracks.RemoveAt(0);queue.Tracks.Add(track);dirty=true;
        }
        internal void Flush(){if(dirty){ConfigFiles.Atomic(path,new JavaScriptSerializer().Serialize(queue));dirty=false;}}
        internal string Deliver(LastFmConfig config,DateTime now,Func<LastFmConfig,string,IDictionary<string,string>,Dictionary<string,object>> call){
            // Persist before sending. If the process exits while offline, the same
            // account can retry with the original playback timestamp next start.
            Flush();if(Stopped||queue.Tracks.Count==0||now<NextAttempt)return null;
            try{
                var result=call(config,"track.scrobble",queue.Tracks[0].Parameters(config.Session,true));
                var summary=LastFmApi.Object(LastFmApi.Object(result,"scrobbles"),"@attr");
                string accepted=LastFmApi.Text(summary,"accepted"),ignored=LastFmApi.Text(summary,"ignored");
                if(accepted!="1"&&ignored!="1")throw new LastFmFailure(0);
                queue.Tracks.RemoveAt(0);dirty=true;Flush();delay=15;NextAttempt=now.AddSeconds(2);
                return accepted=="1"?"Scrobble accepted.":config.Libre?"Track ignored by Libre.fm.":"Track ignored by Last.fm.";
            }catch(LastFmFailure error){
                if(error.Retry){NextAttempt=now.AddSeconds(delay);delay=Math.Min(delay*2,900);}
                else if(error.Code==6){queue.Tracks.RemoveAt(0);dirty=true;Flush();NextAttempt=now.AddSeconds(2);}
                else Stopped=true;
                return "Request failed (code "+error.Code+")."+(Stopped?" Check account linking and credentials, then restart the game.":"");
            }
        }
    }
    internal sealed class LastFmService {
        private readonly string folder,account;
        private readonly bool libre;
        private readonly object sync=new object();
        private readonly Queue<LastFmTrack> incoming=new Queue<LastFmTrack>();
        private LastFmTrack nowPlaying;
        internal volatile bool Ready;
        internal LastFmService(string folder,LastFmConfig config){
            this.folder=folder;libre=config.Libre;account=LastFmOutbox.Account(config);Ready=config.Ready;
            new Thread(Run){IsBackground=true,Name=libre?"ReRiff Libre.fm":"ReRiff Last.fm"}.Start();
        }
        internal void Submit(LastFmTrack track){lock(sync){if(incoming.Count<100)incoming.Enqueue(track);}}
        internal void Playing(LastFmTrack track){lock(sync){nowPlaying=track;}}
        private void Log(string message){NativeReelBridge.Log("Last.fm: "+message);}
        private void Run(){
            LastFmOutbox box;try{box=new LastFmOutbox(Path.Combine(folder,libre?"librefm-queue.json":"lastfm-queue.json"),account);}catch{Ready=false;Log("Could not open the local queue. Restart the game after checking file permissions.");return;}
            DateTime nextPlaying=DateTime.MinValue;
            while(true){
                try{
                    var config=LastFmConfig.Load(folder,libre);Ready=config.Ready&&LastFmOutbox.Account(config)==account&&!box.Stopped;
                    if(!Ready){lock(sync){incoming.Clear();nowPlaying=null;}Thread.Sleep(1000);continue;}
                    LastFmTrack playing;
                    lock(sync){while(incoming.Count>0)box.Add(incoming.Dequeue());playing=nowPlaying;nowPlaying=null;}
                    box.Flush();
                    if(playing!=null&&playing.Valid&&DateTime.UtcNow>=nextPlaying&&DateTimeOffset.UtcNow.ToUnixSecondsCompat()-playing.Started<30){
                        try{LastFmApi.Call(config,"track.updateNowPlaying",playing.Parameters(config.Session,false));}
                        catch(LastFmFailure error){nextPlaying=DateTime.UtcNow.AddMinutes(1);Log("Now Playing request failed (code "+error.Code+").");}
                    }
                    string status=box.Deliver(config,DateTime.UtcNow,LastFmApi.Call);if(status!=null)Log(status);
                }catch{Log("Local scrobbling operation failed; retrying later.");Thread.Sleep(5000);}
                Thread.Sleep(1000);
            }
        }
    }
    internal static class LastFmClock {
        internal static long ToUnixSecondsCompat(this DateTimeOffset value){return (long)(value.UtcDateTime-new DateTime(1970,1,1,0,0,0,DateTimeKind.Utc)).TotalSeconds;}
    }
}
