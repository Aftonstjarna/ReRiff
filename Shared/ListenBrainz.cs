using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Net;
using System.Text;
using System.Web.Script.Serialization;

namespace ReRiff {
    internal sealed class ListenBrainzConfig {
        internal string Token="";
        internal bool Enabled;
        internal int Percent=80;
        internal bool Ready {get{return Enabled&&ValidToken(Token);}}
        internal static bool ValidToken(string value) {Guid id;return value!=null&&value.Length==36&&Guid.TryParseExact(value,"D",out id)&&id!=Guid.Empty;}
        internal static ListenBrainzConfig Load(string folder) {
            var values=ConfigFiles.Read(folder,"listenbrainz");string value;var result=new ListenBrainzConfig();
            if(values.TryGetValue("user_token",out value))result.Token=LastFmConfig.ReadCredential(value);
            result.Enabled=values.TryGetValue("enabled",out value)&&(value=="1"||value.Equals("true",StringComparison.OrdinalIgnoreCase));
            int percent;if(values.TryGetValue("scrobble_percent",out value)&&Int32.TryParse(value,out percent)&&percent>=1&&percent<=100)result.Percent=percent;
            return result;
        }
    }
    internal sealed class ListenBrainzFailure:Exception {
        internal readonly int Status,WaitSeconds;
        internal bool Retry {get{return Status==0||Status==408||Status==429||Status>=500;}}
        internal ListenBrainzFailure(int status,int wait=0):base("ListenBrainz request failed") {Status=status;WaitSeconds=wait;}
    }
    internal static class ListenBrainzApi {
        internal static string Payload(LastFmTrack track,bool playing) {
            if(track==null||!track.Valid||(!playing&&track.Started<=0))throw new ArgumentException("Invalid listen metadata");
            var metadata=new Dictionary<string,object>{{"artist_name",track.Artist},{"track_name",track.Title},
                {"additional_info",new Dictionary<string,object>{{"media_player","Riff Racer"},{"submission_client","ReRiff"},{"duration_ms",(long)(track.Duration*1000)}}}};
            if(!String.IsNullOrWhiteSpace(track.Album)&&track.Album.Length<=1024)metadata["release_name"]=track.Album;
            var listen=new Dictionary<string,object>{{"track_metadata",metadata}};
            if(!playing)listen["listened_at"]=track.Started;
            return new JavaScriptSerializer().Serialize(new Dictionary<string,object>{{"listen_type",playing?"playing_now":"single"},{"payload",new[]{listen}}});
        }
        internal static int Wait(WebHeaderCollection headers) {
            int wait=1,value;
            if(Int32.TryParse(headers["Retry-After"],out value)&&value>0)wait=Math.Max(wait,value);
            if(headers["X-RateLimit-Remaining"]=="0"&&Int32.TryParse(headers["X-RateLimit-Reset-In"],out value)&&value>0)wait=Math.Max(wait,value);
            return wait;
        }
        // Returns the server's minimum delay before any further request.
        internal static int Submit(ListenBrainzConfig config,LastFmTrack track,bool playing) {
            if(!config.Ready)throw new ListenBrainzFailure(401);
            ServicePointManager.SecurityProtocol|=(SecurityProtocolType)3072;
            var request=(HttpWebRequest)WebRequest.Create("https://api.listenbrainz.org/1/submit-listens");
            request.Method="POST";request.AllowAutoRedirect=false;request.Timeout=15000;request.ReadWriteTimeout=15000;
            request.UserAgent="ReRiff/"+typeof(ListenBrainzApi).Assembly.GetName().Version+" (https://github.com/Aftonstjarna/ReRiff)";
            request.ContentType="application/json; charset=utf-8";request.Headers[HttpRequestHeader.Authorization]="Token "+config.Token;
            byte[] body=Encoding.UTF8.GetBytes(Payload(track,playing));request.ContentLength=body.Length;
            try {
                using(var stream=request.GetRequestStream())stream.Write(body,0,body.Length);
                using(var response=(HttpWebResponse)request.GetResponse()) {
                    int wait=Wait(response.Headers);
                    if(response.StatusCode!=HttpStatusCode.OK)throw new ListenBrainzFailure((int)response.StatusCode,wait);
                    // A bounded response and explicit acknowledgement avoid dropping a
                    // queued listen after a proxy returns an unrelated HTTP 200 page.
                    using(var reader=new StreamReader(response.GetResponseStream(),Encoding.UTF8)) {
                        char[] chars=new char[4097];int count=0,n;
                        while(count<chars.Length&&(n=reader.Read(chars,count,chars.Length-count))>0)count+=n;
                        if(count==chars.Length)throw new ListenBrainzFailure(0,wait);
                        Dictionary<string,object> result;
                        try{result=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(new string(chars,0,count));}catch{throw new ListenBrainzFailure(0,wait);}
                        object status;if(result==null||!result.TryGetValue("status",out status)||!Equals(status,"ok"))throw new ListenBrainzFailure(0,wait);
                    }
                    return wait;
                }
            }catch(WebException error) {
                using(var response=error.Response as HttpWebResponse) {
                    throw new ListenBrainzFailure(response==null?0:(int)response.StatusCode,response==null?0:Wait(response.Headers));
                }
            }
        }
    }
}
