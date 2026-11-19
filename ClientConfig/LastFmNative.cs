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
        private static LastFmService service,libreService;
        private static LastFmPlayback librePlayback;
        private static int librePercent;
        private static LastFmPlayback playback,brainPlayback;
        private static ListenBrainzService brainService;
        private static int brainPercent;
        private static readonly Stopwatch clock=Stopwatch.StartNew();
        private static string song="";private static double previousPosition,previousRace;
        private static bool reset=true,reportedFailure;private static int percent;
        internal static void Configure(Module game){
            string folder=Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location);var settings=LastFmConfig.Load(folder);var brain=ListenBrainzConfig.Load(folder);var libre=LastFmConfig.Load(folder,true);
            if(!settings.Ready&&!brain.Ready&&!libre.Ready){NativeReelBridge.Log("Scrobbling: no enabled provider configured.");return;}
            if(IntPtr.Size!=4)throw new PlatformNotSupportedException();
            audio=Singleton(game,"?ms_pInstance@gaAudioManager@@0PAV1@A");race=Singleton(game,"?ms_pInstance@gaRaceManager@@0PAV1@A");
            config=Singleton(game,"?ms_pInstance@gaRaceConfig@@0PAV1@A");save=Singleton(game,"?ms_instance@gaSaveGame@@0PAV1@A");
            var method=game.GetMethods(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic).Single(m=>m.Name=="gaAudioManager.getMusicFileDuration");
            duration=(Duration)NativeReelBridge.CreateCall(method,typeof(Duration),false);percent=settings.Percent;brainPercent=brain.Percent;librePercent=libre.Percent;
            if(ReRiffConfigureScrobbler(callback)!=1)throw new IOException("Scrobbler registration failed");
            if(libre.Ready)libreService=new LastFmService(folder,libre);
            if(settings.Ready)service=new LastFmService(folder,settings);
            if(brain.Ready)brainService=new ListenBrainzService(folder,brain);
            NativeReelBridge.Log("Scrobbling: shared playback observer configured.");
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
        private static void Feed(double position,double now,bool active,bool completed) {
            if(playback!=null&&service!=null&&playback.Observe(position,now,active&&service.Ready,completed&&service.Ready)&&service.Ready)service.Submit(playback.Track);
            if(librePlayback!=null&&libreService!=null&&librePlayback.Observe(position,now,active&&libreService.Ready,completed&&libreService.Ready)&&libreService.Ready)libreService.Submit(librePlayback.Track);
            if(brainPlayback!=null&&brainService!=null&&brainPlayback.Observe(position,now,active&&brainService.Ready,completed&&brainService.Ready)&&brainService.Ready)brainService.Submit(brainPlayback.Track);
        }
        // Called from the original race tick/entry/results thread, never a worker.
        private static void Observe(int kind,int state){
            try{
                if((service==null||!service.Ready)&&(brainService==null||!brainService.Ready)&&(libreService==null||!libreService.Ready)){reset=true;return;}
                // Original enter explicitly restarts for states 45/50; other entries
                // include pause/resume and must retain listening progress.
                if(kind==1){if(state==45||state==50)reset=true;return;}
                if(kind==2){if(!reset)Feed(previousPosition,clock.Elapsed.TotalSeconds,false,true);return;}
                IntPtr manager=race(),sound=audio(),settings=config();if(manager==IntPtr.Zero||sound==IntPtr.Zero||settings==IntPtr.Zero)return;
                byte[] r=ProgressionNative.Read(manager,34),a=ProgressionNative.Read(sound,165);
                double position=BitConverter.ToSingle(a,160),raceTime=BitConverter.ToSingle(r,24),now=clock.Elapsed.TotalSeconds;
                if(Double.IsNaN(position)||Double.IsInfinity(position)||position<0)return;
                bool active=r[33]!=0&&r[12]==0&&a[109]==0&&a[164]==0&&BitConverter.ToInt32(a,0)!=0;
                if(!active){if(r[33]==0)reset=true;Feed(position,now,false,false);return;}
                string id=Text(IntPtr.Add(settings,216));if(id.Length==0)return;
                if(reset||id!=song||position+1<previousPosition||raceTime+1<previousRace){
                    song=id;reset=false;playback=new LastFmPlayback(percent);brainPlayback=new LastFmPlayback(brainPercent);librePlayback=new LastFmPlayback(librePercent);LastFmTrack track=Metadata(id,duration(sound));
                    playback.Start(track,position,now);brainPlayback.Start(track,position,now);librePlayback.Start(track,position,now);
                    if(track!=null){if(libreService!=null&&libreService.Ready)libreService.Playing(track);if(service!=null&&service.Ready)service.Playing(track);if(brainService!=null&&brainService.Ready)brainService.Playing(track);}
                    NativeReelBridge.Log(track==null?"Scrobbling: track skipped (missing usable tagged metadata or duration).":"Scrobbling: race playback started.");
                }
                previousPosition=position;previousRace=raceTime;
                Feed(position,now,true,false);
            }catch{reset=true;if(!reportedFailure){reportedFailure=true;NativeReelBridge.Log("Scrobbling: playback snapshot unavailable; skipping until a valid snapshot is available.");} /* Never affect game control flow. */}
        }
    }
}
