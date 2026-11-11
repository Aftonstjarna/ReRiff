#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain
#define CHECK(c) do {if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    GetCurrentDirectoryW(CAP,folder);path(ini,L"automatic-preference-test.ini");DeleteFileW(ini);
    CHECK(!launcher_preference(L"auto_update_on_skip")&&!should_skip_launcher(FALSE));
    CHECK(WritePrivateProfileStringW(L"launcher",L"skip_launcher",L"true",ini));
    CHECK(WritePrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"true",ini));
    CHECK(launcher_preference(L"auto_update_on_skip")&&should_skip_launcher(FALSE));
    LPWSTR args[]={L"ReRiff-Launcher.exe",L"--skip-launcher",L"--no-auto-update",L"--",L"Game.exe",L"space value",L"quote\"value"};
    BOOL skip,selftest;CHECK(parse_launcher_arguments(7,args,&skip,&selftest)&&skip&&suppress_auto_update);
    WCHAR work[CAP],name[CAP],resume[CAP];CHECK(GetTempPathW(CAP,work));CHECK(GetTempFileNameW(work,L"rra",0,name));DeleteFileW(name);CHECK(CreateDirectoryW(name,NULL));
    CHECK(save_auto_resume(name));_snwprintf(work,CAP,L"%ls\\resume-command.txt",name);
    HANDLE file=CreateFileW(work,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);DWORD bytes=0;
    CHECK(file!=INVALID_HANDLE_VALUE&&ReadFile(file,resume,sizeof(resume)-2,&bytes,NULL));CloseHandle(file);resume[bytes/2]=0;
    int count;LPWSTR *decoded=CommandLineToArgvW(resume,&count);CHECK(decoded&&count==7);
    CHECK(parse_launcher_arguments(count,decoded,&skip,&selftest)&&skip&&suppress_auto_update);
    CHECK(wcsstr(game_arguments,L"space value")&&wcsstr(game_arguments,L"quote"));LocalFree(decoded);
    LPWSTR trial[]={L"ReRiff-Launcher.exe",L"--skip-launcher",L"--test-auto-update"};
    CHECK(parse_launcher_arguments(3,trial,&skip,&selftest)&&skip&&force_auto_reinstall&&!suppress_auto_update);
    CHECK(parse_launcher_arguments(7,args,&skip,&selftest)&&!force_auto_reinstall&&suppress_auto_update);
    DeleteFileW(work);RemoveDirectoryW(name);DeleteFileW(ini);
    puts("PASS: shared skip preference, opt-in automatic updates, resume arguments, and loop suppression");return 0;
}
