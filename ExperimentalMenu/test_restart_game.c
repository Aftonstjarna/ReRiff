/* Disposable native fixture: exercises real restart IPC, never the real game. */
#include "engine_menu.c"
#include <shellapi.h>
static int run_number;
static WCHAR directory[MAX_PATH];

static LRESULT CALLBACK test_window(HWND window,UINT event,WPARAM wp,LPARAM lp) {
    if(event==WM_TIMER){
        KillTimer(window,1);
        if(run_number==1){
            load_settings();wcscpy(pending.external,L"http://restart.example:5144");pending.only=TRUE;pending.enabled=FALSE;
            if(!save_settings()||!restart_game()){DestroyWindow(window);PostQuitMessage(9);}
        }else PostMessageW(window,WM_CLOSE,0,0);
        return 0;
    }
    if(event==WM_CLOSE){DestroyWindow(window);return 0;}
    if(event==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(window,event,wp,lp);
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR arguments,int show) {
    (void)previous;(void)arguments;(void)show;
    GetModuleFileNameW(NULL,directory,MAX_PATH);WCHAR *slash=wcsrchr(directory,L'\\');if(!slash)return 1;slash[1]=0;
    wcscpy(config_path,directory);wcscat(config_path,L"server.ini");
    wcscpy(log_path,directory);wcscat(log_path,L"restart-fixture.log");
    WCHAR file[MAX_PATH];swprintf(file,MAX_PATH,L"%lsrunning.lock",directory);
    HANDLE lock=CreateFileW(file,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(lock==INVALID_HANDLE_VALUE)return 2; /* Fail if old process still owns its lock. */
    swprintf(file,MAX_PATH,L"%lsruns.txt",directory);FILE *count=_wfopen(file,L"r");
    if(count){if(fscanf(count,"%d",&run_number)!=1)run_number=0;fclose(count);}
    run_number++;if(run_number>2)return 3;
    count=_wfopen(file,L"w");if(!count)return 4;fprintf(count,"%d",run_number);fclose(count);
    int argc;LPWSTR *argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    swprintf(file,MAX_PATH,L"%lsarguments-%d.bin",directory,run_number);
    HANDLE recorded=CreateFileW(file,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);DWORD written;
    if(recorded==INVALID_HANDLE_VALUE||!argv)return 5;
    for(int i=1;i<argc;i++)WriteFile(recorded,argv[i],(DWORD)((wcslen(argv[i])+1)*sizeof(WCHAR)),&written,NULL);
    CloseHandle(recorded);LocalFree(argv);
    if(run_number==2){
        load_settings();if(wcscmp(pending.external,L"http://restart.example:5144")||!pending.only||pending.enabled)return 6;
    }
    WNDCLASSW cls;ZeroMemory(&cls,sizeof(cls));cls.lpfnWndProc=test_window;cls.hInstance=instance;cls.lpszClassName=L"ReRiffRestartFixture";
    if(!RegisterClassW(&cls))return 7;
    HWND window=CreateWindowW(cls.lpszClassName,L"Restart test game",WS_OVERLAPPEDWINDOW,0,0,800,600,NULL,NULL,instance,NULL);
    if(!window)return 8;ShowWindow(window,SW_SHOW);SetTimer(window,1,300,NULL);
    MSG message;while(GetMessageW(&message,NULL,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
    /* A replacement process must wait for process exit, not just window close. */
    Sleep(500);
    if(run_number==2&&!message.wParam){
        swprintf(file,MAX_PATH,L"%lsrestart-complete",directory);
        HANDLE done=CreateFileW(file,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if(done!=INVALID_HANDLE_VALUE)CloseHandle(done);
    }
    (void)lock; /* Keep the exclusive lock until Windows exits this process. */
    return (int)message.wParam;
}
