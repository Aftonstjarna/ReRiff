/* Windows-only, consented bootstrap of the pinned private Python runtime.
 * Shared by setup and launcher; never installs into PATH or the registry. */
#include "logging.h"
#include <wincrypt.h>
#include <wininet.h>
#define PYTHON_RUNTIME_URL L"https://www.python.org/ftp/python/3.13.17/python-3.13.17-embed-amd64.zip"
#define PYTHON_RUNTIME_SHA "9e35319bffedab8b36ad44d8f377cb4f1733bcec257a0b6d04d94f01486ddb38"
static int (WINAPI *python_runtime_prompt)(HWND,LPCWSTR,LPCWSTR,UINT)=MessageBoxW;
static BOOL python_file_hash(const WCHAR *path,const char *expected){
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;BOOL ok=FALSE;BYTE data[65536],result[32];DWORD count,size=32;char hex[65];
    if(CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){
        ok=TRUE;for(;;){if(!ReadFile(file,data,sizeof(data),&count,NULL)){ok=FALSE;break;}if(!count)break;if(!CryptHashData(hash,data,count,0)){ok=FALSE;break;}}
        ok=ok&&CryptGetHashParam(hash,HP_HASHVAL,result,&size,0);if(ok){for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",result[i]);ok=!_stricmp(hex,expected);}
    }
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);CloseHandle(file);return ok;
}
static BOOL private_python_ready(const WCHAR *root){
    static const WCHAR *names[]={L"python.exe",L"python313.dll",L"python313.zip"};
    static const char *hashes[]={"7c2fc2b303a4ee3cc14cc17b47bc6e8bca98c78792edd123b846a560da44b203","26026530507a7389e7892ce130e7c1753b32a2df03ec3721d45ad8c1705d40eb","58af5bf33dcfdedf72f60b0926fdb619cc2960c9d4200677531d35f626c8ee24"};
    WCHAR name[CAP];for(int i=0;i<3;i++){if(_snwprintf(name,CAP,L"%ls\\%ls",root,names[i])<0||!python_file_hash(name,hashes[i]))return FALSE;}return TRUE;
}
static void python_remove_stage(const WCHAR *root){
    WCHAR pattern[CAP],path[CAP];if(_snwprintf(pattern,CAP,L"%ls\\*",root)<0)return;
    WIN32_FIND_DATAW data;HANDLE list=FindFirstFileW(pattern,&data);
    if(list!=INVALID_HANDLE_VALUE){do{if(!wcscmp(data.cFileName,L".")||!wcscmp(data.cFileName,L".."))continue;
        if(_snwprintf(path,CAP,L"%ls\\%ls",root,data.cFileName)<0)continue;
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)RemoveDirectoryW(path);else python_remove_stage(path);}else DeleteFileW(path);
    }while(FindNextFileW(list,&data));FindClose(list);}RemoveDirectoryW(root);
}
static BOOL python_download(const WCHAR *destination){
    HINTERNET internet=InternetOpenW(L"ReRiff Python setup",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);if(!internet)return FALSE;
    DWORD timeout=30000;InternetSetOptionW(internet,INTERNET_OPTION_CONNECT_TIMEOUT,&timeout,sizeof(timeout));InternetSetOptionW(internet,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(timeout));
    HINTERNET request=InternetOpenUrlW(internet,PYTHON_RUNTIME_URL,NULL,0,INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_UI,0);
    DWORD status=0,size=sizeof(status),total=0,count,written;BOOL ok=FALSE;
    if(request&&HttpQueryInfoW(request,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&status,&size,NULL)&&status==200){
        HANDLE file=CreateFileW(destination,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE){BYTE block[65536];ok=TRUE;for(;;){if(!InternetReadFile(request,block,sizeof(block),&count)){ok=FALSE;break;}if(!count)break;if(count>32*1024*1024-total||!WriteFile(file,block,count,&written,NULL)||written!=count){ok=FALSE;break;}total+=count;}CloseHandle(file);}
    }
    if(request)InternetCloseHandle(request);InternetCloseHandle(internet);return ok&&python_file_hash(destination,PYTHON_RUNTIME_SHA);
}
static BOOL python_ps_literal(const WCHAR *source,WCHAR *out,size_t capacity){
    size_t n=0;for(;*source;source++){if(n+(*source==L'\''?2:1)>=capacity)return FALSE;out[n++]=*source;if(*source==L'\'')out[n++]=L'\'';}out[n]=0;return TRUE;
}
static int python_logging_override=-1;
static void python_runtime_log(const WCHAR *game,const char *text){
    if(!(python_logging_override>=0?python_logging_override:rr_logging_enabled(game,FALSE)))return;
    WCHAR path[CAP];if(_snwprintf(path,CAP,L"%ls\\logs",game)<0)return;CreateDirectoryW(path,NULL);
    if(_snwprintf(path,CAP,L"%ls\\logs\\ReRiff-python-setup.log",game)<0)return;
    HANDLE file=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);DWORD written;
    if(file!=INVALID_HANDLE_VALUE){WriteFile(file,text,(DWORD)strlen(text),&written,NULL);WriteFile(file,"\r\n",2,&written,NULL);CloseHandle(file);}
}
static BOOL ensure_private_python(HWND owner,const WCHAR *game,BOOL prompt){
    WCHAR local[CAP],runtime[CAP],stage[CAP],archive[CAP],expanded[CAP],script[CAP],system[CAP],command[CAP],a[CAP],b[CAP],old[CAP]=L"";
    if(_snwprintf(local,CAP,L"%ls\\localserver",game)<0||_snwprintf(runtime,CAP,L"%ls\\windows-python",local)<0)return FALSE;
    if(private_python_ready(runtime))return TRUE;
    if(!prompt||python_runtime_prompt(owner,L"Install Python for ReRiff?\n\nSetup will download a verified private Python runtime from python.org into this game's localserver\\windows-python folder. It is used for Windows setup, updates and optional server controls.\n\nYour system Python, PATH and registry will stay unchanged.",L"ReRiff — Install private Python",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)!=IDYES)return FALSE;
    python_runtime_log(game,"Private Python 3.13.17 installation requested.");
    CreateDirectoryW(local,NULL);
    if(_snwprintf(stage,CAP,L"%ls\\.python-install-%lu-%llu",local,(unsigned long)GetCurrentProcessId(),(unsigned long long)GetTickCount64())<0||!CreateDirectoryW(stage,NULL))return FALSE;
    BOOL ok=FALSE;
    if(_snwprintf(archive,CAP,L"%ls\\python.zip",stage)<0||_snwprintf(expanded,CAP,L"%ls\\runtime",stage)<0||_snwprintf(script,CAP,L"%ls\\extract.ps1",stage)<0)goto done;
    if(!python_download(archive))goto done;
    python_runtime_log(game,"Python archive checksum verified; extracting with Windows PowerShell.");
    if(!python_ps_literal(archive,a,CAP)||!python_ps_literal(expanded,b,CAP))goto done;
    if(_snwprintf(command,CAP,L"$ErrorActionPreference = 'Stop'\r\nExpand-Archive -LiteralPath '%ls' -DestinationPath '%ls'\r\n",a,b)<0)goto done;
    HANDLE output=CreateFileW(script,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);DWORD written;
    if(output==INVALID_HANDLE_VALUE)goto done;
    const WCHAR bom=0xfeff;BOOL saved=WriteFile(output,&bom,2,&written,NULL)&&WriteFile(output,command,(DWORD)(wcslen(command)*2),&written,NULL);CloseHandle(output);if(!saved)goto done;
    GetSystemDirectoryW(system,CAP);
    if(_snwprintf(command,CAP,L"\"%ls\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%ls\"",system,script)<0)goto done;
    STARTUPINFOW start;PROCESS_INFORMATION process;ZeroMemory(&start,sizeof(start));start.cb=sizeof(start);start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=SW_HIDE;
    if(!CreateProcessW(NULL,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,stage,&start,&process))goto done;
    CloseHandle(process.hThread);DWORD code=1;if(WaitForSingleObject(process.hProcess,90000)==WAIT_OBJECT_0)GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);
    if(code||!private_python_ready(expanded))goto done;
    if(_snwprintf(a,CAP,L"%ls\\python313._pth",expanded)<0)goto done;output=CreateFileW(a,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(output==INVALID_HANDLE_VALUE)goto done;
    static const char pth[]="python313.zip\n.\n..\n../python-libs\nimport site\n";saved=WriteFile(output,pth,sizeof(pth)-1,&written,NULL)&&written==sizeof(pth)-1;CloseHandle(output);if(!saved)goto done;
    old[0]=0;
    if(GetFileAttributesW(runtime)!=INVALID_FILE_ATTRIBUTES){if(_snwprintf(old,CAP,L"%ls\\previous-runtime",stage)<0||!MoveFileW(runtime,old))goto done;}
    if(!MoveFileW(expanded,runtime)){if(*old)MoveFileW(old,runtime);goto done;}ok=TRUE;
 done:
    if(ok||!*old||GetFileAttributesW(old)==INVALID_FILE_ATTRIBUTES)python_remove_stage(stage);
    else python_runtime_log(game,"Private runtime rollback could not complete; previous runtime retained in the temporary installation folder.");
    python_runtime_log(game,ok?"Private Python verified and installed.":"Private Python installation failed; download, checksum or extraction did not complete.");
    if(!ok)python_runtime_prompt(owner,L"Python installation did not complete. Check your connection and available disk space, then retry. The existing client and saves were retained.",L"ReRiff Python setup",MB_OK|MB_ICONERROR);
    return ok;
}
