using dnlib.DotNet;
using dnlib.DotNet.Emit;
using dnlib.DotNet.Writer;

if (args.Length == 3 && args[0] == "--inspect-client") {
    using var endpoint = ModuleDefMD.Load(Path.Combine(args[1], "ParseOctane.dll"));
    using var transport = ModuleDefMD.Load(Path.Combine(args[1], "Parse.NetFx45.dll"));
    bool Calls(ModuleDefMD source, string name) => source.GetTypes().SelectMany(t => t.Methods)
        .Where(m => m.HasBody).SelectMany(m => m.Body.Instructions)
        .Any(i => i.Operand is IMethod method && method.FullName.Contains(name));
    File.WriteAllText(args[2], System.Text.Json.JsonSerializer.Serialize(new {
        endpoint = Calls(endpoint, "ReRiff.ClientConfig::GetServer"),
        offline = Calls(transport, "ReRiff.ClientConfig::CanSend"),
        news = Calls(transport, "ReRiff.NewsHeaders::Add")
    }));
    return;
}

if (args.Length == 4 && args[0] == "--news-header") {
    using var sdk = ModuleDefMD.Load(args[1]);
    using var headers = ModuleDefMD.Load(args[2]);
    var execute = sdk.Types.Single(t => t.FullName == "Parse.Internal.HttpClient").Methods.Single(m => m.Name == "ExecuteAsync");
    var body = execute.Body.Instructions;
    if (body.Any(i => i.Operand is IMethod m && m.FullName.Contains("ReRiff.NewsHeaders::Add")))
        throw new InvalidOperationException("News header already installed; refusing duplicate injection.");
    var marker = body.Single(i => i.OpCode == OpCodes.Ldstr && Equals(i.Operand, "Cache-Control"));
    int headerIndex = body.IndexOf(marker);
    if (headerIndex < 3 || headerIndex+2 >= body.Count || body[headerIndex-2].OpCode != OpCodes.Ldfld
        || body[headerIndex-1].Operand is not IMethod getter
        || getter.Name != "get_Headers" || body[headerIndex+2].Operand is not IMethod headerSetter || headerSetter.Name != "set_Item")
        throw new InvalidOperationException("Unexpected transport header setup; refusing patch.");
    var add = headers.Types.Single(t => t.FullName == "ReRiff.NewsHeaders").Methods.Single(m => m.Name == "Add");
    body.Insert(headerIndex+3, body[headerIndex-3].Clone());
    body.Insert(headerIndex+4, body[headerIndex-2].Clone());
    body.Insert(headerIndex+5, Instruction.Create(OpCodes.Call, sdk.Import(add)));
    execute.Body.SimplifyBranches(); execute.Body.OptimizeBranches();
    var headerOptions = new ModuleWriterOptions(sdk); headerOptions.MetadataOptions.Flags |= MetadataFlags.PreserveAll;
    sdk.Write(args[3], headerOptions);
    using var headerCheck = ModuleDefMD.Load(args[3]);
    var verified = headerCheck.Types.Single(t => t.FullName == "Parse.Internal.HttpClient").Methods.Single(m => m.Name == "ExecuteAsync");
    if (verified.Body.Instructions.Count(i => i.Operand is IMethod m && m.FullName.Contains("ReRiff.NewsHeaders::Add")) != 1)
        throw new InvalidOperationException("News header verification failed.");
    Console.WriteLine("Verified: client version header added after existing transport header setup.");
    return;
}

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
