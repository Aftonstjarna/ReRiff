using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Linq;
using ReRiff;
class CarComposerSmoke {
 static void Check(bool b,string s){if(!b)throw new Exception(s);}
 static void Main(string[] args){
  using(var z=ZipFile.OpenRead(args[0])){
   var entries=z.Entries.ToDictionary(e=>e.FullName.ToLowerInvariant(),StringComparer.Ordinal);Func<string,byte[]> read=delegate(string path){ZipArchiveEntry e;if(!entries.TryGetValue(path,out e))return null;using(var s=e.Open())using(var m=new MemoryStream()){s.CopyTo(m);return m.ToArray();}};
   var a=new CarModComposer.Package{Folder="a",Assets=new Dictionary<string,byte[]>(),Cars=CarSlotRegistry.Package("a","overwrite=0\n[origins]\nalf=alf")};
   var b=new CarModComposer.Package{Folder="b",Assets=new Dictionary<string,byte[]>(),Cars=CarSlotRegistry.Package("b","overwrite=0\n[origins]\nalf=alf")};
   a.Assets["graphics/shared/car-test.texture"]=new byte[]{1,2,3};
   var list=new List<CarModComposer.Package>{a,b};var state=CarSlotRegistry.Plan(null,a.Cars.Concat(b.Cars));CarModComposer.DonorPaths=id=>entries.Keys.Where(p=>p.StartsWith("graphics/vehicles/"+id+"/")||p.StartsWith("graphics/vehicles/decals/"+id+"_")||p.StartsWith("graphics/be/hud/car/"+id+"/"));
   var output=CarModComposer.Compose(read,list,state);Check(output["graphics/shared/car-test.texture"].SequenceEqual(new byte[]{1,2,3}),"auxiliary textures retained");var roster=NativeCarConfig.Find(NativeCarConfig.Read(output["scripts/config/vehicleselect.cfg"]),"carNames");Check(roster.Children.Count==18,"two appended cars");Check(roster.Children[16].Text=="r00"&&roster.Children[17].Text=="r01","native slot identities");Check(output.ContainsKey("graphics/vehicles/r00/lod0_livery.bdae")&&output.ContainsKey("graphics/vehicles/r01/lod0_livery.bdae"),"both model copies");
   var strings=CarModStrings.Build(read,entries.Keys,list,state);Check(strings["strings/tokens.dat"].Length>read("strings/tokens.dat").Length,"new name tokens");
   foreach(var p in output.Where(e=>e.Key.EndsWith(".cfg"))){var nodes=NativeCarConfig.Read(p.Value);Check(NativeCarConfig.Write(nodes).SequenceEqual(p.Value),"config roundtrip");}
   a.Cars=CarSlotRegistry.Package("a","overwrite=1\n[origins]\nalf=alf");list=new List<CarModComposer.Package>{a};state=CarSlotRegistry.Plan(null,a.Cars);output=CarModComposer.Compose(read,list,state);Check(NativeCarConfig.Find(NativeCarConfig.Read(output["scripts/config/vehicleselect.cfg"]),"carNames").Children.Count==16,"replacement consumes no new slot");Check(output.ContainsKey("graphics/vehicles/alf/lod0_livery.bdae"),"replacement donor model");
   Console.WriteLine("PASS: two donor clones, distinct native IDs/localization, CFG roundtrips and overwrite without roster growth");
  }
 }
}
