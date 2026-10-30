#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR arguments,int show) {
    (void)instance;(void)previous;(void)arguments;(void)show;
    STARTUPINFOW startup={0};PROCESS_INFORMATION process={0};startup.cb=sizeof(startup);
    WCHAR command[]=L"ReRiff-Launcher.exe";
    if(!CreateProcessW(NULL,command,NULL,NULL,FALSE,0,NULL,NULL,&startup,&process))return 1;
    CloseHandle(process.hThread);
    HWND window=NULL;
    for(int i=0;i<100&&!window;i++){window=FindWindowW(L"ReRiffNativeLauncher",NULL);Sleep(100);}
    BOOL passed=window!=NULL;
    if(window){
        HWND save=GetDlgItem(window,105),only=GetDlgItem(window,108),fallback=GetDlgItem(window,102);
        for(int i=0;i<100&&!IsWindowEnabled(save);i++)Sleep(100);
        passed=passed&&IsWindowEnabled(save)&&GetDlgItem(window,109)&&GetDlgItem(window,110);
        passed=passed&&SendMessageW(only,BM_GETCHECK,0,0)==BST_CHECKED&&!IsWindowEnabled(fallback);
        SendMessageW(only,BM_SETCHECK,BST_UNCHECKED,0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(108,BN_CLICKED),(LPARAM)only);
        passed=passed&&IsWindowEnabled(fallback);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(105,BN_CLICKED),(LPARAM)save);
        WCHAR value[32];GetPrivateProfileStringW(L"server",L"external_only",L"",value,32,L".\\server.ini");
        passed=passed&&!wcscmp(value,L"false");
        SendMessageW(window,WM_CLOSE,0,0);
    }
    if(WaitForSingleObject(process.hProcess,5000)!=WAIT_OBJECT_0){TerminateProcess(process.hProcess,1);passed=FALSE;}
    CloseHandle(process.hProcess);
    FILE *output=fopen("controls-test.txt","w");
    if(output){fputs(passed?"PASS: external-only checkbox, fallback enable/disable, persistence, stop and update controls\n":"FAIL\n",output);fclose(output);}
    return passed?0:1;
}
