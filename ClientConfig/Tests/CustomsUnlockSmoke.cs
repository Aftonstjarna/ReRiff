using System;using System.IO;using System.Collections.Generic;using ReRiff;
namespace ReRiff {internal static class NativeReelBridge {internal static Delegate CreateCall(System.Reflection.MethodInfo m,Type t,bool s){throw new NotImplementedException();}internal static void Log(string s){Console.WriteLine(s);}}internal static class ProgressionNative {internal static byte[] Read(IntPtr p,int n){var b=new byte[n];System.Runtime.InteropServices.Marshal.Copy(p,b,0,n);return b;}}}
class CustomsUnlockSmoke {
 static void Check(bool v,string message){if(!v)throw new Exception(message);}
 static int Main(string[] args){try{
  var stock=CustomsUnlocks.Defaults();Check(CustomsUnlocks.Matches(stock[0],"THE POLICE","Any song"),"Police case");Check(!CustomsUnlocks.Matches(stock[0],"Police Academy","Any song"),"Police artist");Check(CustomsUnlocks.Matches(stock[1],"Fleetwood Mac","The Chain (Live)"),"Monaco combo");Check(!CustomsUnlocks.Matches(stock[1],"Fleetwood Mac","Dreams"),"Monaco title");
  var r=new CustomsUnlocks.Rule{Car="alf",Artist="Artist",Title="Title"};Check(CustomsUnlocks.Matches(r,"aRTIST feat. X","TITLE (live)"),"Custom combo");Check(!CustomsUnlocks.Matches(r,"Different","Title"),"Wrong artist");Check(!CustomsUnlocks.Matches(r,"Artist","Different"),"Wrong title");
  var rules=CustomsUnlocks.Parse(File.ReadAllBytes(args[0]));Check(rules.Exists(x=>x.Car=="alf"&&x.Artist=="Custom Artist"&&x.Title=="Custom Title"),"Exported artist-title");Check(!rules.Exists(x=>x.Car=="pl1"),"Shop override removes old Police rule");Check(rules.Exists(x=>x.Car=="f1r"&&x.Artist=="Fleetwood Mac"&&x.Title=="Chain"),"Preserve Monaco");
  byte[] bad=File.ReadAllBytes(args[0]);bad[0]=0;bool rejected=false;try{CustomsUnlocks.Parse(bad);}catch(IOException){rejected=true;}Check(rejected,"Invalid config accepted");CustomsUnlocks.Load(null);Check(CustomsUnlocks.Rules.Count==2,"Default fallback");
  string root=Path.GetFullPath(args[1]),asset=Path.Combine(root,"test","assets","scripts","config");Directory.CreateDirectory(asset);
  foreach(string name in new[]{"unlocks_pc.cfg","unlocks_pc.luac","customs_unlocks.txt"})File.Copy(Path.Combine(Path.GetDirectoryName(args[0]),name),Path.Combine(asset,name));
  var plans=ModArchiveCache.Prepare(root,1);CustomsUnlocks.LoadPlan(plans);Check(CustomsUnlocks.Rules.Exists(x=>x.Car=="alf"),"Mounted custom rules");
  File.AppendAllText(Path.Combine(asset,"unlocks_pc.luac"),"changed");bool mismatch=false;try{CustomsUnlocks.LoadPlan(ModArchiveCache.Prepare(root,1));}catch(IOException){mismatch=true;}Check(mismatch,"Mismatched CFG/Lua accepted");
  Directory.Move(Path.Combine(root,"test"),Path.Combine(root,".disabled-test"));CustomsUnlocks.LoadPlan(ModArchiveCache.Prepare(root,1));Check(CustomsUnlocks.Rules.Count==2&&!CustomsUnlocks.Rules.Exists(x=>x.Car=="alf"),"Disabled rules retained");
  Console.WriteLine("PASS native defaults, case-insensitive artist/title matches, exported custom rules, shop override, invalid input and fallback");return 0;
 }catch(Exception e){Console.Error.WriteLine(e);return 1;}}
}
