// Test-only extraction. Normalize pointer arguments to IntPtr; preserve IL.
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using dnlib.DotNet;
using dnlib.DotNet.Emit;
class ExtractCarFallbackFixture {
    static void Main(string[] args){
        byte[] bytes=File.ReadAllBytes(args[0]);
        if(Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant()!="2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2")throw new IOException("Original hash differs");
        using var source=ModuleDefMD.Load(bytes);
        var original=(MethodDef)source.ResolveToken(0x0600127c);
        if(original.Name!="gaSaveGameChunkSettings.setChosenCar"||original.Body.HasExceptionHandlers||original.Body.Variables.Count!=0)throw new IOException("Original method differs");
        var target=new ModuleDefUser("OriginalCarFallbackFixture.dll"){Kind=ModuleKind.Dll,RuntimeVersion="v4.0.30319"};
        target.Cor20HeaderFlags=dnlib.DotNet.MD.ComImageFlags.ILOnly|dnlib.DotNet.MD.ComImageFlags.Bit32Required;
        new AssemblyDefUser("OriginalCarFallbackFixture",new Version(1,0,0,0)).Modules.Add(target);
        var type=new TypeDefUser("Fixture","OriginalProgression",target.CorLibTypes.Object.TypeDefOrRef);target.Types.Add(type);
        var method=new MethodDefUser(original.Name,MethodSig.CreateStatic(target.CorLibTypes.Void,target.CorLibTypes.IntPtr,target.CorLibTypes.Int32),MethodImplAttributes.IL|MethodImplAttributes.Managed,MethodAttributes.Public|MethodAttributes.Static);
        type.Methods.Add(method);method.Body=new CilBody{MaxStack=original.Body.MaxStack};
        var map=new Dictionary<Instruction,Instruction>();
        foreach(var i in original.Body.Instructions){var copy=new Instruction(i.OpCode,null);map.Add(i,copy);method.Body.Instructions.Add(copy);}
        foreach(var i in original.Body.Instructions){
            object value=i.Operand;
            if(value is Instruction branch)value=map[branch];
            else if(value is Parameter parameter)value=method.Parameters[parameter.Index];
            else if(value!=null&&!(value is int)&&!(value is sbyte))throw new IOException("Unexpected operand");
            map[i].Operand=value;
        }
        target.Write(args[1]);Console.WriteLine("Extracted token 0600127c RVA 002fd39c from hash-pinned original; test-only pointer-normalized IL, no game startup");
    }
}
