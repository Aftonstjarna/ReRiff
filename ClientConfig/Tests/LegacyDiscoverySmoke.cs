using System;
using System.IO;
using System.Reflection;
using System.Collections.Generic;
class LegacyDiscoverySmoke {
    static void Main(string[] args){
        string folder=Path.Combine(Path.GetTempPath(),"reriff-discovery-"+Guid.NewGuid());Directory.CreateDirectory(folder);
        var method=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.ProgressionRemote").GetMethod("Connect",BindingFlags.NonPublic|BindingFlags.Static);
        Func<string,object> connect=url=>method.Invoke(null,new object[]{folder,url,new Func<Dictionary<string,object>>(()=>null),new Func<string,bool,string>((u,r)=>{throw new Exception("Legacy discovery must not prompt");})});
        try{
            foreach(string path in new[]{"/legacy","/preview"})if(connect(args[0]+path)!=null)throw new Exception("Legacy server should use original connection");
            bool refused=false;try{connect(args[0]+"/old-prestige");}catch(TargetInvocationException e){refused=e.InnerException is IOException&&e.InnerException.Message.Contains("Update this server");}
            if(!refused)throw new Exception("Earlier prestige protocol must require update");
            Console.WriteLine("PASS: legacy and preview servers bypass account setup; earlier prestige server requires update");
        }finally{Directory.Delete(folder,true);}
    }
}
