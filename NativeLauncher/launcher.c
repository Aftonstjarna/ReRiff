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
enum { EXTERNAL=101, FALLBACK, TIMEOUT, POPUP, SAVE, PLAY, LOCAL, EXTERNAL_ONLY, STOP, UPDATE, EXPERIMENTAL, NETWORKING };
static WCHAR folder[CAP], ini[CAP], message[1024];
static WCHAR game_arguments[CAP];
static HWND window, status, external, fallback, timeout_edit, popup, play_button, save_button;
static HWND only_checkbox, stop_button, update_button;
static BOOL busy = FALSE, playing = FALSE;
static HWND experimental_checkbox;
static HWND networking_checkbox;
static ULONGLONG settings_signature;
static BOOL settings_observed=FALSE;
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

static BOOL append_command_character(WCHAR *out,size_t *used,WCHAR value) {
    if(*used>=CAP-1)return FALSE;
    out[(*used)++]=value;out[*used]=0;return TRUE;
}

/* Windows command-line quoting: backslashes before quotes (including the
 * closing quote) must be doubled so the game's argv retains the input. */
static BOOL append_game_argument(const WCHAR *argument) {
    size_t used=wcslen(game_arguments);
    if(used&&!append_command_character(game_arguments,&used,L' '))return FALSE;
    if(!append_command_character(game_arguments,&used,L'"'))return FALSE;
    const WCHAR *cursor=argument;
    for(;;){
        size_t slashes=0;while(*cursor==L'\\'){slashes++;cursor++;}
        size_t copies=(*cursor==L'"'||!*cursor)?slashes*2:slashes;
        for(size_t i=0;i<copies;i++)if(!append_command_character(game_arguments,&used,L'\\'))return FALSE;
        if(!*cursor)break;
        if(*cursor==L'"'&&!append_command_character(game_arguments,&used,L'\\'))return FALSE;
        if(!append_command_character(game_arguments,&used,*cursor++))return FALSE;
    }
    return append_command_character(game_arguments,&used,L'"');
}

static BOOL is_forwarded_game(const WCHAR *argument) {
    WCHAR expected[CAP],resolved[CAP];path(expected,L"Game.exe");
    if(!_wcsicmp(argument,L"Game.exe"))return TRUE;
    DWORD length=GetFullPathNameW(argument,CAP,resolved,NULL);
    return length&&length<CAP&&!_wcsicmp(expected,resolved);
}

static BOOL parse_launcher_arguments(int count,LPWSTR *args,BOOL *skip,BOOL *selftest) {
    *skip=FALSE;*selftest=FALSE;game_arguments[0]=0;
    for(int i=1;i<count;i++){
        if(!wcscmp(args[i],L"--skip-launcher")||!wcscmp(args[i],L"--skip"))*skip=TRUE;
        else if(!wcscmp(args[i],L"--self-test"))*selftest=TRUE;
        else {
            if(!wcscmp(args[i],L"--")&&++i>=count)return FALSE;
            if(!is_forwarded_game(args[i]))return FALSE;
            for(i++;i<count;i++)if(!append_game_argument(args[i]))return FALSE;
            return TRUE;
        }
    }
    return TRUE;
}

static BOOL game_command(WCHAR *command) {
    WCHAR game[CAP];path(game,L"Game.exe");
    int length=_snwprintf(command,CAP,L"\"%ls\"%ls%ls",game,*game_arguments?L" ":L"",game_arguments);
    command[CAP-1]=0;
    return length>=0&&length<CAP;
}

static BOOL external_only(void) {
    WCHAR value[32];GetPrivateProfileStringW(L"server",L"external_only",L"false",value,32,ini);
    return !_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes");
}

