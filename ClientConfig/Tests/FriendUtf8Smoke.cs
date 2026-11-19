using System;using System.IO;using System.Reflection;using System.Reflection.Emit;using System.Runtime.InteropServices;using System.Text;using System.Text.RegularExpressions;
class FriendUtf8Smoke {
    static byte[] Bytes(string source,string name){string s=source.Substring(source.IndexOf(name+"[]={")+name.Length+4);s=s.Substring(0,s.IndexOf("};"));var m=Regex.Matches(s,"0x([0-9a-f]{2})");var b=new byte[m.Count];for(int i=0;i<b.Length;i++)b[i]=Convert.ToByte(m[i].Groups[1].Value,16);return b;}
    static void Check(bool ok,string message){if(!ok)throw new Exception(message);}
    static int Main(){
        string source;using(var r=new StreamReader(typeof(FriendUtf8Smoke).Assembly.GetManifestResourceStream("FriendOriginal")))source=r.ReadToEnd();
        byte[] raw=Bytes(source,"friend_utf8_decoder"),b=new byte[359];Check(raw.Length==371,"decoder size");Array.Copy(raw,12,b,0,b.Length);
        var method=new DynamicMethod("OriginalUtf8Decoder",typeof(int),new[]{typeof(IntPtr),typeof(IntPtr),typeof(int)},typeof(FriendUtf8Smoke),true);var il=method.GetILGenerator();
        for(int i=0;i<11;i++)il.DeclareLocal(i==1?typeof(byte):i==8?typeof(bool):typeof(int));
        var labels=new Label[b.Length];for(int i=0;i<b.Length;i++)labels[i]=il.DefineLabel();
        var codes=new System.Collections.Generic.Dictionary<short,OpCode>();foreach(var f in typeof(OpCodes).GetFields(BindingFlags.Public|BindingFlags.Static)){var op=(OpCode)f.GetValue(null);codes[op.Value]=op;}
        for(int at=0;at<b.Length;){il.MarkLabel(labels[at]);short v=b[at++];if(v==0xfe)v=(short)(0xfe00|b[at++]);var op=codes[v];switch(op.OperandType){
            case OperandType.InlineNone:il.Emit(op);break;
            case OperandType.ShortInlineI:il.Emit(op,(sbyte)b[at++]);break;
            case OperandType.InlineI:il.Emit(op,BitConverter.ToInt32(b,at));at+=4;break;
            case OperandType.ShortInlineVar:il.Emit(op,b[at++]);break;
            case OperandType.ShortInlineBrTarget:{int target=at+1+(sbyte)b[at];at++;il.Emit(op,labels[target]);break;}
            case OperandType.InlineBrTarget:{int target=at+4+BitConverter.ToInt32(b,at);at+=4;il.Emit(op,labels[target]);break;}
            default:throw new Exception("Unexpected decoder opcode "+op);
        }}
        var run=(Func<IntPtr,IntPtr,int,int>)method.CreateDelegate(typeof(Func<IntPtr,IntPtr,int,int>));
        byte[] patch=Bytes(source,"friend_utf8_patch");Check(BitConverter.ToString(patch)=="18-1E-62-28-EA-12-00-06-26-00","patch shape and original decoder token");
        foreach(string text in new[]{"","ASCII","HungryShark\u2122","Beyoncé","日本語",new string('a',510)+"™",new string('b',600)}){
            byte[] input=Encoding.UTF8.GetBytes(text+"\0");IntPtr src=Marshal.AllocHGlobal(input.Length),dst=Marshal.AllocHGlobal(1028);
            try{Marshal.Copy(input,0,src,input.Length);for(int i=0;i<1028;i++)Marshal.WriteByte(dst,i,0xcc);run(dst,src,512);
                string expected=text.Length>511?text.Substring(0,511):text;Check(Marshal.PtrToStringUni(dst)==expected,"decoded "+text.Length);Check(Marshal.ReadInt32(dst,1024)==unchecked((int)0xcccccccc),"buffer overrun");
            }finally{Marshal.FreeHGlobal(src);Marshal.FreeHGlobal(dst);}
        }
        Console.WriteLine("PASS: original UTF-8 decoder handles trademark, ASCII, accented/Japanese names, empty and bounded names");return 0;
    }
}
