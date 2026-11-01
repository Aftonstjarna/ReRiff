/* Experimental in-process settings panel. No Game.exe addresses or patches. */
#define ReRiffShowServerToastW OriginalServerToastW
#include "../NativeLauncher/notification.c"
#undef ReRiffShowServerToastW
#include <wininet.h>

#define MENU_CLASS L"ReRiffExperimentalMenu"
#define EXTERNAL_ID 101
#define FALLBACK_ID 102
#define ONLY_ID 103
#define SAVE_ID 104
#define STATUS_ID 105
static volatile LONG menu_started;
static WCHAR ini_path[MAX_PATH];
static HWND panel, owner;

static void status(const WCHAR *text) {
    SetDlgItemTextW(panel, STATUS_ID, text);
}

static BOOL valid_url(WCHAR *url) {
    if (!*url) return TRUE;
    URL_COMPONENTSW parts;
    ZeroMemory(&parts, sizeof(parts)); parts.dwStructSize=sizeof(parts);
    parts.dwHostNameLength=parts.dwUserNameLength=parts.dwPasswordLength=1;
    parts.dwExtraInfoLength=1;
    for (WCHAR *p=url; *p; ++p) if (*p<=32 || *p==L'\\') return FALSE;
    if (!InternetCrackUrlW(url,0,0,&parts)) return FALSE;
    return (parts.nScheme==INTERNET_SCHEME_HTTP || parts.nScheme==INTERNET_SCHEME_HTTPS)
        && parts.dwHostNameLength && !parts.dwUserNameLength
        && !parts.dwPasswordLength && !parts.dwExtraInfoLength;
}

static void load_settings(void) {
    WCHAR value[2048];
    GetPrivateProfileStringW(L"server",L"external_url",L"",value,2048,ini_path);
    SetDlgItemTextW(panel,EXTERNAL_ID,value);
    GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,2048,ini_path);
    SetDlgItemTextW(panel,FALLBACK_ID,value);
    GetPrivateProfileStringW(L"server",L"external_only",L"false",value,2048,ini_path);
    CheckDlgButton(panel,ONLY_ID,(!_wcsicmp(value,L"true") || !_wcsicmp(value,L"yes")
        || !wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED);
    status(L"Changes apply after restarting the game.");
}

