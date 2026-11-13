/* Downloads are restricted to the official GitHub release/tag names. The
 * existing transactional installer applies only after this process exits. */
#include <wincrypt.h>
#include <stdlib.h>
#include <ctype.h>
static BOOL update_handoff=FALSE;
static BOOL stop_server(void);
static int automatic_progress_stage=PROGRESS_CHECK;

static BOOL update_download(HINTERNET session,const WCHAR *url,const WCHAR *destination,DWORD maximum) {
    update_problem[0]=0;if(automatic_update&&progress_cancelled())return FALSE;
    HINTERNET req=InternetOpenUrlW(session,url,L"Accept: application/vnd.github+json\r\n",(DWORD)-1,
        INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_UI|INTERNET_FLAG_NO_COOKIES,0);
    DWORD error=req?0:GetLastError(),code=0,size=sizeof(code),read=0,written=0,total=0;BOOL ok=FALSE,created=FALSE;
    if(req&&HttpQueryInfoW(req,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&code,&size,NULL)&&code==200){
        DWORD expected_bytes=0,expected_size=sizeof(expected_bytes);
        HttpQueryInfoW(req,HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,&expected_bytes,&expected_size,NULL);
        if(expected_bytes>maximum){_snwprintf(update_problem,512,L"The download is %.1f MiB; this launcher allows %.1f MiB. Install the newer release manually.",expected_bytes/(1024.0*1024.0),maximum/(1024.0*1024.0));}
        else {
            HANDLE file=CreateFileW(destination,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
            if(file==INVALID_HANDLE_VALUE)error=GetLastError();
            else {
                char block[65536];ok=created=TRUE;
                for(;;){
                    if(automatic_update&&progress_cancelled()){ok=FALSE;break;}
                    if(!InternetReadFile(req,block,sizeof(block),&read)){error=GetLastError();ok=FALSE;break;}
                    if(!read)break;
                    if(read>maximum-total){_snwprintf(update_problem,512,L"The download exceeded this launcher's %.1f MiB limit. Install the newer release manually.",maximum/(1024.0*1024.0));ok=FALSE;break;}
                    if(!WriteFile(file,block,read,&written,NULL)||written!=read){error=GetLastError();_snwprintf(update_problem,512,L"Could not save the download (system error %lu). Check disk space and folder permissions.",(unsigned long)error);ok=FALSE;break;}
                    total+=read;
                    if(automatic_update)progress_report(automatic_progress_stage,expected_bytes?(int)((ULONGLONG)total*100/expected_bytes):-1);
                }
                CloseHandle(file);
            }
        }
    }else if(req&&code==0)error=GetLastError();
    if(req)InternetCloseHandle(req);
    if(!ok){
        if(!*update_problem){if(code==404)wcscpy(update_problem,L"The release download was not found on GitHub (HTTP 404).");
            else if(code&&code!=200)_snwprintf(update_problem,512,L"GitHub refused the download (HTTP %lu).",(unsigned long)code);
            else _snwprintf(update_problem,512,L"The download could not complete (HTTP %lu, system error %lu). Check the connection and available disk space.",(unsigned long)code,(unsigned long)error);}
        update_problem[511]=0;log_message(update_problem);if(created)DeleteFileW(destination);
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
        WCHAR host_tool[CAP],runtime[CAP],host_runtime[CAP];
        if(prepare)path(runtime,L"localserver\\linux-runtime.sh");
        else _snwprintf(runtime,CAP,L"%ls\\linux-runtime.sh",work);
        runtime[CAP-1]=0;
        if(!update_host_path(runtime,host_runtime))return FALSE;
        if(!update_host_path(tool,host_tool)||!update_host_path(work,host_work)||!update_host_path(folder,host_game)
           ||!update_host_path(archive,host_archive))return FALSE;
        GetSystemDirectoryW(system,CAP);
        if(prepare)_snwprintf(out,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" update --work \"%ls\" --prepare --archive \"%ls\" --sha256 %ls --game-dir \"%ls\" --platform linux",system,host_runtime,host_work,host_archive,hash,host_game);
        else _snwprintf(out,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" update --work \"%ls\"",system,host_runtime,host_work);
    }else{
        if(prepare)path(python,L"localserver\\windows-python\\python.exe");
        else {_snwprintf(python,CAP,L"%ls\\runtime\\python.exe",work);python[CAP-1]=0;}
        if(GetFileAttributesW(python)==INVALID_FILE_ATTRIBUTES)return FALSE;
        if(prepare)_snwprintf(out,CAP,L"\"%ls\" -X utf8 \"%ls\" --work \"%ls\" --prepare --archive \"%ls\" --sha256 %ls --game-dir \"%ls\" --platform windows",python,tool,work,archive,hash,folder);
        else _snwprintf(out,CAP,L"\"%ls\" -X utf8 \"%ls\" --work \"%ls\"",python,tool,work);
    }
    if(prepare){
        size_t length=wcslen(out);
        _snwprintf(out+length,CAP-length,L" --channel %ls",requested_experimental?L"experimental":L"stable");
    }
    out[CAP-1]=0;return TRUE;
}

#include "release_channel.h"
#define UPDATE_JSON_LIMIT (2*1024*1024)
static BOOL save_auto_resume(const WCHAR *work) {
    WCHAR resume[CAP],name[CAP];
    int length=_snwprintf(resume,CAP,L"\"%ls\\ReRiff-Launcher.exe\" --skip-launcher --no-auto-update -- \"%ls\\Game.exe\"%ls%ls",folder,folder,*game_arguments?L" ":L"",game_arguments);
    if(length<0||length>=CAP)return FALSE;
    _snwprintf(name,CAP,L"%ls\\resume-command.txt",work);name[CAP-1]=0;
    HANDLE output=CreateFileW(name,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);DWORD written=0;
    DWORD bytes=(DWORD)(length*sizeof(WCHAR));
    BOOL ok=output!=INVALID_HANDLE_VALUE&&WriteFile(output,resume,bytes,&written,NULL)&&written==bytes;
    if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);return ok;
}

