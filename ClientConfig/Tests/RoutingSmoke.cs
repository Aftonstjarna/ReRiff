using System;
using System.IO;
using System.Net;
using System.Text;

class RoutingSmoke {
    static void Check(bool value, string message) { if (!value) throw new Exception(message); Console.WriteLine("PASS: " + message); }
    static void Main() {
        string folder = Path.Combine(Path.GetTempPath(), "reriff-routing-" + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(folder);
        try {
            using (var primary = new ReRiff.Singleplayer(Path.Combine(folder, "primary")))
            using (var secondary = new ReRiff.Singleplayer(Path.Combine(folder, "secondary"))) {
                string first = primary.Endpoint.TrimEnd('/'), second = secondary.Endpoint.TrimEnd('/');
                string ini = Path.Combine(folder, "server.ini");
                File.WriteAllText(ini, "[server]\nexternal_url=" + first + "\nfallback_url=" + second + "\n");
                Check(ReRiff.ClientConfig.Select(folder) == first, "healthy primary wins over secondary");
                File.WriteAllText(ini, "[server]\nexternal_url=http://127.0.0.1:1\nfallback_url=" + second + "\ntimeout_seconds=0.05\n");
                Check(ReRiff.ClientConfig.Select(folder) == second, "healthy secondary selected after primary fails");
                File.AppendAllText(ini, "external_only=true\n");
                Check(ReRiff.Singleplayer.IsEndpoint(ReRiff.ClientConfig.Select(folder)), "primary-only skips secondary but retains singleplayer fallback");
                foreach (string disabled in new[] { "false", "no", "0" }) {
                    File.WriteAllText(ini, "[server]\nnetworking_enabled=" + disabled + "\nexternal_url=" + first + "\nfallback_url=" + second + "\n");
                    string before = File.ReadAllText(ini);
                    Check(ReRiff.Singleplayer.IsEndpoint(ReRiff.ClientConfig.Select(folder)), "explicit singleplayer overrides reachable servers: " + disabled);
                    Check(File.ReadAllText(ini) == before, "saved server addresses retained");
                }
                File.Delete(ini);
                Check(ReRiff.Singleplayer.IsEndpoint(ReRiff.ClientConfig.Select(folder)), "fresh install without configuration starts singleplayer");
                File.WriteAllText(ini, "[server]\nfallback_url=ftp://invalid\n");
                Check(ReRiff.Singleplayer.IsEndpoint(ReRiff.ClientConfig.Select(folder)), "invalid configuration falls back to singleplayer");
                File.WriteAllText(ini, "[server]\nnetworking_enabled=false\nshow_server_popup=false\n");
            }
        } finally { Directory.Delete(folder, true); }
    }
}
