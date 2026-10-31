#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#define CAP 32768
int wmain(int argc,WCHAR **args) {
#ifdef RERIFF_TEST_RELAUNCH
    (void)argc;(void)args;
    FILE *marker=_wfopen(L"reopened",L"wb");if(!marker)return 1;fputs("visible launcher restarted",marker);fclose(marker);return 0;
#else
    if(argc!=4)return 1;
    WCHAR command[CAP],marker[CAP];
    _snwprintf(command,CAP,L"\"%ls\" %lu \"%ls\" \"%ls\"",args[1],(unsigned long)GetCurrentProcessId(),args[2],args[3]);command[CAP-1]=0;
    STARTUPINFOW start={0};PROCESS_INFORMATION process={0};start.cb=sizeof(start);
    if(!CreateProcessW(NULL,command,NULL,NULL,FALSE,0,NULL,args[3],&start,&process))return 2;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    Sleep(500);
    _snwprintf(marker,CAP,L"%ls\\success",args[2]);marker[CAP-1]=0;
    return GetFileAttributesW(marker)==INVALID_FILE_ATTRIBUTES?0:3;
#endif
}
