using System;using System.IO;using System.Reflection;using System.Reflection.Emit;using System.Runtime.InteropServices;using System.Security.Cryptography;
class RecentTracksSmoke {
    public static bool Bundled;public static int Comparator=0;static int sorts,erases;
    public static void Sort(int first,int last,int count,int compare){if(last-first!=count*200)throw new Exception("sort range");sorts++;}
    public static IntPtr Erase(IntPtr vector,IntPtr result,int entry){int end=Marshal.ReadInt32(vector,4);for(int p=entry;p<end-200;p++)Marshal.WriteByte(new IntPtr(p),Marshal.ReadByte(new IntPtr(p+200)));Marshal.WriteInt32(vector,4,end-200);Marshal.WriteInt32(result,entry);erases++;return result;}
    static int Offset(byte[] b,int rva){int pe=BitConverter.ToInt32(b,60),sections=pe+24+BitConverter.ToUInt16(b,pe+20);for(int i=0;i<BitConverter.ToUInt16(b,pe+6);i++){int s=sections+i*40,start=BitConverter.ToInt32(b,s+12),size=BitConverter.ToInt32(b,s+16);if(rva>=start&&rva<start+size)return BitConverter.ToInt32(b,s+20)+rva-start;}throw new Exception("RVA");}
    static Action<IntPtr,IntPtr> Compile(byte[] original,int limit){
        byte[] b=(byte[])original.Clone();b[0x58]=(byte)(limit==0?20:limit);if(limit==0)b[0x99]=0x16;
        var method=new DynamicMethod("OriginalRecentFilter",typeof(void),new[]{typeof(IntPtr),typeof(IntPtr)},typeof(RecentTracksSmoke),true);var il=method.GetILGenerator();
        var locals=new LocalBuilder[14];for(int i=0;i<14;i++)locals[i]=il.DeclareLocal(i==1||i==2||i==6?typeof(IntPtr):typeof(int));
        var labels=new Label[b.Length];for(int i=0;i<b.Length;i++)labels[i]=il.DefineLabel();
        var codes=new System.Collections.Generic.Dictionary<short,OpCode>();foreach(var f in typeof(OpCodes).GetFields(BindingFlags.Public|BindingFlags.Static)){var op=(OpCode)f.GetValue(null);codes[op.Value]=op;}
        for(int at=0;at<b.Length;){
            int start=at;il.MarkLabel(labels[start]);short value=b[at++];if(value==0xfe)value=(short)(0xfe00|b[at++]);var op=codes[value];
            switch(op.OperandType){
                case OperandType.InlineNone:il.Emit(op);break;
                case OperandType.ShortInlineI:il.Emit(op,(sbyte)b[at++]);break;
                case OperandType.InlineI:il.Emit(op,BitConverter.ToInt32(b,at));at+=4;break;
                case OperandType.ShortInlineVar:il.Emit(op,locals[b[at++]]);break;
                case OperandType.ShortInlineBrTarget:{int target=at+1+(sbyte)b[at];at++;il.Emit(op,labels[target]);break;}
                case OperandType.InlineBrTarget:{int target=at+4+BitConverter.ToInt32(b,at);at+=4;il.Emit(op,labels[target]);break;}
                case OperandType.InlineField:il.Emit(op,typeof(RecentTracksSmoke).GetField(start==0?"Bundled":"Comparator"));at+=4;break;
                case OperandType.InlineMethod:il.Emit(op,typeof(RecentTracksSmoke).GetMethod(start==0x7e?"Sort":"Erase"));at+=4;break;
                default:throw new Exception("Unsupported original opcode "+op);
            }
        }
        return (Action<IntPtr,IntPtr>)method.CreateDelegate(typeof(Action<IntPtr,IntPtr>));
    }
    static int Main(string[] args){
        byte[] body;
        if(args.Length>0){
            byte[] exe=File.ReadAllBytes(args[0]);using(var sha=SHA256.Create())if(BitConverter.ToString(sha.ComputeHash(exe)).Replace("-","").ToLowerInvariant()!="2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2")throw new Exception("original hash");
            body=new byte[199];Array.Copy(exe,Offset(exe,0x363884),body,0,199);
        }else{
            string source;using(var reader=new StreamReader(typeof(RecentTracksSmoke).Assembly.GetManifestResourceStream("RecentTracksOriginal")))source=reader.ReadToEnd();
            source=source.Substring(source.IndexOf("[]={")+4);source=source.Substring(0,source.IndexOf("};"));
            var matches=System.Text.RegularExpressions.Regex.Matches(source,"0x([0-9a-f]{2})");body=new byte[matches.Count];for(int i=0;i<body.Length;i++)body[i]=Convert.ToByte(matches[i].Groups[1].Value,16);
        }
        using(var sha=SHA256.Create())if(BitConverter.ToString(sha.ComputeHash(body)).Replace("-","").ToLowerInvariant()!="dbd6cfbaaf2e6bd637f2ed5066d2196ebe979856356ab4274ebefb293aa6989f")throw new Exception("original body hash");
        foreach(int limit in new[]{0,20,50,100})foreach(bool bundled in new[]{false,true})foreach(int count in new[]{0,1,20,21,113,1000}){
            var run=Compile(body,limit);IntPtr data=Marshal.AllocHGlobal(Math.Max(1,count)*200),vector=Marshal.AllocHGlobal(12);
            try{for(int i=0;i<count;i++){Marshal.WriteByte(data,i*200+198,(byte)(i%2));}Marshal.WriteInt32(vector,data.ToInt32());Marshal.WriteInt32(vector,4,data.ToInt32()+count*200);Marshal.WriteInt32(vector,8,data.ToInt32()+count*200);
                Bundled=bundled;sorts=erases=0;run(IntPtr.Zero,vector);int remaining=(Marshal.ReadInt32(vector,4)-data.ToInt32())/200;
                int expected=bundled?count/2:limit==0?count:Math.Min(limit,count);if(remaining!=expected||sorts!=(bundled?0:1)||erases!=count-expected)throw new Exception("filter semantics");
            }finally{Marshal.FreeHGlobal(vector);Marshal.FreeHGlobal(data);}
        }
        Console.WriteLine("PASS: 48 original/patched CIL executions; empty, 20/21, 113 and 1000 rows; bundled filter and native-sort invocation preserved");return 0;
    }
}
