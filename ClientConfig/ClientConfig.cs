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
        private const string Disabled = "reriff-disabled://offline";

        // Called inside ParseOctane.dll before ParseClient.Initialize.
        public static string GetServer() {
            lock (Sync) {
                if (!initialized) {
                    string folder = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                    selected = Select(folder);
                    initialized = true;
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

        [DllImport("ReRiff.Notify.dll", CharSet = CharSet.Unicode,
                   CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void ReRiffShowServerToastW(string text);

        private static void ShowServerPopup(string endpoint) {
            try {
                ReRiffShowServerToastW(endpoint == Disabled ? "Networking disabled" : endpoint);
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
                if (external.Length > 0 && Available(external, timeout)) {
                    endpoint = external; reason = "external_available";
                } else if (fallback.Length > 0) {
                    endpoint = fallback; reason = "configured_fallback";
                } else {
                    endpoint = Disabled; reason = externalOnly ? "external_only_unavailable" : "no_available_configured_address";
                }
                Log(folder, endpoint, reason);
                return endpoint;
            } catch (Exception e) {
                Log(folder, Disabled, "configuration_error:" + e.GetType().Name);
                return Disabled;
            }
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

        private static bool Available(string endpoint, int timeout) {
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
                        return text.Contains("\"results\"") && text.TrimStart().StartsWith("{");
                    }
                }
            } catch (WebException) { return false; }
              catch (IOException) { return false; }
        }

        private static void Log(string folder, string endpoint, string reason) {
            try {
                File.AppendAllText(Path.Combine(folder, "ReRiff-client.log"),
                    DateTime.UtcNow.ToString("o") + " endpoint=" + endpoint + " reason=" + reason + Environment.NewLine);
            } catch (IOException) { }
              catch (UnauthorizedAccessException) { }
        }
    }
}
