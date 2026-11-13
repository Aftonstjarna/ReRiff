#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <string.h>
static DWORD response=200,length=0;static BOOL consumed;
static HINTERNET WINAPI fake_open(HINTERNET h,LPCWSTR u,LPCWSTR headers,DWORD n,DWORD flags,DWORD_PTR context){(void)h;(void)u;(void)headers;(void)n;(void)flags;(void)context;consumed=FALSE;return (HINTERNET)1;}
static BOOL WINAPI fake_query(HINTERNET h,DWORD type,LPVOID out,LPDWORD size,LPDWORD index){(void)h;(void)size;(void)index;*(DWORD *)out=(type&0xffff)==HTTP_QUERY_STATUS_CODE?response:length;return TRUE;}
static BOOL WINAPI fake_read(HINTERNET h,LPVOID out,DWORD size,LPDWORD count){(void)h;(void)size;*count=consumed?0:4;if(*count)memcpy(out,"test",4);consumed=TRUE;return TRUE;}
static BOOL WINAPI fake_close(HINTERNET h){(void)h;return TRUE;}
#define InternetOpenUrlW fake_open
#define HttpQueryInfoW fake_query
#define InternetReadFile fake_read
#define InternetCloseHandle fake_close
#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain
#define CHECK(c) do{if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    WCHAR tmp[CAP],dest[CAP];CHECK(GetTempPathW(CAP,tmp));CHECK(GetTempFileNameW(tmp,L"rud",0,folder));CHECK(DeleteFileW(folder)&&CreateDirectoryW(folder,NULL));path(ini,L"server.ini");CHECK(WritePrivateProfileStringW(L"diagnostics",L"logging_enabled",L"false",ini));path(dest,L"package.run");
    length=140924930;CHECK(!update_download(NULL,L"https://example.invalid/package",dest,64*1024*1024));CHECK(wcsstr(update_problem,L"64.0 MiB")&&GetFileAttributesW(dest)==INVALID_FILE_ATTRIBUTES);
    length=0;CHECK(!update_download(NULL,L"https://example.invalid/package",dest,1));CHECK(wcsstr(update_problem,L"limit")&&GetFileAttributesW(dest)==INVALID_FILE_ATTRIBUTES);
    response=404;CHECK(!update_download(NULL,L"https://example.invalid/package",dest,100));CHECK(wcsstr(update_problem,L"not found")&&wcsstr(update_problem,L"404"));
    response=200;length=4;CHECK(update_download(NULL,L"https://example.invalid/package",dest,100));CHECK(!*update_problem);
    CHECK(!update_download(NULL,L"https://example.invalid/package",dest,100));CHECK(GetFileAttributesW(dest)!=INVALID_FILE_ATTRIBUTES);
    DeleteFileW(dest);DeleteFileW(ini);CHECK(RemoveDirectoryW(folder));puts("PASS: declared/streamed size-limit reasons, HTTP errors, successful download and preservation of pre-existing destinations.");return 0;
}