static BOOL needs_local_server(void) {
    WCHAR address[4096],host[256],enabled[32];
    WritePrivateProfileStringW(NULL,NULL,NULL,ini);
    GetPrivateProfileStringW(L"server",L"networking_enabled",L"true",enabled,32,ini);
    if(!_wcsicmp(enabled,L"false") || !_wcsicmp(enabled,L"no") || !wcscmp(enabled,L"0"))return FALSE;
    if(external_only())return FALSE;
    GetPrivateProfileStringW(L"server",L"fallback_url",L"",address,4096,ini);
    URL_COMPONENTSW parts;ZeroMemory(&parts,sizeof(parts));parts.dwStructSize=sizeof(parts);
    parts.lpszHostName=host;parts.dwHostNameLength=256;
    return *address && InternetCrackUrlW(address,0,0,&parts)
        && parts.nScheme==INTERNET_SCHEME_HTTP && parts.nPort==5144
        && (!_wcsicmp(host,L"localhost") || !wcscmp(host,L"127.0.0.1"));
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
    WCHAR command[CAP]; PROCESS_INFORMATION process;
    if(!game_command(command)){wcscpy(message,L"The forwarded game command is too long.");return FALSE;}
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
    BOOL local=needs_local_server();
    BOOL ok=action==3?check_updates():action==2?stop_server():(local?start_server():TRUE);
    if (ok && play) ok=launch_game();
    if (ok && action<2) wcscpy(message,!local?(play?L"Game closed. No local backend was needed.":L"Saved settings do not require local server startup."):(play?L"Game closed. Local server remains running.":L"Local server is ready."));
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
    BOOL enabled=SendMessageW(networking_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
    if ((enabled&&(!valid_url(a)||(!only&&!valid_url(b))))||!*t||*end||!(seconds>0 && seconds<=60)) {
        MessageBoxW(window,L"Use HTTP(S) URLs and a timeout greater than 0 and at most 60 seconds.",L"Invalid settings",MB_OK|MB_ICONWARNING);return FALSE;
    }
    BOOL ok=WritePrivateProfileStringW(L"server",L"external_url",a,ini)
        && WritePrivateProfileStringW(L"server",L"fallback_url",b,ini)
        && WritePrivateProfileStringW(L"server",L"timeout_seconds",t,ini)
        && WritePrivateProfileStringW(L"server",L"external_only",only?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"server",L"networking_enabled",enabled?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"server",L"show_server_popup",SendMessageW(popup,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini);
    if (!ok) MessageBoxW(window,L"Could not save server.ini.",L"ReRiff",MB_OK|MB_ICONERROR);
    else SetWindowTextW(status,L"Settings saved. They apply on the next game start.");
    return ok;
}

/* Watch content rather than timestamps: the game replaces the INI atomically. */
static BOOL read_settings_signature(ULONGLONG *signature) {
    HANDLE file=CreateFileW(ini,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE){
        if(GetLastError()==ERROR_FILE_NOT_FOUND){*signature=0;return TRUE;}
        return FALSE;
    }
    BYTE block[4096];DWORD count;ULONGLONG hash=14695981039346656037ULL;BOOL ok=TRUE;
    for(;;){
        if(!ReadFile(file,block,sizeof(block),&count,NULL)){ok=FALSE;break;}
        if(!count)break;
        for(DWORD i=0;i<count;i++){hash^=block[i];hash*=1099511628211ULL;}
    }
    CloseHandle(file);if(ok)*signature=hash;return ok;
}

static void update_networking_controls(void) {
    BOOL enabled=SendMessageW(networking_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
    EnableWindow(external,enabled);EnableWindow(only_checkbox,enabled);
    EnableWindow(fallback,enabled&&SendMessageW(only_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED);
}

static void reload_settings_controls(void) {
    WCHAR value[4096];
    settings_observed=read_settings_signature(&settings_signature);
    /* Another process (the game menu) can atomically replace this file. */
    WritePrivateProfileStringW(NULL,NULL,NULL,ini);
    GetPrivateProfileStringW(L"server",L"external_url",L"",value,4096,ini);SetWindowTextW(external,value);
    GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,4096,ini);SetWindowTextW(fallback,value);
    GetPrivateProfileStringW(L"server",L"timeout_seconds",L"5",value,4096,ini);SetWindowTextW(timeout_edit,value);
    GetPrivateProfileStringW(L"server",L"show_server_popup",L"true",value,4096,ini);
    SendMessageW(popup,BM_SETCHECK,(_wcsicmp(value,L"false")&&_wcsicmp(value,L"no")&&wcscmp(value,L"0"))?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(only_checkbox,BM_SETCHECK,external_only()?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"server",L"networking_enabled",L"true",value,4096,ini);
    SendMessageW(networking_checkbox,BM_SETCHECK,(_wcsicmp(value,L"false")&&_wcsicmp(value,L"no")&&wcscmp(value,L"0"))?BST_CHECKED:BST_UNCHECKED,0);
    update_networking_controls();
}

static void refresh_changed_settings(void) {
    ULONGLONG current;
    if(networking_checkbox&&read_settings_signature(&current)&&(!settings_observed||current!=settings_signature)){
        reload_settings_controls();
        if(!busy)SetWindowTextW(status,L"Saved server settings changed. The launcher has been updated.");
    }
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
        networking_checkbox=control(L"BUTTON",L"Enable networking (keep saved addresses when disabled)",BS_AUTOCHECKBOX|WS_TABSTOP,22,259,600,28,NETWORKING);
        control(L"STATIC",L"Stop local server affects this installation's Python backend, not Docker.",0,22,296,600,24,0);
        status=control(L"STATIC",L"Checking server settings...",0,22,323,600,62,0);
        control(L"BUTTON",L"Use local server",WS_TABSTOP,22,409,145,34,LOCAL);
        stop_button=control(L"BUTTON",L"Stop local server",WS_TABSTOP,180,409,150,34,STOP);
        save_button=control(L"BUTTON",L"Save settings",WS_TABSTOP,345,409,130,34,SAVE);
        play_button=control(L"BUTTON",L"Play",WS_TABSTOP|BS_DEFPUSHBUTTON,490,409,130,34,PLAY);
        update_button=control(L"BUTTON",L"Update from GitHub",WS_TABSTOP,22,453,155,30,UPDATE);
        experimental_checkbox=control(L"BUTTON",L"Use experimental builds",BS_AUTOCHECKBOX|WS_TABSTOP,195,453,420,30,EXPERIMENTAL);
        SendMessageW(experimental_checkbox,BM_SETCHECK,installed_experimental?BST_CHECKED:BST_UNCHECKED,0);
        channel_title();
        reload_settings_controls();
        SetTimer(hwnd,1,500,NULL);
        begin(FALSE);return 0;
    }
    if (event==WM_COMMAND) {
        switch (LOWORD(wp)) {
            case LOCAL:SendMessageW(networking_checkbox,BM_SETCHECK,BST_CHECKED,0);SendMessageW(only_checkbox,BM_SETCHECK,BST_UNCHECKED,0);update_networking_controls();SetWindowTextW(fallback,L"http://127.0.0.1:5144");break;
            case NETWORKING:case EXTERNAL_ONLY:update_networking_controls();break;
            case STOP:begin(2);break;
            case EXPERIMENTAL:
                requested_experimental=SendMessageW(experimental_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
                begin(3);break;
            case UPDATE:begin(3);break;
            case SAVE:if(save())settings_observed=read_settings_signature(&settings_signature);break;
            case PLAY:begin(TRUE);break;
        }return 0;
    }
    if(event==WM_TIMER&&wp==1){refresh_changed_settings();return 0;}
    if(event==WM_ACTIVATE&&LOWORD(wp)!=WA_INACTIVE)refresh_changed_settings();
    if (event==WM_GAME_RUNNING) { SetWindowTextW(status,L"Game running. Keep the launcher open until the game closes.");ShowWindow(window,SW_MINIMIZE);return 0; }
    if (event==WM_COMPLETE) {
        if (lp==2) { DestroyWindow(hwnd);return 0; }
        if (lp==1) reload_settings_controls();
        requested_experimental=installed_experimental;
        SendMessageW(experimental_checkbox,BM_SETCHECK,installed_experimental?BST_CHECKED:BST_UNCHECKED,0);
        busy=FALSE;playing=FALSE;EnableWindow(play_button,TRUE);EnableWindow(save_button,TRUE);EnableWindow(stop_button,TRUE);EnableWindow(update_button,TRUE);EnableWindow(experimental_checkbox,TRUE);SetWindowTextW(status,message);
        if (lp) ShowWindow(window,SW_RESTORE);return 0;
    }
    if (event==WM_CLOSE) {
        if (busy) { SetWindowTextW(status,L"Wait for server startup or close the game before closing this launcher.");return 0; }
        DestroyWindow(hwnd);return 0;
    }
    if (event==WM_DESTROY) { KillTimer(hwnd,1);DeleteObject(font);PostQuitMessage(0);return 0; }
    return DefWindowProcW(hwnd,event,wp,lp);
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR command,int show) {
    (void)previous;(void)command;
    GetModuleFileNameW(NULL,folder,CAP);WCHAR *slash=wcsrchr(folder,L'\\');if (!slash) return 1;*slash=0;
    path(ini,L"server.ini");load_channel();
    BOOL skip=FALSE,selftest=FALSE;int count;LPWSTR *args=CommandLineToArgvW(GetCommandLineW(),&count);
    if(!args||!parse_launcher_arguments(count,args,&skip,&selftest)) {
        log_message(L"Unknown launcher argument. Steam may forward only the Game.exe beside this launcher and its arguments.");
        if(args)LocalFree(args);return 2;
    }
    LocalFree(args);
    if (selftest) {
        WCHAR value[4096];GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,4096,ini);
        log_message(L"SELFTEST: native launcher initialized; no game or server started.");log_message(ini);log_message(value);return 0;
    }
    if (skip) {
        if ((needs_local_server()&&!start_server())||!launch_game()) {log_message(message);return 1;}
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
