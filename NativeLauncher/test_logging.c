#define wWinMain launcher_main
#include "launcher.c"
#undef wWinMain
#include <assert.h>
#undef assert
#define assert(c) do { if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);} } while(0)
int main(void){
 WCHAR previous[CAP],temp[CAP],root[CAP],logs[CAP],file[CAP],marker[CAP];GetCurrentDirectoryW(CAP,previous);GetTempPathW(CAP,temp);GetTempFileNameW(temp,L"rrl",0,root);DeleteFileW(root);assert(CreateDirectoryW(root,NULL));assert(SetCurrentDirectoryW(root));
 wcscpy(folder,root);path(ini,L"server.ini");path(logs,L"logs");path(file,L"logs\\ReRiff-launcher.log");path(marker,L"localserver\\client-channel.ini");CreateDirectoryW(L"localserver",NULL);WritePrivateProfileStringW(L"client",L"channel",L"stable",marker);load_channel();
 log_message(L"disabled");assert(GetFileAttributesW(logs)==INVALID_FILE_ATTRIBUTES);
 WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LoggingTest";RegisterClassW(&cls);busy=TRUE;
 HWND hwnd=CreateWindowW(cls.lpszClassName,L"",WS_OVERLAPPED,0,0,660,615,NULL,NULL,cls.hInstance,NULL);assert(hwnd);
 assert(SendMessageW(logging_checkbox,BM_GETCHECK,0,0)==BST_UNCHECKED);assert(SendMessageW(launcher_tabs,TCM_GETITEMCOUNT,0,0)==3);
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_CHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);assert(rr_logging_ini(ini));log_message(L"enabled");assert(GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES);
 WIN32_FILE_ATTRIBUTE_DATA before,after;assert(GetFileAttributesExW(file,GetFileExInfoStandard,&before));
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);assert(!rr_logging_ini(ini));log_message(L"must not append");assert(GetFileAttributesExW(file,GetFileExInfoStandard,&after));assert(before.nFileSizeLow==after.nFileSizeLow&&GetFileAttributesW(logs)!=INVALID_FILE_ATTRIBUTES);
 DestroyWindow(hwnd);DeleteFileW(file);RemoveDirectoryW(logs);DeleteFileW(ini);DeleteFileW(marker);RemoveDirectoryW(L"localserver");SetCurrentDirectoryW(previous);RemoveDirectoryW(root);
 puts("PASS: native Diagnostics toggle, no disabled log directory, enabled writes and preserved disabled logs");return 0;
}
