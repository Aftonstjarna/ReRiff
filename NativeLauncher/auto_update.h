/* Downloads are restricted to the official GitHub release/tag names. The
 * existing transactional installer applies only after this process exits. */
#include <wincrypt.h>
#include <stdlib.h>
#include <ctype.h>
static BOOL update_handoff=FALSE;
static BOOL stop_server(void);

static BOOL update_download(HINTERNET session,const WCHAR *url,const WCHAR *destination,DWORD maximum) {
    HINTERNET req=InternetOpenUrlW(session,url,L"Accept: application/vnd.github+json\r\n",(DWORD)-1,
        INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_UI|INTERNET_FLAG_NO_COOKIES,0);
    DWORD code=0,size=sizeof(code),read=0,written=0,total=0;BOOL ok=FALSE;
    if(req&&HttpQueryInfoW(req,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&code,&size,NULL)&&code==200){
        HANDLE file=CreateFileW(destination,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE){
            char block[65536];ok=TRUE;
            for(;;){
                if(!InternetReadFile(req,block,sizeof(block),&read)){ok=FALSE;break;}
                if(!read)break;
                if(read>maximum-total||!WriteFile(file,block,read,&written,NULL)||written!=read){ok=FALSE;break;}
                total+=read;
            }
            CloseHandle(file);
        }
    }
    if(req)InternetCloseHandle(req);
    if(!ok){
        WCHAR detail[160];_snwprintf(detail,160,L"Update download failed: HTTP %lu, system error %lu",(unsigned long)code,(unsigned long)GetLastError());
        log_message(detail);DeleteFileW(destination);
    }
    return ok;
}

static BOOL update_digest(const WCHAR *name,const char *expected) {
    HANDLE file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;BOOL ok=FALSE;DWORD count=0,size=32;
    BYTE block[65536],digest[32];char hex[65];
    if(CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){
        ok=TRUE;
        for(;;){if(!ReadFile(file,block,sizeof(block),&count,NULL)){ok=FALSE;break;}if(!count)break;if(!CryptHashData(hash,block,count,0)){ok=FALSE;break;}}
        ok=ok&&CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0);
        if(ok){for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);ok=!_stricmp(hex,expected);}
    }
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);CloseHandle(file);return ok;
}

static BOOL update_host_path(const WCHAR *source,WCHAR *out) {
    typedef char * (__cdecl *unix_fn)(LPCWSTR);
    union {FARPROC address;unix_fn function;} convert;
    convert.address=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"wine_get_unix_file_name");
    if(!convert.address)return FALSE;
    char *value=convert.function(source);if(!value)return FALSE;
    BOOL ok=MultiByteToWideChar(CP_UTF8,0,value,-1,out,CAP)!=0;HeapFree(GetProcessHeap(),0,value);return ok;
}

static BOOL update_command(WCHAR *out,const WCHAR *work,const WCHAR *archive,const WCHAR *hash,BOOL prepare) {
    WCHAR tool[CAP],python[CAP],host_work[CAP],host_game[CAP],host_archive[CAP],system[CAP];
    _snwprintf(tool,CAP,L"%ls\\apply-client-update.py",work);tool[CAP-1]=0;
    if(is_wine()){
        WCHAR host_tool[CAP];
        if(!update_host_path(tool,host_tool)||!update_host_path(work,host_work)||!update_host_path(folder,host_game)
           ||!update_host_path(archive,host_archive))return FALSE;
        GetSystemDirectoryW(system,CAP);
        if(prepare)_snwprintf(out,CAP,L"\"%ls\\start.exe\" /wait /unix /usr/bin/python3 \"%ls\" --work \"%ls\" --prepare --archive \"%ls\" --sha256 %ls --game-dir \"%ls\" --platform linux",system,host_tool,host_work,host_archive,hash,host_game);
        else _snwprintf(out,CAP,L"\"%ls\\start.exe\" /wait /unix /usr/bin/python3 \"%ls\" --work \"%ls\"",system,host_tool,host_work);
    }else{
        if(prepare)path(python,L"localserver\\windows-python\\python.exe");
        else {_snwprintf(python,CAP,L"%ls\\runtime\\python.exe",work);python[CAP-1]=0;}
        if(GetFileAttributesW(python)==INVALID_FILE_ATTRIBUTES)return FALSE;
        if(prepare)_snwprintf(out,CAP,L"\"%ls\" \"%ls\" --work \"%ls\" --prepare --archive \"%ls\" --sha256 %ls --game-dir \"%ls\" --platform windows",python,tool,work,archive,hash,folder);
        else _snwprintf(out,CAP,L"\"%ls\" \"%ls\" --work \"%ls\"",python,tool,work);
    }
    out[CAP-1]=0;return TRUE;
}

