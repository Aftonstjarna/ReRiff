using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using ReRiff;

namespace ReRiff {
 internal static class ModArchiveCache {internal static CarSlotRegistry.State CurrentCars,PreviousCars;}
 internal static class NativeReelBridge {internal static void Log(string message){Console.WriteLine(message);}}
 internal static class ProgressionCheckpoints {internal static Action<string> PrepareCarSave,ObserveCarLoad,AfterCarLoad;}
 internal static class CarFallback {
  internal static void Paint(IntPtr settings,int slot,int value){Marshal.WriteInt32(settings,236+slot*4,value);Marshal.WriteByte(settings,4,1);}
  internal static void Select(IntPtr settings,int slot){Marshal.WriteInt32(settings,324,slot);Marshal.WriteByte(settings,4,1);}
 }
 internal static class ProgressionNative {
  internal static readonly Binding Bound=new Binding();
  internal static byte[] Read(IntPtr address,int count){var bytes=new byte[count];Marshal.Copy(address,bytes,0,count);return bytes;}
  internal sealed class Binding {
   internal IntPtr SaveAddress;internal string File;internal int Saves;internal bool InLoad;
   internal void SaveCurrent(){if(InLoad)throw new Exception("Nested SaveCurrent during load observer");Saves++;ProgressionCheckpoints.PrepareCarSave(File);}
   internal void RestoreCarOwnership(IntPtr progression,int slot,bool owned,ushort packs){int offset=53+slot/8,mask=1<<(slot%8),value=Marshal.ReadByte(progression,offset);Marshal.WriteByte(progression,offset,(byte)(owned?value|mask:value&~mask));Marshal.WriteInt16(progression,56+slot*2,(short)packs);Marshal.WriteByte(progression,4,1);}
  }
 }
}
class CarSaveRuntimeSmoke {
 static void Check(bool value,string label){if(!value)throw new Exception(label);}
 static void Refuse(Action action,string label){try{action();}catch(IOException){return;}throw new Exception("Expected refusal: "+label);}
 static CarSlotRegistry.Car Car(string id,string donor){return CarSlotRegistry.Package(id,"overwrite=0\n[origins]\nalf="+donor+"\n")[0];}
 static byte[] Read(IntPtr p,int count){return ProgressionNative.Read(p,count);}
 static void Load(string file){ProgressionNative.Bound.File=file;ProgressionNative.Bound.InLoad=true;try{ProgressionCheckpoints.ObserveCarLoad(file);ProgressionCheckpoints.AfterCarLoad(file);}finally{ProgressionNative.Bound.InLoad=false;}}
 static void StockUnchanged(byte[] settings,byte[] progression,IntPtr s,IntPtr p){
  byte[] nowS=Read(s,328),nowP=Read(p,296);
  Check(settings.Skip(236).Take(60).SequenceEqual(nowS.Skip(236).Take(60)),"stock paints preserved");
  Check(progression.Skip(188).Take(60).SequenceEqual(nowP.Skip(188).Take(60)),"stock usage preserved");
  Check(progression.Skip(56).Take(30).SequenceEqual(nowP.Skip(56).Take(30)),"stock packs preserved");
  Check(progression[53]==nowP[53]&&(progression[54]&127)==(nowP[54]&127),"stock ownership bits preserved");
 }
 static int Main(){
  Check(IntPtr.Size==4,"run this test as 32-bit under Wine");string folder=Path.Combine(Path.GetTempPath(),"reriff-car-runtime-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(folder);string file=Path.Combine(folder,"profile.save");
  IntPtr root=Marshal.AllocHGlobal(52),settings=Marshal.AllocHGlobal(328),progression=Marshal.AllocHGlobal(296);
  try{
   Marshal.Copy(new byte[52],0,root,52);Marshal.Copy(new byte[328],0,settings,328);Marshal.Copy(new byte[296],0,progression,296);
   Marshal.WriteByte(root,12,1);Marshal.WriteInt32(root,16,settings.ToInt32());Marshal.WriteInt32(root,32,progression.ToInt32());Marshal.WriteByte(settings,5,1);Marshal.WriteByte(progression,5,1);
   var a=Car("a","alf");var b=Car("b","bui");var c=Car("c","cor");var before=CarSlotRegistry.Plan(null,new[]{a,b});var after=CarSlotRegistry.Plan(before,new[]{b,c});
   var state=new CarSlotMigration.Save{Selected=15};for(int i=0;i<15;i++){state.Slots[i].Paint=i%9;state.Slots[i].Driven=1000+i;state.Slots[i].Owned=i%2==0;state.Slots[i].Packs=(ushort)(i+1);}
   state.Slots[15]=new CarSlotMigration.Value{Paint=4,Driven=100,Owned=true,Packs=31};state.Slots[16]=new CarSlotMigration.Value{Paint=6,Driven=200,Owned=true,Packs=127};
   for(int i=0;i<22;i++){Marshal.WriteInt32(settings,236+i*4,state.Slots[i].Paint);Marshal.WriteInt32(progression,188+i*4,state.Slots[i].Driven);ProgressionNative.Bound.RestoreCarOwnership(progression,i,state.Slots[i].Owned,state.Slots[i].Packs);}Marshal.WriteInt32(settings,324,state.Selected);Marshal.WriteByte(settings,4,0);Marshal.WriteByte(progression,4,0);
   byte[] originalSettings=Read(settings,328),originalProgression=Read(progression,296),native=Enumerable.Range(0,777).Select(i=>(byte)i).ToArray();
   File.WriteAllBytes(file,CarSaveEnvelope.Write(native,before,state));ModArchiveCache.PreviousCars=before;ModArchiveCache.CurrentCars=after;ProgressionNative.Bound.SaveAddress=root;CarSaveRuntime.Configure();
   Check(ProgressionCheckpoints.PrepareCarSave!=null&&ProgressionCheckpoints.ObserveCarLoad!=null&&ProgressionCheckpoints.AfterCarLoad!=null,"checkpoint callbacks configured");
   Load(file);Check(ProgressionNative.Bound.Saves==0,"load observer does not recursively save");
   Check(Marshal.ReadInt32(settings,324)==0&&Marshal.ReadInt32(settings,236+15*4)==6&&Marshal.ReadInt32(progression,188+15*4)==200,"phase1 migration precedes native unlock continuation");
   Check(Marshal.ReadInt32(settings,236+16*4)==0&&Marshal.ReadInt32(progression,188+16*4)==0,"new occupant starts clean");StockUnchanged(originalSettings,originalProgression,settings,progression);
   CarSaveRuntime.Tick(root);CarSaveRuntime.Tick(root);Check(ProgressionNative.Bound.Saves==1,"main tick commits migration exactly once");
   var sealedDoc=CarSaveEnvelope.Read(File.ReadAllBytes(file));Check(sealedDoc.Native.SequenceEqual(native)&&sealedDoc.Layout.Revision==after.Revision&&sealedDoc.State.Retired[a.Key].Paint==4,"sealed native prefix and retired identity");
   byte[] sealedBytes=File.ReadAllBytes(file);CarSaveRuntime.Seal(file,root);Check(sealedBytes.SequenceEqual(File.ReadAllBytes(file)),"repeated Seal appends one deterministic envelope");
   byte[] unchangedS=Read(settings,328),unchangedP=Read(progression,296);Load(file);CarSaveRuntime.Tick(root);Check(ProgressionNative.Bound.Saves==1&&unchangedS.SequenceEqual(Read(settings,328))&&unchangedP.SequenceEqual(Read(progression,296)),"same layout does not mutate or save");
   // No installed packages still needs the envelope's layout to retire its custom cars.
   Marshal.WriteInt32(settings,324,15);ModArchiveCache.CurrentCars=null;Load(file);Check(CarSaveRuntime.Enabled&&Marshal.ReadInt32(settings,324)==1,"uninstalled selected car falls back to donor");CarSaveRuntime.Tick(root);Check(ProgressionNative.Bound.Saves==2,"uninstall migration committed once");
   var removed=CarSaveEnvelope.Read(File.ReadAllBytes(file));Check(removed.Layout.Cars.Count==0&&removed.State.Retired[a.Key].Paint==4&&removed.State.Retired[b.Key].Paint==6&&removed.Native.SequenceEqual(native),"uninstalled identities portable and native prefix retained");StockUnchanged(originalSettings,originalProgression,settings,progression);
   ModArchiveCache.CurrentCars=after;Load(file);Check(Marshal.ReadInt32(settings,236+15*4)==6&&Marshal.ReadInt32(progression,188+15*4)==200,"reread and reinstall restores retired state");CarSaveRuntime.Tick(root);Check(ProgressionNative.Bound.Saves==3,"reinstall committed once");
   string legacy=Path.Combine(folder,"legacy.save");File.WriteAllBytes(legacy,native);ProgressionCheckpoints.ObserveCarLoad(legacy);byte[] legacyS=Read(settings,328),legacyP=Read(progression,296);Refuse(()=>ProgressionCheckpoints.AfterCarLoad(legacy),"ambiguous legacy progress");
   Check(legacyS.SequenceEqual(Read(settings,328))&&legacyP.SequenceEqual(Read(progression,296))&&File.ReadAllBytes(legacy).SequenceEqual(native)&&ProgressionNative.Bound.Saves==3,"legacy refusal preserves memory and file");Refuse(()=>CarSaveRuntime.Seal(legacy,root),"save before successful migration");
   Console.WriteLine("PASS: phase1 migration, deferred single commit, unchanged layout, uninstall donor/retired state, stock preservation, Seal, reread/reinstall and legacy refusal");return 0;
  }finally{Marshal.FreeHGlobal(root);Marshal.FreeHGlobal(settings);Marshal.FreeHGlobal(progression);Directory.Delete(folder,true);}
 }
}
