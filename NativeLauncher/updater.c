/* Run from a private temporary copy so the installed updater can be replaced. */
#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdlib.h>
#define CAP 32768

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR arguments,int show) {
    (void)instance;(void)previous;(void)arguments;(void)show;
    int count=0;LPWSTR *args=CommandLineToArgvW(GetCommandLineW(),&count);
    if(count!=4){if(args)LocalFree(args);return 2;}
    DWORD pid=(DWORD)wcstoul(args[1],NULL,10);
    WCHAR work[CAP],game[CAP],command[CAP],name[CAP];
    wcsncpy(work,args[2],CAP-1);work[CAP-1]=0;wcsncpy(game,args[3],CAP-1);game[CAP-1]=0;LocalFree(args);
    HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,pid);
    if(!parent&&GetLastError()!=ERROR_INVALID_PARAMETER)return 3;
    if(parent){DWORD waited=WaitForSingleObject(parent,120000);CloseHandle(parent);if(waited!=WAIT_OBJECT_0)return 3;}
    _snwprintf(name,CAP,L"%ls\\apply-command.txt",work);name[CAP-1]=0;
    HANDLE input=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    DWORD bytes=0;BOOL loaded=input!=INVALID_HANDLE_VALUE&&ReadFile(input,command,sizeof(command)-sizeof(WCHAR),&bytes,NULL);
    if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);
    if(!loaded||bytes%sizeof(WCHAR))return 4;command[bytes/sizeof(WCHAR)]=0;
    STARTUPINFOW start={0};PROCESS_INFORMATION process={0};start.cb=sizeof(start);start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=SW_HIDE;
    BOOL ok=CreateProcessW(NULL,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,game,&start,&process);
    if(ok){
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
        ULONGLONG deadline=GetTickCount64()+120000;
        for(;;){
            _snwprintf(name,CAP,L"%ls\\success",work);name[CAP-1]=0;
            if(GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES)break;
            _snwprintf(name,CAP,L"%ls\\failure",work);name[CAP-1]=0;
            if(GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES)break;
            if(GetTickCount64()>deadline){
                MessageBoxW(NULL,L"The updater has not reported completion. Check update.log before reopening the launcher.",L"ReRiff update pending",MB_OK|MB_ICONERROR);return 5;
            }
            Sleep(100);
        }
    }
    _snwprintf(name,CAP,L"%ls\\success",work);name[CAP-1]=0;
    ok=ok&&GetFileAttributesW(name)!=INVALID_FILE_ATTRIBUTES;
    if(!ok){
        _snwprintf(command,CAP,L"The update could not finish. Existing files were retained or rolled back. Details: %ls\\update.log",work);
        MessageBoxW(NULL,command,L"ReRiff update failed",MB_OK|MB_ICONERROR);
    }
    _snwprintf(command,CAP,L"\"%ls\\ReRiff-Launcher.exe\"",game);command[CAP-1]=0;
    start.wShowWindow=SW_SHOWNORMAL;
    if(CreateProcessW(NULL,command,NULL,NULL,FALSE,0,NULL,game,&start,&process)){
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
    }
    return ok?0:1;
}
