using System;
using System.IO;
using ReRiff;
class ModCacheSmoke {
    static void Check(bool pass,string label){if(!pass)throw new Exception(label);}
    static void Asset(string root,string mod,string name,string text){string path=Path.Combine(root,mod,"assets",name);Directory.CreateDirectory(Path.GetDirectoryName(path));File.WriteAllText(path,text);}
    static void Main(string[] args){
        string root=Path.GetFullPath(args[0]);Directory.CreateDirectory(root);
        Asset(root,"a-first","graphics/vehicles/decals/alf_01/batchtextures.brres","first mod");
        Asset(root,"z-last","graphics/vehicles/decals/alf_01/batchtextures.brres","second mod");Asset(root,"z-last","second.txt","second only");
        Asset(root,".disabled/ignored","hidden.txt","disabled");
        var initial=ModArchiveCache.Prepare(root,1);Check(initial.Count==2,"two mods");
        Check(Path.GetDirectoryName(initial[0].Stem)==Path.Combine(root,"a-first"),"individual cache is in mod");
        Check(System.Text.Encoding.UTF8.GetString(ModArchiveCache.Resource(initial,"graphics/vehicles/decals/alf_01/batchtextures.brres"))=="second mod","resource uses final mounted priority");
        Check(ModArchiveCache.Resource(initial,"hidden.txt")==null,"disabled resource not loaded");
        DateTime stamp=File.GetLastWriteTimeUtc(initial[0].Stem+".zip");var warm=ModArchiveCache.Prepare(root,1);
        Check(initial[0].Stem==warm[0].Stem&&stamp==File.GetLastWriteTimeUtc(warm[0].Stem+".zip"),"reuse without rewrite");
        // Only slot 31 available. Merged output MUST be directly under mods root.
        var merged=ModArchiveCache.Prepare(root,0x7fffffffu);Check(merged.Count==1&&merged[0].Id==31,"reserve numbered slots");
        Check(Path.GetDirectoryName(merged[0].Stem)==root,"root-level merged cache");
        Check(System.Text.Encoding.UTF8.GetString(ModArchiveCache.Resource(merged,"graphics/vehicles/decals/alf_01/batchtextures.brres"))=="second mod","merged resource priority");
        string changed=Path.Combine(root,"z-last","assets","second.txt");DateTime savedTime=File.GetLastWriteTimeUtc(changed);
        File.WriteAllText(changed,"second best");File.SetLastWriteTimeUtc(changed,savedTime);
        Check(ModArchiveCache.Prepare(root,1)[1].Stem!=initial[1].Stem,"same-size same-time edit detected");
        File.WriteAllText(changed,"second only");
        string old=merged[0].Stem;Check(ModArchiveCache.Prepare(root,0x7fffffffu)[0].Stem==old,"merge reused");
        Asset(root,"a-first","unique.txt","changed");Check(ModArchiveCache.Prepare(root,0x7fffffffu)[0].Stem!=old,"merge invalidated on add");
        File.Delete(Path.Combine(root,"a-first","assets","unique.txt"));Check(ModArchiveCache.Prepare(root,0x7fffffffu)[0].Stem==old,"source restoration reuses cache");
        // Convert one mod to legacy pair, then exercise mixed merge.
        Directory.Delete(Path.Combine(root,"a-first","assets"),true);
        File.Copy(initial[0].Stem+".dat",Path.Combine(root,"a-first","vfs.dat"));File.Copy(initial[0].Stem+".zip",Path.Combine(root,"a-first","vfs.zip"));
        merged=ModArchiveCache.Prepare(root,0x7fffffffu);Check(merged.Count==1,"mixed loose/archive merge");
        File.WriteAllText(Path.Combine(root,"plan.txt"),merged[0].Stem);
        bool rejected=false;try{ModArchiveCache.Prepare(root,uint.MaxValue);}catch(IOException){rejected=true;}Check(rejected,"no free slots rejected");
                Asset(root,"bad","Upper.txt","invalid");rejected=false;try{ModArchiveCache.Prepare(root,1);}catch(IOException){rejected=true;}Check(rejected,"uppercase rejected");Directory.Delete(Path.Combine(root,"bad"),true);
        // More than 31 mods: earlier mods retain individual priority, tail merges.
        for(int i=0;i<33;i++)Asset(root,"m"+i.ToString("D2"),"common.txt",i.ToString());
        var many=ModArchiveCache.Prepare(root,1);Check(many.Count==31,"over 31 merges");Check(Path.GetDirectoryName(many[30].Stem)==root,"overflow merge stays at root");
        // A corrupt cache must not be reused or overwritten.
        var repaired=ModArchiveCache.Prepare(root,1);
        var loose=many[1];File.AppendAllText(loose.Stem+".zip","bad");repaired=ModArchiveCache.Prepare(root,1);Check(repaired[1].Stem!=loose.Stem,"corrupt cache replaced safely");
        Check(ModArchiveCache.Prepare(root,1)[1].Stem==repaired[1].Stem,"repaired cache reused");
        string last=Path.Combine(root,"z-last"),disabled=Path.Combine(root,".disabled","z-last");Directory.Move(last,disabled);
        var off=ModArchiveCache.Prepare(root,0x7fffffffu);Check(off[0].Stem!=merged[0].Stem,"disabling invalidates merge");Directory.Move(disabled,last);
        Console.WriteLine("PASS cache reuse, root merge, invalidation, mixed archives, slot budget, disabled mods, invalid paths, corruption");
    }
}
