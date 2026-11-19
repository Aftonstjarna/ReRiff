using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Collections.Generic;
using ReRiff;

class CarModCacheSmoke {
 static void Check(bool value,string label){if(!value)throw new Exception(label);}
 static byte[] Bytes(ZipArchiveEntry entry){Check(entry!=null,"required ZIP entry exists");using(var input=entry.Open())using(var output=new MemoryStream()){input.CopyTo(output);return output.ToArray();}}
 static byte[] Resource(string stem,string name){using(var zip=ZipFile.OpenRead(stem+".zip"))return Bytes(zip.GetEntry(name));}
 static string[] Roster(string stem){return NativeCarConfig.Find(NativeCarConfig.Read(Resource(stem,"scripts/config/vehicleselect.cfg")),"carNames").Children.Select(n=>n.Text).ToArray();}
 static string Origin(bool overwrite,string id){return "id="+id+"\noverwrite="+(overwrite?"1":"0")+"\n[origins]\nalf=alf\n";}
 static void Mod(string root,string name,bool overwrite,string asset,byte[] bytes){string folder=Path.Combine(root,name);Directory.CreateDirectory(folder);File.WriteAllText(Path.Combine(folder,"origin.txt"),Origin(overwrite,"customs-"+name));string target=Path.Combine(folder,"assets",asset);Directory.CreateDirectory(Path.GetDirectoryName(target));File.WriteAllBytes(target,bytes);}
 static string Combined(List<ModArchiveCache.Plan> plans,string mods){Check(plans.Count==1,"one combined car cache");var plan=plans[0];Check(plan.Id==1&&Path.GetDirectoryName(plan.Stem)==mods&&Path.GetFileName(plan.Stem).StartsWith(".reriff-cars-",StringComparison.Ordinal),"combined cache directly under mods with native mount ID");Check(File.Exists(plan.Stem+".dat")&&File.Exists(plan.Stem+".zip")&&File.Exists(plan.Stem+".receipt"),"complete combined cache");return plan.Stem;}
 static CarSlotRegistry.State Registry(string mods){return CarSlotRegistry.Decode(File.ReadAllText(Path.Combine(mods,"car-slots.ini")));}
 static void Main(string[] args){
  Check(args.Length==2,"usage: CarModCacheSmoke STOCK_ROOT NEW_FIXTURE_ROOT");string stock=Path.GetFullPath(args[0]),fixture=Path.GetFullPath(args[1]);
  Check(!Directory.Exists(fixture)&&!File.Exists(fixture),"fixture must be a new path");Directory.CreateDirectory(fixture);
  foreach(string extension in new[]{".dat",".zip"})File.Copy(Path.Combine(stock,"vfs"+extension),Path.Combine(fixture,"vfs"+extension));
  string mods=Path.Combine(fixture,"mods");Directory.CreateDirectory(mods);string asset;byte[] assetBytes;string[] stockRoster;
  using(var zip=ZipFile.OpenRead(Path.Combine(stock,"vfs.zip"))){
   var sample=zip.Entries.Where(e=>e.FullName.StartsWith("graphics/vehicles/alf/",StringComparison.Ordinal)&&e.Length>0&&!e.FullName.EndsWith("/",StringComparison.Ordinal)).OrderBy(e=>e.Length).First();asset=sample.FullName;assetBytes=Bytes(sample);
   stockRoster=NativeCarConfig.Find(NativeCarConfig.Read(Bytes(zip.Entries.Single(e=>String.Equals(e.FullName,"scripts/config/vehicleselect.cfg",StringComparison.OrdinalIgnoreCase)))),"carNames").Children.Select(n=>n.Text).ToArray();
  }
  Check(stockRoster.Length==16,"expected stock roster");Mod(mods,"a",false,asset,assetBytes);Mod(mods,"b",false,asset,assetBytes);
  string first=Combined(ModArchiveCache.Prepare(mods,1),mods);string[] firstRoster=Roster(first);
  Check(firstRoster.SequenceEqual(stockRoster.Concat(new[]{"r00","r01"})),"master roster appends two identities");
  foreach(string id in new[]{"r00","r01"})Check(Resource(first,asset.Replace("/alf/","/"+id+"/")).SequenceEqual(assetBytes),"donor asset preserved for "+id);
  var initial=Registry(mods);Check(initial.Cars.Count==2&&initial.Cars.All(c=>c.Active)&&initial.Cars.Single(c=>c.Key=="customs-a|alf").Slot==15&&initial.Cars.Single(c=>c.Key=="customs-b|alf").Slot==16,"initial registry assignments");
  DateTime stamp=File.GetLastWriteTimeUtc(first+".zip");string registryText=File.ReadAllText(Path.Combine(mods,"car-slots.ini"));
  string warm=Combined(ModArchiveCache.Prepare(mods,1),mods);Check(warm==first&&stamp==File.GetLastWriteTimeUtc(warm+".zip")&&registryText==File.ReadAllText(Path.Combine(mods,"car-slots.ini")),"warm cache and registry reuse");
  Directory.Delete(Path.Combine(mods,"a"),true);string afterA=Combined(ModArchiveCache.Prepare(mods,1),mods);
  Check(Roster(afterA).SequenceEqual(stockRoster.Concat(new[]{"r00"})),"removal compacts master roster");
  var shifted=Registry(mods);Check(!shifted.Cars.Single(c=>c.Key=="customs-a|alf").Active&&shifted.Cars.Single(c=>c.Key=="customs-a|alf").Slot==-1&&shifted.Cars.Single(c=>c.Key=="customs-b|alf").Slot==15,"removed identity inactive and survivor shifted");
  Directory.Delete(Path.Combine(mods,"b"),true);Check(ModArchiveCache.Prepare(mods,1).Count==0,"last removal mounts no custom cache");
  Check(Registry(mods).Cars.Count==2&&Registry(mods).Cars.All(c=>!c.Active&&c.Slot==-1)&&ModArchiveCache.CurrentCars.Cars.All(c=>!c.Active),"both identities retained inactive");
  Mod(mods,"replacement",true,asset,assetBytes);string replacement=Combined(ModArchiveCache.Prepare(mods,1),mods);
  Check(Roster(replacement).SequenceEqual(stockRoster),"overwrite keeps stock roster size and identities");Check(Resource(replacement,asset).SequenceEqual(assetBytes),"replacement asset preserved");
  var replaced=Registry(mods).Cars.Single(c=>c.Active);Check(replaced.Overwrite&&replaced.Slot==0&&replaced.Native=="alf","overwrite maps donor slot");
  byte[] lastZip=File.ReadAllBytes(replacement+".zip"),lastDat=File.ReadAllBytes(replacement+".dat");string lastRegistry=File.ReadAllText(Path.Combine(mods,"car-slots.ini"));
  string origin=Path.Combine(mods,"replacement","origin.txt");File.WriteAllText(origin,"overwrite=invalid\n[origins]\nalf=alf\n");bool refused=false;try{ModArchiveCache.Prepare(mods,1);}catch(IOException){refused=true;}
  Check(refused,"malformed origins refused");Check(lastRegistry==File.ReadAllText(Path.Combine(mods,"car-slots.ini"))&&lastZip.SequenceEqual(File.ReadAllBytes(replacement+".zip"))&&lastDat.SequenceEqual(File.ReadAllBytes(replacement+".dat")),"refusal preserves last registry and combined masterlist");
  File.WriteAllText(origin,Origin(true,"customs-replacement"));Check(Combined(ModArchiveCache.Prepare(mods,1),mods)==replacement,"restored origin reuses last valid cache");
  using(var f=new FileStream(replacement+".zip",FileMode.Append,FileAccess.Write))f.WriteByte(123);byte[] modified=File.ReadAllBytes(replacement+".zip");
  string repaired=Combined(ModArchiveCache.Prepare(mods,1),mods);Check(repaired!=replacement&&modified.SequenceEqual(File.ReadAllBytes(replacement+".zip")),"modified cache preserved and replaced at new stem");
  Check(Roster(repaired).SequenceEqual(stockRoster)&&Combined(ModArchiveCache.Prepare(mods,1),mods)==repaired,"replacement cache valid and reused");
  Console.WriteLine("PASS: actual Prepare with stock fixture; combined master roster/assets, reuse, removal/compaction, inactive identities, overwrite, malformed origins and modified-cache preservation");
  Console.WriteLine("Fixture retained: "+fixture);
 }
}
