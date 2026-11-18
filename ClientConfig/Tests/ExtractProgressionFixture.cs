// Test-only extraction from the hash-pinned original. Never shipped with a
// client. Instruction opcodes/operands are retained; pointer signatures and
// locals become native ints, and internal call tokens are remapped. This is
// an execution fixture, not a matching-byte reconstruction.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using dnlib.DotNet;
using dnlib.DotNet.Emit;

class ExtractProgressionFixture {
    static readonly uint[] Tokens={0x06001254,0x06000e82,0x0600124c,0x0600124d,0x0600124e,0x0600124f,0x0600125b,0x0600125c,0x0600125d,0x0600125e,0x0600138b,0x06001036,0x06001263,0x0600138d,0x0600138e,0x06001259,0x060005d4,0x060007ac};
    static TypeSig Normalize(ModuleDef target,TypeSig type){
        type=type.RemovePinnedAndModifiers();
        if(type.FullName=="gaAchievements/AchievementTypes"){
            var definition=type.ToTypeDefOrRef().ResolveTypeDef();
            if(definition==null||!definition.IsEnum||definition.FindField("value__").FieldType.ElementType!=ElementType.I4)throw new IOException("Original achievement enum differs");
            return target.CorLibTypes.Int32;
        }
        switch(type.ElementType){
            case ElementType.Ptr: case ElementType.I:return target.CorLibTypes.IntPtr;
            case ElementType.Void:return target.CorLibTypes.Void;
            case ElementType.Boolean:return target.CorLibTypes.Boolean;
            case ElementType.U1:return target.CorLibTypes.Byte;
            case ElementType.R4:return target.CorLibTypes.Single;
            case ElementType.R8:return target.CorLibTypes.Double;
            case ElementType.I4:return target.CorLibTypes.Int32;
            case ElementType.U4:return target.CorLibTypes.UInt32;
            default:throw new InvalidOperationException("Unreviewed fixture type: "+type);
        }
    }
    static void Main(string[] args){
        byte[] original=File.ReadAllBytes(args[0]);string digest;
        using(var sha=SHA256.Create())digest=BitConverter.ToString(sha.ComputeHash(original)).Replace("-","").ToLowerInvariant();
        if(digest!="2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2")throw new IOException("Unsupported original Game.exe");
        using(var source=ModuleDefMD.Load(original)){
            var target=new ModuleDefUser("OriginalProgressionFixture.dll"){Kind=ModuleKind.Dll,RuntimeVersion="v4.0.30319"};
            target.Cor20HeaderFlags=dnlib.DotNet.MD.ComImageFlags.ILOnly|dnlib.DotNet.MD.ComImageFlags.Bit32Required;
            new AssemblyDefUser("OriginalProgressionFixture",new Version(1,0,0,0)).Modules.Add(target);
            var type=new TypeDefUser("Fixture","OriginalProgression",target.CorLibTypes.Object.TypeDefOrRef);
            target.Types.Add(type);
            var saveRoot=new FieldDefUser("SaveRoot",new FieldSig(target.CorLibTypes.IntPtr),FieldAttributes.Public|FieldAttributes.Static);type.Fields.Add(saveRoot);
            foreach(var item in new[]{new{Token=0x0400126fU,Name="?ms_instance@gaSaveGame@@0PAV1@A"},new{Token=0x04000db1U,Name="?ms_pInstance@gaAchievements@@0PAV1@A"},new{Token=0x0400147dU,Name="?ms_pInstance@gaVehiclePaintManager@@0PAV1@A"}}){
                var field=source.ResolveToken(item.Token) as FieldDef;
                if(field==null||!field.IsStatic||field.Name!=item.Name||field.FieldType.RemovePinnedAndModifiers().ElementType!=ElementType.Ptr)throw new IOException("Original singleton signature changed");
                Console.WriteLine("Original singleton {0:x8}: {1}, pointer type verified",item.Token,item.Name);
            }
            var methods=new Dictionary<uint,MethodDef>();
            foreach(uint token in Tokens){
                var old=source.ResolveToken(token) as MethodDef;
                if(old==null||!old.IsStatic||old.Body==null||old.Body.HasExceptionHandlers)throw new IOException("Unsupported original body");
                var signature=MethodSig.CreateStatic(Normalize(target,old.MethodSig.RetType),old.MethodSig.Params.Select(p=>Normalize(target,p)).ToArray());
                var next=new MethodDefUser(old.Name,signature,MethodImplAttributes.IL|MethodImplAttributes.Managed,MethodAttributes.Public|MethodAttributes.Static);
                type.Methods.Add(next);methods.Add(token,next);
            }
            foreach(uint token in Tokens){
                var old=(MethodDef)source.ResolveToken(token);var next=methods[token];var body=new CilBody{InitLocals=old.Body.InitLocals,MaxStack=old.Body.MaxStack};next.Body=body;
                foreach(var local in old.Body.Variables)body.Variables.Add(new Local(Normalize(target,local.Type)));
                var map=new Dictionary<Instruction,Instruction>();
                foreach(var instruction in old.Body.Instructions){var copy=new Instruction(instruction.OpCode,null);body.Instructions.Add(copy);map.Add(instruction,copy);}
                foreach(var instruction in old.Body.Instructions){
                    object operand=instruction.Operand;
                    if(operand is Instruction)operand=map[(Instruction)operand];
                    else if(operand is IList<Instruction>)operand=((IList<Instruction>)operand).Select(i=>map[i]).ToArray();
                    else if(operand is Local)operand=body.Variables[((Local)operand).Index];
                    else if(operand is Parameter)operand=next.Parameters[((Parameter)operand).Index];
                    else if(operand is IMethod){
                        var reference=(IMethod)operand;
                        if(token==0x060007ac&&instruction.Offset==7&&reference.FullName=="System.Double System.Math::Floor(System.Double)")
                            operand=target.Import(typeof(Math).GetMethod("Floor",new[]{typeof(double)}));
                        else {
                            var called=reference.ResolveMethodDef();
                            if(called==null||!methods.ContainsKey(called.MDToken.Raw))throw new IOException("External call forbidden in fixture: "+operand);
                            operand=methods[called.MDToken.Raw];
                        }
                    }else if(operand is IField){
                        var field=((IField)operand).ResolveFieldDef();
                        if(field==null||field.MDToken.Raw!=0x0400126f)throw new IOException("External field forbidden in fixture");
                        operand=saveRoot;
                    }else if(operand!=null&&!(operand is int)&&!(operand is sbyte)&&!(operand is byte)&&!(operand is long)&&!(operand is float)&&!(operand is double))throw new IOException("Unreviewed fixture operand: "+operand);
                    map[instruction].Operand=operand;
                }
                Console.WriteLine("Original token {0:x8}, RVA {1:x8}: {2}, {3} instructions",token,(uint)old.RVA,old.Name,body.Instructions.Count);
            }
            // Deliberately no file-system/Steam implementation in this fixture.
            // The adapter's save binding is exercised against an explicit stub;
            // the native save path is covered by separate live checkpoint tests.
            var save=new MethodDefUser("gaSaveGame.save",MethodSig.CreateStatic(target.CorLibTypes.Boolean,target.CorLibTypes.IntPtr),MethodImplAttributes.IL|MethodImplAttributes.Managed,MethodAttributes.Public|MethodAttributes.Static);
            var observer=new FieldDefUser("SaveObserver",new FieldSig(target.Import(typeof(Func<IntPtr,bool>)).ToTypeSig()),FieldAttributes.Public|FieldAttributes.Static);type.Fields.Add(observer);
            save.Body=new CilBody();save.Body.Instructions.Add(Instruction.Create(OpCodes.Ldsfld,observer));save.Body.Instructions.Add(Instruction.Create(OpCodes.Ldarg_0));
            save.Body.Instructions.Add(Instruction.Create(OpCodes.Callvirt,target.Import(typeof(Func<IntPtr,bool>).GetMethod("Invoke"))));save.Body.Instructions.Add(Instruction.Create(OpCodes.Ret));type.Methods.Add(save);
            target.Write(args[1]);
            Console.WriteLine("Save method is a test stub; no native save, file or Steam calls are included.");
            Console.WriteLine("Fixture source SHA256="+digest+"; test-only remapped IL, no matching-byte claim.");
        }
    }
}
