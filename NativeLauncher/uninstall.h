/* Uninstall uses the same verified planner shipped with the installer. */
static BOOL uninstall_action(void){
    WCHAR tool[CAP],result[CAP],temporary[CAP],command[CAP];
    if(!GetTempPathW(CAP,temporary)||!GetTempFileNameW(temporary,L"rrf",0,result))return FALSE;DeleteFileW(result);
    if(is_wine()){
        WCHAR host_tool[CAP],host_game[CAP],host_result[CAP],system[CAP];path(tool,L"localserver\\ReRiff-ClientHost");
        if(GetFileAttributesW(tool)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The native uninstaller is missing. Reinstall the complete client package.");return FALSE;}
        path(tool,L"localserver\\linux-runtime.sh");
        if(!update_host_path(tool,host_tool)||!update_host_path(folder,host_game)||!update_host_path(result,host_result))return FALSE;
        GetSystemDirectoryW(system,CAP);
        _snwprintf(command,CAP,L"\"%ls\\start.exe\" /wait /unix /bin/sh \"%ls\" uninstall --game-dir \"%ls\" --result \"%ls\"",system,host_tool,host_game,host_result);
    }else{
        path(tool,L"localserver\\language-host\\ReRiff-Patcher.exe");
        if(GetFileAttributesW(tool)==INVALID_FILE_ATTRIBUTES){wcscpy(message,L"The uninstaller is missing. Install the complete client package.");return FALSE;}
        _snwprintf(command,CAP,L"\"%ls\" --uninstall --game-dir \"%ls\" --result \"%ls\"",tool,folder,result);
    }
    command[CAP-1]=0;PROCESS_INFORMATION process;if(!spawn(command,TRUE,&process)){wcscpy(message,L"Could not start uninstall.");return FALSE;}
    CloseHandle(process.hThread);ULONGLONG deadline=GetTickCount64()+120000;
    while(GetFileAttributesW(result)==INVALID_FILE_ATTRIBUTES&&GetTickCount64()<deadline){
        if(!is_wine()&&WaitForSingleObject(process.hProcess,100)==WAIT_OBJECT_0)break;
        Sleep(100);
    }
    CloseHandle(process.hProcess);
    WCHAR ok[16];GetPrivateProfileStringW(L"result",L"ok",L"false",ok,16,result);
    GetPrivateProfileStringW(L"result",L"message",L"Uninstall did not report completion. Check the retained localserver folder.",message,1024,result);
    DeleteFileW(result);return !wcscmp(ok,L"true");
}
