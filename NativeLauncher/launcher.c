#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <wininet.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

#define CAP 32768
#ifndef RERIFF_VERSION
#define RERIFF_VERSION "0.1.0"
#endif
#ifndef RERIFF_DEVELOPMENT_BUILD
#define RERIFF_DEVELOPMENT_BUILD 0
#endif
#define RERIFF_WIDE_LITERAL_(value) L##value
#define RERIFF_WIDE_LITERAL(value) RERIFF_WIDE_LITERAL_(value)
#ifndef RERIFF_HEALTH_URL
#define RERIFF_HEALTH_URL L"http://127.0.0.1:5144/"
#endif
#include "logging.h"
#include "error_dialog.h"
#define WM_COMPLETE (WM_APP + 1)
#define WM_GAME_RUNNING (WM_APP + 2)
enum { EXTERNAL=101, FALLBACK, TIMEOUT, POPUP, SAVE, PLAY, LOCAL, EXTERNAL_ONLY, STOP, UPDATE, EXPERIMENTAL, NETWORKING, SKIP_LAUNCHER, AUTO_UPDATE_SKIP, CLOSE_LAUNCHER, CJK_INSTALL, LOGGING, LATIN_INSTALL, UPDATE_DETAILS, LASTFM_ENABLED, LASTFM_PERCENT, LASTFM_LINK, LASTFM_DISCONNECT, LASTFM_SAVE, LASTFM_KEY, LASTFM_SECRET, UNINSTALL, LISTENBRAINZ_TOKEN, LISTENBRAINZ_ENABLED, LISTENBRAINZ_PERCENT, LISTENBRAINZ_SAVE, LISTENBRAINZ_GET_TOKEN, LIBRE_ENABLED, LIBRE_PERCENT, LIBRE_LINK, LIBRE_DISCONNECT, LIBRE_SAVE, LIBRE_CREDENTIAL, GRAPHICS_SAVE, CONTROLS_SAVE, CONTROLS_TRACE, CONTROLS_DEFAULTS, LOG_CATEGORY_BASE=500, CONTROLLER_CAPTURE_BASE=600, KEYBOARD_CAPTURE_BASE=700 };
#include "branding.h"

static WCHAR folder[CAP], ini[CAP], message[1024];
static WCHAR game_arguments[CAP];
static BOOL force_show_launcher, suppress_auto_update, automatic_update, force_auto_reinstall;
static DWORD restart_parent_pid;
#include "restart_protocol.h"
static HWND window, status, external, fallback, timeout_edit, popup, play_button, save_button;
static HWND only_checkbox, stop_button, update_button, launcher_progress, status_separator, footer_separator;
static BOOL busy = FALSE, playing = FALSE, checking_update = FALSE, exit_after_game = FALSE;
static HWND experimental_checkbox;
static HWND networking_checkbox, skip_checkbox, auto_update_checkbox, close_checkbox;
static ULONGLONG settings_signature;
static BOOL settings_observed=FALSE;
static BOOL installed_experimental=FALSE, requested_experimental=FALSE;
static HFONT font,restart_font;
enum { PAGE_SERVERS, PAGE_GRAPHICS, PAGE_CONTROLS, PAGE_LANGUAGES, PAGE_SCROBBLING, PAGE_DIAGNOSTICS, PAGE_COUNT };
static HWND scrobbling_tabs,launcher_tabs,controls_tabs,latin_checkbox,language_checkbox,checkbox_tooltips,logging_checkbox;
enum { CONTROLS_CONTROLLER, CONTROLS_KEYBOARD, CONTROLS_HOTKEYS, CONTROLS_PART_COUNT };
static const WCHAR *log_categories[]={L"launcher",L"connections",L"game",L"text",L"languages",L"localserver",L"update",L"installer",L"python"};
static const WCHAR *log_labels[]={L"Launcher",L"Connections, network and news",L"Menus, progression and saves",L"Unicode and song metadata",L"Font installation",L"Local server and helper",L"Updates",L"Installer",L"Python setup"};
#define LOG_CATEGORY_COUNT (sizeof(log_categories)/sizeof(log_categories[0]))
static HWND log_checkboxes[LOG_CATEGORY_COUNT];
static HWND diagnostic_controls[24];static unsigned diagnostic_count;static BOOL creating_diagnostics;
static HWND server_page_controls[40],language_page_controls[8];
static unsigned server_page_count,language_page_count;
static HWND graphics_controls[16];static unsigned graphics_count;static BOOL creating_graphics;
static HWND controls_page_controls[CONTROLS_PART_COUNT][48];static unsigned controls_page_count[CONTROLS_PART_COUNT];static int controls_part=CONTROLS_HOTKEYS;static BOOL creating_controls;
static BOOL creating_languages,changing_language,creating_lastfm,creating_listenbrainz,creating_libre;
static HWND libre_controls_group[12];static unsigned libre_count;
static HWND listenbrainz_controls[12];static unsigned listenbrainz_count;
static HWND lastfm_controls[16];static unsigned lastfm_count;
static void language_controls(void);

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

static const WCHAR *launcher_title(void) {
#if RERIFF_DEVELOPMENT_BUILD
    return L"ReRiff Launcher — development build";
#else
    return installed_experimental
        ? L"ReRiff Launcher v" RERIFF_WIDE_LITERAL(RERIFF_VERSION) L" — Experimental"
        : L"ReRiff Launcher v" RERIFF_WIDE_LITERAL(RERIFF_VERSION);
#endif
}
static void channel_title(void) {
    SetWindowTextW(window,launcher_title());
}

static void log_message(const WCHAR *text) {
    if(!rr_log_enabled(folder,L"launcher",strstr(RERIFF_VERSION,"-")!=NULL))return;
    WCHAR name[CAP]; char utf8[4096];
    path(name,L"logs");CreateDirectoryW(name,NULL);
    path(name, L"logs\\ReRiff-launcher.log");
    int bytes = WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, sizeof(utf8)-3, NULL, NULL);
    if (!bytes) return;
    rr_log_append(name,"launcher",utf8);
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

#include "python_runtime.h"

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
    *skip=FALSE;*selftest=FALSE;force_show_launcher=FALSE;game_arguments[0]=0;
    restart_parent_pid=0;suppress_auto_update=FALSE;force_auto_reinstall=FALSE;
    for(int i=1;i<count;i++){
        if(!wcscmp(args[i],L"--skip-launcher")||!wcscmp(args[i],L"--launcher-skip")||!wcscmp(args[i],L"--skip")){
            *skip=TRUE;force_show_launcher=FALSE;
        }
        else if(!wcscmp(args[i],L"--show-launcher")){*skip=FALSE;force_show_launcher=TRUE;}
        else if(!wcscmp(args[i],L"--test-auto-update")){force_auto_reinstall=TRUE;*skip=TRUE;force_show_launcher=FALSE;}
        else if(!wcscmp(args[i],L"--no-auto-update"))suppress_auto_update=TRUE;
        else if(!wcscmp(args[i],L"--self-test"))*selftest=TRUE;
        else if(!wcscmp(args[i],L"--restart-game")){
            if(restart_parent_pid || ++i>=count || args[i][0]<L'1' || args[i][0]>L'9')return FALSE;
            WCHAR *end;unsigned long pid=wcstoul(args[i],&end,10);
            if(*end || !pid || pid==GetCurrentProcessId())return FALSE;
            restart_parent_pid=(DWORD)pid;
        }
        else {
            if(!wcscmp(args[i],L"--")&&++i>=count)return FALSE;
            if(!is_forwarded_game(args[i]))return FALSE;
            for(i++;i<count;i++){
                /* Steam commonly appends launch options after %command%.
                 * These updater-only flags must not reach the original EXE. */
                if(!wcscmp(args[i],L"--test-auto-update")){force_auto_reinstall=TRUE;*skip=TRUE;force_show_launcher=FALSE;}
                else if(!wcscmp(args[i],L"--no-auto-update"))suppress_auto_update=TRUE;
                else if(!append_game_argument(args[i]))return FALSE;
            }
            return TRUE;
        }
    }
    return TRUE;
}

