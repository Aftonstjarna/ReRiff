using System;
using System.IO;
using System.Net;
using System.Reflection;

class SigningSmoke {
    static int Main() {
        foreach (Assembly assembly in new[] { typeof(SigningSmoke).Assembly,
                 typeof(ReRiff.ClientConfig).Assembly, typeof(ReRiff.NewsHeaders).Assembly }) {
            if (assembly.GetName().GetPublicKeyToken().Length == 0)
                throw new Exception("Unsigned assembly: " + assembly.FullName);
        }
        string root = AppDomain.CurrentDomain.BaseDirectory;
        File.WriteAllText(Path.Combine(root, "server.ini"),
            "[server]\nnetworking_enabled=false\nshow_server_popup=false\n");
        if (ReRiff.ClientConfig.CanSend()) throw new Exception("Expected offline fixture");
        string local = Path.Combine(root, "localserver");
        Directory.CreateDirectory(local);
        File.WriteAllText(Path.Combine(local, "install-state.json"), "{\"version\":\"1.2.1-2\"}");
        WebRequest request = WebRequest.Create("http://127.0.0.1/classes/News");
        ReRiff.NewsHeaders.Add(request);
        if (request.Headers["X-ReRiff-Client-Version"] != "1.2.1")
            throw new Exception("Revision version header was not set");
        Console.WriteLine("PASS: signed x86 .NET Framework caller loads both signed helpers; offline routing and revision header work.");
        return 0;
    }
}
