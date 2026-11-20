/* Native selection fallback and managed identity-migration callback.
 * Allocated slots use portable save identities; legacy authored slots retain
 * donor-only removal fallback through the original guarded settings setter. */
typedef void (__cdecl *CarFallbackSetter)(void *,int);
static CarFallbackSetter car_fallback_setter;
typedef int (__cdecl *CarMigrationCallback)(void *);
static CarMigrationCallback car_migration_callback;
static BOOL car_slots_active;
__declspec(dllexport) void __cdecl ReRiffSetCarSlotsActive(int active){car_slots_active=active!=0;}
__declspec(dllexport) int __cdecl ReRiffConfigureCarMigration(void *callback){if(!callback||car_migration_callback)return 0;car_migration_callback=(CarMigrationCallback)callback;return 1;}
__declspec(dllexport) int __cdecl ReRiffConfigureCarFallback(void *setter){
    if(!setter||car_fallback_setter)return 0;
    car_fallback_setter=(CarFallbackSetter)setter;return 1;
}
static BOOL car_fallback_name(void *manager,int index,char name[4]){
    BYTE *text=(BYTE *)manager+264+24*index;
    /* All supported stock origins have three-byte inline native IDs. */
    if(*(unsigned *)(text+16)!=3||*(unsigned *)(text+20)!=15)return FALSE;
    memcpy(name,text,3);name[3]=0;return TRUE;
}
static int car_fallback_target(void *manager,void *settings,const char *origin){
    static const char *stock[]={"alf","bui","cor","rs1","vau","fab","hon","cad","rol","mer","mot","pha","che","f1r","pl1"};
    if(!readable(manager,796)||!readable(settings,328)||!origin)return -1;
    int count=*(int *)((BYTE *)manager+792),selected=*(int *)((BYTE *)settings+324);
    /* Never reinterpret an active slot, or a missing/invalid stock slot. */
    if(count<15||count>22||selected<15||selected>=22||selected<count||!((BYTE *)settings)[5])return -1;
    BOOL known=FALSE;for(unsigned i=0;i<15;i++)if(!strcmp(origin,stock[i]))known=TRUE;
    if(!known)return -1;
    int found=-1;
    for(int i=0;i<count;i++){
        char name[4];if(car_fallback_name(manager,i,name)&&!strcmp(name,origin)){
            if(found>=0)return -1;found=i;
        }
    }
    return found;
}
/* Prefer the declared missing-car fallback, then the donor. An unavailable or
 * invalid fallback never blocks the donor fallback. */
static int car_fallback_resolve(void *manager,void *settings,const char *base,const char *fallback,const char **used){
    int target=fallback&&*fallback?car_fallback_target(manager,settings,fallback):-1;
    if(target>=0){if(used)*used=fallback;return target;}
    if(used)*used=base;
    return car_fallback_target(manager,settings,base);
}
/* Package metadata is read once after native car names are initialized.
 * Keep origins outside the mod so disabling/removing it retains fallback. */
