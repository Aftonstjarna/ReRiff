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
        private static int newsDiagnostics;
        private static void NewsLog(string message){
            if(Interlocked.Increment(ref newsDiagnostics)>12)return;
            try{string folder=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"logs");Directory.CreateDirectory(folder);File.AppendAllText(Path.Combine(folder,"ReRiff-news.log"),DateTime.UtcNow.ToString("o")+" "+message+Environment.NewLine);}catch(IOException){}catch(UnauthorizedAccessException){}
        }
        public static Task<Tuple<HttpStatusCode,string>> WithClientNews(Task<Tuple<HttpStatusCode,string>> source, Uri uri, string method) {
            if(source==null||uri==null||method!="GET"||!uri.AbsolutePath.EndsWith("/classes/News",StringComparison.Ordinal))return source;
            Dictionary<string,object> entry;
            try {
                string folder=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"localserver");
                string version=File.ReadAllText(Path.Combine(folder,"client-version.txt")).Trim();
                if(!System.Text.RegularExpressions.Regex.IsMatch(version,@"\A[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}(?:-[1-9][0-9]{0,3})?\z"))return source;
                string notes=File.ReadAllText(Path.Combine(folder,"client-release-notes.md"));
                if(notes.Length==0||notes.Length>131072||!notes.StartsWith("# ReRiff "+version+" ",StringComparison.Ordinal))return source;
                entry=new Dictionary<string,object>{{"objectId","ReRiffClientRelease"},{"ID",0},{"Status","Active"},{"Pinned",true}};
                foreach(string lang in new[]{"en","de","ru","es","it","fr"}){entry[lang+"Title"]="ReRiff "+version;entry[lang]=notes;}
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
