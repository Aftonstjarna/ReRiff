#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

#define CAP 32768
#ifndef RERIFF_VERSION
#define RERIFF_VERSION "0.1.0"
#endif
#define RERIFF_WIDE_LITERAL_(value) L##value
#define RERIFF_WIDE_LITERAL(value) RERIFF_WIDE_LITERAL_(value)
#ifndef RERIFF_HEALTH_URL
#define RERIFF_HEALTH_URL L"http://127.0.0.1:5144/"
#endif
#define WM_COMPLETE (WM_APP + 1)
#define WM_GAME_RUNNING (WM_APP + 2)
enum { EXTERNAL=101, FALLBACK, TIMEOUT, POPUP, SAVE, PLAY, LOCAL, EXTERNAL_ONLY, STOP, UPDATE, EXPERIMENTAL };
static WCHAR folder[CAP], ini[CAP], message[1024];
static HWND window, status, external, fallback, timeout_edit, popup, play_button, save_button;
static HWND only_checkbox, stop_button, update_button;
static BOOL busy = FALSE, playing = FALSE;
static HWND experimental_checkbox;
static BOOL installed_experimental=FALSE, requested_experimental=FALSE;
static HFONT font;

static void path(WCHAR *out, const WCHAR *relative) {
    _snwprintf(out, CAP, L"%ls\\%ls", folder, relative);
    out[CAP-1] = 0;
}

static void load_channel(void) {
    WCHAR name[CAP],value[32];path(name,L"localserver\\client-channel.ini");
    GetPrivateProfileStringW(L"client",L"channel",L"stable",value,32,name);
    installed_experimental=!wcscmp(value,L"experimental");
    requested_experimental=installed_experimental;
}

static void channel_title(void) {
    SetWindowTextW(window,installed_experimental
        ? L"ReRiff Launcher v" RERIFF_WIDE_LITERAL(RERIFF_VERSION) L" — Experimental"
        : L"ReRiff Launcher v" RERIFF_WIDE_LITERAL(RERIFF_VERSION));
}

static void log_message(const WCHAR *text) {
    WCHAR name[CAP]; char utf8[4096]; DWORD written;
    path(name, L"ReRiff-launcher.log");
    int bytes = WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, sizeof(utf8)-3, NULL, NULL);
    if (!bytes) return;
    utf8[bytes-1]='\r'; utf8[bytes]='\n';
    HANDLE file=CreateFileW(name, FILE_APPEND_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) { WriteFile(file, utf8, bytes+1, &written, NULL); CloseHandle(file); }
}

static BOOL spawn(WCHAR *command, BOOL hidden, PROCESS_INFORMATION *process) {
    STARTUPINFOW start; ZeroMemory(&start, sizeof(start)); ZeroMemory(process, sizeof(*process));
    start.cb=sizeof(start);
    if (hidden) { start.dwFlags=STARTF_USESHOWWINDOW; start.wShowWindow=SW_HIDE; }
    return CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, folder, &start, process);
}

static BOOL is_wine(void) {
    return GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version") != NULL;
}

static BOOL external_only(void) {
    WCHAR value[32];GetPrivateProfileStringW(L"server",L"external_only",L"false",value,32,ini);
    return !_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes");
}

#include "auto_update.h"

