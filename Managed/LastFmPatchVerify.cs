// Build-tool test: verify startup CIL edit and stage a network-free query fixture.
using System;using System.IO;using System.Linq;using System.Collections.Generic;
using System.Text.RegularExpressions;using dnlib.DotNet;using dnlib.DotNet.Emit;using dnlib.DotNet.Writer;
class LastFmPatchVerify {
 static void Check(bool ok,string message){if(!ok)throw new Exception(message);}
 static byte[] Array(string source,string name){return Regex.Match(source,name+@"\[\]=\{([^}]+)\}").Groups[1].Value.Split(',').Select(x=>x.Trim()).Where(x=>x.Length>0).Select(x=>Convert.ToByte(x,x.StartsWith("0x")?16:10)).ToArray();}
 static string Body(MethodDef method){return string.Join("\n",method.Body.Instructions.Select(i=>i.ToString()));}
 static void Main(string[] args){
  byte[] game=File.ReadAllBytes(args[0]);using var original=ModuleDefMD.Load(game);
  var method=(MethodDef)original.ResolveToken(0x060014de);Check(method.Name=="gaNewTrackScreen.updateEssentiaLastFM","Original handler token");
  const uint start=0xa6,end=0xc5;var code=method.Body.Instructions;
  foreach(var i in code.Where(i=>i.Offset<start||i.Offset>=end)){
   IEnumerable<Instruction> targets=i.Operand is Instruction target?new[]{target}:i.Operand as IList<Instruction>??System.Array.Empty<Instruction>();
   foreach(var t in targets)Check(t.Offset<=start||t.Offset>=end,"Outside branch enters replaced block");
  }
  foreach(var h in method.Body.ExceptionHandlers)foreach(var i in new[]{h.TryStart,h.TryEnd,h.HandlerStart,h.HandlerEnd,h.FilterStart})
   if(i!=null)Check(i.Offset<=start||i.Offset>=end,"Exception boundary inside replaced block");
  var copy=(MethodDef)original.ResolveToken(0x06000631);Check(copy.Name.String.EndsWith(".=")&&copy.MethodSig.Params.Count==2,"Existing copy signature");
  Check(code.Any(i=>i.Operand is IMethod m&&m.MDToken.Raw==copy.MDToken.Raw),"Copy is already used by this handler");
  string header=File.ReadAllText("ExperimentalMenu/lastfm_utf8.h");byte[] before=Array(header,"lastfm_utf8_original"),after=Array(header,"lastfm_utf8_copy");
  int at=(int)(uint)original.Metadata.PEImage.ToFileOffset((dnlib.PE.RVA)0x34ff4e);
  Check(game.Skip(at).Take(before.Length).SequenceEqual(before),"Original block differs");
  byte[] changed=(byte[])game.Clone();System.Array.Clear(changed,at,before.Length);System.Array.Copy(after,0,changed,at,after.Length);
  using var patched=ModuleDefMD.Load(changed);var updated=(MethodDef)patched.ResolveToken(method.MDToken);
  Check(updated.Body.Instructions.Single(i=>i.Offset==start).OpCode==OpCodes.Ldloca_S,"Replacement entry");
  Check(updated.Body.Instructions.Single(i=>i.Offset==start+4).Operand is IMethod targetCopy&&targetCopy.MDToken.Raw==copy.MDToken.Raw,"Replacement call token");
  Check(code.Where(i=>i.Offset>=end).Select(i=>i.ToString()).SequenceEqual(updated.Body.Instructions.Where(i=>i.Offset>=end).Select(i=>i.ToString())),"Other handler instructions changed");
  Console.WriteLine("PASS: exact original Last.fm IL guard, existing copy token, unchanged outside instructions and safe branch/EH boundaries");
  using var live=ModuleDefMD.Load(args[1]);using var query=ModuleDefMD.Load(args[2]);
  bool range=args.Length>4&&args[4]=="range";var allowed=range?new uint[]{0x060000f2,0x060000f3}:new uint[]{0x060000f0,0x060000f4};
  foreach(var f in live.GetTypes().SelectMany(t=>t.Methods).Where(f=>f.HasBody)){
   var other=(MethodDef)query.ResolveToken(f.MDToken);if(!allowed.Contains(f.MDToken.Raw))Check(Body(f)==Body(other),"Unrelated managed body changed: "+f.MDToken);
  }
  byte[] oldBridge=File.ReadAllBytes(args[1]),newBridge=File.ReadAllBytes(args[2]);int nativeEntries=0;
  foreach(var f in live.GetTypes().SelectMany(t=>t.Methods).Where(f=>!f.HasBody&&(uint)f.RVA!=0)){
   var other=(MethodDef)query.ResolveToken(f.MDToken);Check(f.RVA==other.RVA,"Native RVA changed");
   int oldOffset=(int)(uint)live.Metadata.PEImage.ToFileOffset(f.RVA),newOffset=(int)(uint)query.Metadata.PEImage.ToFileOffset(other.RVA);
   Check(oldBridge.Skip(oldOffset).Take(16).SequenceEqual(newBridge.Skip(newOffset).Take(16)),"Native entry bytes changed");nativeEntries++;
  }
  Console.WriteLine("PASS: "+nativeEntries+" native RVAs and first 16 entry bytes retained (not a full binary match)");
  int decodes=0;
  foreach(uint token in allowed){var f=(MethodDef)query.ResolveToken(token);
   decodes+=f.Body.Instructions.Count(i=>i.Operand is IMethod m&&m.FullName.Contains("MetadataText::DecodeValue"));
   foreach(var i in f.Body.Instructions.Where(i=>i.OpCode==OpCodes.Call&&i.Operand is IMethod m&&m.Name=="FindAsync").ToArray()){
    int index=f.Body.Instructions.IndexOf(i);i.OpCode=OpCodes.Pop;i.Operand=null;f.Body.Instructions.Insert(index+1,Instruction.Create(OpCodes.Ldnull));
   }f.Body.SimplifyBranches();f.Body.OptimizeBranches();
  }
  Check(decodes==(range?3:5),"Expected scoped equality decodes");
  var writer=new NativeModuleWriterOptions(query,optimizeImageSize:false);writer.MetadataOptions.Flags|=MetadataFlags.PreserveAll;query.NativeWrite(args[3],writer);
  Console.WriteLine("PASS: two scoped query bodies changed; all other managed bodies retained; separate test fixture prevents network calls");
 }
}
