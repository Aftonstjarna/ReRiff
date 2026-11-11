#define wWinMain reriff_launcher_entry
#include "launcher.c"
#undef wWinMain
static DWORD WINAPI exercise(LPVOID unused){
    (void)unused;
    if(!linux_server_action(L"status")||podman_state!=1)goto fail;
    if(!linux_server_action(L"start")||podman_state!=2)goto fail;
    if(!linux_server_action(L"status")||podman_state!=2)goto fail;
    if(!linux_server_action(L"start")||podman_state!=2)goto fail;
    if(!linux_server_action(L"stop")||podman_state!=1)goto fail;
    if(!linux_server_action(L"stop")||podman_state!=1)goto fail;
    puts("PASS: Wine launcher native AppImage status/start/stop and UTF-16 completion handoff");return 0;
fail:
    wprintf(L"FAIL: %ls state=%d\n",message,podman_state);return 1;
}
int wmain(int count,WCHAR **args){
    if(count!=2)return 2;
    wcsncpy(folder,args[1],CAP-1);folder[CAP-1]=0;
    HANDLE thread=CreateThread(NULL,8*1024*1024,exercise,NULL,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
    if(!thread)return 3;
    WaitForSingleObject(thread,INFINITE);DWORD code=4;GetExitCodeThread(thread,&code);CloseHandle(thread);return (int)code;
}
