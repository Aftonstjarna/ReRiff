using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;
using System.Collections.Generic;
namespace ReRiff {
 // Native load2 reads fixed per-chunk byte ranges. A checked trailer carries
 // portable car identities through existing opaque local/server checkpoints.
 internal static class CarSaveEnvelope {
  static readonly byte[] Magic=Encoding.ASCII.GetBytes("RRCAR001");
  internal sealed class Document {internal byte[] Native;internal CarSlotRegistry.State Layout;internal CarSlotMigration.Save State;}
  static bool Equal(byte[] a,byte[] b){if(a.Length!=b.Length)return false;int x=0;for(int i=0;i<a.Length;i++)x|=a[i]^b[i];return x==0;}
  internal static bool Has(byte[] bytes){if(bytes.Length<16)return false;for(int i=0;i<8;i++)if(bytes[bytes.Length-8+i]!=Magic[i])return false;return true;}
  internal static byte[] Strip(byte[] bytes){if(!Has(bytes))return bytes;long size=BitConverter.ToInt64(bytes,bytes.Length-16);if(size<48||size>1048576||size>bytes.Length)throw new IOException("Invalid car save trailer length");var result=new byte[bytes.Length-(int)size];Buffer.BlockCopy(bytes,0,result,0,result.Length);return result;}
  internal static Document Read(byte[] bytes){
   if(!Has(bytes))return new Document{Native=bytes};byte[] native=Strip(bytes);int payload=bytes.Length-native.Length-48;
   using(var hash=SHA256.Create()){byte[] expected=hash.ComputeHash(bytes,0,native.Length+payload);var actual=new byte[32];Buffer.BlockCopy(bytes,native.Length+payload,actual,0,32);if(!Equal(expected,actual))throw new IOException("Car save trailer does not match native snapshot");}
   using(var r=new BinaryReader(new MemoryStream(bytes,native.Length,payload),Encoding.UTF8)){
    if(r.ReadInt32()!=1)throw new IOException("Unsupported car save schema");string layout=r.ReadString();if(layout.Length>262144)throw new IOException("Oversized car save layout");var state=new CarSlotMigration.Save();state.Selected=r.ReadInt32();if(state.Selected<0||state.Selected>=22)throw new IOException("Invalid saved selection");
    for(int i=0;i<22;i++)state.Slots[i]=Value(r);int count=r.ReadInt32();if(count<0||count>4096)throw new IOException("Too many saved car identities");for(int i=0;i<count;i++){string key=r.ReadString();if(key.Length==0||key.Length>256||state.Retired.ContainsKey(key))throw new IOException("Invalid saved car identity");state.Retired.Add(key,Value(r));}if(r.BaseStream.Position!=r.BaseStream.Length)throw new IOException("Unexpected car save data");return new Document{Native=native,Layout=CarSlotRegistry.Decode(layout),State=state};
   }
  }
  static CarSlotMigration.Value Value(BinaryReader r){var v=new CarSlotMigration.Value{Paint=r.ReadInt32(),Driven=r.ReadInt32(),Owned=r.ReadBoolean(),Packs=r.ReadUInt16()};if(v.Paint<0||v.Paint>8||v.Driven<0||(v.Packs&~511)!=0)throw new IOException("Invalid saved custom car values");return v;}
  static void Value(BinaryWriter w,CarSlotMigration.Value v){w.Write(v.Paint);w.Write(v.Driven);w.Write(v.Owned);w.Write(v.Packs);}
  internal static byte[] Write(byte[] native,CarSlotRegistry.State layout,CarSlotMigration.Save state){
   native=Strip(native);using(var m=new MemoryStream())using(var w=new BinaryWriter(m,Encoding.UTF8)){w.Write(native);w.Write(1);w.Write(CarSlotRegistry.Encode(layout));w.Write(state.Selected);foreach(var v in state.Slots)Value(w,v);w.Write(state.Retired.Count);var keys=new List<string>(state.Retired.Keys);keys.Sort(StringComparer.Ordinal);foreach(string key in keys){w.Write(key);Value(w,state.Retired[key]);}w.Flush();using(var sha=SHA256.Create())w.Write(sha.ComputeHash(m.ToArray()));w.Write(m.Length-native.Length+16L);w.Write(Magic);w.Flush();byte[] result=m.ToArray();if(result.Length>1048576)throw new IOException("Car save exceeds the checkpoint size limit");Read(result);return result;}
  }
 }
}
