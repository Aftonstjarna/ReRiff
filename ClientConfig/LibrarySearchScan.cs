using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;

namespace ReRiff {
    internal sealed class LibrarySearchScanResult {
        internal readonly LibrarySearchSong[] Songs;
        internal readonly string[] UnreadableFolders;
        internal readonly bool LimitReached;
        internal LibrarySearchScanResult(List<LibrarySearchSong> songs,List<string> unreadable,bool limited) {
            Songs=songs.ToArray();UnreadableFolders=unreadable.ToArray();LimitReached=limited;
        }
    }

    internal static class LibrarySearchScan {
        private static bool Audio(string path) {
            switch(Path.GetExtension(path).ToLowerInvariant()) {
                case ".mp3":case ".ogg":case ".wav":case ".flac":case ".fla":case ".m4a":return true;
                default:return false;
            }
        }
        // Run on a worker. Enumerate names only: no decoding, cache writes,
        // network requests or reads of audio contents. M4A visibility does not
        // imply restored decoding support.
        internal static LibrarySearchScanResult Run(string[] roots,LibrarySearchSong[] cached,CancellationToken cancellation,int limit,Action<LibrarySearchSong[]> progress=null) {
            if(roots==null)throw new ArgumentNullException("roots");
            if(cached==null)throw new ArgumentNullException("cached");
            if(limit<1)throw new ArgumentOutOfRangeException("limit");
            var metadata=new Dictionary<string,LibrarySearchSong>(StringComparer.OrdinalIgnoreCase);
            foreach(var song in cached) {
                cancellation.ThrowIfCancellationRequested();
                if(song!=null&&song.Filename.StartsWith("hst:",StringComparison.Ordinal)) {
                    try{metadata[Path.GetFullPath(song.Filename.Substring(4))]=song;}
                    catch(ArgumentException){}catch(NotSupportedException){}catch(PathTooLongException){}
                }
            }
            var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var visited=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var pending=new Stack<string>();
            var songs=new List<LibrarySearchSong>();var unreadable=new List<string>();
            var clock=System.Diagnostics.Stopwatch.StartNew();long publishedAt=-1000;int publishedCount=0;
            foreach(string root in roots) {
                cancellation.ThrowIfCancellationRequested();
                if(string.IsNullOrEmpty(root))continue;
                try{pending.Push(Path.GetFullPath(root));}
                catch(ArgumentException){unreadable.Add(root);}catch(NotSupportedException){unreadable.Add(root);}catch(PathTooLongException){unreadable.Add(root);}
            }
            while(pending.Count>0) {
                cancellation.ThrowIfCancellationRequested();
                string folder=pending.Pop();if(!visited.Add(folder))continue;
                try {
                    foreach(string file in Directory.EnumerateFiles(folder)) {
                        cancellation.ThrowIfCancellationRequested();
                        if(!Audio(file)||!seen.Add(file))continue;
                        if(songs.Count==limit)return new LibrarySearchScanResult(songs,unreadable,true);
                        LibrarySearchSong song;
                        if(!metadata.TryGetValue(file,out song))song=new LibrarySearchSong("hst:"+file,"",Path.GetFileNameWithoutExtension(file),"");
                        songs.Add(song);
                        if(progress!=null&&songs.Count!=publishedCount&&clock.ElapsedMilliseconds-publishedAt>=500){
                            progress(songs.ToArray());publishedAt=clock.ElapsedMilliseconds;publishedCount=songs.Count;
                        }
                    }
                    foreach(string child in Directory.EnumerateDirectories(folder)) {
                        cancellation.ThrowIfCancellationRequested();
                        // A selected root may itself be the Linux music link.
                        // Do not follow nested links/junctions into cycles or
                        // unrelated trees. They can be supplied as roots.
                        try {if((File.GetAttributes(child)&FileAttributes.ReparsePoint)==0)pending.Push(child);}
                        catch(IOException){unreadable.Add(child);}catch(UnauthorizedAccessException){unreadable.Add(child);}
                    }
                }catch(IOException){unreadable.Add(folder);}catch(UnauthorizedAccessException){unreadable.Add(folder);}
            }
            songs.Sort((a,b)=>StringComparer.OrdinalIgnoreCase.Compare(a.Filename,b.Filename));
            return new LibrarySearchScanResult(songs,unreadable,false);
        }
    }
}
