using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Web.Script.Serialization;

namespace ReRiff {
    internal sealed class LastFmConfig {
        [StructLayout(LayoutKind.Sequential)] private struct Blob {internal int Size;internal IntPtr Data;}
        [DllImport("crypt32.dll",SetLastError=true)] private static extern bool CryptUnprotectData(ref Blob input,IntPtr description,IntPtr entropy,IntPtr reserved,IntPtr prompt,int flags,out Blob output);
        [DllImport("kernel32.dll")] private static extern IntPtr LocalFree(IntPtr memory);
        internal static string ReadCredential(string value){
            if(!value.StartsWith("dpapi:",StringComparison.Ordinal))return value;
            Blob input=new Blob(),output=new Blob();
            try{
                string hex=value.Substring(6);if(hex.Length==0||hex.Length%2!=0||hex.Length>4000)return "";
                byte[] bytes=new byte[hex.Length/2];for(int i=0;i<bytes.Length;i++)bytes[i]=Convert.ToByte(hex.Substring(i*2,2),16);
                input.Size=bytes.Length;input.Data=Marshal.AllocHGlobal(bytes.Length);Marshal.Copy(bytes,0,input.Data,bytes.Length);
                if(!CryptUnprotectData(ref input,IntPtr.Zero,IntPtr.Zero,IntPtr.Zero,IntPtr.Zero,1,out output)||output.Size<=0||output.Size>=64)return "";
                bytes=new byte[output.Size];Marshal.Copy(output.Data,bytes,0,bytes.Length);string result=Encoding.UTF8.GetString(bytes);Array.Clear(bytes,0,bytes.Length);return result;
            }catch{return "";}finally{
                if(input.Data!=IntPtr.Zero)Marshal.FreeHGlobal(input.Data);
                if(output.Data!=IntPtr.Zero){for(int i=0;i<output.Size;i++)Marshal.WriteByte(output.Data,i,0);LocalFree(output.Data);}
            }
        }
        internal string Key, Secret, Session, User;
        internal bool Enabled;
        internal int Percent=80;
        internal bool Credentials {get{return ValidKey(Key)&&ValidKey(Secret);}}
        internal bool Ready {get{return Enabled&&Credentials&&ValidToken(Session)&&!String.IsNullOrEmpty(User);}}
        // Authentication tokens/session identifiers are opaque ASCII, not MD5 hex.
        internal static bool ValidToken(string value){return value!=null&&value.Length==32&&value.All(c=>c<128&&(Char.IsLetterOrDigit(c)||c=='-'||c=='_'));}
        internal static bool ValidKey(string value){return value!=null&&value.Length==32&&value.All(c=>Uri.IsHexDigit(c));}
        internal static LastFmConfig Load(string folder){
            var d=ConfigFiles.Read(folder,"lastfm");string v;var c=new LastFmConfig();
            c.Key=d.TryGetValue("api_key",out v)?v:"";c.Secret=d.TryGetValue("shared_secret",out v)?v:"";
            c.Session=d.TryGetValue("session_key",out v)?v:"";c.User=d.TryGetValue("username",out v)?v:"";
            c.Key=ReadCredential(c.Key);c.Secret=ReadCredential(c.Secret);c.Session=ReadCredential(c.Session);
            c.Enabled=d.TryGetValue("enabled",out v)&&(v=="1"||v.Equals("true",StringComparison.OrdinalIgnoreCase));
            int p;if(d.TryGetValue("scrobble_percent",out v)&&Int32.TryParse(v,out p)&&p>=1&&p<=100)c.Percent=p;
            return c;
        }
        // Update only Last.fm keys; never rewrite unrelated settings from a stale snapshot.
        internal static void Save(string folder,IDictionary<string,string> values){
            string path=Path.Combine(folder,"client.ini");var lines=File.Exists(path)?File.ReadAllLines(path).ToList():new List<string>();
            foreach(var pair in values){
                if(pair.Value.IndexOfAny(new[]{'\r','\n'})>=0)throw new InvalidDataException();
                bool active=false;
                for(int i=0;i<lines.Count;i++){
                    string line=lines[i].Trim();
                    if(line.StartsWith("[")&&line.EndsWith("]"))active=line.Equals("[lastfm]",StringComparison.OrdinalIgnoreCase);
                    else if(active){int eq=line.IndexOf('=');if(eq>0&&line.Substring(0,eq).Trim().Equals(pair.Key,StringComparison.OrdinalIgnoreCase))lines.RemoveAt(i--);}
                }
                int section=lines.FindIndex(line=>line.Trim().Equals("[lastfm]",StringComparison.OrdinalIgnoreCase));
                if(section<0){lines.Add("");lines.Add("[lastfm]");section=lines.Count-1;}
                int end=section+1;while(end<lines.Count&&!lines[end].TrimStart().StartsWith("["))end++;
                lines.Insert(end,pair.Key+" = "+pair.Value);
            }
            ConfigFiles.Atomic(path,String.Join(Environment.NewLine,lines)+Environment.NewLine);
        }
    }
    internal sealed class LastFmFailure:Exception {
        internal readonly int Code;
        internal LastFmFailure(int code):base("Last.fm request failed (code "+code.ToString(CultureInfo.InvariantCulture)+")."){Code=code;}
        internal bool Retry {get{return Code==0||Code==8||Code==11||Code==16||Code==29;}}
    }
    internal static class LastFmApi {
        internal static string Signature(IDictionary<string,string> values,string secret){
            var input=new StringBuilder();foreach(var p in values.OrderBy(p=>p.Key,StringComparer.Ordinal))if(p.Key!="format"&&p.Key!="callback"&&p.Key!="api_sig")input.Append(p.Key).Append(p.Value);
            input.Append(secret);using(var md5=MD5.Create())return BitConverter.ToString(md5.ComputeHash(Encoding.UTF8.GetBytes(input.ToString()))).Replace("-","").ToLowerInvariant();
        }
        internal static string Encode(IDictionary<string,string> values){return String.Join("&",values.Select(p=>Uri.EscapeDataString(p.Key)+"="+Uri.EscapeDataString(p.Value)).ToArray());}
        internal static Dictionary<string,object> Call(LastFmConfig config,string method,IDictionary<string,string> args){
            var p=new Dictionary<string,string>(args);p["method"]=method;p["api_key"]=config.Key;
            p["api_sig"]=Signature(p,config.Secret);p["format"]="json";
            // net40 runs on the installed CLR4 runtime. Require TLS1.2; never downgrade or ignore certificate errors.
            ServicePointManager.SecurityProtocol|=(SecurityProtocolType)3072;
            var request=(HttpWebRequest)WebRequest.Create("https://ws.audioscrobbler.com/2.0/");
            request.Method="POST";request.AllowAutoRedirect=false;request.Timeout=15000;request.ReadWriteTimeout=15000;
            request.UserAgent="ReRiff/LastFm";request.ContentType="application/x-www-form-urlencoded; charset=utf-8";
            byte[] body=Encoding.UTF8.GetBytes(Encode(p));request.ContentLength=body.Length;
            try {
                using(var stream=request.GetRequestStream())stream.Write(body,0,body.Length);
                using(var response=(HttpWebResponse)request.GetResponse())return Parse(response);
            }catch(WebException error){
                using(var response=error.Response as HttpWebResponse){if(response!=null){try{return Parse(response);}catch(LastFmFailure){throw;}catch{}}}
                throw new LastFmFailure(0);
            }
        }
        private static Dictionary<string,object> Parse(HttpWebResponse response){
            using(var reader=new StreamReader(response.GetResponseStream(),Encoding.UTF8)){
                var text=new StringBuilder();char[] buffer=new char[4096];int n;
                while((n=reader.Read(buffer,0,buffer.Length))>0){text.Append(buffer,0,n);if(text.Length>65536)throw new LastFmFailure(0);}
                Dictionary<string,object> json;try{json=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(text.ToString());}catch{throw new LastFmFailure(0);}
                object code;if(json==null)throw new LastFmFailure(0);if(json.TryGetValue("error",out code))throw new LastFmFailure(Convert.ToInt32(code,CultureInfo.InvariantCulture));
                if((int)response.StatusCode!=200)throw new LastFmFailure(0);return json;
            }
        }
        internal static Dictionary<string,object> Object(Dictionary<string,object> parent,string key){object v;var result=parent.TryGetValue(key,out v)?v as Dictionary<string,object>:null;if(result==null)throw new LastFmFailure(0);return result;}
        internal static string Text(Dictionary<string,object> parent,string key){object v;return parent.TryGetValue(key,out v)?Convert.ToString(v,CultureInfo.InvariantCulture):"";}
    }
    internal sealed class LastFmTrack {
        public string Artist {get;set;} public string Title {get;set;} public string Album {get;set;}
        public double Duration {get;set;} public long Started {get;set;}
        internal bool Valid {get{return Good(Artist)&&Good(Title)&&!Double.IsNaN(Duration)&&!Double.IsInfinity(Duration)&&Duration>30&&Duration<=86400;}}
        private static bool Good(string s){return !String.IsNullOrWhiteSpace(s)&&s.Length<=1024&&!s.Any(Char.IsControl)&&!s.Trim().Equals("unknown",StringComparison.OrdinalIgnoreCase)&&!s.Trim().Equals("unknown artist",StringComparison.OrdinalIgnoreCase);}
        internal Dictionary<string,string> Parameters(string session,bool scrobble){
            var p=new Dictionary<string,string>{{"sk",session},{"artist",Artist},{"track",Title},{"duration",((int)Duration).ToString(CultureInfo.InvariantCulture)}};
            if(!String.IsNullOrWhiteSpace(Album)&&Album.Length<=1024)p["album"]=Album;
            if(scrobble){p["timestamp"]=Started.ToString(CultureInfo.InvariantCulture);p["chosenByUser"]="1";}return p;
        }
    }
    // Pure playback policy, driven only by snapshots taken on the game's thread.
    internal sealed class LastFmPlayback {
        internal LastFmTrack Track; internal double Listened;
        private double position,wall;private bool playing,sent;private readonly int percent;
        internal LastFmPlayback(int percent){this.percent=Math.Max(1,Math.Min(100,percent));}
        internal void Start(LastFmTrack track,double position,double wall){Track=track;Listened=0;this.position=position;this.wall=wall;playing=false;sent=false;}
        internal bool Observe(double next,double now,bool active,bool completed){
            double elapsed=now-wall,delta=next-position;
            if(active&&playing&&elapsed>0&&elapsed<=2&&delta>=0&&delta<=elapsed+0.5)Listened+=Math.Min(delta,elapsed);
            position=next;wall=now;playing=active;
            if(sent||Track==null||!Track.Valid)return false;
            if(Listened<Math.Min(Track.Duration/2,240))return false;
            if(!completed&&Listened<Track.Duration*percent/100)return false;
            sent=true;return true;
        }
    }
}