static BOOL save_settings(void) {
    WCHAR external[2048], fallback[2048], temporary[MAX_PATH];
    GetDlgItemTextW(panel,EXTERNAL_ID,external,2048);
    GetDlgItemTextW(panel,FALLBACK_ID,fallback,2048);
    if (!valid_url(external) || !valid_url(fallback)) {
        status(L"Use HTTP(S) URLs without credentials, queries or fragments."); return FALSE;
    }
    /* Stage the whole file: failed writes must not leave half a configuration. */
    if (wcslen(ini_path)+5>=MAX_PATH) { status(L"Settings path is too long."); return FALSE; }
    wcscpy(temporary,ini_path); wcscat(temporary,L".tmp");
    if (!CopyFileW(ini_path,temporary,FALSE)) {
        if (GetLastError()!=ERROR_FILE_NOT_FOUND) { status(L"Cannot stage settings."); return FALSE; }
        HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if (file==INVALID_HANDLE_VALUE) { status(L"Cannot create settings."); return FALSE; }
        CloseHandle(file);
    }
    BOOL ok=WritePrivateProfileStringW(L"server",L"external_url",external,temporary)
        && WritePrivateProfileStringW(L"server",L"fallback_url",fallback,temporary)
        && WritePrivateProfileStringW(L"server",L"external_only",
            IsDlgButtonChecked(panel,ONLY_ID)==BST_CHECKED?L"true":L"false",temporary);
    /* Cache-flush calls do not use the ordinary key-write success contract. */
    if (ok) {
        WritePrivateProfileStringW(NULL,NULL,NULL,temporary);
        ok=MoveFileExW(temporary,ini_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
        WritePrivateProfileStringW(NULL,NULL,NULL,ini_path);
    }
    if (!ok) DeleteFileW(temporary);
    status(ok?L"Saved. Restart the game to use these settings.":L"Save failed; original settings retained.");
    return ok;
}

static HWND control(const WCHAR *kind,const WCHAR *text,DWORD style,int x,int y,int w,int h,int id) {
    HWND child=CreateWindowExW(!wcscmp(kind,L"EDIT")?WS_EX_CLIENTEDGE:0,kind,text,
        WS_CHILD|WS_VISIBLE|style,x,y,w,h,panel,(HMENU)(INT_PTR)id,module,NULL);
    SendMessageW(child,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
    return child;
}

static LRESULT CALLBACK menu_proc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    if (message==WM_COMMAND) {
        if (LOWORD(wp)==SAVE_ID) { save_settings(); return 0; }
        if (LOWORD(wp)==IDCANCEL) { ShowWindow(window,SW_HIDE); SetForegroundWindow(owner); return 0; }
    }
    if (message==WM_CLOSE) { ShowWindow(window,SW_HIDE); SetForegroundWindow(owner); return 0; }
    return DefWindowProcW(window,message,wp,lp);
}

static DWORD WINAPI menu_thread(LPVOID unused) {
    (void)unused;
    WCHAR executable[MAX_PATH];
    DWORD length=GetModuleFileNameW(NULL,executable,MAX_PATH);
    if (!length || length>=MAX_PATH) return 1;
    WCHAR *slash=wcsrchr(executable,L'\\');
    if (!slash || (size_t)(slash-executable)+12>=MAX_PATH) return 1;
    slash[1]=0; wcscpy(ini_path,executable); wcscat(ini_path,L"server.ini");
    Toast search; ZeroMemory(&search,sizeof(search));
    DWORD start=GetTickCount();
    while (!search.game && GetTickCount()-start<45000) {
        EnumWindows(find_game,(LPARAM)&search); if (!search.game) Sleep(100);
    }
    if (!(owner=search.game)) return 2;
    WNDCLASSW cls; ZeroMemory(&cls,sizeof(cls));
    cls.lpfnWndProc=menu_proc; cls.hInstance=module; cls.lpszClassName=MENU_CLASS;
    cls.hCursor=LoadCursorW(NULL,IDC_ARROW); cls.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
    if (!RegisterClassW(&cls)) return 3;
    RECT rect; GetWindowRect(owner,&rect);
    panel=CreateWindowExW(WS_EX_CONTROLPARENT|WS_EX_TOOLWINDOW,MENU_CLASS,
        L"ReRiff experimental — Servers",WS_CAPTION|WS_SYSMENU|WS_POPUP,
        rect.left+50,rect.top+50,600,290,owner,NULL,module,NULL);
    if (!panel) return 4;
    control(L"STATIC",L"External server",0,16,16,550,20,0);
    HWND first=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,16,38,550,25,EXTERNAL_ID);
    control(L"STATIC",L"Fallback server",0,16,72,550,20,0);
    HWND second=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,16,94,550,25,FALLBACK_ID);
    SendMessageW(first,EM_SETLIMITTEXT,2047,0); SendMessageW(second,EM_SETLIMITTEXT,2047,0);
    control(L"BUTTON",L"External server only",WS_TABSTOP|BS_AUTOCHECKBOX,16,130,300,24,ONLY_ID);
    control(L"BUTTON",L"Save for next restart",WS_TABSTOP|BS_DEFPUSHBUTTON,16,166,220,30,SAVE_ID);
    control(L"BUTTON",L"Close",WS_TABSTOP,250,166,100,30,IDCANCEL);
    control(L"STATIC",L"",0,16,210,550,36,STATUS_ID);
    load_settings();
    SetTimer(panel,77,50,NULL);
    BOOL was_down=FALSE;
    MSG msg;
    while (GetMessageW(&msg,NULL,0,0)>0) {
        if (msg.message==WM_TIMER && msg.hwnd==panel) {
            if (!IsWindow(owner)) break;
            BOOL down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
            HWND foreground=GetForegroundWindow();
            if (down && !was_down && (foreground==owner || foreground==panel)) {
                if (IsWindowVisible(panel)) { ShowWindow(panel,SW_HIDE); SetForegroundWindow(owner); }
                else { load_settings(); ShowWindow(panel,SW_SHOW); SetForegroundWindow(panel); SetFocus(first); }
            }
            was_down=down;
        }
        if (!IsWindowVisible(panel) || !IsDialogMessageW(panel,&msg)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
    }
    KillTimer(panel,77); DestroyWindow(panel); return 0;
}

__declspec(dllexport) void __cdecl ReRiffStartExperimentalMenu(void) {
    if (InterlockedCompareExchange(&menu_started,1,0)) return;
    HANDLE thread=CreateThread(NULL,0,menu_thread,NULL,0,NULL);
    if (thread) CloseHandle(thread); else InterlockedExchange(&menu_started,0);
}

__declspec(dllexport) void __cdecl ReRiffShowServerToastW(const WCHAR *endpoint) {
    ReRiffStartExperimentalMenu();
    OriginalServerToastW(endpoint);
}
