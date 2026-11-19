using System;
using System.IO;
using System.Text;

namespace ReRiff {
    internal sealed class LibrarySearchSong {
        internal readonly string Filename, SongId, Title, Artist;
        internal LibrarySearchSong(string filename,string songId,string title,string artist) {
            Filename=filename;SongId=songId;Title=title;Artist=artist;
        }
        internal string SearchText { get { return Filename+"\n"+Title+"\n"+Artist; } }
    }

    // Read-only v19 catalogue projection. Layout follows the existing
    // NativeLauncher/song_sync.h parser and original writeSongCache evidence.
    // Does not decode audio, change song IDs, or write the game's cache.
    internal static class LibrarySearchCache {
        private static readonly Encoding Utf8=new UTF8Encoding(false,true);
        private static readonly Encoding DisplayUtf8=new UTF8Encoding(false,false);
        internal static bool TryRead(byte[] bytes,out LibrarySearchSong[] songs) {
            try {songs=Read(bytes);return true;}
            catch(InvalidDataException){songs=new LibrarySearchSong[0];return false;}
        }
        internal static LibrarySearchSong[] Read(byte[] bytes) {
            if(bytes==null)throw new ArgumentNullException("bytes");
            if(bytes.Length<16||bytes.Length>16*1024*1024)throw new InvalidDataException("Invalid song cache size");
            using(var stream=new MemoryStream(bytes,false))using(var reader=new BinaryReader(stream)) {
                uint magic=reader.ReadUInt32(),version=reader.ReadUInt32(),count=reader.ReadUInt32(),recordVersion=reader.ReadUInt32();
                if(magic!=1128350512||version!=19||recordVersion!=18||count>10000)throw new InvalidDataException("Unsupported song cache");
                var songs=new LibrarySearchSong[(int)count];
                int cursor=16;
                for(int i=0;i<songs.Length;i++) {
                    string[] fields=new string[7];
                    for(int j=0;j<fields.Length;j++) {
                        int end=Array.IndexOf(bytes,(byte)0,cursor);
                        if(end<0)throw new InvalidDataException("Unterminated song cache field");
                        // Only display metadata may replace malformed sequences. Paths and
                        // song IDs remain strict so selection cannot target another file.
                        try {fields[j]=(j<2?Utf8:DisplayUtf8).GetString(bytes,cursor,end-cursor);}
                        catch(DecoderFallbackException error){throw new InvalidDataException("Invalid song cache UTF-8",error);}
                        cursor=end+1;
                    }
                    if(bytes.Length-cursor<29)throw new InvalidDataException("Truncated song cache record");
                    cursor+=29;
                    songs[i]=new LibrarySearchSong(fields[0],fields[1],fields[2],fields[3]);
                }
                if(cursor!=bytes.Length)throw new InvalidDataException("Unexpected song cache trailing data");
                return songs;
            }
        }
    }
}