static BOOL launcher_preference(const WCHAR *key) {
    WCHAR value[32];GetPrivateProfileStringW(L"launcher",key,L"false",value,32,ini);
    return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
}
static BOOL should_skip_launcher(BOOL requested) {
    WCHAR value[32];
    GetPrivateProfileStringW(L"launcher",L"skip_launcher",L"false",value,32,ini);
    return !force_show_launcher&&(requested||!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1"));
}

static BOOL game_command(WCHAR *command) {
    WCHAR game[CAP];path(game,L"Game.exe");
    int length=_snwprintf(command,CAP,L"\"%ls\"%ls%ls",game,*game_arguments?L" ":L"",game_arguments);
    command[CAP-1]=0;
    return length>=0&&length<CAP;
}

/* Used only when Game.exe was started without a supervising launcher. The
 * game waits for our ready acknowledgement before posting its normal close. */
static int wait_for_restart_parent(void) {
    WCHAR expected[CAP],actual[CAP],name[96];DWORD length=CAP;
    HANDLE parent=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,restart_parent_pid);
    if(!parent){wcscpy(message,L"Could not open the game requesting restart.");return -1;}
    path(expected,L"Game.exe");
    if(!QueryFullProcessImageNameW(parent,0,actual,&length)||_wcsicmp(expected,actual)){
        CloseHandle(parent);wcscpy(message,L"Restart refused: process is not this installation's Game.exe.");return -1;
    }
    restart_event_name(name,96,restart_parent_pid,FALSE);
    HANDLE request=OpenEventW(SYNCHRONIZE,FALSE,name);
    restart_event_name(name,96,restart_parent_pid,TRUE);
    HANDLE ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
    if(!request||!ready||!SetEvent(ready)){
        if(request)CloseHandle(request);if(ready)CloseHandle(ready);CloseHandle(parent);
        wcscpy(message,L"Restart refused: game handoff is missing.");return -1;
    }
    CloseHandle(ready);
    BOOL closed=WaitForSingleObject(parent,INFINITE)==WAIT_OBJECT_0;
    BOOL committed=WaitForSingleObject(request,0)==WAIT_OBJECT_0;
    CloseHandle(request);CloseHandle(parent);
    return closed&&committed?1:0;
}

static BOOL external_only(void) {
    WCHAR value[32];GetPrivateProfileStringW(L"server",L"external_only",L"false",value,32,ini);
    return !_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes");
}

#include "update_progress.h"
#include "update_details.h"
#include "auto_update.h"

