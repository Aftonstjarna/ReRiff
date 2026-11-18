using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace ReRiff {
    internal static class LastFmNative {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Observer(int kind,int state);
        private delegate float Duration(IntPtr audio);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigureScrobbler(Observer observer);
        private static readonly Observer callback=Observe;
        private static Func<IntPtr> audio,race,config,save;
        private static Duration duration;
        private static LastFmService service;
        private static LastFmPlayback playback;
        private static readonly Stopwatch clock=Stopwatch.StartNew();
        private static string song="";private static double previousPosition,previousRace;
        private static bool reset=true,reportedFailure;private static int percent;
        internal static void Configure(Module game){
            string folder=Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location);var settings=LastFmConfig.Load(folder);
            if(!settings.Ready){NativeReelBridge.Log("Last.fm: disabled or account not linked.");return;}
            if(IntPtr.Size!=4)throw new PlatformNotSupportedException();
            audio=Singleton(game,"?ms_pInstance@gaAudioManager@@0PAV1@A");race=Singleton(game,"?ms_pInstance@gaRaceManager@@0PAV1@A");
            config=Singleton(game,"?ms_pInstance@gaRaceConfig@@0PAV1@A");save=Singleton(game,"?ms_instance@gaSaveGame@@0PAV1@A");
            var method=game.GetMethods(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic).Single(m=>m.Name=="gaAudioManager.getMusicFileDuration");
            duration=(Duration)NativeReelBridge.CreateCall(method,typeof(Duration),false);percent=settings.Percent;
            if(ReRiffConfigureScrobbler(callback)!=1)throw new IOException("Scrobbler registration failed");
            service=new LastFmService(folder,settings);NativeReelBridge.Log("Last.fm: playback observer configured.");
        }
        private static Func<IntPtr> Singleton(Module game,string name){var field=game.GetFields(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic).Single(f=>f.Name==name);return ProgressionNative.Singleton(game,field.MetadataToken,name);}
        private static IntPtr Pointer(byte[] data,int at){return new IntPtr(BitConverter.ToInt32(data,at));}
        private static string Text(IntPtr address){
            byte[] b=ProgressionNative.Read(address,24);int size=BitConverter.ToInt32(b,16),capacity=BitConverter.ToInt32(b,20);
            if(size<0||size>4096||capacity<size)throw new IOException();
            return size==0?"":new UTF8Encoding(false,true).GetString(ProgressionNative.Read(capacity<16?address:Pointer(b,0),size));
        }
        private static LastFmTrack Metadata(string id,double length){
            IntPtr root=save();if(root==IntPtr.Zero)return null;
            byte[] vector=ProgressionNative.Read(IntPtr.Add(root,176),12);uint first=BitConverter.ToUInt32(vector,0),last=BitConverter.ToUInt32(vector,4),end=BitConverter.ToUInt32(vector,8);
            if(first==0||last<first||end<last||(last-first)%200!=0||(last-first)/200>100000)return null;
            for(uint p=first;p<last;p+=200){
                IntPtr entry=new IntPtr(unchecked((int)p));if(Text(IntPtr.Add(entry,24))!=id)continue;
                var track=new LastFmTrack{Title=Text(IntPtr.Add(entry,112)),Artist=Text(IntPtr.Add(entry,136)),Duration=length,Started=DateTimeOffset.UtcNow.ToUnixSecondsCompat()};
                return track.Valid?track:null;
            }
            return null;
        }
        // Called from the original race tick/entry/results thread, never a worker.
        private static void Observe(int kind,int state){
            try{
                if(service==null||!service.Ready){reset=true;return;}
                // Original enter explicitly restarts for states 45/50; other entries
                // include pause/resume and must retain listening progress.
                if(kind==1){if(state==45||state==50)reset=true;return;}
                if(kind==2){if(playback!=null&&!reset&&playback.Observe(previousPosition,clock.Elapsed.TotalSeconds,false,true))service.Submit(playback.Track);return;}
                IntPtr manager=race(),sound=audio(),settings=config();if(manager==IntPtr.Zero||sound==IntPtr.Zero||settings==IntPtr.Zero)return;
                byte[] r=ProgressionNative.Read(manager,34),a=ProgressionNative.Read(sound,165);
                double position=BitConverter.ToSingle(a,160),raceTime=BitConverter.ToSingle(r,24),now=clock.Elapsed.TotalSeconds;
                if(Double.IsNaN(position)||Double.IsInfinity(position)||position<0)return;
                bool active=r[33]!=0&&r[12]==0&&a[109]==0&&a[164]==0&&BitConverter.ToInt32(a,0)!=0;
                if(!active){if(r[33]==0)reset=true;if(playback!=null)playback.Observe(position,now,false,false);return;}
                string id=Text(IntPtr.Add(settings,216));if(id.Length==0)return;
                if(reset||id!=song||position+1<previousPosition||raceTime+1<previousRace){
                    song=id;reset=false;playback=new LastFmPlayback(percent);LastFmTrack track=Metadata(id,duration(sound));
                    playback.Start(track,position,now);if(track!=null)service.Playing(track);
                    NativeReelBridge.Log(track==null?"Last.fm: track skipped (missing usable tagged metadata or duration).":"Last.fm: race playback started.");
                }
                previousPosition=position;previousRace=raceTime;
                if(playback.Observe(position,now,true,false))service.Submit(playback.Track);
            }catch{reset=true;if(!reportedFailure){reportedFailure=true;NativeReelBridge.Log("Last.fm: playback snapshot unavailable; skipping until a valid snapshot is available.");} /* Never affect game control flow. */}
        }
    }
}
