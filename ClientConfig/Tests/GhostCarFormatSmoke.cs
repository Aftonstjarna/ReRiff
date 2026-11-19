using System;
using System.IO;
using System.Linq;
using System.Text;
using ReRiff;
class GhostCarFormatSmoke {
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static void Text(BinaryWriter writer,string text){byte[] b=Encoding.ASCII.GetBytes(text);writer.Write(b.Length);writer.Write(b);}
    static byte[] Sample(){using(var s=new MemoryStream()){using(var w=new BinaryWriter(s)){w.Write(18);w.Write(1);w.Write(120f);Text(w,"123");w.Write(100f);Text(w,"abc");Text(w,"dod");w.Write(1);w.Write(80);w.Write(new byte[80]);return s.ToArray();}}}
    static int Main(string[] args){
        byte[] mod=Enumerable.Range(0,32).Select(i=>(byte)i).ToArray(),raw=Sample(),before=(byte[])raw.Clone();
        var custom=GhostCarTestFormat.ReadHeader(raw);byte[] wire=GhostCarTestFormat.Publish(raw,mod);
        Check(raw.SequenceEqual(before),"Source recording changed");
        var stock=GhostCarTestFormat.ReadHeader(wire);Check(stock.Car=="alf"&&stock.Pack==1,"Stock header not valid");
        var old=CachedGhost.Read(new MemoryStream(wire),"abc-123.ghost","123");Check(old!=null&&old.Car=="alf"&&old.Pack==1,"Existing legacy header parser rejects extension");
        Check(wire.Skip(custom.PackOffset+4).Take(raw.Length-custom.PackOffset-4).SequenceEqual(raw.Skip(custom.PackOffset+4)),"Stream headers/payload changed");
        Check(ReferenceEquals(wire,GhostCarTestFormat.Publish(wire,mod)),"Publication not idempotent");
        Check(GhostCarTestFormat.Restore(wire,null)==null,"Absent mod restored");
        byte[] other=(byte[])mod.Clone();other[0]++;Check(GhostCarTestFormat.Restore(wire,other)==null,"Different mod restored");
        Check(GhostCarTestFormat.Restore(wire,mod)==null,"Matching mod restored despite fallback-only mode");
        Check(wire.Length==raw.Length,"Custom identity trailer added");
        for(int n=0;n<raw.Length;n++){try{GhostCarTestFormat.ReadHeader(raw.Take(n).ToArray());throw new Exception("Truncated ghost accepted");}catch(EndOfStreamException){}catch(InvalidDataException){}}
        if(args.Length>=1){
            byte[] real=File.ReadAllBytes(args[0]);var source=GhostCarTestFormat.ReadHeader(real);
            Check(source.Car.Length==3,"Real fixture car identifier length");
            Buffer.BlockCopy(Encoding.ASCII.GetBytes("dod"),0,real,source.CarOffset,3);Buffer.BlockCopy(BitConverter.GetBytes(1),0,real,source.PackOffset,4);
            byte[] shared=GhostCarTestFormat.Publish(real,mod);
            Check(GhostCarTestFormat.Restore(shared,mod)==null,"Model sharing still enabled");
            Buffer.BlockCopy(Encoding.ASCII.GetBytes("dod"),0,shared,source.CarOffset,3);
            Check(shared.SequenceEqual(real),"Real recording payload changed beyond fallback car");
            Console.WriteLine("PASS real cached ghost fallback, same file size and unchanged replay payload");
        }
        if(args.Length>1){
            byte[] expected=GhostCarTestFormat.Publish(File.ReadAllBytes(args[0]),null);
            foreach(string path in args.Skip(1)){
                byte[] bytes=File.ReadAllBytes(path),canonical=GhostCarTestFormat.Publish(bytes,null);
                Check(canonical.SequenceEqual(expected),"Live test trailer removal changed replay payload: "+path);
                Check(ReferenceEquals(canonical,GhostCarTestFormat.Publish(canonical,null)),"Cleaned live file is not idempotent");
            }
            Console.WriteLine("PASS live test stock/custom upload and nested-trailer fixtures normalize to original replay bytes");
        }
        Console.WriteLine("PASS stock-only header, existing reader compatibility, unchanged source/payload, no model restoration, truncation and idempotency");return 0;
    }
}
