using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
namespace ReRiff {
 internal static class CarModComposer {
  internal sealed class Package {internal string Folder;internal Dictionary<string,byte[]> Assets;internal List<CarSlotRegistry.Car> Cars;}
  static NativeCarConfig.Node Require(List<NativeCarConfig.Node> nodes,string name){var n=NativeCarConfig.Find(nodes,name);if(n==null)throw new IOException("Car mod is missing native table "+name);return n;}
  static string Map(string value,string from,string to){if(value==from)return to;return value.Replace("/"+from+"/","/"+to+"/").Replace("/"+from+"_","/"+to+"_");}
  static void Remap(NativeCarConfig.Node n,string from,string to){n.Name=Map(n.Name,from,to);if(n.Type==0)n.Text=Map(n.Text,from,to);foreach(var c in n.Children)Remap(c,from,to);}
  static NativeCarConfig.Node Clone(List<NativeCarConfig.Node> nodes,string source,string donor,string target){var node=NativeCarConfig.Find(nodes,source)??NativeCarConfig.Find(nodes,donor);if(node==null)throw new IOException("Missing donor car configuration: "+donor);var copy=node.Copy();Remap(copy,source,target);if(source!=donor)Remap(copy,donor,target);copy.Name=target;return copy;}
  static void Put(List<NativeCarConfig.Node> nodes,NativeCarConfig.Node value){int i=nodes.FindIndex(n=>n.Name==value.Name);if(i<0)nodes.Add(value);else nodes[i]=value;}
  internal static Dictionary<string,byte[]> Compose(Func<string,byte[]> stock,List<Package> packages,CarSlotRegistry.State state){
   var output=new Dictionary<string,byte[]>(StringComparer.Ordinal);var cars=state.Cars.FindAll(c=>c.Active);cars.Sort((a,b)=>a.Slot.CompareTo(b.Slot));
   var owners=new Dictionary<string,Package>(StringComparer.Ordinal);foreach(var p in packages)foreach(var c in p.Cars)owners.Add(c.Key,p);
   Func<CarSlotRegistry.Car,string,byte[]> source=delegate(CarSlotRegistry.Car car,string name){byte[] bytes;return owners[car.Key].Assets.TryGetValue(name,out bytes)?bytes:stock(name);};
   foreach(var car in cars){
    // Fill a new native identity from its complete donor, then overlay authored
    // source assets. Binary model/material payloads retain their native bytes.
    foreach(string path in DonorPaths(car.Base)){byte[] bytes=stock(path);if(bytes!=null)output[Map(path,car.Base,car.Native)]=bytes;}
    foreach(var asset in owners[car.Key].Assets){string path=asset.Key;bool own=path.StartsWith("graphics/vehicles/"+car.Source+"/",StringComparison.Ordinal)||path.StartsWith("graphics/vehicles/decals/"+car.Source+"_",StringComparison.Ordinal)||path.StartsWith("graphics/be/hud/car/"+car.Source+"/",StringComparison.Ordinal);if(own)output[Map(path,car.Source,car.Native)]=asset.Value;}

   }
   foreach(string stem in new[]{"vehicleselect","unlocks","unlocks_pc"}){
    string path="scripts/config/"+stem+".cfg";var tree=NativeCarConfig.Read(stock(path));var names=Require(tree,"carNames");var defaults=Require(tree,"default");
    if(names.Children.Count!=16)throw new IOException("Original car roster differs; allocation refused");
    foreach(var car in cars){
     if(!car.Overwrite){names.Children.Add(NativeCarConfig.Text("",car.Native));tree.Insert(tree.IndexOf(defaults),NativeCarConfig.Number(car.Native,car.Slot+1));defaults.Children.Add(NativeCarConfig.Number(car.Native,car.Slot+1));}
     if(stem.StartsWith("unlocks",StringComparison.Ordinal)){var src=NativeCarConfig.Read(source(car,path));var srcNames=Require(src,"carNames").Children;int index=srcNames.FindIndex(n=>n.Type==0&&n.Text==car.Source);if(index<0)index=srcNames.FindIndex(n=>n.Type==0&&n.Text==car.Base);var list=Require(src,"carUnlocks").Children;if(index<0||index>=list.Count)throw new IOException("Missing donor unlock entry");var unlock=list[index].Copy();
      var target=Require(tree,"carUnlocks").Children;if(car.Overwrite)target[car.Slot+1]=unlock;else{if(target.Count!=car.Slot+1)throw new IOException("Non-contiguous car allocation");target.Add(unlock);}
     }
    }
    Emit(output,path,tree);
   }
   foreach(string stem in new[]{"paint","paint_pc"}){
    string path="scripts/config/"+stem+".cfg";var tree=NativeCarConfig.Read(stock(path));var paint=Require(tree,"Paints");
    foreach(var car in cars){var src=NativeCarConfig.Read(source(car,path));var clone=Clone(Require(src,"Paints").Children,car.Source,car.Base,car.Native);if(clone.Children.Count>9)throw new IOException("Car has more than nine paint packs");for(int i=0;i<clone.Children.Count;i++){var label=NativeCarConfig.Find(clone.Children[i].Children,"packName");if(label!=null&&label.Type==0)label.Text=car.Native+"modpack"+(stem=="paint_pc"?"pc":"")+i;}Put(paint.Children,clone);}Emit(output,path,tree);
   }
   // Vehicle physics/audio tables vary by platform. Preserve all native tables
   // and only add an explicit per-car record where the donor has one.
   foreach(string stem in new[]{"vehicles","vehicles_pc","vehicleaudio"}){
    string path="scripts/config/"+stem+".cfg";byte[] original=stock(path);if(original==null)continue;var tree=NativeCarConfig.Read(original);
    foreach(var car in cars){var src=NativeCarConfig.Read(source(car,path));MergeDonor(tree,src,car);}Emit(output,path,tree);
   }
   // Keep auxiliary files (shared textures/materials, etc.) from managed
   // packages. Only resources composed above are replaced by the adapter.
   var configs=new HashSet<string>(StringComparer.Ordinal);
   foreach(string stem in new[]{"vehicleselect","unlocks","unlocks_pc","paint","paint_pc","vehicles","vehicles_pc","vehicleaudio"})
    foreach(string ext in new[]{".cfg",".luac",".luac64"})configs.Add("scripts/config/"+stem+ext);
   foreach(var package in packages)foreach(var asset in package.Assets){
    string path=asset.Key;if(configs.Contains(path)||path.StartsWith("strings/",StringComparison.Ordinal))continue;
    bool own=package.Cars.Any(c=>path.StartsWith("graphics/vehicles/"+c.Source+"/",StringComparison.Ordinal)||path.StartsWith("graphics/vehicles/decals/"+c.Source+"_",StringComparison.Ordinal)||path.StartsWith("graphics/be/hud/car/"+c.Source+"/",StringComparison.Ordinal));
    if(own)continue;byte[] existing;if(output.TryGetValue(path,out existing)&&!existing.SequenceEqual(asset.Value))throw new IOException("Conflicting auxiliary car resource: "+path);
    output[path]=asset.Value;
   }
   return output;
  }
  // Filled by the archive adapter with the stock paths, not guessed filenames.
  internal static Func<string,IEnumerable<string>> DonorPaths=delegate(string donor){return new string[0];};
  static void MergeDonor(List<NativeCarConfig.Node> dst,List<NativeCarConfig.Node> src,CarSlotRegistry.Car car){
   var node=NativeCarConfig.Find(src,car.Source)??NativeCarConfig.Find(src,car.Base);if(node!=null){Put(dst,Clone(src,car.Source,car.Base,car.Native));return;}
   foreach(var n in src)if(n.Type==2){var target=NativeCarConfig.Find(dst,n.Name);if(target!=null&&target.Type==2)MergeDonor(target.Children,n.Children,car);}
  }
  static void Emit(Dictionary<string,byte[]> output,string path,List<NativeCarConfig.Node> tree){output[path]=NativeCarConfig.Write(tree);string stem=path.Substring(0,path.Length-4);output[stem+".luac"]=NativeCarConfig.Lua(tree,4);output[stem+".luac64"]=NativeCarConfig.Lua(tree,8);}
 }
}
