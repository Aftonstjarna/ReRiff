using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using ReRiff;
public static class LanguageTextSmoke {
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    public static int Main(string[] args){
        bool latin=args.Length>0&&args[0]=="latin";bool enabled=args.Length==0||latin;string ini=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"server.ini");
        File.WriteAllText(ini,"[languages]\ncjk_enabled="+(enabled&&!latin?"true":"false")+"\nlatin_enabled="+(latin?"true":"false")+"\n[diagnostics]\nmetadata_utf8_probe="+(enabled?"0":"1")+"\n");
        try{
            string sample="BUCK‐TICK 日本語 繁體 한글 Dawid Podsiadło ĄąĆćĘęŁłŃńÓóŚśŹźŻż";byte[] data=Encoding.UTF8.GetBytes(sample);
            IntPtr key=Marshal.StringToHGlobalAnsi("Artist"),opaque=Marshal.StringToHGlobalAnsi("objectId"),input=Marshal.AllocHGlobal(data.Length+1);
            try{
                Marshal.Copy(data,0,input,data.Length);Marshal.WriteByte(input,data.Length,0);
                string legacy=Marshal.PtrToStringAnsi(input);
                Check(MetadataText.DecodeValue(input,key)==(enabled?sample:legacy),"Language checkbox did not control UTF-8 decoding.");
                Check(MetadataText.DecodeValue(input,opaque)==legacy,"Opaque identifiers changed.");
                IntPtr output=MetadataText.AllocateValue(sample,key);
                try{
                    if(enabled){int length=0;while(Marshal.ReadByte(output,length)!=0)length++;byte[] copy=new byte[length];Marshal.Copy(output,copy,0,length);Check(Encoding.UTF8.GetString(copy)==sample,"UTF-8 round trip failed.");}
                    else {IntPtr old=Marshal.StringToHGlobalAnsi(sample);try{Check(Marshal.PtrToStringAnsi(output)==Marshal.PtrToStringAnsi(old),"Disabled encoding changed.");}finally{Marshal.FreeHGlobal(old);}}
                }finally{Marshal.FreeHGlobal(output);}
            }finally{Marshal.FreeHGlobal(input);Marshal.FreeHGlobal(key);Marshal.FreeHGlobal(opaque);}
            Console.WriteLine("PASS: Languages setting overrides legacy probes; "+(enabled?(latin?"Latin-only UTF-8 metadata round trip":"CJK UTF-8 metadata round trip"):"disabled legacy metadata")+"; opaque identifiers retained.");return 0;
        }finally{File.Delete(ini);}
    }
}
