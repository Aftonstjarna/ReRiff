/* Only successful no-update checks are cached. Manual/forced checks bypass it. */
#define UPDATE_CHECK_AGE (15ULL*60*10000000)
typedef struct { DWORD magic; ULONGLONG checked; char identity[192]; } UpdateCheckCache;
static ULONGLONG update_clock(void){FILETIME now;GetSystemTimeAsFileTime(&now);return ((ULONGLONG)now.dwHighDateTime<<32)|now.dwLowDateTime;}
static void update_cache_identity(char out[192]){
    WCHAR executable[CAP];WIN32_FILE_ATTRIBUTE_DATA info={0};
    if(GetModuleFileNameW(NULL,executable,CAP))GetFileAttributesExW(executable,GetFileExInfoStandard,&info);
    memset(out,0,192);snprintf(out,192,"%s/%d/%d/%lu/%lu/%lu/%lu",RERIFF_VERSION,installed_experimental,requested_experimental,
        (unsigned long)info.nFileSizeHigh,(unsigned long)info.nFileSizeLow,
        (unsigned long)info.ftLastWriteTime.dwHighDateTime,(unsigned long)info.ftLastWriteTime.dwLowDateTime);
}
static BOOL update_check_recent(void){
    if(!automatic_update||force_auto_reinstall)return FALSE;
    WCHAR name[CAP];path(name,L"localserver\\update-check-cache.bin");
    FILE *file=_wfopen(name,L"rb");if(!file)return FALSE;
    UpdateCheckCache saved;BOOL valid=fread(&saved,sizeof(saved),1,file)==1&&fgetc(file)==EOF;fclose(file);
    char identity[192];update_cache_identity(identity);ULONGLONG now=update_clock();
    return valid&&saved.magic==0x31435552&&!memcmp(saved.identity,identity,sizeof(identity))
        &&now>=saved.checked&&now-saved.checked<UPDATE_CHECK_AGE;
}
static void update_check_remember(void){
    UpdateCheckCache saved={0};saved.magic=0x31435552;saved.checked=update_clock();update_cache_identity(saved.identity);
    WCHAR name[CAP],temporary[CAP];path(name,L"localserver\\update-check-cache.bin");
    if(wcslen(name)>CAP-32)return;
    _snwprintf(temporary,CAP,L"%ls.%lu.tmp",name,(unsigned long)GetCurrentProcessId());
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return;
    DWORD written;BOOL ok=WriteFile(file,&saved,sizeof(saved),&written,NULL)&&written==sizeof(saved)&&FlushFileBuffers(file);CloseHandle(file);
    if(ok)ok=MoveFileExW(temporary,name,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok)DeleteFileW(temporary);
}
