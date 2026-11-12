// Shared optional-font lifecycle for Windows setup/launcher and the native Linux host.
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

internal static class LanguageComponent
{
    const string State = "localserver/languages/cjk.json";
    [DataContract]
    internal sealed class Package
    {
        [DataMember] public int schema = 1;
        [DataMember] public string version = "";
        [DataMember] public Dictionary<string, string> files = new Dictionary<string, string>();
        [DataMember] public Dictionary<string, string[]> legacy = new Dictionary<string, string[]>();
    }
    [DataContract]
    internal sealed class Installed
    {
        [DataMember] public int schema = 1;
        [DataMember] public string version = "";
        [DataMember] public Dictionary<string, string> files = new Dictionary<string, string>();
        [DataMember] public Dictionary<string, string> archives = new Dictionary<string, string>();
    }
    static T Read<T>(byte[] bytes) { using (var stream = new MemoryStream(bytes)) return (T)new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { UseSimpleDictionaryFormat = true }).ReadObject(stream); }
    static byte[] Json<T>(T value) { using (var stream = new MemoryStream()) { new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { UseSimpleDictionaryFormat = true }).WriteObject(stream, value); return stream.ToArray(); } }
    internal static string Hash(byte[] data) { using (var hash = SHA256.Create()) return string.Concat(hash.ComputeHash(data).Select(b => b.ToString("x2"))); }
    static string Under(string game, string name) { if (name.Length == 0 || name.Contains('\\') || name.Contains(':') || name.Split('/').Any(p => p == "" || p == "." || p == "..") || Path.IsPathRooted(name)) throw new IOException("Unsafe language component path."); string target = Path.GetFullPath(game); foreach (string part in name.Split('/')) { target = Path.Combine(target, part); if ((File.Exists(target) || Directory.Exists(target)) && (File.GetAttributes(target) & FileAttributes.ReparsePoint) != 0) throw new IOException("Language component paths must not be links."); } return target; }
    static bool FontPath(string name) => Regex.IsMatch(name, @"^vfs[1-9a-f][0-9a-f]?\.(dat|zip)$") || Regex.IsMatch(name, @"^fonts/noto-cjk/(jp|kr|sc|tc|hk)(64|96)?\.rrcf$") || name == "fonts/noto-cjk/OFL.txt";
    internal static bool Present(string game) => File.Exists(Under(game, State));
    internal static bool Enabled(string game)
    {
        string path = Path.Combine(game, "server.ini"); if (!File.Exists(path)) return false;
        string section = ""; bool? saved = null; bool legacy = false;
        foreach (string raw in File.ReadAllLines(path))
        {
            string line = raw.Trim(); if (line.StartsWith("[")) section = line.ToLowerInvariant(); int eq = line.IndexOf('='); if (eq < 1) continue;
            string key = line.Substring(0, eq).Trim(), value = line.Substring(eq + 1).Trim(); bool enabled = value == "1" || value.Equals("true", StringComparison.OrdinalIgnoreCase);
            if (section == "[languages]" && key.Equals("cjk_enabled", StringComparison.OrdinalIgnoreCase)) saved = enabled;
            if (section == "[diagnostics]" && key.Equals("cjk_render_probe", StringComparison.OrdinalIgnoreCase)) legacy = enabled;
        }
        return saved ?? legacy;
    }
    static byte[] Config(string game, bool enabled) { string path = Path.Combine(game, "server.ini"); var lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string> { "[server]", "external_url =", "fallback_url =", "timeout_seconds = 5", "networking_enabled = true", "show_server_popup = true", "", "[launcher]", "skip_launcher = false", "auto_update_on_skip = false" }; bool section = false, found = false; int languageAt = -1; for (int i = 0; i < lines.Count; i++) { string line = lines[i].Trim(); if (line.StartsWith("[")) section = line.Equals("[languages]", StringComparison.OrdinalIgnoreCase); if (section && line.StartsWith("[")) languageAt = i; int eq = line.IndexOf('='); if (section && eq > 0 && line.Substring(0, eq).Trim().Equals("cjk_enabled", StringComparison.OrdinalIgnoreCase)) { lines[i] = "cjk_enabled = " + (enabled ? "true" : "false"); found = true; } } if (!found) { if (languageAt >= 0) lines.Insert(languageAt + 1, "cjk_enabled = " + (enabled ? "true" : "false")); else lines.AddRange(new[] { "", "[languages]", "cjk_enabled = " + (enabled ? "true" : "false") }); } return new UTF8Encoding(false).GetBytes(string.Join("\n", lines) + "\n"); }
    static Installed Existing(string game) { var old = Read<Installed>(File.ReadAllBytes(Under(game, State))); if (old.schema != 1 || old.files == null || old.archives == null) throw new IOException("Unsupported CJK component state."); foreach (var entry in old.files) { if (!FontPath(entry.Key)) throw new IOException("Unexpected owned font path."); string path = Under(game, entry.Key); if (!File.Exists(path) || Hash(File.ReadAllBytes(path)) != entry.Value) throw new IOException("Installed CJK file changed or is missing; retained: " + entry.Key); } return old; }
    internal static Dictionary<string, byte[]> Plan(string game, string asset, string expected, bool enable)
    {
        var plan = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
        if (!enable) { plan["server.ini"] = Config(game, false); return plan; }
        if (asset == "") { if (!Present(game)) throw new IOException("Install the CJK font component first."); Existing(game); plan["server.ini"] = Config(game, true); return plan; }
        if (!Regex.IsMatch(expected, @"^[0-9a-fA-F]{64}$") || Hash(File.ReadAllBytes(asset)) != expected.ToLowerInvariant()) throw new IOException("CJK download checksum mismatch.");
        using (var zip = ZipFile.OpenRead(asset))
        {
            if (zip.Entries.Count > 64 || zip.Entries.Sum(e => e.Length) > 256L * 1024 * 1024) throw new IOException("CJK package is too large.");
            var entries = new Dictionary<string, ZipArchiveEntry>(StringComparer.OrdinalIgnoreCase);
            foreach (var entry in zip.Entries) { Under(game, entry.FullName); if (entries.ContainsKey(entry.FullName)) throw new IOException("Duplicate CJK package entry."); entries.Add(entry.FullName, entry); }
            ZipArchiveEntry metadata; if (!entries.TryGetValue("component.json", out metadata) || metadata.Length > 65536) throw new IOException("Missing CJK component manifest.");
            Func<ZipArchiveEntry, byte[]> bytes = e => { using (var input = e.Open()) using (var output = new MemoryStream()) { input.CopyTo(output); return output.ToArray(); } };
            var package = Read<Package>(bytes(metadata)); if (package.schema != 1 || package.files == null || package.legacy == null || package.files.Count + 1 != entries.Count) throw new IOException("Invalid CJK component manifest.");
            var old = Present(game) ? Existing(game) : null; var state = new Installed { version = package.version }; var used = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            // Windows mounts volume numbers case-insensitively, including padded hex suffixes.
            var occupied = new HashSet<int>();
            foreach(string path in Directory.GetFiles(game)){
                var number=Regex.Match(Path.GetFileName(path),@"^vfs([0-9a-f]{1,7})\.(dat|zip)$",RegexOptions.IgnoreCase);
                int id;if(number.Success&&int.TryParse(number.Groups[1].Value,System.Globalization.NumberStyles.HexNumber,System.Globalization.CultureInfo.InvariantCulture,out id))occupied.Add(id);
            }
            foreach (string label in new[] { "base", "hd" })
            {
                string stem = "";
                if (old != null) { old.archives.TryGetValue(label, out stem); if (string.IsNullOrEmpty(stem) || !old.files.ContainsKey(stem + ".dat") || !old.files.ContainsKey(stem + ".zip")) throw new IOException("Incomplete CJK archive ownership record."); }
                if (string.IsNullOrEmpty(stem)) foreach (var previous in package.legacy)
                    {
                        if (previous.Value.Length != 2 || !Regex.IsMatch(previous.Key, @"^vfs[1-9a-f][0-9a-f]?$")) continue; string dat = Under(game, previous.Key + ".dat"), archive = Under(game, previous.Key + ".zip"); if (!used.Contains(previous.Key) && File.Exists(dat) && File.Exists(archive) && Hash(File.ReadAllBytes(dat)) == previous.Value[0] && Hash(File.ReadAllBytes(archive)) == previous.Value[1])
                        { // Identify the old volume by its resource group.
                            using (var prior = ZipFile.OpenRead(archive)) if (prior.Entries.Any(e => e.FullName.Contains(label == "hd" ? "/jp64/" : "/jp/"))) { stem = previous.Key; break; }
                        }
                    }
                if (string.IsNullOrEmpty(stem)) for (int id = 1; id <= 31; id++) { string candidate = "vfs" + id.ToString("x"); if (!occupied.Contains(id) && !used.Contains(candidate) && !File.Exists(Under(game, candidate + ".dat")) && !File.Exists(Under(game, candidate + ".zip"))) { stem = candidate; break; } }
                if (string.IsNullOrEmpty(stem) || !Regex.IsMatch(stem, @"^vfs[1-9a-f][0-9a-f]?$") || used.Contains(stem)) throw new IOException("No unused native archive slot for CJK fonts."); used.Add(stem); state.archives[label] = stem;
            }
            foreach (var file in package.files)
            {
                ZipArchiveEntry entry; if (!entries.TryGetValue(file.Key, out entry)) throw new IOException("Missing CJK component file."); byte[] data = bytes(entry); if (Hash(data) != file.Value) throw new IOException("CJK component file checksum mismatch: " + file.Key); string target = file.Key;
                if (file.Key.StartsWith("archives/")) { var match = Regex.Match(file.Key, @"^archives/(base|hd)\.(dat|zip)$"); if (!match.Success) throw new IOException("Unexpected font archive."); target = state.archives[match.Groups[1].Value] + "." + match.Groups[2].Value; }
                if (!FontPath(target)) throw new IOException("Unexpected CJK package path."); string existing = Under(game, target);
                if (File.Exists(existing) && !target.StartsWith("vfs") && (old == null || !old.files.ContainsKey(target)) && Hash(File.ReadAllBytes(existing)) != file.Value) throw new IOException("Unowned font file retained: " + target);
                plan[target] = data; state.files[target] = file.Value;
            }
            foreach (string label in new[] { "base", "hd" }) foreach (string ext in new[] { "dat", "zip" }) if (!plan.ContainsKey(state.archives[label] + "." + ext)) throw new IOException("Incomplete font archive pair.");
            foreach (string file in new[] { "jp.rrcf", "jp64.rrcf", "jp96.rrcf", "OFL.txt" }) if (!plan.ContainsKey("fonts/noto-cjk/" + file)) throw new IOException("Incomplete CJK metrics/license.");
            plan[State] = Json(state); plan["server.ini"] = Config(game, true); return plan;
        }
    }
    static void Atomic(string path, byte[] data) { Directory.CreateDirectory(Path.GetDirectoryName(path)); string temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp"; try { File.WriteAllBytes(temp, data); if (File.Exists(path)) File.Replace(temp, path, null); else File.Move(temp, path); } finally { if (File.Exists(temp)) File.Delete(temp); } }
    internal static void Apply(string game, Dictionary<string, byte[]> plan) { var before = new Dictionary<string, byte[]>(); var changed = new List<string>(); try { foreach (var entry in plan) { string path = Under(game, entry.Key); before[entry.Key] = File.Exists(path) ? File.ReadAllBytes(path) : null; if (before[entry.Key] != null && Hash(before[entry.Key]) != Hash(entry.Value)) { string backup = Under(game, "bak/versions/" + Hash(before[entry.Key]) + "-" + Path.GetFileName(path)); if (!File.Exists(backup)) Atomic(backup, before[entry.Key]); } Atomic(path, entry.Value); changed.Add(entry.Key); } } catch { foreach (string name in changed.AsEnumerable().Reverse()) { string path = Under(game, name); if (before[name] == null) File.Delete(path); else Atomic(path, before[name]); } throw; } }
    static byte[] Download(string url, int maximum) { ServicePointManager.SecurityProtocol = (SecurityProtocolType)3072; var request = WebRequest.Create(url); request.Timeout = 30000; using (var response = request.GetResponse()) using (var input = response.GetResponseStream()) using (var output = new MemoryStream()) { byte[] block = new byte[65536]; int count; while ((count = input.Read(block, 0, block.Length)) > 0) { if (output.Length + count > maximum) throw new IOException("Language download is too large."); output.Write(block, 0, count); } return output.ToArray(); } }
    internal static int Run(string[] args)
    {
        string result = "", game = "", temporary = ""; bool ok = false; string message = ""; try
        {
            if (args.Length == 0) throw new ArgumentException("Language action required."); var options = new Dictionary<string, string>(); for (int i = 1; i < args.Length; i += 2) { if (i + 1 >= args.Length || options.ContainsKey(args[i])) throw new ArgumentException("Invalid language option."); options.Add(args[i], args[i + 1]); }
            game = Path.GetFullPath(options["--game-dir"]); if (options.ContainsKey("--result")) result = Path.GetFullPath(options["--result"]); string action = args[0];
            if (action != "status")
            {
                string asset = options.ContainsKey("--asset") ? options["--asset"] : "", expected = options.ContainsKey("--sha256") ? options["--sha256"] : ""; bool enable = action != "disable" && (!options.ContainsKey("--enable") || options["--enable"] == "true");
                if (action != "install" && action != "disable" && action != "stage") throw new ArgumentException("Unknown language action.");
                if (action == "install" && enable && asset == "" && !Present(game))
                {
                    string version = File.ReadAllText(Under(game, "localserver/client-version.txt")).Trim(); if (!Regex.IsMatch(version, @"^\d+\.\d+\.\d+(?:-beta\.\d+)?$")) throw new IOException("Invalid client version.");
                    string origin = "https://github.com/Aftonstjarna/ReRiff/releases/download/v" + version + "/"; string sums = Encoding.UTF8.GetString(Download(origin + "SHA256SUMS", 65536)); var match = Regex.Match(sums, @"(?m)^([0-9a-fA-F]{64})\s+ReRiff-CJK\.zip\s*$"); if (!match.Success) throw new IOException("This release has no verified CJK component."); expected = match.Groups[1].Value; temporary = Path.GetTempFileName(); File.WriteAllBytes(temporary, Download(origin + "ReRiff-CJK.zip", 256 * 1024 * 1024)); asset = temporary;
                }
                var plan = Plan(game, asset, expected, enable);
                if (action == "stage") { string stage = options["--stage"]; foreach (var file in plan) { string path = Under(stage, file.Key); Directory.CreateDirectory(Path.GetDirectoryName(path)); File.WriteAllBytes(path, file.Value); } }
                else Apply(game, plan);
                message = enable ? "CJK fonts installed and enabled. Restart the game to apply." : "CJK fonts disabled. Restart the game to apply. Installed fonts are retained.";
            }
            else message = Present(game) ? "CJK font component is installed." : "CJK font component is not installed.";
            ok = true;
        }
        catch (Exception error) { message = error.Message; Console.Error.WriteLine(error); if (game != "") try { string log = Under(game, "logs/ReRiff-languages.log"); Directory.CreateDirectory(Path.GetDirectoryName(log)); File.AppendAllText(log, error + "\n"); } catch (IOException) { } }
        finally { if (temporary != "" && File.Exists(temporary)) File.Delete(temporary); if (result != "") { bool present = false, enabled = false; try { if (game != "") { present = Present(game); enabled = Enabled(game); } } catch (IOException) { } string content = "[result]\nok=" + (ok ? "true" : "false") + "\ninstalled=" + (present ? "true" : "false") + "\nenabled=" + (enabled ? "true" : "false") + "\nmessage=" + message.Replace('\n', ' ').Replace('\r', ' ') + "\n"; Atomic(result, Encoding.Unicode.GetPreamble().Concat(Encoding.Unicode.GetBytes(content)).ToArray()); } }
        return ok ? 0 : 1;
    }
}
