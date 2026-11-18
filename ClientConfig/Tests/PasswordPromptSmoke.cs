using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
class PasswordPromptSmoke {
    delegate bool EnumProc(IntPtr window,IntPtr value);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr FindWindow(string name,string title);
    [DllImport("user32.dll")]static extern bool EnumChildWindows(IntPtr window,EnumProc callback,IntPtr value);
    [DllImport("user32.dll")]static extern int GetWindowLong(IntPtr window,int index);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr SendMessage(IntPtr window,int message,IntPtr wp,string text);
    [DllImport("user32.dll")]static extern bool PostMessage(IntPtr window,int message,IntPtr wp,IntPtr lp);
    static void Main(){
        var type=typeof(ReRiff.ClientConfig).Assembly.GetType("ReRiff.ProgressionRemote");
        var method=type.GetMethod("PromptPassword",BindingFlags.Static|BindingFlags.NonPublic);
        foreach(bool cancel in new[]{true,false}){
            var automation=new Thread(()=>{
                for(int i=0;i<150;i++){
                    IntPtr window=FindWindow(null,"ReRiff server password");
                    if(window!=IntPtr.Zero){
                        if(cancel){PostMessage(window,0x10,IntPtr.Zero,IntPtr.Zero);return;}
                        IntPtr edit=IntPtr.Zero;
                        EnumChildWindows(window,(child,unused)=>{if((GetWindowLong(child,-16)&0x20)!=0){var kind=new StringBuilder(128);GetClassName(child,kind,kind.Capacity);if(kind.ToString()=="Edit")edit=child;}return true;},IntPtr.Zero);
                        if(edit!=IntPtr.Zero){SendMessage(edit,0xC,IntPtr.Zero,"test-password");PostMessage(window,0x111,new IntPtr(1),IntPtr.Zero);return;}
                    }
                    Thread.Sleep(100);
                }
                Console.Error.WriteLine("Native password dialog automation timed out");Environment.Exit(1);
            });automation.Start();
            string result=(string)method.Invoke(null,new object[]{"https://test.example",false});automation.Join();
            if(cancel?result!=null:result!="test-password")throw new Exception("Native password result differs");
        }
        Console.WriteLine("PASS: native masked password entry, accept and cancel");
    }
    [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern int GetClassName(IntPtr window,StringBuilder name,int length);
}
