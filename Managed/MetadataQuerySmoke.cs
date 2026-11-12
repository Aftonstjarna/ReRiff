// Offline original query-builder regression. The private fixture replaces
// only FindAsync with a null task; all original query construction executes.
using System;using System.Collections;using System.IO;using System.Reflection;
using System.Runtime.InteropServices;using System.Text;using Parse;
class MetadataQuerySmoke {
    static unsafe object Ptr(IntPtr p){return Pointer.Box((void*)p,typeof(sbyte*));}
    static IntPtr Text(string s){byte[] b=Encoding.UTF8.GetBytes(s);IntPtr p=Marshal.AllocHGlobal(b.Length+1);Marshal.Copy(b,0,p,b.Length);Marshal.WriteByte(p,b.Length,0);return p;}
    static void Check(bool x,string s){if(!x)throw new Exception(s);}
    static int Main(string[] args){bool enabled=Array.IndexOf(args,"--disabled")<0;bool range=Array.IndexOf(args,"--range")>=0;string root=AppDomain.CurrentDomain.BaseDirectory;
        File.WriteAllText(Path.Combine(root,"server.ini"),"[diagnostics]\nmetadata_utf8_probe="+(enabled?"1":"0")+"\n");
        try{
            ParseClient.Initialize(new ParseClient.Configuration{ApplicationId="query-test",WindowsKey="test",Server="http://127.0.0.1:9/"});
            var a=Assembly.LoadFrom(Path.Combine(root,"ParseOctane.dll"));Type t=a.GetType("wsNetworkParseImpl",true);
            BindingFlags flags=BindingFlags.Instance|BindingFlags.NonPublic|BindingFlags.Public;
            IntPtr lengthKey=Text("Length"),bpmKey=Text("Bpm");
            IntPtr[] p=Array.ConvertAll(new[]{"Track","Artist","BUCK\u2010TICK","Title","日本語","objectId","é","Genre","한글"},Text);
            try{
                foreach(int token in range?new[]{0x060000f2,0x060000f3}:new[]{0x060000f0,0x060000f4}){
                    object instance=Activator.CreateInstance(t,true);var method=(MethodInfo)a.ManifestModule.ResolveMethod(token);
                    object[] input=token==0x060000f0?Array.ConvertAll(p,Ptr):new object[]{Ptr(p[0]),Ptr(p[1]),Ptr(p[2]),Ptr(p[3]),10,2};
                    if(token==0x060000f2)input=new object[]{Ptr(p[0]),Ptr(p[1]),Ptr(p[2]),Ptr(p[3]),Ptr(p[4]),Ptr(lengthKey),100f,5f};
                    if(token==0x060000f3)input=new object[]{Ptr(p[0]),Ptr(p[1]),Ptr(p[2]),Ptr(bpmKey),120f,2f,Ptr(lengthKey),100f,5f};
                    if(token==0x060000f4)foreach(var parameter in method.GetParameters()){if(parameter.Name=="_limit")input[parameter.Position]=10;if(parameter.Name=="_skip")input[parameter.Position]=2;}
                    method.Invoke(instance,input);object query=t.GetField("m_currentQuery",flags).GetValue(instance);
                    var where=(IDictionary)query.GetType().GetField("where",flags).GetValue(query);
                    Check((string)where["Artist"]==(enabled?"BUCK\u2010TICK":Marshal.PtrToStringAnsi(p[2])),"Artist equality differs");
                    if(token==0x060000f0){
                        Check((string)where["Title"]==(enabled?"日本語":Marshal.PtrToStringAnsi(p[4])),"Title equality differs");
                        Check((string)where["Genre"]==(enabled?"한글":Marshal.PtrToStringAnsi(p[8])),"Genre equality differs");
                        Check((string)where["objectId"]==Marshal.PtrToStringAnsi(p[6]),"Opaque equality changed");
                    }else if(token==0x060000f4){
                        Check((int?)query.GetType().GetField("limit",flags).GetValue(query)==10,"Limit changed");
                        Check((int?)query.GetType().GetField("skip",flags).GetValue(query)==2,"Skip changed");
                    }
                    // Original range parameters are full widths: endpoints are value +/- range/2.
                    if(range){var bounds=(IDictionary)where["Length"];Check(Convert.ToSingle(bounds["$gte"])==97.5f&&Convert.ToSingle(bounds["$lte"])==102.5f,"Length range changed");
                        if(token==0x060000f2)Check((string)where["Title"]==(enabled?"日本語":Marshal.PtrToStringAnsi(p[4])),"Range Title equality differs");
                        else{var bpm=(IDictionary)where["Bpm"];Check(Convert.ToSingle(bpm["$gte"])==119f&&Convert.ToSingle(bpm["$lte"])==121f,"Bpm range changed");}}
                }
            }finally{Marshal.FreeHGlobal(lengthKey);Marshal.FreeHGlobal(bpmKey);foreach(var ptr in p)Marshal.FreeHGlobal(ptr);}
            Console.WriteLine("PASS: "+(enabled?"UTF-8":"legacy")+" original object equality queries, multiple song fields, opaque values, sorting limits/skip; no requests sent");return 0;
        }catch(Exception e){Console.WriteLine(e);return 1;}finally{File.Delete(Path.Combine(root,"server.ini"));}
    }
}
