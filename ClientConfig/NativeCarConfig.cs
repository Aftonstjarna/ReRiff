// Native GFCF/Lua table serialization follows scripts/build-car-clone-test.py.
// This is authored mod configuration, never original-byte reconstruction.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
namespace ReRiff {
 internal static class NativeCarConfig {
  internal sealed class Node {
   internal string Name,Text;internal int Type;internal float Number;internal List<Node> Children=new List<Node>();
   internal Node Copy(){var n=new Node{Name=Name,Text=Text,Type=Type,Number=Number};foreach(var c in Children)n.Children.Add(c.Copy());return n;}
  }
  static void Need(bool b){if(!b)throw new IOException("Unsupported native car configuration");}
  static uint Hash(string s){uint h=5387;foreach(byte b in Encoding.UTF8.GetBytes(s))h=unchecked(h*33+b);return h;}
  internal static Node Find(List<Node> nodes,string name){foreach(var n in nodes)if(n.Name==name)return n;return null;}
  internal static Node Text(string name,string value){return new Node{Name=name,Type=0,Text=value};}
  internal static Node Number(string name,float value){return new Node{Name=name,Type=1,Number=value};}
  internal static List<Node> Read(byte[] b){
   Need(b.Length>=20&&BitConverter.ToUInt32(b,0)==0x46434647&&BitConverter.ToUInt32(b,4)==3);uint count=BitConverter.ToUInt32(b,12),roots=BitConverter.ToUInt32(b,16);long pool64=20+count*32L;
   Need(count>0&&count<100000&&roots<=count&&pool64+BitConverter.ToUInt32(b,8)==b.Length);int pool=(int)pool64;
   Func<uint,string> text=delegate(uint p){Need(p<b.Length-pool);int start=pool+(int)p,end=start;while(end<b.Length&&b[end]!=0)end++;Need(end<b.Length);return new UTF8Encoding(false,true).GetString(b,start,end-start);};
   var seen=new HashSet<uint>();Func<uint,int,Node> read=null;read=delegate(uint index,int depth){Need(index<count&&depth<100&&seen.Add(index));int p=20+(int)index*32;uint value=BitConverter.ToUInt32(b,p+8),type=BitConverter.ToUInt32(b,p+20),size=BitConverter.ToUInt32(b,p+24);
    Need(type<=2&&BitConverter.ToUInt32(b,p+4)==0&&BitConverter.ToUInt32(b,p+12)==0&&BitConverter.ToUInt32(b,p+28)==0);var n=new Node{Name=text(BitConverter.ToUInt32(b,p)),Type=(int)type};Need(Hash(n.Name)==BitConverter.ToUInt32(b,p+16));
    if(type==0){Need(size==0);n.Text=text(value);}else if(type==1){Need(size==0);n.Number=BitConverter.ToSingle(b,p+8);Need(!float.IsNaN(n.Number)&&!float.IsInfinity(n.Number));}else{Need(value<=count&&size<=count-value);for(uint i=0;i<size;i++)n.Children.Add(read(value+i,depth+1));}return n;};
   var result=new List<Node>();for(uint i=0;i<roots;i++)result.Add(read(i,0));Need(seen.Count==count);return result;
  }
  internal static byte[] Write(List<Node> roots){
   var records=new List<byte[]>();foreach(var n in roots)records.Add(null);var pool=new MemoryStream();var strings=new Dictionary<string,uint>(StringComparer.Ordinal);
   Func<string,uint> str=delegate(string s){Need(s!=null&&s.IndexOf('\0')<0);uint offset;if(strings.TryGetValue(s,out offset))return offset;offset=(uint)pool.Length;byte[] data=Encoding.UTF8.GetBytes(s);pool.Write(data,0,data.Length);pool.WriteByte(0);strings.Add(s,offset);return offset;};
   Action<int,Node> fill=null;fill=delegate(int index,Node n){uint value=0,count=0;if(n.Type==2){value=(uint)records.Count;count=(uint)n.Children.Count;foreach(var c in n.Children)records.Add(null);for(int j=0;j<n.Children.Count;j++)fill((int)value+j,n.Children[j]);}else if(n.Type==0)value=str(n.Text);else{Need(n.Type==1);value=BitConverter.ToUInt32(BitConverter.GetBytes(n.Number),0);}
    using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(str(n.Name));w.Write(0u);w.Write(value);w.Write(0u);w.Write(Hash(n.Name));w.Write((uint)n.Type);w.Write(count);w.Write(0u);records[index]=m.ToArray();}};
   for(int i=0;i<roots.Count;i++)fill(i,roots[i]);using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(0x46434647u);w.Write(3u);w.Write((uint)pool.Length);w.Write((uint)records.Count);w.Write((uint)roots.Count);foreach(var record in records)w.Write(record);w.Write(pool.ToArray());return m.ToArray();}
  }
  internal static byte[] Lua(List<Node> roots,int word){
   Need(word==4||word==8);var code=new List<uint>();var constants=new List<object>();int registers=3;
   Func<object,int> constant=delegate(object v){int i=constants.IndexOf(v);if(i>=0)return i;Need(constants.Count<262144);constants.Add(v);return constants.Count-1;};
   Action<int,int,int,int> abc=(op,a,b,c)=>code.Add((uint)(op|(a<<6)|(c<<14)|(b<<23)));
   Action<int,int,int> abx=(op,a,b)=>code.Add((uint)(op|(a<<6)|(b<<14)));
   Action<Node,int> load=null;load=delegate(Node n,int r){registers=Math.Max(registers,r+3);Need(registers<250);if(n.Type!=2){if(n.Name=="unlockedAtStart")abc(2,r,n.Number!=0?1:0,0);else abx(1,r,constant(n.Type==0?(object)n.Text:n.Number));return;}
    abc(10,r,0,0);bool array=n.Name=="default"||n.Children.TrueForAll(x=>x.Name.Length==0);int first=n.Children.Count>0&&n.Children[0].Type==0&&n.Children[0].Text==""?0:1;
    for(int i=0;i<n.Children.Count;i++){abx(1,r+1,constant(array?(object)(float)(i+first):n.Children[i].Name));load(n.Children[i],r+2);abc(9,r,r+1,r+2);}};
   foreach(var n in roots){load(n,0);abx(7,0,constant(n.Name));}abc(30,0,1,0);
   using(var m=new MemoryStream())using(var w=new BinaryWriter(m)){w.Write(new byte[]{27,76,117,97,81,0,1,4,(byte)word,4,4,0});w.Write(new byte[word+8]);w.Write(new byte[]{0,0,2,(byte)registers});w.Write(code.Count);foreach(uint c in code)w.Write(c);w.Write(constants.Count);foreach(object c in constants){if(c is string){w.Write((byte)4);byte[] s=Encoding.UTF8.GetBytes((string)c);w.Write(s.Length+1);if(word==8)w.Write(0);w.Write(s);w.Write((byte)0);}else{w.Write((byte)3);w.Write((float)c);}}w.Write(new byte[16]);return m.ToArray();}
  }
 }
}
