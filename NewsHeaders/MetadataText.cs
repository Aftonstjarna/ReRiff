// Opt-in correction at the original ParseOctane native string boundary.
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace ReRiff {
    public static class MetadataText {
        private static readonly UTF8Encoding Utf8=new UTF8Encoding(false,true);
        private static readonly bool Enabled=ReadEnabled();
        static MetadataText(){} // Read opt-in configuration at first use.
        private static bool ReadEnabled(){
            try{
                bool diagnostics=false,languages=false,result=false;bool? installed=null;bool latin=false;
                foreach(string raw in File.ReadAllLines(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"server.ini"))){
                    string line=raw.Trim();
                    if(line.StartsWith("[",StringComparison.Ordinal)){diagnostics=string.Equals(line,"[diagnostics]",StringComparison.OrdinalIgnoreCase);languages=string.Equals(line,"[languages]",StringComparison.OrdinalIgnoreCase);}
                    int equals=line.IndexOf('=');
                    if(languages&&equals>0&&string.Equals(line.Substring(0,equals).Trim(),"cjk_enabled",StringComparison.OrdinalIgnoreCase)){
                        string value=line.Substring(equals+1).Trim();installed=value=="1"||string.Equals(value,"true",StringComparison.OrdinalIgnoreCase);
                    }
                    if(languages&&equals>0&&string.Equals(line.Substring(0,equals).Trim(),"latin_enabled",StringComparison.OrdinalIgnoreCase)){
                        string value=line.Substring(equals+1).Trim();latin=value=="1"||string.Equals(value,"true",StringComparison.OrdinalIgnoreCase);
                    }
                    if(diagnostics&&equals>0&&string.Equals(line.Substring(0,equals).Trim(),"metadata_utf8_probe",StringComparison.OrdinalIgnoreCase)){
                        string value=line.Substring(equals+1).Trim();result=value=="1"||string.Equals(value,"true",StringComparison.OrdinalIgnoreCase);
                    }
                }
                return latin||(installed??result);
            }catch(IOException){return false;}catch(UnauthorizedAccessException){return false;}
        }
        private static string Field(IntPtr key){
            if(key==IntPtr.Zero)return null;
            var name=new StringBuilder();
            for(int i=0;i<32;i++){
                byte b=Marshal.ReadByte(key,i);if(b==0){
                    string value=name.ToString();
                    switch(value.ToLowerInvariant()){
                        case "title":case "artist":case "album":case "genre":
                        case "songname":case "songtitle":case "artistname":case "albumname":return value;
                        default:return null;
                    }
                }
                if(b>127)return null;name.Append((char)b);
            }
            return null;
        }
        public static string DecodeValue(IntPtr value,IntPtr key){
            string field=Field(key);
            if(field==null)return Marshal.PtrToStringAnsi(value)??string.Empty;
            if(value==IntPtr.Zero)return string.Empty;
            int length=0;while(length<65536&&Marshal.ReadByte(value,length)!=0)length++;
            if(length==65536)return Marshal.PtrToStringAnsi(value)??string.Empty;
            byte[] bytes=new byte[length];Marshal.Copy(value,bytes,0,length);
            string decoded;
            try{decoded=Utf8.GetString(bytes);}
            catch(DecoderFallbackException){
                NewsHeaders.TraceMetadataBridge("bridge-in",field,bytes,null,false);
                return Marshal.PtrToStringAnsi(value)??string.Empty;
            }
            NewsHeaders.TraceMetadataBridge("bridge-in",field,bytes,decoded,true);
            return Enabled?decoded:(Marshal.PtrToStringAnsi(value)??string.Empty);
        }
        public static IntPtr AllocateValue(string value,IntPtr key){
            string field=Field(key);
            if(!Enabled||field==null||value==null)return Marshal.StringToHGlobalAnsi(value);
            // Original strncpy copies 4095 bytes into a shared 4096-byte
            // buffer, then frees this temporary with FreeHGlobal. Include a
            // terminator inside that copied range and never split UTF-8.
            byte[] bytes=new byte[4095];int charsUsed,bytesUsed;bool completed;
            try{
                char[] chars=value.ToCharArray();
                Utf8.GetEncoder().Convert(chars,0,chars.Length,bytes,0,4094,true,out charsUsed,out bytesUsed,out completed);
            }catch(EncoderFallbackException){return Marshal.StringToHGlobalAnsi(value);}
            IntPtr result=Marshal.AllocHGlobal(bytesUsed+1);
            try{
                Marshal.Copy(bytes,0,result,bytesUsed);Marshal.WriteByte(result,bytesUsed,0);
                NewsHeaders.TraceMetadataBridge("bridge-out",field,null,Utf8.GetString(bytes,0,bytesUsed),true);
                return result;
            }catch{Marshal.FreeHGlobal(result);throw;}
        }
    }
}
