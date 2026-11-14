using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace ReRiff {
    internal sealed class ProgressionBalance {
        internal decimal CoinRate=1m,CreatorRate=1m,PrestigeGrowth=1m,PaintScale=1m;
        internal decimal? MultiplierCap;
        internal decimal Discount10=.10m,Discount20=.20m,Discount30=.25m,DiscountMaximum=.30m,GapFactor=.5m;
        internal int DiscountInterval=10;
        internal int MainLevelCap;
        internal ulong MainLevelXpStep;
        internal decimal MainLevelXpGrowth=1m;
        internal static ProgressionBalance Current=new ProgressionBalance();
        internal const string Defaults="[balance]\n; Applied to isolated Singleplayer only; restart to apply.\ncoin_reward_rate = 1\ncreator_reward_rate = 1\nprestige_multiplier_growth = 1\n; 0 means uncapped; otherwise at least 1.\nprestige_multiplier_cap = 0\npaint_price_scale = 1\n; Discounts are fractions: 0.10 means 10 percent.\ndiscount_level_10 = 0.10\ndiscount_level_20 = 0.20\ndiscount_level_30 = 0.25\ndiscount_maximum = 0.30\ndiscount_interval = 10\ndiscount_remaining_gap_factor = 0.5\n; Permanent main levels only; 0 cap is uncapped, 0 step uses native 39-to-40 XP.\nmain_level_cap = 0\nmain_level_xp_step = 0\n; Each later level costs step * growth^(level - 40). 1 keeps a constant cost.\nmain_level_xp_growth = 1\n";
        private static decimal Number(Dictionary<string,string> values,string key,decimal fallback,decimal min,decimal max) {
            string text;decimal value;if(!values.TryGetValue(key,out text))return fallback;
            if(!Decimal.TryParse(text,NumberStyles.AllowDecimalPoint|NumberStyles.AllowLeadingSign,CultureInfo.InvariantCulture,out value)||value<min||value>max)throw new InvalidDataException("Invalid Singleplayer balance value: "+key);
            return value;
        }
        internal static ProgressionBalance Parse(Dictionary<string,string> values) {
            var b=new ProgressionBalance();b.CoinRate=Number(values,"coin_reward_rate",1,0,1000000);b.CreatorRate=Number(values,"creator_reward_rate",1,0,1000000);
            b.PrestigeGrowth=Number(values,"prestige_multiplier_growth",1,0,1000000);decimal cap=Number(values,"prestige_multiplier_cap",0,0,1000000);
            if(cap>0&&cap<1)throw new InvalidDataException("Multiplier cap must be 0 or at least 1");b.MultiplierCap=cap==0?(decimal?)null:cap;
            b.PaintScale=Number(values,"paint_price_scale",1,0,1000000);
            b.DiscountMaximum=Number(values,"discount_maximum",.30m,0,.99m);
            b.Discount10=Number(values,"discount_level_10",.10m,0,b.DiscountMaximum);b.Discount20=Number(values,"discount_level_20",.20m,b.Discount10,b.DiscountMaximum);b.Discount30=Number(values,"discount_level_30",.25m,b.Discount20,b.DiscountMaximum);
            decimal interval=Number(values,"discount_interval",10,1,1000000);if(interval!=Decimal.Truncate(interval))throw new InvalidDataException("Discount interval must be a whole number");b.DiscountInterval=(int)interval;
            b.GapFactor=Number(values,"discount_remaining_gap_factor",.5m,0,1);
            decimal levelCap=Number(values,"main_level_cap",0,0,Int32.MaxValue);
            if(levelCap!=Decimal.Truncate(levelCap)||(levelCap>0&&levelCap<40))throw new InvalidDataException("Main level cap must be 0 or a whole number of at least 40");
            b.MainLevelCap=(int)levelCap;
            decimal xpStep=Number(values,"main_level_xp_step",0,0,UInt64.MaxValue);
            if(xpStep!=Decimal.Truncate(xpStep))throw new InvalidDataException("Main level XP step must be a whole number");
            b.MainLevelXpStep=(ulong)xpStep;
            b.MainLevelXpGrowth=Number(values,"main_level_xp_growth",1,1,1000000);
            return b;
        }
        internal static void Load(string folder) {
            string file=Path.Combine(folder,"singleplayer.ini");
            if(!File.Exists(file))ConfigFiles.Atomic(file,"[progression]\nisolate_singleplayer = 0\ntrace_enabled = 0\n\n"+Defaults);
            var values=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);bool active=false;
            foreach(string raw in File.ReadAllLines(file)) {
                string line=raw.Trim();if(line.StartsWith("[")){active=line.Equals("[balance]",StringComparison.OrdinalIgnoreCase);continue;}
                if(!active||line.StartsWith(";")||line.StartsWith("#"))continue;int eq=line.IndexOf('=');if(eq>0)values[line.Substring(0,eq).Trim()]=line.Substring(eq+1).Trim();
            }
            if(values.Count==0)ConfigFiles.Atomic(file,File.ReadAllText(file)+Environment.NewLine+Defaults);
            Current=Parse(values);
        }
        internal decimal Discount(int level) {
            if(level<0)throw new ArgumentOutOfRangeException("level");if(level<10)return 0;if(level<20)return Discount10;if(level<30)return Discount20;
            decimal gap=DiscountMaximum-Discount30;for(int i=0,n=Math.Min((level-30)/DiscountInterval,96);i<n;i++)gap*=GapFactor;return DiscountMaximum-gap;
        }
        private static decimal BoundedMultiply(decimal left,decimal right,decimal limit){
            if(left==0||right==0)return 0;
            return left>=limit/right?limit:left*right;
        }
        private static decimal BoundedAdd(decimal left,decimal right,decimal limit){return left>=limit-right?limit:left+right;}
        // Sum geometric costs by doubling blocks, bounded before arithmetic.
        // This avoids a per-level loop and overflow for arbitrary saved XP.
        private static decimal CostThrough(int levels,decimal step,decimal growth,decimal limit){
            decimal total=0,power=1,block=step,factor=growth;
            while(levels>0){
                if((levels&1)!=0){total=BoundedAdd(total,BoundedMultiply(power,block,limit),limit);power=BoundedMultiply(power,factor,limit);}
                levels>>=1;if(levels==0)break;
                block=BoundedMultiply(block,BoundedAdd(1,Math.Min(factor,limit),limit),limit);
                factor=BoundedMultiply(factor,factor,limit);
            }
            return total;
        }
        internal decimal ExtendedMainLevel(ulong xp,int nativeMaximum,int nativeThreshold,int nativeStep){
            if(nativeMaximum!=40||nativeThreshold<0||nativeStep<=0||xp<(ulong)nativeThreshold)throw new ArgumentOutOfRangeException("nativeThreshold");
            int cap=MainLevelCap==0?Int32.MaxValue:MainLevelCap;
            if(cap<nativeMaximum)throw new InvalidDataException("Main level cap is below the native maximum");
            int available=cap-nativeMaximum;if(available==0)return cap;
            decimal budget=xp-(ulong)nativeThreshold,step=MainLevelXpStep==0?(ulong)nativeStep:MainLevelXpStep;
            if(MainLevelXpGrowth==1){
                decimal complete=Decimal.Floor(budget/step);
                if(complete>=available)return cap;
                return nativeMaximum+complete+(budget-complete*step)/step;
            }
            int low=0,high=available;
            while(low<high){
                int middle=low+(int)(((long)high-low+1)/2);
                if(CostThrough(middle,step,MainLevelXpGrowth,budget+1)<=budget)low=middle;else high=middle-1;
            }
            if(low==available)return cap;
            decimal spent=CostThrough(low,step,MainLevelXpGrowth,budget+1);
            decimal next=CostThrough(low+1,step,MainLevelXpGrowth,Decimal.MaxValue)-spent;
            return nativeMaximum+low+(budget-spent)/next;
        }
        internal float MainLevelFill(decimal level){
            // Preserve native whole-cap fill when a cap is configured. An
            // uncapped identity instead shows progress to its next level.
            if(MainLevelCap>0)return (float)Math.Min(1m,level/MainLevelCap);
            return (float)(level<40?level/40m:level-Decimal.Floor(level));
        }
        internal string Revision {
            get {string text=String.Join("|",Array.ConvertAll(new decimal[]{CoinRate,CreatorRate,PrestigeGrowth,MultiplierCap??0,PaintScale,Discount10,Discount20,Discount30,DiscountMaximum,GapFactor,DiscountInterval,MainLevelCap,MainLevelXpStep,MainLevelXpGrowth},v=>v.ToString(CultureInfo.InvariantCulture)));
                using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(text))).Replace("-","").ToLowerInvariant();}
        }
    }
}
