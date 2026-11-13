using System;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

public class MetadataRequest {
    public Uri Uri {get;set;}
    public MemoryStream Data {get;set;}
}
public class MetadataTraceSmoke {
    static void Check(bool value,string name){if(!value)throw new Exception(name);Console.WriteLine("PASS: "+name);}
    static string ReadLog(string path){using(var stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.ReadWrite))using(var reader=new StreamReader(stream))return reader.ReadToEnd();}
    public static void Main(string[] args){
        string root=AppDomain.CurrentDomain.BaseDirectory,log=Path.Combine(root,"logs/ReRiff-metadata.log");
        bool enabled=args.Length==0;
        File.WriteAllText(Path.Combine(root,"server.ini"),"[diagnostics]\nmetadata_trace="+(enabled?"1":"0")+"\n");
        if(File.Exists(log))File.Delete(log);
        string json="{\"SongName\":\"くちづけ 月下麗人 ドレス 한글 𠀋\",\"ArtistName\":\"Artist‐Name\",\"password\":\"secret-sentinel\",\"Authorization\":\"credential-sentinel\"}";
        var request=new MetadataRequest{Uri=new Uri("https://private-host.example/classes/Score?where=%7B%22SongName%22%3A%22%E6%97%A5%E6%9C%AC%E8%AA%9E%22%7D"),Data=new MemoryStream(Encoding.UTF8.GetBytes(json))};
        request.Data.Position=3;
        var source=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();
        Check(Object.ReferenceEquals(source.Task,ReRiff.NewsHeaders.TraceMetadata(source.Task,request)),"original task identity");
        Check(request.Data.Position==3,"request stream position retained");
        source.SetResult(Tuple.Create(HttpStatusCode.OK,"{\"results\":["+json+"]}"));
        if(!enabled){Thread.Sleep(100);Check(!File.Exists(log),"disabled by default/config");return;}
        for(int i=0;i<300;i++){if(File.Exists(log)&&ReadLog(log).Contains("stage=response"))break;Thread.Sleep(10);}
        string output=ReadLog(log);
        Check(output.Contains("stage=request")&&output.Contains("stage=query")&&output.Contains("stage=response"),"request/query/response observed");
        Check(output.Contains("\\u304F")&&output.Contains("\\u2010")&&output.Contains("\\uD840\\uDC0B"),"exact UTF16 including surrogate pair retained");
        Check(!output.Contains("secret-sentinel")&&!output.Contains("credential-sentinel")&&!output.Contains("private-host.example"),"credentials and hostname excluded");
        long length=new FileInfo(log).Length;
        request.Uri=new Uri("https://private-host.example/login");
        ReRiff.NewsHeaders.TraceMetadata(source.Task,request);Thread.Sleep(100);
        Check(new FileInfo(log).Length==length,"authentication route excluded");
        request.Uri=new Uri("https://private-host.example/classes/Score");
        var cancelled=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();cancelled.SetCanceled();
        Check(Object.ReferenceEquals(cancelled.Task,ReRiff.NewsHeaders.TraceMetadata(cancelled.Task,request))&&cancelled.Task.IsCanceled,"cancellation retained");
        var fault=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();fault.SetException(new IOException("fault-sentinel"));
        Check(Object.ReferenceEquals(fault.Task,ReRiff.NewsHeaders.TraceMetadata(fault.Task,request))&&fault.Task.Exception!=null,"original failure retained");
        request.Data.Dispose();
        var closed=new TaskCompletionSource<Tuple<HttpStatusCode,string>>();
        Check(Object.ReferenceEquals(closed.Task,ReRiff.NewsHeaders.TraceMetadata(closed.Task,request)),"closed request stream tolerated");
        closed.SetResult(Tuple.Create(HttpStatusCode.OK,"{\"SongName\":\"closed-response-probe\"}"));
        for(int i=0;i<300;i++){if(ReadLog(log).Contains("closed-response-probe"))break;Thread.Sleep(10);}
        Check(ReadLog(log).Contains("closed-response-probe"),"response observed after request stream disposal");
        Console.WriteLine("PASS: bounded metadata-only observation ready for live test");
    }
}
