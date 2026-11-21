using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ReRiff {
    internal static class ProgressionDomain {
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffPrepareProgressionDomain(int singleplayer);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionCheckpoint(IntPtr callback);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true, CharSet=CharSet.Ansi)]
        private static extern int ReRiffPrepareMultiplayerProgression(string serverId,string accountId,int importOriginal);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern void ReRiffRefreshDiagnosticIdentity();
        [DllImport("user32.dll", CharSet=CharSet.Unicode, ExactSpelling=true)]
        private static extern int MessageBoxW(IntPtr window,string message,string title,uint flags);

        internal static bool Enabled(string folder) {
            string value;return ConfigFiles.Read(folder,"progression").TryGetValue("isolate_singleplayer",out value)&&value=="1";
        }

        internal static void Prepare(string folder,string endpoint) {
            bool singleplayer=Singleplayer.IsEndpoint(endpoint);
            if(singleplayer){try{ProgressionBalance.Load(folder);}catch(Exception error){ProgressionBalance.Current=new ProgressionBalance();NativeReelBridge.Log("Singleplayer balance rejected; default policy retained: "+error.Message);}}
            ProgressionGhost.SetSingleplayer(singleplayer);
            if(singleplayer&&!Enabled(folder))return;
            if(!singleplayer&&!ProgressionRemote.Enabled(folder))return;
            try {
                if(singleplayer){
                    if(ReRiffConfigureProgressionCheckpoint(ProgressionCheckpoints.CallbackPointer)!=1)throw new InvalidOperationException("Native checkpoint registration failed.");
                    if(ReRiffPrepareProgressionDomain(1)==1)return;
                }
                else {
                    var remote=ProgressionRemote.Connect(folder,endpoint,()=>ProgressionNative.Bound.ObserveCheckpoint(),ProgressionRemote.PromptPassword);
                    if(remote==null)return;
                    Logging.SetAccountIdentity(folder,remote.ServerId,remote.AccountId);
                    try{ReRiffRefreshDiagnosticIdentity();}catch(DllNotFoundException){}catch(EntryPointNotFoundException){}

                    if(ReRiffConfigureProgressionCheckpoint(ProgressionCheckpoints.CallbackPointer)!=1)throw new InvalidOperationException("Native checkpoint registration failed.");
                    ProgressionCheckpoints.Remote=remote;
                    ProgressionCheckpoints.SaveInitial=()=>ProgressionNative.Bound.SaveCurrent();
                    if(ReRiffPrepareMultiplayerProgression(remote.ServerId,remote.AccountId,remote.ImportOriginal?1:0)==1)return;
                }
                throw new InvalidOperationException("The original native save/account guards did not pass.");
            } catch(OperationCanceledException) {
                Environment.Exit(0);
            } catch(Exception error) {
                Logging.Append(folder,"ReRiff-engine-menu.log","Progression domain preparation failed: "+error+Environment.NewLine);
                NativeReelBridge.ShowError("ReRiff experimental progression","The isolated progression save could not be prepared. The game will close to avoid using the shared original save.\n\nYour original save has not been reset.\n\n"+error.Message+"\n\nSee logs/ReRiff-game.log when logging is enabled.");
                Environment.Exit(1);
            }
        }
    }
}
