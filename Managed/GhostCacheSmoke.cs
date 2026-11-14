using System;using System.Linq;using System.IO;using System.Reflection;using System.Text;using ReRiff;
class GhostCacheSmoke {
 static void Check(bool value,string text){if(!value)throw new Exception(text);}
 static void Text(BinaryWriter w,string value){var b=Encoding.ASCII.GetBytes(value);w.Write(b.Length);w.Write(b);}
 static byte[] Fixture(int version,string owner,string song,float score){using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(version);w.Write(5);w.Write(216f);Text(w,owner);w.Write(score);Text(w,song);Text(w,"fab");w.Write(6);w.Write(0);w.Write((byte)1);return m.ToArray();}}
 static void Main(string[] a){string root=Path.Combine(Path.GetTempPath(),"reriff-ghost-cache-"+Guid.NewGuid());Directory.CreateDirectory(root);try {
  string file=Path.Combine(root,"Song1-123456789.ghost");byte[] valid=Fixture(18,"123456789","Song1",169388f);File.WriteAllBytes(file,valid);
  var r=CachedGhost.Read(file,"123456789");Check(r!=null&&r.Score==169388f&&r.Song=="Song1","native header order");Check(CachedGhost.Read(file,"987654321")==null,"other account rejected");
  foreach(var item in new[]{Fixture(17,"123456789","Song1",1),Fixture(18,"123456789","Other",1),Fixture(18,"987654321","Song1",1),Fixture(18,"123456789","Song1",Single.NaN),Fixture(18,"123456789","Song1",Single.PositiveInfinity),Fixture(18,"123456789","Song1",-1)}){File.WriteAllBytes(file,item);Check(CachedGhost.Read(file,"123456789")==null,"bad header refused");}
  for(int size=0;size<=valid.Length-5;size++){File.WriteAllBytes(file,new ArraySegment<byte>(valid,0,size).ToArray());try{Check(CachedGhost.Read(file,"123456789")==null,"truncation refused");}catch(InvalidDataException){}catch(IOException){}}
  if(a.Length==2){var actual=CachedGhost.Read(a[0],a[1]);Check(actual!=null,"existing native cache accepted");Console.WriteLine("Native cached replay: song={0} score={1}",actual.Song,actual.Score);}
  Console.WriteLine("PASS: native v18 header, filename/owner/song consistency, foreign-account rejection, finite score and every truncated header prefix; no file rewriting or game calls");
 }finally{Directory.Delete(root,true);}}
}
