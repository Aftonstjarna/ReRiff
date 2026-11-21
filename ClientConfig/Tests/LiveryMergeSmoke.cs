using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using ReRiff;
class LiveryMergeSmoke {
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static string Hex(byte[] data){using(var hash=SHA256.Create())return BitConverter.ToString(hash.ComputeHash(data)).Replace("-","").ToLowerInvariant();}
    static NativeCarConfig.Node Text(string name,string value){return NativeCarConfig.Text(name,value);}
    static NativeCarConfig.Node Number(string name,float value){return NativeCarConfig.Number(name,value);}
    static NativeCarConfig.Node Table(string name,params NativeCarConfig.Node[] children){
        var node=new NativeCarConfig.Node{Name=name,Type=2};node.Children.AddRange(children);return node;
    }
    static NativeCarConfig.Node Pack(string name,float price,float livery){
        return Table("",Text("packName",name),Number("packPrice",price),Number("livery",livery));
    }
    static byte[] Config(int changedPack=-1,string changedName=null,float changedLivery=0){
        var packs=new List<NativeCarConfig.Node>();
        string[] names={"Arctic","Miami","Shadow"};float[] livery={-1,1,2};float[] price={0,1000,1000};
        for(int i=0;i<3;i++)packs.Add(i==changedPack?Pack(changedName,price[i],changedLivery):Pack(names[i],price[i],livery[i]));
        return NativeCarConfig.Write(new List<NativeCarConfig.Node>{Table("Paints",Table("alf",packs.ToArray()))});
    }
    static byte[] Bank(params string[] text){
        using(var memory=new MemoryStream())using(var writer=new BinaryWriter(memory)){writer.Write(3u);writer.Write(2u);foreach(string item in text){byte[] data=Encoding.Unicode.GetBytes(item);writer.Write((uint)data.Length/2);writer.Write(data);}return memory.ToArray();}
    }
    static List<string> Strings(byte[] data){
        var result=new List<string>();int at=8;while(at<data.Length){uint count=BitConverter.ToUInt32(data,at);at+=4;result.Add(Encoding.Unicode.GetString(data,at,(int)count*2));at+=(int)count*2;}return result;
    }
    static void WritePair(string stem,List<KeyValuePair<string,byte[]>> files){
        var entries=new List<ModArchiveCache.Entry>();
        foreach(var file in files)entries.Add(new ModArchiveCache.Entry{Name=file.Key,Data=file.Value,Size=(uint)file.Value.Length,Packed=(uint)file.Value.Length});
        var method=typeof(ModArchiveCache).GetMethod("WritePair",BindingFlags.NonPublic|BindingFlags.Static);
        method.Invoke(null,new object[]{stem,entries});
    }
    static string Add(string root,string name,string car,int pack,byte[] config,string decal,byte[] decalBytes,byte[] strings=null){
        string folder=Path.Combine(root,name),assets=Path.Combine(folder,"assets");Directory.CreateDirectory(assets);
        var resources=new List<string>();
        Action<string,byte[]> add=delegate(string path,byte[] data){
            string target=Path.Combine(assets,path);Directory.CreateDirectory(Path.GetDirectoryName(target));File.WriteAllBytes(target,data);
            resources.Add("{\"path\":\""+path+"\",\"sha256\":\""+Hex(data)+"\"}");
        };
        add("scripts/config/paint_pc.cfg",config);add("graphics/vehicles/decals/"+decal+"/batchtextures.brres",decalBytes);
        if(strings!=null)add("strings/common/us.str",strings);
        string metadata="{\"tool\":\"reriff-livery-maker-v1\",\"selected_car\":\""+car+"\",\"selected_pack_index\":"+pack+",\"resources\":["+String.Join(",",resources.ToArray())+"]}";
        File.WriteAllText(Path.Combine(folder,"livery-maker.json"),metadata);return folder;
    }
    static string AddTexture(string root,string name,string car,int pack,string decal,byte[] decalBytes){
        string folder=Path.Combine(root,name),assets=Path.Combine(folder,"assets");Directory.CreateDirectory(assets);
        string target=Path.Combine(assets,"graphics/vehicles/decals/"+decal+"/batchtextures.brres");Directory.CreateDirectory(Path.GetDirectoryName(target));File.WriteAllBytes(target,decalBytes);
        string metadata="{\"tool\":\"reriff-livery-maker-v1\",\"selected_car\":\""+car+"\",\"selected_pack_index\":"+pack+",\"resources\":[{\"path\":\"graphics/vehicles/decals/"+decal+"/batchtextures.brres\",\"sha256\":\""+Hex(decalBytes)+"\"}]}";
        File.WriteAllText(Path.Combine(folder,"livery-maker.json"),metadata);return folder;
    }
    static string Stem(List<ModArchiveCache.Plan> plans){return plans.Single(p=>Path.GetFileName(p.Stem).StartsWith(".reriff-liveries-",StringComparison.Ordinal)).Stem;}
    static byte[] Read(string stem,string name){
        using(var zip=ZipFile.OpenRead(stem+".zip")){var entry=zip.Entries.Single(e=>e.FullName==name);using(var input=entry.Open())using(var output=new MemoryStream()){input.CopyTo(output);return output.ToArray();}}
    }
    static void Main(string[] args){
        string fixture=Path.GetFullPath(args[0]);string root=Path.Combine(fixture,"mods");
        Check(!Directory.Exists(fixture),"fixture must be a new path");Directory.CreateDirectory(root);
        byte[] stockConfig=Config(),stockBank=Bank("Pulse","Arctic","Miami","car_alf","car_mot");
        WritePair(Path.Combine(fixture,"vfs"),new List<KeyValuePair<string,byte[]>>{
            new KeyValuePair<string,byte[]>("scripts/config/paint_pc.cfg",stockConfig),
            new KeyValuePair<string,byte[]>("strings/common/us.str",stockBank)
        });
        // A non-livery mod changes an unrelated string, as the composed car plan
        // does for custom names. Livery deltas must not revert it.
        string custom=Path.Combine(root,"zz-custom-strings","assets","strings","common");Directory.CreateDirectory(custom);
        byte[] customBank=Bank("Baby","Arctic","Miami","car_alf","car_mot");File.WriteAllBytes(Path.Combine(custom,"us.str"),customBank);
        Add(root,"livery-a","alf",0,Config(0,"Arctic",4),"alf_04",Encoding.ASCII.GetBytes("decal four"));
        AddTexture(root,"livery-texture","pl1",0,"pl1_07",Encoding.ASCII.GetBytes("texture only"));
        string b=Add(root,"livery-b","alf",1,Config(1,"Kazuho",3),"alf_03",Encoding.ASCII.GetBytes("decal three"),Bank("Pulse","Kazuho","Miami","car_alf","car_mot"));
        var plans=ModArchiveCache.Prepare(root,1);string stem=Stem(plans);
        Check(plans.Count==2&&Path.GetDirectoryName(stem)==root,"one root livery volume beside the loose string mod");
        var tree=NativeCarConfig.Read(Read(stem,"scripts/config/paint_pc.cfg"));
        var packs=NativeCarConfig.Find(NativeCarConfig.Find(tree,"Paints").Children,"alf").Children;
        Check(NativeCarConfig.Find(packs[0].Children,"livery").Number==4,"first approved pack applied");
        Check(NativeCarConfig.Find(packs[1].Children,"livery").Number==3,"second approved pack applied");
        Check(NativeCarConfig.Find(packs[2].Children,"livery").Number==2,"untouched pack preserved");
        Check(Read(stem,"graphics/vehicles/decals/alf_04/batchtextures.brres").SequenceEqual(Encoding.ASCII.GetBytes("decal four")),"first decal copied");
        Check(Read(stem,"graphics/vehicles/decals/alf_03/batchtextures.brres").SequenceEqual(Encoding.ASCII.GetBytes("decal three")),"second decal copied");
        Check(Read(stem,"graphics/vehicles/decals/pl1_07/batchtextures.brres").SequenceEqual(Encoding.ASCII.GetBytes("texture only")),"texture-only package copied without a paint table");
        var strings=Strings(Read(stem,"strings/common/us.str"));
        Check(strings[0]=="Baby"&&strings[1]=="Kazuho","string delta applied over the effective base");
        Check(Read(stem,"scripts/config/paint_pc.luac").Length>0&&Read(stem,"scripts/config/paint_pc.luac64").Length>0,"matching Lua chunks emitted");
        DateTime stamp=File.GetLastWriteTimeUtc(stem+".zip");Check(Stem(ModArchiveCache.Prepare(root,1))==stem&&stamp==File.GetLastWriteTimeUtc(stem+".zip"),"livery cache reused");
        string duplicate=Add(root,"livery-dup","alf",0,Config(0,"Duplicate",4),"alf_05",Encoding.ASCII.GetBytes("duplicate"));
        bool refused=false;try{ModArchiveCache.Prepare(root,1);}catch(IOException){refused=true;}Check(refused,"duplicate car/paint refused");Directory.Delete(duplicate,true);
        string multi=Add(root,"livery-multi","alf",0,Config(0,"Multi",4),"alf_06",Encoding.ASCII.GetBytes("multi"));
        string cfgPath=Path.Combine(multi,"assets","scripts","config","paint_pc.cfg");
        byte[] treeBytes=File.ReadAllBytes(cfgPath);var multiTree=NativeCarConfig.Read(treeBytes);var multiPacks=NativeCarConfig.Find(NativeCarConfig.Find(multiTree,"Paints").Children,"alf").Children;
        NativeCarConfig.Find(multiPacks[2].Children,"livery").Number=5;byte[] changed=NativeCarConfig.Write(multiTree);File.WriteAllBytes(cfgPath,changed);
        string metadata=File.ReadAllText(Path.Combine(multi,"livery-maker.json")).Replace(Hex(treeBytes),Hex(changed));File.WriteAllText(Path.Combine(multi,"livery-maker.json"),metadata);
        refused=false;try{ModArchiveCache.Prepare(root,1);}catch(IOException){refused=true;}Check(refused,"multi-paint package refused");Directory.Delete(multi,true);
        File.WriteAllBytes(Path.Combine(b,"assets","graphics","vehicles","decals","alf_03","batchtextures.brres"),Encoding.ASCII.GetBytes("edited decal"));
        string meta=File.ReadAllText(Path.Combine(b,"livery-maker.json"));meta=meta.Replace(Hex(Encoding.ASCII.GetBytes("decal three")),Hex(Encoding.ASCII.GetBytes("edited decal")));File.WriteAllText(Path.Combine(b,"livery-maker.json"),meta);
        Check(Stem(ModArchiveCache.Prepare(root,1))!=stem,"editing a decal invalidates the livery volume");
        Console.WriteLine("PASS livery merge: two packs, strings, decals, Lua, reuse, duplicate/multi-paint refusal and invalidation");
    }
}
