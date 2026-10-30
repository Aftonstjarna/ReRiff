using System.Net;
using System.Net.Sockets;
using System.Text;

var root = Path.Combine(Path.GetTempPath(), "reriff-config-tests-" + Guid.NewGuid());
Directory.CreateDirectory(root);
void Check(bool condition, string description) {
    if (!condition) throw new Exception(description);
    Console.WriteLine("PASS: " + description);
}
string Select(string ini) {
    File.WriteAllText(Path.Combine(root, "server.ini"), ini);
    return ReRiff.ClientConfig.Select(root);
}
try {
    Check(Select("[server]\nexternal_url=\nfallback_url=\n") == "reriff-disabled://offline", "both addresses blank disables networking");
    Check(Select("[server]\nfallback_url=http://127.0.0.1:6123\n") == "http://127.0.0.1:6123", "configured fallback and custom port");
    Check(Select("[server]\nexternal_url=http://127.0.0.1:1\nfallback_url=http://127.0.0.1:6123\ntimeout_seconds=0.1\n") == "http://127.0.0.1:6123", "unavailable external selects configured fallback");
    Check(Select("[server]\nfallback_url=ftp://host\n") == "reriff-disabled://offline", "invalid URL fails closed");
    File.Delete(Path.Combine(root, "server.ini"));
    Check(ReRiff.ClientConfig.Select(root) == "reriff-disabled://offline", "missing file fails closed");
    var listener = new TcpListener(IPAddress.Loopback, 0);
    listener.Start();
    var port = ((IPEndPoint)listener.LocalEndpoint).Port;
    var serve = Task.Run(async () => {
        using var client = await listener.AcceptTcpClientAsync();
        using var stream = client.GetStream();
        byte[] buffer = new byte[8192];
        await stream.ReadAsync(buffer);
        byte[] body = Encoding.UTF8.GetBytes("{\"results\":[{\"Ver\":100}]}");
        byte[] headers = Encoding.ASCII.GetBytes("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + body.Length + "\r\nConnection: close\r\n\r\n");
        await stream.WriteAsync(headers); await stream.WriteAsync(body);
    });
    try {
        var external = "http://127.0.0.1:" + port;
        Check(Select("[server]\nexternal_url=" + external + "\nfallback_url=http://127.0.0.1:6123\n") == external, "reachable external selected directly");
        await serve;
    } finally { listener.Stop(); }
    Check(ReRiff.ClientConfig.OfflineTask<string>().IsFaulted, "offline transport returns failed task without networking");
    // Exercise the actual patched SDK entry point with no request object. A
    // successful offline gate must return before attempting to read that object.
    string assemblyFolder = AppContext.BaseDirectory;
    File.WriteAllText(Path.Combine(assemblyFolder, "server.ini"), "[server]\nexternal_url=\nfallback_url=\n");
    System.Reflection.Assembly.LoadFrom(Path.Combine(assemblyFolder, "Parse.dll"));
    var platform = System.Reflection.Assembly.LoadFrom(Path.Combine(assemblyFolder, "Parse.NetFx45.dll"));
    var transport = platform.GetType("Parse.Internal.HttpClient", true);
    var execute = transport.GetMethod("ExecuteAsync");
    var instance = Activator.CreateInstance(transport, true);
    var task = (Task)execute.Invoke(instance, new object[] { null, null, null, CancellationToken.None });
    Check(task.IsFaulted, "patched SDK blocks at transport entry before touching a request");
    Check(task.Exception.InnerException is WebException, "offline failure follows normal network-error task handling");
} finally { Directory.Delete(root, true); }
