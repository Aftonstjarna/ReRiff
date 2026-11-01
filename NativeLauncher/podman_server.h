/* Optional image management runs on the native host, including under Proton. */
static int podman_state;
static WCHAR podman_address[128]=L"http://127.0.0.1:5144";
static HWND podman_hint;
static void podman_controls(void){
    if(stop_button)SetWindowTextW(stop_button,podman_state==2?L"Stop local server":podman_state==1?L"Start local server":L"Install local server");
    if(podman_hint){WCHAR text[256];_snwprintf(text,256,L"Local server: %ls — enter in Secondary server",podman_address);SetWindowTextW(podman_hint,text);}
}
static BOOL podman_action(const WCHAR *action){
    WCHAR script[CAP],config[CAP],result[CAP],temporary[CAP],command[CAP];
    path(script,L"localserver\\podman-server.py");path(config,L"localserver\\podman\\config.json");
    if(!wcscmp(action,L"status")&&GetFileAttributesW(config)==INVALID_FILE_ATTRIBUTES){podman_state=0;return TRUE;}
    if(GetFileAttributesW(script)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The optional Podman helper is missing. Reinstall the complete client package.");return FALSE;}
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rrp",0,result))return FALSE;DeleteFileW(result);
    if(is_wine()){
        WCHAR host_script[CAP],host_result[CAP],system[CAP];
        if(!update_host_path(script,host_script)||!update_host_path(result,host_result))return FALSE;
        GetSystemDirectoryW(system,CAP);
        _snwprintf(command,CAP,L"\"%ls\\start.exe\" /wait /unix /usr/bin/python3 \"%ls\" %ls --result \"%ls\"",system,host_script,action,host_result);
    }else{
        WCHAR python[CAP];path(python,L"localserver\\windows-python\\python.exe");
        if(GetFileAttributesW(python)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The installed Python helper is missing. Reinstall the complete client package.");return FALSE;}
        _snwprintf(command,CAP,L"\"%ls\" \"%ls\" %ls --result \"%ls\"",python,script,action,result);
    }
    command[CAP-1]=0;PROCESS_INFORMATION process;
    if(!spawn(command,TRUE,&process)){wcscpy(message,L"Could not run the optional Podman helper.");return FALSE;}
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    ULONGLONG deadline=GetTickCount64()+(!wcscmp(action,L"install")?660000:!wcscmp(action,L"start")?180000:45000);
    while(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES&&GetTickCount64()<deadline)Sleep(100);
    if(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"Local-server operation did not report completion. See logs/ReRiff-podman.log; no server was force-stopped.");return FALSE;}
    WCHAR value[64];GetPrivateProfileStringW(L"result",L"state",L"",value,64,result);
    if(!wcscmp(value,L"running"))podman_state=2;else if(!wcscmp(value,L"stopped"))podman_state=1;else if(!wcscmp(value,L"uninstalled"))podman_state=0;
    GetPrivateProfileStringW(L"result",L"address",L"http://127.0.0.1:5144",podman_address,128,result);
    GetPrivateProfileStringW(L"result",L"message",L"Local-server operation returned no details.",message,1024,result);
    GetPrivateProfileStringW(L"result",L"ok",L"false",value,64,result);DeleteFileW(result);
    return !wcscmp(value,L"true");
}
