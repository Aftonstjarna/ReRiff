// Reuses the original Parse SDK and game lifecycle. Only its storage endpoint
// changes: no replacement SDK, child process, fixed port, or external runtime.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;

namespace ReRiff {
    public sealed class Singleplayer : IDisposable {
        private static readonly object StartLock = new object();
        private static Singleplayer current;
        private readonly object storageLock = new object();
        private readonly JavaScriptSerializer json = new JavaScriptSerializer { MaxJsonLength = 64 * 1024 * 1024 };
        private readonly TcpListener listener;
        private readonly string root;
        private readonly string token = Guid.NewGuid().ToString("N");
        private volatile bool stopped;
        public string Endpoint { get; private set; }

        public static string Start(string folder) {
            lock (StartLock) {
                if (current == null) current = new Singleplayer(Path.Combine(folder, "singleplayer"));
                return current.Endpoint;
            }
        }
        public static bool IsEndpoint(string endpoint) {
            lock (StartLock) return current != null && current.Endpoint == endpoint;
        }
        // Public constructor gives storage and protocol tests an isolated game session.
        public Singleplayer(string directory) {
            root = Path.GetFullPath(directory);
            Directory.CreateDirectory(Path.Combine(root, "classes"));
            Directory.CreateDirectory(Path.Combine(root, "files"));
            ImportLegacy();
            Seed();
            listener = new TcpListener(IPAddress.Loopback, 0);
            listener.Start(16);
            Endpoint = "http://127.0.0.1:" + ((IPEndPoint)listener.LocalEndpoint).Port + "/" + token + "/";
            var thread = new Thread(Accept) { IsBackground = true, Name = "ReRiff singleplayer" };
            thread.Start();
        }
        public void Dispose() { stopped = true; listener.Stop(); }
        private void Accept() {
            while (!stopped) {
                try { var client = listener.AcceptTcpClient(); ThreadPool.QueueUserWorkItem(delegate { Serve(client); }); }
                catch (SocketException) { if (!stopped) throw; }
                catch (ObjectDisposedException) { return; }
            }
        }
        private static string ReadLine(Stream stream) {
            var bytes = new List<byte>();
            for (int n; (n = stream.ReadByte()) >= 0;) {
                if (n == 10) return Encoding.ASCII.GetString(bytes.ToArray()).TrimEnd('\r');
                if (bytes.Count >= 16384) throw new InvalidDataException("HTTP header too long");
                bytes.Add((byte)n);
            }
            throw new EndOfStreamException();
        }
        private void Serve(TcpClient client) {
            using (client) {
                client.ReceiveTimeout = client.SendTimeout = 15000;
                try {
                    Stream stream = client.GetStream();
                    string[] request = ReadLine(stream).Split(' ');
                    if (request.Length != 3) throw new InvalidDataException("Invalid HTTP request");
                    var headers = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                    int headerBytes = 0;
                    for (string line; (line = ReadLine(stream)).Length != 0;) {
                        if ((headerBytes += line.Length) > 65536) throw new InvalidDataException("Too many headers");
                        int colon = line.IndexOf(':');
                        if (colon < 1) throw new InvalidDataException("Invalid header");
                        headers[line.Substring(0, colon)] = line.Substring(colon + 1).Trim();
                    }
                    string raw;
                    int length = headers.TryGetValue("Content-Length", out raw) ? int.Parse(raw, CultureInfo.InvariantCulture) : 0;
                    if (length < 0 || length > 64 * 1024 * 1024 || headers.ContainsKey("Transfer-Encoding"))
                        throw new InvalidDataException("Unsupported request size or encoding");
                    if (headers.TryGetValue("Expect", out raw) && raw.Equals("100-continue", StringComparison.OrdinalIgnoreCase)) {
                        byte[] interim = Encoding.ASCII.GetBytes("HTTP/1.1 100 Continue\r\n\r\n"); stream.Write(interim, 0, interim.Length);
                    }
                    byte[] body = new byte[length];
                    for (int offset = 0; offset < length;) {
                        int read = stream.Read(body, offset, length - offset);
                        if (read == 0) throw new EndOfStreamException(); offset += read;
                    }
                    int status = 200; byte[] output; string contentType = "application/json";
                    lock (storageLock) {
                        try {
                            string path = request[1];
                            if (!path.StartsWith("/" + token + "/", StringComparison.Ordinal)) {
                                status = 404; output = Encode(Obj("code", 101, "error", "Not found"));
                            } else {
                                object result = Dispatch(request[0], path.Substring(token.Length + 1), body, headers);
                                output = result as byte[];
                                if (output != null) contentType = "application/octet-stream";
                                else output = Encode(result);
                            }
                        } catch (FileNotFoundException) { status = 404; output = Encode(Obj("code", 101, "error", "Object not found")); }
                          catch (Exception e) {
                            status = e is ArgumentException || e is FormatException ? 400 : 500;
                            output = Encode(Obj("code", 1, "error", "Singleplayer storage request failed: " + e.GetType().Name));
                        }
                    }
                    byte[] response = Encoding.ASCII.GetBytes("HTTP/1.1 " + status + " Response\r\nContent-Type: " + contentType
                        + "\r\nContent-Length: " + output.Length + "\r\nConnection: close\r\n\r\n");
                    stream.Write(response, 0, response.Length); stream.Write(output, 0, output.Length);
                } catch (IOException) { } catch (SocketException) { } catch (FormatException) { }
                  catch (ArgumentException) { } catch (OverflowException) { }
            }
        }
        private byte[] Encode(object value) { return Encoding.UTF8.GetBytes(json.Serialize(value)); }
        private static Dictionary<string, object> Obj(params object[] pairs) {
            var value = new Dictionary<string, object>();
            for (int i = 0; i < pairs.Length; i += 2) value[(string)pairs[i]] = pairs[i + 1];
            return value;
        }
        private static string Stamp() { return DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ", CultureInfo.InvariantCulture); }
        private static string Safe(string name) {
            if (String.IsNullOrEmpty(name) || name == "." || name == ".." || name.Any(c => !(Char.IsLetterOrDigit(c) || c == '_' || c == '-' || c == '.')))
                throw new ArgumentException("Unsafe storage name");
            return name;
        }
        private string ObjectPath(string cls, string id) { return Path.Combine(Path.Combine(Path.Combine(root, "classes"), Safe(cls)), Safe(id) + ".json"); }
        private Dictionary<string, object> Load(string path) {
            if (!File.Exists(path)) throw new FileNotFoundException();
            return json.Deserialize<Dictionary<string, object>>(File.ReadAllText(path));
        }
        private void Write(string path, byte[] data) {
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try {
                using (var file = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write)) { file.Write(data, 0, data.Length); file.Flush(true); }
                if (File.Exists(path)) File.Replace(temporary, path, null); else File.Move(temporary, path);
            } finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }
        private void Save(string cls, Dictionary<string, object> value) { Write(ObjectPath(cls, (string)value["objectId"]), Encode(value)); }
        private void ImportLegacy() {
            // Copy once, never move or overwrite the Python backend's database.
            // Its per-class JSON and file layout is also the singleplayer format.
            string marker = Path.Combine(root, "initialized");
            if (File.Exists(marker)) return;
            string legacy = Path.Combine(Path.GetDirectoryName(root), Path.Combine("localserver", Path.Combine("server", "data")));
            foreach (string category in new[] { "classes", "files" }) {
                string source = Path.Combine(legacy, category);
                if (!Directory.Exists(source)) continue;
                foreach (string file in Directory.GetFiles(source, "*", SearchOption.AllDirectories)) {
                    string relative = file.Substring(source.Length + 1);
                    string destination = Path.Combine(Path.Combine(root, category), relative);
                    if (File.Exists(destination)) continue;
                    if (category == "classes" && !file.EndsWith(".json", StringComparison.OrdinalIgnoreCase)) continue;
                    Write(destination, File.ReadAllBytes(file));
                }
            }
            Write(marker, Encoding.UTF8.GetBytes("ReRiff singleplayer storage v1\n"));
        }
        private List<Dictionary<string, object>> All(string cls) {
            string directory = Path.Combine(Path.Combine(root, "classes"), Safe(cls));
            if (!Directory.Exists(directory)) return new List<Dictionary<string, object>>();
            return Directory.GetFiles(directory, "*.json").OrderBy(p => p, StringComparer.Ordinal).Select(Load).ToList();
        }
        private void Seed() {
            SeedClass("Configuration", Obj("objectId", "ITz5K79wcQ", "TrackConversionCost", 0, "TrackCreatorCoinReward", 100,
                "OnTrackCoinValue", 10, "WatchAdCoinReward", 200, "ShareScoreCoinReward", 200, "RegionGroup", "Default"));
            SeedClass("Version", Obj("objectId", "BoJMZ1TQJZ", "Ver", 100));
            SeedClass("Profile_Steam", Obj("objectId", "SingleplayerProfile", "SystemID", "1", "Username", "PLAYER", "Platform", "PC",
                "DriverLevel", 0, "FavouriteCar", 1, "FavouriteSong", "null", "FavouriteGenre", "pop"));
            var news = Obj("objectId", "SingleplayerNews", "ID", 16, "Status", "Active", "Pinned", true);
            foreach (string language in new[] { "en", "es", "fr", "de", "it", "ru" }) {
                news[language + "Title"] = "ReRiff Singleplayer";
                news[language] = "Playing singleplayer. Scores and ghosts are saved on this PC.";
            }
            SeedClass("News", news);
        }
        private void SeedClass(string cls, Dictionary<string, object> value) {
            if (All(cls).Count != 0) return;
            value["createdAt"] = value["updatedAt"] = Stamp(); Save(cls, value);
        }
        private static Dictionary<string, string> Query(string query) {
            var result = new Dictionary<string, string>();
            foreach (string pair in query.TrimStart('?').Split('&')) {
                if (pair.Length == 0) continue;
                string[] parts = pair.Split(new[] { '=' }, 2);
                result[Uri.UnescapeDataString(parts[0].Replace('+', ' '))] = parts.Length == 2 ? Uri.UnescapeDataString(parts[1].Replace('+', ' ')) : "";
            }
            return result;
        }
        private object Dispatch(string method, string target, byte[] body, Dictionary<string, string> headers) {
            int queryIndex = target.IndexOf('?');
            var query = Query(queryIndex < 0 ? "" : target.Substring(queryIndex));
            string path = queryIndex < 0 ? target : target.Substring(0, queryIndex);
            string[] parts = path.Trim('/').Split('/').Select(Uri.UnescapeDataString).ToArray();
            var data = body.Length == 0 || parts[0] == "files" ? Obj() : json.Deserialize<Dictionary<string, object>>(Encoding.UTF8.GetString(body));
            if (parts[0] == "files" && parts.Length == 2) {
                string file = Safe(parts[1]), location = Path.Combine(Path.Combine(root, "files"), file);
                if (method == "GET") {
                    if (!File.Exists(location)) throw new FileNotFoundException();
                    return File.ReadAllBytes(location);
                }
                if (method == "POST") {
                    file = Guid.NewGuid().ToString("N") + "_" + file;
                    Write(Path.Combine(Path.Combine(root, "files"), file), body);
                    return Obj("name", file, "url", Endpoint + "files/" + file);
                }
                if (method == "DELETE") { File.Delete(location); return Obj(); }
            }
            if (path == "/login" || (path == "/users" && method == "POST")) {
                string username = path == "/login" ? query["username"] : Convert.ToString(data["username"]);
                var user = All("_User").FirstOrDefault(u => Convert.ToString(u["username"]) == username);
                if (user == null) { user = Obj("objectId", Guid.NewGuid().ToString("N"), "createdAt", Stamp(), "username", username); }
                user["sessionToken"] = "r:" + Guid.NewGuid().ToString("N"); user["updatedAt"] = Stamp(); Save("_User", user);
                return user;
            }
            if (path == "/users/me") {
                string session;
                if (!headers.TryGetValue("X-Parse-Session-Token", out session)) throw new FileNotFoundException();
                var user = All("_User").FirstOrDefault(u => u.ContainsKey("sessionToken") && Convert.ToString(u["sessionToken"]) == session);
                if (user == null) throw new FileNotFoundException(); return user;
            }
            if (path == "/logout" || parts[0] == "events" || parts[0] == "push") return Obj();
            if (path == "/batch" && method == "POST") {
                var results = new List<object>();
                foreach (Dictionary<string, object> item in (IEnumerable)data["requests"]) {
                    string batchPath = Convert.ToString(item["path"]);
                    int classes = batchPath.IndexOf("/classes/", StringComparison.Ordinal);
                    if (classes >= 0) batchPath = batchPath.Substring(classes);
                    try { results.Add(Obj("success", Dispatch(Convert.ToString(item["method"]), batchPath,
                        item.ContainsKey("body") ? Encode(item["body"]) : new byte[0], headers))); }
                    catch (Exception e) { results.Add(Obj("error", Obj("code", 1, "error", e.GetType().Name))); }
                }
                return results;
            }
            if (parts[0] == "installations") parts[0] = "classes";
            if (parts[0] == "classes" && parts.Length >= 2) {
                string cls = Safe(parts[1]);
                if (method == "GET" && parts.Length == 2) return Find(cls, query);
                if (method == "GET" && parts.Length == 3) return RefreshFiles(Load(ObjectPath(cls, parts[2])));
                if (method == "DELETE" && parts.Length == 3) { File.Delete(ObjectPath(cls, parts[2])); return Obj(); }
                if ((method == "POST" && parts.Length == 2) || (method == "PUT" && parts.Length == 3)) {
                    bool create = method == "POST";
                    var value = create ? Obj("objectId", Guid.NewGuid().ToString("N"), "createdAt", Stamp()) : Load(ObjectPath(cls, parts[2]));
                    foreach (var pair in data) {
                        if (pair.Key == "objectId" || pair.Key == "createdAt" || pair.Key == "updatedAt") continue;
                        object old; value.TryGetValue(pair.Key, out old);
                        var operation = pair.Value as Dictionary<string, object>;
                        if (operation != null && operation.ContainsKey("__op")) Apply(value, pair.Key, old, operation);
                        else value[pair.Key] = pair.Value;
                    }
                    value["updatedAt"] = Stamp(); Save(cls, value);
                    return create ? Obj("objectId", value["objectId"], "createdAt", value["createdAt"]) : Obj("updatedAt", value["updatedAt"]);
                }
            }
            throw new ArgumentException("Unsupported singleplayer request: " + method + " " + path);
        }
        private static void Apply(Dictionary<string, object> value, string key, object old, Dictionary<string, object> operation) {
            string kind = Convert.ToString(operation["__op"]);
            if (kind == "Delete") { value.Remove(key); return; }
            if (kind == "Increment") { value[key] = Convert.ToDouble(old ?? 0, CultureInfo.InvariantCulture) + Convert.ToDouble(operation["amount"], CultureInfo.InvariantCulture); return; }
            var list = old is IEnumerable && !(old is string) ? ((IEnumerable)old).Cast<object>().ToList() : new List<object>();
            foreach (object item in (IEnumerable)operation["objects"]) {
                if (kind == "Add") list.Add(item);
                else if (kind == "AddUnique") { if (!list.Any(x => Equal(x, item))) list.Add(item); }
                else if (kind == "Remove") list.RemoveAll(x => Equal(x, item));
                else throw new ArgumentException("Unsupported update operation");
            }
            value[key] = list;
        }
        private object RefreshFiles(object value) {
            var dictionary = value as Dictionary<string, object>;
            if (dictionary != null) {
                object type, name;
                if (dictionary.TryGetValue("__type", out type) && Equals(type, "File") && dictionary.TryGetValue("name", out name))
                    dictionary["url"] = Endpoint + "files/" + Uri.EscapeDataString(Safe(Convert.ToString(name)));
                else foreach (string key in dictionary.Keys.ToArray()) dictionary[key] = RefreshFiles(dictionary[key]);
            } else if (value is IEnumerable && !(value is string)) return ((IEnumerable)value).Cast<object>().Select(RefreshFiles).ToArray();
            return value;
        }
        private object Find(string cls, Dictionary<string, string> query) {
            string raw;
            var where = query.TryGetValue("where", out raw) ? json.Deserialize<Dictionary<string, object>>(raw) : Obj();
            IEnumerable<Dictionary<string, object>> matches = All(cls).Where(value => Matches(value, where));
            int count = matches.Count();
            if (query.TryGetValue("order", out raw)) {
                IOrderedEnumerable<Dictionary<string, object>> sorted = null;
                foreach (string order in raw.Split(',')) {
                    bool descending = order.StartsWith("-"); string key = descending ? order.Substring(1) : order;
                    Func<Dictionary<string, object>, object> field = value => Field(value, key);
                    var comparer = new ValueComparer();
                    sorted = sorted == null ? (descending ? matches.OrderByDescending(field, comparer) : matches.OrderBy(field, comparer))
                        : (descending ? sorted.ThenByDescending(field, comparer) : sorted.ThenBy(field, comparer));
                }
                matches = sorted;
            }
            int skip = query.TryGetValue("skip", out raw) ? Math.Max(0, int.Parse(raw, CultureInfo.InvariantCulture)) : 0;
            int limit = query.TryGetValue("limit", out raw) ? int.Parse(raw, CultureInfo.InvariantCulture) : 100;
            var results = matches.Skip(skip).Take(limit < 0 ? int.MaxValue : limit).Select(v => RefreshFiles(v)).ToArray();
            var response = Obj("results", results);
            if (query.TryGetValue("count", out raw) && (raw == "1" || raw == "true")) response["count"] = count;
            return response;
        }
        private static object Field(Dictionary<string, object> value, string key) { object result; return value.TryGetValue(key, out result) ? result : null; }
        private sealed class ValueComparer : IComparer<object> {
            public int Compare(object a, object b) { return Singleplayer.Compare(a, b); }
        }
        private static bool Numeric(object value) { return value is IConvertible && !(value is string) && !(value is bool); }
        private static int Compare(object a, object b) {
            if (a == null) return b == null ? 0 : -1; if (b == null) return 1;
            if (Numeric(a) && Numeric(b)) return Convert.ToDouble(a, CultureInfo.InvariantCulture).CompareTo(Convert.ToDouble(b, CultureInfo.InvariantCulture));
            var ad = a as Dictionary<string, object>; var bd = b as Dictionary<string, object>;
            if (ad != null && bd != null && ad.ContainsKey("iso") && bd.ContainsKey("iso")) return Compare(ad["iso"], bd["iso"]);
            return StringComparer.Ordinal.Compare(Convert.ToString(a, CultureInfo.InvariantCulture), Convert.ToString(b, CultureInfo.InvariantCulture));
        }
        private static bool Equal(object a, object b) {
            var ad = a as Dictionary<string, object>; var bd = b as Dictionary<string, object>;
            if (ad != null && bd != null) return ad.Count == bd.Count && ad.All(pair => bd.ContainsKey(pair.Key) && Equal(pair.Value, bd[pair.Key]));
            if (ad != null || bd != null) return false;
            return Compare(a, b) == 0;
        }
        private static bool Matches(Dictionary<string, object> value, Dictionary<string, object> where) {
            foreach (var pair in where) {
                if (pair.Key == "$or" || pair.Key == "$and") {
                    var terms = ((IEnumerable)pair.Value).Cast<Dictionary<string, object>>();
                    if (pair.Key == "$or" ? !terms.Any(w => Matches(value, w)) : !terms.All(w => Matches(value, w))) return false;
                    continue;
                }
                object actual = Field(value, pair.Key);
                var operations = pair.Value as Dictionary<string, object>;
                if (operations == null || !operations.Keys.Any(k => k.StartsWith("$"))) { if (!Equal(actual, pair.Value)) return false; continue; }
                foreach (var op in operations) {
                    int cmp = Compare(actual, op.Value); bool match;
                    switch (op.Key) {
                        case "$ne": match = !Equal(actual, op.Value); break;
                        case "$gt": match = cmp > 0; break; case "$gte": match = cmp >= 0; break;
                        case "$lt": match = cmp < 0; break; case "$lte": match = cmp <= 0; break;
                        case "$exists": match = value.ContainsKey(pair.Key) == Convert.ToBoolean(op.Value); break;
                        case "$in": match = ((IEnumerable)op.Value).Cast<object>().Any(x => Equal(actual, x)); break;
                        case "$nin": match = !((IEnumerable)op.Value).Cast<object>().Any(x => Equal(actual, x)); break;
                        default: throw new ArgumentException("Unsupported query operator: " + op.Key);
                    }
                    if (!match) return false;
                }
            }
            return true;
        }
    }
}