static BOOL stop_server(void) {
    if (is_wine()) {
        typedef char * (__cdecl *unix_fn)(LPCWSTR);
        union {FARPROC address;unix_fn function;} conversion;
        conversion.address=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"wine_get_unix_file_name");
        WCHAR script[CAP],host[CAP],system[CAP],command[CAP],temporary[CAP],result[CAP],host_result[CAP];
        path(script,L"localserver\\stop-server-host.sh");
        if(!conversion.address||GetFileAttributesW(script)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The local stop-server helper is missing.");return FALSE;}
        if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rrs",0,result))return FALSE;
        DeleteFileW(result);
        char *value=conversion.function(script);if(!value)return FALSE;
        MultiByteToWideChar(CP_UTF8,0,value,-1,host,CAP);HeapFree(GetProcessHeap(),0,value);
        value=conversion.function(result);if(!value)return FALSE;
        MultiByteToWideChar(CP_UTF8,0,value,-1,host_result,CAP);HeapFree(GetProcessHeap(),0,value);
        GetSystemDirectoryW(system,CAP);
        _snwprintf(command,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" --result \"%ls\"",system,host,host_result);command[CAP-1]=0;
        PROCESS_INFORMATION process;if(!spawn(command,TRUE,&process)){wcscpy(message,L"Could not invoke the local stop helper.");return FALSE;}
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
        ULONGLONG deadline=GetTickCount64()+15000;
        while(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES&&GetTickCount64()<deadline)Sleep(100);
        FILE *file=_wfopen(result,L"rb");char outcome[2048]="";
        if(file){size_t count=fread(outcome,1,sizeof(outcome)-1,file);outcome[count]=0;fclose(file);DeleteFileW(result);}
        if(strcmp(outcome,"ok\n")){
            if(*outcome){WCHAR detail[2048];MultiByteToWideChar(CP_UTF8,0,outcome,-1,detail,2048);log_message(detail);}
            wcscpy(message,L"Server shutdown was not confirmed. Check localserver/server-console.log.");return FALSE;
        }
        wcscpy(message,L"Local server stopped or already stopped. Docker and unrelated services were left untouched.");return TRUE;
    }
    WCHAR record[CAP],value[64];path(record,L"localserver\\server-process.ini");
    DWORD pid=GetPrivateProfileIntW(L"process",L"pid",0,record);
    GetPrivateProfileStringW(L"process",L"creation",L"0",value,64,record);
    ULONGLONG expected=_wcstoui64(value,NULL,10);
    if(!pid||!expected){wcscpy(message,L"No launcher-owned server record. Docker and other services were left untouched.");return TRUE;}
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_TERMINATE|SYNCHRONIZE,FALSE,pid);
    if(!process){wcscpy(message,L"Recorded server is not running or cannot be accessed.");return FALSE;}
    FILETIME created,exited,kernel,user;
    BOOL match=GetProcessTimes(process,&created,&exited,&kernel,&user)
        && ((((ULONGLONG)created.dwHighDateTime<<32)|created.dwLowDateTime)==expected);
    BOOL ok=match&&TerminateProcess(process,0);
    if(ok){WaitForSingleObject(process,5000);DeleteFileW(record);}
    CloseHandle(process);
    wcscpy(message,ok?L"Local Python server stopped.":L"Server identity did not match or stop failed. No unrelated process was stopped.");
    return ok;
}

/* 1: ReRiff, 0: no reachable server, -1: another HTTP service. */
static int health(void) {
    HINTERNET session=InternetOpenW(L"ReRiffLauncher/1", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!session) return 0;
    DWORD limit=500;
    InternetSetOptionW(session, INTERNET_OPTION_CONNECT_TIMEOUT, &limit, sizeof(limit));
    InternetSetOptionW(session, INTERNET_OPTION_RECEIVE_TIMEOUT, &limit, sizeof(limit));
    InternetSetOptionW(session, INTERNET_OPTION_SEND_TIMEOUT, &limit, sizeof(limit));
    HINTERNET request=InternetOpenUrlW(session,RERIFF_HEALTH_URL,NULL,0,
        INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_UI|INTERNET_FLAG_NO_COOKIES,0);
    int result=0;
    if (request) {
        char buffer[8192]; DWORD count=0, code=0, size=sizeof(code);
        result=-1;
        if (HttpQueryInfoW(request,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&code,&size,NULL)
            && code==200 && InternetReadFile(request,buffer,sizeof(buffer)-1,&count)) {
            buffer[count]=0;
            if (strstr(buffer,"\"server\"") && strstr(buffer,"\"RiffApiLocal\"")) result=1;
        }
        InternetCloseHandle(request);
    }
    InternetCloseHandle(session);
    return result;
}

