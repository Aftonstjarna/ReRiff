using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
namespace ReRiff {
 internal static class CarModStrings {
  sealed class Token {internal byte Group;internal string Name;internal int Index;}
  static void Need(bool b){if(!b)throw new IOException("Unsupported native localization bank");}
  static int Bucket(string name){uint h=5381;foreach(byte c in Encoding.ASCII.GetBytes(name))h=unchecked(h*33+(uint)(c&~32));return (int)(h&255);}
  static List<string> Strings(byte[] b,int count){Need(b!=null&&b.Length>=8&&BitConverter.ToUInt32(b,0)==3&&BitConverter.ToUInt32(b,4)==2);var result=new List<string>();using(var r=new BinaryReader(new MemoryStream(b),Encoding.Unicode)){r.ReadBytes(8);for(int i=0;i<count;i++){uint n=r.ReadUInt32();Need(n<=100000&&r.BaseStream.Position+n*2<=b.Length);result.Add(Encoding.Unicode.GetString(r.ReadBytes((int)n*2)));}Need(r.BaseStream.Position==b.Length);}return result;}
  internal static Dictionary<string,byte[]> Build(Func<string,byte[]> stock,IEnumerable<string> paths,List<CarModComposer.Package> packages,CarSlotRegistry.State state){
   byte[] bytes=stock("strings/tokens.dat");Need(bytes!=null);var names=new List<string>();var counts=new List<int>();var capacities=new List<uint>();var old=new List<Token>();int[] sizes;
   using(var r=new BinaryReader(new MemoryStream(bytes))){Need(r.ReadUInt32()==3&&r.ReadUInt32()==2);uint groups=r.ReadUInt32();Need(groups>0&&groups<=32);for(int i=0;i<groups;i++){int len=r.ReadInt32(),count=r.ReadInt32();uint cap=r.ReadUInt32();Need(len>0&&len<100&&count>=0&&count<100000);names.Add(Encoding.ASCII.GetString(r.ReadBytes(len)).ToLowerInvariant());counts.Add(count);capacities.Add(cap);}sizes=new int[256];for(int i=0;i<256;i++)sizes[i]=r.ReadInt32();var used=new int[groups];
    for(int bucket=0;bucket<256;bucket++){long end=r.BaseStream.Position+sizes[bucket];Need(sizes[bucket]>=0&&end<=bytes.Length);while(r.BaseStream.Position<end){byte group=r.ReadByte();Need(group<groups);var text=new List<byte>();byte c;while((c=r.ReadByte())!=0){Need(c<128&&text.Count<200);text.Add(c);}string name=Encoding.ASCII.GetString(text.ToArray());Need(Bucket(name)==bucket);old.Add(new Token{Group=group,Name=name,Index=used[group]++});}Need(r.BaseStream.Position==end);}Need(r.BaseStream.Position==bytes.Length);for(int i=0;i<groups;i++)Need(used[i]==counts[i]);
   }
   int common=names.IndexOf("common");Need(common>=0);
   // Put added car/paint tokens in an empty bank so existing token IDs and
   // string indices never shift; the game resolves names across groups.
   int addition=common;for(int i=0;i<names.Count;i++)if(i!=common&&counts[i]==0){addition=i;break;}
   var all=new List<Token>(old);var sources=new Dictionary<string,KeyValuePair<CarModComposer.Package,string>>(StringComparer.Ordinal);
   foreach(var car in state.Cars)if(car.Active){var package=packages.Single(p=>p.Cars.Any(c=>c.Key==car.Key));string target="car_"+car.Native,source="car_"+car.Source;if(!old.Any(t=>t.Name==source))source="car_"+car.Base;Need(old.Any(t=>t.Group==common&&t.Name==source));sources[target]=new KeyValuePair<CarModComposer.Package,string>(package,source);if(!all.Any(t=>t.Name==target))all.Add(new Token{Group=(byte)addition,Name=target,Index=-1});}
   foreach(var car in state.Cars)if(car.Active){
    var package=packages.Single(p=>p.Cars.Any(c=>c.Key==car.Key));
    foreach(string stem in new[]{"paint","paint_pc"}){
     string path="scripts/config/"+stem+".cfg";byte[] data;if(!package.Assets.TryGetValue(path,out data))data=stock(path);
     var paint=NativeCarConfig.Find(NativeCarConfig.Read(data),"Paints");Need(paint!=null);
     var model=NativeCarConfig.Find(paint.Children,car.Source)??NativeCarConfig.Find(paint.Children,car.Base);Need(model!=null);
     for(int i=0;i<model.Children.Count;i++){
      var label=NativeCarConfig.Find(model.Children[i].Children,"packName");if(label==null||label.Type!=0)continue;
      string source=label.Text.ToLowerInvariant(),target=car.Native+"modpack"+(stem=="paint_pc"?"pc":"")+i;
      Need(old.Any(t=>t.Group==common&&t.Name==source));sources[target]=new KeyValuePair<CarModComposer.Package,string>(package,source);
      if(!all.Any(t=>t.Name==target))all.Add(new Token{Group=(byte)addition,Name=target,Index=-1});
     }
    }
   }
   // Preserve ordering within existing buckets; additions follow existing tokens.
   all=all.OrderBy(t=>Bucket(t.Name)).ToList();var output=new Dictionary<string,byte[]>(StringComparer.Ordinal);var maxima=new uint[names.Count];
   foreach(string path in paths){if(!path.StartsWith("strings/",StringComparison.Ordinal)||!path.EndsWith(".str",StringComparison.Ordinal))continue;string[] parts=path.Split('/');if(parts.Length!=3)continue;int group=names.IndexOf(parts[1]);if(group<0)continue;var original=Strings(stock(path),counts[group]);var loaded=new Dictionary<string,List<string>>(StringComparer.Ordinal);
    using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(3u);w.Write(2u);foreach(var token in all.Where(t=>t.Group==group)){string text;KeyValuePair<CarModComposer.Package,string> source;if(sources.TryGetValue(token.Name,out source)){var sourceToken=old.Single(t=>t.Name==source.Value);string sourcePath="strings/"+names[sourceToken.Group]+"/"+parts[2];string key=source.Key.Folder+"\n"+sourcePath;List<string> bank;if(!loaded.TryGetValue(key,out bank)){byte[] custom;bank=Strings(source.Key.Assets.TryGetValue(sourcePath,out custom)?custom:stock(sourcePath),counts[sourceToken.Group]);loaded.Add(key,bank);}text=bank[sourceToken.Index];}else{Need(token.Index>=0);text=original[token.Index];}byte[] utf16=Encoding.Unicode.GetBytes(text);w.Write(utf16.Length/2);w.Write(utf16);}byte[] data=m.ToArray();output[path]=data;maxima[group]=Math.Max(maxima[group],(uint)data.Length-8);}
   }
   using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(3u);w.Write(2u);w.Write(names.Count);for(int i=0;i<names.Count;i++){byte[] name=Encoding.ASCII.GetBytes(i==0?"Common":i==1?"FrontEnd":i==2?"BackEnd":names[i]);w.Write(name.Length);w.Write(all.Count(t=>t.Group==i));w.Write(Math.Max(capacities[i],maxima[i]));w.Write(name);}for(int i=0;i<256;i++)w.Write(all.Where(t=>Bucket(t.Name)==i).Sum(t=>t.Name.Length+2));foreach(var t in all){w.Write(t.Group);w.Write(Encoding.ASCII.GetBytes(t.Name));w.Write((byte)0);}output["strings/tokens.dat"]=m.ToArray();}return output;
  }
 }
}
