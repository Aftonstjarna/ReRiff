using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;

void Check(bool condition, string name) {
    if (!condition) throw new Exception(name);
    Console.WriteLine("PASS: " + name);
}
var folder = AppContext.BaseDirectory;
Directory.CreateDirectory(Path.Combine(folder, "localserver"));
var versionFile = Path.Combine(folder, "localserver/client-version.txt");
try {
    foreach (var version in new[] { "0.1.8", "0.10.0" }) {
        File.WriteAllText(versionFile, version + "\n");
        var request = WebRequest.Create("http://127.0.0.1/");
        ReRiff.NewsHeaders.Add(request);
        Check(request.Headers["X-ReRiff-Client-Version"] == version, "valid client version header " + version);
    }
    foreach (var version in new[] { "bad", "0.1.8\r\nInjected: yes", "99999.0.0" }) {
        File.WriteAllText(versionFile, version);
        var request = WebRequest.Create("http://127.0.0.1/");
        ReRiff.NewsHeaders.Add(request);
        Check(request.Headers["X-ReRiff-Client-Version"] == null, "invalid version omitted");
    }
    File.Delete(versionFile);
    ReRiff.NewsHeaders.Add(WebRequest.Create("http://127.0.0.1/"));
    Console.WriteLine("PASS: missing version file leaves request usable");
    var installState = Path.Combine(folder, "localserver/install-state.json");
    File.WriteAllText(versionFile, "0.1.8");
    File.WriteAllText(installState, "{\"version\":\"0.1.9\"}");
    var updated = WebRequest.Create("http://127.0.0.1/");
    ReRiff.NewsHeaders.Add(updated);
    Check(updated.Headers["X-ReRiff-Client-Version"] == "0.1.9", "installer version supersedes stale version file after update");
    File.Delete(installState);
    var listener = new TcpListener(IPAddress.Loopback, 0); listener.Start();
    try {
        int port = ((IPEndPoint)listener.LocalEndpoint).Port;
        File.WriteAllText(versionFile, "0.1.8");
        File.WriteAllText(Path.Combine(folder, "server.ini"), "[server]\nfallback_url=http://127.0.0.1:" + port + "\nshow_server_popup=false\n");
        var parse = Assembly.LoadFrom(Path.Combine(folder, "Parse.dll"));
        var transport = Assembly.LoadFrom(Path.Combine(folder, "Parse.NetFx45.dll"));
        var requestType = parse.GetType("Parse.Internal.HttpRequest", true);
        var request = Activator.CreateInstance(requestType, true);
        requestType.GetProperty("Uri").SetValue(request, new Uri("http://127.0.0.1:" + port + "/classes/News"));
        requestType.GetProperty("Method").SetValue(request, "GET");
        requestType.GetProperty("Headers").SetValue(request, new List<KeyValuePair<string,string>> { new("X-Preserved", "yes") });
        var received = Task.Run(async () => {
            using var connection = await listener.AcceptTcpClientAsync();
            using var stream = connection.GetStream();
            using var reader = new StreamReader(stream, Encoding.ASCII, false, 4096, true);
            var headers = new StringBuilder();
            while (true) { var line = await reader.ReadLineAsync(); if (string.IsNullOrEmpty(line)) break; headers.AppendLine(line); }
            byte[] response = Encoding.ASCII.GetBytes("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 14\r\nConnection: close\r\n\r\n{\"results\":[]}");
            await stream.WriteAsync(response);
            return headers.ToString();
        });
        var type = transport.GetType("Parse.Internal.HttpClient", true);
        var task = (Task)type.GetMethod("ExecuteAsync").Invoke(Activator.CreateInstance(type, true), new object[] { request, null, null, CancellationToken.None });
        await task.WaitAsync(TimeSpan.FromSeconds(15));
        string wire = await received.WaitAsync(TimeSpan.FromSeconds(15));
        Check(wire.Contains("X-ReRiff-Client-Version: 0.1.8", StringComparison.OrdinalIgnoreCase), "actual patched Parse transport reports client version");
        Check(wire.Contains("X-Preserved: yes", StringComparison.OrdinalIgnoreCase), "existing request headers preserved");
        Check(wire.Contains("Cache-Control: no-cache", StringComparison.OrdinalIgnoreCase), "original cache header preserved");
        File.WriteAllText(Path.Combine(folder, "server.ini"), "[server]\nexternal_url=\nfallback_url=\nshow_server_popup=false\n");
        var config = AppDomain.CurrentDomain.GetAssemblies().Single(a => a.GetName().Name == "ReRiff.ClientConfig")
            .GetType("ReRiff.ClientConfig", true);
        config.GetField("initialized", BindingFlags.Static | BindingFlags.NonPublic).SetValue(null, false);
        var offline = (Task)type.GetMethod("ExecuteAsync").Invoke(Activator.CreateInstance(type, true),
            new object[] { null, null, null, CancellationToken.None });
        Check(offline.IsFaulted && offline.Exception.InnerException is WebException,
            "existing offline gate still rejects before accessing a request");
    } finally { listener.Stop(); }
} finally {
    File.Delete(versionFile); File.Delete(Path.Combine(folder, "server.ini"));
}
