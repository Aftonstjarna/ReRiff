#define wWinMain reriff_launcher_entry
#include "launcher.c"
#undef wWinMain
#ifdef TEST_WINDOWS_SERVER
#define tested_server_action windows_server_action
#else
#define tested_server_action linux_server_action
#endif
static DWORD WINAPI exercise(LPVOID unused){
    (void)unused;
    if(!tested_server_action(L"status")||local_server_state!=1)goto fail;
    if(!tested_server_action(L"start")||local_server_state!=2)goto fail;
    if(!tested_server_action(L"status")||local_server_state!=2)goto fail;
    if(!tested_server_action(L"start")||local_server_state!=2)goto fail;
    if(!tested_server_action(L"stop")||local_server_state!=1)goto fail;
    if(!tested_server_action(L"stop")||local_server_state!=1)goto fail;
    puts("PASS: launcher platform-local server status/start/stop and UTF-16 completion handoff");return 0;
fail:
    wprintf(L"FAIL: %ls state=%d\n",message,local_server_state);return 1;
}
int wmain(int count,WCHAR **args){
    if(count!=2)return 2;
    wcsncpy(folder,args[1],CAP-1);folder[CAP-1]=0;
    HANDLE thread=CreateThread(NULL,8*1024*1024,exercise,NULL,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
    if(!thread)return 3;
    WaitForSingleObject(thread,INFINITE);DWORD code=4;GetExitCodeThread(thread,&code);CloseHandle(thread);return (int)code;
}
