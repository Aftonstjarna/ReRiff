using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using ReRiff;

namespace ReRiff {
    // Transport logging dependencies only. The production bridge and original
    // call adapter are compiled unchanged into this test executable.
    public class ClientConfig {}
    internal static class Logging {internal static void Append(string folder,string file,string text){}}
    internal static class ProgressionDomain {internal static bool Enabled(string folder){throw new NotSupportedException("Fixture uses direct binding, not live opt-in configuration");}}
}
class ProgressionNativeSmoke {
    private delegate void TokenDelta(IntPtr progression,int delta);
    private delegate int PriceGetter(IntPtr paint,int car,int pack);
    static void Check(bool value,string text){if(!value)throw new Exception(text);}
    static byte[] Bytes(IntPtr pointer,int size){var result=new byte[size];Marshal.Copy(pointer,result,0,size);return result;}
    static void Fill(IntPtr pointer,int size,byte value){Marshal.Copy(Enumerable.Repeat(value,size).ToArray(),0,pointer,size);}
    static bool Refused(Action action){try{action();return false;}catch(IOException){return true;}catch(InvalidOperationException){return true;}}
    static int Main(string[] args){
        Check(IntPtr.Size==4,"Run this fixture under a 32-bit CLR");
        var type=Assembly.LoadFile(Path.GetFullPath(args[0])).GetType("Fixture.OriginalProgression",true);
        IntPtr root=Marshal.AllocHGlobal(64),settings=Marshal.AllocHGlobal(328),allocation=Marshal.AllocHGlobal(328),achievements=Marshal.AllocHGlobal(52),paint=Marshal.AllocHGlobal(796),packs=Marshal.AllocHGlobal(9*176),scores=Marshal.AllocHGlobal(256);
        IntPtr progression=IntPtr.Add(allocation,16),driver=Marshal.AllocHGlobal(12),thresholds=Marshal.AllocHGlobal(41*4);
        try {
            ProgressionEconomy.BindAndValidate(type.GetMethod("gaVehiclePaintManager.getPackPrice"),type.GetMethod("gaSaveGameChunkProgression.incTokens"),type.GetMethod("gaVehiclePaintManager.getPackIAPIndex"));
            type.GetField("SaveObserver").SetValue(null,new Func<IntPtr,bool>(pointer=>true));
            var native=new ProgressionNative((token,name)=>type.GetMethod(name,BindingFlags.Public|BindingFlags.Static),()=>root,()=>achievements,()=>paint);
            Fill(root,64,0);Marshal.WriteByte(root,12,1);Marshal.WriteIntPtr(root,16,settings);Marshal.WriteIntPtr(root,32,progression);Marshal.WriteIntPtr(root,48,scores);
            type.GetField("SaveRoot").SetValue(null,root);
            for(int i=0;i<41;i++)Marshal.WriteInt32(thresholds,i*4,i*1000);
            Marshal.WriteIntPtr(driver,0,thresholds);Marshal.WriteIntPtr(driver,4,IntPtr.Add(thresholds,41*4));Marshal.WriteIntPtr(driver,8,IntPtr.Add(thresholds,41*4));
            Fill(paint,796,0);Fill(packs,9*176,0);Fill(scores,256,0x73);
            for(int car=0;car<22;car++){
                Marshal.WriteIntPtr(paint,car*12,packs);Marshal.WriteIntPtr(paint,car*12+4,IntPtr.Add(packs,9*176));Marshal.WriteIntPtr(paint,car*12+8,IntPtr.Add(packs,9*176));
            }
            Action<int,int> initialize=delegate(int car,int pack){
                Fill(allocation,328,0xa5);Marshal.WriteByte(progression,5,1);
                for(int i=0;i<39;i++)Marshal.WriteByte(progression,12+i,1);
                Marshal.WriteInt32(progression,280,1234);Marshal.WriteInt32(progression,284,9999);
                Fill(settings,328,0x2c);Marshal.WriteByte(settings,5,1);Marshal.WriteInt32(settings,324,car);Marshal.WriteInt32(settings,236+4*car,pack);
                Fill(achievements,52,0);for(int i=0;i<39;i++)Marshal.WriteByte(achievements,i,1);
            };
            int scenarios=0;
            for(int car=0;car<22;car++)for(int pack=0;pack<9;pack++){
                initialize(car,pack);byte[] expected=Bytes(settings,328);
                Check(native.Eligible(),"Complete native fixture eligible");native.SaveCurrent();native.ResetAndRetain();scenarios++;
                Check(Bytes(settings,328).SequenceEqual(expected),"Original settings and selection preserved");
                Check(Bytes(scores,256).All(b=>b==0x73),"High-score/race-record sentinel preserved");
                Check(Bytes(allocation,16).All(b=>b==0xa5)&&Bytes(IntPtr.Add(progression,296),16).All(b=>b==0xa5),"Original reset remains within the native chunk bounds");
                Check(!native.Eligible(),"A wiped profile is ineligible for a second wipe");
                var observed=native.ObserveCheckpoint();
                Check((int)observed["cycleXp"]==0&&(int)observed["coins"]==0,"Server observation matches original reset counters");
                Check(((int[])observed["achievements"]).SequenceEqual(new[]{18}),"Server observation matches First Prestige");
                Check(((int[])observed["ownedCars"]).SequenceEqual(new[]{car})&&((int[])observed["ownedPacks"]).SequenceEqual(new[]{car*9+pack}),"Server observation matches original retained ownership");
                Check(Marshal.ReadByte(progression,12+18)==1&&Marshal.ReadByte(achievements,18)==1,"First Prestige flag is retained through each reset without platform award calls");
                Check(Marshal.ReadByte(progression,4)==1,"Original setters mark progression dirty");
            }
            initialize(2,3);byte[] untouched=Bytes(progression,296);
            Marshal.WriteInt32(achievements,44,4);
            Check(!native.Eligible()&&Refused(()=>native.ResetAndRetain())&&Bytes(progression,296).SequenceEqual(untouched),"Pending popup queue refuses mutation");
            initialize(2,3);Marshal.WriteByte(progression,12+18,0);Check(native.Eligible(),"Hidden storage slot 18 is excluded exactly as native Profile");
            initialize(2,3);Marshal.WriteByte(progression,12+38,0);Check(!native.Eligible()&&Refused(()=>native.ResetAndRetain()),"Incomplete achievements refused");
            initialize(2,0);Marshal.WriteInt32(settings,324,22);Check(!native.Eligible(),"Invalid car rejected before indexing");
            initialize(2,9);Check(!native.Eligible(),"Invalid pack rejected before indexing");
            initialize(2,8);Marshal.WriteIntPtr(paint,2*12+4,IntPtr.Add(packs,8*176));Check(!native.Eligible(),"Selected pack must exist in native paint definitions");Marshal.WriteIntPtr(paint,2*12+4,IntPtr.Add(packs,9*176));
            initialize(2,3);Marshal.WriteByte(root,13,1);Check(!native.Eligible()&&Refused(()=>native.SaveCurrent()),"Skip-save state blocks native saving and reset");Marshal.WriteByte(root,13,0);
            initialize(2,3);
            string temporary=Path.Combine(Path.GetTempPath(),"reriff-original-reset-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(temporary);
            try {
                string path=Path.Combine(temporary,"reriff-sp-fixture.bin");
                Func<byte[]> snapshot=()=>Bytes(progression,296).Concat(Bytes(settings,328)).Concat(Bytes(scores,256)).ToArray();
                byte[] preferences=Bytes(settings,328);
                File.WriteAllBytes(path,snapshot());ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,1234);
                type.GetField("SaveObserver").SetValue(null,new Func<IntPtr,bool>(pointer=>{
                    Check(pointer==root,"Adapter saves the original root");File.WriteAllBytes(path,snapshot());
                    ProgressionCheckpoints.Process(path,2,Marshal.ReadInt32(progression,280));return true;
                }));
                native.CommitConfirmed();
                var store=new ProgressionStore(Path.Combine(temporary,Path.Combine("ReRiff-progression","reriff-sp-fixture")),"singleplayer|reriff-sp-fixture");
                var checkpoint=store.Read();
                Check(checkpoint.Progress.MainXp==1234&&checkpoint.Progress.CycleXp==0&&checkpoint.Progress.PrestigeCount==1,"Original reset adapter commits permanent/cycle counters through the real controller");
                Check(checkpoint.NativeSave.SequenceEqual(snapshot()),"Reset ownership/settings and counters share one snapshot");
                Check(Bytes(settings,328).SequenceEqual(preferences)&&Bytes(scores,256).All(b=>b==0x73),"Combined transaction preserves selection and race records");
                Check(Refused(()=>native.CommitConfirmed())&&store.Read().Progress.PrestigeCount==1,"Second reset requires earning achievements again");
                ProgressionCheckpoints.Process(path,0,0);ProgressionCheckpoints.Process(path,1,0);
                Check(store.Read().Progress.PrestigeCount==1,"Combined reset checkpoint survives simulated native reload");
                var calculate=(ProgressionView.ProfileLevel)NativeReelBridge.CreateCall(type.GetMethod("gaDriverLevel.getDriverProfileLevel"),typeof(ProgressionView.ProfileLevel),false);
                byte[] untouchedProgression=Bytes(progression,296);
                Check(calculate(driver,IntPtr.Zero)==0,"Native level still reads zero cycle XP");
                var display=ProgressionCheckpoints.DisplayProgress(0);
                Check((int)ProgressionView.PermanentLevel(driver,display.MainXp,calculate)==1,"Permanent display uses saved main XP through original level helper");
                float barFill;Check((int)ProgressionView.PermanentLevel(driver,display.MainXp,calculate,out barFill)==1
                    &&Math.Abs(barFill-((float)(display.MainXp-1000UL)/1000f))<0.001f,"Permanent display bar uses XP within the current native level");
                Check(ProgressionView.PermanentLevel(driver,40000,calculate)==40&&ProgressionView.PermanentLevel(driver,41500,calculate)==41.5m,"Permanent main levels extend the original final interval");
                Check(ProgressionView.PermanentLevel(driver,UInt64.MaxValue,calculate)==Int32.MaxValue,"Uncapped permanent identity saturates safely at its integer representation");
                var oldPolicy=ProgressionBalance.Current;
                try{ProgressionBalance.Current=new ProgressionBalance{MainLevelCap=40};Check(ProgressionView.PermanentLevel(driver,UInt64.MaxValue,calculate)==40,"Configurable main level cap can retain the original maximum");}
                finally{ProgressionBalance.Current=oldPolicy;}
                Check(Bytes(progression,296).SequenceEqual(untouchedProgression),"Displaying permanent identity never changes cycle progression");
                Check(ProgressionView.Identity(40,0)=="40"&&ProgressionView.Identity(40,1)=="40 *"&&ProgressionView.Identity(40,2)=="40 *x2","Prestige identity and no font-pack dependency");
                Check(ProgressionView.Identity(100,1)=="100 *","Extended main identity retains the native star marker");
                Check(ProgressionCheckpoints.DisplayProgress(50).MainXp==1284&&store.Read().Progress.MainXp==1234,"Display accounts for unsaved native XP without writing counters");
                Marshal.WriteInt32(thresholds,4,0);Check(Refused(()=>ProgressionView.PermanentLevel(driver,1234,calculate)),"Malformed native level table refused");Marshal.WriteInt32(thresholds,4,1000);
                var coins=(TokenDelta)NativeReelBridge.CreateCall(type.GetMethod("gaSaveGameChunkProgression.incTokens"),typeof(TokenDelta),false);
                Marshal.WriteInt32(progression,284,Int32.MaxValue-10);coins(progression,ProgressionRewards.Coins(100,Int32.MaxValue-10,2,1m,1m,null));
                Check(Marshal.ReadInt32(progression,284)==Int32.MaxValue,"Bounded multiplier avoids overflow in original coin addition");
                Marshal.WriteInt32(progression,284,100);coins(progression,ProgressionRewards.Coins(-12,100,99,1m,1m,null));
                Check(Marshal.ReadInt32(progression,284)==88,"Original purchase debit remains unmultiplied");
                // Separate purchase ledger: execute the real price callback,
                // native relock setter and original signed debit on fixtures.
                IntPtr results=Marshal.AllocHGlobal(400);
                try {
                    Fill(results,400,0);Marshal.WriteInt32(results,388,180);Marshal.WriteInt32(results,392,100);Marshal.WriteInt32(results,396,1000);
                    Marshal.WriteInt32(progression,284,1000);
                    int reward=ProgressionEconomy.ApplyRaceQuote(results,280,1000,1);coins(progression,reward);
                    Check(reward==560&&Marshal.ReadInt32(results,388)==360&&Marshal.ReadInt32(results,392)==200&&Marshal.ReadInt32(progression,284)==1560,"Native race display components and original credited coins match at prestige one");
                    byte[] unchanged=Bytes(results,400);
                    Check(Refused(()=>ProgressionEconomy.ApplyRaceQuote(results,560,999,1))&&Bytes(results,400).SequenceEqual(unchanged),"Mismatched pre-award balance refuses without altering display");
                    Marshal.WriteInt32(results,388,100);Marshal.WriteInt32(results,392,100);Marshal.WriteInt32(results,396,Int32.MaxValue-150);
                    reward=ProgressionEconomy.ApplyRaceQuote(results,200,Int32.MaxValue-150,1);
                    Check(reward==150&&Marshal.ReadInt32(results,388)==150&&Marshal.ReadInt32(results,392)==0,"Capped display sum equals capped credit");
                }finally{Marshal.FreeHGlobal(results);}
                initialize(0,0);
                var setOwned=(ProgressionNative.SetPack)NativeReelBridge.CreateCall(type.GetMethod("gaSaveGameChunkProgression.setPackUnlocked"),typeof(ProgressionNative.SetPack),false);
                var owns=(ProgressionNative.HasPack)NativeReelBridge.CreateCall(type.GetMethod("gaSaveGameChunkProgression.isPackUnlocked"),typeof(ProgressionNative.HasPack),false);
                var rawPrice=(PriceGetter)NativeReelBridge.CreateCall(type.GetMethod("gaVehiclePaintManager.getPackPrice"),typeof(PriceGetter),false);
                setOwned(progression,0,1,true);Marshal.WriteInt32(packs,176+36,1000);Marshal.WriteInt32(packs,176+40,-1);
                for(int i=0;i<41;i++)Marshal.WriteInt32(thresholds,i*4,i*37);
                typeof(ProgressionNative).GetProperty("Bound",BindingFlags.NonPublic|BindingFlags.Static).GetSetMethod(true).Invoke(null,new object[]{native});
                typeof(ProgressionView).GetField("nativeLevel",BindingFlags.NonPublic|BindingFlags.Static).SetValue(null,calculate);
                typeof(ProgressionView).GetField("driver",BindingFlags.NonPublic|BindingFlags.Static).SetValue(null,new Func<IntPtr>(()=>driver));
                string purchasePath=Path.Combine(temporary,"reriff-sp-purchase-fixture.bin");
                File.WriteAllBytes(purchasePath,snapshot());ProgressionCheckpoints.Process(purchasePath,0,0);ProgressionCheckpoints.Process(purchasePath,1,1234);
                type.GetField("SaveObserver").SetValue(null,new Func<IntPtr,bool>(pointer=>{File.WriteAllBytes(purchasePath,snapshot());ProgressionCheckpoints.Process(purchasePath,2,Marshal.ReadInt32(progression,280));return true;}));
                IntPtr thunkPointer=(IntPtr)typeof(ProgressionEconomy).GetMethod("ReRiffGetProgressionEconomyThunk",BindingFlags.NonPublic|BindingFlags.Static).Invoke(null,new object[]{0});
                Delegate quote=ProgressionEconomy.Thunk(thunkPointer,true);
                for(int i=0;i<5;i++)Check((int)quote.DynamicInvoke(paint,0,1)==750,"Shared getter returns one non-compounding 25 percent coin quote");
                Check(rawPrice(paint,0,1)==1000,"Native base definitions remain untouched");
                Check(ProgressionEconomy.CoinPackPrice(1000,0,33)==1000&&ProgressionEconomy.CoinPackPrice(-1,-1,33)==-1,"IAP and sentinel prices preserved");
                bool changed=false;byte[] oldSettings=Bytes(settings,328),oldFlags=Bytes(IntPtr.Add(progression,12),39);
                native.RelockForPurchaseTest(0,1,ref changed);
                Check(changed&&!owns(progression,0,1)&&Marshal.ReadInt32(progression,284)==9999,"Only authorized paint ownership removed; coins retained");
                Check(Bytes(settings,328).SequenceEqual(oldSettings)&&Bytes(IntPtr.Add(progression,12),39).SequenceEqual(oldFlags),"Relock preserves equipped settings and achievements");
                ulong relockGeneration=ProgressionCheckpoints.Generation;native.RelockForPurchaseTest(0,1,ref changed);
                Check(!changed&&ProgressionCheckpoints.Generation==relockGeneration,"Already-locked target is idempotent");
                File.WriteAllText(ProgressionCheckpoints.PurchaseTestRequest,"relock-car-0-pack-1-v1");
                Check(ProgressionPurchaseTest.Run()==1&&!File.Exists(ProgressionCheckpoints.PurchaseTestRequest)&&File.Exists(ProgressionCheckpoints.PurchaseTestRequest+".consumed"),"Explicit test request consumed once");
                int amount=(int)quote.DynamicInvoke(paint,0,1);coins(progression,-amount);setOwned(progression,0,1,true);native.SaveCurrent();
                Check(Marshal.ReadInt32(progression,284)==9999-amount&&owns(progression,0,1),"Original debit equals the shared quote and restores ownership");
                Check(ProgressionPurchaseTest.Run()==1&&owns(progression,0,1),"Consumed request cannot relock the repurchased pack");
                typeof(ProgressionPurchaseTest).GetField("inspected",BindingFlags.NonPublic|BindingFlags.Static).SetValue(null,false);
                Check(ProgressionPurchaseTest.Run()==1&&owns(progression,0,1),"Simulated process restart does not replay the test request");
                Marshal.WriteInt32(packs,176+40,0);Check((int)quote.DynamicInvoke(paint,0,1)==1000,"Marketplace pack bypasses the discount in the actual callback");
                Marshal.WriteInt32(settings,236,1);bool protectedChange=false;
                Check(Refused(()=>native.RelockForPurchaseTest(0,1,ref protectedChange))&&!protectedChange&&owns(progression,0,1),"Selected pack cannot be relocked");
                initialize(0,0);Marshal.WriteByte(progression,12+18,0);
                byte[] beforeConfirm=Bytes(progression,296),keptSettings=Bytes(settings,328);
                Check(ProgressionPrestige.Dispatch(99)==0,"Unknown native action cannot reset");
                Check(ProgressionPrestige.Dispatch(0)==1&&Bytes(progression,296).SequenceEqual(beforeConfirm),"Readiness with all 38 displayed achievements performs no writes");
                Check(ProgressionPrestige.Dispatch(1)==1,"Explicit confirmed action commits through native reset and serializer adapter");
                Check(ProgressionCheckpoints.DisplayProgress(0).PrestigeCount==1&&Marshal.ReadInt32(progression,284)==0&&Marshal.ReadInt32(progression,280)==0,"Confirmed action advances prestige and resets cycle/coins");
                Check(Bytes(settings,328).SequenceEqual(keptSettings)&&Bytes(scores,256).All(v=>v==0x73),"Confirmed callback retains equipped settings and records");
                Check(ProgressionPrestige.Dispatch(1)==0,"Confirmed action cannot replay on the reset profile");
            }finally{Directory.Delete(temporary,true);}
            Console.WriteLine("PASS: {0} car/pack combinations execute extracted original reset and ownership IL; equipped settings, race-record sentinel and memory bounds preserved; popup/readiness/eligibility guards pass. Save is stubbed; no Steam or real save calls.",scenarios);
            return 0;
        }finally{foreach(var pointer in new[]{root,settings,allocation,achievements,paint,packs,scores,driver,thresholds})Marshal.FreeHGlobal(pointer);}
    }
}
