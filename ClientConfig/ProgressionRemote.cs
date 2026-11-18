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
    // An automatically provisioned credential is separate from the game's legacy
    // Parse login stub. Never send it to redirects, aliases or fallback URLs.
    internal sealed class ProgressionRemote : IProgressionRemote, IProgressionImport {
        private readonly string endpoint,token,serverId,accountId;
        private string password="";
        public bool ImportOriginal {get;private set;}
        private Action importFinished;
        private readonly Func<Dictionary<string,object>> observe;
        private string revision;
        private uint? maximumPrestige;
        private Dictionary<string,object> lastSnapshot;
        internal string ServerId {get{return serverId;}}
        internal string AccountId {get{return accountId;}}
        public string Revision {get{return revision;}}
        internal static bool Enabled(string folder){string value;return !ConfigFiles.Read(folder,"multiplayer_progression").TryGetValue("multiplayer_enabled",out value)||value=="1";}
        internal ProgressionRemote(string endpoint,Dictionary<string,string> settings,Func<Dictionary<string,object>> observe){
            Uri uri;
            if(!Uri.TryCreate(endpoint,UriKind.Absolute,out uri)||uri.UserInfo.Length!=0||uri.Query.Length!=0||uri.Fragment.Length!=0||
               (uri.Scheme!="https"&&uri.Scheme!="http"))throw new IOException("Invalid progression server URL");
            string configured=Setting(settings,"multiplayer_url").TrimEnd('/');
            if(!String.Equals(endpoint.TrimEnd('/'),configured,StringComparison.Ordinal))throw new IOException("Selected server differs from the progression credential URL");
            string insecure;
            if(uri.Scheme!="https"&&!uri.IsLoopback&&(!settings.TryGetValue("multiplayer_allow_http",out insecure)||insecure!="1"))throw new IOException("Remote progression credentials require HTTPS; explicit HTTP opt-in is for trusted LAN testing only");
            this.endpoint=endpoint.TrimEnd('/');this.token=Setting(settings,"multiplayer_token");
            if(token.Length!=64||token.Any(c=>!(c>='0'&&c<='9'||c>='a'&&c<='f')))throw new IOException("Invalid progression credential");
            serverId=Setting(settings,"multiplayer_server_id");Guid id;
            if(!Guid.TryParse(serverId,out id)||id.ToString()!=serverId)throw new IOException("Invalid progression server UUID");
            accountId=Setting(settings,"multiplayer_account_id");
            if(!Guid.TryParse(accountId,out id)||id.ToString()!=accountId)throw new IOException("Invalid immutable progression account UUID");
            this.observe=observe;
        }
        private static string Setting(Dictionary<string,string> values,string key){string value;if(!values.TryGetValue(key,out value)||String.IsNullOrWhiteSpace(value))throw new IOException("Missing "+key);return value.Trim();}
        private static Dictionary<string,object> Object(object value){var result=value as Dictionary<string,object>;if(result==null)throw new IOException("Invalid server object");return result;}
        private static string Text(object value){var result=value as string;if(result==null)throw new IOException("Invalid server text");return result;}
        private static ulong Number(object value){decimal number; if(value==null||value is bool||!Decimal.TryParse(Convert.ToString(value,CultureInfo.InvariantCulture),NumberStyles.None,CultureInfo.InvariantCulture,out number)||number<0||number>UInt64.MaxValue||Decimal.Truncate(number)!=number)throw new IOException("Invalid server counter");return (ulong)number;}
        private static string Hash(byte[] bytes){using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-","").ToLowerInvariant();}
        private static string HashText(object value){string text=Text(value);if(text.Length!=64||text.Any(c=>!(c>='0'&&c<='9'||c>='a'&&c<='f')))throw new IOException("Invalid server digest");return text;}
        internal sealed class HttpError : IOException {
            internal readonly int Status;
            internal HttpError(int status):base("Progression server refused the request (HTTP "+status+")."){Status=status;}
        }
        private Dictionary<string,object> Request(string route,Dictionary<string,object> payload){return Send(endpoint,route,payload,token,password);}
        private static Dictionary<string,object> Send(string endpoint,string route,Dictionary<string,object> payload,string token,string password){
            var json=new JavaScriptSerializer {MaxJsonLength=2097152,RecursionLimit=32};
            byte[] body=payload==null?null:Encoding.UTF8.GetBytes(json.Serialize(payload));
            for(int attempt=0;;attempt++){
                var request=(HttpWebRequest)WebRequest.Create(endpoint+route);
                request.AllowAutoRedirect=false;request.Timeout=5000;request.ReadWriteTimeout=5000;
                if(token!=null)request.Headers["Authorization"]="Bearer "+token;
                if(!String.IsNullOrEmpty(password))request.Headers["X-ReRiff-Server-Password"]=Convert.ToBase64String(Encoding.UTF8.GetBytes(password));
                request.Method=body==null?"GET":"POST";request.Proxy=null;
                try {
                    if(body!=null){request.ContentType="application/json";request.ContentLength=body.Length;using(var output=request.GetRequestStream())output.Write(body,0,body.Length);}
                    using(var response=(HttpWebResponse)request.GetResponse()){
                        if(response.StatusCode!=HttpStatusCode.OK)throw new HttpError((int)response.StatusCode);
                        using(var stream=response.GetResponseStream())using(var output=new MemoryStream()){
                            byte[] buffer=new byte[8192];int count;
                            while((count=stream.Read(buffer,0,buffer.Length))>0){if(output.Length+count>2097152)throw new IOException("Progression response too large");output.Write(buffer,0,count);}
                            return Object(json.DeserializeObject(Encoding.UTF8.GetString(output.ToArray())));
                        }
                    }
                }catch(WebException error){
                    if(error.Response!=null){int status=(int)((HttpWebResponse)error.Response).StatusCode;error.Response.Close();throw new HttpError(status);}
                    if(attempt>=1)throw new IOException("Progression server unavailable. Reconnect to recover its committed save.",error);
                    // Retry identical operation ID/content, including pending account creation.
                }
            }
        }
        private static string Uuid(object value){string text=Text(value);Guid id;if(!Guid.TryParse(text,out id)||id.ToString()!=text)throw new IOException("Invalid server/account UUID");return text;}
        internal static ProgressionRemote Connect(string folder,string endpoint,Func<Dictionary<string,object>> observe,Func<string,bool,string> prompt){
            var settings=ConfigFiles.Read(folder,"multiplayer_progression");
            endpoint=endpoint.TrimEnd('/');Uri uri;
            if(!Uri.TryCreate(endpoint,UriKind.Absolute,out uri)||uri.UserInfo.Length!=0||uri.Query.Length!=0||uri.Fragment.Length!=0||
               (uri.Scheme!="https"&&uri.Scheme!="http"))throw new IOException("Invalid progression server URL");
            string directory=Path.Combine(folder,"localserver/accounts");
            string file=Path.Combine(directory,Hash(Encoding.UTF8.GetBytes(endpoint))+".json");
            Dictionary<string,object> info;
            try{info=Send(endpoint,"/reriff/progression/info",null,null,null);}
            catch(HttpError error){
                // Legacy servers have no account discovery. Never silently abandon
                // a known account if its server disables/removes progression.
                if(error.Status==404&&!File.Exists(file)&&!settings.ContainsKey("multiplayer_token")){
                    Dictionary<string,object> advertised=null;
                    try{advertised=Send(endpoint,"/reriff/balance",null,null,null);}catch(HttpError legacy){if(legacy.Status!=404)throw;}
                    object checkpoints;
                    if(advertised!=null&&Object(advertised["capabilities"]).TryGetValue("serverCheckpoints",out checkpoints)&&System.Object.Equals(checkpoints,true))throw new IOException("Update this server to support automatic accounts before connecting.");
                    return null;
                }
                throw;
            }
            string server=Uuid(info["serverId"]);
            if(Number(info["protocol"])!=1||!System.Object.Equals(info["automaticAccounts"],true))throw new IOException("Unsupported account discovery protocol");
            string insecure;
            if(uri.Scheme!="https"&&!uri.IsLoopback&&(!settings.TryGetValue("multiplayer_allow_http",out insecure)||insecure!="1"))throw new IOException("This HTTP server requires multiplayer_allow_http=1 for trusted LAN testing.");
            Directory.CreateDirectory(directory);
            // Hold across disk/network operations: two local processes must not
            // allocate separate identities before either persists its response.
            using(var guard=new FileStream(file+".lock",FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None)){
                var json=new JavaScriptSerializer {MaxJsonLength=65536,RecursionLimit=8};
                Dictionary<string,object> account;
                if(File.Exists(file)){
                    if(new FileInfo(file).Length>65536)throw new IOException("Invalid saved account");
                    account=Object(json.DeserializeObject(File.ReadAllText(file)));
                    if(Text(account["url"])!=endpoint||Uuid(account["serverId"])!=server)throw new IOException("Server identity changed. Restore or verify the server before reconnecting.");
                    if(account["accountId"]!=null)Uuid(account["accountId"]);
                    HashText(account["token"]);
                }else{
                    string token,oldUrl;
                    object oldAccount=null;
                    if(settings.TryGetValue("multiplayer_token",out token)){
                        if(!settings.TryGetValue("multiplayer_url",out oldUrl)||oldUrl.TrimEnd('/')!=endpoint||Setting(settings,"multiplayer_server_id")!=server)throw new IOException("Existing progression credential belongs to another server");
                        oldAccount=Uuid(Setting(settings,"multiplayer_account_id"));HashText(token);
                    }else{
                        byte[] bytes=new byte[32];using(var rng=RandomNumberGenerator.Create())rng.GetBytes(bytes);
                        token=BitConverter.ToString(bytes).Replace("-","").ToLowerInvariant();
                    }
                    account=new Dictionary<string,object>{{"url",endpoint},{"serverId",server},{"accountId",oldAccount},{"token",token}};
                    SaveAccount(file,json.Serialize(account)); // Must survive a lost first response.
                }
                bool locked=System.Object.Equals(info["passwordRequired"],true);
                string password="";Dictionary<string,object> joined=null;
                for(int attempt=0;attempt<3;attempt++){
                    if(locked){password=prompt(endpoint,attempt>0);if(password==null)throw new OperationCanceledException("Server connection cancelled");}
                    try{joined=Send(endpoint,"/reriff/progression/connect",new Dictionary<string,object>{{"create",account["accountId"]==null}},Text(account["token"]),password);break;}
                    catch(HttpError error){if(error.Status!=403||attempt==2)throw;locked=true;}
                }
                string id=Uuid(joined["accountId"]);
                if(Number(joined["protocol"])!=1||Uuid(joined["serverId"])!=server||(account["accountId"]!=null&&Text(account["accountId"])!=id))throw new IOException("Progression account identity changed");
                account["accountId"]=id;SaveAccount(file,json.Serialize(account));
                settings["multiplayer_url"]=endpoint;settings["multiplayer_server_id"]=server;settings["multiplayer_account_id"]=id;settings["multiplayer_token"]=Text(account["token"]);
                var remote=new ProgressionRemote(endpoint,settings,observe);remote.password=password;
                object import;remote.ImportOriginal=account.TryGetValue("importOriginalOnce",out import)&&System.Object.Equals(import,true);
                remote.importFinished=()=>{
                    using(var importGuard=new FileStream(file+".lock",FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None)){
                        var current=Object(json.DeserializeObject(File.ReadAllText(file)));
                        if(Text(current["serverId"])!=server||Text(current["accountId"])!=id||Text(current["token"])!=remote.token)throw new IOException("Account cache changed during import");
                        current.Remove("importOriginalOnce");SaveAccount(file,json.Serialize(current));
                    }
                };
                return remote;
            }
        }
        private static void SaveAccount(string path,string value){
            string temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try{
                byte[] bytes=Encoding.UTF8.GetBytes(value);
                using(var output=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){output.Write(bytes,0,bytes.Length);output.Flush(true);}
                if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);
            }finally{if(File.Exists(temp))File.Delete(temp);}
        }
        // Startup runs before the game's menu/input lifecycle. Reuse Windows'
        // credential dialog (masked entry, DPI/layout, Enter/Escape), not a game menu.
        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)]
        private struct CredentialInfo {internal int size;internal IntPtr owner;internal string message,caption;internal IntPtr banner;}
        [DllImport("credui.dll",CharSet=CharSet.Unicode,ExactSpelling=true)]
        private static extern int CredUIPromptForCredentialsW(ref CredentialInfo info,string target,IntPtr reserved,int error,StringBuilder user,int userSize,StringBuilder password,int passwordSize,ref int save,int flags);
        internal static string PromptPassword(string endpoint,bool retry){
            var info=new CredentialInfo {message=(retry?"Incorrect password. ":"")+"Enter the password for "+endpoint,caption="ReRiff server password"};
            info.size=Marshal.SizeOf(typeof(CredentialInfo));
            var user=new StringBuilder("Player",128);var password=new StringBuilder(256);int save=0;
            int result=CredUIPromptForCredentialsW(ref info,endpoint,IntPtr.Zero,0,user,user.Capacity,password,password.Capacity,ref save,0x40000|0x80|0x2|0x100000);
            if(result==1223)return null;
            if(result!=0)throw new IOException("The server password prompt could not open ("+result+").");
            string value=password.ToString();password.Clear();return value;
        }
        private ProgressionStore.Checkpoint Decode(Dictionary<string,object> response,bool load){
            if(Number(response["protocol"])!=1||Text(response["serverId"])!=serverId||Text(response["accountId"])!=accountId)throw new IOException("Progression server/account identity changed");
            var capabilities=Object(response["capabilities"]);
            if(!System.Object.Equals(capabilities["serverCheckpoints"],true)||!System.Object.Equals(capabilities["prestigeReset"],true)||!System.Object.Equals(capabilities["trustedClientProgression"],true))throw new IOException("Server does not support trusted progression");
            object importCapability;
            if(ImportOriginal&&response["checkpoint"]==null&&(!capabilities.TryGetValue("initialSaveImport",out importCapability)||!System.Object.Equals(importCapability,true)))throw new IOException("Update the test server before importing the original save.");
            string nextRevision=HashText(response["balanceRevision"]);
            if(!load&&nextRevision!=revision)throw new IOException("Server balance changed during save; reconnect");
            if(load){ProgressionBalance.Current=ReadBalance(Object(response["balance"]));revision=nextRevision;}
            if(response["checkpoint"]==null){if(!load)throw new IOException("Server did not commit progression");lastSnapshot=null;return null;}
            var value=Object(response["checkpoint"]);byte[] native=Convert.FromBase64String(Text(value["nativeSave"]));
            if(native.Length==0||native.Length>1048576||Hash(native)!=HashText(value["nativeSaveHash"]))throw new IOException("Server native save checksum differs");
            ulong generation=Number(value["generation"]);if(generation==0)throw new IOException("Invalid server generation");
            var state=new ProgressionStore.State(generation,Number(value["mainXp"]),checked((int)Number(value["cycleXp"])),checked((uint)Number(value["prestigeCount"])),HashText(value["balanceRevision"]));
            lastSnapshot=Object(value["snapshot"]);
            if(Number(lastSnapshot["cycleXp"])!=(ulong)state.CycleXp)throw new IOException("Server checkpoint XP differs");
            return new ProgressionStore.Checkpoint(state,native);
        }
        private ProgressionBalance ReadBalance(Dictionary<string,object> value){
            var coins=Object(value["coins"]);var discounts=Object(value["discounts"]);var levels=Object(value["main_levels"]);var unlocks=Object(value["unlocks"]);var xp=Object(value["xp"]);var prestige=Object(value["prestige"]);
            if(Number(value["schema"])!=1||Text(value["anti_cheat"])!="disabled"||Text(xp["main_rate"])!="1"||Text(xp["prestige_rate"])!="1"||Object(unlocks["car_levels"]).Count!=0||Text(prestige["required_achievements"])!="native_all")throw new IOException("Unsupported server progression policy");
            maximumPrestige=prestige["maximum_count"]==null?(uint?)null:checked((uint)Number(prestige["maximum_count"]));
            var settings=new Dictionary<string,string>();
            Action<string,object> add=(key,item)=>settings.Add(key,Convert.ToString(item,CultureInfo.InvariantCulture));
            add("coin_reward_rate",coins["base_rate"]);add("creator_reward_rate",coins["creator_rate"]);add("prestige_multiplier_growth",coins["prestige_growth"]);add("prestige_multiplier_cap",coins["multiplier_cap"]??"0");add("paint_price_scale",unlocks["paint_price_scale"]);
            add("discount_mode",discounts["mode"]);add("discount_start_level",discounts["start_level"]);add("discount_start",discounts["start"]);add("discount_increase",discounts["increase"]);add("discount_maximum",discounts["maximum"]);add("discount_interval",discounts["diminishing_interval"]);add("discount_remaining_gap_factor",discounts["remaining_gap_factor"]);
            var milestones=(object[])discounts["milestones"];
            if(milestones.Length!=3)throw new IOException("Unsupported discount milestones");
            for(int i=0;i<3;i++){var milestone=Object(milestones[i]);if(Number(milestone["level"])!=(ulong)((i+1)*10))throw new IOException("Unsupported discount milestone");add("discount_level_"+((i+1)*10),milestone["fraction"]);}
            add("main_level_cap",levels["cap"]);add("main_level_xp_step",levels["xp_step"]);add("main_level_xp_growth",levels["xp_growth"]);
            return ProgressionBalance.Parse(settings);
        }
        private void FinishImport(){if(importFinished!=null)importFinished();ImportOriginal=false;}
        public ProgressionStore.Checkpoint Load(){var value=Decode(Request("/reriff/progression",null),true);if(value!=null&&ImportOriginal)FinishImport();return value;}
        public ProgressionStore.Checkpoint Import(byte[] native,int cycleXp){
            if(!ImportOriginal)throw new IOException("Original save import was not requested");
            var result=CommitTo(native,cycleXp,0,"import");
            if(result.Progress.MainXp!=(ulong)cycleXp||result.Progress.PrestigeCount!=0)throw new IOException("Unexpected imported progression counters");
            FinishImport();return result;
        }
        public void PreparePrestige(ulong generation){
            var current=Decode(Request("/reriff/progression",null),false);
            if(current.Progress.Generation!=generation||!CanPrestige(current.Progress))throw new IOException("Server progression changed before prestige");
            var achievements=(object[])lastSnapshot["achievements"];
            var earned=new HashSet<ulong>(achievements.Select(Number));
            if(Enumerable.Range(0,39).Where(i=>i!=18).Any(i=>!earned.Contains((ulong)i)))throw new IOException("Server checkpoint is not eligible for prestige");
        }
        public bool CanPrestige(ProgressionStore.State state){return !maximumPrestige.HasValue||state.PrestigeCount<maximumPrestige.Value;}
        public ProgressionStore.Checkpoint Commit(byte[] native,int cycleXp,ulong generation,bool prestige){return CommitTo(native,cycleXp,generation,prestige?"prestige":"checkpoint");}
        private ProgressionStore.Checkpoint CommitTo(byte[] native,int cycleXp,ulong generation,string action){
            var observed=observe();
            if(Number(observed["cycleXp"])!=(ulong)cycleXp)throw new IOException("Native checkpoint XP changed during save");
            var payload=new Dictionary<string,object>{{"operationId",Guid.NewGuid().ToString()},{"expectedGeneration",generation},{"balanceRevision",revision},{"nativeSave",Convert.ToBase64String(native)},{"snapshot",observed}};
            var result=Decode(Request("/reriff/progression/"+action,payload),false);
            if(result.Progress.Generation!=checked(generation+1)||result.Progress.CycleXp!=cycleXp||!result.NativeSave.SequenceEqual(native))throw new IOException("Server committed an unexpected checkpoint");
            return result;
        }
    }
}
