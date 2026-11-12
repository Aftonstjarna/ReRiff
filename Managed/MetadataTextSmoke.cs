using System;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using Parse;
using ReRiff;

public static unsafe class MetadataTextSmoke {
    static readonly UTF8Encoding Utf8=new UTF8Encoding(false,true);
    static bool enabled;
    static void Check(bool condition,string message){if(!condition)throw new Exception(message);}
    static IntPtr Bytes(byte[] bytes){IntPtr p=Marshal.AllocHGlobal(bytes.Length+1);Marshal.Copy(bytes,0,p,bytes.Length);Marshal.WriteByte(p,bytes.Length,0);return p;}
    static IntPtr Text(string value){return Bytes(Utf8.GetBytes(value));}
    static byte[] Read(IntPtr p){int n=0;while(n<4095&&Marshal.ReadByte(p,n)!=0)n++;Check(n<4095,"native output not terminated inside copy range");var b=new byte[n];Marshal.Copy(p,b,0,n);return b;}
    static object Box(IntPtr p){return Pointer.Box(p.ToPointer(),typeof(sbyte*));}
    static string Ansi(string value){IntPtr p=Marshal.StringToHGlobalAnsi(value);try{return Marshal.PtrToStringAnsi(p);}finally{Marshal.FreeHGlobal(p);}}
    public static int Main(string[] args){
        enabled=args.Length==0;string root=AppDomain.CurrentDomain.BaseDirectory;
        File.WriteAllText(Path.Combine(root,"server.ini"),"[diagnostics]\nmetadata_trace=1\nmetadata_utf8_probe="+(enabled?"1":"0")+"\n");
        try{
            IntPtr key=Text("Title"),unknown=Text("ObjectId");
            try{
                string[] samples={"ミスターフィクサー","BUCK‐TICK","繁體 简体 한글","\U0002000b 😀 e\u0301","ASCII","","Ã©"};
                foreach(string value in samples){
                    IntPtr input=Text(value);
                    try{
                        string old=new string((sbyte*)input.ToPointer());
                        Check(MetadataText.DecodeValue(input,key)==(enabled?value:old),"decode differs");
                        Check(MetadataText.DecodeValue(input,unknown)==old,"opaque field changed");
                    }finally{Marshal.FreeHGlobal(input);}
                    IntPtr output=MetadataText.AllocateValue(value,key);
                    try{Check(enabled?Utf8.GetString(Read(output))==value:Marshal.PtrToStringAnsi(output)==Ansi(value),"encode differs");}
                    finally{Marshal.FreeHGlobal(output);}
                }
                IntPtr invalid=Bytes(new byte[]{0xff,0xe9});
                try{Check(MetadataText.DecodeValue(invalid,key)==new string((sbyte*)invalid.ToPointer()),"invalid UTF8 changed legacy behavior");}finally{Marshal.FreeHGlobal(invalid);}
                Check(MetadataText.DecodeValue(IntPtr.Zero,key)==new string((sbyte*)null),"null input semantics changed");
                Check(MetadataText.AllocateValue(null,key)==IntPtr.Zero,"null output semantics changed");
                IntPtr legacy=MetadataText.AllocateValue("é",unknown);
                try{Check(Marshal.PtrToStringAnsi(legacy)=="é","opaque output changed");}finally{Marshal.FreeHGlobal(legacy);}
                if(enabled){
                    foreach(string longText in new[]{new string('漢',2000)+"😀",new string('a',4092)+"😀x",new string('a',4093)+"漢"}){
                        IntPtr output=MetadataText.AllocateValue(longText,key);
                        try{byte[] bytes=Read(output);string prefix=Utf8.GetString(bytes);Check(bytes.Length<=4094&&longText.StartsWith(prefix,StringComparison.Ordinal),"UTF8 truncation split a scalar");}
                        finally{Marshal.FreeHGlobal(output);}
                    }
                }
            }finally{Marshal.FreeHGlobal(key);Marshal.FreeHGlobal(unknown);}
            RunOriginalBridge(root);
            Console.WriteLine("PASS: "+(enabled?"enabled UTF8":"disabled legacy")+" metadata boundary, original mixed-mode bridge write/read, buffer termination, nulls, invalid input and opaque fields");return 0;
        }finally{File.Delete(Path.Combine(root,"server.ini"));}
    }
    static void RunOriginalBridge(string root){
        ParseClient.Initialize(new ParseClient.Configuration{ApplicationId="metadata-probe",WindowsKey="probe",Server="http://127.0.0.1:9/"});
        Assembly bridge=Assembly.LoadFrom(Path.Combine(root,"ParseOctane.dll"));
        Type type=bridge.GetType("wsNetworkParseImpl",true);object instance=Activator.CreateInstance(type,true);
        const BindingFlags flags=BindingFlags.Public|BindingFlags.NonPublic|BindingFlags.Instance;
        var objects=(IDictionary)type.GetField("m_objects",flags).GetValue(instance);
        var track=new ParseObject("Track");objects.Add(7,track);
        MethodInfo write=null,read=null;
        foreach(MethodInfo method in type.GetMethods(flags)){
            if(method.MetadataToken==0x060000de)write=method;
            if(method.MetadataToken==0x060000fc)read=method;
        }
        Check(write!=null&&read!=null,"original bridge tokens missing");
        IntPtr key=Text("Title");
        try{
            string[] samples={"ミスターフィクサー","ひきずるビート","Artist‐Name 漢字 한글 \U0002000b 😀 e\u0301"};
            foreach(string value in samples){
                IntPtr input=Text(value);
                try{write.Invoke(instance,new object[]{7,Box(key),Box(input)});
                    Check((string)track["Title"]==(enabled?value:new string((sbyte*)input.ToPointer())),"bridge write differs");
                }finally{Marshal.FreeHGlobal(input);}
                // Also simulate properly encoded server data, independent of
                // whether the disabled original writer corrupted its input.
                track["Title"]=value;
                IntPtr output=new IntPtr(Pointer.Unbox(read.Invoke(instance,new object[]{7,Box(key)})));
                Check(enabled?Utf8.GetString(Read(output))==value:Marshal.PtrToStringAnsi(output)==Ansi(value),"bridge server read differs");
            }
            if(enabled){
                track["Title"]=new string('漢',2000)+"😀";
                IntPtr output=new IntPtr(Pointer.Unbox(read.Invoke(instance,new object[]{7,Box(key)})));
                string prefix=Utf8.GetString(Read(output));Check(((string)track["Title"]).StartsWith(prefix,StringComparison.Ordinal),"native strncpy split UTF8");
            }
        }finally{Marshal.FreeHGlobal(key);}
    }
}
