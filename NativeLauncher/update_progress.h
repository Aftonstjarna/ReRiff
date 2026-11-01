/* Small native status window shared by the launcher and temporary updater.
 * 0 = cancellable, 1 = cancelled, 2 = installer handoff committed. */
#include <commctrl.h>
#define RERIFF_PROGRESS_EVENT (WM_APP+71)
enum { PROGRESS_CHECK, PROGRESS_DOWNLOAD, PROGRESS_VERIFY, PROGRESS_PREPARE, PROGRESS_INSTALL, PROGRESS_RESUME };
static HWND progress_window,progress_label,progress_bar,progress_cancel;
static HFONT progress_font;
static volatile LONG progress_state;
static LONG progress_last=-999;
static BOOL progress_cancelled(void){return InterlockedCompareExchange(&progress_state,0,0)==1;}
static inline BOOL progress_commit(void){return InterlockedCompareExchange(&progress_state,2,0)!=1;}
static void progress_report(int stage,int percent){
    LONG value=stage*102+percent+1;
    if(progress_window&&InterlockedExchange(&progress_last,value)!=value)
        PostMessageW(progress_window,RERIFF_PROGRESS_EVENT,(WPARAM)stage,(LPARAM)percent);
}
static LRESULT CALLBACK progress_procedure(HWND self,UINT event,WPARAM wp,LPARAM lp){
    if((event==WM_COMMAND&&LOWORD(wp)==IDCANCEL)||event==WM_CLOSE){
        if(InterlockedCompareExchange(&progress_state,1,0)==0){
            EnableWindow(progress_cancel,FALSE);SetWindowTextW(progress_label,L"Canceling update; the installed game will start...");
        }
        return 0;
    }
    if(event==RERIFF_PROGRESS_EVENT){
        if(progress_cancelled())return 0;
        static const WCHAR *labels[]={L"Checking for updates...",L"Downloading update...",L"Verifying download...",L"Preparing installation...",L"Installing update; please wait...",L"Starting game..."};
        if(wp<sizeof(labels)/sizeof(labels[0]))SetWindowTextW(progress_label,labels[wp]);
        int percent=(int)(INT_PTR)lp;
        LONG_PTR style=GetWindowLongPtrW(progress_bar,GWL_STYLE);
        if(percent>=0)SendMessageW(progress_bar,PBM_SETMARQUEE,FALSE,0);
        SetWindowLongPtrW(progress_bar,GWL_STYLE,percent<0?style|PBS_MARQUEE:style&~PBS_MARQUEE);
        if(percent<0)SendMessageW(progress_bar,PBM_SETMARQUEE,TRUE,40);
        if(percent>=0)SendMessageW(progress_bar,PBM_SETPOS,percent,0);
        EnableWindow(progress_cancel,InterlockedCompareExchange(&progress_state,0,0)==0);
        return 0;
    }
    return DefWindowProcW(self,event,wp,lp);
}
static BOOL progress_open(BOOL cancellable){
    InterlockedExchange(&progress_state,cancellable?0:2);progress_last=-999;
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);
    WNDCLASSW cls={0};cls.lpfnWndProc=progress_procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"ReRiffUpdateProgress";
    cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&cls);
    RECT bounds={0,0,470,142};AdjustWindowRect(&bounds,WS_CAPTION|WS_SYSMENU,FALSE);
    int width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;
    progress_window=CreateWindowExW(0,cls.lpszClassName,L"ReRiff update",WS_CAPTION|WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN)-width)/2,(GetSystemMetrics(SM_CYSCREEN)-height)/2,width,height,NULL,NULL,cls.hInstance,NULL);
    if(!progress_window)return FALSE;
    progress_font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    progress_label=CreateWindowW(L"STATIC",L"Checking for updates...",WS_CHILD|WS_VISIBLE,18,18,434,26,progress_window,NULL,cls.hInstance,NULL);
    progress_bar=CreateWindowW(PROGRESS_CLASSW,L"",WS_CHILD|WS_VISIBLE|PBS_MARQUEE,18,52,434,22,progress_window,NULL,cls.hInstance,NULL);
    progress_cancel=CreateWindowW(L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,352,94,100,30,progress_window,(HMENU)(INT_PTR)IDCANCEL,cls.hInstance,NULL);
    SendMessageW(progress_label,WM_SETFONT,(WPARAM)progress_font,TRUE);SendMessageW(progress_cancel,WM_SETFONT,(WPARAM)progress_font,TRUE);
    SendMessageW(progress_bar,PBM_SETRANGE32,0,100);EnableWindow(progress_cancel,cancellable);
    /* The helper is spawned hidden. SetWindowPos avoids the first-ShowWindow
     * STARTUPINFO override while keeping the status window non-activating. */
    SetWindowPos(progress_window,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    UpdateWindow(progress_window);return TRUE;
}
static void progress_pump(void){
    MSG msg;while(PeekMessageW(&msg,NULL,0,0,PM_REMOVE)){
        if(!progress_window||!IsDialogMessageW(progress_window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
}
static DWORD progress_wait(HANDLE process,DWORD timeout){
    if(!progress_window)return process?WaitForSingleObject(process,timeout):(Sleep(timeout),WAIT_TIMEOUT);
    ULONGLONG end=GetTickCount64()+timeout;
    for(;;){
        progress_pump();ULONGLONG now=GetTickCount64();
        if(now>=end)return process?WaitForSingleObject(process,0):WAIT_TIMEOUT;
        DWORD remaining=(DWORD)(end-now);
        DWORD result=MsgWaitForMultipleObjects(process?1:0,process?&process:NULL,FALSE,remaining,QS_ALLINPUT);
        if(process&&result==WAIT_OBJECT_0)return result;
        if(result==WAIT_FAILED||result==WAIT_TIMEOUT)return result;
    }
}
static void progress_close(void){
    if(progress_window)DestroyWindow(progress_window);
    progress_window=progress_label=progress_bar=progress_cancel=NULL;
    if(progress_font)DeleteObject(progress_font);progress_font=NULL;
}
