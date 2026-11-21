/* Copyable error dialog shared by the launcher, updater and in-game helper.
 * The message is a read-only multiline edit so it can be selected and copied;
 * a Copy button also places the whole message on the clipboard. Falls back to
 * MessageBoxW if the window cannot be created. */
#ifndef RERIFF_ERROR_DIALOG_H
#define RERIFF_ERROR_DIALOG_H
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

#define RR_ERROR_EDIT 201
#define RR_ERROR_COPY 202
#define RR_ERROR_CLOSE 203

static BOOL rr_error_closed;

static void rr_error_clipboard(HWND edit) {
    int length=GetWindowTextLengthW(edit);
    if(length<=0)return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,(SIZE_T)(length+1)*sizeof(WCHAR));
    if(!memory)return;
    WCHAR *text=(WCHAR *)GlobalLock(memory);
    if(!text){GlobalFree(memory);return;}
    GetWindowTextW(edit,text,length+1);
    GlobalUnlock(memory);
    if(!OpenClipboard(edit)){GlobalFree(memory);return;}
    EmptyClipboard();
    if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);
    CloseClipboard();
}

static LRESULT CALLBACK rr_error_proc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    switch(message) {
    case WM_COMMAND:
        if(LOWORD(wparam)==RR_ERROR_COPY){rr_error_clipboard(GetDlgItem(window,RR_ERROR_EDIT));return 0;}
        if(LOWORD(wparam)==RR_ERROR_CLOSE||LOWORD(wparam)==IDCANCEL||LOWORD(wparam)==IDOK){DestroyWindow(window);return 0;}
        break;
    case WM_CLOSE:
        DestroyWindow(window);return 0;
    case WM_DESTROY:
        rr_error_closed=TRUE;
        PostThreadMessageW(GetCurrentThreadId(),WM_NULL,0,0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window,message,wparam,lparam);
}

static void rr_show_error(HWND owner,const WCHAR *title,const WCHAR *text) {
    static BOOL registered;
    HINSTANCE instance=GetModuleHandleW(NULL);
    if(!registered) {
        WNDCLASSEXW cls;
        ZeroMemory(&cls,sizeof(cls));
        cls.cbSize=sizeof(cls);
        cls.lpfnWndProc=rr_error_proc;
        cls.hInstance=instance;
        cls.hCursor=LoadCursorW(NULL,IDC_ARROW);
        cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
        cls.lpszClassName=L"ReRiffCopyableError";
        if(!RegisterClassExW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) {
            MessageBoxW(owner,text,title,MB_OK|MB_ICONERROR|MB_TOPMOST);
            return;
        }
        registered=TRUE;
    }
    RECT area;
    if(!(owner&&GetWindowRect(owner,&area)))SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);
    if(area.right<=area.left||area.bottom<=area.top) {
        area.left=0;area.top=0;area.right=GetSystemMetrics(SM_CXSCREEN);area.bottom=GetSystemMetrics(SM_CYSCREEN);
    }
    int width=560,height=280;
    int x=area.left+((area.right-area.left)-width)/2;
    int y=area.top+((area.bottom-area.top)-height)/2;
    HWND window=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_TOPMOST|WS_EX_CONTROLPARENT,
        L"ReRiffCopyableError",title?title:L"ReRiff",WS_POPUP|WS_CAPTION|WS_SYSMENU,
        x,y,width,height,owner,NULL,instance,NULL);
    if(!window) {
        MessageBoxW(owner,text,title,MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",text?text:L"",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
        12,12,width-36,height-84,window,(HMENU)(INT_PTR)RR_ERROR_EDIT,instance,NULL);
    HWND copy=CreateWindowExW(0,L"BUTTON",L"Copy",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        width-220,height-62,92,28,window,(HMENU)(INT_PTR)RR_ERROR_COPY,instance,NULL);
    HWND close=CreateWindowExW(0,L"BUTTON",L"Close",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
        width-120,height-62,92,28,window,(HMENU)(INT_PTR)RR_ERROR_CLOSE,instance,NULL);
    if(!edit||!copy||!close) {
        DestroyWindow(window);
        MessageBoxW(owner,text,title,MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    HFONT font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessageW(edit,WM_SETFONT,(WPARAM)font,TRUE);
    SendMessageW(copy,WM_SETFONT,(WPARAM)font,TRUE);
    SendMessageW(close,WM_SETFONT,(WPARAM)font,TRUE);
    if(owner)EnableWindow(owner,FALSE);
    rr_error_closed=FALSE;
    ShowWindow(window,SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(edit);
    SendMessageW(edit,EM_SETSEL,0,-1);
    MSG message;
    while(!rr_error_closed&&GetMessageW(&message,NULL,0,0)>0) {
        if(message.hwnd&&IsChild(window,message.hwnd)&&IsDialogMessageW(window,&message))continue;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if(IsWindow(window))DestroyWindow(window);
    if(owner)EnableWindow(owner,TRUE);
}
#endif
