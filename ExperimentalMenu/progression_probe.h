/* Read-only progression observation for the hash-pinned original Game.exe.
 * Original metadata/layout provenance: https://github.com/Aftonstjarna/ReRiff/wiki/Research-PRESTIGE-NATIVE-BOUNDARIES.
 * This never resets a chunk, awards anything, or changes Steam Cloud state. */
static LONG progression_probe_started;
static BOOL progression_probe_read(const void *source,void *target,SIZE_T size){
    SIZE_T received=0;
    return source&&ReadProcessMemory(GetCurrentProcess(),source,target,size,&received)&&received==size;
}
static BOOL progression_probe_getter_offset(const BYTE *code,DWORD *offset){
    if(code[0]==0x8b&&code[1]==0x41&&code[3]==0xc3){*offset=code[2];return *offset<128&&*offset%4==0;}
    if(code[0]==0x8b&&code[1]==0x81&&code[6]==0xc3){memcpy(offset,code+2,4);return *offset<4096&&*offset%4==0;}
    return FALSE;
}
static void progression_probe_code(BYTE *base,const void *method,const char *label){
    PIMAGE_DOS_HEADER dos=(PIMAGE_DOS_HEADER)base;
    PIMAGE_NT_HEADERS32 nt=(PIMAGE_NT_HEADERS32)(base+dos->e_lfanew);
    uintptr_t start=(uintptr_t)base,at=(uintptr_t)method;
    if(at<start||at-start>=nt->OptionalHeader.SizeOfImage)return;
    BYTE code[32];if(!progression_probe_read(method,code,sizeof(code)))return;
    char text[192];int used=snprintf(text,sizeof(text),"Progression native %s RVA=%08lx bytes=",label,(unsigned long)(at-start));
    for(unsigned i=0;i<sizeof(code);i++)used+=snprintf(text+used,sizeof(text)-(size_t)used,"%02x",code[i]);
    log_line(text);
}
static void progression_probe_cloud(BYTE *base){
    void *network=NULL,**table=NULL,*getter=NULL,*cloud=NULL,**cloud_table=NULL,*enable=NULL;
    BYTE code[8];DWORD offset=0;
    if(!progression_probe_read(base+0x80d57c,&network,sizeof(network))||!network
        ||!progression_probe_read(network,&table,sizeof(table))||!table
        ||!progression_probe_read(table+11,&getter,sizeof(getter)))return;
    progression_probe_code(base,getter,"cloud_getter");
    if(!progression_probe_read(getter,code,sizeof(code))||!progression_probe_getter_offset(code,&offset)
        ||!progression_probe_read((BYTE *)network+offset,&cloud,sizeof(cloud))||!cloud
        ||!progression_probe_read(cloud,&cloud_table,sizeof(cloud_table))||!cloud_table
        ||!progression_probe_read(cloud_table,&enable,sizeof(enable)))return;
    progression_probe_code(base,enable,"cloud_enable");
}
static void progression_probe_startup(BYTE *base){
    void *save=NULL,*driver=NULL,*device=NULL;
    progression_probe_read(base+0x7ed7b8,&save,sizeof(save));
    progression_probe_read(base+0x7e9428,&driver,sizeof(driver));
    progression_probe_read(base+0x7fcb4c,&device,sizeof(device));
    char text[192];snprintf(text,sizeof(text),"Progression startup boundary: save_allocated=%u driver_allocated=%u save_device_allocated=%u (endpoint initialization)",!!save,!!driver,!!device);
    log_line(text);
    progression_probe_cloud(base);
}
static void progression_probe_save_names(BYTE *base){
    void *save=NULL;DWORD count=0;
    if(!progression_probe_read(base+0x7ed7b8,&save,sizeof(save))||!save
        ||!progression_probe_read((BYTE *)save+172,&count,sizeof(count))||count>3)return;
    for(DWORD index=0;index<count;index++){
        char name[33]={0},message[112];
        if(!progression_probe_read((BYTE *)save+64+32*index,name,32))continue;
        BOOL valid=TRUE;for(unsigned i=0;name[i];i++)if((BYTE)name[i]<32||(BYTE)name[i]>126)valid=FALSE;
        if(!valid)continue;
        snprintf(message,sizeof(message),"Progression native save-file slot %lu: %s",(unsigned long)index,name);log_line(message);
    }
    void *device=NULL;BYTE object[44];
    if(!progression_probe_read(base+0x7fcb4c,&device,sizeof(device))
        ||!progression_probe_read(device,object,sizeof(object)))return;
    void **vtable=NULL,*open_method=NULL;memcpy(&vtable,object,sizeof(vtable));
    if(!vtable||!progression_probe_read(vtable+2,&open_method,sizeof(open_method)))return;
    char type[160];snprintf(type,sizeof(type),"Progression native save-device open: original_local_implementation=%u",open_method==(void *)(base+0x135b10));log_line(type);
}
static BOOL progression_probe_snapshot(BYTE *base,char *text,size_t capacity){
    void *save=NULL,*progression=NULL,*settings=NULL,*driver=NULL;
    BYTE cloud=0,save_flags[4],chunk[296],preferences[328];
    if(!progression_probe_read(base+0x7ed7b8,&save,sizeof(save))||!save
        ||!progression_probe_read((BYTE *)save+12,save_flags,sizeof(save_flags))
        ||!progression_probe_read((BYTE *)save+32,&progression,sizeof(progression))
        ||!progression_probe_read((BYTE *)save+16,&settings,sizeof(settings))
        ||!progression_probe_read(progression,chunk,sizeof(chunk))
        ||!progression_probe_read(settings,preferences,sizeof(preferences)))return FALSE;
    progression_probe_read(base+0x7ed7bc,&cloud,1);
    int car=-1,pack=-1,xp=0,coins=0;unsigned completed=0;uint64_t mask=0;
    memcpy(&car,preferences+324,4);memcpy(&xp,chunk+280,4);memcpy(&coins,chunk+284,4);
    if(car>=0&&car<22)memcpy(&pack,preferences+236+4*car,4);
    for(unsigned index=0;index<39;index++)if(chunk[12+index]){if(index!=18)completed++;mask|=UINT64_C(1)<<index;}
    int used=snprintf(text,capacity,"Progression observation: native_xp=%d coins=%d equipped_car=%d equipped_pack=%d achievements=%u/38 mask=%010llx native_use_cloud=%u",
        xp,coins,car,pack,completed,(unsigned long long)mask,(unsigned)!!cloud);
    if(used<0||(size_t)used>=capacity)return FALSE;
    int flags=snprintf(text+used,capacity-(size_t)used," native_save_skip=%u settings_dirty=%u settings_enabled=%u progression_dirty=%u progression_enabled=%u",
        (unsigned)save_flags[1],(unsigned)preferences[4],(unsigned)preferences[5],(unsigned)chunk[4],(unsigned)chunk[5]);
    if(flags<0||(size_t)flags>=capacity-(size_t)used)return FALSE;used+=flags;
    void *range[3]={NULL,NULL,NULL};
    if(progression_probe_read(base+0x7e9428,&driver,sizeof(driver))&&driver
        &&progression_probe_read(driver,range,sizeof(range))){
        uintptr_t first=(uintptr_t)range[0],end=(uintptr_t)range[1];
        if(first&&end>=first&&(end-first)%4==0&&(end-first)/4>=2&&(end-first)/4<=128){
            int thresholds[128];size_t count=(end-first)/4;
            if(progression_probe_read(range[0],thresholds,count*4)){
                BOOL valid=thresholds[0]>=0;
                for(size_t index=1;index<count;index++)if(thresholds[index]<=thresholds[index-1])valid=FALSE;
                if(valid){
                    int size=snprintf(text+used,capacity-(size_t)used," native_xp_thresholds=");
                    if(size<0||(size_t)size>=capacity-(size_t)used)return FALSE;used+=size;
                    for(size_t index=0;index<count;index++){
                        size=snprintf(text+used,capacity-(size_t)used,"%s%d",index?",":"",thresholds[index]);
                        if(size<0||(size_t)size>=capacity-(size_t)used)return FALSE;used+=size;
                    }
                }
            }
        }
    }
    return TRUE;
}
static DWORD WINAPI progression_probe_worker(LPVOID argument){
    char previous[4096]="",current[4096];
    BOOL names_logged=FALSE;
    for(unsigned sample=0;sample<1200;sample++){
        if(!rr_log_ini(config_path,L"engine_menu"))break;
        if(progression_probe_snapshot((BYTE *)argument,current,sizeof(current))&&strcmp(previous,current)){
            log_line(current);strcpy(previous,current);
            if(!names_logged){progression_probe_save_names((BYTE *)argument);names_logged=TRUE;}
        }
        Sleep(500);
    }
    log_line("Progression read-only observer finished; the observer performs no writes.");
    return 0;
}
static void start_progression_probe(BYTE *base){
    if(!rr_log_ini(config_path,L"engine_menu")||!GetPrivateProfileIntW(L"progression",L"trace_enabled",0,config_path)
        ||InterlockedCompareExchange(&progression_probe_started,1,0))return;
    progression_probe_startup(base);
    HANDLE thread=CreateThread(NULL,0,progression_probe_worker,base,0,NULL);
    if(thread)CloseHandle(thread);else log_line("Progression observation could not start.");
}
