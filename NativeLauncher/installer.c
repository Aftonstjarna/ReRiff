#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <wchar.h>
#include <stdio.h>

#define CAP 32768
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR arguments,int show) {
    (void)instance;(void)previous;(void)show;
    WCHAR root[CAP],command[CAP],game[CAP]=L"",log[CAP];
    GetModuleFileNameW(NULL,root,CAP);WCHAR *slash=wcsrchr(root,L'\\');if(!slash)return 1;*slash=0;
    BOOL interactive=!*arguments;
    if(interactive){
        OPENFILENAMEW dialog;ZeroMemory(&dialog,sizeof(dialog));dialog.lStructSize=sizeof(dialog);
        dialog.lpstrFilter=L"Riff Racer (Game.exe)\0Game.exe\0";dialog.lpstrFile=game;dialog.nMaxFile=CAP;
        dialog.lpstrTitle=L"Choose Riff Racer's Game.exe — close the game before installing";
        dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
        if(!GetOpenFileNameW(&dialog))return 0;
        WCHAR *end=wcsrchr(game,L'\\');if(!end)return 1;*end=0;
        _snwprintf(command,CAP,L"\"%ls\\python\\python.exe\" \"%ls\\installer.py\" --platform windows --game-dir \"%ls\"",root,root,game);
    }else{
        _snwprintf(command,CAP,L"\"%ls\\python\\python.exe\" \"%ls\\installer.py\" --platform windows %ls",root,root,arguments);
    }
    command[CAP-1]=0;
    _snwprintf(log,CAP,L"%ls\\installer.log",root);log[CAP-1]=0;
    SECURITY_ATTRIBUTES security={sizeof(security),NULL,TRUE};
    HANDLE output=CreateFileW(log,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    STARTUPINFOW start;PROCESS_INFORMATION process;ZeroMemory(&start,sizeof(start));ZeroMemory(&process,sizeof(process));start.cb=sizeof(start);
    start.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;start.wShowWindow=SW_HIDE;
    start.hStdOutput=output;start.hStdError=output;start.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    if(output==INVALID_HANDLE_VALUE || !CreateProcessW(NULL,command,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,root,&start,&process)){
        if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);
        if(interactive)MessageBoxW(NULL,L"Extract the complete Windows ZIP before running setup. Portable Python and installer.py must be beside the installer.",L"ReRiff setup failed",MB_OK|MB_ICONERROR);
        return 1;
    }
    CloseHandle(output);CloseHandle(process.hThread);WaitForSingleObject(process.hProcess,INFINITE);
    DWORD code=1;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);
    if(interactive){
        if(code)MessageBoxW(NULL,L"Installation failed. See installer.log beside ReRiff-Setup.exe for details. Existing saves are retained.",L"ReRiff setup",MB_OK|MB_ICONERROR);
        else MessageBoxW(NULL,L"ReRiff installed. Use ReRiff-Launcher.exe beside Game.exe. Your settings and saved server data were preserved.",L"ReRiff setup",MB_OK|MB_ICONINFORMATION);
    }
    return (int)code;
}
