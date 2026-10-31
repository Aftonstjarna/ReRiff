using System.Net;
using System.Net.Sockets;
using System.Text;

var folder=Path.Combine(Path.GetTempPath(),"reriff-network-toggle-"+Guid.NewGuid());
Directory.CreateDirectory(folder);
var listener=new TcpListener(IPAddress.Loopback,0);listener.Start();
try {
    var endpoint="http://127.0.0.1:"+((IPEndPoint)listener.LocalEndpoint).Port;
    string Settings(string enabled)=>"[server]\nnetworking_enabled="+enabled+"\nexternal_url="+endpoint+"\nfallback_url=http://127.0.0.1:5144\nexternal_only=true\n";
    var file=Path.Combine(folder,"server.ini");
    foreach(var value in new[]{"false","no","0"}) {
        File.WriteAllText(file,Settings(value));var before=File.ReadAllBytes(file);
        if(ReRiff.ClientConfig.Select(folder)!="reriff-disabled://offline" || listener.Pending())throw new Exception("Disabled networking probed an endpoint");
        if(!before.SequenceEqual(File.ReadAllBytes(file)))throw new Exception("Selection changed saved addresses");
    }
    File.WriteAllText(file,Settings("true"));
    var serve=Task.Run(async()=> {
        using var client=await listener.AcceptTcpClientAsync();using var stream=client.GetStream();
        var input=new byte[8192];await stream.ReadAsync(input);
        var body="{\"results\":[]}";
        await stream.WriteAsync(Encoding.ASCII.GetBytes("HTTP/1.1 200 OK\r\nContent-Length: "+body.Length+"\r\nConnection: close\r\n\r\n"+body));
    });
    if(ReRiff.ClientConfig.Select(folder)!=endpoint)throw new Exception("Re-enable did not reuse the saved endpoint");
    await serve.WaitAsync(TimeSpan.FromSeconds(5));
    Console.WriteLine("PASS: disabled skips endpoints and preserves INI; re-enable reuses saved address");
} finally {listener.Stop();Directory.Delete(folder,true);}
