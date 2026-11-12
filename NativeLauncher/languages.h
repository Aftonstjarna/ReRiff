/* Native Languages page; component installation shares the installer's host. */
static BOOL language_action(const WCHAR *action){
    WCHAR tool[CAP],result[CAP],temporary[CAP],command[CAP];
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rrf",0,result))return FALSE;DeleteFileW(result);
    if(is_wine()){
        WCHAR host_tool[CAP],host_game[CAP],host_result[CAP],system[CAP];path(tool,L"localserver\\linux-runtime.sh");
        if(!update_host_path(tool,host_tool)||!update_host_path(folder,host_game)||!update_host_path(result,host_result))return FALSE;
        GetSystemDirectoryW(system,CAP);
        _snwprintf(command,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" language-component %ls --game-dir \"%ls\" --result \"%ls\"",system,host_tool,action,host_game,host_result);
    }else{
        path(tool,L"localserver\\language-host\\ReRiff-Patcher.exe");
        if(GetFileAttributesW(tool)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The language installer is missing. Install the complete client package.");return FALSE;}
        _snwprintf(command,CAP,L"\"%ls\" --language-component %ls --game-dir \"%ls\" --result \"%ls\"",tool,action,folder,result);
    }
    command[CAP-1]=0;PROCESS_INFORMATION process;if(!spawn(command,TRUE,&process)){wcscpy(message,L"Could not start language installation.");return FALSE;}
    CloseHandle(process.hThread);CloseHandle(process.hProcess);ULONGLONG deadline=GetTickCount64()+600000;
    while(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES&&GetTickCount64()<deadline)Sleep(100);
    WCHAR ok[16];GetPrivateProfileStringW(L"result",L"ok",L"false",ok,16,result);
    GetPrivateProfileStringW(L"result",L"message",L"Language installation did not report completion. See logs/ReRiff-languages.log.",message,1024,result);
    DeleteFileW(result);return !wcscmp(ok,L"true");
}
