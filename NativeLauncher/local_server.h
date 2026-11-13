/* Platform-local optional server controls; Windows uses private Python/Flask. */
static int local_server_state;
static WCHAR local_server_address[128]=L"http://127.0.0.1:5144";
static HWND local_server_hint;
static void local_server_controls(void){
    if(stop_button)SetWindowTextW(stop_button,local_server_state==2?L"Stop local server":local_server_state==1?L"Start local server":L"Install local server");
    if(local_server_hint){WCHAR text[256];_snwprintf(text,256,L"Local server: %ls — enter in Secondary server",local_server_address);SetWindowTextW(local_server_hint,text);}
}
#include "linux_server.h"
static BOOL windows_server_action(const WCHAR *action){
    WCHAR script[CAP],root[CAP],config[CAP],python[CAP],result[CAP],temporary[CAP],command[CAP];
    WCHAR work[CAP]=L"",asset[CAP]=L"",hash_w[65]=L"";
    path(script,L"localserver\\windows-server-control.py");path(root,L"localserver");path(config,L"localserver\\native\\config.json");
    if(!wcscmp(action,L"status")&&GetFileAttributesW(config)==INVALID_FILE_ATTRIBUTES){local_server_state=0;return TRUE;}
    if(GetFileAttributesW(script)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The Windows local-server helper is missing. Install the complete client package.");return FALSE;}
    if(!ensure_private_python(window,folder,wcscmp(action,L"status")&&wcscmp(action,L"stop"))){wcscpy(message,L"Private Python setup was canceled or is unavailable. Run ReRiff setup to install it.");return FALSE;}
    path(python,L"localserver\\windows-python\\python.exe");
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rrw",0,result))return FALSE;DeleteFileW(result);
    if(!wcscmp(action,L"install")){
        WCHAR sums[CAP],url[CAP];char expected[65]="",line[1024],listed[512],sum[65];
        if(!GetTempFileNameW(temporary,L"rrs",0,work))return FALSE;
        DeleteFileW(work);if(!CreateDirectoryW(work,NULL))return FALSE;
        _snwprintf(sums,CAP,L"%ls\\SHA256SUMS",work);
        HINTERNET session=InternetOpenW(L"ReRiffLocalServer/" RERIFF_VERSION,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
        if(!session){python_remove_stage(work);return FALSE;}
        DWORD timeout=30000;InternetSetOptionW(session,INTERNET_OPTION_CONNECT_TIMEOUT,&timeout,sizeof(timeout));InternetSetOptionW(session,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(timeout));
        _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/v%ls/SHA256SUMS",RERIFF_WIDE_LITERAL(RERIFF_VERSION));
        BOOL ok=update_download(session,url,sums,65535);FILE *file=ok?_wfopen(sums,L"rb"):NULL;
        if(file){while(fgets(line,sizeof(line),file))if(sscanf(line,"%64[0123456789abcdefABCDEF] %511s",sum,listed)==2&&strlen(sum)==64&&!strcmp(listed,"ReRiff-Server-windows.zip"))strcpy(expected,sum);fclose(file);}
        _snwprintf(asset,CAP,L"%ls\\ReRiff-Server-windows.zip",work);
        _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/v%ls/ReRiff-Server-windows.zip",RERIFF_WIDE_LITERAL(RERIFF_VERSION));
        ok=ok&&*expected&&update_download(session,url,asset,256*1024*1024)&&update_digest(asset,expected);InternetCloseHandle(session);
        if(!ok){python_remove_stage(work);wcscpy(message,L"Could not download a verified Windows local-server package for this client release. See logs/ReRiff-launcher.log.");return FALSE;}
        MultiByteToWideChar(CP_UTF8,0,expected,-1,hash_w,65);
    }
    if(*asset)_snwprintf(command,CAP,L"\"%ls\" -I \"%ls\" %ls --root \"%ls\" --result \"%ls\" --asset \"%ls\" --sha256 %ls",python,script,action,root,result,asset,hash_w);
    else _snwprintf(command,CAP,L"\"%ls\" -I \"%ls\" %ls --root \"%ls\" --result \"%ls\"",python,script,action,root,result);
    command[CAP-1]=0;PROCESS_INFORMATION process;
    if(!spawn(command,TRUE,&process)){if(*work)python_remove_stage(work);wcscpy(message,L"Could not start the Windows local-server helper.");return FALSE;}
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    ULONGLONG deadline=GetTickCount64()+90000;
    while(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES&&GetTickCount64()<deadline)Sleep(100);
    WCHAR value[64];GetPrivateProfileStringW(L"result",L"state",L"",value,64,result);
    if(!wcscmp(value,L"running"))local_server_state=2;else if(!wcscmp(value,L"stopped"))local_server_state=1;else if(!wcscmp(value,L"uninstalled"))local_server_state=0;
    GetPrivateProfileStringW(L"result",L"address",L"http://127.0.0.1:5144",local_server_address,128,result);
    GetPrivateProfileStringW(L"result",L"message",L"Local-server operation did not report completion. See logs/ReRiff-localserver.log; no server was force-stopped.",message,1024,result);
    GetPrivateProfileStringW(L"result",L"ok",L"false",value,64,result);DeleteFileW(result);
    if(*work)python_remove_stage(work);return !wcscmp(value,L"true");
}
static BOOL local_server_action(const WCHAR *action){
    return is_wine()?linux_server_action(action):windows_server_action(action);
}
