using dnlib.DotNet;
using dnlib.DotNet.Emit;
using dnlib.DotNet.Writer;

if (args.Length > 0 && args[0] == "--language-component") { Environment.ExitCode=LanguageComponent.Run(args.Skip(1).ToArray()); return; }

#if NET8_0_OR_GREATER
if (args.Length > 0 && args[0] == "--client-host") {
    if (!OperatingSystem.IsLinux()) throw new PlatformNotSupportedException("Linux client helper required.");
    Environment.ExitCode = LinuxClientHost.Run(args.Skip(1).ToArray());
    return;
}
#endif

if (args.Length == 4 && args[0] == "--retarget-helpers") {
    using var source = ModuleDefMD.Load(args[1]);
    int updated = 0;
    foreach (var reference in source.GetAssemblyRefs()) {
        string name = reference.Name.String;
        if (name != "ReRiff.ClientConfig" && name != "ReRiff.NewsHeaders") continue;
        using var target = ModuleDefMD.Load(Path.Combine(args[2], name + ".dll"));
        if (!target.Assembly.HasPublicKey)
            throw new InvalidOperationException("Helper must be fully strong-name signed: " + name);
        reference.PublicKeyOrToken = target.Assembly.PublicKey.Token;
        reference.Attributes &= ~AssemblyAttributes.PublicKey;
        updated++;
    }
    if(source.Assembly.Name=="ParseOctane") InstallMetadata(source,Path.Combine(args[2],"ReRiff.NewsHeaders.dll"));
    if(source.Assembly.Name=="Parse.NetFx45") InstallClientNews(source,args[2]);
    if (updated == 0) throw new InvalidOperationException("No existing helper references found.");
    if (source.IsILOnly) {
        var writer = new ModuleWriterOptions(source);
        writer.MetadataOptions.Flags |= MetadataFlags.PreserveAll;
        source.Write(args[3], writer);
    } else {
        var writer = new NativeModuleWriterOptions(source, optimizeImageSize: false);
        writer.MetadataOptions.Flags |= MetadataFlags.PreserveAll;
        source.NativeWrite(args[3], writer);
    }
    Console.WriteLine("Retargeted existing client hooks to signed ReRiff helpers.");
    return;
}

if (args.Length == 3 && args[0] == "--inspect-client") {
    using var endpoint = ModuleDefMD.Load(Path.Combine(args[1], "ParseOctane.dll"));
    using var transport = ModuleDefMD.Load(Path.Combine(args[1], "Parse.NetFx45.dll"));
    bool Calls(ModuleDefMD source, string name) => source.GetTypes().SelectMany(t => t.Methods)
        .Where(m => m.HasBody).SelectMany(m => m.Body.Instructions)
        .Any(i => i.Operand is IMethod method && method.FullName.Contains(name));
    File.WriteAllText(args[2], "{\"endpoint\":" + Calls(endpoint, "ReRiff.ClientConfig::GetServer").ToString().ToLowerInvariant()
        + ",\"offline\":" + Calls(transport, "ReRiff.ClientConfig::CanSend").ToString().ToLowerInvariant()
        + ",\"news\":" + Calls(transport, "ReRiff.NewsHeaders::Add").ToString().ToLowerInvariant() + "}");
    return;
}

if (args.Length == 4 && args[0] == "--metadata-utf8") {
    using var bridge=ModuleDefMD.Load(args[1]);using var textHelper=ModuleDefMD.Load(args[2]);
    if(bridge.Assembly.Name!="ParseOctane"||textHelper.Assembly.Name!="ReRiff.NewsHeaders"||!textHelper.Assembly.HasPublicKey)
        throw new InvalidOperationException("Expected original bridge and signed ReRiff metadata helper.");
    InstallMetadataBoundary(bridge,textHelper);
    var writer=new NativeModuleWriterOptions(bridge,optimizeImageSize:false);writer.MetadataOptions.Flags|=MetadataFlags.PreserveAll;
    bridge.NativeWrite(args[3],writer);
    Console.WriteLine("Installed opt-in UTF-8 song metadata boundary; original native buffer copy/free retained.");return;
}

