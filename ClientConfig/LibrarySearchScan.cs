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

    // One bounded, process-local completed scan. It owns no native UI state.
    // A short freshness window avoids repeat disk walks; older results can be
    // shown while a replacement scan removes deleted files and discovers additions.
    internal sealed class LibrarySearchSnapshot {
        internal readonly LibrarySearchSong[] Songs;
        internal readonly PickerSearchIndex Index;
        internal readonly bool Partial;
        private readonly HashSet<string> roots;
        private readonly byte[] catalogue;
        private readonly DateTime completed;
        internal LibrarySearchSnapshot(string[] roots,byte[] catalogue,LibrarySearchSong[] songs,PickerSearchIndex index,bool partial,DateTime completed) {
            this.roots=new HashSet<string>(roots,StringComparer.OrdinalIgnoreCase);
            this.catalogue=catalogue==null?null:(byte[])catalogue.Clone();
            Songs=(LibrarySearchSong[])songs.Clone();Index=index;Partial=partial;this.completed=completed;
        }
        internal bool Matches(string[] paths) {return roots.SetEquals(paths);}
        internal bool Fresh(byte[] bytes,DateTime now) {
            if(Partial||now<completed||now-completed>=TimeSpan.FromSeconds(30)||bytes==null||catalogue==null||bytes.Length!=catalogue.Length)return false;
            for(int i=0;i<bytes.Length;i++)if(bytes[i]!=catalogue[i])return false;
            return true;
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
                    // FileSystemInfo retains attributes from enumeration: one pass,
                    // without a separate attribute lookup for every directory.
                    foreach(var entry in new DirectoryInfo(folder).EnumerateFileSystemInfos()) {
                        cancellation.ThrowIfCancellationRequested();
                        string file=entry.FullName;
                        FileAttributes attributes;
                        try{attributes=entry.Attributes;}
                        catch(IOException){unreadable.Add(file);continue;}
                        catch(UnauthorizedAccessException){unreadable.Add(file);continue;}
                        if((attributes&FileAttributes.Directory)!=0) {
                            // Explicit roots may be links; nested links/junctions
                            // remain excluded to prevent cycles and tree escapes.
                            if((attributes&FileAttributes.ReparsePoint)==0)pending.Push(file);
                            continue;
                        }
                        if(!Audio(file)||!seen.Add(file))continue;
                        if(songs.Count==limit)return new LibrarySearchScanResult(songs,unreadable,true);
                        LibrarySearchSong song;
                        if(!metadata.TryGetValue(file,out song))song=new LibrarySearchSong("hst:"+file,"",Path.GetFileNameWithoutExtension(file),"");
                        songs.Add(song);
                        if(progress!=null&&songs.Count!=publishedCount&&clock.ElapsedMilliseconds-publishedAt>=500){
                            progress(songs.ToArray());publishedAt=clock.ElapsedMilliseconds;publishedCount=songs.Count;
                        }
                    }
                }catch(IOException){unreadable.Add(folder);}catch(UnauthorizedAccessException){unreadable.Add(folder);}
            }
            songs.Sort((a,b)=>StringComparer.OrdinalIgnoreCase.Compare(a.Filename,b.Filename));
            return new LibrarySearchScanResult(songs,unreadable,false);
        }
    }
}
