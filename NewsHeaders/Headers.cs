using System;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;

namespace ReRiff {
    // Thin transport adapter. Version comparison and News content live in C++.
    public static class NewsHeaders {
        public static void Add(WebRequest request) {
            try {
                string directory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "localserver");
                string state = Path.Combine(directory, "install-state.json");
                string version;
                if (File.Exists(state)) {
                    Match match = Regex.Match(File.ReadAllText(state),
                        "\"version\"\\s*:\\s*\"([0-9]{1,4}\\.[0-9]{1,4}\\.[0-9]{1,4})\"");
                    if (!match.Success) return;
                    version = match.Groups[1].Value;
                } else {
                    string file = Path.Combine(directory, "client-version.txt");
                    if (!File.Exists(file)) return;
                    version = File.ReadAllText(file).Trim();
                }
                if (Regex.IsMatch(version, @"\A[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}\z"))
                    request.Headers["X-ReRiff-Client-Version"] = version;
            } catch (IOException) { }
              catch (UnauthorizedAccessException) { }
        }
    }
}
