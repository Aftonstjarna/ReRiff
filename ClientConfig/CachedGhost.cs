using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace ReRiff {
    internal sealed class CachedGhost {
        internal string Filename,Song,Owner,Car;
        internal int Pack;
        internal float Score;
        private static string Text(BinaryReader reader) {
            int length=reader.ReadInt32();if(length<1||length>64)throw new InvalidDataException("Invalid native ghost string length");
            byte[] bytes=reader.ReadBytes(length);if(bytes.Length!=length||bytes.Any(b=>b<33||b>126))throw new InvalidDataException("Invalid native ghost identifier");
            return Encoding.ASCII.GetString(bytes);
        }
        internal static CachedGhost Read(string path,string owner) {
            string filename=Path.GetFileName(path);
            if(!Regex.IsMatch(owner??"",@"^[0-9]{1,20}$")||!Regex.IsMatch(filename,@"^[A-Za-z0-9]{1,64}-"+owner+@"\.ghost$"))return null;
            using(var stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read))return Read(stream,filename,owner);
        }
        internal static CachedGhost Read(Stream stream,string filename,string owner) {
            if(!Regex.IsMatch(owner??"",@"^[0-9]{1,20}$")||!Regex.IsMatch(filename,@"^[A-Za-z0-9]{1,64}-"+owner+@"\.ghost$"))return null;
            using(var reader=new BinaryReader(stream)) {
                // Original gaReplay.readFromFile header order; the original
                // upload/replay code still validates and consumes the payload.
                if(reader.ReadInt32()!=18)return null;reader.ReadInt32();
                float duration=reader.ReadSingle();if(Single.IsNaN(duration)||Single.IsInfinity(duration)||duration<=0)return null;
                string actualOwner=Text(reader);float score=reader.ReadSingle();string song=Text(reader);string car=Text(reader);int pack=reader.ReadInt32();
                if(actualOwner!=owner||filename!=song+"-"+owner+".ghost"||Single.IsNaN(score)||Single.IsInfinity(score)||score<0||(double)score>Int32.MaxValue||pack<0||pack>1024||stream.Position>=stream.Length)return null;
                return new CachedGhost{Filename=filename,Song=song,Owner=owner,Score=score,Car=car,Pack=pack};
            }
        }
    }
}
