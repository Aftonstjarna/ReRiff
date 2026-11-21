using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
namespace ReRiff {
 internal static partial class ModArchiveCache {
  internal static bool CarSlotsRequested;
  internal static CarSlotRegistry.State CurrentCars,PreviousCars;
  sealed class CarsPlan {internal HashSet<string> Folders=new HashSet<string>(StringComparer.Ordinal);internal List<Entry> Entries=new List<Entry>();internal string Fingerprint;internal CarSlotRegistry.State Before,After;}
  static byte[] CarBytes(Entry entry){
   if(entry.Size>128*1024*1024||entry.Packed>128*1024*1024)throw new IOException("Car asset exceeds bounds");
   using(var packed=new MemoryStream()){Copy(entry,packed);packed.Position=0;byte[] result;
    if(entry.Method==0)result=packed.ToArray();else using(var zip=new System.IO.Compression.DeflateStream(packed,System.IO.Compression.CompressionMode.Decompress))using(var output=new MemoryStream()){
     byte[] buffer=new byte[65536];int n;while((n=zip.Read(buffer,0,buffer.Length))>0){if(output.Length+n>entry.Size)throw new IOException("Car asset decompression exceeds bounds");output.Write(buffer,0,n);}result=output.ToArray();}
    if(result.Length!=entry.Size)throw new IOException("Car asset size mismatch");if(entry.SourceHash==null)using(var data=new MemoryStream(result))if(Crc(data,result.Length)!=entry.Crc)throw new IOException("Car asset CRC mismatch");return result;
   }
  }
  static bool Authored(string folder){
   string assets=Path.Combine(folder,"assets");
   return File.Exists(Path.Combine(assets,"scripts","config","vehicleselect.cfg"))&&Directory.Exists(Path.Combine(assets,"strings","common"));
  }
  static CarsPlan PlanCars(string root,string[] folders){
   var packages=new List<CarModComposer.Package>();var plan=new CarsPlan();string registry=Path.Combine(root,"car-slots.ini");
   foreach(string folder in folders){if(Path.GetFileName(folder).StartsWith(".",StringComparison.Ordinal)||(File.GetAttributes(folder)&FileAttributes.ReparsePoint)!=0)continue;string path=Path.Combine(folder,"origin.txt");if(!File.Exists(path))continue;Regular(path);if(new FileInfo(path).Length>65536)throw new IOException("Car origin metadata too large");string text=File.ReadAllText(path);var cars=CarSlotRegistry.Package(folder,text);if(cars.Count==0)continue;
    // A package that carries its own roster, configs and localization already
    // addresses an authored native slot. Mount it loose so stock string
    // indices and the package's own tokens remain unchanged.
    if(Authored(folder))continue;
    if(text.IndexOf("overwrite",StringComparison.Ordinal)>=0)CarSlotsRequested=true;
    CarSlotsRequested=true;string assets=Path.Combine(folder,"assets"),stem=Path.Combine(folder,"vfs");var entries=new List<Entry>();if(Directory.Exists(assets)){if(File.Exists(stem+".dat")||File.Exists(stem+".zip"))throw new IOException("Mixed loose and packed car mod");Walk(assets,"",entries,new StringBuilder());}else entries=ReadPair(stem);
    var package=new CarModComposer.Package{Folder=folder,Cars=cars,Assets=new Dictionary<string,byte[]>(StringComparer.Ordinal)};long total=0;foreach(var e in entries){total+=e.Size;if(total>256*1024*1024)throw new IOException("Car package exceeds bounds");package.Assets.Add(e.Name,CarBytes(e));}packages.Add(package);plan.Folders.Add(folder);
   }
   if(packages.Count==0&&!File.Exists(registry))return null;CarSlotsRequested=true;
   if(File.Exists(registry)){Regular(registry);if(new FileInfo(registry).Length>1048576)throw new IOException("Car registry exceeds bounds");plan.Before=CarSlotRegistry.Decode(File.ReadAllText(registry));}
   plan.After=CarSlotRegistry.Plan(plan.Before,packages.SelectMany(p=>p.Cars));plan.Fingerprint="car-slots-v1\n"+plan.After.Revision;
   if(packages.Count>0){
    string game=Path.GetDirectoryName(root);var stock=ReadPair(Path.Combine(game,"vfs"),true).ToDictionary(e=>e.Name,StringComparer.Ordinal);var memo=new Dictionary<string,byte[]>(StringComparer.Ordinal);
    // Preserve effective config/localization from other enabled mods.
    foreach(string folder in folders){if(plan.Folders.Contains(folder)||LiveryFolder(folder)||Path.GetFileName(folder).StartsWith(".",StringComparison.Ordinal)||(File.GetAttributes(folder)&FileAttributes.ReparsePoint)!=0)continue;
     var extra=new List<Entry>();string assets=Path.Combine(folder,"assets"),stem=Path.Combine(folder,"vfs");if(Directory.Exists(assets))Walk(assets,"",extra,new StringBuilder());else if(File.Exists(stem+".dat")&&File.Exists(stem+".zip"))extra=ReadPair(stem);
     foreach(var e in extra)if(e.Name.StartsWith("scripts/config/",StringComparison.Ordinal)||e.Name.StartsWith("strings/",StringComparison.Ordinal))stock[e.Name]=e;
    }
    Func<string,byte[]> read=delegate(string name){byte[] result;if(memo.TryGetValue(name,out result))return result;Entry e;if(!stock.TryGetValue(name,out e))return null;result=CarBytes(e);memo.Add(name,result);return result;};
    CarModComposer.DonorPaths=id=>stock.Keys.Where(p=>p.StartsWith("graphics/vehicles/"+id+"/",StringComparison.Ordinal)||p.StartsWith("graphics/vehicles/decals/"+id+"_",StringComparison.Ordinal)||p.StartsWith("graphics/be/hud/car/"+id+"/",StringComparison.Ordinal));
    var output=CarModComposer.Compose(read,packages,plan.After);foreach(var entry in CarModStrings.Build(read,stock.Keys,packages,plan.After))output[entry.Key]=entry.Value;
    var key=new StringBuilder(plan.Fingerprint);foreach(var item in output.OrderBy(x=>x.Key,StringComparer.Ordinal)){Name(item.Key);key.Append('\n').Append(item.Key).Append(':');using(var sha=System.Security.Cryptography.SHA256.Create())key.Append(Hex(sha.ComputeHash(item.Value)));plan.Entries.Add(new Entry{Name=item.Key,Data=item.Value,Size=(uint)item.Value.Length,Packed=(uint)item.Value.Length});}plan.Fingerprint=key.ToString();
   }
   return plan;
  }
  static void CarRegistryAtomic(string file,byte[] bytes){string temp=file+"."+Guid.NewGuid().ToString("N")+".tmp";try{using(var f=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){f.Write(bytes,0,bytes.Length);f.Flush(true);}if(File.Exists(file))File.Replace(temp,file,file+".bak");else File.Move(temp,file);}finally{if(File.Exists(temp))File.Delete(temp);}}
  static void CommitCars(string root,CarsPlan plan){
   string path=Path.Combine(root,"car-slots.ini");string text=CarSlotRegistry.Encode(plan.After);if(!File.Exists(path)||File.ReadAllText(path)!=text)CarRegistryAtomic(path,Encoding.UTF8.GetBytes(text));
   PreviousCars=plan.Before;CurrentCars=plan.After;
  }
 }
}