static BOOL start_server(void) {
    HANDLE mutex=CreateMutexW(NULL,FALSE,L"Local\\ReRiffServerStart5144");
    if (!mutex) return FALSE;
    DWORD lock=WaitForSingleObject(mutex,15000);
    if (lock!=WAIT_OBJECT_0 && lock!=WAIT_ABANDONED) {
        wcscpy(message,L"Another launcher is starting the server. Try again shortly."); CloseHandle(mutex); return FALSE;
    }
    BOOL ok=FALSE;
    int state=health();
    if (state==1) { ok=TRUE; goto finish; }
    if (state<0) { wcscpy(message,L"Port 5144 is used by an unrecognized HTTP service."); goto finish; }
    WCHAR command[CAP], executable[CAP]; PROCESS_INFORMATION process;
    if (is_wine()) {
        typedef char * (__cdecl *unix_name_fn)(LPCWSTR);
        union { FARPROC address; unix_name_fn function; } conversion;
        conversion.address=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"wine_get_unix_file_name");
        unix_name_fn unix_name=conversion.function;
        path(executable,L"localserver\\start-server-host.sh");
        if (GetFileAttributesW(executable)==INVALID_FILE_ATTRIBUTES || !unix_name) {
            wcscpy(message,L"localserver/start-server-host.sh is missing, or Wine path conversion is unavailable."); goto finish;
        }
        char *unix_path=unix_name(executable);
        if (!unix_path) { wcscpy(message,L"Could not translate the server path for Proton."); goto finish; }
        WCHAR host_path[CAP];
        MultiByteToWideChar(CP_UTF8,0,unix_path,-1,host_path,CAP);
        HeapFree(GetProcessHeap(),0,unix_path);
        /* Wine's start /unix bridge launches the Linux backend, not Windows Python. */
        WCHAR system[CAP]; GetSystemDirectoryW(system,CAP);
        _snwprintf(command,CAP,L"\"%ls\\start.exe\" /unix /bin/sh \"%ls\"",system,host_path);
    } else {
        path(executable,L"localserver\\windows-python\\python.exe");
        if (GetFileAttributesW(executable)==INVALID_FILE_ATTRIBUTES)
            path(executable,L"localserver\\windows-venv\\Scripts\\python.exe");
        WCHAR runner[CAP], source[CAP]; path(runner,L"localserver\\trace-server.py"); path(source,L"localserver\\server");
        if (GetFileAttributesW(executable)!=INVALID_FILE_ATTRIBUTES)
            _snwprintf(command,CAP,L"\"%ls\" \"%ls\" --server-dir \"%ls\"",executable,runner,source);
        else
            _snwprintf(command,CAP,L"py.exe -3 \"%ls\" --server-dir \"%ls\"",runner,source);
    }
    command[CAP-1]=0;
    if (!spawn(command,TRUE,&process)) {
        wcscpy(message,L"Could not start the backend. On Windows, install Python 3 and Flask or create localserver/windows-venv."); goto finish;
    }
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    for (int i=0;i<48;i++) {
        if (health()==1) { ok=TRUE; break; }
        Sleep(250);
    }
    if (!ok) wcscpy(message,L"The backend did not become ready. Check localserver/server-console.log and ReRiff-launcher.log.");
finish:
    ReleaseMutex(mutex); CloseHandle(mutex); return ok;
}

#include "song_sync.h"

static BOOL launch_game(void) {
    sync_cached_songs(ini);
    WCHAR game[CAP],command[CAP]; PROCESS_INFORMATION process;
    path(game,L"Game.exe");
    _snwprintf(command,CAP,L"\"%ls\"",game);command[CAP-1]=0;
    if (!spawn(command,FALSE,&process)) { wcscpy(message,L"Could not start Game.exe beside the launcher."); return FALSE; }
    if (window) PostMessageW(window,WM_GAME_RUNNING,0,0);
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess,INFINITE);
    CloseHandle(process.hProcess);
    sync_cached_songs(ini);
    return TRUE;
}

static DWORD WINAPI worker(LPVOID parameter) {
    int action=(int)(INT_PTR)parameter;
    BOOL play=action==1;
    BOOL only=external_only();
    BOOL ok=action==3?check_updates():action==2?stop_server():(only?TRUE:start_server());
    if (ok && play) ok=launch_game();
    if (ok && action<2) wcscpy(message,only?(play?L"Game closed. External-only mode did not start a local server.":L"External-only mode: local autostart and fallback are disabled."):(play?L"Game closed. Local server remains running.":L"Local server is ready."));
    log_message(message);
    PostMessageW(window,WM_COMPLETE,ok,update_handoff?2:play);
    return 0;
}

