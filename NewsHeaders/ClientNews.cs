// Extend the original Parse HTTP result; the game's News parsing and UI remain native.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace ReRiff {
    public static partial class NewsHeaders {
        private static string NewsPreview(string version,string notes){
            // Prefer the release author's short, complete in-game summary.
            // Older releases retain the bounded preview fallback below.
            const string summaryHeading="## In-game summary";
            string[] noteLines=notes.Replace("\r","").Split('\n');
            for(int i=0;i<noteLines.Length;i++){
                if(!string.Equals(noteLines[i].Trim(),summaryHeading,StringComparison.OrdinalIgnoreCase))continue;
                var summary=new List<string>();
                for(int j=i+1;j<noteLines.Length;j++){
                    if(noteLines[j].TrimStart().StartsWith("#",StringComparison.Ordinal))break;
                    summary.Add(noteLines[j]);
                }
                if(summary.Exists(delegate(string line){return line.TrimStart().StartsWith("- ",StringComparison.Ordinal);}))
                    notes=string.Join("\n",summary.ToArray());
                break;
            }
            var bullets=new List<string>();string current=null;
            foreach(string raw in notes.Replace("\r","").Split('\n')){
                string line=raw.Trim();
                if(line.StartsWith("- ",StringComparison.Ordinal)){
                    if(current!=null)bullets.Add(current);
                    current=line.Substring(2);
                }else if(current!=null){
                    if(line.Length==0||line.StartsWith("#",StringComparison.Ordinal)){bullets.Add(current);current=null;}
                    else current+=" "+line;
                }
            }
            if(current!=null)bullets.Add(current);
            if(bullets.Count==0){
                foreach(string line in notes.Replace("\r","").Split('\n'))
                    if(line.Trim().Length>0&&!line.TrimStart().StartsWith("#",StringComparison.Ordinal)){bullets.Add(line.Trim());break;}
            }
            var preview=new List<string>();
            foreach(string bullet in bullets){
                string text=System.Text.RegularExpressions.Regex.Replace(bullet,@"\[([^\]]+)\]\([^)]+\)","$1");
                text=System.Text.RegularExpressions.Regex.Replace(text,@"(?:https?://|www\.)\S+","link");
                text=System.Text.RegularExpressions.Regex.Replace(text.Replace("`","").Replace("**",""),@"\s+"," ").Trim();
                if(text.Length>72){
                    int end=text.LastIndexOf(' ',68,68);if(end<40)end=69;
                    if(end>0&&char.IsHighSurrogate(text[end-1]))end--;
                    text=text.Substring(0,end).TrimEnd()+"...";
                }
                if(text.Length>0)preview.Add("- "+text);
                if(preview.Count==3)break;
            }
            if(preview.Count==0)preview.Add("ReRiff has been updated.");
            // The original gaNewsMenu extracts the final URL and supplies its
            // existing Visit URL button. Keep it last, without punctuation.
            return string.Join("\n",preview.ToArray())+"\n\nCheck release on GitHub for full notes\nhttps://github.com/Aftonstjarna/ReRiff/releases/tag/v"+version;
        }
        private static int newsDiagnostics;
        private static void NewsLog(string message){
            if(!Logging.Enabled(AppDomain.CurrentDomain.BaseDirectory))return;
            if(Interlocked.Increment(ref newsDiagnostics)>12)return;
            try{Logging.Append(AppDomain.CurrentDomain.BaseDirectory,"ReRiff-news.log",DateTime.UtcNow.ToString("o")+" "+message+Environment.NewLine);}catch(IOException){}catch(UnauthorizedAccessException){}
        }
        public static Task<Tuple<HttpStatusCode,string>> WithClientNews(Task<Tuple<HttpStatusCode,string>> source, Uri uri, string method) {
            if(source==null||uri==null||method!="GET"||!uri.AbsolutePath.EndsWith("/classes/News",StringComparison.Ordinal))return source;
            Dictionary<string,object> entry;
            try {
                string folder=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"localserver");
                string version=File.ReadAllText(Path.Combine(folder,"client-version.txt")).Trim();
                if(!System.Text.RegularExpressions.Regex.IsMatch(version,@"\A[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}(?:-(?:beta\.)?[1-9][0-9]{0,3})?\z"))return source;
                string notes=File.ReadAllText(Path.Combine(folder,"client-release-notes.md"));
                if(notes.Length==0||notes.Length>131072||!notes.StartsWith("# ReRiff "+version+" ",StringComparison.Ordinal))return source;
                string preview=NewsPreview(version,notes);
                entry=new Dictionary<string,object>{{"objectId","ReRiffClientRelease"},{"ID",0},{"Status","Active"},{"Pinned",true}};
                foreach(string lang in new[]{"en","de","ru","es","it","fr"}){entry[lang+"Title"]="ReRiff "+version;entry[lang]=preview;}
            } catch(IOException error){NewsLog("Client notes unavailable: "+error.Message);return source;}catch(UnauthorizedAccessException error){NewsLog("Client notes unavailable: "+error.Message);return source;}
            var completion=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();
            source.ContinueWith(delegate(Task<Tuple<HttpStatusCode,string>> task){
                if(task.IsCanceled){completion.TrySetCanceled();return;}
                if(task.IsFaulted){completion.TrySetException(task.Exception.InnerExceptions);return;}
                var original=task.Result;
                try {
                    if(original.Item1!=HttpStatusCode.OK){completion.TrySetResult(original);return;}
                    var json=new JavaScriptSerializer{MaxJsonLength=4*1024*1024};
                    var document=json.DeserializeObject(original.Item2) as Dictionary<string,object>;
                    object rows;
                    if(document==null||!document.TryGetValue("results",out rows)||!(rows is IEnumerable)){completion.TrySetResult(original);return;}
                    var merged=new List<object>{entry};
                    foreach(object row in (IEnumerable)rows){
                        var record=row as Dictionary<string,object>;object id;
                        if(record!=null&&record.TryGetValue("objectId",out id)&&id is string &&
                            ((string)id=="ReRiffClientRelease"||(string)id=="ReRiffServerStatus"||
                             (string)id=="RiffApiLocalNews"||(string)id=="SingleplayerNews"))continue;
                        merged.Add(row);
                    }
                    document["results"]=merged;
                    NewsLog("Installed client notes merged into native News results; rows="+merged.Count);
                    if(document.ContainsKey("count"))document["count"]=merged.Count;
                    completion.TrySetResult(Tuple.Create(original.Item1,json.Serialize(document)));
                }catch(Exception){completion.TrySetResult(original);}
            },CancellationToken.None,TaskContinuationOptions.ExecuteSynchronously,TaskScheduler.Default);
            return completion.Task;
        }
    }
}
