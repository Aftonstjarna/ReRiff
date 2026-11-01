// Offline reference host. Original methods are executed unchanged in the
// DLL-form diagnostic image. Only file I/O and input/output plumbing are hosted.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

unsafe partial class OriginalSongCapture
{
    const BindingFlags Flags = BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;
    static Module module;
    static MethodInfo[] methods;
    static readonly List<IntPtr> allocations = new List<IntPtr>();
    static readonly CultureInfo Invariant = CultureInfo.InvariantCulture;
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern bool SetDllDirectory(string path);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern IntPtr GetModuleHandle(string name);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern uint GetModuleFileName(IntPtr handle, StringBuilder path, int size);

    static IntPtr Alloc(int size)
    {
        IntPtr p = Marshal.AllocHGlobal(size); allocations.Add(p);
        for (int i=0;i<size;i++) ((byte*)p)[i]=0;
        return p;
    }
    static IntPtr Utf8(string value)
    {
        byte[] bytes=Encoding.UTF8.GetBytes(value);IntPtr p=Alloc(bytes.Length+1);
        Marshal.Copy(bytes,0,p,bytes.Length);return p;
    }
    static object Call(string name, params object[] args)
    {
        foreach (MethodInfo method in methods)
        {
            if (method.Name!=name) continue;
            ParameterInfo[] parameters=method.GetParameters();
            if (parameters.Length!=args.Length) continue;
            bool matches=true;
            for (int i=0;i<args.Length;i++)
                if ((args[i] is IntPtr)!=parameters[i].ParameterType.IsPointer) matches=false;
            if (!matches) continue;
            object[] converted=(object[])args.Clone();
            for (int i=0;i<args.Length;i++)
                if (parameters[i].ParameterType.IsPointer) converted[i]=Pointer.Box((void*)(IntPtr)args[i],parameters[i].ParameterType);
            return method.Invoke(null,converted);
        }
        throw new MissingMethodException(name);
    }
    static FieldInfo Field(string name)
    {
        FieldInfo f=module.GetField(name,Flags);
        if (f==null) throw new MissingFieldException(name);
        return f;
    }
    static IntPtr Global(string name) { return (IntPtr)Pointer.Unbox(Field(name).GetValue(null)); }
    static void SetGlobal(string name,IntPtr value)
    {
        FieldInfo f=Field(name);f.SetValue(null,Pointer.Box((void*)value,f.FieldType));
    }
    static void SetShortString(IntPtr address,string value)
    {
        byte[] bytes=Encoding.UTF8.GetBytes(value);
        if (bytes.Length>15) throw new ArgumentException("Expected SSO genre string");
        // Storage has already been constructed by gaRaceConfig's constructor.
        byte* p=(byte*)address;
        if (((uint*)p)[5]!=15 || ((uint*)p)[4]!=0) throw new InvalidOperationException("Genre string not initially empty SSO");
        Marshal.Copy(bytes,0,address,bytes.Length);p[bytes.Length]=0;((int*)p)[4]=bytes.Length;
    }
    static IntPtr Json(string filename)
    {
        IntPtr p=Alloc(4);Call("elJSON.{ctor}",p);
        if (!(bool)Call("elJSON.parse",p,Utf8(File.ReadAllText(filename)))) throw new IOException("Original JSON parser rejected "+filename);
        return p;
    }
    static void Dump(IntPtr begin,int length,string filename)
    {
        if (length<0 || length>64*1024*1024) throw new IOException("Invalid capture length");
        byte[] bytes=new byte[length];Marshal.Copy(begin,bytes,0,length);File.WriteAllBytes(filename,bytes);
    }
    static string F(float value) { return value.ToString("R",Invariant); }