static void begin(int action) {
    if (busy) return;
    busy=TRUE;playing=action==1;
    EnableWindow(play_button,FALSE);EnableWindow(save_button,FALSE);EnableWindow(stop_button,FALSE);EnableWindow(update_button,FALSE);EnableWindow(experimental_checkbox,FALSE);
    SetWindowTextW(status,action==3?L"Checking published GitHub releases...":action==2?L"Stopping this installation's local server...":L"Checking server settings...");
    HANDLE thread=CreateThread(NULL,4*1024*1024,worker,(LPVOID)(INT_PTR)action,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
    if (thread) CloseHandle(thread);
    else { busy=FALSE; EnableWindow(play_button,TRUE);EnableWindow(save_button,TRUE);EnableWindow(stop_button,TRUE);EnableWindow(update_button,TRUE);EnableWindow(experimental_checkbox,TRUE);SetWindowTextW(status,L"Could not start launcher worker."); }
}

static HWND control(const WCHAR *cls,const WCHAR *text,DWORD style,int x,int y,int w,int h,int id) {
    HWND handle=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    SendMessageW(handle,WM_SETFONT,(WPARAM)font,TRUE);return handle;
}

static BOOL valid_url(const WCHAR *value) {
    if (!*value) return TRUE;
    URL_COMPONENTSW parts;ZeroMemory(&parts,sizeof(parts));parts.dwStructSize=sizeof(parts);
    WCHAR host[1024],user[256],password[256];parts.lpszHostName=host;parts.dwHostNameLength=1024;
    parts.lpszUserName=user;parts.dwUserNameLength=256;parts.lpszPassword=password;parts.dwPasswordLength=256;
    return InternetCrackUrlW(value,0,0,&parts) && parts.dwHostNameLength>0
        && (parts.nScheme==INTERNET_SCHEME_HTTP||parts.nScheme==INTERNET_SCHEME_HTTPS)
        && !parts.dwUserNameLength && !parts.dwPasswordLength && !wcschr(value,L'?') && !wcschr(value,L'#');
}

static BOOL save(void) {
    WCHAR a[4096],b[4096],t[32];
    GetWindowTextW(external,a,4096);GetWindowTextW(fallback,b,4096);GetWindowTextW(timeout_edit,t,32);
    WCHAR *end; double seconds=wcstod(t,&end);
    BOOL only=SendMessageW(only_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
    if (!valid_url(a)||(!only&&!valid_url(b))||!*t||*end||!(seconds>0 && seconds<=60)) {
        MessageBoxW(window,L"Use HTTP(S) URLs and a timeout greater than 0 and at most 60 seconds.",L"Invalid settings",MB_OK|MB_ICONWARNING);return FALSE;
    }
    BOOL ok=WritePrivateProfileStringW(L"server",L"external_url",a,ini)
        && WritePrivateProfileStringW(L"server",L"fallback_url",b,ini)
        && WritePrivateProfileStringW(L"server",L"timeout_seconds",t,ini)
        && WritePrivateProfileStringW(L"server",L"external_only",only?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"server",L"show_server_popup",SendMessageW(popup,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini);
    if (!ok) MessageBoxW(window,L"Could not save server.ini.",L"ReRiff",MB_OK|MB_ICONERROR);
    else SetWindowTextW(status,L"Settings saved. They apply on the next game start.");
    return ok;
}

static LRESULT CALLBACK procedure(HWND hwnd,UINT event,WPARAM wp,LPARAM lp) {
    if (event==WM_CREATE) {
        window=hwnd;font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        control(L"STATIC",L"Riff Racer — Server settings",0,22,18,600,30,0);
        control(L"STATIC",L"External server",0,22,66,145,24,0);
        external=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,172,62,448,28,EXTERNAL);
        control(L"STATIC",L"Fallback server",0,22,108,145,24,0);
        fallback=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,172,104,448,28,FALLBACK);
        control(L"STATIC",L"Timeout (seconds)",0,22,150,145,24,0);
        timeout_edit=control(L"EDIT",L"5",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,172,146,95,28,TIMEOUT);
        popup=control(L"BUTTON",L"Show the selected server when the game starts",BS_AUTOCHECKBOX|WS_TABSTOP,22,191,590,28,POPUP);
        only_checkbox=control(L"BUTTON",L"External server only (no local autostart or fallback)",BS_AUTOCHECKBOX|WS_TABSTOP,22,225,590,28,EXTERNAL_ONLY);
        control(L"STATIC",L"Both addresses blank disables game networking.\nStop local server affects this installation's Python backend, not Docker.",0,22,267,600,46,0);
        status=control(L"STATIC",L"Checking server settings...",0,22,323,600,62,0);
        control(L"BUTTON",L"Use local server",WS_TABSTOP,22,409,145,34,LOCAL);
        stop_button=control(L"BUTTON",L"Stop local server",WS_TABSTOP,180,409,150,34,STOP);
        save_button=control(L"BUTTON",L"Save settings",WS_TABSTOP,345,409,130,34,SAVE);
        play_button=control(L"BUTTON",L"Save and play",WS_TABSTOP|BS_DEFPUSHBUTTON,490,409,130,34,PLAY);
        update_button=control(L"BUTTON",L"Update from GitHub",WS_TABSTOP,22,453,155,30,UPDATE);
        experimental_checkbox=control(L"BUTTON",L"Use experimental builds",BS_AUTOCHECKBOX|WS_TABSTOP,195,453,420,30,EXPERIMENTAL);
        SendMessageW(experimental_checkbox,BM_SETCHECK,installed_experimental?BST_CHECKED:BST_UNCHECKED,0);
        channel_title();
        WCHAR value[4096];
        GetPrivateProfileStringW(L"server",L"external_url",L"",value,4096,ini);SetWindowTextW(external,value);
        GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,4096,ini);SetWindowTextW(fallback,value);
        GetPrivateProfileStringW(L"server",L"timeout_seconds",L"5",value,4096,ini);SetWindowTextW(timeout_edit,value);
        GetPrivateProfileStringW(L"server",L"show_server_popup",L"true",value,4096,ini);
        SendMessageW(popup,BM_SETCHECK,(_wcsicmp(value,L"false")&&_wcsicmp(value,L"no")&&wcscmp(value,L"0"))?BST_CHECKED:BST_UNCHECKED,0);
        SendMessageW(only_checkbox,BM_SETCHECK,external_only()?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(fallback,!external_only());
        begin(FALSE);return 0;
    }
    if (event==WM_COMMAND) {
        switch (LOWORD(wp)) {
            case LOCAL:SendMessageW(only_checkbox,BM_SETCHECK,BST_UNCHECKED,0);EnableWindow(fallback,TRUE);SetWindowTextW(fallback,L"http://127.0.0.1:5144");break;
            case EXTERNAL_ONLY:EnableWindow(fallback,SendMessageW(only_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED);break;
            case STOP:begin(2);break;
            case EXPERIMENTAL:
                requested_experimental=SendMessageW(experimental_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
                begin(3);break;
            case UPDATE:begin(3);break;
            case SAVE:save();break;
            case PLAY:if (save()) begin(TRUE);break;
        }return 0;
    }
    if (event==WM_GAME_RUNNING) { SetWindowTextW(status,L"Game running. Keep the launcher open until the game closes.");ShowWindow(window,SW_MINIMIZE);return 0; }
    if (event==WM_COMPLETE) {
        if (lp==2) { DestroyWindow(hwnd);return 0; }
        requested_experimental=installed_experimental;
        SendMessageW(experimental_checkbox,BM_SETCHECK,installed_experimental?BST_CHECKED:BST_UNCHECKED,0);
        busy=FALSE;playing=FALSE;EnableWindow(play_button,TRUE);EnableWindow(save_button,TRUE);EnableWindow(stop_button,TRUE);EnableWindow(update_button,TRUE);EnableWindow(experimental_checkbox,TRUE);SetWindowTextW(status,message);
        if (lp) ShowWindow(window,SW_RESTORE);return 0;
    }
    if (event==WM_CLOSE) {
        if (busy) { SetWindowTextW(status,L"Wait for server startup or close the game before closing this launcher.");return 0; }
        DestroyWindow(hwnd);return 0;
    }
    if (event==WM_DESTROY) { DeleteObject(font);PostQuitMessage(0);return 0; }
    return DefWindowProcW(hwnd,event,wp,lp);
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR command,int show) {
    (void)previous;(void)command;
    GetModuleFileNameW(NULL,folder,CAP);WCHAR *slash=wcsrchr(folder,L'\\');if (!slash) return 1;*slash=0;
    path(ini,L"server.ini");load_channel();
    BOOL skip=FALSE,selftest=FALSE;int count;LPWSTR *args=CommandLineToArgvW(GetCommandLineW(),&count);
    for (int i=1;i<count;i++) {
        if (!wcscmp(args[i],L"--skip-launcher")) skip=TRUE;
        else if (!wcscmp(args[i],L"--self-test")) selftest=TRUE;
        else { log_message(L"Unknown launcher argument.");LocalFree(args);return 2; }
    }
    LocalFree(args);
    if (selftest) {
        WCHAR value[4096];GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,4096,ini);
        log_message(L"SELFTEST: native launcher initialized; no game or server started.");log_message(ini);log_message(value);return 0;
    }
    if (skip) {
        if ((!external_only()&&!start_server())||!launch_game()) {log_message(message);return 1;}
        return 0;
    }
    WNDCLASSW cls;ZeroMemory(&cls,sizeof(cls));cls.lpfnWndProc=procedure;cls.hInstance=instance;cls.lpszClassName=L"ReRiffNativeLauncher";
    cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    RegisterClassW(&cls);
    HWND handle=CreateWindowExW(0,cls.lpszClassName,L"ReRiff Launcher v" RERIFF_WIDE_LITERAL(RERIFF_VERSION),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,660,545,NULL,NULL,instance,NULL);
    if (!handle) return 1;
    ShowWindow(handle,show);UpdateWindow(handle);
    MSG msg;while (GetMessageW(&msg,NULL,0,0)>0) {if (!IsDialogMessageW(handle,&msg)) {TranslateMessage(&msg);DispatchMessageW(&msg);}}
    return (int)msg.wParam;
}
