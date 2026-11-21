using System;
using System.IO;
using System.Collections.Generic;
using System.Runtime.InteropServices;
namespace ReRiff {
 internal static class CarSaveRuntime {
  static readonly CarSlotRegistry.State Empty=CarSlotRegistry.Plan(null,new CarSlotRegistry.Car[0]);
  internal static CarSlotRegistry.State Current {get{return ModArchiveCache.CurrentCars??(loaded!=null&&loaded.Layout!=null?Empty:null);}}
  internal static void Configure(){ProgressionCheckpoints.PrepareCarSave=file=>Seal(file,ProgressionNative.Bound.SaveAddress);ProgressionCheckpoints.ObserveCarLoad=Read;ProgressionCheckpoints.AfterCarLoad=file=>Apply(ProgressionNative.Bound.SaveAddress);}
  static CarSaveEnvelope.Document loaded;
  static string path;
  static bool applied,busy,pending;
  static Dictionary<string,CarSlotMigration.Value> retired=new Dictionary<string,CarSlotMigration.Value>(StringComparer.Ordinal);
  internal static bool Enabled {get{return Current!=null;}}
  internal static void Read(string file){path=file;loaded=File.Exists(file)?CarSaveEnvelope.Read(File.ReadAllBytes(file)):null;applied=false;pending=false;retired=new Dictionary<string,CarSlotMigration.Value>(StringComparer.Ordinal);}
  static CarSlotMigration.Save Capture(IntPtr save,out IntPtr settings,out IntPtr progression){
   byte[] root=ProgressionNative.Read(save,52);if(root[12]!=1||root[13]!=0||root[14]!=0)throw new IOException("Native save is not ready for car migration");
   settings=new IntPtr(BitConverter.ToInt32(root,16));progression=new IntPtr(BitConverter.ToInt32(root,32));byte[] s=ProgressionNative.Read(settings,328),p=ProgressionNative.Read(progression,296);
   if(s[5]!=1||p[5]!=1)throw new IOException("Native car saving disabled");var state=new CarSlotMigration.Save{Selected=BitConverter.ToInt32(s,324)};
   for(int i=0;i<22;i++){state.Slots[i].Paint=BitConverter.ToInt32(s,236+i*4);state.Slots[i].Driven=BitConverter.ToInt32(p,188+i*4);state.Slots[i].Owned=(p[53+i/8]&(1<<(i%8)))!=0;state.Slots[i].Packs=(ushort)(BitConverter.ToUInt16(p,56+i*2)&511);}
   foreach(var item in retired)state.Retired.Add(item.Key,item.Value.Copy());return state;
  }
  internal static void Tick(IntPtr save){
   if(!Enabled||busy)return;
   Apply(save);
   if(pending){ProgressionNative.Bound.SaveCurrent();pending=false;}
  }
  // The native file-close observer runs after all chunks have loaded and before
  // gaLoaderFE's existing checkPackUnlocks. Do not recursively save from load2.
  static bool LegacyAllowed(){
   try{string value;return ConfigFiles.Read(AppDomain.CurrentDomain.BaseDirectory,"diagnostics").TryGetValue("legacy_car_migration",out value)&&(value=="1"||value.Equals("true",StringComparison.OrdinalIgnoreCase));}catch{return false;}
  }
  static bool LegacyMatches(CarSlotMigration.Save input){
   var state=Current;if(state==null)return false;var slots=new HashSet<int>();
   foreach(var car in state.Cars)if(car.Active&&!car.Overwrite){if(car.Slot<15||car.Slot>=22||!slots.Add(car.Slot))return false;var value=input.Slots[car.Slot];if(!value.Owned&&value.Paint==0&&value.Driven==0&&value.Packs<=1)return false;}
   if(slots.Count==0)return false;
   if(input.Selected>=15&&!slots.Contains(input.Selected))return false;
   for(int i=15;i<22;i++)if(!slots.Contains(i)){var value=input.Slots[i];if(value.Owned||value.Paint!=0||value.Driven!=0||value.Packs>1)return false;}
   return true;
  }
  internal static void Apply(IntPtr save){
   if(!Enabled||applied||busy||save==IntPtr.Zero)return;
   if(path==null)throw new IOException("Automatic car slots require ReRiff isolated save checkpoints");
   busy=true;try{
    IntPtr settings,progression;var input=Capture(save,out settings,out progression);var old=loaded!=null&&loaded.Layout!=null?loaded.Layout:Empty;
    if(loaded==null||loaded.Layout==null){
     // A machine-wide previous registry cannot prove which layout wrote an
     // unlabelled profile (especially a save downloaded from another client).
     // Never attach an old occupant's progress to an arbitrary new package.
     // An explicit local opt-in may assert that the current active slots own
     // this progress, but only when every active slot and no other custom slot
     // carries legacy state.
     bool migrated=false;
     if(LegacyAllowed()&&LegacyMatches(input)){old=Current;migrated=true;NativeReelBridge.Log("Car slots: explicit legacy migration accepted; the next save records portable identities.");}
     if(!migrated){
      if(input.Selected>=15)throw new IOException("Legacy custom selection has no portable car identity; preserve this save for explicit migration, or set legacy_car_migration=1 under [diagnostics] in client.ini when the active car slots own it");
      for(int i=15;i<22;i++){var v=input.Slots[i];if(v.Owned||v.Paint!=0||v.Driven!=0||v.Packs>1)throw new IOException("Legacy custom progress has no portable car identity; preserve this save for explicit migration, or set legacy_car_migration=1 under [diagnostics] in client.ini when the active car slots own it");}
     }
    }
    if(loaded!=null&&loaded.State!=null)foreach(var item in loaded.State.Retired)input.Retired[item.Key]=item.Value.Copy();
    if(loaded!=null&&loaded.Layout!=null&&old.Revision==Current.Revision){
     retired=input.Retired;applied=true;return;
    }
    var next=CarSlotMigration.Move(input,old,Current);
    // Only the custom storage region moves. Stock settings/progression stay native.
    for(int i=15;i<22;i++){
     var v=next.Slots[i];CarFallback.Paint(settings,i,v.Paint);Marshal.WriteInt32(progression,188+i*4,v.Driven);
     ProgressionNative.Bound.RestoreCarOwnership(progression,i,v.Owned,v.Packs);
    }
    // Native has an increment-only usage API, so migration restores its
    // verified fixed counter array directly and marks that chunk dirty.
    Marshal.WriteByte(progression,4,1);
    CarFallback.Select(settings,next.Selected);retired=next.Retired;applied=true;pending=true;
    NativeReelBridge.Log("Car slots: migrated native selection, paints, ownership and usage by stable package identity.");
   }finally{busy=false;}
  }
  internal static void Seal(string file,IntPtr save){
   if(!Enabled)return;
   // Fresh profiles can save before the first menu tick. They have no old
   // custom state to reinterpret. Existing profiles must migrate first.
   if(!applied){if(loaded!=null)throw new IOException("Car save attempted before identity migration");applied=true;}
   IntPtr settings,progression;var state=Capture(save,out settings,out progression);byte[] native=File.ReadAllBytes(file);byte[] encoded=CarSaveEnvelope.Write(native,Current,state);Atomic(file,encoded);path=file;pending=false;
  }
  internal static void Atomic(string file,byte[] bytes){string temp=file+".car-"+Guid.NewGuid().ToString("N")+".tmp";try{using(var f=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){f.Write(bytes,0,bytes.Length);f.Flush(true);}if(File.Exists(file))File.Replace(temp,file,null);else File.Move(temp,file);}finally{if(File.Exists(temp))File.Delete(temp);}}
 }
}
