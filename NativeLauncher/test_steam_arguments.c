#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain
#define CHECK(value) do { if(!(value)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#value);return 1;} } while(0)

int main(void) {
    GetModuleFileNameW(NULL,folder,CAP);WCHAR *slash=wcsrchr(folder,L'\\');CHECK(slash);*slash=0;
    WCHAR game[CAP],command[CAP];path(game,L"Game.exe");
    BOOL skip,selftest;
    LPWSTR forwarded[]={L"ReRiff-Launcher.exe",game,L"--windowed",L"space value",L"quote\"value",L"C:\\trailing\\",L"",L"音楽"};
    CHECK(parse_launcher_arguments(8,forwarded,&skip,&selftest));
    CHECK(!skip&&!selftest&&game_command(command));
    int count;LPWSTR *decoded=CommandLineToArgvW(command,&count);
    CHECK(decoded&&count==7);
    for(int i=0;i<count;i++)CHECK(!wcscmp(decoded[i],forwarded[i+1]));
    LocalFree(decoded);
    LPWSTR direct[]={L"ReRiff-Launcher.exe",L"--skip"};
    CHECK(parse_launcher_arguments(2,direct,&skip,&selftest)&&skip&&!*game_arguments);
    path(ini,L"skip-preference-test.ini");DeleteFileW(ini);
    CHECK(!should_skip_launcher(FALSE));
    CHECK(WritePrivateProfileStringW(L"launcher",L"skip_launcher",L"true",ini));
    CHECK(should_skip_launcher(FALSE));
    LPWSTR show[]={L"ReRiff-Launcher.exe",L"--show-launcher"};
    CHECK(parse_launcher_arguments(2,show,&skip,&selftest)&&!should_skip_launcher(skip));
    LPWSTR alias[]={L"ReRiff-Launcher.exe",L"--launcher-skip"};
    CHECK(parse_launcher_arguments(2,alias,&skip,&selftest)&&should_skip_launcher(skip));
    CHECK(WritePrivateProfileStringW(L"launcher",L"skip_launcher",L"false",ini));
    CHECK(!should_skip_launcher(FALSE));DeleteFileW(ini);
    LPWSTR bridge[]={L"ReRiff-Launcher.exe",L"--skip-launcher",L"--",game,L"--game-option"};
    CHECK(parse_launcher_arguments(5,bridge,&skip,&selftest)&&skip&&!selftest);
    LPWSTR unknown[]={L"ReRiff-Launcher.exe",L"C:\\another-installation\\Game.exe"};
    CHECK(!parse_launcher_arguments(2,unknown,&skip,&selftest));
    LPWSTR placeholder[]={L"ReRiff-Launcher.exe",L"%command%"};
    CHECK(!parse_launcher_arguments(2,placeholder,&skip,&selftest));
    LPWSTR missing[]={L"ReRiff-Launcher.exe",L"--"};
    CHECK(!parse_launcher_arguments(2,missing,&skip,&selftest));
    static WCHAR oversized[CAP];for(int i=0;i<CAP-1;i++)oversized[i]=L'x';
    LPWSTR too_long[]={L"ReRiff-Launcher.exe",game,oversized};
    CHECK(!parse_launcher_arguments(3,too_long,&skip,&selftest));
    puts("PASS: Steam command forwarding, argument quoting/Unicode, skip aliases, and invalid-command refusal");
    return 0;
}