static BOOL check_updates(void) {
    WCHAR temporary[CAP],work[CAP],api[CAP],sums[CAP],archive[CAP],url[CAP],filename[256],hash_w[65],command[CAP],name[CAP],source[CAP];
    BOOL ok=FALSE;PROCESS_INFORMATION process;char expected[65]="",body[65536],version[64];int a,b,c,x,y,z;char tail;
    const WCHAR *failure=L"Could not read the GitHub release";
    work[0]=0;
    HINTERNET session=InternetOpenW(L"ReRiffLauncher/" RERIFF_VERSION,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    if(!session){wcscpy(message,L"Could not connect to GitHub.");return FALSE;}
    DWORD limit=15000;InternetSetOptionW(session,INTERNET_OPTION_CONNECT_TIMEOUT,&limit,sizeof(limit));
    InternetSetOptionW(session,INTERNET_OPTION_RECEIVE_TIMEOUT,&limit,sizeof(limit));
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rru",0,work))goto done;
    DeleteFileW(work);if(!CreateDirectoryW(work,NULL))goto done;
    _snwprintf(api,CAP,L"%ls\\release.json",work);api[CAP-1]=0;
    if(!update_download(session,L"https://api.github.com/repos/Aftonstjarna/ReRiff/releases/latest",api,65535))goto done;
    FILE *file=_wfopen(api,L"rb");if(!file)goto done;
    size_t n=fread(body,1,sizeof(body)-1,file);body[n]=0;fclose(file);
    char *tag=strstr(body,"\"tag_name\"");
    if(!tag||!(tag=strchr(tag,':'))||!(tag=strchr(tag,'"')))goto done;
    tag++;if(*tag=='v')tag++;size_t length=strcspn(tag,"\"");
    if(length>=sizeof(version))goto done;memcpy(version,tag,length);version[length]=0;
    if(sscanf(version,"%d.%d.%d%c",&a,&b,&c,&tail)!=3||a<0||a>9999||b<0||b>9999||c<0||c>9999
       ||sscanf(RERIFF_VERSION,"%d.%d.%d",&x,&y,&z)!=3)goto done;
    if(a<x||(a==x&&(b<y||(b==y&&c<=z)))){wcscpy(message,L"You have the latest published release.");ok=TRUE;goto done;}
    failure=L"Could not read the release checksums";
    WCHAR version_w[64];MultiByteToWideChar(CP_UTF8,0,version,-1,version_w,64);
    _snwprintf(filename,256,L"ReRiff-%ls-%ls",version_w,is_wine()?L"linux.run":L"windows.zip");filename[255]=0;
    _snwprintf(sums,CAP,L"%ls\\SHA256SUMS",work);sums[CAP-1]=0;
    _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/v%ls/SHA256SUMS",version_w);url[CAP-1]=0;
    SetWindowTextW(status,L"Downloading and verifying the latest ReRiff update...");
    if(!update_download(session,url,sums,65535))goto done;
    char filename_u8[256],line[1024],listed[512],sum[65];
    WideCharToMultiByte(CP_UTF8,0,filename,-1,filename_u8,sizeof(filename_u8),NULL,NULL);
    file=_wfopen(sums,L"rb");if(!file)goto done;
    while(fgets(line,sizeof(line),file)){
        if(sscanf(line,"%64[0123456789abcdefABCDEF] %511s",sum,listed)==2&&strlen(sum)==64&&!strcmp(listed,filename_u8))strcpy(expected,sum);
    }fclose(file);if(!*expected)goto done;
    _snwprintf(archive,CAP,L"%ls\\%ls",work,filename);archive[CAP-1]=0;
    _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/v%ls/%ls",version_w,filename);url[CAP-1]=0;
    failure=L"Could not download the release package";
    if(!update_download(session,url,archive,64*1024*1024))goto done;
    failure=L"The release checksum did not match";
    if(!update_digest(archive,expected))goto done;
    MultiByteToWideChar(CP_UTF8,0,expected,-1,hash_w,65);
    path(source,L"localserver\\apply-client-update.py");
    failure=L"Could not start installer preflight";
    _snwprintf(name,CAP,L"%ls\\apply-client-update.py",work);name[CAP-1]=0;
    if(!CopyFileW(source,name,TRUE)||!update_command(command,work,archive,hash_w,TRUE)||!spawn(command,TRUE,&process))goto done;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    /* Wine start /unix can finish before the native Unix process. Use the
     * tool's completion markers, not the start.exe exit code, on both OSes. */
    ULONGLONG deadline=GetTickCount64()+120000;
    failure=L"Installer preflight failed; see update.log";
    for(;;){
        _snwprintf(name,CAP,L"%ls\\failure",work);name[CAP-1]=0;
        if(GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES)goto done;
        _snwprintf(name,CAP,L"%ls\\prepared.json",work);name[CAP-1]=0;
        if(GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES)break;
        if(GetTickCount64()>deadline)goto done;Sleep(100);
    }
    /* The Linux apply tool confirms owned-server shutdown directly. Wine's
     * start.exe exit status cannot establish native Unix child completion. */
    if(!is_wine()&&!stop_server()){log_message(message);failure=L"Could not stop this installation's local server";goto done;}
    failure=L"Could not start the separate updater";
    if(!update_command(command,work,archive,hash_w,FALSE))goto done;
    _snwprintf(name,CAP,L"%ls\\apply-command.txt",work);name[CAP-1]=0;
    HANDLE output=CreateFileW(name,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);DWORD written=0;
    DWORD bytes=(DWORD)(wcslen(command)*sizeof(WCHAR));
    BOOL saved=output!=INVALID_HANDLE_VALUE&&WriteFile(output,command,bytes,&written,NULL)&&written==bytes;
    if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);if(!saved)goto done;
    path(source,L"ReRiff-Updater.exe");_snwprintf(name,CAP,L"%ls\\ReRiff-Updater.exe",work);name[CAP-1]=0;
    if(!CopyFileW(source,name,TRUE))goto done;
    _snwprintf(command,CAP,L"\"%ls\" %lu \"%ls\" \"%ls\"",name,(unsigned long)GetCurrentProcessId(),work,folder);command[CAP-1]=0;
    if(!spawn(command,TRUE,&process))goto done;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    update_handoff=TRUE;wcscpy(message,L"Installing the verified update. The launcher will reopen automatically.");ok=TRUE;
done:
    InternetCloseHandle(session);
    if(!ok){_snwprintf(message,1024,L"%ls. Installed files were retained. Details: %ls",failure,work);message[1023]=0;}
    return ok;
}
