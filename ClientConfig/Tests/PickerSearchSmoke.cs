using System;
using System.IO;
using System.Text;
using System.Threading;
using ReRiff;
class PickerSearchSmoke {
    static int checks;
    static void Check(bool ok,string label){checks++;if(!ok)throw new Exception(label);}
    static void Results(PickerSearchIndex index,string query,params int[] expected) {
        int[] actual=index.Find(query);Check(string.Join(",",actual)==string.Join(",",expected),"Results for "+query);
    }
    static byte[] Cache() {
        using(var output=new MemoryStream())using(var writer=new BinaryWriter(output)) {
            writer.Write(1128350512U);writer.Write(19U);writer.Write(2U);writer.Write(18U);
            foreach(string path in new[]{"hst:C:/Music/one/song.flac","hst:D:/Music/two/song.flac"}) {
                foreach(string field in new[]{path,"original-id", "つき", "Artist", "", "", ""}) {
                    writer.Write(Encoding.UTF8.GetBytes(field));writer.Write((byte)0);
                }
                writer.Write(new byte[29]);
            }
            writer.Flush();return output.ToArray();
        }
    }
    static void Rejected(byte[] bytes,string label) {
        bool rejected=false;try{LibrarySearchCache.Read(bytes);}catch(InvalidDataException){rejected=true;}
        Check(rejected,label);
    }
    static void Main(string[] args) {
        string[] source={"つき.mp3","ツキ.flac","ﾂｷ.ogg","Moon.wav","つき.mp3","月下麗人.m4a",null,"odd\ud800.wav"};
        var index=new PickerSearchIndex(source);source[0]="changed";
        Check(index.Count==8&&index.Name(0)=="つき.mp3","Snapshot preserves original identity");
        Results(index,"tsuki",0,1,2,4);Results(index,"つき",0,1,2,4);
        Results(index,"MOON",3);Results(index,"月下",5);Results(index,"gekka");
        Results(index,"",0,1,2,3,4,5,7);Results(index,"missing");
        Results(index,"odd\ud800",7);Results(index,".wav",3,7);
        int[] changed=index.Find("tsuki");changed[0]=99;Results(index,"tsuki",0,1,2,4);
        var replacement=new PickerSearchIndex(new[]{"new folder.mp3"});
        Results(replacement,"tsuki");Results(replacement,"folder",0);
        string[] many=new string[100000];for(int i=0;i<many.Length;i++)many[i]="Track "+i+" つき.flac";
        var large=new PickerSearchIndex(many);Check(large.Find("tsuki").Length==many.Length,"Large directory");
        Results(large,"Track 99999",99999);
        var reused=new PickerSearchIndex(new[]{"Moon.wav","つき.mp3","つき.mp3",null,"updated.flac","odd\ud800.wav"},index,null);
        Results(reused,"tsuki",1,2);Results(reused,"Moon",0);Results(reused,"updated",4);Results(reused,"odd\ud800",5);
        Results(index,"tsuki",0,1,2,4); // The old snapshot remains immutable.
        bool stopped=false;int polls=0;
        try{large.Find("missing",()=>++polls==2);}catch(OperationCanceledException){stopped=true;}
        Check(stopped&&polls==2,"Obsolete query stops during scan");
        stopped=false;
        try{new PickerSearchIndex(many,large,()=>true);}catch(OperationCanceledException){stopped=true;}
        Check(stopped,"Cancelled publication stops index rebuild");
        byte[] cache=Cache(),before=(byte[])cache.Clone();var songs=LibrarySearchCache.Read(cache);
        Check(songs.Length==2&&songs[0].SongId=="original-id"&&songs[1].Filename=="hst:D:/Music/two/song.flac","Cross-folder identities preserved");
        Results(new PickerSearchIndex(new[]{songs[0].SearchText,songs[1].SearchText}),"tsuki",0,1);
        Check(Convert.ToBase64String(cache)==Convert.ToBase64String(before),"Cache remains unchanged");
        for(int cut=0;cut<cache.Length;cut++){var truncated=new byte[cut];Array.Copy(cache,truncated,cut);Rejected(truncated,"Truncation at "+cut);}
        byte[] wrong=(byte[])cache.Clone();wrong[4]=20;Rejected(wrong,"Unknown cache version");
        wrong=(byte[])cache.Clone();wrong[16]=0xff;Rejected(wrong,"Invalid UTF-8");
        wrong=new byte[cache.Length+1];Array.Copy(cache,wrong,cache.Length);Rejected(wrong,"Trailing bytes");
        string folder=Path.Combine(Path.GetTempPath(),"reriff-search-"+Guid.NewGuid().ToString("N"));
        try {
            string child=Path.Combine(folder,"nested");Directory.CreateDirectory(child);
            string imported=Path.Combine(folder,"first.FLAC");File.WriteAllBytes(imported,new byte[]{1,2,3});
            File.WriteAllBytes(Path.Combine(child,"つき.m4a"),new byte[]{4,5});
            File.WriteAllText(Path.Combine(child,"ignore.txt"),"unrelated");
            var known=new[]{new LibrarySearchSong("hst:"+imported,"retained-id","Cached title","Cached artist")};
            LibrarySearchSong[] rejected;
            Check(!LibrarySearchCache.TryRead(new byte[]{0xff},out rejected)&&rejected.Length==0,"Bad cache falls back to an empty catalogue");
            Check(LibrarySearchScan.Run(new[]{folder},rejected,CancellationToken.None,100).Songs.Length==2,"Bad cache does not prevent filesystem search");
            int publications=0;LibrarySearchSong[] firstSnapshot=null;
            var scan=LibrarySearchScan.Run(new[]{folder,child},known,CancellationToken.None,100,snapshot=>{publications++;if(firstSnapshot==null)firstSnapshot=snapshot;});
            Check(publications>0&&firstSnapshot.Length==1,"First match published before complete traversal");
            Check(scan.Songs.Length==2&&!scan.LimitReached&&scan.UnreadableFolders.Length==0,"Recursive scan and overlapping roots");
            Check(Array.Exists(scan.Songs,s=>s.SongId=="retained-id"&&s.Title=="Cached title"),"Imported metadata retained");
            Check(Array.Exists(scan.Songs,s=>s.SongId==""&&s.Title=="つき"),"Unimported M4A indexed without decoding or invented ID");
            Check(File.ReadAllBytes(imported).Length==3,"Audio untouched");
            Check(StringComparer.OrdinalIgnoreCase.Compare(scan.Songs[0].Filename,scan.Songs[1].Filename)<0,"Complete scan sorted deterministically");
            using(var during=new CancellationTokenSource()) {
                bool stoppedDuring=false;
                try{LibrarySearchScan.Run(new[]{folder},known,during.Token,100,snapshot=>during.Cancel());}catch(OperationCanceledException){stoppedDuring=true;}
                Check(stoppedDuring,"Cancellation after first publication interrupts traversal");
            }
            scan=LibrarySearchScan.Run(new[]{folder},known,CancellationToken.None,1);
            Check(scan.Songs.Length==1&&scan.LimitReached,"Bounded scan reports partial results");
            scan=LibrarySearchScan.Run(new[]{Path.Combine(folder,"missing")},known,CancellationToken.None,100);
            Check(scan.Songs.Length==0&&scan.UnreadableFolders.Length==1,"Missing folder reported");
            using(var cancel=new CancellationTokenSource()) {
                cancel.Cancel();bool cancelled=false;
                try{LibrarySearchScan.Run(new[]{folder},known,cancel.Token,100);}catch(OperationCanceledException){cancelled=true;}
                Check(cancelled,"Scan cancellation");
            }
        }finally{if(Directory.Exists(folder))Directory.Delete(folder,true);}
        if(args.Length>0) {
            string fixture=args[0];
            var scan=LibrarySearchScan.Run(new[]{Path.Combine(fixture,"tree")},new LibrarySearchSong[0],CancellationToken.None,100);
            Check(scan.Songs.Length==1,"Nested directory link and cycle excluded");
            scan=LibrarySearchScan.Run(new[]{Path.Combine(fixture,"root-link")},new LibrarySearchSong[0],CancellationToken.None,100);
            Check(scan.Songs.Length==1,"Explicit linked root remains searchable");
        }
        Console.WriteLine("PASS: {0} picker search snapshot, Unicode, identity and large-directory checks",checks);
    }
}
