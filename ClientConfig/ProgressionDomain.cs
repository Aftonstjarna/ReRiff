using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ReRiff {
    internal static class ProgressionDomain {
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffPrepareProgressionDomain(int singleplayer);
        [DllImport("ReRiff.Notify.dll", CallingConvention=CallingConvention.Cdecl, ExactSpelling=true)]
        private static extern int ReRiffConfigureProgressionCheckpoint(IntPtr callback);
        [DllImport("user32.dll", CharSet=CharSet.Unicode, ExactSpelling=true)]
        private static extern int MessageBoxW(IntPtr window,string message,string title,uint flags);

        internal static bool Enabled(string folder) {
            string value;return ConfigFiles.Read(folder,"progression").TryGetValue("isolate_singleplayer",out value)&&value=="1";
        }

        internal static void Prepare(string folder,string endpoint) {
            bool singleplayer=Singleplayer.IsEndpoint(endpoint);
            if(singleplayer){try{ProgressionBalance.Load(folder);}catch(Exception error){ProgressionBalance.Current=new ProgressionBalance();NativeReelBridge.Log("Singleplayer balance rejected; default policy retained: "+error.Message);}}
            ProgressionGhost.SetSingleplayer(singleplayer);
            if(!singleplayer)return;
            if(!Enabled(folder))return;
            try {
                if(ReRiffConfigureProgressionCheckpoint(ProgressionCheckpoints.CallbackPointer)!=1)throw new InvalidOperationException("Native checkpoint registration failed.");
                if(ReRiffPrepareProgressionDomain(1)==1)return;
                throw new InvalidOperationException("The original native save/account guards did not pass.");
            } catch(Exception error) {
                Logging.Append(folder,"ReRiff-engine-menu.log",DateTime.UtcNow.ToString("o")+" Progression domain preparation failed: "+error+Environment.NewLine);
                MessageBoxW(IntPtr.Zero,"The isolated singleplayer save could not be prepared. The game will close to avoid using the shared original save.\n\nYour original save has not been reset. See logs/ReRiff-engine-menu.log when logging is enabled.","ReRiff experimental progression",0x10);
                Environment.Exit(1);
            }
        }
    }
}
