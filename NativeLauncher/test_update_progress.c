#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain
#define CHECK(c) do {if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static DWORD WINAPI cancel_later(LPVOID unused){
    (void)unused;Sleep(40);PostMessageW(progress_window,WM_COMMAND,IDCANCEL,0);return 0;
}
int main(void){
    CHECK(progress_open(TRUE)&&IsWindowVisible(progress_window));progress_report(PROGRESS_DOWNLOAD,35);progress_pump();
    CHECK(SendMessageW(progress_bar,PBM_GETPOS,0,0)==35&&IsWindowEnabled(progress_cancel));
    HANDLE wake=CreateEventW(NULL,TRUE,FALSE,NULL),thread=CreateThread(NULL,0,cancel_later,NULL,0,NULL);CHECK(wake&&thread);
    CHECK(progress_wait(wake,1000)==WAIT_TIMEOUT&&progress_cancelled());
    CHECK(!IsWindowEnabled(progress_cancel)&&!progress_commit());
    automatic_update=TRUE;CHECK(!update_download(NULL,L"https://unused.invalid",L"should-not-exist",1));
    WaitForSingleObject(thread,1000);CloseHandle(thread);CloseHandle(wake);progress_close();
    CHECK(progress_open(TRUE)&&progress_commit());progress_report(PROGRESS_INSTALL,-1);progress_pump();
    CHECK(!IsWindowEnabled(progress_cancel));
    SendMessageW(progress_window,WM_COMMAND,IDCANCEL,0);SendMessageW(progress_window,WM_CLOSE,0,0);
    CHECK(!progress_cancelled()&&IsWindow(progress_window));
    progress_report(PROGRESS_RESUME,100);progress_pump();CHECK(SendMessageW(progress_bar,PBM_GETPOS,0,0)==100);
    progress_close();
    puts("PASS: progress percentage, responsive cancellation while waiting, canceled download gate, and locked installation phase");return 0;
}
