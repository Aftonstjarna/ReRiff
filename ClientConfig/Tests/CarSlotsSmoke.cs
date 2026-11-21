using System;
using System.IO;
using System.Linq;
using ReRiff;
class CarSlotsSmoke {
 static void Check(bool b,string s){if(!b)throw new Exception(s);}
 static CarSlotRegistry.Car Car(string id){return CarSlotRegistry.Package(id,"overwrite=0\n[origins]\nalf=alf\n")[0];}
 static void Refuse(Action a){try{a();}catch(IOException){return;}throw new Exception("Expected refusal");}
 static string Encoded(CarSlotRegistry.State state){state.Revision=CarSlotRegistry.Hash(CarSlotRegistry.Encode(state,false));return CarSlotRegistry.Encode(state);}
 static void Invalid(Action<CarSlotRegistry.Car> change){var state=CarSlotRegistry.Plan(null,new[]{Car("fixture")});change(state.Cars[0]);Refuse(()=>CarSlotRegistry.Decode(Encoded(state)));}
 static void RegistryGuards(){
  var explicitPackage=CarSlotRegistry.Package("pack folder","id=customs-012345abcdef\noverwrite=0\n[origins]\nabc=alf\n");
  explicitPackage[0].Active=false;
  var explicitPlan=CarSlotRegistry.Plan(null,explicitPackage);
  Check(explicitPlan.Cars[0].Active,"requested cars activated despite stale flag");
  Check(CarSlotRegistry.Encode(CarSlotRegistry.Decode(Encoded(explicitPlan)))==CarSlotRegistry.Encode(explicitPlan),"explicit package identity roundtrip");
  foreach(string identity in new[]{"", ".", "..", "bad/name", "bad\\name", "bad|name", "bad=name", "bad[name", "bad]name", "bad\tname",new string('x',129)})Refuse(()=>CarSlotRegistry.Package("folder","id="+identity+"\noverwrite=0\n[origins]\nabc=alf\n"));
  var valid=CarSlotRegistry.Plan(null,new[]{Car("fixture")});string encoded=Encoded(valid);
  foreach(string field in new[]{"key","package","source","base","native","slot","active","overwrite"}){
   string missing=String.Join("\n",encoded.Split('\n').Where(line=>!line.StartsWith(field+"=",StringComparison.Ordinal)));
   Refuse(()=>CarSlotRegistry.Decode(missing));
  }
  foreach(string flag in new[]{"active","overwrite"})foreach(string value in new[]{"2","-1","true",""})Refuse(()=>CarSlotRegistry.Decode(encoded.Replace(flag+"=0",flag+"="+value).Replace(flag+"=1",flag+"="+value)));
  foreach(string key in new[]{"", "no-separator", "|alf", "fixture|bad|alf", "fixture|bui",new string('x',129)+"|alf","bad\tname|alf","bad=name|alf","bad[name|alf"})Invalid(c=>c.Key=key);
  foreach(string package in new[]{"", ".", "..", "../fixture", "fixture/sub", "fixture\\sub", "bad\tname",new string('x',256)})Invalid(c=>c.Package=package);
  foreach(string source in new[]{"", "abcd", "ABC", "a/1"})Invalid(c=>{c.Source=source;c.Key="fixture|"+source;});
  Invalid(c=>c.Base="zzz");
  foreach(int slot in new[]{-1,0,14,22})Invalid(c=>c.Slot=slot);
  foreach(string native in new[]{"", "alf", "r01", "r07", "../"})Invalid(c=>c.Native=native);
  Invalid(c=>c.Active=false);Invalid(c=>{c.Active=false;c.Slot=-1;c.Native="r07";});
  var retired=CarSlotRegistry.Plan(valid,new CarSlotRegistry.Car[0]);Check(CarSlotRegistry.Decode(Encoded(retired)).Cars[0].Native=="r00","inactive native identity retained");
  var authored=CarSlotRegistry.Plan(null,CarSlotRegistry.Package("authored","overwrite=0\n[origins]\ndod=mot\n"));
  Check(authored.Cars[0].Native=="dod","authored non-stock identity reused for localization");
  var replacement=CarSlotRegistry.Plan(null,CarSlotRegistry.Package("replacement","overwrite=1\n[origins]\ndod=alf\n"));
  Check(CarSlotRegistry.Decode(Encoded(replacement)).Cars[0].Slot==0,"replacement roundtrip");
  var oldReplacement=CarSlotRegistry.Plan(replacement,new CarSlotRegistry.Car[0]);Check(CarSlotRegistry.Decode(Encoded(oldReplacement)).Cars[0].Native=="alf","retired replacement identity retained");
  replacement.Cars[0].Slot=1;Refuse(()=>CarSlotRegistry.Decode(Encoded(replacement)));replacement.Cars[0].Slot=0;replacement.Cars[0].Native="bui";Refuse(()=>CarSlotRegistry.Decode(Encoded(replacement)));
  var duplicateSlot=CarSlotRegistry.Plan(null,new[]{Car("a"),Car("b")});duplicateSlot.Cars[1].Slot=15;duplicateSlot.Cars[1].Native="r00";Refuse(()=>CarSlotRegistry.Decode(Encoded(duplicateSlot)));
  var duplicates=CarSlotRegistry.Plan(null,new[]{Car("a")});duplicates.Cars.Add(duplicates.Cars[0].Copy());Refuse(()=>CarSlotRegistry.Decode(Encoded(duplicates)));
  var tooMany=new CarSlotRegistry.State();for(int i=0;i<4097;i++){var car=Car("retired"+i);car.Active=false;car.Native="r00";tooMany.Cars.Add(car);}Refuse(()=>CarSlotRegistry.Decode(Encoded(tooMany)));
 }
 static void Main(){
  RegistryGuards();
  var a=Car("a");var b=Car("b");var c=Car("c");var first=CarSlotRegistry.Plan(null,new[]{a,b});Check(first.Cars[0].Slot==15&&first.Cars[1].Slot==16,"assignment");
  Check(CarSlotRegistry.Encode(CarSlotRegistry.Decode(CarSlotRegistry.Encode(first)))==CarSlotRegistry.Encode(first),"roundtrip");
  var second=CarSlotRegistry.Plan(first,new[]{b,c});Check(second.Cars.Single(x=>x.Key==b.Key).Slot==15&&second.Cars.Single(x=>x.Key==c.Key).Slot==16,"reuse");
  var save=new CarSlotMigration.Save();save.Selected=16;save.Slots[15].Owned=true;save.Slots[15].Paint=4;save.Slots[15].Packs=31;save.Slots[15].Driven=42;save.Slots[16].Paint=2;save.Slots[16].Driven=7;save.Slots[0].Driven=500;
  var moved=CarSlotMigration.Move(save,first,second);Check(moved.Selected==15&&moved.Slots[15].Paint==2&&moved.Slots[15].Driven==7,"moved selection and state");Check(!moved.Slots[16].Owned&&moved.Slots[16].Packs==0&&moved.Slots[16].Driven==0,"new occupant clean");Check(moved.Slots[0].Driven==500&&save.Selected==16,"stock and input preserved");
  var third=CarSlotRegistry.Plan(second,new[]{a,b,c});var restored=CarSlotMigration.Move(moved,second,third);int slot=third.Cars.Single(x=>x.Key==a.Key).Slot;Check(restored.Slots[slot].Paint==4&&restored.Slots[slot].Owned&&restored.Slots[slot].Packs==31&&restored.Slots[slot].Driven==42,"reinstall restored");
  save.Selected=15;Check(CarSlotMigration.Move(save,first,second).Selected==0,"removed selection donor");
  var overwrite=CarSlotRegistry.Package("replacement","[mod]\noverwrite=1\n[origins]\ndod=alf\n")[0];Check(CarSlotRegistry.Plan(null,new[]{overwrite}).Cars[0].Slot==0,"overwrite stock slot");
  Refuse(()=>CarSlotRegistry.Package("bad","overwrite=2\n[origins]\ndod=alf"));Refuse(()=>CarSlotRegistry.Plan(null,new[]{a,a}));Refuse(()=>CarSlotRegistry.Plan(null,Enumerable.Range(0,8).Select(i=>Car("m"+i))));
  Console.WriteLine("PASS: slot allocation/reuse, donor replacement, registry integrity, state migration, removal/reinstall and capacity guards");
 }
}
