using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;

namespace ReRiff {
    // Share durable queue storage/deduplication with Last.fm, but keep accounts,
    // requests and retry state independent so either provider can run alone.
    internal sealed class ListenBrainzDelivery {
        internal DateTime Next=DateTime.MinValue;
        internal bool Stopped;
        private int delay=15;
        internal string Send(LastFmOutbox box,ListenBrainzConfig config,LastFmTrack playing,DateTime now,Func<ListenBrainzConfig,LastFmTrack,bool,int> submit) {
            box.Flush();if(Stopped||now<Next)return null;
            var track=playing??box.First;if(track==null)return null;
            try {
                int wait=submit(config,track,playing!=null);Next=now.AddSeconds(Math.Max(1,wait));delay=15;
                if(playing==null){box.RemoveFirst();return "Listen accepted.";}
                return null;
            }catch(ListenBrainzFailure error) {
                if(error.Retry){Next=now.AddSeconds(Math.Max(delay,error.WaitSeconds));delay=Math.Min(delay*2,900);}
                else if(error.Status==400&&playing==null){box.RemoveFirst();Next=now.AddSeconds(Math.Max(1,error.WaitSeconds));}
                else if(error.Status==401||error.Status==403||error.Status!=400)Stopped=true;
                else Next=now.AddSeconds(Math.Max(1,error.WaitSeconds));
                return "Request failed (HTTP "+error.Status+")."+(Stopped?" Check the token and restart the game.":"");
            }
        }
    }
    internal sealed class ListenBrainzService {
        private readonly string folder,account;
        private readonly object sync=new object();
        private readonly Queue<LastFmTrack> incoming=new Queue<LastFmTrack>();
        private LastFmTrack nowPlaying;
        internal volatile bool Ready;
        internal ListenBrainzService(string folder,ListenBrainzConfig config) {
            this.folder=folder;account=Account(config);Ready=config.Ready;
            new Thread(Run){IsBackground=true,Name="ReRiff ListenBrainz"}.Start();
        }
        internal static string Account(ListenBrainzConfig config){return LastFmOutbox.Account(new LastFmConfig{Key="listenbrainz",Session=config.Token});}
        internal void Submit(LastFmTrack track){lock(sync){if(incoming.Count<100)incoming.Enqueue(track);}}
        internal void Playing(LastFmTrack track){lock(sync){nowPlaying=track;}}
        private static void Log(string message){NativeReelBridge.Log("ListenBrainz: "+message);}
        private void Run() {
            LastFmOutbox box;
            try{box=new LastFmOutbox(Path.Combine(folder,"listenbrainz-queue.json"),account);}catch{Ready=false;Log("Cannot open the local queue. Check permissions and restart the game.");return;}
            var delivery=new ListenBrainzDelivery();
            while(true) {
                try {
                    var config=ListenBrainzConfig.Load(folder);Ready=config.Ready&&Account(config)==account&&!delivery.Stopped;
                    if(!Ready){lock(sync){incoming.Clear();nowPlaying=null;}Thread.Sleep(1000);continue;}
                    LastFmTrack playing=null;
                    lock(sync){
                        while(incoming.Count>0)box.Add(incoming.Dequeue());
                        if(DateTime.UtcNow>=delivery.Next){playing=nowPlaying;nowPlaying=null;}
                    }
                    if(playing!=null&&DateTimeOffset.UtcNow.ToUnixSecondsCompat()-playing.Started>=30)playing=null;
                    var started=DateTime.UtcNow;bool due=started>=delivery.Next;
                    var elapsed=System.Diagnostics.Stopwatch.StartNew();
                    string message=delivery.Send(box,config,playing,started,ListenBrainzApi.Submit);
                    // Rate-limit headers describe the response time, not the start
                    // of a potentially slow network request.
                    if(due&&delivery.Next>started)delivery.Next=delivery.Next.Add(elapsed.Elapsed);
                    if(message!=null)Log(message);
                }catch{Log("Local submission failed; retrying later.");Thread.Sleep(5000);}
                Thread.Sleep(1000);
            }
        }
    }
}