static BOOL check_updates(void) {
    update_problem[0]=update_failure[0]=update_work[0]=0;
    if(!is_wine()&&!ensure_private_python(window,folder,TRUE)){wcscpy(message,L"Python setup was canceled or failed; the installed client was retained.");wcscpy(update_failure,message);return FALSE;}
    WCHAR temporary[CAP],work[CAP],api[CAP],sums[CAP],archive[CAP],url[CAP],filename[256],hash_w[65],command[CAP],name[CAP],source[CAP];
    BOOL ok=FALSE;PROCESS_INFORMATION process;char expected[65]="",*body=NULL;GithubRelease selected={0};ReleasePackage package;
    const WCHAR *failure=L"Could not read the GitHub release list";
    work[0]=0;
    HINTERNET session=InternetOpenW(L"ReRiffLauncher/" RERIFF_VERSION,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    if(!session){wcscpy(message,L"Could not connect to GitHub.");wcscpy(update_failure,message);return FALSE;}
    body=HeapAlloc(GetProcessHeap(),0,UPDATE_JSON_LIMIT);if(!body)goto done;
    DWORD limit=15000;InternetSetOptionW(session,INTERNET_OPTION_CONNECT_TIMEOUT,&limit,sizeof(limit));
    InternetSetOptionW(session,INTERNET_OPTION_RECEIVE_TIMEOUT,&limit,sizeof(limit));
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rru",0,work))goto done;
    DeleteFileW(work);if(!CreateDirectoryW(work,NULL))goto done;
    for(unsigned page=1;;page++){
        /* GitHub returns at most 100 releases per page. Publication order can
         * differ from tag/commit order, so inspect every page before choosing. */
        _snwprintf(api,CAP,L"%ls\\release-%u.json",work,page);api[CAP-1]=0;
        if(requested_experimental)_snwprintf(url,CAP,L"https://api.github.com/repos/Aftonstjarna/ReRiff/releases?per_page=100&page=%u",page);
        else wcscpy(url,L"https://api.github.com/repos/Aftonstjarna/ReRiff/releases/latest");
        if(!update_download(session,url,api,UPDATE_JSON_LIMIT-1))goto done;
        FILE *file=_wfopen(api,L"rb");if(!file)goto done;
        size_t n=fread(body,1,UPDATE_JSON_LIMIT-1,file);body[n]=0;fclose(file);
        unsigned count=0;
        failure=L"GitHub returned invalid release metadata";
        if(!select_github_release(body,requested_experimental,&selected,&count))goto done;
        if(!requested_experimental||count<100)break;
        failure=L"The GitHub release list exceeded the update check limit";
        if(page>=100)goto done;
        failure=L"Could not read the next GitHub release page";
    }
    failure=requested_experimental?L"No published experimental release is available":L"No published stable release is available";
    if(!selected.flags)goto done;
    /* Metadata supplies the package identity and platform filename. Neither
     * needs to resemble the GitHub tag, or follow a particular version syntax. */
    char encoded_tag[768];WCHAR release_tag[768],metadata[CAP];
    failure=L"Could not read the selected release's package metadata";
    if(!release_tag_path(selected.tag,encoded_tag,sizeof(encoded_tag))
       ||!MultiByteToWideChar(CP_UTF8,0,encoded_tag,-1,release_tag,768))goto done;
    _snwprintf(metadata,CAP,L"%ls\\latest-release.json",work);metadata[CAP-1]=0;
    _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/%ls/release.json",release_tag);url[CAP-1]=0;
    if(!update_download(session,url,metadata,65535))goto done;
    FILE *file=_wfopen(metadata,L"rb");if(!file)goto done;
    size_t n=fread(body,1,UPDATE_JSON_LIMIT-1,file);body[n]=0;fclose(file);
    failure=L"The selected release does not list a valid installer for this platform";
    if(!release_package(body,is_wine(),&package))goto done;
    if(!release_needs_install(package.version,RERIFF_VERSION,requested_experimental,installed_experimental,force_auto_reinstall)){
        wcscpy(message,L"You have the latest published release on this channel.");ok=TRUE;goto done;
    }
    if(automatic_update&&force_auto_reinstall&&!strcmp(package.version,RERIFF_VERSION))
        log_message(L"TEST: unattended reinstall of the current published version; resume will skip the update check.");
    failure=L"Could not read the release checksums";
    if(!MultiByteToWideChar(CP_UTF8,0,package.filename,-1,filename,256))goto done;
    _snwprintf(sums,CAP,L"%ls\\SHA256SUMS",work);sums[CAP-1]=0;
    _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/%ls/SHA256SUMS",release_tag);url[CAP-1]=0;
    SetWindowTextW(status,L"Downloading and verifying the latest ReRiff update...");
    if(!update_download(session,url,sums,65535))goto done;
    char filename_u8[256],line[1024],listed[512],sum[65];
    WideCharToMultiByte(CP_UTF8,0,filename,-1,filename_u8,sizeof(filename_u8),NULL,NULL);
    file=_wfopen(sums,L"rb");if(!file)goto done;
    while(fgets(line,sizeof(line),file)){
        if(sscanf(line,"%64[0123456789abcdefABCDEF] %511s",sum,listed)==2&&strlen(sum)==64&&!strcmp(listed,filename_u8))strcpy(expected,sum);
    }fclose(file);if(!*expected)goto done;
    _snwprintf(archive,CAP,L"%ls\\%ls",work,filename);archive[CAP-1]=0;
    _snwprintf(url,CAP,L"https://github.com/Aftonstjarna/ReRiff/releases/download/%ls/%ls",release_tag,filename);url[CAP-1]=0;
    failure=L"Could not download the release package";
    if(automatic_update){automatic_progress_stage=PROGRESS_DOWNLOAD;progress_report(PROGRESS_DOWNLOAD,0);}
    if(!update_download(session,url,archive,256*1024*1024))goto done;
    failure=L"The release checksum did not match";
    if(automatic_update){if(progress_cancelled())goto done;progress_report(PROGRESS_VERIFY,-1);}
    if(!update_digest(archive,expected))goto done;
    if(automatic_update&&progress_cancelled())goto done;
    MultiByteToWideChar(CP_UTF8,0,expected,-1,hash_w,65);
    path(source,L"localserver\\apply-client-update.py");
    failure=L"Could not start installer preflight";
    if(automatic_update)progress_report(PROGRESS_PREPARE,-1);
    _snwprintf(name,CAP,L"%ls\\apply-client-update.py",work);name[CAP-1]=0;
    if(!is_wine()) {
        WCHAR policy[CAP],destination[CAP];path(policy,L"localserver\\logging_policy.py");
        _snwprintf(destination,CAP,L"%ls\\logging_policy.py",work);destination[CAP-1]=0;
        if(!CopyFileW(policy,destination,TRUE))goto done;
    }
    if((!is_wine()&&!CopyFileW(source,name,TRUE))||!update_command(command,work,archive,hash_w,TRUE)||!spawn(command,TRUE,&process))goto done;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    /* Wine start /unix can finish before the native Unix process. Use the
     * tool's completion markers, not the start.exe exit code, on both OSes. */
    ULONGLONG deadline=GetTickCount64()+120000;
    failure=L"Installer preflight failed";
    for(;;){
        if(automatic_update&&progress_cancelled())goto done;
        _snwprintf(name,CAP,L"%ls\\failure",work);name[CAP-1]=0;
        if(GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES)goto done;
        _snwprintf(name,CAP,L"%ls\\prepared.json",work);name[CAP-1]=0;
        if(GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES)break;
        if(GetTickCount64()>deadline){failure=L"Installer preflight timed out after two minutes";goto done;}Sleep(100);
    }
    if(automatic_update&&progress_cancelled())goto done;
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
    if(automatic_update&&!save_auto_resume(work))goto done;
    _snwprintf(command,CAP,L"\"%ls\" %lu \"%ls\" \"%ls\"",name,(unsigned long)GetCurrentProcessId(),work,folder);command[CAP-1]=0;
    if(automatic_update){if(!progress_commit())goto done;progress_report(PROGRESS_INSTALL,-1);}
    if(!spawn(command,TRUE,&process))goto done;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    update_remember(work);update_handoff=TRUE;wcscpy(message,automatic_update?L"Installing the verified update. The game will start automatically.":L"Installing the verified update. The launcher will reopen automatically.");ok=TRUE;
done:
    if(body)HeapFree(GetProcessHeap(),0,body);
    InternetCloseHandle(session);
    if(automatic_update&&progress_cancelled()){wcscpy(message,L"Update canceled; starting the installed game.");return FALSE;}
    if(!ok){update_remember(work);wcsncpy(update_work,work,CAP-1);update_work[CAP-1]=0;
        _snwprintf(update_failure,2048,L"%ls. %ls%lsInstalled files were retained.",failure,*update_problem?update_problem:L"",*update_problem?L"\r\n":L"");update_failure[2047]=0;
        wcscpy(message,L"Update failed; installed files were retained. Open Diagnostics → View update details.");}
    return ok;
}
