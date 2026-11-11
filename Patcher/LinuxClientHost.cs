#if NET8_0_OR_GREATER
#nullable enable
using System.Diagnostics;
using System.Formats.Tar;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;

// Linux installation and update orchestration share the existing self-contained
// patcher's runtime. Python is used only by the optional server AppImage.
[System.Runtime.Versioning.SupportedOSPlatform("linux")]
internal static class LinuxClientHost {
    static string Self => Environment.ProcessPath ?? throw new IOException("Client helper path unavailable.");
    static string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();
    static string HashFile(string path) { using var file=File.OpenRead(path);return Convert.ToHexString(SHA256.HashData(file)).ToLowerInvariant(); }
    static JsonObject ReadJson(string path) => JsonNode.Parse(File.ReadAllText(path))?.AsObject() ?? throw new IOException("Invalid JSON: "+path);
    static string Text(JsonObject obj,string key,string fallback="") => obj[key]?.GetValue<string>() ?? fallback;
    static bool Flag(JsonObject obj,string key,bool fallback=false) => obj[key]?.GetValue<bool>() ?? fallback;
    static JsonObject Map(JsonObject obj,string key) => obj[key]?.AsObject() ?? new JsonObject();
    static string Under(string root,string relative) {
        if(string.IsNullOrWhiteSpace(relative)||Path.IsPathRooted(relative)||relative.Contains('\\')||relative.Contains(':')||relative.Split('/').Any(p=>p==".."))throw new IOException("Unsafe package path.");
        string full=Path.GetFullPath(Path.Combine(root,relative));
        if(!full.StartsWith(Path.GetFullPath(root).TrimEnd('/')+"/",StringComparison.Ordinal))throw new IOException("Package path escapes its root.");
        for(string? p=full;p!=null&&p!=Path.GetFullPath(root);p=Path.GetDirectoryName(p))
            if((File.Exists(p)||Directory.Exists(p))&&(File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Refusing package symlink: "+p);
        return full;
    }
    static void Atomic(string path,byte[] bytes,bool executable=false) {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);string temp=path+".reriff-"+Guid.NewGuid().ToString("N");
        try {File.WriteAllBytes(temp,bytes);File.SetUnixFileMode(temp,File.Exists(path)?File.GetUnixFileMode(path):UnixFileMode.UserRead|UnixFileMode.UserWrite);if(executable)File.SetUnixFileMode(temp,File.GetUnixFileMode(temp)|UnixFileMode.UserExecute|UnixFileMode.GroupExecute|UnixFileMode.OtherExecute);File.Move(temp,path,true);}
        finally {if(File.Exists(temp))File.Delete(temp);}
    }
    static void AtomicJson(string path,JsonObject value) => Atomic(path,Encoding.UTF8.GetBytes(value.ToJsonString()+"\n"));
    static string Temp(string parent) {Directory.CreateDirectory(parent);string path=Path.Combine(parent,".reriff-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(path);File.SetUnixFileMode(path,UnixFileMode.UserRead|UnixFileMode.UserWrite|UnixFileMode.UserExecute);return path;}
    static int RunProcess(string file,IEnumerable<string> arguments,int seconds=120,bool terminateOnTimeout=true) {
        var info=new ProcessStartInfo(file){UseShellExecute=false};foreach(string arg in arguments)info.ArgumentList.Add(arg);
        using var process=Process.Start(info)??throw new IOException("Could not run "+file);
        if(!process.WaitForExit(seconds*1000)){if(terminateOnTimeout){process.Kill(entireProcessTree:true);process.WaitForExit();}throw new IOException("Helper timed out; inspect logs before retrying.");}return process.ExitCode;
    }
    sealed class Options {
        public readonly Dictionary<string,string> Values=new();public readonly HashSet<string> Flags=new();
        public Options(string[] args) {
            var switches=new HashSet<string>{"--dry-run","--restore","--no-music-link","--prepare","--cli","--help"};
            var values=new HashSet<string>{"--bundle","--game-dir","--backup-dir","--music-dir","--root","--result","--asset","--sha256","--work","--archive","--channel","--platform"};
            for(int i=0;i<args.Length;i++) {string key=args[i];if(switches.Contains(key)){if(!Flags.Add(key))throw new ArgumentException("Duplicate option: "+key);}else if(values.Contains(key)&&i+1<args.Length){if(!Values.TryAdd(key,args[++i]))throw new ArgumentException("Duplicate option: "+key);}else throw new ArgumentException("Unknown or incomplete option: "+key);}
        }
        public string Get(string key,string fallback="")=>Values.GetValueOrDefault(key,fallback);
        public string Need(string key)=>Values.TryGetValue(key,out var v)?Path.GetFullPath(v):throw new ArgumentException("Required option: "+key);
        public bool Has(string key)=>Flags.Contains(key);
    }
    internal static int Run(string[] args) {
        if(!OperatingSystem.IsLinux())throw new PlatformNotSupportedException("This client helper is for Linux.");
        Console.OutputEncoding=new UTF8Encoding(false);
        try {
            if(args.Length==0)throw new ArgumentException("Use install, update, server-control, stop-servers, print-game-dir or print-music-dir.");
            string command=args[0];int offset=command=="server-control"?2:1;var options=new Options(args.Skip(offset).ToArray());
            switch(command) {
                case "print-game-dir":Console.WriteLine(AutoGame());break;
                case "print-music-dir":Console.WriteLine(MusicDir());break;
                case "install":
                    if(options.Has("--help")){Console.WriteLine("ReRiff Linux setup: --game-dir PATH [--dry-run] [--restore] [--backup-dir PATH] [--music-dir PATH | --no-music-link]. No Python or system .NET required.");break;}
                    Install(options.Need("--bundle"),options.Get("--game-dir",AutoGame()),options);break;
                case "update":Update(options);break;
                case "server-control":if(args.Length<2)throw new ArgumentException("Server action required.");return Server(args[1],options);
                case "stop-servers":
                    try {StopServers(options.Need("--root"));if(options.Get("--result")!="")Atomic(options.Need("--result"),Encoding.UTF8.GetBytes("ok\n"));}
                    catch(Exception e){if(options.Get("--result")!="")Atomic(options.Need("--result"),Encoding.UTF8.GetBytes("error: "+e.Message+"\n"));throw;}break;
                default:throw new ArgumentException("Unknown client command: "+command);
            }
            return 0;
        } catch(Exception error){Console.Error.WriteLine(error.GetType().Name+": "+error.Message);return 1;}
    }
    static string AutoGame() {
        string home=Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        var libraries=new List<string>{home+"/.local/share/Steam",home+"/.steam/steam",home+"/.steam/root",home+"/.var/app/com.valvesoftware.Steam/.local/share/Steam"};
        foreach(string root in libraries.ToArray()) {string vdf=root+"/steamapps/libraryfolders.vdf";if(File.Exists(vdf))foreach(Match m in Regex.Matches(File.ReadAllText(vdf),"\"path\"\\s*\"([^\"]+)\""))libraries.Add(m.Groups[1].Value.Replace("\\\\","\\"));}
        return libraries.Select(p=>p+"/steamapps/common/Riff Racer").FirstOrDefault(p=>File.Exists(p+"/Game.exe"))??"";
    }
    static string MusicDir() {
        string home=Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),config=Environment.GetEnvironmentVariable("XDG_CONFIG_HOME")??home+"/.config";
        string file=config+"/user-dirs.dirs";
        if(File.Exists(file))foreach(string line in File.ReadLines(file)) {
            var match=Regex.Match(line,"^\\s*XDG_MUSIC_DIR\\s*=\\s*\"([^\"]*)\"\\s*(?:#.*)?$");
            if(match.Success) {string value=match.Groups[1].Value.Replace("${HOME}",home).Replace("$HOME",home);if(Path.IsPathRooted(value)&&!value.Contains('$')&&!value.Contains('`'))return Path.GetFullPath(value)==home?home+"/Music":value;}
        }
        return home+"/Music";
    }
    [DllImport("libc",SetLastError=true)]static extern IntPtr realpath(string path,IntPtr resolved);
    [DllImport("libc")]static extern void free(IntPtr pointer);
    static string RealPath(string path) {
        path=Path.GetFullPath(path);IntPtr pointer=realpath(path,IntPtr.Zero);
        if(pointer!=IntPtr.Zero)try{return Marshal.PtrToStringUTF8(pointer)!;}finally{free(pointer);}
        string? parent=Path.GetDirectoryName(path);return parent==null||parent==path?path:Path.Combine(RealPath(parent),Path.GetFileName(path));
    }
    [DllImport("libc",EntryPoint="rename",SetLastError=true)]static extern int RenameLink(string source,string destination);
    static void LinkMusic(string game,string source) {
        var candidates=new List<string>{game+"/prefix/pfx",game+"/prefix"};var folder=new DirectoryInfo(game);
        if(folder.Parent?.Name=="common"&&folder.Parent.Parent?.Name=="steamapps")candidates.Add(folder.Parent.Parent.FullName+"/compatdata/351990/pfx");
        string? prefix=candidates.FirstOrDefault(p=>Directory.Exists(p+"/drive_c/users/steamuser"));
        if(prefix==null){Console.WriteLine("Music link deferred: start the game once to initialize its Proton prefix.");return;}
        string destination=Path.GetFullPath(RealPath(prefix)+"/drive_c/users/steamuser/Music");source=RealPath(source);
        if(source==destination||source.StartsWith(destination+"/",StringComparison.Ordinal)){Console.WriteLine("Music link skipped: source must be outside prefix Music.");return;}
        var entry=new DirectoryInfo(destination);string? link=entry.LinkTarget;
        if(link!=null&&RealPath(Path.GetFullPath(link,entry.Parent!.FullName))==source){Console.WriteLine("Music link already correct.");return;}
        if(link==null&&(File.Exists(destination)||(entry.Exists&&entry.EnumerateFileSystemInfos().Any()))){Console.WriteLine("Existing prefix Music was preserved.");return;}
        Directory.CreateDirectory(source);string temporary=destination+".reriff-"+Guid.NewGuid().ToString("N");
        bool removed=false;
        try {
            Directory.CreateSymbolicLink(temporary,source);
            if(link==null&&entry.Exists){Directory.Delete(destination);removed=true;}
            if(RenameLink(temporary,destination)!=0){if(removed&&!Directory.Exists(destination)&&!File.Exists(destination))Directory.CreateDirectory(destination);throw new IOException("Could not replace Music link: "+new System.ComponentModel.Win32Exception(Marshal.GetLastPInvokeError()).Message);}
        }
        finally {if(new DirectoryInfo(temporary).LinkTarget!=null)File.Delete(temporary);}
        Console.WriteLine("Music linked: "+destination);
    }
    static void Install(string bundle,string game,Options options) {
        bundle=Path.GetFullPath(bundle);if(string.IsNullOrEmpty(game))throw new IOException("Game folder not found. Use --game-dir.");game=Path.GetFullPath(game);
        if(!File.Exists(game+"/Game.exe"))throw new IOException("Game.exe was not found in "+game);
        JsonObject manifest=ReadJson(bundle+"/manifest.json");
        if(Text(manifest,"platform")!="linux")throw new IOException("Expected a Linux package.");
        foreach(var pair in Map(manifest,"files"))if(HashFile(Under(bundle,pair.Key))!=pair.Value!.GetValue<string>())throw new IOException("Package integrity check failed: "+pair.Key);
        string statePath=Under(game,"localserver/install-state.json");JsonObject state=File.Exists(statePath)?ReadJson(statePath):new();bool previousInstall=state.ContainsKey("version");
        string backups=Path.GetFullPath(options.Get("--backup-dir",game+"/bak")),previous=Text(state,"backup_dir");bool restore=options.Has("--restore");
        var originals=new Dictionary<string,byte[]>();var plan=new Dictionary<string,byte[]>();
        if(Map(manifest,"patches").Count!=0)throw new IOException("Legacy binary-delta packages require their original installer.");
        JsonObject required=Map(manifest,"required_existing"),vanilla=Map(manifest,"vanilla_originals"),legacy=Map(manifest,"compatible_existing"),upgrades=Map(manifest,"client_upgrades");
        string scratch=Temp(Path.GetTempPath());
        try {
            foreach(var pair in required) {
                string name=pair.Key,current=Under(game,name);if(!File.Exists(current))throw new IOException("Required game DLL is missing: "+name);
                string input=HashFile(current),expected=pair.Value!.GetValue<string>(),original=Text(vanilla,name);
                JsonObject changes=Map(upgrades,name);var ready=new HashSet<string>(changes.Select(p=>p.Value!.GetValue<string>())){expected};
                if(original==""){if(input!=expected)throw new IOException("Unsupported existing DLL: "+name);continue;}
                if(input!=original&&input!=Text(legacy,name)&&!ready.Contains(input)&&!changes.ContainsKey(input))throw new IOException("Unsupported game DLL; retained: "+name);
                string saved=Under(backups,"originals/"+name);string? candidate=File.Exists(saved)?saved:previous!=""&&File.Exists(Under(previous,"originals/"+name))?Under(previous,"originals/"+name):input==original?current:null;
                if(candidate!=null){byte[] data=File.ReadAllBytes(candidate);if(Hash(data)!=original)throw new IOException("Original backup hash mismatch: "+name);originals[name]=data;}
                if(restore){if(!originals.TryGetValue(name,out var bytes))throw new IOException("Verified original backup missing: "+name);plan[name]=bytes;continue;}
                if(ready.Contains(input))continue;
                bool fresh=originals.ContainsKey(name);string source=scratch+"/"+name,output=scratch+"/patched-"+name;
                File.WriteAllBytes(source,fresh?originals[name]:File.ReadAllBytes(current));
                void Patch(string[] flags,string from,string helper,string to) {
                    if(RunProcess(Self,flags.Concat(new[]{from,Under(bundle,"payload/"+helper),to}))!=0)throw new IOException("Local DLL patching failed: "+name);
                }
                if(!fresh&&changes.ContainsKey(input)) {
                    if(name=="Parse.NetFx45.dll"&&input==Text(legacy,name)){string next=scratch+"/with-news.dll";Patch(new[]{"--news-header"},source,"ReRiff.NewsHeaders.dll",next);source=next;}
                    if(RunProcess(Self,new[]{"--retarget-helpers",source,bundle+"/payload",output})!=0)throw new IOException("Helper reference repair failed.");
                } else if(name=="ParseOctane.dll")Patch(Array.Empty<string>(),source,"ReRiff.ClientConfig.dll",output);
                else {if(fresh){string next=scratch+"/offline.dll";Patch(new[]{"--offline-gate"},source,"ReRiff.ClientConfig.dll",next);source=next;}Patch(new[]{"--news-header"},source,"ReRiff.NewsHeaders.dll",output);}
                byte[] result=File.ReadAllBytes(output);string outputHash=fresh?expected:Text(changes,input,expected);
                if(Hash(result)!=outputHash)throw new IOException("Local patch output hash mismatch: "+name);plan[name]=result;
            }
            if(!restore) {
                foreach(var pair in Map(manifest,"files"))if(pair.Key.StartsWith("payload/",StringComparison.Ordinal)){string name=pair.Key[8..];Under(game,name);plan[name]=File.ReadAllBytes(Under(bundle,pair.Key));}
                // An older stable package can be installed with this helper too.
                // Keep a native recovery entry point even if that payload predates it.
                if(!plan.ContainsKey("localserver/ReRiff-ClientHost"))plan["localserver/ReRiff-ClientHost"]=File.ReadAllBytes(Self);
                plan["ReRiff GitHub.desktop"]=Encoding.UTF8.GetBytes("[Desktop Entry]\nType=Link\nName=ReRiff on GitHub\nURL=https://github.com/Aftonstjarna/ReRiff\nIcon=web-browser\n");
                if(!File.Exists(game+"/server.ini"))plan["server.ini"]=Encoding.UTF8.GetBytes("[server]\nexternal_url =\nfallback_url =\ntimeout_seconds = 5\nnetworking_enabled = true\nshow_server_popup = true\n\n[launcher]\nskip_launcher = false\nauto_update_on_skip = false\n");
                var patched=new JsonObject();foreach(var pair in required)patched[pair.Key]=Hash(plan.TryGetValue(pair.Key,out var data)?data:File.ReadAllBytes(Under(game,pair.Key)));
                bool music=!options.Has("--no-music-link")&&(options.Get("--music-dir")!=""||Flag(state,"music_link",true));
                string musicPath=Path.GetFullPath(options.Get("--music-dir",Text(state,"music_dir",MusicDir())));
                state=new JsonObject{["version"]=Text(manifest,"version"),["backup_dir"]=backups,["patched"]=patched,["music_link"]=music,["music_dir"]=musicPath};
                plan["localserver/install-state.json"]=Encoding.UTF8.GetBytes(state.ToJsonString()+"\n");
                if(options.Get("--work")!=""&&File.Exists(Path.Combine(options.Get("--work"),"resume-command.txt")))plan["localserver/update-notification.ini"]=Encoding.UTF8.GetBytes("[update]\nversion="+Text(manifest,"version")+"\n");
            }
            foreach(var pair in plan)Under(game,pair.Key);
            Console.WriteLine("Validated ReRiff "+Text(manifest,"version")+": "+plan.Count+" files; backups: "+backups);
            if(options.Has("--dry-run"))return;
            foreach(var pair in originals){string path=Under(backups,"originals/"+pair.Key);if(File.Exists(path)&&HashFile(path)!=Hash(pair.Value))throw new IOException("Original backup conflict: "+pair.Key);}
            foreach(var pair in originals) {
                string path=Under(backups,"originals/"+pair.Key);if(File.Exists(path))continue;Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                try{using var file=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None);file.Write(pair.Value);}catch(IOException){if(!File.Exists(path)||HashFile(path)!=Hash(pair.Value))throw;}
            }
            var changed=new List<(string Path,byte[]? Before,UnixFileMode Mode)>();
            try {
                foreach(var pair in plan) {
                    string path=Under(game,pair.Key);byte[]? before=File.Exists(path)?File.ReadAllBytes(path):null;
                    if(before!=null&&before.AsSpan().SequenceEqual(pair.Value))continue;
                    var mode=before!=null?File.GetUnixFileMode(path):UnixFileMode.UserRead|UnixFileMode.UserWrite;
                    if(before!=null&&!originals.ContainsKey(pair.Key)){string backup=Under(backups,"versions/"+Hash(before)+"-"+Path.GetFileName(path));if(!File.Exists(backup))Atomic(backup,before);}
                    changed.Add((path,before,mode));Atomic(path,pair.Value,path.EndsWith(".sh",StringComparison.Ordinal)||path.EndsWith(".AppImage",StringComparison.Ordinal)||pair.Key=="localserver/ReRiff-ClientHost");
                }
                if(!restore&&previousInstall)foreach(string obsolete in new[]{"ReRiff-Runtime-x86_64.AppImage","apply-client-update.py","server-control.py","podman-server.py"}) {
                    string path=Under(game,"localserver/"+obsolete);if(!File.Exists(path)||plan.ContainsKey("localserver/"+obsolete))continue;
                    byte[] before=File.ReadAllBytes(path);string backup=Under(backups,"versions/"+Hash(before)+"-"+obsolete);if(!File.Exists(backup))Atomic(backup,before);
                    changed.Add((path,before,File.GetUnixFileMode(path)));File.Delete(path);
                }
            } catch {foreach(var entry in changed.AsEnumerable().Reverse()){if(entry.Before==null){if(File.Exists(entry.Path))File.Delete(entry.Path);}else{Atomic(entry.Path,entry.Before);File.SetUnixFileMode(entry.Path,entry.Mode);}}throw;}
            if(!restore&&Flag(state,"music_link"))try{LinkMusic(game,Text(state,"music_dir"));}catch(Exception e) when(e is IOException or UnauthorizedAccessException){Console.WriteLine("Music link unchanged: "+e.Message);}
            Console.WriteLine(restore?"Original DLLs restored.":"ReRiff "+Text(manifest,"version")+" installed.");Console.WriteLine("Settings and saved data preserved.");
        } finally {Directory.Delete(scratch,true);}
    }
    static void Extract(string archive,string target) {
        using var input=File.OpenRead(archive);byte[] marker=Encoding.ASCII.GetBytes("\n__RERIFF_PAYLOAD__\n");int matched=0,read,total=0;
        while((read=input.ReadByte())!=-1&&total++<65536){if(read==marker[matched]){if(++matched==marker.Length)break;}else matched=read==marker[0]?1:0;}
        if(matched!=marker.Length)throw new IOException("Invalid Linux release package.");
        using var gzip=new GZipStream(input,CompressionMode.Decompress);using var tar=new TarReader(gzip);long expanded=0;TarEntry? entry;
        while((entry=tar.GetNextEntry())!=null) {
            if(entry.EntryType!=TarEntryType.Directory&&entry.EntryType!=TarEntryType.RegularFile&&entry.EntryType!=TarEntryType.V7RegularFile)throw new IOException("Archive links and special files are not supported.");
            string path=Under(target,entry.Name);if(entry.Length>512L*1024*1024-expanded)throw new IOException("Release is too large.");expanded+=entry.Length;
            if(entry.EntryType==TarEntryType.Directory)Directory.CreateDirectory(path);
            else {Directory.CreateDirectory(Path.GetDirectoryName(path)!);using var output=new FileStream(path,FileMode.CreateNew);entry.DataStream?.CopyTo(output);}
        }
    }
    static void Update(Options options) {
        string work=options.Need("--work"),game=options.Get("--game-dir");
        if(game==""&&File.Exists(work+"/prepared.json"))game=Text(ReadJson(work+"/prepared.json"),"game");
        string log=game!=""?Path.Combine(game,"logs/update.log"):work+"/update.log";Directory.CreateDirectory(Path.GetDirectoryName(log)!);
        var stdout=Console.Out;var stderr=Console.Error;
        using var writer=new StreamWriter(log,append:true){AutoFlush=true};Console.SetOut(writer);Console.SetError(writer);
        try {
            if(options.Has("--prepare")) {
                string archive=options.Need("--archive"),expected=options.Get("--sha256");if(HashFile(archive)!=expected.ToLowerInvariant())throw new IOException("Release checksum mismatch.");
                string extracted=work+"/extracted";if(Directory.Exists(extracted))throw new IOException("Update work directory already used.");Directory.CreateDirectory(extracted);Extract(archive,extracted);
                var manifests=Directory.GetFiles(extracted,"manifest.json").Concat(Directory.GetDirectories(extracted).Select(p=>p+"/manifest.json").Where(File.Exists)).ToArray();if(manifests.Length!=1)throw new IOException("Expected one release manifest.");
                string bundle=Path.GetDirectoryName(manifests[0])!;var manifest=ReadJson(manifests[0]);
                if(Text(manifest,"platform")!="linux"||!Flag(manifest,"update_only"))throw new IOException("Expected an update-only Linux package.");
                if(options.Get("--channel")!=""&&options.Get("--channel")!=Text(manifest,"channel","stable"))throw new IOException("Release channel mismatch.");
                if(File.Exists(work+"/latest-release.json")){var metadata=ReadJson(work+"/latest-release.json");if(Text(metadata,"version")!=Text(manifest,"version")||metadata["artifacts"]?.AsArray().Any(p=>p?.GetValue<string>()==Path.GetFileName(archive))!=true)throw new IOException("Release metadata mismatch.");}
                string next=Path.Combine(bundle,"payload/localserver/ReRiff-ClientHost");
                if(File.Exists(next)) {
                    string expectedHost=Text(Map(manifest,"files"),"payload/localserver/ReRiff-ClientHost");if(expectedHost==""||HashFile(next)!=expectedHost)throw new IOException("Native helper checksum mismatch.");
                    File.SetUnixFileMode(next,File.GetUnixFileMode(next)|UnixFileMode.UserExecute);
                    if(RunProcess(next,new[]{"--client-host","install","--bundle",bundle,"--game-dir",game,"--dry-run"})!=0)throw new IOException("Installer preflight failed.");
                }else {next=Self;Install(bundle,game,new Options(new[]{"--dry-run"}));}
                Atomic(work+"/ReRiff-ClientHost",File.ReadAllBytes(next),true);
                string bridge=Path.Combine(game,"localserver/linux-runtime.sh");Atomic(work+"/linux-runtime.sh",File.ReadAllBytes(bridge),true);
                AtomicJson(work+"/prepared.json",new JsonObject{["bundle"]=bundle,["game"]=game,["platform"]="linux"});
            } else {
                var plan=ReadJson(work+"/prepared.json");game=Text(plan,"game");StopServers(game+"/localserver");
                Install(Text(plan,"bundle"),game,new Options(new[]{"--work",work}));Atomic(work+"/success",Encoding.UTF8.GetBytes("success\n"));
            }
        } catch(Exception error) {writer.WriteLine(error.ToString());Atomic(work+"/failure",Encoding.UTF8.GetBytes("Native client update failed; see logs/update.log.\n"));throw;}
        finally {Console.SetOut(stdout);Console.SetError(stderr);}
    }
    static int Server(string action,Options options) {
        string root=options.Need("--root"),image=root+"/ReRiff-Server-x86_64.AppImage",result=options.Need("--result");
        try {
            if(action=="install") {
                image=options.Need("--asset");if(HashFile(image)!=options.Get("--sha256").ToLowerInvariant())throw new IOException("Local server checksum mismatch.");
                // Container-built or shared assets may be executable but not
                // owned by this user. Avoid changing their permissions.
                var mode=File.GetUnixFileMode(image);if((mode&UnixFileMode.UserExecute)==0)File.SetUnixFileMode(image,mode|UnixFileMode.UserExecute);
            }
            if(!File.Exists(image)) {
                if(action!="status"&&action!="stop")throw new IOException("Install the optional local server first.");
                Atomic(result,Encoding.Unicode.GetPreamble().Concat(Encoding.Unicode.GetBytes("[result]\nok=true\nstate=uninstalled\naddress=http://127.0.0.1:5144\nmessage=Local server is not installed.\n")).ToArray());return 0;
            }
            var args=new List<string>{"--server-control",action,"--root",root,"--result",result};if(action=="install")args.AddRange(new[]{"--asset",image,"--sha256",options.Get("--sha256")});
            return RunProcess(image,args,90,terminateOnTimeout:false);
        } catch(Exception e){Atomic(result,Encoding.Unicode.GetPreamble().Concat(Encoding.Unicode.GetBytes("[result]\nok=false\nstate=uninstalled\nmessage="+e.Message.Replace('\n',' ')+"\n")).ToArray());return 1;}
    }
    [DllImport("libc",SetLastError=true)]static extern int kill(int pid,int signal);
    static void StopServers(string root) {
        root=Path.GetFullPath(root);
        if(File.Exists(root+"/ReRiff-Server-x86_64.AppImage")&&File.Exists(root+"/native/config.json")) {
            string result=Path.GetTempFileName();try{if(Server("stop",new Options(new[]{"--root",root,"--result",result}))!=0)throw new IOException("Owned server shutdown failed.");}finally{File.Delete(result);}
        }
        // Old trace-server processes are matched by both script and server root.
        // Retain the old helper's graceful SIGTERM policy; never force-kill.
        string? Stamp(string process) {try {string stat=File.ReadAllText(process+"/stat");var fields=stat[(stat.LastIndexOf(')')+1)..].Split(' ',StringSplitOptions.RemoveEmptyEntries);return fields[0]=="Z"?null:fields[19];}catch(Exception e) when(e is IOException or UnauthorizedAccessException){return null;}}
        bool Owned(string process) {try{string[] args=File.ReadAllText(process+"/cmdline").Split('\0');int i=Array.IndexOf(args,"--server-dir");return args.Contains(root+"/trace-server.py")&&i>=0&&i+1<args.Length&&Path.GetFullPath(args[i+1])==root+"/server";}catch(Exception e) when(e is IOException or UnauthorizedAccessException){return false;}}
        foreach(string entry in Directory.GetDirectories("/proc"))if(int.TryParse(Path.GetFileName(entry),out int pid)&&pid>1&&Owned(entry)) {
            string? stamp=Stamp(entry);if(stamp==null)continue;if(Owned(entry)&&Stamp(entry)==stamp&&kill(pid,15)!=0)throw new IOException("Could not stop owned legacy server.");
            var timer=Stopwatch.StartNew();while(Stamp(entry)==stamp&&timer.Elapsed.TotalSeconds<10)Thread.Sleep(100);if(Stamp(entry)==stamp)throw new IOException("Legacy server is still stopping; no force kill was attempted.");
        }
    }
}
#endif
