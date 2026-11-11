/* Optional Linux backend. Only the server AppImage contains Python. */
static BOOL linux_server_action(const WCHAR *action){
    WCHAR bridge[CAP],host_bridge[CAP],root[CAP],host_root[CAP],system[CAP],command[CAP];
    WCHAR result[CAP],host_result[CAP],temporary[CAP],asset[CAP]=L"",host_asset[CAP]=L"",hash_w[65]=L"";
    path(bridge,L"localserver\\ReRiff-ClientHost");
    if(GetFileAttributesW(bridge)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The native Linux client helper is missing. Install the complete Linux client package.");return FALSE;}
    path(bridge,L"localserver\\linux-runtime.sh");path(root,L"localserver");
    if(GetFileAttributesW(bridge)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The native Linux client helper is missing. Install the complete Linux client package.");return FALSE;}
    if(!update_host_path(bridge,host_bridge)||!update_host_path(root,host_root))return FALSE;
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rrl",0,result))return FALSE;DeleteFileW(result);
    if(!update_host_path(result,host_result))return FALSE;
    if(!wcscmp(action,L"install")){
        WCHAR work[CAP],sums[CAP],url[CAP];char expected[65]="",line[1024],listed[512],sum[65];
        if(!GetTempFileNameW(temporary,L"rra",0,work))return FALSE;
        DeleteFileW(work);if(!CreateDirectoryW(work,NULL))return FALSE;
        _snwprintf(sums,CAP,L"%ls\\SHA256SUMS",work);
        HINTERNET session=InternetOpenW(L"ReRiffLocalServer/" RERIFF_VERSION,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
        if(!session)return FALSE;
        DWORD limit=30000;InternetSetOptionW(session,INTERNET_OPTION_CONNECT_TIMEOUT,&limit,sizeof(limit));
        InternetSetOptionW(session,INTERNET_OPTION_RECEIVE_TIMEOUT,&limit,sizeof(limit));
        _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/v%ls/SHA256SUMS",RERIFF_WIDE_LITERAL(RERIFF_VERSION));
        BOOL ok=update_download(session,url,sums,65535);
        FILE *file=ok?_wfopen(sums,L"rb"):NULL;
        if(file){while(fgets(line,sizeof(line),file))if(sscanf(line,"%64[0123456789abcdefABCDEF] %511s",sum,listed)==2&&strlen(sum)==64&&!strcmp(listed,"ReRiff-Server-x86_64.AppImage"))strcpy(expected,sum);fclose(file);}
        _snwprintf(asset,CAP,L"%ls\\ReRiff-Server-x86_64.AppImage",work);
        _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/v%ls/ReRiff-Server-x86_64.AppImage",RERIFF_WIDE_LITERAL(RERIFF_VERSION));
        ok=ok&&*expected&&update_download(session,url,asset,256*1024*1024)&&update_digest(asset,expected);
        InternetCloseHandle(session);DeleteFileW(sums);
        if(!ok){DeleteFileW(asset);RemoveDirectoryW(work);wcscpy(message,L"Could not download a verified local-server AppImage for this client release. See logs/ReRiff-launcher.log.");return FALSE;}
        if(!update_host_path(asset,host_asset))return FALSE;
        MultiByteToWideChar(CP_UTF8,0,expected,-1,hash_w,65);
    }
    GetSystemDirectoryW(system,CAP);
    if(*asset)_snwprintf(command,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" server-control %ls --root \"%ls\" --result \"%ls\" --asset \"%ls\" --sha256 %ls",system,host_bridge,action,host_root,host_result,host_asset,hash_w);
    else _snwprintf(command,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" server-control %ls --root \"%ls\" --result \"%ls\"",system,host_bridge,action,host_root,host_result);
    command[CAP-1]=0;PROCESS_INFORMATION process;
    if(!spawn(command,TRUE,&process)){wcscpy(message,L"Could not start the bundled Linux helper.");return FALSE;}
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    ULONGLONG deadline=GetTickCount64()+90000;
    while(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES&&GetTickCount64()<deadline)Sleep(100);
    WCHAR value[64];GetPrivateProfileStringW(L"result",L"state",L"",value,64,result);
    if(!wcscmp(value,L"running"))podman_state=2;else if(!wcscmp(value,L"stopped"))podman_state=1;else if(!wcscmp(value,L"uninstalled"))podman_state=0;
    GetPrivateProfileStringW(L"result",L"address",L"http://127.0.0.1:5144",podman_address,128,result);
    GetPrivateProfileStringW(L"result",L"message",L"The Linux helper did not report completion. See logs/ReRiff-localserver.log.",message,1024,result);
    GetPrivateProfileStringW(L"result",L"ok",L"false",value,64,result);DeleteFileW(result);
    if(*asset){DeleteFileW(asset);WCHAR *slash=wcsrchr(asset,L'\\');if(slash){*slash=0;RemoveDirectoryW(asset);}}
    return !wcscmp(value,L"true");
}
