// Opt-in Pulse Test protocol experiment; not a general custom-car registry.
using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
namespace ReRiff {
    internal static class GhostCarTestFormat {
        internal const int TrailerSize=92,MaxSize=64*1024*1024;
        private static readonly byte[] Magic=Encoding.ASCII.GetBytes("RRGCAR1\0");
        internal sealed class Header {internal int CarOffset,PackOffset,Pack;internal string Car;}
        internal static bool Enabled(string folder){string value;return ConfigFiles.Read(folder,"diagnostics").TryGetValue("car_ghost_test",out value)&&(value=="1"||value.Equals("true",StringComparison.OrdinalIgnoreCase));}
        private static string Text(BinaryReader reader){
            int size=reader.ReadInt32();if(size<1||size>64)throw new InvalidDataException("Ghost identifier length");
            byte[] bytes=reader.ReadBytes(size);if(bytes.Length!=size||bytes.Any(b=>b<33||b>126))throw new InvalidDataException("Ghost identifier");return Encoding.ASCII.GetString(bytes);
        }
        internal static Header ReadHeader(byte[] bytes){
            if(bytes==null||bytes.Length>MaxSize)throw new InvalidDataException("Ghost size");
            using(var reader=new BinaryReader(new MemoryStream(bytes))){
                if(reader.ReadInt32()!=18)throw new InvalidDataException("Ghost version");
                int streams=reader.ReadInt32();float duration=reader.ReadSingle();
                if(streams<1||streams>32||float.IsNaN(duration)||float.IsInfinity(duration)||duration<=0)throw new InvalidDataException("Ghost streams/duration");
                Text(reader);float score=reader.ReadSingle();if(float.IsNaN(score)||float.IsInfinity(score)||score<0)throw new InvalidDataException("Ghost score");Text(reader);
                int carOffset=checked((int)reader.BaseStream.Position+4);string car=Text(reader);int packOffset=(int)reader.BaseStream.Position;int pack=reader.ReadInt32();
                int payload=reader.ReadInt32();if(pack<0||pack>8||payload<=0||payload>bytes.Length-reader.BaseStream.Position)throw new InvalidDataException("Ghost paint/payload");
                return new Header{Car=car,Pack=pack,CarOffset=carOffset,PackOffset=packOffset};
            }
        }
        internal static byte[] Digest(byte[] bytes,int length){using(var hash=SHA256.Create())return hash.ComputeHash(bytes,0,length);}
        // Fallback-only publication. The argument is retained for fixture ABI
        // compatibility; no mod identity is written or used to restore a car.
        internal static byte[] Publish(byte[] recorded,byte[] unusedModHash){
            Header header=ReadHeader(recorded);
            if(header.Car!="dod"&&header.Car!="alf")return recorded;
            if(header.Car=="dod"&&header.Pack>1)throw new InvalidDataException("Unsupported test paint (car="+header.Car+", pack="+header.Pack+")");
            byte[] canonical=(byte[])recorded.Clone();
            if(header.Car=="dod")Buffer.BlockCopy(Encoding.ASCII.GetBytes("alf"),0,canonical,header.CarOffset,3);
            int length=canonical.Length;
            // Remove only verifiable old experiment trailers, including the
            // nested trailer observed in the live test. Never trim race data.
            while(length>=TrailerSize){
                int at=length-TrailerSize;
                if(!canonical.Skip(at).Take(8).SequenceEqual(Magic)||!canonical.Skip(at+84).Take(8).SequenceEqual(Magic))break;
                if(BitConverter.ToInt32(canonical,at+8)!=at||Encoding.ASCII.GetString(canonical,at+12,3)!="dod"||canonical[at+15]!=0
                    ||BitConverter.ToInt32(canonical,at+16)!=header.Pack||!canonical.Skip(at+52).Take(32).SequenceEqual(Digest(canonical,at)))
                    throw new InvalidDataException("Invalid legacy custom-car trailer");
                length=at;
            }
            if(length==recorded.Length&&header.Car=="alf")return recorded;
            byte[] result=new byte[length];Buffer.BlockCopy(canonical,0,result,0,length);ReadHeader(result);return result;
        }
        internal static Header Restore(byte[] shared,byte[] installedModHash){return null;}
    }
}
