// Opt-in observation of the existing Parse transport. No body/task mutation.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace ReRiff {
    public static partial class NewsHeaders {
        private static readonly object metadataLock=new object();
        private static readonly HashSet<string> metadataSeen=new HashSet<string>();
        private static int metadataEnabled=-1,metadataRecords;
        private static bool MetadataEnabled(){
            if(metadataEnabled>=0)return metadataEnabled==1;
            bool diagnostics=false,enabled=false;
            try{
                foreach(string raw in File.ReadAllLines(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"server.ini"))){
                    string line=raw.Trim();
                    if(line.StartsWith("[",StringComparison.Ordinal))diagnostics=string.Equals(line,"[diagnostics]",StringComparison.OrdinalIgnoreCase);
                    int equals=line.IndexOf('=');
                    if(diagnostics&&equals>0&&string.Equals(line.Substring(0,equals).Trim(),"metadata_trace",StringComparison.OrdinalIgnoreCase)){
                        string value=line.Substring(equals+1).Trim();enabled=value=="1"||string.Equals(value,"true",StringComparison.OrdinalIgnoreCase);
                    }
                }
            }catch(IOException){}catch(UnauthorizedAccessException){}
            metadataEnabled=enabled?1:0;return enabled;
        }
        private static string MetadataEscape(string value){
            var output=new StringBuilder();
            foreach(char c in value){
                if(c>=32&&c<127&&c!='\\'&&c!='"')output.Append(c);
                else output.Append("\\u").Append(((int)c).ToString("X4"));
            }
            return output.ToString();
        }
        private static void MetadataWrite(string line){
            lock(metadataLock){
                if(metadataRecords>=4096||metadataSeen.Contains(line))return;
                string folder=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"logs");Directory.CreateDirectory(folder);
                string path=Path.Combine(folder,"ReRiff-metadata.log");
                if(File.Exists(path)&&new FileInfo(path).Length>=4*1024*1024)return;
                File.AppendAllText(path,DateTime.UtcNow.ToString("o")+" "+line+Environment.NewLine,Encoding.UTF8);
                metadataRecords++;if(metadataSeen.Count<512)metadataSeen.Add(line);
            }
        }
        private static bool MetadataField(string key){
            switch(key.ToLowerInvariant()){
                case "songname":case "artistname":case "albumname":case "songtitle":
                case "title":case "artist":case "album":case "genre":case "songid":return true;
                default:return false;
            }
        }
        internal static void TraceMetadataBridge(string stage,string field,byte[] bytes,string text,bool valid){
            try{
                if(!MetadataEnabled()||!MetadataField(field))return;
                string hex=bytes==null||bytes.Length==0?"":BitConverter.ToString(bytes,0,Math.Min(bytes.Length,64));
                MetadataWrite("stage="+stage+" field="+MetadataEscape(field)+" utf8_valid="+valid.ToString().ToLowerInvariant()
                    +" bytes="+hex+" text=\""+MetadataEscape(text==null?"":text.Substring(0,Math.Min(text.Length,512)))+"\"");
            }catch(IOException){}catch(UnauthorizedAccessException){}catch(System.Security.SecurityException){}
        }
        private static void MetadataWalk(object value,string stage,string path,int depth){
            if(depth>12)return;
            var map=value as IDictionary<string,object>;
            if(map!=null){
                foreach(var pair in map){
                    string text=pair.Value as string;
                    if(text!=null&&MetadataField(pair.Key)){
                        bool bounded=text.Length>512;if(bounded)text=text.Substring(0,512);
                        MetadataWrite("stage="+stage+" path="+MetadataEscape(path)+" field="+MetadataEscape(pair.Key)
                            +" text=\""+MetadataEscape(text)+"\" bounded="+bounded.ToString().ToLowerInvariant());
                    }else if(!(pair.Value is string))MetadataWalk(pair.Value,stage,path,depth+1);
                }
            }else{
                var list=value as IEnumerable;
                if(list!=null&&!(value is string)){int count=0;foreach(object item in list){if(++count>256)break;MetadataWalk(item,stage,path,depth+1);}}
            }
        }
        private static void MetadataJson(string text,string stage,string path){
            if(text==null||text.Length>1024*1024)return;
            MetadataWalk(new JavaScriptSerializer().DeserializeObject(text),stage,path,0);
        }
        private static void MetadataError(string stage,Exception error){
            try{MetadataWrite("stage=observation-error operation="+stage+" type="+error.GetType().Name);}catch(Exception){}
        }
        private static bool MetadataRoute(string path){
            return path.IndexOf("/classes/",StringComparison.Ordinal)>=0||path.EndsWith("/batch",StringComparison.Ordinal)
                ||path.EndsWith("/reriff/songs/register",StringComparison.Ordinal);
        }
        public static void ObserveMetadataRequest(object request){
            try{
                if(request==null||!MetadataEnabled())return;
                Type type=request.GetType();var uri=(Uri)type.GetProperty("Uri").GetValue(request,null);
                string path=uri.AbsolutePath;
                if(!MetadataRoute(path))return;
                MetadataWrite("stage=probe-enabled fields=song-metadata-only encoding=escaped-UTF16; no headers, hosts, complete bodies or files logged");
                var data=type.GetProperty("Data").GetValue(request,null) as MemoryStream;
                if(data!=null&&data.Length<=1024*1024){
                    try{MetadataJson(new UTF8Encoding(false,true).GetString(data.ToArray()),"request",path);}catch(ArgumentException){}
                }
                foreach(string part in uri.Query.TrimStart('?').Split('&'))if(part.StartsWith("where=",StringComparison.Ordinal)){
                    try{MetadataJson(Uri.UnescapeDataString(part.Substring(6).Replace("+"," ")),"query",path);}catch(ArgumentException){}
                }
            }catch(Exception error){MetadataError("request",error);}
        }
        public static Task<Tuple<HttpStatusCode,string>> TraceMetadata(Task<Tuple<HttpStatusCode,string>> source,object request){
            try{
                if(source==null||request==null||!MetadataEnabled())return source;
                var uri=(Uri)request.GetType().GetProperty("Uri").GetValue(request,null);
                string path=uri.AbsolutePath;if(!MetadataRoute(path))return source;
                ObserveMetadataRequest(request);
                source.ContinueWith(delegate(Task<Tuple<HttpStatusCode,string>> task){
                    try{if(task.Status==TaskStatus.RanToCompletion)MetadataJson(task.Result.Item2,"response",path);}catch(Exception error){MetadataError("response",error);}
                },CancellationToken.None,TaskContinuationOptions.None,TaskScheduler.Default);
            }catch(Exception error){MetadataError("transport",error);} // Never break the original request.
            return source;
        }
    }
}
