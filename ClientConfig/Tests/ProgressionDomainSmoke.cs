using System;
using System.IO;
using System.Reflection;

class ProgressionDomainSmoke {
    static void Check(bool value) {
        if(!value)throw new Exception("Progression opt-in assertion failed");
    }
    static int Main() {
        string folder=Path.Combine(Path.GetTempPath(),"reriff-domain-"+Guid.NewGuid());
        Directory.CreateDirectory(folder);
        try {
            var type=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.ProgressionDomain");
            var enabled=type.GetMethod("Enabled",BindingFlags.NonPublic|BindingFlags.Static);
            var prepare=type.GetMethod("Prepare",BindingFlags.NonPublic|BindingFlags.Static);
            var ghostMode=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.ProgressionGhost").GetField("singleplayer",BindingFlags.NonPublic|BindingFlags.Static);
            Func<bool> read=()=> (bool)enabled.Invoke(null,new object[]{folder});
            Check(!read());
            string ini=Path.Combine(folder,"server.ini");
            File.WriteAllText(ini,"[graphics]\nisolate_singleplayer=1\n");Check(!read());
            File.WriteAllText(ini,"[progression]\ntrace_enabled=1\nisolate_singleplayer=0\n");Check(!read());
            File.WriteAllText(ini,"[PROGRESSION]\n; isolate_singleplayer=0\nisolate_singleplayer = 1\n");Check(read());
            // Multiplayer must return without calling the native export even when opted in.
            prepare.Invoke(null,new object[]{folder,"https://server.example"});
            Check(!(bool)ghostMode.GetValue(null));
            File.WriteAllText(ini,"[progression]\nisolate_singleplayer=0\n");
            string endpoint=ReRiff.Singleplayer.Start(folder);
            Check(ReRiff.Singleplayer.IsEndpoint(endpoint));
            try {
                prepare.Invoke(null,new object[]{folder,endpoint}); Check((bool)ghostMode.GetValue(null));
                prepare.Invoke(null,new object[]{folder,"http://127.0.0.1:5144"});Check(!(bool)ghostMode.GetValue(null));
                prepare.Invoke(null,new object[]{folder,endpoint});Check((bool)ghostMode.GetValue(null));
            }
            finally {
                ((IDisposable)typeof(ReRiff.Singleplayer).GetField("current",BindingFlags.NonPublic|BindingFlags.Static).GetValue(null)).Dispose();
            }
            prepare.Invoke(null,new object[]{folder,"https://server.example"});Check(!(bool)ghostMode.GetValue(null));
            Console.WriteLine("PASS: explicit isolation opt-in, disabled default, multiplayer bypass and ghost achievement mode for singleplayer without isolation");
            return 0;
        } finally { Directory.Delete(folder,true); }
    }
}
