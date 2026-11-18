using System;
using ReRiff;
class ProgressionRewardsSmoke {
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static void Main(){
        for(uint count=0;count<10;count++)Check(ProgressionRewards.Coins(100,0,count,1m,1m,null)==100*(count+1),"Default count+1 multiplier");
        Check(ProgressionRewards.Coins(-100,500,99,100m,10m,null)==-100,"Purchases are not multiplied");
        Check(ProgressionRewards.Coins(Int32.MinValue,500,99,100m,10m,null)==Int32.MinValue,"Signed debit boundary preserved");
        Check(ProgressionRewards.Coins(0,0,99,1m,1m,null)==0,"Zero awards unchanged");
        Check(ProgressionRewards.Coins(100,0,10,1m,1m,3m)==300,"Multiplier cap");
        Check(ProgressionRewards.Coins(3,0,0,.5m,1m,null)==1,"Fractional award rounds down");
        Check(ProgressionRewards.Coins(100,Int32.MaxValue-5,1,1m,1m,null)==5,"Bound before native signed addition");
        Check(ProgressionRewards.Coins(Int32.MaxValue,1,UInt32.MaxValue,1000000m,1000000m,null)==Int32.MaxValue-1,"Extreme multiplication saturates without decimal overflow");
        Check(ProgressionRewards.Coins(100,Int32.MaxValue,0,0m,0m,null)==0,"No destination room");
        Check(ProgressionRewards.CycleXp(100,0,1m)==100&&ProgressionRewards.CycleXp(100,1999999999,1000000m)==1,"Cycle XP native cap enforced before addition");
        int[] levels={0,9,10,19,20,29,30,40,50};decimal[] rates={0,0,.10m,.10m,.20m,.20m,.25m,.275m,.2875m};
        for(int i=0;i<levels.Length;i++)Check(ProgressionRewards.DefaultDiscount(levels[i])==rates[i],"Discount milestone");
        Check(ProgressionRewards.DefaultDiscount(Int32.MaxValue)<=.30m,"Diminishing discount bounded and terminates");
        Check(ProgressionRewards.PaintPrice(1000,30,1m)==750&&ProgressionRewards.PaintPrice(0,30,1m)==0,"Discounted and free pack prices");
        Check(ProgressionRewards.PaintPrice(1,40,0m)==1&&ProgressionRewards.PaintPrice(Int32.MaxValue,0,1000000m)==Int32.MaxValue,"Paid minimum and upper bound");
        for(uint count=0;count<5;count++){
            int[] parts=ProgressionRewards.RaceCoins(180,100,1000,count);
            Check(parts[0]==180*(count+1)&&parts[1]==100*(count+1),"Race/bonus display components match multiplier");
            Check(ProgressionRewards.GhostBonus(100,count)==parts[1],"Versus nominal bonus matches the later credited ghost component");
            Check(parts[0]+parts[1]==ProgressionRewards.Coins(280,1000,count,1m,1m,null),"Component sum equals credited reward");
        }
        var originalBalance=ProgressionBalance.Current;
        try {
            ProgressionBalance.Current=new ProgressionBalance{CoinRate=.375m,PrestigeGrowth=.5m,MultiplierCap=2m,CreatorRate=99m};
            for(uint count=0;count<10;count++){
                int quote=ProgressionRewards.GhostBonus(101,count);
                Check(quote==ProgressionRewards.RaceCoins(180,101,1000,count)[1],"Versus uses coin rate, growth, cap and floor rather than creator rate");
            }
            ProgressionBalance.Current.CoinRate=0;
            Check(ProgressionRewards.GhostBonus(100,1)==0,"Disabled rewards display zero");
        }finally{ProgressionBalance.Current=originalBalance;}
        int[] capped=ProgressionRewards.RaceCoins(100,100,Int32.MaxValue-150,1);
        Check(capped[0]==150&&capped[1]==0,"Race and bonus share destination headroom");
        int[] empty=ProgressionRewards.RaceCoins(0,100,Int32.MaxValue-5,2);
        Check(empty[0]==0&&empty[1]==5,"Bonus uses headroom when race reward is zero");
        bool refused=false;try{ProgressionRewards.Coins(1,0,0,-1m,1m,null);}catch(ArgumentOutOfRangeException){refused=true;}Check(refused,"Invalid policy refused");
        Console.WriteLine("PASS: prestige coin awards and versus bonus quote, untouched debits, cap/rounding/overflow, native cycle-XP bound and permanent-level pack discounts");
    }
}