static BOOL car_origin_stock(const WCHAR *id){
    static const WCHAR *stock[]={L"alf",L"bui",L"cor",L"rs1",L"vau",L"fab",L"hon",L"cad",L"rol",L"mer",L"mot",L"pha",L"che",L"f1r",L"pl1"};
    for(unsigned i=0;i<15;i++)if(!wcscmp(id,stock[i]))return TRUE;
    return FALSE;
}
static void car_origin_packages(void *manager,const WCHAR *registry){
    int count=*(int *)((BYTE *)manager+792);
    if(count<15||count>22)return;
    WCHAR root[MAX_PATH],pattern[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,root,MAX_PATH);
    if(!n||n>=MAX_PATH)return;
    WCHAR *slash=wcsrchr(root,L'\\');if(!slash)return;slash[1]=0;
    if(wcslen(root)+12>=MAX_PATH)return;wcscat(root,L"mods");
    DWORD attrs=GetFileAttributesW(root);if(attrs==INVALID_FILE_ATTRIBUTES||(attrs&FILE_ATTRIBUTE_REPARSE_POINT))return;
    _snwprintf(pattern,MAX_PATH,L"%ls\\*",root);
    WCHAR origins[7][16]={{0}},fallbacks[7][16]={{0}};BOOL conflict[7]={0},fallbackConflict[7]={0};
    WIN32_FIND_DATAW fd;HANDLE find=FindFirstFileW(pattern,&fd);if(find==INVALID_HANDLE_VALUE)return;
    do {
        if(fd.cFileName[0]==L'.'||!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(fd.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))continue;
        WCHAR path[MAX_PATH];if(wcslen(root)+wcslen(fd.cFileName)+13>=MAX_PATH)continue;
        _snwprintf(path,MAX_PATH,L"%ls\\%ls\\origin.txt",root,fd.cFileName);
        attrs=GetFileAttributesW(path);if(attrs==INVALID_FILE_ATTRIBUTES||(attrs&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))continue;
        for(int slot=15;slot<count;slot++){
            char id[4];if(!car_fallback_name(manager,slot,id))continue;
            WCHAR key[4]={(WCHAR)id[0],(WCHAR)id[1],(WCHAR)id[2],0},base[16],fallback[16];
            GetPrivateProfileStringW(L"origins",key,L"",base,16,path);
            if(!*base)continue;
            int i=slot-15;
            if(!car_origin_stock(base)||(origins[i][0]&&wcscmp(origins[i],base)))conflict[i]=TRUE;
            else wcscpy(origins[i],base);
            GetPrivateProfileStringW(L"fallbacks",key,L"",fallback,16,path);
            if(!*fallback)continue;
            if(!car_origin_stock(fallback)||(fallbacks[i][0]&&wcscmp(fallbacks[i],fallback)))fallbackConflict[i]=TRUE;
            else wcscpy(fallbacks[i],fallback);
        }
    }while(FindNextFileW(find,&fd));FindClose(find);
    for(int slot=15;slot<count;slot++){
        WCHAR section[24];_snwprintf(section,24,L"slot_%d",slot);int i=slot-15;
        /* Clear stale mappings for reused active slots lacking valid metadata. */
        if(!WritePrivateProfileStringW(section,L"base",conflict[i]||!*origins[i]?NULL:origins[i],registry))
            log_line("Car origin: could not persist package donor; removal fallback unavailable.");
        if(!WritePrivateProfileStringW(section,L"fallback",conflict[i]||fallbackConflict[i]||!*fallbacks[i]?NULL:fallbacks[i],registry))
            log_line("Car origin: could not persist package fallback; donor fallback remains active.");
        if(conflict[i])log_line("Car origin: invalid or conflicting package donors refused.");
        if(fallbackConflict[i])log_line("Car origin: invalid or conflicting package fallbacks refused.");
    }
}
static void car_fallback_main(void){
    if(!car_fallback_setter)return;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    void *manager=*(void **)(base+0x7f3d2c),*save=*(void **)(base+0x7ed7b8);
    if(!readable(save,20)||!readable(manager,796))return;
    if(car_migration_callback&&car_migration_callback(save)!=1){
        MessageBoxW(NULL,L"Custom car identities could not be migrated safely. The last committed save is retained. See logs/ReRiff-game.log.",L"ReRiff car slots",MB_OK|MB_ICONERROR);ExitProcess(1);return;
    }
    if(car_slots_active)return; /* Portable identity migration owns allocated slots. */
    void *settings=*(void **)((BYTE *)save+16);if(!readable(settings,328))return;
    int selected=*(int *)((BYTE *)settings+324),count=*(int *)((BYTE *)manager+792);
    if(count<15||count>22)return;
    WCHAR path[MAX_PATH],section[24],origin[16],fallback[16];
    const WCHAR *slash=wcsrchr(config_path,L'\\');if(!slash)return;
    size_t length=(size_t)(slash-config_path)+1;
    if(length+32>=MAX_PATH)return;
    memcpy(path,config_path,length*sizeof(WCHAR));wcscpy(path+length,L"car-origins.ini");
    static BOOL origins_scanned;
    if(!origins_scanned){car_origin_packages(manager,path);origins_scanned=TRUE;}
    if(selected<15||selected>=22||selected<count)return;
    _snwprintf(section,24,L"slot_%d",selected);
    GetPrivateProfileStringW(section,L"base",L"",origin,16,path);
    GetPrivateProfileStringW(section,L"fallback",L"",fallback,16,path);
    char id[4],fallbackId[4]={0};if(wcslen(origin)!=3)return;
    for(unsigned i=0;i<3;i++){if(origin[i]>127)return;id[i]=(char)origin[i];}id[3]=0;
    /* An unusable declared fallback is ignored and the donor still applies. */
    if(wcslen(fallback)==3){BOOL valid=TRUE;for(unsigned i=0;i<3;i++){if(fallback[i]>127)valid=FALSE;else fallbackId[i]=(char)fallback[i];}if(!valid)fallbackId[0]=0;fallbackId[3]=0;}
    const char *used=NULL;
    int target=car_fallback_resolve(manager,settings,id,fallbackId,&used);if(target<0)return;
    car_fallback_setter(settings,target);
    char message[192];snprintf(message,sizeof(message),"Car fallback: missing slot=%d selected stock base=%s used=%s slot=%d through original setter; paints and unlocks retained.",selected,id,used,target);
    log_line(message);
}