if (args.Length == 4 && (args[0] == "--metadata-query-utf8"||args[0] == "--metadata-range-query-utf8")) {
    using var bridge=ModuleDefMD.Load(args[1]);using var queryHelper=ModuleDefMD.Load(args[2]);
    if(bridge.Assembly.Name!="ParseOctane"||queryHelper.Assembly.Name!="ReRiff.NewsHeaders"||!queryHelper.Assembly.HasPublicKey)
        throw new InvalidOperationException("Expected bridge and signed metadata helper.");
    InstallMetadataQueries(bridge,queryHelper,args[0]=="--metadata-range-query-utf8");
    var writer=new NativeModuleWriterOptions(bridge,optimizeImageSize:false);writer.MetadataOptions.Flags|=MetadataFlags.PreserveAll;
    bridge.NativeWrite(args[3],writer);Console.WriteLine("Installed field-scoped UTF-8 object equality queries; user queries and opaque fields retained.");return;
}

if (args.Length == 4 && args[0] == "--metadata-trace") {
    using var sdk=ModuleDefMD.Load(args[1]);using var metadataHelper=ModuleDefMD.Load(args[2]);
    var execute=sdk.Types.Single(t=>t.FullName=="Parse.Internal.HttpClient").Methods.Single(m=>m.Name=="ExecuteAsync");
    var wrapper=metadataHelper.Types.Single(t=>t.FullName=="ReRiff.NewsHeaders").Methods.Single(m=>m.Name=="TraceMetadata");
    var observe=metadataHelper.Types.Single(t=>t.FullName=="ReRiff.NewsHeaders").Methods.Single(m=>m.Name=="ObserveMetadataRequest");
    if(execute.ReturnType.FullName!=wrapper.Parameters[0].Type.FullName)throw new InvalidOperationException("Unexpected transport task type.");
    if(execute.Body.Instructions.Any(i=>i.Operand is IMethod m&&m.FullName.Contains("::TraceMetadata")))throw new InvalidOperationException("Metadata trace already installed.");
    execute.Body.Instructions.Insert(0,Instruction.Create(OpCodes.Ldarg_1));
    execute.Body.Instructions.Insert(1,Instruction.Create(OpCodes.Call,sdk.Import(observe)));
    foreach(var ret in execute.Body.Instructions.Where(i=>i.OpCode==OpCodes.Ret).ToArray()){
        int at=execute.Body.Instructions.IndexOf(ret);ret.OpCode=OpCodes.Ldarg_1;ret.Operand=null;
        execute.Body.Instructions.Insert(++at,Instruction.Create(OpCodes.Call,sdk.Import(wrapper)));
        execute.Body.Instructions.Insert(++at,Instruction.Create(OpCodes.Ret));
    }
    execute.Body.SimplifyBranches();execute.Body.OptimizeBranches();
    var writer=new ModuleWriterOptions(sdk);writer.MetadataOptions.Flags|=MetadataFlags.PreserveAll;sdk.Write(args[3],writer);
    Console.WriteLine("Installed opt-in metadata observation; original tasks and streams retained.");return;
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
    InstallClientNews(sdk,Path.GetDirectoryName(args[2])!);
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
InstallMetadata(module,Path.Combine(Path.GetDirectoryName(args[1])!,"ReRiff.NewsHeaders.dll"));
var options = new NativeModuleWriterOptions(module, optimizeImageSize: false);
options.MetadataOptions.Flags |= MetadataFlags.PreserveAll;
module.NativeWrite(args[2], options);
using var check = ModuleDefMD.Load(args[2]);
var patched = check.Types.Single(t => t.Name == "wsNetworkParseImpl").Methods.Single(m => m.Name == "open");
if (!patched.Body.Instructions.Any(i => i.Operand is IMethod m && m.FullName.Contains("ReRiff.ClientConfig::GetServer")))
    throw new InvalidOperationException("Patch verification failed.");
Console.WriteLine("Verified: ParseOctane.open calls ReRiff.ClientConfig.GetServer directly.");


static void InstallClientNews(ModuleDefMD sdk,string helperFolder) {
    var execute=sdk.Types.Single(t=>t.FullName=="Parse.Internal.HttpClient").Methods.Single(m=>m.Name=="ExecuteAsync");
    if(execute.Body.Instructions.Any(i=>i.Operand is IMethod m&&m.FullName.Contains("ReRiff.NewsHeaders::WithClientNews")))return;
    using var helper=ModuleDefMD.Load(Path.Combine(helperFolder,"ReRiff.NewsHeaders.dll"));
    var wrapper=helper.Types.Single(t=>t.FullName=="ReRiff.NewsHeaders").Methods.Single(m=>m.Name=="WithClientNews");
    if(execute.ReturnType.FullName!=wrapper.Parameters[0].Type.FullName)
        throw new InvalidOperationException("Unexpected original Parse HTTP task result.");
    var uri=execute.Body.Instructions.Select(i=>i.Operand).OfType<IMethod>().First(m=>m.Name=="get_Uri"&&m.DeclaringType.FullName=="Parse.Internal.HttpRequest");
    var method=execute.Body.Instructions.Select(i=>i.Operand).OfType<IMethod>().First(m=>m.Name=="get_Method"&&m.DeclaringType.FullName=="Parse.Internal.HttpRequest");
    var returns=execute.Body.Instructions.Where((i,index)=>i.OpCode==OpCodes.Ret && !(index>0 && execute.Body.Instructions[index-1].Operand is IMethod previous && previous.FullName.Contains("::OfflineTask"))).ToArray();
    foreach(var ret in returns){
        // Leave the original task on the stack, including its cancellation/errors.
        // Mutate the return instruction so existing branches also run the wrapper.
        int at=execute.Body.Instructions.IndexOf(ret);ret.OpCode=OpCodes.Ldarg_1;ret.Operand=null;
        foreach(var instruction in new[]{Instruction.Create(OpCodes.Callvirt,sdk.Import(uri)),Instruction.Create(OpCodes.Ldarg_1),Instruction.Create(OpCodes.Callvirt,sdk.Import(method)),Instruction.Create(OpCodes.Call,sdk.Import(wrapper)),Instruction.Create(OpCodes.Ret)})
            execute.Body.Instructions.Insert(++at,instruction);
    }
    execute.Body.SimplifyBranches();execute.Body.OptimizeBranches();
}

static void InstallMetadata(ModuleDefMD bridge,string path){using var helper=ModuleDefMD.Load(path);if(!helper.Assembly.HasPublicKey)throw new InvalidOperationException("Unsigned metadata helper.");InstallMetadataBoundary(bridge,helper);InstallMetadataQueries(bridge,helper,false);InstallMetadataQueries(bridge,helper,true);}
static void InstallMetadataBoundary(ModuleDefMD bridge,ModuleDefMD textHelper){
    var owner=bridge.Types.Single(t=>t.Name=="wsNetworkParseImpl");
    var write=owner.Methods.Single(m=>m.MDToken.Raw==0x060000de);
    var read=owner.Methods.Single(m=>m.MDToken.Raw==0x060000fc);
    var operations=write.Body.Instructions;
    if(operations.Any(i=>i.Operand is IMethod m&&m.FullName.Contains("ReRiff.MetadataText::DecodeValue"))){
        if(operations.Count(i=>i.Operand is IMethod m&&m.FullName.Contains("ReRiff.MetadataText::DecodeValue"))!=1
            ||read.Body.Instructions.Count(i=>i.Operand is IMethod m&&m.FullName.Contains("ReRiff.MetadataText::AllocateValue"))!=1)
            throw new InvalidOperationException("Partial metadata boundary patch.");
        return;
    }
    bool AnsiCtor(Instruction i)=>i.OpCode==OpCodes.Newobj&&i.Operand is IMethod method
        &&method.DeclaringType.FullName=="System.String"&&method.Name==".ctor"
        &&method.MethodSig.Params.Count==1&&method.MethodSig.Params[0].FullName=="System.SByte*";
    if(write.Name!="updateObject"||read.Name!="refreshObjectFromString"||operations.Count!=10
        ||!AnsiCtor(operations[5])||!AnsiCtor(operations[7])||operations[4].OpCode!=OpCodes.Ldarg_2
        ||operations[6].OpCode!=OpCodes.Ldarg_3||operations[8].Operand is not IMethod metadataSetter
        ||metadataSetter.FullName!="System.Void Parse.ParseObject::set_Item(System.String,System.Object)")
        throw new InvalidOperationException("Original metadata-write instructions differ; refusing patch.");
    var allocate=read.Body.Instructions.Single(i=>i.Operand is IMethod method
        &&method.FullName=="System.IntPtr System.Runtime.InteropServices.Marshal::StringToHGlobalAnsi(System.String)");
    if(!read.Body.Instructions.Any(i=>i.Operand is IMethod method&&method.FullName=="System.Void System.Runtime.InteropServices.Marshal::FreeHGlobal(System.IntPtr)"))
        throw new InvalidOperationException("Original metadata-read ownership differs; refusing patch.");
    var helperType=textHelper.Types.Single(t=>t.FullName=="ReRiff.MetadataText");
    var decode=helperType.Methods.Single(m=>m.Name=="DecodeValue");
    var encode=helperType.Methods.Single(m=>m.Name=="AllocateValue");
    // Mutate existing instruction objects so branch targets and exception
    // boundaries continue entering the complete replacement operation.
    operations[7].OpCode=OpCodes.Conv_I;operations[7].Operand=null;
    operations.Insert(8,Instruction.Create(OpCodes.Ldarg_2));operations.Insert(9,Instruction.Create(OpCodes.Conv_I));
    operations.Insert(10,Instruction.Create(OpCodes.Call,bridge.Import(decode)));
    int at=read.Body.Instructions.IndexOf(allocate);allocate.OpCode=OpCodes.Ldarg_2;allocate.Operand=null;
    read.Body.Instructions.Insert(++at,Instruction.Create(OpCodes.Conv_I));
    read.Body.Instructions.Insert(++at,Instruction.Create(OpCodes.Call,bridge.Import(encode)));
    write.Body.SimplifyBranches();write.Body.OptimizeBranches();read.Body.SimplifyBranches();read.Body.OptimizeBranches();
}
static void InstallMetadataQueries(ModuleDefMD bridge,ModuleDefMD queryHelper,bool range){
    var decode=queryHelper.Types.Single(t=>t.FullName=="ReRiff.MetadataText").Methods.Single(m=>m.Name=="DecodeValue");
    bool Ansi(Instruction i)=>i.OpCode==OpCodes.Newobj&&i.Operand is IMethod m&&m.DeclaringType.FullName=="System.String"
        &&m.Name==".ctor"&&m.MethodSig.Params.Count==1&&m.MethodSig.Params[0].FullName=="System.SByte*";
    var querySites=range
        ?new[]{new {Token=0x060000f2u,Name="findAllObjectsTypeWhereRange",Count=2},new {Token=0x060000f3u,Name="findAllObjectsTypeWhereRange",Count=1}}
        :new[]{new {Token=0x060000f0u,Name="findAllObjectsTypeWhere",Count=4},new {Token=0x060000f4u,Name="findSortedObjectsTypeWhere",Count=1}};
    foreach(var item in querySites){
        var method=(MethodDef)bridge.ResolveToken(item.Token);var body=method.Body.Instructions;
        if(method.DeclaringType.Name!="wsNetworkParseImpl"||method.Name!=item.Name)throw new InvalidOperationException("Query token differs.");
        var sites=body.Where(i=>i.OpCode==OpCodes.Call&&i.Operand is IMethod m
            &&m.FullName.Contains("Parse.ParseQuery`1<Parse.ParseObject>::WhereEqualTo(System.String,System.Object)")).ToArray();
        int existing=body.Count(i=>i.Operand is IMethod m&&m.FullName.Contains("ReRiff.MetadataText::DecodeValue"));
        if(existing==item.Count&&sites.Length==item.Count)continue;
        if(existing!=0)throw new InvalidOperationException("Partial metadata query patch.");
        if(sites.Length!=item.Count)throw new InvalidOperationException("Original equality-query count differs.");
        foreach(var site in sites){
            int at=body.IndexOf(site);if(at<4)throw new InvalidOperationException("Truncated query instructions.");
            var value=body[at-1];var key=body[at-4];
            bool Arg(Instruction i)=>i.OpCode.Code is Code.Ldarg_1 or Code.Ldarg_2 or Code.Ldarg_3 or Code.Ldarg_S or Code.Ldarg;
            if(at<4||!Ansi(value)||!Ansi(body[at-3])||!Arg(key)||!Arg(body[at-2]))
                throw new InvalidOperationException("Original query argument instructions differ; refusing duplicate or incompatible patch.");
            value.OpCode=OpCodes.Conv_I;value.Operand=null;
            body.Insert(at++,new Instruction(key.OpCode,key.Operand));body.Insert(at++,Instruction.Create(OpCodes.Conv_I));
            body.Insert(at,Instruction.Create(OpCodes.Call,bridge.Import(decode)));
        }
        method.Body.SimplifyBranches();method.Body.OptimizeBranches();
    }
}
