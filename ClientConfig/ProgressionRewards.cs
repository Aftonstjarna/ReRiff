using System;

namespace ReRiff {
    // Award/price transforms for verified native call boundaries. These are
    // Paint prices use the shared native getter; awards remain observation-only.
    // Never infer rewards from net coins.
    internal static class ProgressionRewards {
        private static void Rate(decimal value){if(value<0||value>1000000m)throw new ArgumentOutOfRangeException("rate");}
        internal static int Coins(int nativeDelta,int current,uint prestige,decimal baseRate,decimal growth,decimal? cap){
            if(current<0)throw new ArgumentOutOfRangeException("current");
            Rate(baseRate);Rate(growth);if(cap.HasValue&&(cap.Value<1||cap.Value>1000000m))throw new ArgumentOutOfRangeException("cap");
            // Purchases and zero changes keep their original signed amount.
            if(nativeDelta<=0)return nativeDelta;
            decimal multiplier=1m+prestige*growth;
            if(cap.HasValue)multiplier=Math.Min(multiplier,cap.Value);
            decimal rate=baseRate*multiplier;int room=Int32.MaxValue-current;
            // Compare before multiplication: extreme policy/count products can
            // exceed Decimal even though the native destination is only int32.
            if(rate>=room/(decimal)nativeDelta)return room;
            return (int)Decimal.Floor(nativeDelta*rate);
        }
        // Allocate the bounded award across the original Results base/bonus
        // fields so their sum is exactly the amount passed to native incTokens.
        internal static int[] RaceCoins(int race,int bonus,int current,uint prestige){
            if(race<0||bonus<0)throw new ArgumentOutOfRangeException("reward");
            var b=ProgressionBalance.Current;
            int first=Coins(race,current,prestige,b.CoinRate,b.PrestigeGrowth,b.MultiplierCap);
            int second=Coins(bonus,checked(current+first),prestige,b.CoinRate,b.PrestigeGrowth,b.MultiplierCap);
            return new[]{first,second};
        }
        // Versus advertises the ghost bonus before ResultsTokens allocates the
        // total bank capacity between race and bonus. Keep that native nominal
        // quote, using the same rate/cap/rounding as the subsequent payment.
        internal static int GhostBonus(int amount,uint prestige){
            var b=ProgressionBalance.Current;
            return Coins(amount,0,prestige,b.CoinRate,b.PrestigeGrowth,b.MultiplierCap);
        }
        internal static int CycleXp(int nativeReward,int current,decimal rate){
            Rate(rate);if(nativeReward<0||current<0||current>2000000000)throw new ArgumentOutOfRangeException("xp");
            int room=2000000000-current;
            if(nativeReward==0)return 0;
            return rate>=room/(decimal)nativeReward?room:(int)Decimal.Floor(nativeReward*rate);
        }
        internal static decimal DefaultDiscount(int mainLevel){
            if(mainLevel<0)throw new ArgumentOutOfRangeException("mainLevel");
            if(mainLevel<10)return 0;if(mainLevel<20)return .10m;if(mainLevel<30)return .20m;
            decimal gap=.05m;
            for(int i=0,steps=Math.Min((mainLevel-30)/10,96);i<steps;i++)gap*=.5m;
            return .30m-gap;
        }
        internal static int PaintPrice(int nativePrice,int mainLevel,decimal scale){
            Rate(scale);if(nativePrice<0)throw new ArgumentOutOfRangeException("price");
            if(nativePrice==0)return 0;
            decimal adjusted=Decimal.Floor(nativePrice*scale*(1m-ProgressionBalance.Current.Discount(mainLevel)));
            return (int)Math.Min(Int32.MaxValue,Math.Max(1m,adjusted));
        }
    }
}
