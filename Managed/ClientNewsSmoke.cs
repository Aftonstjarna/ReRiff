using System;
using System.IO;
using System.Net;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Web.Script.Serialization;

class ClientNewsSmoke {
    static void Check(bool ok,string label){if(!ok)throw new Exception(label);Console.WriteLine("PASS: "+label);}
    static Task<Tuple<HttpStatusCode,string>> Result(string body){
        var completion=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();
        completion.SetResult(Tuple.Create(HttpStatusCode.OK,body));return completion.Task;
    }
    public static void Run(){
        string directory=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"localserver");
        Directory.CreateDirectory(directory);
        string versionFile=Path.Combine(directory,"client-version.txt");
        string notesFile=Path.Combine(directory,"client-release-notes.md");
        File.WriteAllText(versionFile,"1.2.3-4");
        File.WriteAllText(Path.Combine(directory,"install-state.json"),"{\"version\":\"1.2.3-4\"}");
        File.WriteAllText(notesFile,"# ReRiff 1.2.3-4 — notes\nChanges \\\" & Unicode ü");
        var uri=new Uri("http://127.0.0.1/classes/News");
        var json=new JavaScriptSerializer();
        var source=Result("{\"results\":[{\"objectId\":\"custom\",\"en\":\"Server post\"},{\"objectId\":\"ReRiffServerStatus\"},{\"objectId\":\"SingleplayerNews\"}],\"count\":3}");
        var result=ReRiff.NewsHeaders.WithClientNews(source,uri,"GET").Result;
        var document=json.DeserializeObject(result.Item2) as Dictionary<string,object>;
        var rows=(object[])document["results"];
        Check(rows.Length==2&&Convert.ToInt32(document["count"])==2,"client notes prepended and count updated");
        Check((string)((Dictionary<string,object>)rows[1])["en"]=="Server post","custom server post preserved");
        Check((string)((Dictionary<string,object>)rows[0])["enTitle"]=="ReRiff 1.2.3-4","full experimental revision retained");
        Check(Object.ReferenceEquals(source,ReRiff.NewsHeaders.WithClientNews(source,new Uri("http://127.0.0.1/classes/Score"),"GET")),"non-News task unchanged");
        Check(Object.ReferenceEquals(source,ReRiff.NewsHeaders.WithClientNews(source,uri,"POST")),"writes unchanged");
        var canceled=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();canceled.SetCanceled();
        Check(ReRiff.NewsHeaders.WithClientNews(canceled.Task,uri,"GET").IsCanceled,"cancellation retained");
        var fault=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();fault.SetException(new IOException("original"));
        Check(ReRiff.NewsHeaders.WithClientNews(fault.Task,uri,"GET").IsFaulted,"transport failure retained");
        var request=WebRequest.Create(uri);ReRiff.NewsHeaders.Add(request);
        Check(request.Headers["X-ReRiff-Client-Version"]=="1.2.3-4","version header preserves prerelease");
        foreach(string version in new[]{"1.2.3","1.2.3-8"}){
            File.WriteAllText(versionFile,version);
            File.WriteAllText(notesFile,"# ReRiff "+version+" — Long heading must not repeat in body\r\n\r\n"+
                "- First change with a [source](https://example.com/other) and lots of explanatory words that cannot fit in one small News preview.\r\n"+
                "  Wrapped continuation must count as the same bullet.\r\n"+
                "- Second change with `formatting` and **details** about updated behaviour in several areas of the game.\r\n"+
                "- Third change with more information for the full online release notes.\r\n"+
                "- Fourth change must be omitted.\r\n\r\n"+new string('x',10000));
            result=ReRiff.NewsHeaders.WithClientNews(source,uri,"GET").Result;
            document=json.DeserializeObject(result.Item2) as Dictionary<string,object>;
            var entry=(Dictionary<string,object>)((object[])document["results"])[0];
            string body=(string)entry["en"];
            Check(body.Length<=280&&body.Split('\n').Length<=6,"preview stays within compact character/line budget "+version);
            Check(!body.Contains("# ReRiff")&&!body.Contains("Long heading")&&!body.Contains("Fourth change"),"heading and excess notes omitted");
            Check(body.Contains("...")&&body.Contains("GitHub"),"long entries truncated with full-notes direction");
            string link="https://github.com/Aftonstjarna/ReRiff/releases/tag/v"+version;
            Check(body.EndsWith(link,StringComparison.Ordinal)&&body.IndexOf("https://",StringComparison.Ordinal)==body.LastIndexOf("https://",StringComparison.Ordinal),"one exact release link for native Visit URL");
            foreach(string language in new[]{"de","ru","es","it","fr"})Check((string)entry[language]==body,"same bounded preview in "+language);
        }
        File.WriteAllText(notesFile,"# ReRiff 1.2.3-3 — stale");
        Check(Object.ReferenceEquals(source,ReRiff.NewsHeaders.WithClientNews(source,uri,"GET")),"stale notes rejected");
    }
}
