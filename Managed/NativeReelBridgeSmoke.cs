using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

internal static class NativeReelBridgeSmoke {
    private delegate IntPtr CtorCaller(IntPtr function,IntPtr memory,IntPtr parent,float spacing,int direction);
    private delegate void AddCaller(IntPtr function,IntPtr memory,IntPtr option,IntPtr screen);
    private delegate void TickCaller(IntPtr function,IntPtr instance);
    private static DynamicMethod Caller(string name,Type result,Type[] arguments) {
        Type[] wrapper=new Type[arguments.Length+1];wrapper[0]=typeof(IntPtr);Array.Copy(arguments,0,wrapper,1,arguments.Length);
        var method=new DynamicMethod(name,result,wrapper,typeof(NativeReelBridgeSmoke).Module,true);
        ILGenerator il=method.GetILGenerator();
        for(int i=1;i<wrapper.Length;i++)il.Emit(OpCodes.Ldarg,(short)i);
        il.Emit(OpCodes.Ldarg_0);
        il.EmitCalli(OpCodes.Calli,CallingConvention.Cdecl,result,arguments);il.Emit(OpCodes.Ret);
        return method;
    }
    internal static void Run() {
        var assembly=AppDomain.CurrentDomain.DefineDynamicAssembly(new AssemblyName("ReRiffPointerFixture"),AssemblyBuilderAccess.Run);
        ModuleBuilder module=assembly.DefineDynamicModule("OriginalGlobalMethodFixture");
        Type pointer=typeof(byte).MakePointerType();
        MethodBuilder ctor=module.DefineGlobalMethod("ReelCtorFixture",MethodAttributes.Public|MethodAttributes.Static,
            pointer,new Type[]{pointer,pointer,typeof(float),typeof(int)});
        var il=ctor.GetILGenerator();
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldarg_1);il.Emit(OpCodes.Stind_I);
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldc_I4,8);il.Emit(OpCodes.Add);il.Emit(OpCodes.Ldarg_2);il.Emit(OpCodes.Stind_R4);
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldc_I4,16);il.Emit(OpCodes.Add);il.Emit(OpCodes.Ldarg_3);il.Emit(OpCodes.Stind_I4);
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ret);
        MethodBuilder add=module.DefineGlobalMethod("ReelAddFixture",MethodAttributes.Public|MethodAttributes.Static,
            typeof(void),new Type[]{pointer,pointer.MakePointerType(),pointer});
        il=add.GetILGenerator();il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldarg_1);il.Emit(OpCodes.Ldind_I);il.Emit(OpCodes.Stind_I);
        il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldc_I4,8);il.Emit(OpCodes.Add);il.Emit(OpCodes.Ldarg_2);il.Emit(OpCodes.Stind_I);il.Emit(OpCodes.Ret);
        MethodBuilder resize=module.DefineGlobalMethod("GraphicsResizeFixture",MethodAttributes.Public|MethodAttributes.Static,
            typeof(void),new Type[]{pointer});
        il=resize.GetILGenerator();il.Emit(OpCodes.Ldarg_0);il.Emit(OpCodes.Ldc_I4,31415);il.Emit(OpCodes.Stind_I4);il.Emit(OpCodes.Ret);
        module.CreateGlobalFunctions();
        Type bridge=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.NativeReelBridge",true);
        MethodInfo factory=bridge.GetMethod("CreateCall",BindingFlags.NonPublic|BindingFlags.Static);
        IntPtr memory=Marshal.AllocHGlobal(32);
        try {
            Delegate ctorAdapter=(Delegate)factory.Invoke(null,new object[]{module.GetMethod("ReelCtorFixture"),bridge.GetNestedType("Construct",BindingFlags.NonPublic),false});
            var ctorCaller=(CtorCaller)Caller("CtorCdeclRoundtrip",typeof(IntPtr),new Type[]{typeof(IntPtr),typeof(IntPtr),typeof(float),typeof(int)}).CreateDelegate(typeof(CtorCaller));
            if(ctorCaller(Marshal.GetFunctionPointerForDelegate(ctorAdapter),memory,new IntPtr(1234),211.5f,1)!=memory
                ||Marshal.ReadIntPtr(memory)!=new IntPtr(1234)||Marshal.ReadInt32(memory,8)!=BitConverter.ToInt32(BitConverter.GetBytes(211.5f),0)
                ||Marshal.ReadInt32(memory,16)!=1)throw new Exception("Native reel cdecl constructor transport failed");
            GC.KeepAlive(ctorAdapter);
            Delegate addAdapter=(Delegate)factory.Invoke(null,new object[]{module.GetMethod("ReelAddFixture"),bridge.GetNestedType("Add",BindingFlags.NonPublic),true});
            var addCaller=(AddCaller)Caller("AddCdeclRoundtrip",typeof(void),new Type[]{typeof(IntPtr),typeof(IntPtr),typeof(IntPtr)}).CreateDelegate(typeof(AddCaller));
            addCaller(Marshal.GetFunctionPointerForDelegate(addAdapter),memory,new IntPtr(5678),new IntPtr(9012));
            if(Marshal.ReadIntPtr(memory)!=new IntPtr(5678)||Marshal.ReadIntPtr(memory,8)!=new IntPtr(9012))
                throw new Exception("Native reel SSP indirection transport failed");
            GC.KeepAlive(addAdapter);
            Delegate resizeAdapter=(Delegate)factory.Invoke(null,new object[]{module.GetMethod("GraphicsResizeFixture"),bridge.GetNestedType("Tick",BindingFlags.NonPublic),false});
            var tickCaller=(TickCaller)Caller("GraphicsCdeclRoundtrip",typeof(void),new Type[]{typeof(IntPtr)}).CreateDelegate(typeof(TickCaller));
            tickCaller(Marshal.GetFunctionPointerForDelegate(resizeAdapter),memory);
            if(Marshal.ReadInt32(memory)!=31415)throw new Exception("Original graphics pointer callback transport failed");
            GC.KeepAlive(resizeAdapter);
        } finally {Marshal.FreeHGlobal(memory);}
        Console.WriteLine("PASS: original-style global pointer methods through cdecl callbacks, float arguments and owning SSP indirection.");
    }
}
