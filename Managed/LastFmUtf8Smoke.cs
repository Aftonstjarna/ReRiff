// Offline validation of the exact original string-copy token used by the
// scoped Last.fm CIL replacement. No game loop, user saves or HTTP requests.
using System;using System.IO;using System.Linq;using System.Reflection;
using System.Reflection.Emit;using System.Runtime.InteropServices;using System.Text;
class LastFmUtf8Smoke {
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern bool SetDllDirectory(string path);
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr LoadLibrary(string path);
    static void Check(bool ok,string label){if(!ok)throw new Exception(label);}
    static unsafe int Main(string[] args){try{
        Check(IntPtr.Size==4&&Type.GetType("Mono.Runtime")==null,"Requires x86 .NET Framework");
        string fixture=Path.GetDirectoryName(Path.GetFullPath(args[0]));Check(SetDllDirectory(fixture),"Fixture DLL search");
        foreach(string dll in new[]{"libfftw3f-3.dll","msvcr120.dll","msvcp120.dll","ParseOctane.dll","X3DAudio1_7.dll","d3dx9_43.dll","XINPUT1_3.dll"}){
            if(LoadLibrary(dll)==IntPtr.Zero)throw new Exception("Fixture dependency "+dll+" error "+Marshal.GetLastWin32Error());
        }
        AppDomain.CurrentDomain.AssemblyResolve += delegate(object sender,ResolveEventArgs request){string file=Path.Combine(fixture,new AssemblyName(request.Name).Name+".dll");return File.Exists(file)?Assembly.LoadFrom(file):null;};
        Console.WriteLine("Original fixture dependencies loaded");
        var module=Assembly.LoadFile(Path.GetFullPath(args[0])).ManifestModule;
        var copy=(MethodInfo)module.ResolveMethod(0x06000631);
        Check(copy.Name.EndsWith(".=")&&copy.IsStatic&&copy.GetParameters().Length==2,"Original copy token");
        Type native=copy.GetParameters()[0].ParameterType.GetElementType();
        var tidy=module.GetMethods(BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic)
            .Single(m=>m.Name=="std.basic_string<char,std::char_traits<char>,std::allocator<char> >._Tidy"&&m.GetParameters().Length==3);
        var method=new DynamicMethod("LastFmOriginalCopy",typeof(void),new[]{typeof(IntPtr),typeof(IntPtr)},module,true);
        var il=method.GetILGenerator();for(int i=0;i<49;i++)il.DeclareLocal(i==47||i==48?native:typeof(int));
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Conv_U);il.Emit(OpCodes.Ldobj,native);il.Emit(OpCodes.Stloc_S,(byte)47);
        il.Emit(OpCodes.Ldarg_1);il.Emit(OpCodes.Conv_U);il.Emit(OpCodes.Ldobj,native);il.Emit(OpCodes.Stloc_S,(byte)48);
        // Same local indices, call target and stack behavior as patched IL_A6.
        il.Emit(OpCodes.Ldloca_S,(byte)47);il.Emit(OpCodes.Ldloca_S,(byte)48);il.Emit(OpCodes.Call,copy);il.Emit(OpCodes.Pop);
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Conv_U);il.Emit(OpCodes.Ldloc_S,(byte)47);il.Emit(OpCodes.Stobj,native);il.Emit(OpCodes.Ret);
        var run=(Action<IntPtr,IntPtr>)method.CreateDelegate(typeof(Action<IntPtr,IntPtr>));
        string[] fixtures={"","ASCII","BUCK\u2010TICK","{\"artist\":{\"name\":\"BUCK\u2010TICK\"}}",
            "{\"artist\":{\"name\":\"BUCK\\u2010TICK\"}}","日本語 中文 한국어",new string('a',8192)+"\u2010"};
        foreach(string text in fixtures){
            byte[] bytes=Encoding.UTF8.GetBytes(text);IntPtr src=Marshal.AllocHGlobal(24),dst=Marshal.AllocHGlobal(24),external=IntPtr.Zero;
            for(int i=0;i<24;i++){Marshal.WriteByte(src,i,0);Marshal.WriteByte(dst,i,0);}Marshal.WriteInt32(dst,20,15);
            try{
                IntPtr data=src;if(bytes.Length>=16){external=Marshal.AllocHGlobal(bytes.Length+1);data=external;Marshal.WriteIntPtr(src,external);}
                Marshal.Copy(bytes,0,data,bytes.Length);Marshal.WriteByte(data,bytes.Length,0);
                Marshal.WriteInt32(src,16,bytes.Length);Marshal.WriteInt32(src,20,bytes.Length>=16?bytes.Length:15);
                run(dst,src);Check(Marshal.ReadInt32(dst,16)==bytes.Length,"Copied length");
                IntPtr output=Marshal.ReadInt32(dst,20)<16?dst:Marshal.ReadIntPtr(dst);
                byte[] actual=new byte[bytes.Length];Marshal.Copy(output,actual,0,actual.Length);
                Check(bytes.SequenceEqual(actual)&&Marshal.ReadByte(output,actual.Length)==0,"UTF-8 bytes and terminator");
                Check(Marshal.ReadInt32(src,16)==bytes.Length,"Source length unchanged");
                Marshal.Copy(data,actual,0,actual.Length);Check(bytes.SequenceEqual(actual),"Source bytes unchanged");
                if(external!=IntPtr.Zero)Check(output!=external,"Independent native allocation");
            }finally{
                tidy.Invoke(null,new object[]{Pointer.Box((void*)dst,tidy.GetParameters()[0].ParameterType),true,(uint)0});
                if(external!=IntPtr.Zero)Marshal.FreeHGlobal(external);Marshal.FreeHGlobal(src);Marshal.FreeHGlobal(dst);
            }
        }
        Console.WriteLine("PASS: original CLR string-copy token, patched local-address call shape, UTF-8/ASCII/empty/long/escaped values, independent native allocation and original cleanup");return 0;
    }catch(Exception e){Console.WriteLine(e);return 1;}}
}
