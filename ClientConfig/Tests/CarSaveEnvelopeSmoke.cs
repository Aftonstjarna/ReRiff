using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using ReRiff;

class CarSaveEnvelopeSmoke {
 static void Check(bool condition,string message){if(!condition)throw new Exception(message);}
 static void Refuse(Action action,string message){try{action();}catch(IOException){return;}throw new Exception("Expected refusal: "+message);}
 static void Same(byte[] expected,byte[] actual,string message){Check(expected.SequenceEqual(actual),message);}
 static void SameValue(CarSlotMigration.Value a,CarSlotMigration.Value b,string message){Check(a.Paint==b.Paint&&a.Driven==b.Driven&&a.Owned==b.Owned&&a.Packs==b.Packs,message);}
 static void Value(BinaryWriter w,int paint=0,int driven=0,ushort packs=0){w.Write(paint);w.Write(driven);w.Write(true);w.Write(packs);}
 // Build independently of Write so malformed, correctly hashed payloads reach Read's guards.
 static byte[] Payload(string layout,int selected=0,int schema=1,Action<BinaryWriter> first=null,int count=0,Action<BinaryWriter> retired=null,bool extra=false){
  using(var m=new MemoryStream())using(var w=new BinaryWriter(m,Encoding.UTF8)){
   w.Write(schema);w.Write(layout);w.Write(selected);for(int i=0;i<22;i++){if(i==0&&first!=null)first(w);else Value(w);}
   w.Write(count);if(retired!=null)retired(w);if(extra)w.Write((byte)1);w.Flush();return m.ToArray();
  }
 }
 static byte[] Wrap(byte[] native,byte[] payload){
  using(var m=new MemoryStream())using(var w=new BinaryWriter(m,Encoding.UTF8)){
   w.Write(native);w.Write(payload);w.Flush();using(var hash=SHA256.Create())w.Write(hash.ComputeHash(m.ToArray()));
   w.Write((long)payload.Length+48);w.Write(Encoding.ASCII.GetBytes("RRCAR001"));w.Flush();return m.ToArray();
  }
 }
 static byte[] WithSize(byte[] bytes,long size){var copy=(byte[])bytes.Clone();Buffer.BlockCopy(BitConverter.GetBytes(size),0,copy,copy.Length-16,8);return copy;}
 static void Main(){
  byte[] native=Enumerable.Range(0,513).Select(i=>(byte)i).ToArray(),original=(byte[])native.Clone();
  var requests=CarSlotRegistry.Package("custom","overwrite=0\n[origins]\nabc=alf\ndef=bui\n");
  var initial=CarSlotRegistry.Plan(null,requests);var layout=CarSlotRegistry.Plan(initial,new[]{requests[1]});
  var save=new CarSlotMigration.Save{Selected=21};
  for(int i=0;i<22;i++){save.Slots[i].Paint=i%9;save.Slots[i].Driven=i==21?Int32.MaxValue:i*37;save.Slots[i].Owned=i%2==0;save.Slots[i].Packs=(ushort)(i==21?511:i);}
  save.Retired.Add("removed|車",new CarSlotMigration.Value{Paint=8,Driven=12345,Owned=true,Packs=511});
  save.Retired.Add("another|abc",new CarSlotMigration.Value());
  byte[] encoded=CarSaveEnvelope.Write(native,layout,save);var read=CarSaveEnvelope.Read(encoded);
  Check(CarSaveEnvelope.Has(encoded),"written marker");Same(original,native,"input native bytes unchanged");Same(native,encoded.Take(native.Length).ToArray(),"native prefix unchanged");Same(native,read.Native,"native read roundtrip");Same(native,CarSaveEnvelope.Strip(encoded),"native strip roundtrip");
  Check(CarSlotRegistry.Encode(layout)==CarSlotRegistry.Encode(read.Layout),"layout including inactive identities roundtrip");
  Check(read.State.Selected==21&&read.State.Slots.Length==22,"selection and fixed slots roundtrip");
  for(int i=0;i<22;i++)SameValue(save.Slots[i],read.State.Slots[i],"slot "+i);
  Check(read.State.Retired.Count==save.Retired.Count,"retired count");foreach(var item in save.Retired)SameValue(item.Value,read.State.Retired[item.Key],"retired "+item.Key);
  byte[] rewritten=encoded;for(int i=0;i<5;i++)rewritten=CarSaveEnvelope.Write(rewritten,layout,save);Same(encoded,rewritten,"repeated write deterministic without trailer growth");
  var reordered=new CarSlotMigration.Save{Selected=save.Selected};reordered.Slots=save.Slots;
  foreach(var item in save.Retired.Reverse())reordered.Retired.Add(item.Key,item.Value);
  Same(encoded,CarSaveEnvelope.Write(native,layout,reordered),"retired dictionary insertion order does not affect bytes");
  foreach(int index in new[]{0,native.Length,encoded.Length-48}){var damaged=(byte[])encoded.Clone();damaged[index]^=1;Refuse(()=>CarSaveEnvelope.Read(damaged),"native/payload/digest corruption at "+index);}
  foreach(long size in new long[]{-1,0,47,1048577,encoded.Length+1}){var damaged=WithSize(encoded,size);Refuse(()=>CarSaveEnvelope.Read(damaged),"trailer size "+size);Refuse(()=>CarSaveEnvelope.Strip(damaged),"strip size "+size);}
  foreach(byte[] legacy in new[]{new byte[0],new byte[15],native}){var old=CarSaveEnvelope.Read(legacy);Check(!CarSaveEnvelope.Has(legacy)&&old.Layout==null&&old.State==null,"legacy has no portable metadata");Same(legacy,old.Native,"legacy native bytes preserved");Same(legacy,CarSaveEnvelope.Strip(legacy),"legacy strip bytes preserved");}
  string text=CarSlotRegistry.Encode(layout);
  Action<byte[],string> reject=(payload,label)=>Refuse(()=>CarSaveEnvelope.Read(Wrap(native,payload)),label);
  reject(Payload(text,schema:2),"schema");reject(Payload(text,selected:-1),"negative selection");reject(Payload(text,selected:22),"selection above native slots");
  reject(Payload(text,first:w=>Value(w,paint:-1)),"negative paint");reject(Payload(text,first:w=>Value(w,paint:9)),"paint above palette");
  reject(Payload(text,first:w=>Value(w,driven:-1)),"negative distance");reject(Payload(text,first:w=>Value(w,packs:512)),"unknown pack bits");
  reject(Payload(text,count:-1),"negative retired count");reject(Payload(text,count:4097),"retired count limit");
  reject(Payload(text,count:1,retired:w=>{w.Write("");Value(w);}),"empty retired key");
  reject(Payload(text,count:1,retired:w=>{w.Write(new string('k',257));Value(w);}),"retired key length");
  reject(Payload(text,count:2,retired:w=>{for(int i=0;i<2;i++){w.Write("duplicate");Value(w);}}),"duplicate retired key");
  reject(Payload(text,count:1,retired:w=>{w.Write("retired");Value(w,paint:9);}),"invalid retired value");
  reject(Payload(text,extra:true),"unexpected trailing payload");reject(new byte[0],"truncated payload");
  reject(Payload(text).Take(6).ToArray(),"truncated string");reject(Payload(text).Take(Payload(text).Length-1).ToArray(),"truncated count");
  reject(Payload(text.Replace("schema=1","schema=2")),"invalid registry layout");
  string maxLayout=text+"\n#"+new string('x',262144-text.Length-2);
  Check(CarSaveEnvelope.Read(Wrap(native,Payload(maxLayout))).Layout.Revision==layout.Revision,"layout length upper bound accepted");
  reject(Payload(maxLayout+"x"),"layout length above bound");
  var maxKeys=CarSaveEnvelope.Read(Wrap(native,Payload(text,selected:21,first:w=>Value(w,8,Int32.MaxValue,511),count:4096,retired:w=>{for(int i=0;i<4096;i++){w.Write(i==0?new string('k',256):"retired-"+i);Value(w);}})));
  Check(maxKeys.State.Retired.Count==4096&&maxKeys.State.Slots[0].Paint==8&&maxKeys.State.Slots[0].Driven==Int32.MaxValue&&maxKeys.State.Slots[0].Packs==511,"value, key and count upper bounds accepted");
  var invalidSave=new CarSlotMigration.Save{Selected=22};Refuse(()=>CarSaveEnvelope.Write(native,layout,invalidSave),"write validates its result");
  byte[] nearLimit=Enumerable.Repeat((byte)0x5a,1048576-16).ToArray(),nearLimitBefore=(byte[])nearLimit.Clone();
  Refuse(()=>CarSaveEnvelope.Write(nearLimit,layout,save),"whole checkpoint exceeds 1 MiB after envelope");Same(nearLimitBefore,nearLimit,"oversized checkpoint refusal preserves native input");
  Console.WriteLine("PASS: native bytes, portable roundtrip, deterministic rewrites, integrity, legacy saves and payload bounds");
 }
}
