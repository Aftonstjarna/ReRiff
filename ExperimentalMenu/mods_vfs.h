/* Reuse the original directory scanner and secondary VFS mount pass.
 * See reports/MODS-VFS-DIRECTORY.md for original-byte provenance and ABI. */
typedef struct ModsNativeString {
    union { WCHAR small[8];const WCHAR *text; } data;
    unsigned length,capacity;
} ModsNativeString;
typedef void (__fastcall *ModsScanFn)(const ModsNativeString *,void *);
static ModsScanFn mods_scan_original;
static WCHAR mods_directory[MAX_PATH];
static BOOL mods_scanned,mods_only;
static BYTE *mods_manager;
static BYTE **mods_manager_global;
typedef struct { BYTE *begin,*end,*capacity; } ModsVector;
static void mods_scan_path(const WCHAR *folder,void *entries){
    ModsNativeString path={0};path.data.text=folder;
    path.length=(unsigned)wcslen(folder);path.capacity=path.length;
    mods_scan_original(&path,entries);
}
static int mods_folder_compare(const void *a,const void *b){return wcscmp(a,b);}
static void __fastcall mods_scan(const ModsNativeString *original,void *entries){
    if(!mods_only)mods_scan_original(original,entries);
    if(mods_scanned)return;
    mods_scanned=TRUE;
    mods_scan_path(mods_directory,entries);
    ModsVector *vector=entries;DWORD used=1;
    for(BYTE *p=vector->begin;p<vector->end;p+=28)if(*(unsigned *)p<32)used|=1u<<*(unsigned *)p;
    if(mods_manager_global)mods_manager=*mods_manager_global;
    if(mods_manager&&readable(mods_manager,0x54)){
        BYTE *start=*(BYTE **)(mods_manager+0x48),*end=*(BYTE **)(mods_manager+0x4c);
        for(BYTE *p=start;p<end;p+=20)if(*(unsigned *)(p+12)<32)used|=1u<<*(unsigned *)(p+12);
    }
    WCHAR pattern[MAX_PATH];wcscpy(pattern,mods_directory);wcscat(pattern,L"*");
    WIN32_FIND_DATAW data;HANDLE find=FindFirstFileW(pattern,&data);
    WCHAR (*folders)[MAX_PATH]=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,31*MAX_PATH*sizeof(WCHAR));
    if(!folders){if(find!=INVALID_HANDLE_VALUE)FindClose(find);return;}
    unsigned count=0;BOOL overflow=FALSE;
    if(find!=INVALID_HANDLE_VALUE){do{
        if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||data.cFileName[0]==L'.')continue;
        if(wcslen(mods_directory)+wcslen(data.cFileName)+12>=MAX_PATH)continue;
        WCHAR folder[MAX_PATH],archive[MAX_PATH];wcscpy(folder,mods_directory);wcscat(folder,data.cFileName);wcscat(folder,L"\\");
        wcscpy(archive,folder);wcscat(archive,L"vfs.dat");DWORD dat=GetFileAttributesW(archive);
        wcscpy(archive,folder);wcscat(archive,L"vfs.zip");DWORD zip=GetFileAttributesW(archive);
        if(dat==INVALID_FILE_ATTRIBUTES||zip==INVALID_FILE_ATTRIBUTES||((dat|zip)&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))continue;
        // One archive pair per folder. Avoid importing accidental numbered copies.
        wcscpy(archive,folder);wcscat(archive,L"vfs*.dat");WIN32_FIND_DATAW other;BOOL extra=FALSE;
        HANDLE check=FindFirstFileW(archive,&other);if(check!=INVALID_HANDLE_VALUE){do{if(_wcsicmp(other.cFileName,L"vfs.dat"))extra=TRUE;}while(FindNextFileW(check,&other));FindClose(check);}
        if(extra){log_line("Skipped mod folder with multiple VFS indexes.");continue;}
        if(count==31){overflow=TRUE;break;}wcscpy(folders[count++],folder);
    }while(FindNextFileW(find,&data));FindClose(find);}
    if(overflow){log_line("Too many mod folders; named mods were not mounted.");HeapFree(GetProcessHeap(),0,folders);return;}
    qsort(folders,count,sizeof(*folders),mods_folder_compare);
    for(unsigned n=0;n<count;n++){
        unsigned id=1;while(id<32&&(used&(1u<<id)))id++;
        if(id==32){log_line("No free native VFS slot for remaining mod folders.");break;}
        mods_scan_path(folders[n],entries);
        WCHAR stem[MAX_PATH];wcscpy(stem,folders[n]);wcscat(stem,L"vfs");
        for(BYTE *p=vector->begin;p<vector->end;p+=28){
            ModsNativeString *name=(void *)(p+4);const WCHAR *text=name->capacity<8?name->data.small:name->data.text;
            if(*(unsigned *)p==0&&!wcscmp(text,stem)){*(unsigned *)p=id;used|=1u<<id;break;}
        }
    }
    HeapFree(GetProcessHeap(),0,folders);
    log_line("Scanned flat and named mods with original VFS discovery.");
}
static BOOL install_mods_vfs(BYTE *base,const WCHAR *executable){
    size_t length=wcslen(executable);if(length>=MAX_PATH-32)return FALSE;
    wcscpy(mods_directory,executable);WCHAR *slash=wcsrchr(mods_directory,L'\\');if(!slash)return FALSE;
    wcscpy(slash+1,L"mods\\");
    DWORD attributes=GetFileAttributesW(mods_directory);
    if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT)||!(attributes&FILE_ATTRIBUTE_DIRECTORY))return TRUE;
    /* Both instructions belong to the hash-pinned original. The installed hook
       is only at the secondary scan; stock archive discovery is unchanged. */
    BYTE *site=base+0x15ede7;const BYTE call[]={0xe8,0x44,0xf3,0xff,0xff};
    const BYTE scanner[]={0x55,0x8b,0xec,0x6a,0xff};
    if(memcmp(site,call,5)||memcmp(base+0x15e130,scanner,5))return FALSE;
    DWORD old,ignored;if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old))return FALSE;
    mods_manager_global=(BYTE **)(base+0x7fcd2c);
    mods_scan_original=(ModsScanFn)(base+0x15e130);
    *(int32_t *)(site+1)=(int32_t)((BYTE *)(uintptr_t)mods_scan-(site+5));
    FlushInstructionCache(GetCurrentProcess(),site,5);VirtualProtect(site,5,old,&ignored);
    /* Network initialization can follow the initial mount pass. Load mods
       through the game's existing secondary loader in that case. It inserts
       records by descending volume ID, enabling native asset overrides. */
    BYTE *manager=*(BYTE **)(base+0x7fcd2c);mods_manager=manager;
    if(manager&&readable(manager,0x54)&&*(void **)(manager+0x48)){
        mods_only=TRUE;((void(__thiscall *)(void *))(base+0x15ed80))(manager);mods_only=FALSE;
    }
    return TRUE;
}
