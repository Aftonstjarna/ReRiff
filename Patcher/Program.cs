using dnlib.DotNet;
using dnlib.DotNet.Emit;
using dnlib.DotNet.Writer;

if (args.Length == 4 && args[0] == "--offline-gate") {
    using var sdk = ModuleDefMD.Load(args[1]);
    using var config = ModuleDefMD.Load(args[2]);
    var type = config.Types.Single(t => t.FullName == "ReRiff.ClientConfig");
    var execute = sdk.Types.Single(t => t.FullName == "Parse.Internal.HttpClient").Methods.Single(m => m.Name == "ExecuteAsync");
    var result = ((GenericInstSig)execute.ReturnType).GenericArguments.Single();
    var offline = new MethodSpecUser(sdk.Import(type.Methods.Single(m => m.Name == "OfflineTask")), new GenericInstMethodSig(result));
    var first = execute.Body.Instructions[0];
    var gate = new[] {
        Instruction.Create(OpCodes.Call, sdk.Import(type.Methods.Single(m => m.Name == "CanSend"))),
        Instruction.Create(OpCodes.Brtrue, first),
        Instruction.Create(OpCodes.Call, offline),
        Instruction.Create(OpCodes.Ret),
    };
    for (int i = 0; i < gate.Length; i++) execute.Body.Instructions.Insert(i, gate[i]);
    execute.Body.SimplifyBranches();
    execute.Body.OptimizeBranches();
    var writer = new ModuleWriterOptions(sdk);
    writer.MetadataOptions.Flags |= MetadataFlags.PreserveAll;
    sdk.Write(args[3], writer);
    Console.WriteLine("Installed offline gate before Parse HTTP request creation.");
    return;
}
if (args.Length != 3)
    throw new ArgumentException("Usage: Patcher ORIGINAL-ParseOctane.dll ReRiff.ClientConfig.dll OUTPUT.dll");
using var module = ModuleDefMD.Load(args[0]);
using var helper = ModuleDefMD.Load(args[1]);
var open = module.Types.Single(t => t.Name == "wsNetworkParseImpl").Methods.Single(m => m.Name == "open");
var il = open.Body.Instructions;
var setter = il.Single(i => i.Operand is IMethod m && m.Name == "set_Server");
var index = il.IndexOf(setter);
if (index < 2 || il[index - 1].OpCode != OpCodes.Newobj || il[index - 2].OpCode != OpCodes.Ldsfld)
    throw new InvalidOperationException("Unexpected endpoint initialization; refusing to patch.");
var resolve = helper.Types.Single(t => t.FullName == "ReRiff.ClientConfig").Methods.Single(m => m.Name == "GetServer");
il[index - 2].OpCode = OpCodes.Call;
il[index - 2].Operand = module.Import(resolve);
il[index - 1].OpCode = OpCodes.Nop;
il[index - 1].Operand = null;
open.Body.SimplifyBranches();
open.Body.OptimizeBranches();
var options = new NativeModuleWriterOptions(module, optimizeImageSize: false);
options.MetadataOptions.Flags |= MetadataFlags.PreserveAll;
module.NativeWrite(args[2], options);
using var check = ModuleDefMD.Load(args[2]);
var patched = check.Types.Single(t => t.Name == "wsNetworkParseImpl").Methods.Single(m => m.Name == "open");
if (!patched.Body.Instructions.Any(i => i.Operand is IMethod m && m.FullName.Contains("ReRiff.ClientConfig::GetServer")))
    throw new InvalidOperationException("Patch verification failed.");
Console.WriteLine("Verified: ParseOctane.open calls ReRiff.ClientConfig.GetServer directly.");
