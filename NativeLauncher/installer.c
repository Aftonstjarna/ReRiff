#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdio.h>

#define CAP 32768
#include "python_runtime.h"
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

    }

    if(!interactive){
        int count=0;WCHAR **args=CommandLineToArgvW(GetCommandLineW(),&count);
        if(args){for(int i=1;i+1<count;i++)if(!wcscmp(args[i],L"--game-dir")){wcsncpy(game,args[i+1],CAP-1);game[CAP-1]=0;break;}LocalFree(args);}
    }
    if(!*game){if(interactive)MessageBoxW(NULL,L"Choose a game folder before installing Python.",L"ReRiff setup",MB_OK|MB_ICONERROR);return 1;}
    _snwprintf(command,CAP,L"%ls\\Game.exe",game);command[CAP-1]=0;
    if(GetFileAttributesW(command)==INVALID_FILE_ATTRIBUTES){if(interactive)MessageBoxW(NULL,L"Game.exe was not found in that folder.",L"ReRiff setup",MB_OK|MB_ICONERROR);return 1;}
    if(!ensure_private_python(NULL,game,interactive))return 1;
    const WCHAR *bootstrap=L"import runpy,sys;sys.path.insert(0,sys.argv[1]);sys.argv=sys.argv[2:];runpy.run_path(sys.argv[0],run_name='__main__')";
    if(interactive)_snwprintf(command,CAP,L"\"%ls\\localserver\\windows-python\\python.exe\" -X utf8 -c \"%ls\" \"%ls\" \"%ls\\installer.py\" --platform windows --game-dir \"%ls\"",game,bootstrap,root,root,game);
    else _snwprintf(command,CAP,L"\"%ls\\localserver\\windows-python\\python.exe\" -X utf8 -c \"%ls\" \"%ls\" \"%ls\\installer.py\" --platform windows %ls",game,bootstrap,root,root,arguments);
    command[CAP-1]=0;
    _snwprintf(log,CAP,L"%ls\\logs",*game?game:root);log[CAP-1]=0;CreateDirectoryW(log,NULL);
    _snwprintf(log,CAP,L"%ls\\logs\\installer.log",*game?game:root);log[CAP-1]=0;
    SECURITY_ATTRIBUTES security={sizeof(security),NULL,TRUE};
    HANDLE output=CreateFileW(log,GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    STARTUPINFOW start;PROCESS_INFORMATION process;ZeroMemory(&start,sizeof(start));ZeroMemory(&process,sizeof(process));start.cb=sizeof(start);
    start.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;start.wShowWindow=SW_HIDE;
    start.hStdOutput=output;start.hStdError=output;start.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    if(output==INVALID_HANDLE_VALUE || !CreateProcessW(NULL,command,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,root,&start,&process)){
        if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);
        if(interactive)MessageBoxW(NULL,L"Extract the complete Windows ZIP before running setup. installer.py must be beside the installer, and the private Python installation must be complete.",L"ReRiff setup failed",MB_OK|MB_ICONERROR);
        return 1;
    }
    CloseHandle(output);CloseHandle(process.hThread);WaitForSingleObject(process.hProcess,INFINITE);
    DWORD code=1;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);
    if(interactive){
        if(code){_snwprintf(command,CAP,L"Installation failed. See %ls for details. Existing saves are retained.",log);MessageBoxW(NULL,command,L"ReRiff setup",MB_OK|MB_ICONERROR);}
        else if(MessageBoxW(NULL,L"ReRiff installed. Use ReRiff-Launcher.exe beside Game.exe. Your settings and saved server data were preserved.\n\nA ReRiff GitHub shortcut is saved beside the game. Open the GitHub repository now?",L"ReRiff setup",MB_YESNO|MB_DEFBUTTON2|MB_ICONINFORMATION)==IDYES)
            ShellExecuteW(NULL,L"open",L"https://github.com/Aftonstjarna/ReRiff",NULL,NULL,SW_SHOWNORMAL);
    }
    return (int)code;
}
