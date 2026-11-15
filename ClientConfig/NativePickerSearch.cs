using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ReRiff {
    // All original UI/string/vector calls stay on the menu thread. Workers own
    // managed snapshots only and cannot publish into a later picker session.
    internal static class NativePickerSearch {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int Callback(int operation,IntPtr menu,IntPtr text,int argument);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void One(IntPtr value);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void Two(IntPtr first,IntPtr second);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate IntPtr Construct(IntPtr storage,IntPtr text);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate IntPtr ConvertPath(IntPtr menu,IntPtr storage,IntPtr text);
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigurePickerSearch(IntPtr callback);
        private static readonly Callback callback=Dispatch;
        private static Construct wideCtor,narrowCtor;
        private static One wideDtor,narrowDtor,vectorDtor,rebuild,visibility;
        private static Two push,swap,selected;
        private static ConvertPath shortPath;
        private static Func<IntPtr> saveGame;
        private static readonly object gate=new object();
        private sealed class Session {
            internal IntPtr Menu,Saved;
            internal CancellationTokenSource Cancel=new CancellationTokenSource();
            internal PickerSearchIndex Index;
            internal LibrarySearchSong[] Songs,Shown,MatchedSongs;
            internal string Query="";
            internal int Revision,Published=-1,Applied=-1,Total;
            internal int[] Matches;
            internal bool Failed,Partial;
        }
        private static Session current;
        [DllImport("kernel32.dll",CharSet=CharSet.Unicode,ExactSpelling=true)]
        private static extern uint GetLongPathNameW(string path,StringBuilder output,uint capacity);
        private static string LongPath(string path) {
            var buffer=new StringBuilder(32768);uint size=GetLongPathNameW(path,buffer,(uint)buffer.Capacity);
            return size>0&&size<buffer.Capacity?buffer.ToString():path;
        }
        private static T Bind<T>(Module game,int token,string name) where T:class {
            var method=game.ResolveMethod(token) as MethodInfo;
            if(method==null||method.Name!=name)throw new IOException("Picker search binding differs");
            return NativeReelBridge.CreateCall(method,typeof(T),false) as T;
        }
        internal static void Configure(Module game) {
            const string wide="std.basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >";
            const string narrow="std.basic_string<char,std::char_traits<char>,std::allocator<char> >";
            const string vector="std.vector<gaFileSelectInterface::dirEntry,std::allocator<gaFileSelectInterface::dirEntry> >";
            wideCtor=Bind<Construct>(game,0x06001060,wide+".{ctor}");wideDtor=Bind<One>(game,0x06000985,wide+".{dtor}");
            narrowCtor=Bind<Construct>(game,0x06000023,narrow+".{ctor}");narrowDtor=Bind<One>(game,0x06000024,narrow+".{dtor}");
            vectorDtor=Bind<One>(game,0x06001071,vector+".{dtor}");push=Bind<Two>(game,0x06001074,vector+".push_back");swap=Bind<Two>(game,0x0600109f,vector+".swap");
            rebuild=Bind<One>(game,0x0600105b,"gaFileSelectInterface.setupDirectoryList");
            visibility=Bind<One>(game,0x06001056,"gaFileSelectInterface.updateVisibility");
            selected=Bind<Two>(game,0x0600104f,"gaFileSelectInterface.onFileSelected");
            shortPath=Bind<ConvertPath>(game,0x0600105d,"gaFileSelectInterface.getSystemShortFilename");
            saveGame=ProgressionNative.Singleton(game,0x0400126f,"?ms_instance@gaSaveGame@@0PAV1@A");
            if(ReRiffConfigurePickerSearch(Marshal.GetFunctionPointerForDelegate(callback))!=1)throw new IOException("Picker search registration refused");
        }
        private static IntPtr Storage(int bytes) {var p=Marshal.AllocHGlobal(bytes);for(int i=0;i<bytes;i++)Marshal.WriteByte(p,i,0);return p;}
        private static string NativeString(IntPtr value,bool wide) {
            int length=Marshal.ReadInt32(value,16),capacity=Marshal.ReadInt32(value,20);
            if(length<0||length>32767||capacity<length)throw new IOException("Invalid native search string");
            var data=capacity<(wide?8:16)?value:Marshal.ReadIntPtr(value);
            if(wide)return Marshal.PtrToStringUni(data,length);
            byte[] bytes=new byte[length];Marshal.Copy(data,bytes,0,length);return new UTF8Encoding(false,true).GetString(bytes);
        }
        private static string[] Roots(IntPtr menu) {
            var roots=new List<string>();string music=Environment.GetFolderPath(Environment.SpecialFolder.MyMusic);if(music.Length>0)roots.Add(music);
            IntPtr save=saveGame();if(save!=IntPtr.Zero) {
                IntPtr begin=Marshal.ReadIntPtr(save,348),end=Marshal.ReadIntPtr(save,352);
                long bytes=end.ToInt64()-begin.ToInt64();if(bytes<0||bytes%48!=0||bytes/48>10000)throw new IOException("Invalid native music roots");
                for(int i=0;i<bytes/48;i++){string path=NativeString(IntPtr.Add(begin,i*48),false);if(path.StartsWith("hst:",StringComparison.Ordinal))path=path.Substring(4);if(path.Length>0)roots.Add(path);}
            }
            IntPtr first=Marshal.ReadIntPtr(menu,264),last=Marshal.ReadIntPtr(menu,268);
            long size=last.ToInt64()-first.ToInt64();if(size<0||size%24!=0||size/24>1024)throw new IOException("Invalid picker path");
            var folder=new StringBuilder();for(int i=0;i<size/24;i++)folder.Append(NativeString(IntPtr.Add(first,i*24),true)).Append('\\');
            if(folder.Length>0)roots.Add(folder.ToString());return roots.ToArray();
        }
        private static void Start(IntPtr menu) {
            Close(false);string[] roots=Roots(menu);var session=new Session {Menu=menu};lock(gate)current=session;
            ThreadPool.QueueUserWorkItem(_=>{
                try {
                    var cached=new LibrarySearchSong[0];
                    string path=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),"Octane","DAT","cache.bin");
                    if(File.Exists(path))try{if(new FileInfo(path).Length<=16*1024*1024)if(!LibrarySearchCache.TryRead(File.ReadAllBytes(path),out cached))NativeReelBridge.Log("Picker cache ignored: invalid catalogue; continuing filesystem search.");}catch(InvalidDataException){NativeReelBridge.Log("Picker cache ignored: invalid catalogue; continuing filesystem search.");}catch(IOException){}catch(UnauthorizedAccessException){}
                    // Cached paths work even when stored in short form. Publish
                    // immediately instead of resolving every file before search.
                    var clock=System.Diagnostics.Stopwatch.StartNew();
                    cached=Array.FindAll(cached,song=>song!=null&&song.Filename.StartsWith("hst:",StringComparison.Ordinal));
                    Publish(session,cached,true);
                    var allRoots=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                    foreach(string root in roots)if(!string.IsNullOrEmpty(root))allRoots.Add(LongPath(root));
                    foreach(var song in cached)if(song.Filename.StartsWith("hst:",StringComparison.Ordinal))try{string parent=Path.GetDirectoryName(song.Filename.Substring(4));if(!string.IsNullOrEmpty(parent))allRoots.Add(parent);}catch(ArgumentException){}
                    var scan=LibrarySearchScan.Run(new List<string>(allRoots).ToArray(),cached,session.Cancel.Token,100000,songs=>Publish(session,Merge(cached,songs),true));
                    Publish(session,scan.Songs,scan.LimitReached||scan.UnreadableFolders.Length>0);
                    NativeReelBridge.Log("Picker scan completed: tracks="+scan.Songs.Length+" unreadable="+scan.UnreadableFolders.Length+" elapsed_ms="+clock.ElapsedMilliseconds);
                }catch(OperationCanceledException){}catch(Exception error){lock(gate){if(current==session)session.Failed=true;}NativeReelBridge.Log("Picker worker failed: "+error.GetType().Name);}
            });
        }
        private static LibrarySearchSong[] Merge(LibrarySearchSong[] cached,LibrarySearchSong[] scanned) {
            var songs=new Dictionary<string,LibrarySearchSong>(StringComparer.OrdinalIgnoreCase);
            foreach(var song in cached)songs[song.Filename]=song;
            foreach(var song in scanned)if(!songs.ContainsKey(song.Filename))songs[song.Filename]=song;
            var result=new LibrarySearchSong[songs.Count];songs.Values.CopyTo(result,0);return result;
        }
        private static void Publish(Session session,LibrarySearchSong[] songs,bool partial) {
            session.Cancel.Token.ThrowIfCancellationRequested();
            string[] names=new string[songs.Length];for(int i=0;i<names.Length;i++)names[i]=songs[i].SearchText;
            var index=new PickerSearchIndex(names);
            lock(gate){if(current!=session)return;session.Songs=songs;session.Index=index;session.Partial=partial;session.Revision++;}
            Search(session);
        }
        private static void Search(Session session) {
            string query;int revision;PickerSearchIndex index;LibrarySearchSong[] songs;
            lock(gate){if(current!=session||session.Index==null)return;query=session.Query;revision=session.Revision;index=session.Index;songs=session.Songs;}
            ThreadPool.QueueUserWorkItem(_=>{
                try {
                    session.Cancel.Token.ThrowIfCancellationRequested();int[] matches=index.Find(query);
                    lock(gate){if(current!=session||session.Revision!=revision)return;session.Matches=matches;session.MatchedSongs=songs;session.Published=revision;}
                }catch(OperationCanceledException){}catch(Exception error){lock(gate){if(current==session)session.Failed=true;}NativeReelBridge.Log("Picker worker failed: "+error.GetType().Name);}
            });
        }
        private static void Restore(Session session,bool layout) {
            if(session.Saved==IntPtr.Zero)return;
            IntPtr saved=session.Saved;session.Saved=IntPtr.Zero;
            swap(IntPtr.Add(session.Menu,276),saved);vectorDtor(saved);Marshal.FreeHGlobal(saved);
            session.Shown=null;if(layout){rebuild(session.Menu);visibility(session.Menu);}
        }
        private static void Close(bool layout) {
            Session session;lock(gate){session=current;current=null;}
            if(session==null)return;session.Cancel.Cancel();Restore(session,layout);
            // Worker callbacks can still inspect the cancellation token.
        }
        private static void Apply(Session session,int[] matches,LibrarySearchSong[] songs) {
            const int maximumRows=200;int count=Math.Min(matches.Length,maximumRows);
            IntPtr vector=Storage(12),entry=Storage(28);
            var shown=new LibrarySearchSong[count];
            try {
                // Empty results deliberately use zero native entries, not a
                // synthetic file that the original picker might try to open.
                for(int i=0;i<count;i++) {
                    var song=songs[matches[i]];shown[i]=song;
                    string label=Path.GetFileName(song.Filename.Substring(4));
                    if(song.Artist.Length>0)label=song.Artist+" - "+song.Title+"  ("+label+")";
                    IntPtr text=Marshal.StringToHGlobalUni(label);
                    try{wideCtor(entry,text);try{Marshal.WriteInt32(entry,24,4);push(vector,entry);}finally{wideDtor(entry);}}finally{Marshal.FreeHGlobal(text);}
                }
                if(session.Saved==IntPtr.Zero){session.Saved=Storage(12);swap(IntPtr.Add(session.Menu,276),session.Saved);}
                swap(IntPtr.Add(session.Menu,276),vector);session.Shown=shown;session.Total=matches.Length;rebuild(session.Menu);visibility(session.Menu);
            }finally{vectorDtor(vector);Marshal.FreeHGlobal(vector);Marshal.FreeHGlobal(entry);}
        }
        private static int Select(Session session,int row) {
            if(session.Shown==null||row<0||row>=session.Shown.Length)return 0;
            string path=session.Shown[row].Filename.Substring(4);if(!File.Exists(path))return -2;
            IntPtr wide=Storage(24),narrow=Storage(24),chosen=Storage(24),text=Marshal.StringToHGlobalUni(path);
            bool converted=false,constructed=false;
            try {
                wideCtor(wide,text);
                // The original conversion consumes its by-value wide string.
                shortPath(session.Menu,narrow,wide);converted=true;
                int length=Marshal.ReadInt32(narrow,16);if(length==0)return -2;
                IntPtr data=Marshal.ReadInt32(narrow,20)<16?narrow:Marshal.ReadIntPtr(narrow);
                byte[] filename=new byte[length+5];filename[0]=(byte)'h';filename[1]=(byte)'s';filename[2]=(byte)'t';filename[3]=(byte)':';Marshal.Copy(data,filename,4,length);
                IntPtr bytes=Marshal.AllocHGlobal(filename.Length);try{Marshal.Copy(filename,0,bytes,filename.Length);narrowCtor(chosen,bytes);constructed=true;}finally{Marshal.FreeHGlobal(bytes);}
                selected(session.Menu,chosen);return 1;
            }finally{if(constructed)narrowDtor(chosen);if(converted)narrowDtor(narrow);Marshal.FreeHGlobal(wide);Marshal.FreeHGlobal(narrow);Marshal.FreeHGlobal(chosen);Marshal.FreeHGlobal(text);}
        }
        private static int Dispatch(int operation,IntPtr menu,IntPtr text,int argument) {
            try {
                if(operation==0){Start(menu);return 1;}
                Session session;lock(gate)session=current;if(session==null||session.Menu!=menu)return -2;
                if(operation==3){Close(argument!=0);return 1;}
                if(operation==4)return Select(session,argument);
                if(operation==1) {
                    string query=Marshal.PtrToStringUni(text)??"";
                    lock(gate){session.Query=query;session.Revision++;}
                    if(query.Length==0)Restore(session,true);else Search(session);
                }
                int[] matches=null;LibrarySearchSong[] songs=null;
                lock(gate) {
                    if(session.Failed)return -2;
                    if(session.Query.Length==0)return -3;
                    if(session.Index==null||session.Published!=session.Revision)return -1;
                    if(session.Applied!=session.Published){matches=session.Matches;songs=session.MatchedSongs;session.Applied=session.Published;}
                }
                if(matches!=null)Apply(session,matches,songs);
                return session.Partial?-(session.Total+10):session.Total;
            }catch(Exception error){lock(gate){if(current!=null&&current.Menu==menu)current.Failed=true;}NativeReelBridge.Log("Picker search operation failed: "+error.GetType().Name);return -2;}
        }
    }
}
