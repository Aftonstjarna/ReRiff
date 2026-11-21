using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

namespace ReRiff {
    // Calls the original hash-verified IL, not a replacement save serializer or
    // achievement API. Construction binds methods only; it cannot reset a save.
    internal sealed class ProgressionNative {
        internal delegate void Reset(IntPtr self);
        internal delegate bool Save(IntPtr self);
        internal delegate void SetCar(IntPtr self,int car,bool unlocked);
        internal delegate void SetPack(IntPtr self,int car,int pack,bool unlocked);
        internal delegate bool HasCar(IntPtr self,int car);
        internal delegate bool HasPack(IntPtr self,int car,int pack);
        internal delegate int PackCount(IntPtr self,int car);
        internal static ProgressionNative Bound {get;private set;}
        private readonly Func<IntPtr> getSave,getAchievements,getPaint;
        private readonly Reset resetProgression,resetAchievements;
        private readonly Save save;
        private readonly SetCar setCar,setAchievement;
        private readonly SetPack setPack;
        private readonly HasCar hasCar;
        private readonly HasPack hasPack;
        private readonly PackCount packCount;
        [DllImport("kernel32.dll",ExactSpelling=true)]
        private static extern bool ReadProcessMemory(IntPtr process,IntPtr address,[Out] byte[] bytes,UIntPtr size,out UIntPtr received);
        internal static byte[] Read(IntPtr address,int size){
            var bytes=new byte[size];UIntPtr received;
            if(address==IntPtr.Zero||!ReadProcessMemory(new IntPtr(-1),address,bytes,(UIntPtr)(uint)size,out received)||received.ToUInt64()!=(ulong)size)
                throw new IOException("Native progression memory is unavailable");
            return bytes;
        }
        private static IntPtr Pointer(byte[] bytes,int offset){return new IntPtr(BitConverter.ToInt32(bytes,offset));}
        private static T Call<T>(Func<int,string,MethodInfo> resolve,int token,string name) where T:class {
            var method=resolve(token,name);
            if(method==null||!method.IsStatic||method.Name!=name)throw new IOException("Unexpected original progression method: "+name);
            return NativeReelBridge.CreateCall(method,typeof(T),false) as T;
        }
        internal static Func<IntPtr> Singleton(Module game,int token,string name){
            FieldInfo field=game.ResolveField(token);
            if(field==null||!field.IsStatic||field.Name!=name||!field.FieldType.IsPointer)throw new IOException("Unexpected original progression singleton");
            var method=new DynamicMethod("ReRiff_progression_singleton",typeof(IntPtr),Type.EmptyTypes,game,true);
            var il=method.GetILGenerator();il.Emit(OpCodes.Ldsfld,field);il.Emit(OpCodes.Conv_I);il.Emit(OpCodes.Ret);
            return (Func<IntPtr>)method.CreateDelegate(typeof(Func<IntPtr>));
        }
        // Only called after NativeReelBridge has checked the complete original
        // Game.exe SHA-256. A reset requires the separately confirmed Credits action.
        internal static void Configure(Module game){
            Bound=new ProgressionNative((token,name)=>game.ResolveMethod(token) as MethodInfo,
                Singleton(game,0x0400126f,"?ms_instance@gaSaveGame@@0PAV1@A"),
                Singleton(game,0x04000db1,"?ms_pInstance@gaAchievements@@0PAV1@A"),
                Singleton(game,0x0400147d,"?ms_pInstance@gaVehiclePaintManager@@0PAV1@A"));
            if(ProgressionDomain.Enabled(Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location))||ProgressionRemote.Enabled(Path.GetDirectoryName(typeof(ClientConfig).Assembly.Location)))ProgressionPrestige.Register();
            NativeReelBridge.Log("Original progression reset/ownership methods bound; isolated progression requires eligible manual Credits confirmation.");
        }
        internal ProgressionNative(Func<int,string,MethodInfo> resolve,Func<IntPtr> getSave,Func<IntPtr> getAchievements,Func<IntPtr> getPaint){
            if(IntPtr.Size!=4)throw new PlatformNotSupportedException("Original progression requires the 32-bit game ABI");
            this.getSave=getSave;this.getAchievements=getAchievements;this.getPaint=getPaint;
            resetProgression=Call<Reset>(resolve,0x06001254,"gaSaveGameChunkProgression.reset");
            resetAchievements=Call<Reset>(resolve,0x06000e82,"gaAchievements.reset");
            setAchievement=Call<SetCar>(resolve,0x06001259,"gaSaveGameChunkProgression.setAchieved");
            setCar=Call<SetCar>(resolve,0x0600125b,"gaSaveGameChunkProgression.setCarUnlocked");
            setPack=Call<SetPack>(resolve,0x0600125e,"gaSaveGameChunkProgression.setPackUnlocked");
            hasCar=Call<HasCar>(resolve,0x0600125c,"gaSaveGameChunkProgression.isCarUnlocked");
            hasPack=Call<HasPack>(resolve,0x0600125d,"gaSaveGameChunkProgression.isPackUnlocked");
            packCount=Call<PackCount>(resolve,0x0600138b,"gaVehiclePaintManager.getNumPacksForCar");
            save=Call<Save>(resolve,0x060011ed,"gaSaveGame.save");
        }
        private sealed class Snapshot {
            internal IntPtr Save,Progression,Settings,Achievements,Paint;
            internal byte[] Preferences;
            internal int Car,Pack;
        }
        private Snapshot Inspect(){
            var value=new Snapshot{Save=getSave(),Achievements=getAchievements(),Paint=getPaint()};
            byte[] root=Read(value.Save,52);
            if(root[12]!=1||root[13]!=0||root[14]!=0)throw new IOException("Native save is not ready for prestige");
            value.Progression=Pointer(root,32);value.Settings=Pointer(root,16);
            byte[] progression=Read(value.Progression,296);value.Preferences=Read(value.Settings,328);
            if(progression[5]!=1||value.Preferences[5]!=1)throw new IOException("Native progression saving is disabled");
            // Match original Profile list: 39 storage slots, skip internal slot 18.
            for(int i=0;i<39;i++)if(i!=18&&progression[12+i]!=1)throw new InvalidOperationException("All in-game achievements are required");
            byte[] achievements=Read(value.Achievements,52);
            // reset() clears achieved flags only; it does not drain the native
            // pending-popup vector. Wait for normal processing before reset.
            if(BitConverter.ToInt32(achievements,40)!=BitConverter.ToInt32(achievements,44))throw new IOException("Wait for pending achievement popups before prestige");
            value.Car=BitConverter.ToInt32(value.Preferences,324);
            if(value.Car<0||value.Car>=22)throw new IOException("Invalid equipped car");
            value.Pack=BitConverter.ToInt32(value.Preferences,236+4*value.Car);
            if(value.Pack<0||value.Pack>8)throw new IOException("Invalid equipped paint pack");
            byte[] vector=Read(IntPtr.Add(value.Paint,value.Car*12),12);
            uint first=BitConverter.ToUInt32(vector,0),last=BitConverter.ToUInt32(vector,4),end=BitConverter.ToUInt32(vector,8);
            if(first==0||last<first||end<last||(last-first)%176!=0||(last-first)/176>9)throw new IOException("Invalid native paint-pack vector");
            int count=packCount(value.Paint,value.Car);
            if(count!=(last-first)/176||value.Pack>=count)throw new IOException("Equipped paint pack is not defined for this car");
            return value;
        }
        internal bool Eligible(){try{Inspect();return true;}catch(IOException){return false;}catch(InvalidOperationException){return false;}}
        internal int CycleXp(){
            byte[] root=Read(getSave(),36);
            if(root[12]!=1||root[14]!=0)throw new IOException("Native cycle XP is not ready");
            int xp=BitConverter.ToInt32(Read(IntPtr.Add(Pointer(root,32),280),4),0);
            if(xp<0||xp>2000000000)throw new IOException("Invalid native cycle XP");return xp;
        }
        internal void CommitConfirmed(){
            ProgressionCheckpoints.CommitConfirmedPrestige(
                ()=>ProgressionCheckpoints.Ready&&Eligible(),SaveCurrent,ResetAndRetain,SaveCurrent);
        }
        internal int CoinBalance(){
            byte[] root=Read(getSave(),36);
            if(root[12]!=1||root[13]!=0||root[14]!=0)throw new IOException("Native save not ready for reward quote");
            return BitConverter.ToInt32(Read(IntPtr.Add(Pointer(root,32),284),4),0);
        }
        internal Dictionary<string,object> ObserveCheckpoint(){
            byte[] root=Read(getSave(),36);
            if(root[12]!=1||root[14]!=0)throw new IOException("Native save not ready for server checkpoint");
            IntPtr progression=Pointer(root,32);byte[] fields=Read(progression,296),settings=Read(Pointer(root,16),328);
            int car=BitConverter.ToInt32(settings,324);
            if(car<0||car>=22)throw new IOException("Invalid equipped car");
            int pack=BitConverter.ToInt32(settings,236+4*car);
            if(pack<0||pack>=9)throw new IOException("Invalid equipped paint");
            var achieved=new List<int>();var cars=new List<int>();var packs=new List<int>();
            for(int i=0;i<39;i++){if(fields[12+i]>1)throw new IOException("Invalid native achievement byte");if(fields[12+i]==1)achieved.Add(i);}
            for(int i=0;i<22;i++){
                if(hasCar(progression,i))cars.Add(i);
                for(int j=0;j<9;j++)if(hasPack(progression,i,j))packs.Add(i*9+j);
            }
            return new Dictionary<string,object>{{"cycleXp",BitConverter.ToInt32(fields,280)},{"coins",BitConverter.ToInt32(fields,284)},
                {"equippedCar",car},{"equippedPack",pack},{"achievements",achieved.ToArray()},{"ownedCars",cars.ToArray()},{"ownedPacks",packs.ToArray()}};
        }
        // Use the original ownership setters; they mark progression dirty without
        // invoking achievement/network awards.
        internal void RestoreCarOwnership(IntPtr progression,int car,bool owned,ushort packs){
            if(car<15||car>=22||(packs&~511)!=0)throw new IOException("Invalid custom car ownership");
            setCar(progression,car,owned);
            for(int pack=0;pack<9;pack++)setPack(progression,car,pack,(packs&(1<<pack))!=0);
        }
        internal IntPtr SaveAddress {get{return getSave();}}
        internal void SaveCurrent(){
            IntPtr current=getSave();byte[] root=Read(current,15);
            if(root[12]!=1||root[13]!=0||root[14]!=0||!save(current))throw new IOException("Native save refused progression checkpoint");
        }
#if RERIFF_LOCAL_PURCHASE_TEST
        internal string RelockForPurchaseTest(int car,int pack,ref bool changed){
            changed=false;
            if(!ProgressionCheckpoints.Ready||car<0||car>=22||pack<1||pack>8)throw new InvalidOperationException("Isolated purchase test is not ready");
            IntPtr rootPointer=getSave();byte[] root=Read(rootPointer,52);
            if(root[12]!=1||root[13]!=0||root[14]!=0)throw new IOException("Native save is not ready for the purchase test");
            IntPtr progression=Pointer(root,32),settings=Pointer(root,16),paint=getPaint();
            byte[] preferences=Read(settings,328),before=Read(progression,296);
            if(before[5]!=1||!hasCar(progression,car))throw new InvalidOperationException("The test car must already be owned and saveable");
            if(BitConverter.ToInt32(preferences,236+car*4)==pack)throw new InvalidOperationException("The test pack is selected; no purchase was relocked");
            byte[] vector=Read(IntPtr.Add(paint,car*12),12);
            uint first=BitConverter.ToUInt32(vector,0),last=BitConverter.ToUInt32(vector,4),end=BitConverter.ToUInt32(vector,8);
            if(first==0||last<first||end<last||(last-first)%176!=0||(last-first)/176>9||pack>=packCount(paint,car))throw new IOException("Invalid native purchase-test pack");
            var progress=ProgressionCheckpoints.DisplayProgress(BitConverter.ToInt32(before,280));
            int quote=ProgressionEconomy.TestCoinPackPrice(paint,car,pack,ProgressionView.MainLevel(progress));
            int balance=BitConverter.ToInt32(before,284);
            if(balance<quote)throw new InvalidOperationException("Test pack is not affordable; no purchase was relocked");
            if(!hasPack(progression,car,pack))return "Test pack was already locked: car="+car+" pack="+pack+" quoted="+quote;
            SaveCurrent();before=Read(progression,296);preferences=Read(settings,328);
            ulong generation=ProgressionCheckpoints.Generation;
            changed=true;setPack(progression,car,pack,false);
            byte[] expected=(byte[])before.Clone();expected[4]=1;
            int ownershipByte=53+3+2*car+(pack>>3);expected[ownershipByte]=(byte)(expected[ownershipByte]&~(1<<(pack&7)));
            if(!Read(progression,296).SequenceEqual(expected)||!Read(settings,328).SequenceEqual(preferences))throw new IOException("Purchase relock changed unexpected native fields; stop and recover");
            SaveCurrent();
            if(ProgressionCheckpoints.Generation!=checked(generation+1)||hasPack(progression,car,pack)
                ||BitConverter.ToInt32(Read(progression,296),284)!=balance)throw new IOException("Purchase relock did not commit exactly once; stop and recover");
            return "One authorized purchase relocked: car="+car+" pack="+pack+" quoted="+quote+" coins_unchanged="+balance+" achievements_and_selection_unchanged=1";
        }
#endif
        internal void ResetAndRetain(){
            Snapshot value=Inspect();
            resetProgression(value.Progression);resetAchievements(value.Achievements);
            // No original award site uses slot 18, and Profile excludes it.
            // Use the save-only setter: gaAchievements.setAchieved would also
            // invoke platform/network/unlock behavior. Its local manager has
            // no separate setter, so mirror this one verified local byte.
            setAchievement(value.Progression,18,true);Marshal.WriteByte(value.Achievements,18,1);
            setCar(value.Progression,value.Car,true);setPack(value.Progression,value.Car,value.Pack,true);
            byte[] progression=Read(value.Progression,296),achievements=Read(value.Achievements,39);
            if(!Read(value.Settings,328).SequenceEqual(value.Preferences))throw new IOException("Native reset changed equipped settings; stop and recover");
            if(BitConverter.ToInt32(progression,280)!=0||BitConverter.ToInt32(progression,284)!=0
                ||progression.Skip(12).Take(39).Where((b,i)=>i!=18).Any(b=>b!=0)||achievements.Where((b,i)=>i!=18).Any(b=>b!=0)
                ||progression[12+18]!=1||achievements[18]!=1)throw new IOException("Native reset failed to clear local progression");
            for(int car=0;car<22;car++){
                if(hasCar(value.Progression,car)!=(car==value.Car))throw new IOException("Retained car ownership differs from reset proposal");
                for(int pack=0;pack<9;pack++)if(hasPack(value.Progression,car,pack)!=(car==value.Car&&pack==value.Pack))
                    throw new IOException("Retained paint ownership differs from reset proposal");
            }
        }
    }
    // The native dialog calls readiness without mutation, then invokes action 1
    // only after Confirm and after the overlay has closed on the game thread.
    internal static class ProgressionPrestige {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]private delegate int ActionCallback(int action);
        private static readonly ActionCallback Callback=Dispatch;
        [DllImport("ReRiff.Notify.dll",CallingConvention=CallingConvention.Cdecl,ExactSpelling=true)]
        private static extern int ReRiffConfigurePrestigeAction(IntPtr callback);
        internal static void Register(){
            if(ReRiffConfigurePrestigeAction(Marshal.GetFunctionPointerForDelegate(Callback))!=1)throw new IOException("Native prestige confirmation registration refused");
        }
        internal static int Dispatch(int action){
            if(action!=0&&action!=1)return 0;
            var native=ProgressionNative.Bound;
            if(native==null||!ProgressionCheckpoints.CanPrestige||!native.Eligible())return 0;
            if(action==0)return 1;
            try {
                native.CommitConfirmed();
                NativeReelBridge.Log("Confirmed prestige committed; permanent XP, equipped car/paint and race records retained.");return 1;
            }catch(Exception error){
                NativeReelBridge.Log("Confirmed prestige transaction failed; stop and recover the last complete checkpoint: "+error);return -1;
            }
        }
    }

}
