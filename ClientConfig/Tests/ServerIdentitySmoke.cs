using System;using System.IO;using System.Reflection;
class ServerIdentitySmoke {
 static void Check(bool x){if(!x)throw new Exception("Identity assertion failed");}
 static int Main(){string folder=Path.Combine(Path.GetTempPath(),"reriff-identity-"+Guid.NewGuid());Directory.CreateDirectory(folder);
 try {var method=typeof(ReRiff.ClientConfig).GetMethod("RememberServerIdentity",BindingFlags.NonPublic|BindingFlags.Static);Check(method!=null);
 string id=Guid.NewGuid().ToString("D");Action<string,string> remember=(endpoint,value)=>method.Invoke(null,new object[]{folder,endpoint,value});
 remember("http://192.0.2.10:5144",id);remember("http://192.0.2.10:5144",id);remember("https://reriff.example.net",id);
 string[] lines=File.ReadAllLines(Path.Combine(folder,"localserver/server-identities.txt"));Check(lines.Length==2&&lines[0].StartsWith(id+"\t")&&lines[1].StartsWith(id+"\t"));
 Check(File.ReadAllText(Path.Combine(folder,"logs/ReRiff-client.log")).Contains("known_alias=true"));remember("https://other.example","invalid");Check(File.ReadAllLines(Path.Combine(folder,"localserver/server-identities.txt")).Length==2);
 Console.WriteLine("PASS: persistent endpoint aliases, duplicate suppression and legacy/invalid-ID handling");return 0;
 } finally{Directory.Delete(folder,true);}}
}