static BOOL local_server_action(const WCHAR *action);
static BOOL stop_server(void) {
    if(!is_wine()){WCHAR native[CAP];path(native,L"localserver\\native\\config.json");if(GetFileAttributesW(native)!=INVALID_FILE_ATTRIBUTES)return local_server_action(L"stop");}
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

#include "local_server.h"
#include "song_sync.h"
#include "languages.h"
#include "uninstall.h"

static void finish_local_session(BOOL started){
    if(!started)return;
    WCHAR previous[1024];wcscpy(previous,message);
    local_server_action(L"stop");log_message(message);wcscpy(message,previous);
}

static BOOL launch_game(void) {
    ULONGLONG launch_started=GetTickCount64();
    BOOL started_local=FALSE;
    if(!external_only()){
        WCHAR config[CAP],enabled[16],primary[2048],secondary[2048];
        path(config,L"localserver\\native\\config.json");
        GetPrivateProfileStringW(L"server",L"networking_enabled",L"true",enabled,16,ini);
        GetPrivateProfileStringW(L"server",L"external_url",L"",primary,2048,ini);
        GetPrivateProfileStringW(L"server",L"fallback_url",L"",secondary,2048,ini);
        BOOL use_network=_wcsicmp(enabled,L"false")&&wcscmp(enabled,L"0")&&_wcsicmp(enabled,L"no");
        /* Only this installation's explicitly configured loopback backend is
         * auto-started; downloading remains an explicit Install action. */
        if(use_network&&GetFileAttributesW(config)!=INVALID_FILE_ATTRIBUTES){
            if(local_server_action(L"status")&&local_server_state==1
               &&(!_wcsicmp(primary,local_server_address)||!_wcsicmp(secondary,local_server_address))){
                started_local=local_server_action(L"start");log_message(message);
            }
        }
    }
    update_timing(L"local-server readiness",launch_started);
    for(;;){
    WCHAR cache_key[160],cache_base[CAP],cache_id[256];BOOL multiplayer=FALSE;
    if(!song_cache_identity(ini,cache_key,sizeof(cache_key)/sizeof(cache_key[0]),cache_base,cache_id,&multiplayer)){
        wcscpy(cache_key,L"singleplayer");cache_base[0]=0;multiplayer=FALSE;
    }
    WCHAR cache_note[512];
    if(multiplayer)_snwprintf(cache_note,512,L"Song cache: %ls; server=%ls",cache_key,cache_base);
    else _snwprintf(cache_note,512,L"Song cache: offline singleplayer; local only");
    cache_note[511]=0;log_message(cache_note);
    if(!song_cache_prepare(cache_key))log_message(L"Song cache: selected cache could not be prepared; using current cache.");
    if(multiplayer)sync_cached_songs(ini,cache_base);
    WCHAR command[CAP]; PROCESS_INFORMATION process;
    if(!game_command(command)){wcscpy(message,L"The forwarded game command is too long.");finish_local_session(started_local);return FALSE;}
    if (!spawn(command,FALSE,&process)) { wcscpy(message,L"Could not start Game.exe beside the launcher."); finish_local_session(started_local);return FALSE; }
    update_timing(L"launch to Game.exe spawn",launch_started);
    WCHAR event_name[96];restart_event_name(event_name,96,process.dwProcessId,FALSE);
    HANDLE restart=CreateEventW(NULL,TRUE,FALSE,event_name);
    if (window) PostMessageW(window,WM_GAME_RUNNING,0,0);
    CloseHandle(process.hThread);
    BOOL closed=WaitForSingleObject(process.hProcess,INFINITE)==WAIT_OBJECT_0;
    DWORD game_exit;
    if(closed&&GetExitCodeProcess(process.hProcess,&game_exit)){
        WCHAR detail[128];_snwprintf(detail,128,L"Game process %lu exited with code 0x%08lX.",(unsigned long)process.dwProcessId,(unsigned long)game_exit);detail[127]=0;log_message(detail);
    }
    BOOL again=restart&&WaitForSingleObject(restart,0)==WAIT_OBJECT_0;
    if(restart)CloseHandle(restart);
    CloseHandle(process.hProcess);
    if(!song_cache_commit(cache_key))log_message(L"Song cache: could not commit the selected cache.");
    if(multiplayer)sync_cached_songs(ini,cache_base);
    if(!closed){wcscpy(message,L"Could not wait for Game.exe to close.");finish_local_session(started_local);return FALSE;}
    if(!again){finish_local_session(started_local);return TRUE;}
    launch_started=GetTickCount64();
    log_message(L"Game closed for Save and restart; applying saved server settings.");
    }
}

#include "lastfm.h"
#include "listenbrainz.h"
#include "librefm.h"

static DWORD WINAPI worker(LPVOID parameter) {
    int action=(int)(INT_PTR)parameter;
    BOOL play=action==1;
    BOOL ok=action==11||action==12?libre_request(action==12):action==10?uninstall_action():action==8||action==9?lfm_request(action==9):action==6?language_action(L"install --pack latin"):action==7?language_action(L"disable --pack latin"):action==4?language_action(L"install"):action==5?language_action(L"disable"):action==3?check_updates():action==2?local_server_action(local_server_state==2?L"stop":local_server_state==1&&local_server_package_current()?L"start":L"install"):action==0?local_server_action(L"status"):TRUE;
    if (ok && play) ok=launch_game();
    if (ok && action<2) wcscpy(message,play?L"Game closed.":L"Ready. Singleplayer runs inside the game; configured servers are checked at game startup.");
    log_message(message);
    PostMessageW(window,WM_COMPLETE,ok,action==10&&ok?3:update_handoff?2:play);
    return 0;
}

static void begin(int action) {
    if (busy) return;
    progress_last=-999;progress_set_bar(launcher_progress,-1);
    busy=TRUE;libre_action=action==11?8:action==12?9:0;lfm_action=action==8||action==9?action:0;lfm_controls();lb_controls();libre_controls();playing=action==1;checking_update=action==3;changing_language=action>=4&&action<=7;
    EnableWindow(language_checkbox,FALSE);EnableWindow(latin_checkbox,FALSE);EnableWindow(logging_checkbox,FALSE);
    EnableWindow(play_button,FALSE);EnableWindow(save_button,FALSE);EnableWindow(stop_button,FALSE);EnableWindow(update_button,FALSE);EnableWindow(experimental_checkbox,FALSE);
    SetWindowTextW(status,action==10?L"Removing ReRiff; preserving user data and settings…":action>=8?L"Contacting Last.fm…":action==6?L"Installing and enabling Latin/European fonts...":action==7?L"Disabling Latin/European fonts...":action==4?L"Installing and enabling CJK fonts...":action==5?L"Disabling CJK fonts...":action==3?L"Checking published GitHub releases...":action==2?(local_server_state==2?L"Stopping the local server...":local_server_state==1&&local_server_package_current()?L"Starting the local server...":L"Downloading and installing the optional local server..."):L"Checking server settings...");
    HANDLE thread=CreateThread(NULL,4*1024*1024,worker,(LPVOID)(INT_PTR)action,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
    if (thread) CloseHandle(thread);
    else { progress_set_bar(launcher_progress,0);busy=FALSE;lfm_controls();lb_controls();libre_controls(); EnableWindow(play_button,TRUE);EnableWindow(save_button,TRUE);EnableWindow(stop_button,TRUE);EnableWindow(update_button,TRUE);EnableWindow(experimental_checkbox,TRUE);SetWindowTextW(status,L"Could not start launcher worker."); }
}

static HWND control(const WCHAR *cls,const WCHAR *text,DWORD style,int x,int y,int w,int h,int id) {
    HWND handle=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y+34+((creating_lastfm||creating_listenbrainz||creating_libre)?24:0)+(creating_controls?18:0),w,h,window,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    /* Use the standard edit border under Wine. Its themed focus decoration
     * can remain visible above unrelated windows after the launcher loses focus.
     * Keep the native EDIT control, keyboard focus and text handling. */
    if(handle&&is_wine()&&!_wcsicmp(cls,L"EDIT")){
        HMODULE theme=LoadLibraryExW(L"uxtheme.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(theme){
            typedef HRESULT (WINAPI *ThemeFn)(HWND,LPCWSTR,LPCWSTR);
            ThemeFn set_theme=(ThemeFn)GetProcAddress(theme,"SetWindowTheme");
            if(set_theme)set_theme(handle,L"",L"");
            FreeLibrary(theme);
        }
    }
    if(handle){if(creating_graphics){if(graphics_count<16)graphics_controls[graphics_count++]=handle;}else if(creating_controls){if(controls_page_count[controls_part]<48)controls_page_controls[controls_part][controls_page_count[controls_part]++]=handle;}else if(creating_libre){if(libre_count<12)libre_controls_group[libre_count++]=handle;}else if(creating_listenbrainz){if(listenbrainz_count<12)listenbrainz_controls[listenbrainz_count++]=handle;}else if(creating_lastfm){if(lastfm_count<16)lastfm_controls[lastfm_count++]=handle;}else if(creating_diagnostics){if(diagnostic_count<24)diagnostic_controls[diagnostic_count++]=handle;}else if(creating_languages){if(language_page_count<8)language_page_controls[language_page_count++]=handle;}else if(server_page_count<40)server_page_controls[server_page_count++]=handle;}
    SendMessageW(handle,WM_SETFONT,(WPARAM)font,TRUE);return handle;
}

static HWND restart_note(const WCHAR *text,int x,int y,int width){
    if(!restart_font)restart_font=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    HWND note=control(L"STATIC",text,0,x,y,width,20,0);
    SendMessageW(note,WM_SETFONT,(WPARAM)restart_font,TRUE);return note;
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
    BOOL enabled=SendMessageW(networking_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED;
    if ((enabled&&(!valid_url(a)||(!only&&!valid_url(b))))||!*t||*end||!(seconds>0 && seconds<=60)) {
        rr_show_error(window,L"Invalid settings",L"Use HTTP(S) URLs and a timeout greater than 0 and at most 60 seconds.");return FALSE;
    }
    BOOL ok=WritePrivateProfileStringW(L"server",L"external_url",a,ini)
        && WritePrivateProfileStringW(L"server",L"fallback_url",b,ini)
        && WritePrivateProfileStringW(L"server",L"timeout_seconds",t,ini)
        && WritePrivateProfileStringW(L"server",L"external_only",only?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"server",L"networking_enabled",enabled?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"server",L"show_server_popup",SendMessageW(popup,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"launcher",L"skip_launcher",SendMessageW(skip_checkbox,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"launcher",L"auto_update_on_skip",SendMessageW(auto_update_checkbox,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini)
        && WritePrivateProfileStringW(L"launcher",L"close_on_game_start",SendMessageW(close_checkbox,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini);
    if (!ok) rr_show_error(window,L"ReRiff",L"Could not save server.ini.");
    else SetWindowTextW(status,L"Settings saved. They apply on the next game start.");
    return ok;
}

/* Watch content rather than timestamps: the game replaces the INI atomically. */
static BOOL read_settings_signature(ULONGLONG *signature) {
    ULONGLONG hash=14695981039346656037ULL;
    const WCHAR *names[]={L"server.ini",L"client.ini",L"singleplayer.ini"};
    for(unsigned n=0;n<3;n++) {
        WCHAR name[CAP];path(name,names[n]);
        HANDLE file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file==INVALID_HANDLE_VALUE){if(GetLastError()==ERROR_FILE_NOT_FOUND)continue;return FALSE;}
        BYTE block[4096];DWORD count;BOOL ok=TRUE;hash^=n+1;
        for(;;){if(!ReadFile(file,block,sizeof(block),&count,NULL)){ok=FALSE;break;}if(!count)break;for(DWORD i=0;i<count;i++){hash^=block[i];hash*=1099511628211ULL;}}
        CloseHandle(file);if(!ok)return FALSE;
    }
    *signature=hash;return TRUE;
}

static void update_networking_controls(void) {
    BOOL enabled=SendMessageW(networking_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED;
    EnableWindow(external,enabled);EnableWindow(only_checkbox,enabled);
    EnableWindow(fallback,enabled&&SendMessageW(only_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED);
}

static void reload_log_controls(void) {
    BOOL enabled=rr_logging_enabled(folder,installed_experimental);
    if(logging_checkbox)SendMessageW(logging_checkbox,BM_SETCHECK,enabled?BST_CHECKED:BST_UNCHECKED,0);
    for(unsigned i=0;i<LOG_CATEGORY_COUNT;i++)if(log_checkboxes[i]) {
        SendMessageW(log_checkboxes[i],BM_SETCHECK,rr_log_selected(folder,log_categories[i])?BST_CHECKED:BST_UNCHECKED,0);
        EnableWindow(log_checkboxes[i],enabled);
    }
}

#include "graphics.h"
#include "controls.h"
#include "gamepad_prompts.h"

static void reload_settings_controls(void) {
    graphics_reload();
    reload_log_controls();
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
    SendMessageW(networking_checkbox,BM_SETCHECK,(_wcsicmp(value,L"false")&&_wcsicmp(value,L"no")&&wcscmp(value,L"0"))?BST_UNCHECKED:BST_CHECKED,0);
    SendMessageW(skip_checkbox,BM_SETCHECK,launcher_preference(L"skip_launcher")?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(auto_update_checkbox,BM_SETCHECK,launcher_preference(L"auto_update_on_skip")?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(close_checkbox,BM_SETCHECK,launcher_preference(L"close_on_game_start")?BST_CHECKED:BST_UNCHECKED,0);
    update_networking_controls();
}

static void refresh_changed_settings(void) {
    if(changing_language)return;
    ULONGLONG current;
    if(networking_checkbox&&read_settings_signature(&current)&&(!settings_observed||current!=settings_signature)){
        reload_settings_controls();
        if(!busy)SetWindowTextW(status,L"Saved server settings changed. The launcher has been updated.");
    }
}

static void language_controls(void){
    WCHAR value[16];WritePrivateProfileStringW(NULL,NULL,NULL,ini);
    GetPrivateProfileStringW(L"languages",L"cjk_enabled",GetPrivateProfileIntW(L"diagnostics",L"cjk_render_probe",0,ini)?L"true":L"false",value,16,ini);
    SendMessageW(language_checkbox,BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"languages",L"latin_enabled",L"false",value,16,ini);
    SendMessageW(latin_checkbox,BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
}
/* Standard Win32 tooltips preserve native checkbox input and tab navigation.
 * Text is backed by static literals for the lifetime of the launcher. */
static void checkbox_help(HWND checkbox,const WCHAR *text) {
    TOOLINFOW tool={0};tool.cbSize=TTTOOLINFOW_V2_SIZE;
    tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;tool.hwnd=window;
    tool.uId=(UINT_PTR)checkbox;tool.lpszText=(LPWSTR)text;
    SendMessageW(checkbox_tooltips,TTM_ADDTOOLW,0,(LPARAM)&tool);
}
static void select_launcher_page(void){
    static int previous_page=-1;
    int page=(int)SendMessageW(launcher_tabs,TCM_GETCURSEL,0,0);BOOL languages=page==PAGE_LANGUAGES;BOOL controls=page==PAGE_CONTROLS;
    BOOL reload_controls=controls&&previous_page!=PAGE_CONTROLS;
    previous_page=page;
    BOOL lastfm=page==PAGE_SCROBBLING&&SendMessageW(scrobbling_tabs,TCM_GETCURSEL,0,0)==0;
    BOOL listenbrainz=page==PAGE_SCROBBLING&&SendMessageW(scrobbling_tabs,TCM_GETCURSEL,0,0)==1;
    BOOL libre=page==PAGE_SCROBBLING&&SendMessageW(scrobbling_tabs,TCM_GETCURSEL,0,0)==2;
    ShowWindow(scrobbling_tabs,page==PAGE_SCROBBLING?SW_SHOW:SW_HIDE);
    ShowWindow(controls_tabs,controls?SW_SHOW:SW_HIDE);
    int cpart=controls?(int)SendMessageW(controls_tabs,TCM_GETCURSEL,0,0):CONTROLS_HOTKEYS;
    if(cpart<0||cpart>=CONTROLS_PART_COUNT)cpart=CONTROLS_HOTKEYS;
    if(!controls||cpart!=CONTROLS_CONTROLLER)controls_leave();
    else controls_key_capture_cancel();
    SendMessageW(checkbox_tooltips,TTM_POP,0,0);
    for(unsigned i=0;i<server_page_count;i++){
        HWND item=server_page_controls[i];
        if(item!=status&&item!=play_button&&item!=close_checkbox&&item!=update_button&&item!=experimental_checkbox&&item!=launcher_progress&&item!=status_separator&&item!=footer_separator)ShowWindow(item,page!=PAGE_SERVERS?SW_HIDE:SW_SHOW);
    }
    for(unsigned i=0;i<graphics_count;i++)ShowWindow(graphics_controls[i],page==PAGE_GRAPHICS?SW_SHOW:SW_HIDE);
    for(int p=0;p<CONTROLS_PART_COUNT;p++)for(unsigned i=0;i<controls_page_count[p];i++)ShowWindow(controls_page_controls[p][i],(controls&&p==cpart)?SW_SHOW:SW_HIDE);
    for(unsigned i=0;i<language_page_count;i++)ShowWindow(language_page_controls[i],languages?SW_SHOW:SW_HIDE);
    for(unsigned i=0;i<diagnostic_count;i++)ShowWindow(diagnostic_controls[i],page==PAGE_DIAGNOSTICS?SW_SHOW:SW_HIDE);
    for(unsigned i=0;i<lastfm_count;i++)ShowWindow(lastfm_controls[i],lastfm?SW_SHOW:SW_HIDE);
    for(unsigned i=0;i<listenbrainz_count;i++)ShowWindow(listenbrainz_controls[i],listenbrainz?SW_SHOW:SW_HIDE);
    for(unsigned i=0;i<libre_count;i++)ShowWindow(libre_controls_group[i],libre?SW_SHOW:SW_HIDE);
    if(page==PAGE_SCROBBLING){SetFocus(scrobbling_tabs);return;}
    if(controls&&reload_controls)controls_reload();
    if(page==PAGE_DIAGNOSTICS)controls_trace_reload();
    if(controls){SetFocus(cpart==CONTROLS_HOTKEYS?controls_first:cpart==CONTROLS_CONTROLLER?controls_controller_bindings[0]:controls_keyboard_first);return;}
    SetFocus(page==PAGE_GRAPHICS?graphics_resolution:page==PAGE_DIAGNOSTICS?logging_checkbox:languages?language_checkbox:external);
}
static LRESULT CALLBACK procedure(HWND hwnd,UINT event,WPARAM wp,LPARAM lp) {
    if(event==WM_ERASEBKGND){RECT area;GetClientRect(hwnd,&area);FillRect((HDC)wp,&area,GetSysColorBrush(COLOR_BTNFACE));return TRUE;}
    if(event==WM_CTLCOLORSTATIC||event==WM_CTLCOLORBTN){SetBkColor((HDC)wp,GetSysColor(COLOR_BTNFACE));return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);}
    if(event==WM_DRAWITEM){
        const DRAWITEMSTRUCT *item=(const DRAWITEMSTRUCT *)lp;
        if(rr_brand_draw(item)||gamepad_prompt_draw(item))return TRUE;
    }
    if (event==WM_CREATE) {
        SetWindowLongPtrW(hwnd,GWL_STYLE,GetWindowLongPtrW(hwnd,GWL_STYLE)|WS_CLIPCHILDREN);
        rr_brand_icon(hwnd);
        window=hwnd;font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        INITCOMMONCONTROLSEX common={sizeof(common),ICC_TAB_CLASSES|ICC_WIN95_CLASSES|ICC_PROGRESS_CLASS|ICC_BAR_CLASSES};InitCommonControlsEx(&common);
        checkbox_tooltips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,NULL,
            WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,
            CW_USEDEFAULT,CW_USEDEFAULT,hwnd,NULL,GetModuleHandleW(NULL),NULL);
        SendMessageW(checkbox_tooltips,TTM_SETMAXTIPWIDTH,0,440);
        SendMessageW(checkbox_tooltips,TTM_SETDELAYTIME,TTDT_AUTOPOP,20000);
        launcher_tabs=CreateWindowExW(0,WC_TABCONTROLW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS,10,6,630,28,hwnd,NULL,GetModuleHandleW(NULL),NULL);
        SendMessageW(launcher_tabs,WM_SETFONT,(WPARAM)font,TRUE);
        TCITEMW tab={0};tab.mask=TCIF_TEXT;tab.pszText=L"Servers";SendMessageW(launcher_tabs,TCM_INSERTITEMW,0,(LPARAM)&tab);
        tab.pszText=L"Graphics";SendMessageW(launcher_tabs,TCM_INSERTITEMW,PAGE_GRAPHICS,(LPARAM)&tab);
        tab.pszText=L"Controls";SendMessageW(launcher_tabs,TCM_INSERTITEMW,PAGE_CONTROLS,(LPARAM)&tab);
        tab.pszText=L"Languages";SendMessageW(launcher_tabs,TCM_INSERTITEMW,PAGE_LANGUAGES,(LPARAM)&tab);
        tab.pszText=L"Scrobbling";SendMessageW(launcher_tabs,TCM_INSERTITEMW,PAGE_SCROBBLING,(LPARAM)&tab);
        tab.pszText=L"Diagnostics";SendMessageW(launcher_tabs,TCM_INSERTITEMW,PAGE_DIAGNOSTICS,(LPARAM)&tab);
        scrobbling_tabs=CreateWindowExW(0,WC_TABCONTROLW,L"",WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS,22,40,600,28,hwnd,NULL,GetModuleHandleW(NULL),NULL);
        SendMessageW(scrobbling_tabs,WM_SETFONT,(WPARAM)font,TRUE);
        tab.pszText=L"Last.fm";SendMessageW(scrobbling_tabs,TCM_INSERTITEMW,0,(LPARAM)&tab);
        tab.pszText=L"ListenBrainz";SendMessageW(scrobbling_tabs,TCM_INSERTITEMW,1,(LPARAM)&tab);
        tab.pszText=L"Libre.fm";SendMessageW(scrobbling_tabs,TCM_INSERTITEMW,2,(LPARAM)&tab);
        control(L"STATIC",L"Riff Racer — Server settings",0,22,18,600,30,0);
        restart_note(L"Server changes require a game restart",300,24,320);
        control(L"STATIC",L"",SS_ETCHEDHORZ,10,52,630,2,0);
        control(L"STATIC",L"Primary server",0,22,66,145,24,0);
        external=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,172,62,448,28,EXTERNAL);
        control(L"STATIC",L"Secondary server",0,22,108,145,24,0);
        fallback=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,172,104,448,28,FALLBACK);
        control(L"STATIC",L"Timeout (seconds)",0,22,150,145,24,0);
        timeout_edit=control(L"EDIT",L"5",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,172,146,95,28,TIMEOUT);
        popup=control(L"BUTTON",L"Show startup notification",BS_AUTOCHECKBOX|WS_TABSTOP,22,191,590,28,POPUP);
        only_checkbox=control(L"BUTTON",L"Primary server only",BS_AUTOCHECKBOX|WS_TABSTOP,22,225,590,28,EXTERNAL_ONLY);
        networking_checkbox=control(L"BUTTON",L"Force singleplayer",BS_AUTOCHECKBOX|WS_TABSTOP,22,259,600,28,NETWORKING);
        skip_checkbox=control(L"BUTTON",L"Skip launcher",BS_AUTOCHECKBOX|WS_TABSTOP,22,293,165,28,SKIP_LAUNCHER);
        auto_update_checkbox=control(L"BUTTON",L"Auto-update when skipping launcher",BS_AUTOCHECKBOX|WS_TABSTOP,195,293,425,28,AUTO_UPDATE_SKIP);
        status_separator=control(L"STATIC",L"",SS_ETCHEDHORZ,10,333,630,2,0);
        status=control(L"STATIC",L"Checking server settings...",0,22,347,600,38,0);
        launcher_progress=control(PROGRESS_CLASSW,L"",0,22,389,600,22,0);
        SendMessageW(launcher_progress,PBM_SETRANGE32,0,100);
        progress_sink=hwnd;
        footer_separator=control(L"STATIC",L"",SS_ETCHEDHORZ,10,435,630,2,0);
        local_server_hint=control(L"STATIC",L"Local server: http://127.0.0.1:5144 — enter in Secondary server",SS_CENTER|SS_CENTERIMAGE,22,437,600,24,0);
        HWND github_button=control(L"BUTTON",L"ReRiff on GitHub",WS_TABSTOP|BS_BITMAP|BS_CENTER|BS_VCENTER,22,461,120,34,RERIFF_BRAND_CONTROL);
        rr_brand_button(github_button);
        stop_button=control(L"BUTTON",L"Install local server",WS_TABSTOP,152,461,130,34,STOP);
        save_button=control(L"BUTTON",L"Save settings",WS_TABSTOP,292,461,110,34,SAVE);
        close_checkbox=control(L"BUTTON",L"Close launcher",BS_AUTOCHECKBOX|WS_TABSTOP,412,461,120,34,CLOSE_LAUNCHER);
        play_button=control(L"BUTTON",L"Play",WS_TABSTOP|BS_DEFPUSHBUTTON,542,461,80,34,PLAY);
        checkbox_help(close_checkbox,L"Hide the launcher automatically once the game starts, then exit when the game closes. Leave unchecked to minimize the launcher instead.");
        update_button=control(L"BUTTON",L"Update from GitHub",WS_TABSTOP,22,505,155,30,UPDATE);
        experimental_checkbox=control(L"BUTTON",L"Use experimental releases",BS_AUTOCHECKBOX|WS_TABSTOP,195,505,420,30,EXPERIMENTAL);
        SendMessageW(experimental_checkbox,BM_SETCHECK,installed_experimental?BST_CHECKED:BST_UNCHECKED,0);
        creating_graphics=TRUE;
        control(L"STATIC",L"Resolution",0,22,26,150,28,0);
        graphics_resolution=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,190,22,370,260,0);
        graphics_choice(graphics_resolution,L"Current game resolution",0);
        DEVMODEW display={0};display.dmSize=sizeof(display);
        for(DWORD index=0;EnumDisplaySettingsW(NULL,index,&display);index++){
            if(display.dmPelsWidth<800||display.dmPelsHeight<600||display.dmPelsWidth>32767||display.dmPelsHeight>32767)continue;
            LPARAM value=MAKELONG(display.dmPelsWidth,display.dmPelsHeight);BOOL found=FALSE;
            for(int i=1;i<SendMessageW(graphics_resolution,CB_GETCOUNT,0,0);i++)if(SendMessageW(graphics_resolution,CB_GETITEMDATA,i,0)==value)found=TRUE;
            if(!found){WCHAR label[64];_snwprintf(label,64,L"%lu × %lu",display.dmPelsWidth,display.dmPelsHeight);graphics_choice(graphics_resolution,label,value);}
        }
        control(L"STATIC",L"Display mode",0,22,70,150,28,0);
        graphics_mode=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,190,66,370,160,0);
        graphics_choice(graphics_mode,L"Current game mode",0);graphics_choice(graphics_mode,L"Windowed",1);graphics_choice(graphics_mode,L"Borderless",2);graphics_choice(graphics_mode,L"Exclusive fullscreen",3);
        graphics_aa=control(L"BUTTON",L"Anti-aliasing",BS_AUTOCHECKBOX|WS_TABSTOP,22,110,280,28,0);
        control(L"STATIC",L"Retention",0,312,110,74,20,0);
        graphics_retention=control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_AUTOTICKS|TBS_HORZ,390,106,180,32,0);
        SendMessageW(graphics_retention,TBM_SETRANGE,TRUE,MAKELONG(0,10));
        SendMessageW(graphics_retention,TBM_SETTICFREQ,1,0);
        graphics_retention_value=control(L"STATIC",L"0",0,576,110,30,20,0);
        graphics_bloom=control(L"BUTTON",L"Bloom",BS_AUTOCHECKBOX|WS_TABSTOP,22,142,280,28,0);
        graphics_dof=control(L"BUTTON",L"Depth of field",BS_AUTOCHECKBOX|WS_TABSTOP,312,142,280,28,0);
        control(L"STATIC",L"UI scale",0,22,182,150,28,0);
        graphics_scale=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,190,178,370,240,0);
        graphics_choice(graphics_scale,L"Automatic",0);
        for(int value=25;value<=100;value+=5){WCHAR label[32];_snwprintf(label,32,L"%d",value);graphics_choice(graphics_scale,label,value);}
        control(L"STATIC",L"Recent Tracks limit",0,22,218,165,28,0);
        graphics_recent=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,190,214,370,160,0);
        graphics_choice(graphics_recent,L"All tracks",0);graphics_choice(graphics_recent,L"20 (original limit)",20);graphics_choice(graphics_recent,L"50",50);graphics_choice(graphics_recent,L"100",100);
        graphics_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,22,254,220,32,GRAPHICS_SAVE);
        checkbox_help(graphics_resolution,L"Use a display resolution reported by Windows or Proton. Current game resolution keeps the game's saved resolution. Restart the game after changes.");
        checkbox_help(graphics_mode,L"Windowed, borderless or exclusive fullscreen. Current game mode keeps the game's original saved fullscreen choice.");
        checkbox_help(graphics_aa,L"Enable ReRiff's anti-aliasing. Uses the same setting as the in-game Graphics menu.");
        checkbox_help(graphics_retention,L"0 keeps vanilla culling and memory use. 1 adds a minimal behind and ahead visual window. 10 uses the maximum 512-block visual window. Higher values keep more road, ramps, rails and scrolling signs for free look and photo mode. Restart the game to apply.");
        checkbox_help(graphics_bloom,L"Enable the game's bloom effect. This is the global default; the in-game L3+X hotkey toggles it for the current session.");
        checkbox_help(graphics_dof,L"Enable the game's depth-of-field effect. This is the global default; the in-game L3+A hotkey toggles it for the current session.");
        checkbox_help(graphics_scale,L"Matches the in-game UI scale: Automatic, or 25–100 in steps of five. 50 is the reference size.");
        checkbox_help(graphics_recent,L"Limit the recent songs shown across Recent Tracks sort tabs. All tracks retains the complete saved catalogue. Bundled tracks are unaffected. Restart the game after changes.");
        restart_note(L"Restart the game after changing these settings",22,298,590);
        graphics_reload();creating_graphics=FALSE;
        controls_tabs=CreateWindowExW(0,WC_TABCONTROLW,L"",WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS,22,40,600,28,window,NULL,GetModuleHandleW(NULL),NULL);
        SendMessageW(controls_tabs,WM_SETFONT,(WPARAM)font,TRUE);
        tab.pszText=L"Controller";SendMessageW(controls_tabs,TCM_INSERTITEMW,CONTROLS_CONTROLLER,(LPARAM)&tab);
        tab.pszText=L"Keyboard";SendMessageW(controls_tabs,TCM_INSERTITEMW,CONTROLS_KEYBOARD,(LPARAM)&tab);
        tab.pszText=L"Hotkeys";SendMessageW(controls_tabs,TCM_INSERTITEMW,CONTROLS_HOTKEYS,(LPARAM)&tab);
        SendMessageW(controls_tabs,TCM_SETCURSEL,CONTROLS_CONTROLLER,0);
        creating_controls=TRUE;controls_create();creating_controls=FALSE;
        creating_languages=TRUE;
        language_checkbox=control(L"BUTTON",L"CJK fonts",BS_AUTOCHECKBOX|WS_TABSTOP,22,26,590,32,CJK_INSTALL);
        latin_checkbox=control(L"BUTTON",L"Latin/European fonts",BS_AUTOCHECKBOX|WS_TABSTOP,22,68,590,32,LATIN_INSTALL);
        restart_note(L"Restart the game after changing fonts",22,116,590);
        creating_languages=FALSE;
        creating_diagnostics=TRUE;
        logging_checkbox=control(L"BUTTON",L"Enable diagnostic logging",BS_AUTOCHECKBOX|WS_TABSTOP,22,26,590,32,LOGGING);
        controls_trace=control(L"BUTTON",L"Camera diagnostic trace",BS_AUTOCHECKBOX|WS_TABSTOP,22,60,590,28,CONTROLS_TRACE);
        checkbox_help(controls_trace,L"Write detailed camera set, camera and track-culling traces to the game log. Use only for diagnostics; it adds log I/O every 90-120 frames.");
        for(unsigned i=0;i<LOG_CATEGORY_COUNT;i++)
            log_checkboxes[i]=control(L"BUTTON",log_labels[i],BS_AUTOCHECKBOX|WS_TABSTOP,22+(i%2)*300,96+(i/2)*32,290,28,LOG_CATEGORY_BASE+i);
        control(L"BUTTON",L"View update details",WS_TABSTOP,22,290,200,32,UPDATE_DETAILS);
        control(L"BUTTON",L"Uninstall ReRiff",WS_TABSTOP,242,290,200,32,UNINSTALL);
        restart_note(L"Restart the game or local server after logging changes",22,260,590);
        creating_diagnostics=FALSE;
        creating_lastfm=TRUE;
        lfm_account=control(L"STATIC",L"No Last.fm account linked",0,22,26,590,28,0);
        control(L"STATIC",L"API key",0,22,64,140,28,0);
        lfm_key_field=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,170,60,390,28,LASTFM_KEY);
        control(L"STATIC",L"Shared secret",0,22,102,140,28,0);
        lfm_secret_field=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,170,98,390,28,LASTFM_SECRET);
        SendMessageW(lfm_key_field,EM_SETLIMITTEXT,32,0);SendMessageW(lfm_secret_field,EM_SETLIMITTEXT,32,0);
        lfm_enabled=control(L"BUTTON",L"Enable Last.fm scrobbling",BS_AUTOCHECKBOX|WS_TABSTOP,22,140,590,28,LASTFM_ENABLED);
        control(L"STATIC",L"Scrobble after listening to (%)",0,22,182,280,28,0);
        lfm_percent=control(L"EDIT",L"80",WS_BORDER|WS_TABSTOP|ES_NUMBER,310,178,80,28,LASTFM_PERCENT);
        SendMessageW(lfm_percent,EM_SETLIMITTEXT,3,0);
        lfm_link=control(L"BUTTON",L"Link account",WS_TABSTOP,22,222,220,32,LASTFM_LINK);
        lfm_disconnect=control(L"BUTTON",L"Disconnect",WS_TABSTOP,260,222,170,32,LASTFM_DISCONNECT);
        lfm_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,22,266,220,32,LASTFM_SAVE);
        checkbox_help(lfm_enabled,L"Scrobble tagged songs after a completed race or the configured listening percentage. Pauses and skips do not count. Last.fm requires songs longer than 30 seconds and half the song or four minutes of listening, whichever is shorter. Restart the game after changing settings.");
        checkbox_help(lfm_percent,L"Default 80%. A completed race can qualify earlier, subject to Last.fm's minimum listening requirement.");
        checkbox_help(lfm_link,L"Without an API key, opens Last.fm application registration. Enter the key and secret here and save settings. Approve access in your browser; linking finishes automatically while this tab is open. No password is entered here.");
        checkbox_help(lfm_secret_field,L"Masked while typing. Save settings encrypts the key, secret and existing session with user-scoped DPAPI. Keep your Windows profile or Proton prefix private.");
        restart_note(L"Restart the game after changing settings",260,272,360);
        lfm_reload();
        creating_lastfm=FALSE;
        creating_listenbrainz=TRUE;
        control(L"STATIC",L"ListenBrainz scrobbling",0,22,26,590,28,0);
        control(L"STATIC",L"User token",0,22,60,140,28,0);
        lb_token=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,170,60,390,28,LISTENBRAINZ_TOKEN);
        SendMessageW(lb_token,EM_SETLIMITTEXT,36,0);
        lb_enabled=control(L"BUTTON",L"Enable ListenBrainz scrobbling",BS_AUTOCHECKBOX|WS_TABSTOP,22,110,590,28,LISTENBRAINZ_ENABLED);
        control(L"STATIC",L"Scrobble after listening to (%)",0,22,154,280,28,0);
        lb_percent=control(L"EDIT",L"80",WS_BORDER|WS_TABSTOP|ES_NUMBER,310,154,80,28,LISTENBRAINZ_PERCENT);
        SendMessageW(lb_percent,EM_SETLIMITTEXT,3,0);
        lb_get_token=control(L"BUTTON",L"Get user token",WS_TABSTOP,22,210,220,32,LISTENBRAINZ_GET_TOKEN);
        lb_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,260,210,220,32,LISTENBRAINZ_SAVE);
        checkbox_help(lb_enabled,L"Send tagged songs to ListenBrainz after completion or the configured percentage. Only actual listening counts, with a minimum of half the track or four minutes. Songs must be longer than 30 seconds. Works independently of Last.fm. Restart the game after changes.");
        checkbox_help(lb_token,L"Copy your personal user token from ListenBrainz settings. Save protects it with user-scoped DPAPI. Keep your Windows profile or Proton prefix private. No application key or shared secret is needed.");
        checkbox_help(lb_percent,L"Default 80%. Completed races can qualify earlier after the minimum listening time. Pauses and skips do not count.");
        restart_note(L"Restart the game after changing settings",22,254,590);
        lb_reload();creating_listenbrainz=FALSE;
        creating_libre=TRUE;
        libre_account=control(L"STATIC",L"No Libre.fm account linked",0,22,26,590,28,0);
        control(L"STATIC",L"API key",0,22,64,140,28,0);
        libre_key_field=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,170,60,390,28,LIBRE_CREDENTIAL);
        SendMessageW(libre_key_field,EM_SETLIMITTEXT,32,0);
        libre_enabled=control(L"BUTTON",L"Enable Libre.fm scrobbling",BS_AUTOCHECKBOX|WS_TABSTOP,22,104,590,28,LIBRE_ENABLED);
        control(L"STATIC",L"Scrobble after listening to (%)",0,22,148,280,28,0);
        libre_percent=control(L"EDIT",L"80",WS_BORDER|WS_TABSTOP|ES_NUMBER,310,148,80,28,LIBRE_PERCENT);
        SendMessageW(libre_percent,EM_SETLIMITTEXT,3,0);
        libre_link=control(L"BUTTON",L"Link account",WS_TABSTOP,22,200,220,32,LIBRE_LINK);
        libre_disconnect=control(L"BUTTON",L"Disconnect",WS_TABSTOP,260,200,170,32,LIBRE_DISCONNECT);
        libre_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,22,244,220,32,LIBRE_SAVE);
        checkbox_help(libre_key_field,L"Paste your Libre.fm API key, then Save settings. The key is masked and encrypted for your Windows profile or Proton prefix. Changing it clears the previous account link. Leave blank to use ReRiff's default client identifier.");
        checkbox_help(libre_link,L"Approve ReRiff in your browser; linking finishes automatically while this subtab is open. No personal API key or password is needed here.");
        checkbox_help(libre_enabled,L"Submit tagged songs to Libre.fm after completion or the configured percentage. Songs must be longer than 30 seconds and actual listening must reach half the track or four minutes. Works independently of Last.fm and ListenBrainz. Restart the game after changes.");
        checkbox_help(libre_percent,L"Default 80%. Pauses and skips do not count. Completed races can qualify earlier after the minimum listening time.");
        restart_note(L"Restart the game after changing settings",260,250,360);
        libre_reload();creating_libre=FALSE;

        SendMessageW(logging_checkbox,BM_SETCHECK,rr_logging_enabled(folder,installed_experimental)?BST_CHECKED:BST_UNCHECKED,0);
        checkbox_help(popup,L"Show the selected server or singleplayer when the game starts.");
        checkbox_help(only_checkbox,L"Skip the secondary server. If the primary server is absent or unavailable, use singleplayer automatically.");
        checkbox_help(networking_checkbox,L"Skip server connections and force singleplayer on the next start. When unchecked, singleplayer is still chosen automatically if servers are absent or unavailable. Saved server addresses are retained.");
        checkbox_help(skip_checkbox,L"Start the game without showing the launcher. Applies on the next start; use --show-launcher to open settings again.");
        checkbox_help(auto_update_checkbox,L"Check the selected release channel for updates before starting when the launcher is skipped. Successful checks are reused for 15 minutes. Update from GitHub always checks immediately. A failed check starts the installed version.");
        checkbox_help(experimental_checkbox,L"Use opt-in beta releases for updates. Uncheck to use stable releases.");
        checkbox_help(language_checkbox,L"Install and enable Chinese, Japanese and Korean characters in filenames, song metadata, leaderboards and pause screens. Downloads about 105 MiB. Restart the game after changing this option. Unchecking retains the fonts. Each font option has its own folder in mods; menus are not translated. High-resolution text is selected automatically.");
        checkbox_help(latin_checkbox,L"Install and enable missing Latin/European letters, including Polish, throughout the game. A separate small download installs in its own folder in mods. Restart the game after changing this option. Unchecking retains the fonts; menus are not translated. High-resolution text is selected automatically.");
        checkbox_help(logging_checkbox,L"Enable diagnostic logging for the selected categories. Related diagnostics share a log. Disabling keeps your selections and existing logs. Restart the game or local server after changes. Logs may contain file paths and server addresses; review before sharing.");
        for(unsigned i=0;i<LOG_CATEGORY_COUNT;i++)
            checkbox_help(log_checkboxes[i],L"Include this category when diagnostic logging is enabled. Related components share a log. Existing logs are kept. Restart the game or local server after changes. Logs may contain file paths and server addresses; review before sharing.");
        reload_log_controls();controls_trace_reload();language_controls();select_launcher_page();
        channel_title();local_server_controls();
        reload_settings_controls();
        SetTimer(hwnd,1,500,NULL);
        begin(FALSE);return 0;
    }
    if(event==WM_NOTIFY&&(((NMHDR *)lp)->hwndFrom==launcher_tabs||((NMHDR *)lp)->hwndFrom==scrobbling_tabs||((NMHDR *)lp)->hwndFrom==controls_tabs)&&((NMHDR *)lp)->code==TCN_SELCHANGE){select_launcher_page();return 0;}
    if (event==WM_COMMAND) {
        if(LOWORD(wp)>=CONTROLLER_CAPTURE_BASE&&LOWORD(wp)<CONTROLLER_CAPTURE_BASE+CONTROLLER_BINDING_COUNT){
            if(HIWORD(wp)==BN_CLICKED&&!busy)controls_capture_begin(LOWORD(wp)-CONTROLLER_CAPTURE_BASE);
            return 0;
        }
        if(LOWORD(wp)>=KEYBOARD_CAPTURE_BASE&&LOWORD(wp)<KEYBOARD_CAPTURE_BASE+KEYBOARD_ACTION_COUNT){
            if(HIWORD(wp)==BN_CLICKED&&!busy)controls_key_capture_begin(LOWORD(wp)-KEYBOARD_CAPTURE_BASE);
            return 0;
        }
        if(LOWORD(wp)>=LOG_CATEGORY_BASE&&LOWORD(wp)<LOG_CATEGORY_BASE+LOG_CATEGORY_COUNT) {
            unsigned i=LOWORD(wp)-LOG_CATEGORY_BASE;WCHAR key[80];_snwprintf(key,80,L"log_%ls",log_categories[i]);
            BOOL enabled=SendMessageW(log_checkboxes[i],BM_GETCHECK,0,0)==BST_CHECKED;
            if(!WritePrivateProfileStringW(L"diagnostics",key,enabled?L"true":L"false",ini))reload_log_controls();
            settings_observed=read_settings_signature(&settings_signature);return 0;
        }
        switch (LOWORD(wp)) {
        case GRAPHICS_SAVE: if(!busy)graphics_save_settings();break;
        case CONTROLS_SAVE: if(!busy)controls_save_settings();break;
        case CONTROLS_TRACE: if(!busy){BOOL ok=controls_trace_toggle();SetWindowTextW(status,ok?L"Camera diagnostic trace setting saved. Restart the game to apply.":L"Could not save the camera trace setting.");}break;
        case CONTROLS_DEFAULTS: if(!busy){int part=(int)SendMessageW(controls_tabs,TCM_GETCURSEL,0,0);if(part==CONTROLS_KEYBOARD)controls_keyboard_defaults();else if(part==CONTROLS_CONTROLLER)controls_controller_defaults();}break;
        case CLOSE_LAUNCHER:
            if(HIWORD(wp)==BN_CLICKED){
                BOOL enabled=SendMessageW(close_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
                BOOL ok=WritePrivateProfileStringW(L"launcher",L"close_on_game_start",enabled?L"true":L"false",ini);
                if(ok){settings_observed=read_settings_signature(&settings_signature);SetWindowTextW(status,enabled?L"Launcher will close when the game starts.":L"Launcher will minimize when the game starts.");}
                else SetWindowTextW(status,L"Could not save the launcher setting.");
            }
            break;
            case RERIFF_BRAND_CONTROL:
                if(HIWORD(wp)==BN_CLICKED)ShellExecuteW(hwnd,L"open",L"https://github.com/Aftonstjarna/ReRiff",NULL,NULL,SW_SHOWNORMAL);
                break;
            case UPDATE_DETAILS:show_update_details();break;
            case UNINSTALL:
                if(!busy&&MessageBoxW(hwnd,L"Remove ReRiff and restore the original game DLLs?\n\nLocalserver, singleplayer saves, INI settings, logs, backups and unrelated mods will be kept. Close the game first.\n\nAfterward, remove ReRiff from Steam's launch options.",L"Uninstall ReRiff",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)==IDYES)begin(10);
                break;
            case LIBRE_LINK:if(!busy){if(*libre_url)libre_open_browser();else {*libre_token=0;begin(11);}}break;
            case LIBRE_DISCONNECT:if(!busy)libre_unlink();break;
            case LIBRE_SAVE:if(!busy)libre_save_settings();break;
            case LASTFM_LINK:if(!busy){if(!lfm_has_api_key())lfm_register_application();else if(*lfm_url)lfm_open_browser();else {*lfm_token=0;begin(8);}}break;
            case LASTFM_DISCONNECT:if(!busy)lfm_unlink();break;
            case LISTENBRAINZ_GET_TOKEN:if(!busy)lb_open_token();break;
            case LISTENBRAINZ_SAVE:if(!busy)lb_save_settings();break;
            case LASTFM_SAVE:if(!busy)lfm_save_settings();break;
            case LOGGING: {
                BOOL enabled=SendMessageW(logging_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
                if(WritePrivateProfileStringW(L"diagnostics",L"logging_enabled",enabled?L"true":L"false",ini)) {
                    if(enabled){WCHAR logs[CAP],source[CAP],target[CAP];path(logs,L"logs");CreateDirectoryW(logs,NULL);path(source,L"localserver\\diagnostics\\Collect-ReRiffStartup.ps1");path(target,L"logs\\Collect-ReRiffStartup.ps1");CopyFileW(source,target,TRUE);}
                    reload_log_controls();settings_observed=read_settings_signature(&settings_signature);SetWindowTextW(status,enabled?L"Diagnostic logging enabled.":L"Logging disabled. Existing logs were kept.");
                }
                break;
            }
            case LATIN_INSTALL:begin(SendMessageW(latin_checkbox,BM_GETCHECK,0,0)==BST_CHECKED?6:7);break;
            case CJK_INSTALL:begin(SendMessageW(language_checkbox,BM_GETCHECK,0,0)==BST_CHECKED?4:5);break;
            case NETWORKING: {
                /* Same saved mode as the native in-game toggle. Do not save
                 * unrelated, possibly unfinished edits or clear server URLs. */
                BOOL singleplayer=SendMessageW(networking_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
                if(WritePrivateProfileStringW(L"server",L"networking_enabled",singleplayer?L"false":L"true",ini)){
                    settings_observed=read_settings_signature(&settings_signature);
                    SetWindowTextW(status,singleplayer?L"Singleplayer forced. Servers will be skipped on the next game start.":L"Servers will be tried first next start, with singleplayer as fallback.");
                }else{
                    SendMessageW(networking_checkbox,BM_SETCHECK,singleplayer?BST_UNCHECKED:BST_CHECKED,0);
                    SetWindowTextW(status,L"Could not save the singleplayer setting to server.ini.");
                }
                update_networking_controls();break;
            }
            case EXTERNAL_ONLY:update_networking_controls();break;
            case STOP:begin(2);break;
            case EXPERIMENTAL:
                requested_experimental=SendMessageW(experimental_checkbox,BM_GETCHECK,0,0)==BST_CHECKED;
                begin(3);break;
            case UPDATE:begin(3);break;
            case SAVE:if(save())settings_observed=read_settings_signature(&settings_signature);break;
            case PLAY:begin(TRUE);break;
        }return 0;
    }
    if(event==WM_KEYDOWN&&(controls_capture_key((int)wp)||controls_key_capture_key((int)wp)))return 0;
    if(event==WM_HSCROLL&&(HWND)lp==graphics_retention){WCHAR label[16];_snwprintf(label,16,L"%d",(int)SendMessageW(graphics_retention,TBM_GETPOS,0,0));SetWindowTextW(graphics_retention_value,label);return 0;}
    if(event==WM_TIMER&&wp==CONTROLS_CAPTURE_TIMER){controls_capture_tick();return 0;}
    if(event==WM_TIMER&&wp==KEYBOARD_CAPTURE_TIMER){controls_key_capture_tick();return 0;}
    if(event==WM_TIMER&&wp==1){refresh_changed_settings();if(SendMessageW(launcher_tabs,TCM_GETCURSEL,0,0)==PAGE_SCROBBLING&&SendMessageW(scrobbling_tabs,TCM_GETCURSEL,0,0)==0&&lfm_poll_due(GetTickCount()))begin(9);if(SendMessageW(launcher_tabs,TCM_GETCURSEL,0,0)==PAGE_SCROBBLING&&SendMessageW(scrobbling_tabs,TCM_GETCURSEL,0,0)==2&&libre_poll_due(GetTickCount()))begin(12);return 0;}
    if(event==WM_ACTIVATE&&LOWORD(wp)!=WA_INACTIVE)refresh_changed_settings();
    if(event==RERIFF_PROGRESS_EVENT){if(busy)progress_set_bar(launcher_progress,(int)(INT_PTR)lp);return 0;}
    if (event==WM_GAME_RUNNING) {
        if(SendMessageW(close_checkbox,BM_GETCHECK,0,0)==BST_CHECKED){exit_after_game=TRUE;ShowWindow(hwnd,SW_HIDE);return 0;}
        progress_set_bar(launcher_progress,0);SetWindowTextW(status,L"Game running. Keep the launcher open until the game closes.");ShowWindow(window,SW_MINIMIZE);return 0;
    }
    if (event==WM_COMPLETE) {
        if(lp==3){MessageBoxW(hwnd,message,L"ReRiff removed",MB_OK|MB_ICONINFORMATION);DestroyWindow(hwnd);return 0;}
        if (lp==2) { DestroyWindow(hwnd);return 0; }
        if (lp==1 && exit_after_game) { DestroyWindow(hwnd);return 0; }
        if(changing_language){settings_observed=read_settings_signature(&settings_signature);changing_language=FALSE;}
        if (lp==1) reload_settings_controls();
        requested_experimental=installed_experimental;
        SendMessageW(experimental_checkbox,BM_SETCHECK,installed_experimental?BST_CHECKED:BST_UNCHECKED,0);
        progress_set_bar(launcher_progress,0);busy=FALSE;playing=FALSE;EnableWindow(play_button,TRUE);EnableWindow(save_button,TRUE);EnableWindow(stop_button,TRUE);EnableWindow(update_button,TRUE);EnableWindow(experimental_checkbox,TRUE);SetWindowTextW(status,message);
        libre_link_completed(libre_action,(BOOL)wp,GetTickCount());if(libre_action==9&&wp)libre_reload();if(libre_action==8&&wp)libre_open_browser();libre_action=0;
        lfm_link_completed(lfm_action,(BOOL)wp,GetTickCount());local_server_controls();language_controls();if(lfm_action==9&&wp)lfm_reload();lfm_controls();lb_controls();libre_controls();if(lfm_action==8&&wp)lfm_open_browser();lfm_action=0;EnableWindow(language_checkbox,TRUE);EnableWindow(latin_checkbox,TRUE);EnableWindow(logging_checkbox,TRUE);
        if(!wp&&checking_update&&*update_failure)show_update_details();checking_update=FALSE;
        if (lp) ShowWindow(window,SW_RESTORE);return 0;
    }
    if (event==WM_CLOSE) {
        if (busy) { SetWindowTextW(status,L"Wait for server startup or close the game before closing this launcher.");return 0; }
        DestroyWindow(hwnd);return 0;
    }
    if (event==WM_DESTROY) { progress_sink=NULL;KillTimer(hwnd,1);KillTimer(hwnd,CONTROLS_CAPTURE_TIMER);KillTimer(hwnd,KEYBOARD_CAPTURE_TIMER);rr_brand_cleanup();DeleteObject(font);PostQuitMessage(0);return 0; }
    return DefWindowProcW(hwnd,event,wp,lp);
}

static DWORD WINAPI automatic_update_worker(LPVOID unused) {
    (void)unused;BOOL ok=check_updates();log_message(message);return ok?0:1;
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
    if(restart_parent_pid){
        int restart=wait_for_restart_parent();
        if(restart<0){log_message(message);return 1;}
        if(!restart)return 0;
        skip=TRUE;force_show_launcher=FALSE;
    }
    if (selftest) {
        WCHAR value[4096];GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,4096,ini);
        log_message(L"SELFTEST: native launcher initialized; no game or server started.");log_message(ini);log_message(value);return 0;
    }
    if(force_auto_reinstall)log_message(L"TEST: unattended update requested.");
    if (should_skip_launcher(skip)) {
        if(!restart_parent_pid&&!suppress_auto_update&&(force_auto_reinstall||launcher_preference(L"auto_update_on_skip"))){
            automatic_update=TRUE;progress_open(TRUE);progress_report(PROGRESS_CHECK,-1);
            HANDLE update=CreateThread(NULL,4*1024*1024,automatic_update_worker,NULL,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
            if(update){while(progress_wait(update,1000)==WAIT_TIMEOUT){}CloseHandle(update);}
            else log_message(L"Could not start automatic update check; starting installed game.");
            progress_close();
            if(update_handoff)return 0;
            /* Network/preflight failures leave installed files intact. Play
             * the existing version rather than blocking a skipped launch. */
        }
        if (!launch_game()) {log_message(message);return 1;}
        return 0;
    }
    WNDCLASSW cls;ZeroMemory(&cls,sizeof(cls));cls.lpfnWndProc=procedure;cls.hInstance=instance;cls.lpszClassName=L"ReRiffNativeLauncher";
    cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
    RegisterClassW(&cls);
    HWND handle=CreateWindowExW(0,cls.lpszClassName,launcher_title(),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,660,631,NULL,NULL,instance,NULL);
    if (!handle) return 1;
    ShowWindow(handle,show);UpdateWindow(handle);
    MSG msg;while (GetMessageW(&msg,NULL,0,0)>0) {
        if (msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE&&(controls_capture_row>=0||controls_key_capture_row>=0)) {
            controls_capture_cancel();controls_key_capture_cancel();
            SetWindowTextW(status,L"Binding capture cancelled.");
            continue;
        }
        if (!IsDialogMessageW(handle,&msg)) {TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    return (int)msg.wParam;
}
