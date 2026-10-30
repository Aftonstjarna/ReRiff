// Diagnostic host: initialize original entity/node managers and make eight
// straight nodes. No game loop, save system or renderer is started. Mixed-mode
// EXEs need a disposable DLL-form reference; never patch the installed game.
using System;
using System.Reflection;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

class OriginalRuntimeProbe
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    static extern IntPtr GetModuleHandle(string name);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    static extern uint GetModuleFileName(IntPtr module, StringBuilder path, int size);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool SetDllDirectory(string path);

    static MethodInfo Find(MethodInfo[] methods, string name)
    {
        foreach (MethodInfo method in methods) if (method.Name == name) return method;
        throw new MissingMethodException(name);
    }

    static unsafe int Main(string[] args)
    {
        try
        {
            bool mono = Type.GetType("Mono.Runtime") != null;
            Console.WriteLine("MONO_RUNTIME=" + mono);
            Console.WriteLine("CLR_VERSION=" + Environment.Version);
            Console.WriteLine("POINTER_BYTES=" + IntPtr.Size);
            Console.WriteLine("MSCORLIB=" + typeof(object).Assembly.Location);
            IntPtr clr = GetModuleHandle("clr.dll");
            var path = new StringBuilder(32768);
            if (clr != IntPtr.Zero) GetModuleFileName(clr, path, path.Capacity);
            Console.WriteLine("LOADED_CLR=" + path);
            if (mono || clr == IntPtr.Zero || IntPtr.Size != 4)
            {
                Console.WriteLine("RESULT=WRONG_RUNTIME");
                return 10;
            }
            if (args.Length < 1 || args.Length > 2) throw new ArgumentException("Pass assembly path and optional original dependency directory");
            string gameDirectory = args.Length == 2 ? args[1] : Path.GetDirectoryName(args[0]);
            if (!SetDllDirectory(gameDirectory)) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
            AppDomain.CurrentDomain.AssemblyResolve += delegate(object sender, ResolveEventArgs request)
            {
                string dependency = Path.Combine(gameDirectory, new AssemblyName(request.Name).Name + ".dll");
                return File.Exists(dependency) ? Assembly.LoadFrom(dependency) : null;
            };
            Assembly assembly = Assembly.LoadFile(args[0]);
            Console.WriteLine("ASSEMBLY=" + assembly.FullName);
            MethodInfo[] methods = assembly.ManifestModule.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
            Console.WriteLine("GLOBAL_METHODS=" + methods.Length);
            foreach (string name in new string[] { "gaEntityManager.create", "gaNodeManager.create" })
            {
                Console.WriteLine("CALL=" + name);
                Find(methods, name).Invoke(null, new object[0]);
            }
            Console.WriteLine("ALLOCATOR=OK");
            var field = assembly.ManifestModule.GetField("?s_instance@gaNodeManager@@0PAV1@A", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
            if (field == null) throw new MissingFieldException("gaNodeManager singleton");
            void* manager = Pointer.Unbox(field.GetValue(null));
            if (manager == null) throw new InvalidOperationException("Null node manager");
            MethodInfo makeNodes = Find(methods, "gaNodeManager.makeNodes");
            Console.WriteLine("CALL=gaNodeManager.makeNodes(8,20)");
            makeNodes.Invoke(null, new object[] { Pointer.Box(manager, makeNodes.GetParameters()[0].ParameterType), 8, 20.0f });
            byte* begin = (byte*)((int*)manager)[1];
            byte* end = (byte*)((int*)manager)[2];
            int count = (int)(end - begin) / 244;
            Console.WriteLine("NODE_COUNT=" + count);
            if (count != 8) throw new InvalidOperationException("Unexpected node count");
            for (int i = 0; i < count; ++i)
            {
                float* position = (float*)(begin + i * 244 + 172);
                Console.WriteLine("NODE=" + i + "," + position[0] + "," + position[1] + "," + position[2]);
                if (position[0] != 0 || position[1] != 0 || position[2] != i * 20.0f)
                    throw new InvalidOperationException("Unexpected original node position");
            }
            Console.WriteLine("RESULT=NODES_OK");
            return 0;
        }
        catch (Exception error)
        {
            Console.WriteLine(error);
            Console.WriteLine("RESULT=EXCEPTION");
            return 1;
        }
    }
}
