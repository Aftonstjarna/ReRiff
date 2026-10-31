#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static HWND host,edit;
static BOOL observed=FALSE,failed=FALSE;
static int ticks=0;
static void finish(BOOL success) {
    if(!success) failed=TRUE;
    FILE *file=fopen("notification-test.txt","w");
    if(file){fputs(success?"PASS: no activation, focus retained, click-through, no controls, automatic dismissal\n":"FAIL\n",file);fclose(file);}
    DestroyWindow(host);
}
static LRESULT CALLBACK procedure(HWND window,UINT event,WPARAM wp,LPARAM lp) {
    if(event==WM_TIMER){
        ticks++;
        HWND toast=FindWindowW(L"ReRiffServerToast",NULL);
        if(toast && IsWindowVisible(toast)){
            observed=TRUE;
            LONG style=GetWindowLongW(toast,GWL_EXSTYLE);
            if(!(style&WS_EX_NOACTIVATE)||!(style&WS_EX_TRANSPARENT)||GetForegroundWindow()!=host||GetFocus()!=edit
                ||SendMessageW(toast,WM_NCHITTEST,0,0)!=HTTRANSPARENT||GetWindow(toast,GW_CHILD)) failed=TRUE;
        }
        if(observed && !toast){finish(!failed);return 0;}
        if(ticks>120){finish(FALSE);return 0;}
    }
    if(event==WM_DESTROY){PostQuitMessage(failed?1:0);return 0;}
    return DefWindowProcW(window,event,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR command,int show){
    (void)previous;(void)command;(void)show;
    WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=instance;cls.lpszClassName=L"ReRiffNotificationTest";
    RegisterClassW(&cls);
    host=CreateWindowW(cls.lpszClassName,L"Isolated game window",WS_OVERLAPPEDWINDOW,0,0,1000,650,NULL,NULL,instance,NULL);
    edit=CreateWindowW(L"EDIT",L"Game input remains focused",WS_VISIBLE|WS_CHILD|WS_BORDER,20,120,400,30,host,NULL,instance,NULL);
    ShowWindow(host,SW_SHOW);SetForegroundWindow(host);SetFocus(edit);
    HMODULE dll=LoadLibraryW(L"ReRiff.Notify.dll");
    typedef void (__cdecl *show_fn)(const WCHAR *);
    union {FARPROC address;show_fn function;} fn;
    fn.address=dll?GetProcAddress(dll,"ReRiffShowServerToastW"):NULL;
    if(!fn.address){failed=TRUE;finish(FALSE);return 1;}
    fn.function(L"http://127.0.0.1:5144");
    SetTimer(host,1,100,NULL);
    MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    /* Process exit releases the overlay module after its worker has finished. */
    return failed||!observed?1:0;
}