    // elConfigFile.load uses only openFile, isOpen, read and the intrusive
    // reference-counted file destructor. Serve original bytes with these APIs.
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] delegate IntPtr OpenFn(IntPtr self,IntPtr result,IntPtr filename,int mode);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] delegate IntPtr DestroyFn(IntPtr self,uint deleting);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] [return:MarshalAs(UnmanagedType.I1)] delegate bool IsOpenFn(IntPtr self);
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] delegate int ReadFn(IntPtr self,IntPtr buffer,int size);
    class StreamState { public byte[] Data; public int Position; }
    static readonly Dictionary<IntPtr,StreamState> streams=new Dictionary<IntPtr,StreamState>();
    static readonly Dictionary<string,byte[]> configs=new Dictionary<string,byte[]>(StringComparer.OrdinalIgnoreCase);
    static OpenFn openCallback=Open;
    static DestroyFn destroyCallback=Destroy;
    static IsOpenFn isOpenCallback=IsOpen;
    static ReadFn readCallback=Read;
    static IntPtr fileTable;
    static string ioError;
    static IntPtr Open(IntPtr self,IntPtr result,IntPtr filename,int mode)
    {
        try
        {
            string name=Marshal.PtrToStringAnsi(filename).Replace('\\','/');
            byte[] data;
            if (mode!=1 || !configs.TryGetValue(name,out data))
            {
                Console.WriteLine("UNSERVED_CONFIG="+name+" mode="+mode);Marshal.WriteIntPtr(result,IntPtr.Zero);return result;
            }
            IntPtr file=Alloc(8);Marshal.WriteIntPtr(file,fileTable);Marshal.WriteInt32(file,4,1);
            streams[file]=new StreamState { Data=data,Position=0 };Marshal.WriteIntPtr(result,file);
            Console.WriteLine("OPEN_CONFIG="+name+" bytes="+data.Length);
        }
        catch(Exception e) { ioError=e.ToString();Marshal.WriteIntPtr(result,IntPtr.Zero); }
        return result;
    }
    static IntPtr Destroy(IntPtr self,uint deleting) { streams.Remove(self);return IntPtr.Zero; }
    static bool IsOpen(IntPtr self) { return streams.ContainsKey(self); }
    static int Read(IntPtr self,IntPtr buffer,int size)
    {
        try
        {
            StreamState stream=streams[self];int count=Math.Min(size,stream.Data.Length-stream.Position);
            if (count<0) return 0;
            Marshal.Copy(stream.Data,stream.Position,buffer,count);stream.Position+=count;return count;
        }
        catch(Exception e) { ioError=e.ToString();return 0; }
    }
    static void InstallFileBridge(string input)
    {
        foreach(string name in new string[] {"segments_PC.cfg","sections_PC.cfg","visualSections.cfg"})
            configs["scripts/config/"+name]=File.ReadAllBytes(Path.Combine(input,name));
        fileTable=Alloc(12);
        Marshal.WriteIntPtr(fileTable,0,Marshal.GetFunctionPointerForDelegate(destroyCallback));
        Marshal.WriteIntPtr(fileTable,4,Marshal.GetFunctionPointerForDelegate(isOpenCallback));
        Marshal.WriteIntPtr(fileTable,8,Marshal.GetFunctionPointerForDelegate(readCallback));
        IntPtr table=Alloc(8);Marshal.WriteIntPtr(table,4,Marshal.GetFunctionPointerForDelegate(openCallback));
        IntPtr manager=Alloc(4);Marshal.WriteIntPtr(manager,table);
        SetGlobal("?ms_instance@?$elSingleton@VelFileManager@@@@0PAVelFileManager@@A",manager);
    }

    static int Main(string[] args)
    {
        try
        {
            if (args.Length!=4) throw new ArgumentException("ASSEMBLY ORIGINAL_GAME_DIR INPUT_DIR OUTPUT_DIR");
            bool mono=Type.GetType("Mono.Runtime")!=null;
            var clr=new StringBuilder(32768);IntPtr handle=GetModuleHandle("clr.dll");
            if (handle!=IntPtr.Zero) GetModuleFileName(handle,clr,clr.Capacity);
            Console.WriteLine("MONO_RUNTIME="+mono+" CLR="+Environment.Version+" LOADED_CLR="+clr+" POINTER_BYTES="+IntPtr.Size);
            if(mono || handle==IntPtr.Zero || IntPtr.Size!=4) throw new InvalidOperationException("Microsoft x86 CLR required");
            if (!SetDllDirectory(args[1])) throw new InvalidOperationException("SetDllDirectory failed");
            AppDomain.CurrentDomain.AssemblyResolve+=delegate(object sender,ResolveEventArgs request)
            {
                string path=Path.Combine(args[1],new AssemblyName(request.Name).Name+".dll");
                return File.Exists(path)?Assembly.LoadFrom(path):null;
            };
            Assembly assembly=Assembly.LoadFile(args[0]);module=assembly.ManifestModule;methods=module.GetMethods(Flags);
            InstallFileBridge(args[2]);
            foreach(string name in new string[] {"gaEntityManager.create","gaNodeManager.create","gaRaceConfig.create","gaSegmentManager.create"})
            { Console.WriteLine("CALL="+name);Call(name); }
            IntPtr race=Global("?ms_pInstance@gaRaceConfig@@0PAV1@A");
            string genre=File.ReadAllText(Path.Combine(args[2],"genre.txt")).Trim();
            foreach(int offset in new int[] {108,132,156,180}) SetShortString(race+offset,genre);
            Console.WriteLine("CALL=gaTrackManager.create");Call("gaTrackManager.create");
            if(ioError!=null) throw new IOException(ioError);
            IntPtr generator=Global("?ms_pInstance@gaTrackGen@@0PAV1@A");
            IntPtr analysis=Json(Path.Combine(args[2],"analysis.json"));IntPtr profile=Json(Path.Combine(args[2],"profile.json"));
            string[] times=File.ReadAllLines(Path.Combine(args[2],"timing.txt"));
            Console.WriteLine("CALL=gaTrackGen.initFromJson");
            Call("gaTrackGen.initFromJson",generator,analysis,profile,float.Parse(times[0],Invariant),float.Parse(times[1],Invariant),float.Parse(times[2],Invariant));
            Console.WriteLine("GENERATION_RETURNED");
            IntPtr managerPtr=Global("?s_instance@gaNodeManager@@0PAV1@A");int* manager=(int*)managerPtr;
            int bytes=manager[2]-manager[1];if(bytes%244!=0 || bytes<=0) throw new IOException("Invalid node vector");
            Directory.CreateDirectory(args[3]);Dump((IntPtr)manager[1],bytes,Path.Combine(args[3],"original-nodes.bin"));
            // getSplineTForSongTime only reads this object's cached first-beat
            // pointer. Provide that input without starting entity/game systems.
            Console.WriteLine("CALL=gaNodeManager.fixupNetwork (timing context)");
            Call("gaNodeManager.fixupNetwork",managerPtr);
            IntPtr timingManager=Alloc(432),distanceOut=Alloc(4);
            int firstNode=((int*)generator)[88];
            *(int*)((byte*)timingManager+428)=manager[1]+firstNode*244;
            var timing=new StringBuilder();float beatSpan=((float*)generator)[40];
            for(int i=0;i<=100;i++)
            {
                float seconds=(float)((double)beatSpan*i/100.0);
                float parameter=(float)Call("gaTrackEntityManager.getSplineTForSongTime",timingManager,seconds,IntPtr.Zero,distanceOut);
                timing.Append(F(seconds)+"\t"+F(parameter)+"\t"+F(*(float*)distanceOut)+"\n");
            }
            File.WriteAllText(Path.Combine(args[3],"original-timing.tsv"),timing.ToString());
            // Scalar vehicle helpers read only explicitly supplied plain data.
            // This does not initialize a rigid body or certify full handling.
            IntPtr parameters=Alloc(1024),vehicle=Alloc(2048),gearbox=Alloc(128);
            float* values=(float*)parameters;
            values[5]=30f;values[11]=6f;values[9]=0f;values[129]=8f;
            Call("gaVehicleParams.updateInterpolatedValues",parameters);
            *(int*)((byte*)vehicle+332)=(int)parameters;
            var steering=new StringBuilder();
            foreach(float speed in new float[] {-80f,0f,5f,10f,30f,100f})
            foreach(float drift in new float[] {0f,.5f,1f})
            foreach(float input in new float[] {-1f,1f})
            foreach(float bias in new float[] {-.5f,0f,.5f})
            {
                ((float*)vehicle)[276]=drift;((float*)vehicle)[268]=input;((float*)vehicle)[284]=bias;
                float result=(float)Call("gaVehiclePhys.phys_calcSteer",vehicle,speed);
                steering.Append(F(speed)+"\t"+F(drift)+"\t"+F(input)+"\t"+F(bias)+"\t"+F(result)+"\n");
            }
            File.WriteAllText(Path.Combine(args[3],"original-steering.tsv"),steering.ToString());
            var brakes=new StringBuilder();
            foreach(bool toggle in new bool[] {false,true})
            foreach(int gear in new int[] {-1,0,1})
            foreach(float speed in new float[] {-2f,-.2f,0f,.2f,2f})
            foreach(float input in new float[] {-1f,0f,1f})
            {
                ((byte*)gearbox)[24]=(byte)(toggle?1:0);((int*)gearbox)[7]=gear;((float*)gearbox)[8]=speed;
                float result=(float)Call("gaVehicleGearbox.determineBrakeInput",gearbox,input);
                brakes.Append((toggle?1:0)+"\t"+gear+"\t"+F(speed)+"\t"+F(input)+"\t"+F(result)+"\n");
            }
            File.WriteAllText(Path.Combine(args[3],"original-braking.tsv"),brakes.ToString());
            CaptureVehicleBehavior(args[2],args[3]);
            int* sm=(int*)Global("?ms_pInstance@gaSegmentManager@@0PAV1@A");
            int segments=(sm[6]-sm[5])/80;
            var text=new StringBuilder();
            for(int i=0;i<segments;i++)
            {
                byte* p=(byte*)sm[5]+i*80;byte* name=*(uint*)(p+72)<16?p+52:*(byte**)(p+52);
                text.Append(i+"\t"+((int*)p)[0]+"\t"+((int*)p)[1]+"\t"+((int*)p)[3]+"\t"+((int*)p)[4]);
                for(int j=5;j<=8;j++) text.Append("\t"+F(((float*)p)[j]));
                text.Append("\t"+Marshal.PtrToStringAnsi((IntPtr)name)+"\n");
            }
            File.WriteAllText(Path.Combine(args[3],"original-segments.tsv"),text.ToString());
            int* gen=(int*)generator;Dump((IntPtr)gen[60],gen[61]-gen[60],Path.Combine(args[3],"original-sections.bin"));
            File.WriteAllText(Path.Combine(args[3],"capture-state.txt"),"seed="+((uint*)gen)[90]+"\nstart="+gen[88]+"\nfinish="+gen[89]+"\nspeed="+F(((float*)Global("?ms_pInstance@gaTrackManager@@0PAV1@A"))[684])+"\nspacing="+F(((float*)gen)[71])+"\n");
            Console.WriteLine("NODE_COUNT="+(bytes/244)+" SEGMENTS="+segments);
            Console.WriteLine("RESULT=SONG_CAPTURE_OK");
            return 0;
        }
        catch(Exception error) { Console.WriteLine(error);Console.WriteLine("RESULT=EXCEPTION");return 1; }
    }
}
