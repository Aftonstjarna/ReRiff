using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
class PickerSearchBridgeSmoke {
    static Type bridge=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.NativePickerSearch",true),songType=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.LibrarySearchSong",true);
    static BindingFlags fields=BindingFlags.Static|BindingFlags.NonPublic,instance=BindingFlags.Instance|BindingFlags.NonPublic;
    static int allocations,layouts,checks;static string chosen,remembered;
    static void Check(bool ok,string text){checks++;if(!ok)throw new Exception(text);}
    static IntPtr Memory(int size){var p=Marshal.AllocHGlobal(size);for(int i=0;i<size;i++)Marshal.WriteByte(p,i,0);return p;}
    static IntPtr Ctor(IntPtr p,IntPtr text){string s=Marshal.PtrToStringUni(text);Marshal.WriteIntPtr(p,Marshal.StringToHGlobalUni(s));Marshal.WriteInt32(p,16,s.Length);Marshal.WriteInt32(p,20,Math.Max(8,s.Length));allocations++;return p;}
    static IntPtr NarrowCtor(IntPtr p,IntPtr text){int count=0;while(Marshal.ReadByte(text,count)!=0)count++;IntPtr data=Marshal.AllocHGlobal(count+1);for(int i=0;i<=count;i++)Marshal.WriteByte(data,i,Marshal.ReadByte(text,i));Marshal.WriteIntPtr(p,data);Marshal.WriteInt32(p,16,count);Marshal.WriteInt32(p,20,Math.Max(16,count));allocations++;return p;}
    static void Dtor(IntPtr p){IntPtr data=Marshal.ReadIntPtr(p);if(data!=IntPtr.Zero){Marshal.FreeHGlobal(data);Marshal.WriteIntPtr(p,IntPtr.Zero);allocations--;}}
    static int Count(IntPtr v){return (Marshal.ReadIntPtr(v,4).ToInt32()-Marshal.ReadIntPtr(v).ToInt32())/28;}
    static void Push(IntPtr vector,IntPtr entry){int count=Count(vector);IntPtr old=Marshal.ReadIntPtr(vector),next=Memory((count+1)*28);if(count>0){byte[] bytes=new byte[count*28];Marshal.Copy(old,bytes,0,bytes.Length);Marshal.Copy(bytes,0,next,bytes.Length);Marshal.FreeHGlobal(old);}Ctor(IntPtr.Add(next,count*28),Marshal.ReadIntPtr(entry));Marshal.WriteInt32(next,count*28+24,Marshal.ReadInt32(entry,24));Marshal.WriteIntPtr(vector,next);Marshal.WriteIntPtr(vector,4,IntPtr.Add(next,(count+1)*28));Marshal.WriteIntPtr(vector,8,IntPtr.Add(next,(count+1)*28));}
    static void Swap(IntPtr a,IntPtr b){for(int i=0;i<12;i+=4){IntPtr value=Marshal.ReadIntPtr(a,i);Marshal.WriteIntPtr(a,i,Marshal.ReadIntPtr(b,i));Marshal.WriteIntPtr(b,i,value);}}
    static void VectorDtor(IntPtr vector){int count=Count(vector);IntPtr data=Marshal.ReadIntPtr(vector);for(int i=0;i<count;i++)Dtor(IntPtr.Add(data,i*28));if(data!=IntPtr.Zero)Marshal.FreeHGlobal(data);for(int i=0;i<12;i+=4)Marshal.WriteInt32(vector,i,0);}
    static void Rebuild(IntPtr menu){layouts++;Check(Count(IntPtr.Add(menu,276))>=0,"Rebuild sees coherent vector");}
    static IntPtr ConvertPath(IntPtr menu,IntPtr storage,IntPtr wide){string path=Marshal.PtrToStringUni(Marshal.ReadIntPtr(wide));Dtor(wide);IntPtr text=Marshal.StringToHGlobalAnsi(path);try{return NarrowCtor(storage,text);}finally{Marshal.FreeHGlobal(text);}}
    static IntPtr SaveGame(){return new IntPtr(1);}
    static void RememberFolder(IntPtr save,IntPtr folder){remembered=Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(folder));Dtor(folder);}
    static void Selected(IntPtr menu,IntPtr filename){byte[] bytes=new byte[Marshal.ReadInt32(filename,16)];Marshal.Copy(Marshal.ReadIntPtr(filename),bytes,0,bytes.Length);chosen=Encoding.UTF8.GetString(bytes);}
    static void Bind(string field,string method){var f=bridge.GetField(field,fields);f.SetValue(null,Delegate.CreateDelegate(f.FieldType,typeof(PickerSearchBridgeSmoke).GetMethod(method,fields)));}
    static object Song(string path,string title){return Activator.CreateInstance(songType,instance,null,new object[]{"hst:"+path,"id",title,"Artist"},null);}
    static void Set(object obj,string name,object value){obj.GetType().GetField(name,instance).SetValue(obj,value);}
    static int Dispatch(int op,IntPtr menu,string text,int arg){IntPtr p=text==null?IntPtr.Zero:Marshal.StringToHGlobalUni(text);try{return (int)bridge.GetMethod("Dispatch",fields).Invoke(null,new object[]{op,menu,p,arg});}finally{if(p!=IntPtr.Zero)Marshal.FreeHGlobal(p);}}
    static void Main(){
        Bind("wideCtor","Ctor");Bind("wideDtor","Dtor");Bind("narrowCtor","NarrowCtor");Bind("narrowDtor","Dtor");Bind("vectorDtor","VectorDtor");Bind("push","Push");Bind("swap","Swap");Bind("rebuild","Rebuild");Bind("visibility","Rebuild");Bind("shortPath","ConvertPath");Bind("selected","Selected");Bind("rememberFolder","RememberFolder");Bind("saveGame","SaveGame");
        IntPtr menu=Memory(512),entry=Memory(28),text=Marshal.StringToHGlobalUni("original-folder");string path=Path.GetTempFileName();
        try {
            Ctor(entry,text);Marshal.WriteInt32(entry,24,3);Push(IntPtr.Add(menu,276),entry);Dtor(entry);
            IntPtr original=Marshal.ReadIntPtr(menu,276);
            object session=Activator.CreateInstance(bridge.GetNestedType("Session",BindingFlags.NonPublic),true);Set(session,"Menu",menu);Set(session,"Query","song");
            Array songs=Array.CreateInstance(songType,2);songs.SetValue(Song(path,"First"),0);songs.SetValue(Song(path,"Second"),1);Set(session,"Songs",songs);Set(session,"MatchedSongs",songs);
            Type indexType=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.PickerSearchIndex",true);
            object index=Activator.CreateInstance(indexType,instance,null,new object[]{new[]{"First","Second"}},null);Set(session,"Index",index);
            Set(session,"Matches",new[]{1,0});Set(session,"Published",0);bridge.GetField("current",fields).SetValue(null,session);
            Check(Dispatch(2,menu,null,0)==2&&Count(IntPtr.Add(menu,276))==2,"Native rows built from matches");
            Check(Marshal.ReadIntPtr(menu,276)!=original&&Marshal.PtrToStringUni(Marshal.ReadIntPtr(original))=="original-folder","Original vector retained without mutation");
            Check(Dispatch(4,menu,null,0)==1&&chosen=="hst:"+path,"Selection uses original conversion and hst prefix");
            Check(remembered==Path.GetDirectoryName(path),"Selection remembers the actual result folder for post-race reopening");
            Set(session,"Matches",new int[0]);Set(session,"Revision",1);Set(session,"Published",1);
            Check(Dispatch(2,menu,null,0)==0&&Count(IntPtr.Add(menu,276))==0,"Zero results have no fake selectable file");
            Check(Dispatch(4,menu,null,0)==0,"Empty selection refused");
            Check(Dispatch(1,menu,"",0)==-3&&Marshal.ReadIntPtr(menu,276)==original,"Clear restores exact original vector");
            // Hold the bridge gate while issuing revisions so the worker must
            // consume the latest query, with no timing-dependent sleep race.
            object sync=bridge.GetField("gate",fields).GetValue(null);
            lock(sync){for(int revision=0;revision<100;revision++)Dispatch(1,menu,revision==99?"Second":"obsolete",0);}
            bool finished=false;
            for(int retry=0;retry<500;retry++){
                lock(sync){finished=!(bool)session.GetType().GetField("Searching",instance).GetValue(session);}
                if(finished)break;System.Threading.Thread.Sleep(10);
            }
            Check(finished,"Coalesced worker completes");
            Check(Dispatch(2,menu,null,0)==1,"Latest query wins after rapid revisions");
            Check(((int[])session.GetType().GetField("Matches",instance).GetValue(session))[0]==1,"Latest result retains source identity");
            Dispatch(1,menu,"",0);
            Set(session,"Failed",true);Set(session,"Index",null);
            Check(Dispatch(1,menu,"new query",0)==-2,"Query cannot hide a failed worker behind endless Searching");
            Check(Dispatch(3,menu,null,0)==1&&bridge.GetField("current",fields).GetValue(null)==null,"Close releases session");
            Check(Dispatch(2,menu,null,0)==-2,"Closed callback refused");
            VectorDtor(IntPtr.Add(menu,276));Check(allocations==0,"All simulated native string ownership balanced");
            Console.WriteLine("PASS: {0} search bridge ownership, empty result, restore, conversion and lifecycle checks (native UI mocked)",checks);
        }finally{File.Delete(path);Marshal.FreeHGlobal(menu);Marshal.FreeHGlobal(entry);Marshal.FreeHGlobal(text);}
    }
}
