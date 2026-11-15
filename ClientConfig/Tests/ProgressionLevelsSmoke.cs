using System;
using System.Collections.Generic;
using System.IO;
using ReRiff;

class ProgressionLevelsSmoke {
    static int checks;
    static void Check(bool value,string message){checks++;if(!value)throw new Exception(message);}
    static void Main(){
        var b=new ProgressionBalance();
        Check(b.ExtendedMainLevel(1000000000,40,1000000000,250000000)==40,"Original maximum boundary");
        Check(b.ExtendedMainLevel(1125000000,40,1000000000,250000000)==40.5m,"Halfway to level 41");
        Check(b.ExtendedMainLevel(1250000000,40,1000000000,250000000)==41,"Default final native interval repeated");
        Check(b.ExtendedMainLevel(1500000000,40,1000000000,250000000)==42,"Second extended default level");
        Check(b.ExtendedMainLevel(UInt64.MaxValue,40,1000000000,250000000)>40,"Full saved XP range accepted");
        b.MainLevelCap=41;
        Check(b.ExtendedMainLevel(UInt64.MaxValue,40,1000000000,250000000)==41,"Configured cap");
        b.MainLevelCap=40;Check(b.ExtendedMainLevel(UInt64.MaxValue,40,1000000000,250000000)==40,"Original cap selectable");
        b.MainLevelCap=0;b.MainLevelXpStep=100;b.MainLevelXpGrowth=2;
        Check(b.ExtendedMainLevel(1150,40,1000,250)==41.25m,"Geometric costs begin with one base step then grow");
        Check(b.ExtendedMainLevel(1300,40,1000,250)==42,"Geometric cumulative threshold");
        b.MainLevelXpGrowth=1000000;b.MainLevelXpStep=UInt64.MaxValue;
        Check(b.ExtendedMainLevel(UInt64.MaxValue,40,1000,250)<41,"Extreme cost and growth do not overflow");
        foreach(ulong step in new ulong[]{1,250000000,UInt64.MaxValue})foreach(decimal growth in new[]{1m,1.000001m,1.1m,2m,1000000m})foreach(int cap in new[]{0,40,41,100,Int32.MaxValue}){
            b=new ProgressionBalance{MainLevelXpStep=step,MainLevelXpGrowth=growth,MainLevelCap=cap};decimal previous=0;
            foreach(ulong xp in new ulong[]{1000000000,1000000001,2000000000,UInt64.MaxValue}){
                decimal level=b.ExtendedMainLevel(xp,40,1000000000,250000000);
                Check(level>=40&&level>=previous&&level<=(cap==0?Int32.MaxValue:cap),"Monotonic bounded level across extreme XP and policies");previous=level;
            }
        }
        var random=new Random(1541);
        for(int sample=0;sample<100;sample++){
            b=new ProgressionBalance{MainLevelXpStep=(ulong)random.Next(1,100),MainLevelXpGrowth=random.Next(1,5),MainLevelCap=random.Next(41,61)};
            ulong spent=0,cost=b.MainLevelXpStep;
            for(int n=0;n<12;n++){
                decimal expected=Math.Min(b.MainLevelCap,40m+n);
                Check(b.ExtendedMainLevel(1000+spent,40,1000,250)==expected,"Independent cumulative threshold comparison");
                if(n>0&&40+n<=b.MainLevelCap)Check((int)b.ExtendedMainLevel(999+spent,40,1000,250)==39+n,"No early level before integer XP threshold");
                ulong half=cost/2;
                decimal actual=b.ExtendedMainLevel(1000+spent+half,40,1000,250);
                decimal reference=Math.Min(b.MainLevelCap,40m+n+half/(decimal)cost);
                Check(Math.Abs(actual-reference)<.0000000001m,"Independent within-level fraction comparison");
                spent+=cost;cost=checked(cost*(ulong)b.MainLevelXpGrowth);
            }
        }
        b=new ProgressionBalance();
        Check(b.MainLevelFill(20)==.5f&&b.MainLevelFill(41.5m)==.5f,"Native pre-40 fill and uncapped post-40 next-level fill");
        b.MainLevelCap=100;Check(b.MainLevelFill(50)==.5f&&b.MainLevelFill(100)==1,"Configured-cap native fill ratio");
        var defaults=new ProgressionBalance();
        var parsed=ProgressionBalance.Parse(new Dictionary<string,string>{{"main_level_cap","100"},{"main_level_xp_step","250000000"},{"main_level_xp_growth","1.1"}});
        Check(parsed.MainLevelCap==100&&parsed.MainLevelXpStep==250000000&&parsed.MainLevelXpGrowth==1.1m,"INI curve settings parsed");
        Check(parsed.Revision!=defaults.Revision,"Curve settings included in balance revision");
        foreach(var pair in new[]{Tuple.Create("main_level_cap","39"),Tuple.Create("main_level_cap","40.5"),Tuple.Create("main_level_xp_step","-1"),Tuple.Create("main_level_xp_step","0.5"),Tuple.Create("main_level_xp_growth","0.99"),Tuple.Create("main_level_xp_growth","NaN")}){
            bool refused=false;try{ProgressionBalance.Parse(new Dictionary<string,string>{{pair.Item1,pair.Item2}});}catch(InvalidDataException){refused=true;}
            Check(refused,"Invalid curve refused: "+pair.Item1);
        }
        var custom=new Dictionary<string,string>{{"discount_mode","interval"},{"discount_start_level","10"},{"discount_start","0.20"},{"discount_interval","5"},{"discount_increase","0.05"},{"discount_maximum","0.50"}};
        b=ProgressionBalance.Parse(custom);
        foreach(int level in new[]{0,9,10,14,15,19,20,24,25,29,30,34,35,39,40,41,Int32.MaxValue}){
            decimal expected=level<10?0:level<15?.20m:level<20?.25m:level<25?.30m:level<30?.35m:level<35?.40m:level<40?.45m:.50m;
            Check(b.Discount(level)==expected,"Requested interval discount at level "+level);
        }
        var oldPolicy=ProgressionBalance.Current;
        try {
            ProgressionBalance.Current=b;
            Check(ProgressionRewards.PaintPrice(1000,10,1)==800&&ProgressionRewards.PaintPrice(1000,15,1)==750
                &&ProgressionRewards.PaintPrice(1000,20,1)==700&&ProgressionRewards.PaintPrice(1000,30,1)==600
                &&ProgressionRewards.PaintPrice(1000,40,1)==500,"Native-price policy uses interval discounts");
            Check(ProgressionRewards.PaintPrice(3,15,1)==2&&ProgressionRewards.PaintPrice(1,40,1)==1
                &&ProgressionRewards.PaintPrice(0,40,1)==0,"Rounding and paid/free pack boundaries preserved");
        }finally{ProgressionBalance.Current=oldPolicy;}
        Check(b.Revision!=defaults.Revision,"Interval settings included in balance revision");
        string revision=b.Revision;custom["discount_start"]="0.200";custom["discount_increase"]="0.050";
        Check(ProgressionBalance.Parse(custom).Revision==revision,"Equivalent fractions preserve revision");
        custom["discount_level_10"]="invalid overridden value";
        Check(ProgressionBalance.Parse(custom).Discount(15)==.25m,"Interval mode overrides legacy milestones");
        foreach(var pair in new[]{Tuple.Create("discount_mode","invalid"),Tuple.Create("discount_start_level","-1"),Tuple.Create("discount_start_level","10.5"),Tuple.Create("discount_start","0.51"),Tuple.Create("discount_increase","-0.1"),Tuple.Create("discount_increase","NaN"),Tuple.Create("discount_interval","0"),Tuple.Create("discount_interval","1.5")}){
            var invalid=new Dictionary<string,string>(custom);invalid[pair.Item1]=pair.Item2;
            bool refused=false;try{ProgressionBalance.Parse(invalid);}catch(InvalidDataException){refused=true;}
            Check(refused,"Invalid interval policy refused: "+pair.Item1);
        }
        b=ProgressionBalance.Parse(new Dictionary<string,string>{{"discount_mode"," "}});
        Check(b.Revision==defaults.Revision,"Legacy balance revision preserved");
        foreach(int level in new[]{0,9,10,19,20,29,30,39,40,50,100})Check(b.Discount(level)==defaults.Discount(level),"Legacy discount compatibility");
        custom.Remove("discount_level_10");custom["discount_start_level"]="0";custom["discount_interval"]="3";
        b=ProgressionBalance.Parse(custom);Check(b.Discount(0)==.20m&&b.Discount(2)==.20m&&b.Discount(3)==.25m,"Starting at zero with arbitrary interval");
        custom["discount_start_level"]="17";custom["discount_increase"]="0";
        b=ProgressionBalance.Parse(custom);Check(b.Discount(16)==0&&b.Discount(Int32.MaxValue)==.20m,"Arbitrary start and zero growth");
        Console.WriteLine("PASS: {0} extended-level and interval-discount, price, cap, boundary, overflow, legacy and INI-policy checks",checks);
    }
}
