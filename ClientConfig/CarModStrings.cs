using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
namespace ReRiff {
 internal static class CarModStrings {
  sealed class Token {internal byte Group;internal string Name;internal int Index;}
  sealed class Table {internal List<string> Names;internal List<int> Counts;internal List<uint> Capacities;internal List<Token> Tokens;}
  sealed class SourceInfo {internal CarModComposer.Package Package;internal int Group;internal int Index;internal int Count;}
  static void Need(bool b){if(!b)throw new IOException("Unsupported native localization bank");}
  static int Bucket(string name){uint h=5381;foreach(byte c in Encoding.ASCII.GetBytes(name))h=unchecked(h*33+(uint)(c&~32));return (int)(h&255);}
  static List<string> Strings(byte[] b,int count){Need(b!=null&&b.Length>=8&&BitConverter.ToUInt32(b,0)==3&&BitConverter.ToUInt32(b,4)==2);var result=new List<string>();using(var r=new BinaryReader(new MemoryStream(b),Encoding.Unicode)){r.ReadBytes(8);for(int i=0;i<count;i++){uint n=r.ReadUInt32();Need(n<=100000&&r.BaseStream.Position+n*2<=b.Length);result.Add(Encoding.Unicode.GetString(r.ReadBytes((int)n*2)));}Need(r.BaseStream.Position==b.Length);}return result;}
  static Table Parse(byte[] bytes,bool strict){
   Need(bytes!=null);var table=new Table{Names=new List<string>(),Counts=new List<int>(),Capacities=new List<uint>(),Tokens=new List<Token>()};
   using(var r=new BinaryReader(new MemoryStream(bytes))){Need(r.ReadUInt32()==3&&r.ReadUInt32()==2);uint groups=r.ReadUInt32();Need(groups>0&&groups<=32);for(int i=0;i<groups;i++){int len=r.ReadInt32(),count=r.ReadInt32();uint cap=r.ReadUInt32();Need(len>0&&len<100&&count>=0&&count<100000);table.Names.Add(Encoding.ASCII.GetString(r.ReadBytes(len)).ToLowerInvariant());table.Counts.Add(count);table.Capacities.Add(cap);}var sizes=new int[256];for(int i=0;i<256;i++)sizes[i]=r.ReadInt32();var used=new int[groups];
    for(int bucket=0;bucket<256;bucket++){long end=r.BaseStream.Position+sizes[bucket];Need(sizes[bucket]>=0&&end<=bytes.Length);while(r.BaseStream.Position<end){byte group=r.ReadByte();Need(group<groups);var text=new List<byte>();byte c;while((c=r.ReadByte())!=0){Need(c<128&&text.Count<200);text.Add(c);}string name=Encoding.ASCII.GetString(text.ToArray());if(strict)Need(Bucket(name)==bucket);table.Tokens.Add(new Token{Group=group,Name=name,Index=used[group]++});}Need(r.BaseStream.Position==end);}Need(r.BaseStream.Position==bytes.Length);for(int i=0;i<groups;i++)Need(used[i]==table.Counts[i]);
   }
   return table;
  }
  internal static Dictionary<string,byte[]> Build(Func<string,byte[]> stock,IEnumerable<string> paths,List<CarModComposer.Package> packages,CarSlotRegistry.State state){
   var stockTable=Parse(stock("strings/tokens.dat"),true);
   /* A new-format package may ship its own token table with car_<source>
    * inserted into Common. The garage resolves Car_<name> through Common, so a
    * token added to an empty bank would render blank. Use that package table as
    * the composed base and preserve its texts; other packages still override
    * their own car/pack tokens by name. */
   Table baseTable=null;CarModComposer.Package basePackage=null;
   foreach(var package in packages){byte[] data;if(package.Assets.TryGetValue("strings/tokens.dat",out data)){baseTable=Parse(data,false);basePackage=package;break;}}
   var table=baseTable??stockTable;var names=table.Names;var counts=table.Counts;var capacities=table.Capacities;var old=stockTable.Tokens;
   int common=names.IndexOf("common");Need(common>=0);
   var all=baseTable!=null?new List<Token>(baseTable.Tokens):new List<Token>(old);
   var sources=new Dictionary<string,SourceInfo>(StringComparer.Ordinal);var tables=new Dictionary<CarModComposer.Package,Table>();
   Func<CarModComposer.Package,Table> tokensFor=delegate(CarModComposer.Package package){Table value;if(tables.TryGetValue(package,out value))return value;byte[] data;if(package.Assets.TryGetValue("strings/tokens.dat",out data))value=Parse(data,false);tables[package]=value;return value;};
   Func<CarModComposer.Package,string,SourceInfo> find=delegate(CarModComposer.Package package,string name){
    var custom=tokensFor(package);if(custom!=null){var token=custom.Tokens.FirstOrDefault(t=>t.Name==name);if(token!=null)return new SourceInfo{Package=package,Group=token.Group,Index=token.Index,Count=custom.Counts[token.Group]};}
    var known=old.FirstOrDefault(t=>t.Name==name);if(known==null)return null;
    return new SourceInfo{Package=package,Group=known.Group,Index=known.Index,Count=stockTable.Counts[known.Group]};
   };
   foreach(var car in state.Cars)if(car.Active){
    var package=packages.Single(p=>p.Cars.Any(c=>c.Key==car.Key));string target="car_"+car.Native;
    var source=find(package,"car_"+car.Source)??find(package,"car_"+car.Base);Need(source!=null);
    sources[target]=source;
    if(!all.Any(t=>t.Name==target))all.Add(new Token{Group=(byte)source.Group,Name=target,Index=-1});
   }
   foreach(var car in state.Cars)if(car.Active){
    var package=packages.Single(p=>p.Cars.Any(c=>c.Key==car.Key));
    foreach(string stem in new[]{"paint","paint_pc"}){
     string path="scripts/config/"+stem+".cfg";byte[] data;if(!package.Assets.TryGetValue(path,out data))data=stock(path);
     var paint=NativeCarConfig.Find(NativeCarConfig.Read(data),"Paints");Need(paint!=null);
     var model=NativeCarConfig.Find(paint.Children,car.Source)??NativeCarConfig.Find(paint.Children,car.Base);Need(model!=null);
     for(int i=0;i<model.Children.Count;i++){
      var label=NativeCarConfig.Find(model.Children[i].Children,"packName");if(label==null||label.Type!=0)continue;
      string source=label.Text.ToLowerInvariant();
      // Reuse the pack's existing stock token; unknown names stay unresolved
      // instead of refusing the whole car cache.
      if(old.Any(t=>t.Group==common&&t.Name==source)){var found=find(package,source);if(found!=null)sources[source]=found;}
     }
    }
   }
   all=all.OrderBy(t=>Bucket(t.Name)).ToList();var output=new Dictionary<string,byte[]>(StringComparer.Ordinal);var maxima=new uint[names.Count];
   foreach(string path in paths){if(!path.StartsWith("strings/",StringComparison.Ordinal)||!path.EndsWith(".str",StringComparison.Ordinal))continue;string[] parts=path.Split('/');if(parts.Length!=3)continue;int group=names.IndexOf(parts[1]);if(group<0)continue;var original=Strings(stock(path),group<stockTable.Counts.Count?stockTable.Counts[group]:0);var loaded=new Dictionary<string,List<string>>(StringComparer.Ordinal);List<string> baseBank=null;
    if(basePackage!=null&&group==common){byte[] custom;if(basePackage.Assets.TryGetValue("strings/"+names[group]+"/"+parts[2],out custom))baseBank=Strings(custom,counts[group]);}
    using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(3u);w.Write(2u);foreach(var token in all.Where(t=>t.Group==group)){string text;SourceInfo source;if(sources.TryGetValue(token.Name,out source)){string sourcePath="strings/"+names[source.Group]+"/"+parts[2];string key=source.Package.Folder+"\n"+sourcePath+"\n"+source.Count;List<string> bank;if(!loaded.TryGetValue(key,out bank)){byte[] custom;bank=Strings(source.Package.Assets.TryGetValue(sourcePath,out custom)?custom:stock(sourcePath),source.Count);loaded.Add(key,bank);}text=bank[source.Index];}else if(baseBank!=null){Need(token.Index>=0&&token.Index<baseBank.Count);text=baseBank[token.Index];}else{Need(token.Index>=0);text=original[token.Index];}byte[] utf16=Encoding.Unicode.GetBytes(text);w.Write(utf16.Length/2);w.Write(utf16);}byte[] data=m.ToArray();output[path]=data;maxima[group]=Math.Max(maxima[group],(uint)data.Length-8);}
   }
   using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(3u);w.Write(2u);w.Write(names.Count);for(int i=0;i<names.Count;i++){byte[] name=Encoding.ASCII.GetBytes(i==0?"Common":i==1?"FrontEnd":i==2?"BackEnd":names[i]);w.Write(name.Length);w.Write(all.Count(t=>t.Group==i));w.Write(Math.Max(capacities[i],maxima[i]));w.Write(name);}for(int i=0;i<256;i++)w.Write(all.Where(t=>Bucket(t.Name)==i).Sum(t=>t.Name.Length+2));foreach(var t in all){w.Write(t.Group);w.Write(Encoding.ASCII.GetBytes(t.Name));w.Write((byte)0);}output["strings/tokens.dat"]=m.ToArray();}return output;
  }
 }
}
