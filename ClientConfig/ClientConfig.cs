using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Net;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace ReRiff {
    public static class ClientConfig {
        private static readonly object Sync = new object();
        private static string selected;
        private static bool initialized;
        private static int networkEvents;
        [ThreadStatic] private static bool recordingException;
        private const string Disabled = "reriff-disabled://offline";

        // Called inside ParseOctane.dll before ParseClient.Initialize.
        public static string GetServer() {
            lock (Sync) {
                if (!initialized) {
                    string folder = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                    AppDomain.CurrentDomain.FirstChanceException += RecordNetworkException;
                    selected = Select(folder);
                    initialized = true;
                    StartEngineMenu();
                    bool showPopup = true;
                    try {
                        string setting;
                        if (ReadIni(Path.Combine(folder, "server.ini")).TryGetValue("show_server_popup", out setting))
                            showPopup = !setting.Equals("false", StringComparison.OrdinalIgnoreCase)
                                && setting != "0" && !setting.Equals("no", StringComparison.OrdinalIgnoreCase);
                    } catch (IOException) { }
                      catch (UnauthorizedAccessException) { }
                      catch (FormatException) { }
                    if (showPopup) ShowServerPopup(selected);
                }
                return selected;
            }
        }

        public static bool CanSend() { return GetServer() != Disabled; }

        private static void RecordNetworkException(object sender, System.Runtime.ExceptionServices.FirstChanceExceptionEventArgs args) {
            Exception error = args.Exception;
            if (recordingException || !(error is WebException || error is FileLoadException
                || error is FileNotFoundException || error is BadImageFormatException
                || error is InvalidProgramException || error is TypeLoadException
                || error is MissingMethodException || error is System.Security.SecurityException
                || error.GetType().FullName.StartsWith("Parse.", StringComparison.Ordinal))) return;
            if (System.Threading.Interlocked.Increment(ref networkEvents) > 32) return;
            recordingException = true;
            try {
                string folder = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                File.AppendAllText(LogPath(folder, "ReRiff-network.log"),
                    DateTime.UtcNow.ToString("o") + Environment.NewLine + error.ToString() + Environment.NewLine);
            } catch (Exception) {
                // Diagnostics must never change exception propagation or fail startup.
            } finally { recordingException = false; }
        }

        [DllImport("ReRiff.Notify.dll", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int ReRiffStartEngineMenu();

        private static void StartEngineMenu() {
            try {
                NativeReelBridge.Configure();
                int result=ReRiffStartEngineMenu();
                NativeReelBridge.Log("Engine menu startup result="+result+"; CLR="+Environment.Version+"; process bits="+(IntPtr.Size*8));
            }
            catch (DllNotFoundException error) { NativeReelBridge.Log(error.ToString()); }
            catch (EntryPointNotFoundException error) { NativeReelBridge.Log(error.ToString()); }
            catch (BadImageFormatException error) { NativeReelBridge.Log(error.ToString()); }
        }

        [DllImport("ReRiff.Notify.dll", CharSet = CharSet.Unicode,
                   CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void ReRiffShowServerToastW(string text);

        private static void ShowServerPopup(string endpoint) {
            try {
                ReRiffShowServerToastW(Singleplayer.IsEndpoint(endpoint) ? "Singleplayer" : endpoint);
            } catch (DllNotFoundException) { }
              catch (EntryPointNotFoundException) { }
              catch (BadImageFormatException) { }
        }

        public static Task<T> OfflineTask<T>() {
            var completion = new TaskCompletionSource<T>();
            completion.SetException(new WebException("ReRiff networking is disabled by server.ini.", WebExceptionStatus.ConnectFailure));
            return completion.Task;
        }

        public static string Select(string folder) {
            string config = Path.Combine(folder, "server.ini");
            string external = "", fallback = "";
            int timeout = 5000;
            try {
                var values = ReadIni(config);
                string enabled;
                if (values.TryGetValue("networking_enabled", out enabled)
                    && (enabled.Equals("false", StringComparison.OrdinalIgnoreCase) || enabled == "0" || enabled.Equals("no", StringComparison.OrdinalIgnoreCase))) {
                    return Local(folder, "singleplayer_selected");
                }
                values.TryGetValue("external_url", out external);
                values.TryGetValue("fallback_url", out fallback);
                string raw;
                double seconds;
                if (values.TryGetValue("timeout_seconds", out raw)) {
                    if (!double.TryParse(raw, NumberStyles.Float, CultureInfo.InvariantCulture, out seconds)
                        || seconds <= 0 || seconds > 60) throw new FormatException("Invalid timeout_seconds");
                    timeout = (int)(seconds * 1000);
                }
                external = Validate(external);
                string onlySetting;
                bool externalOnly = values.TryGetValue("external_only", out onlySetting)
                    && (onlySetting.Equals("true", StringComparison.OrdinalIgnoreCase) || onlySetting == "1" || onlySetting.Equals("yes", StringComparison.OrdinalIgnoreCase));
                fallback = externalOnly ? "" : Validate(fallback);
                string endpoint;
                string reason;
                if (external.Length > 0 && Available(external, timeout, folder)) {
                    endpoint = external; reason = "external_available";
                } else if (fallback.Length > 0 && Available(fallback, timeout, folder)) {
                    endpoint = fallback; reason = "secondary_available";
                } else {
                    return Local(folder, "servers_unavailable");
                }
                Log(folder, endpoint, reason);
                return endpoint;
            } catch (Exception e) {
                return Local(folder, "configuration_error:" + e.GetType().Name);
            }
        }

        private static string Local(string folder, string reason) {
            string endpoint = Singleplayer.Start(folder);
            Log(folder, "singleplayer", reason);
            return endpoint;
        }

        private static Dictionary<string, string> ReadIni(string path) {
            var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string section = "";
            foreach (string line in File.ReadAllLines(path)) {
                string text = line.Trim();
                if (text.Length == 0 || text.StartsWith("#") || text.StartsWith(";")) continue;
                if (text.StartsWith("[") && text.EndsWith("]")) { section = text.Substring(1, text.Length - 2).Trim(); continue; }
                if (!section.Equals("server", StringComparison.OrdinalIgnoreCase)) continue;
                int eq = text.IndexOf('=');
                if (eq < 0) throw new FormatException("Invalid server setting");
                result[text.Substring(0, eq).Trim()] = text.Substring(eq + 1).Trim();
            }
            return result;
        }

        private static string Validate(string text) {
            if (String.IsNullOrWhiteSpace(text)) return "";
            Uri uri;
            if (!Uri.TryCreate(text, UriKind.Absolute, out uri) ||
                (uri.Scheme != "http" && uri.Scheme != "https") || uri.Host.Length == 0 ||
                uri.UserInfo.Length > 0 || uri.Query.Length > 0 || uri.Fragment.Length > 0)
                throw new FormatException("Invalid server URL");
            return text.TrimEnd('/');
        }

        private static bool Available(string endpoint, int timeout, string folder) {
            try {
                var request = (HttpWebRequest)WebRequest.Create(endpoint + "/classes/Version");
                request.Timeout = timeout;
                request.ReadWriteTimeout = timeout;
                request.AllowAutoRedirect = false;
                request.Proxy = null;
                using (var response = (HttpWebResponse)request.GetResponse()) {
                    if (response.StatusCode != HttpStatusCode.OK) return false;
                    using (var reader = new StreamReader(response.GetResponseStream())) {
                        char[] buffer = new char[16384];
                        string text = new string(buffer, 0, reader.ReadBlock(buffer, 0, buffer.Length));
                        bool valid=text.Contains("\"results\"") && text.TrimStart().StartsWith("{");
                        if(valid)RememberServerIdentity(folder,endpoint,response.Headers["X-ReRiff-Server-ID"]);
                        return valid;
                    }
                }
            } catch (WebException) { return false; }
              catch (IOException) { return false; }
        }

        private static void RememberServerIdentity(string folder,string endpoint,string identity) {
            Guid value;
            if(!Guid.TryParseExact(identity,"D",out value))return; // Legacy servers remain supported.
            identity=value.ToString("D");
            try {
                string directory=Path.Combine(folder,"localserver");Directory.CreateDirectory(directory);
                string file=Path.Combine(directory,"server-identities.txt");
                string entry=identity+"\t"+endpoint;
                bool known=false,alias=false;
                if(File.Exists(file))foreach(string line in File.ReadAllLines(file)) {
                    if(line==entry)known=true;
                    if(line.StartsWith(identity+"\t",StringComparison.Ordinal)&&line!=entry)alias=true;
                }
                if(!known)File.AppendAllText(file,entry+Environment.NewLine);
                File.AppendAllText(LogPath(folder,"ReRiff-client.log"),DateTime.UtcNow.ToString("o")+
                    " server_id="+identity+" endpoint="+endpoint+" known_alias="+alias.ToString().ToLowerInvariant()+Environment.NewLine);
            } catch(IOException) { } catch(UnauthorizedAccessException) { }
        }

        internal static string LogPath(string folder,string name) {
            string logs=Path.Combine(folder,"logs");Directory.CreateDirectory(logs);
            return Path.Combine(logs,name);
        }
        private static void Log(string folder, string endpoint, string reason) {
            try {
                File.AppendAllText(LogPath(folder, "ReRiff-client.log"),
                    DateTime.UtcNow.ToString("o") + " endpoint=" + endpoint + " reason=" + reason + Environment.NewLine);
            } catch (IOException) { }
              catch (UnauthorizedAccessException) { }
        }
    }
}
