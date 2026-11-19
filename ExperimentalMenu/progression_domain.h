/* Opt-in singleplayer save-domain prototype. Original serialization, chunk
 * lifecycle and native I/O are retained. No prestige/reset/reward hooks here.
 * Native provenance: reports/PRESTIGE-NATIVE-BOUNDARIES.md. */
typedef BYTE (__thiscall *ProgressExistsFn)(void *,const char *);
typedef BYTE (__thiscall *ProgressCreateFn)(void *,const char *,DWORD);
typedef BYTE (__thiscall *ProgressOpenFn)(void *,const char *,DWORD);
typedef BYTE (__thiscall *ProgressCloseFn)(void *);
typedef DWORD (__thiscall *ProgressSizeFn)(void *);
/* Original stack order: offset, buffer, byte count, [read count]. */
typedef BYTE (__thiscall *ProgressReadFn)(void *,DWORD,void *,DWORD,DWORD *);
typedef BYTE (__thiscall *ProgressWriteFn)(void *,DWORD,const void *,DWORD);
static ProgressExistsFn progression_exists;
static ProgressCreateFn progression_create;
static ProgressOpenFn progression_open;
static ProgressCloseFn progression_close;
static ProgressWriteFn progression_write;
static ProgressReadFn progression_read;
typedef int (__cdecl *ProgressCheckpointFn)(const WCHAR *,int,int);
static ProgressCheckpointFn progression_checkpoint;
static WCHAR progression_domain_path[MAX_PATH];
static DWORD progression_file_mode;
static BOOL progression_file_target,progression_file_failed,progression_load_observed;
static BOOL progression_domain_native_xp(int *xp);
static void progression_domain_stop(const char *reason);
static ProgressExistsFn progression_delete;
static EnableFn progression_cloud_enable;
static char progression_domain_name[96];
static HANDLE progression_domain_lock;
static BOOL progression_domain_active;
static BOOL progression_domain_multiplayer,progression_initializing,progression_import_original;
static char progression_server_id[37],progression_account_id[37];
static WCHAR progression_seed_root[MAX_PATH];
static LONG progression_io_events;
static LONG progression_domain_pending;
static void *progression_local_device,*progression_cloud_device;
static void progression_domain_ensure(void);
static BOOL progression_domain_root(void *device,WCHAR *root);
#include "ghost_car_test.h"
static void progression_domain_io(void *self,const char *name,const char *operation,DWORD detail,BYTE result){
    if(!progression_domain_active||!name||strcmp(name,"save.bin")||InterlockedIncrement(&progression_io_events)>32)return;
    WCHAR root[MAX_PATH];BOOL same=progression_domain_root(self,root)&&!wcscmp(root,progression_seed_root);
    char message[192];snprintf(message,sizeof(message),"Progression isolated I/O: %s detail=%lu result=%u root_matches_import=%u",operation,(unsigned long)detail,(unsigned)result,(unsigned)same);log_line(message);
}

static const char *progression_domain_route(const char *name){
    if(progression_domain_active&&name&&!strcmp(name,"save.bin")){
        progression_domain_ensure();return progression_domain_name;
    }
    return name;
}
static BYTE __thiscall progression_domain_exists(void *self,const char *name){BYTE result=progression_exists(self,progression_domain_route(name));progression_domain_io(self,name,"exists",0,result);return result;}
static BYTE __thiscall progression_domain_create(void *self,const char *name,DWORD size){BYTE result=progression_create(self,progression_domain_route(name),size);progression_domain_io(self,name,"create",size,result);return result;}
static BYTE __thiscall progression_domain_open(void *self,const char *name,DWORD mode){
    ghost_car_before_open(self,name,mode);
    BYTE result=progression_open(self,progression_domain_route(name),mode);
    if(!result)ghost_car_target=ghost_car_patch_ready=FALSE;
    progression_file_target=result&&progression_domain_active&&name&&!strcmp(name,"save.bin");
    progression_file_mode=mode;progression_file_failed=FALSE;
    progression_domain_io(self,name,"open",mode,result);return result;
}
static BYTE __thiscall progression_domain_write(void *self,DWORD offset,const void *buffer,DWORD size){
    BYTE result=progression_write(self,offset,buffer,size);
    if(ghost_car_target&&!result)ghost_car_write_failed=TRUE;
    if(progression_file_target&&!result)progression_file_failed=TRUE;
    return result;
}
static BYTE __thiscall progression_domain_read(void *self,DWORD offset,void *buffer,DWORD size,DWORD *count){
    BYTE result=progression_read(self,offset,buffer,size,count);
    if(result&&ghost_car_patch_ready&&count&&*count<=size)ghost_car_overlay(offset,buffer,*count,&ghost_car_patch);
    return result;
}
static BYTE __thiscall progression_domain_close(void *self){
    BOOL target=progression_file_target,failed=progression_file_failed;DWORD mode=progression_file_mode;
    progression_file_target=FALSE;progression_file_mode=0;
    BYTE result=progression_close(self);
    ghost_car_after_close(result);
    if(target&&result&&!failed&&progression_checkpoint){
        int phase=mode==1?1:(mode==2||mode==3?2:-1),xp=0;
        if(phase>=0&&(phase!=1||!progression_load_observed)){
            if(!progression_domain_native_xp(&xp)||progression_checkpoint(progression_domain_path,phase,xp)!=1)
                progression_domain_stop("Progression checkpoint failed after native file close; committed checkpoint retained.");
            if(phase==1||(phase==2&&progression_domain_multiplayer))progression_load_observed=TRUE;
        }
    }
    return result;
}
static BYTE __thiscall progression_domain_delete(void *self,const char *name){BYTE result=progression_delete(self,progression_domain_route(name));progression_domain_io(self,name,"delete",0,result);return result;}
/* Match createSaveFiles2's native file-size table and device create call,
 * restricted to the one absent, isolated save. Never delete/recreate a file. */
static BOOL progression_domain_create_initial(void *save){
    BYTE state[176];DWORD count=0,size=0;
    if(!progression_domain_active||!progression_domain_multiplayer||!progression_initializing
        ||progression_domain_pending||progression_load_observed||!progression_create
        ||!progression_probe_read(save,state,sizeof(state))||state[12]!=1||state[13]||state[14])return FALSE;
    memcpy(&count,state+172,4);memcpy(&size,state+160,4);
    if(count!=1||memcmp(state+64,"save.bin",9)||!size||size>1024*1024||!*progression_domain_path)return FALSE;
    if(GetFileAttributesW(progression_domain_path)!=INVALID_FILE_ATTRIBUTES||GetLastError()!=ERROR_FILE_NOT_FOUND)return FALSE;
    return progression_create(progression_local_device,progression_domain_name,size)!=FALSE;
}
static void __thiscall progression_domain_cloud_enable(void *self,BYTE enabled){
    progression_cloud_enable(self,progression_domain_active?FALSE:enabled);
    /* gaSaveGame.load disables its save device after load2 returns. Use that
     * existing lifecycle boundary to serialize a fresh server profile before
     * gameplay, never an imported local/Steam save. Nested save disables are
     * guarded while the original save method runs. */
    if(progression_domain_active&&progression_domain_multiplayer&&!enabled
        &&!progression_domain_pending&&!progression_load_observed&&!progression_initializing&&progression_checkpoint){
        int xp=0;void *save=NULL;progression_initializing=TRUE;
        if(!progression_domain_native_xp(&xp)||xp!=0
            ||!progression_probe_read((BYTE *)GetModuleHandleW(NULL)+0x7ed7b8,&save,sizeof(save))
            ||!progression_domain_create_initial(save)||progression_checkpoint(progression_domain_path,3,xp)!=1)
            progression_domain_stop("Fresh server profile checkpoint failed after native load.");
        progression_initializing=FALSE;
    }
}
static BOOL progression_domain_slot(void **slot,void *expected,void *replacement){
    DWORD protect;
    if(*slot!=expected||!VirtualProtect(slot,sizeof(void *),PAGE_READWRITE,&protect))return FALSE;
    InterlockedExchangePointer(slot,replacement);DWORD ignored;
    return VirtualProtect(slot,sizeof(void *),protect,&ignored);
}
static BOOL progression_domain_hash(DWORD account,char *hex){
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;BYTE digest[32];DWORD size=sizeof(digest);
    char input[128];
    if(progression_domain_multiplayer)snprintf(input,sizeof(input),"ReRiff progression v1|server|%s|account|%s",progression_server_id,progression_account_id);
    else snprintf(input,sizeof(input),"ReRiff progression v1|singleplayer|%lu",(unsigned long)account);
    BOOL ok=CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)
        &&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)
        &&CryptHashData(hash,(const BYTE *)input,(DWORD)strlen(input),0)
        &&CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)&&size==32;
    if(ok){for(unsigned i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);}
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);return ok;
}
static BOOL progression_domain_root(void *device,WCHAR *root){
    BYTE object[44];DWORD length=0,capacity=0;const WCHAR *text=NULL;
    if(!progression_probe_read(device,object,sizeof(object)))return FALSE;
    memcpy(&length,object+36,4);memcpy(&capacity,object+40,4);
    if(!length||length>=MAX_PATH-120||capacity<length)return FALSE;
    if(capacity<8)text=(const WCHAR *)((BYTE *)device+20);else memcpy(&text,object+20,sizeof(text));
    if(!progression_probe_read(text,root,(length+1)*sizeof(WCHAR))||root[length])return FALSE;
    if(root[length-1]!=L'\\'&&root[length-1]!=L'/')return FALSE;
    return TRUE;
}
static BOOL progression_domain_account(BYTE *base,DWORD *account){
    /* Native network getter +16 returns wsNetworkLeaderboard's secondary
     * interface at singleton+4. Its +124 getter reads this+0x1d4. */
    void *network_user=NULL;void **table=NULL,*getter=NULL;
    if(!progression_probe_read(base+0x7fb610,&network_user,sizeof(network_user))||!network_user
        ||!progression_probe_read((BYTE *)network_user+4,&table,sizeof(table))||table!=(void **)(base+0x599d50)
        ||!progression_probe_read(table+31,&getter,sizeof(getter))||getter!=(void *)(base+0x150e0)
        ||!progression_probe_read((BYTE *)network_user+0x1d8,account,sizeof(*account))||!*account)return FALSE;
    static const BYTE expected[]={0x8b,0x81,0xd4,0x01,0,0,0xc3};
    return !memcmp(base+0x150e0,expected,sizeof(expected));
}
static BOOL progression_domain_seed(void *local,void *cloud,BOOL use_cloud,const WCHAR *root,const WCHAR *target){
    /* Read the same original save device the game normally selects, once.
     * No cloud write is issued. All source handles close before switching. */
    void *source=local;void **methods=NULL;BYTE *bytes=NULL;DWORD size=0,read=0;
    BOOL cloud_ready=FALSE,opened=FALSE,ok=FALSE;WCHAR temporary[MAX_PATH]=L"";
    if(use_cloud){
        if(!progression_probe_read((BYTE *)cloud+16,&methods,sizeof(methods))||!methods)return FALSE;
        cloud_ready=TRUE;
    }
    if(cloud_ready){progression_cloud_enable(cloud,TRUE);source=(BYTE *)cloud+4;}
    methods=*(void ***)source;
    ProgressOpenFn open=source==local?progression_open:(ProgressOpenFn)methods[2];ProgressCloseFn close=(ProgressCloseFn)methods[3];
    ProgressSizeFn get_size=(ProgressSizeFn)methods[4];ProgressReadFn read_file=(ProgressReadFn)methods[6];
    opened=open(source,"save.bin",1);
    if(!opened){
        /* This import prototype requires an existing original save. Cloud
         * availability cannot be inferred from a failed open/exists pair. */
        log_line("Progression import refused: original save open failed; no empty-profile fallback.");
        goto finish_read;
    }
    size=get_size(source);if(!size||size>1024*1024)goto finish_read;
    bytes=HeapAlloc(GetProcessHeap(),0,size);if(!bytes)goto finish_read;
    ok=read_file(source,0,bytes,size,&read)&&read==size;
    if(ok){
        HCRYPTPROV provider=0;HCRYPTHASH hash=0;BYTE digest[32];DWORD count=32;char hex[65]={0};
        if(CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)
            &&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)&&CryptHashData(hash,bytes,size,0)
            &&CryptGetHashParam(hash,HP_HASHVAL,digest,&count,0)&&count==32)
            for(unsigned i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
        if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);
        char message[192];snprintf(message,sizeof(message),"Progression import source: cloud=%u bytes=%lu sha256=%s",(unsigned)cloud_ready,(unsigned long)size,hex);log_line(message);
    }
finish_read:
    if(opened)close(source);
    if(cloud_ready)progression_cloud_enable(cloud,FALSE);
    if(!ok||!bytes)goto done;
    if(!GetTempFileNameW(root,L"rr4",0,temporary)){ok=FALSE;goto done;}
    WCHAR *last=wcsrchr(temporary,L'\\');if(!last){ok=FALSE;goto done;}
    char name[128];if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,last+1,-1,name,sizeof(name),NULL,NULL)){ok=FALSE;goto done;}
    methods=*(void ***)local;
    if(!progression_create(local,name,size)){ok=FALSE;goto done;}
    if(!progression_open(local,name,2)){ok=FALSE;goto done;}
    ok=((ProgressWriteFn)methods[7])(local,0,bytes,size)!=FALSE;
    ((ProgressCloseFn)methods[3])(local);
    if(ok)ok=MoveFileExW(temporary,target,MOVEFILE_WRITE_THROUGH);
done:
    if(*temporary)DeleteFileW(temporary);
    if(bytes)HeapFree(GetProcessHeap(),0,bytes);
    return ok;
}
static BOOL progression_domain_begin(void){
    BYTE use_cloud=0;WCHAR root[MAX_PATH],target[MAX_PATH],lock_path[MAX_PATH],name[96];
    if(!progression_probe_read((BYTE *)GetModuleHandleW(NULL)+0x7ed7bc,&use_cloud,1)||use_cloud>1
        ||!progression_domain_root(progression_local_device,root))return FALSE;
    wcscpy(progression_seed_root,root);
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,progression_domain_name,-1,name,96))return FALSE;
    _snwprintf(target,MAX_PATH,L"%ls%ls",root,name);target[MAX_PATH-1]=0;
    _snwprintf(lock_path,MAX_PATH,L"%ls.lock",target);lock_path[MAX_PATH-1]=0;
    progression_domain_lock=CreateFileW(lock_path,GENERIC_READ|GENERIC_WRITE|DELETE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL);
    if(progression_domain_lock==INVALID_HANDLE_VALUE){progression_domain_lock=NULL;return FALSE;}
    wcscpy(progression_domain_path,target);
    if(progression_checkpoint&&progression_checkpoint(target,0,0)!=1){
        CloseHandle(progression_domain_lock);progression_domain_lock=NULL;return FALSE;
    }
    if((!progression_domain_multiplayer||progression_import_original)&&!progression_exists(progression_local_device,progression_domain_name)
        &&!progression_domain_seed(progression_local_device,progression_cloud_device,use_cloud,root,target)){
        CloseHandle(progression_domain_lock);progression_domain_lock=NULL;return FALSE;
    }
    log_line(progression_domain_multiplayer?"Server checkpoint loaded before native save-file access; original save is not imported.":"Singleplayer progression import prepared at first native save-file access.");
    return TRUE;
}
static BOOL progression_domain_native_xp(int *xp){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void *save=NULL,*chunk=NULL;BYTE flags[4];
    return progression_probe_read(base+0x7ed7b8,&save,sizeof(save))&&save
        &&progression_probe_read((BYTE *)save+12,flags,sizeof(flags))&&flags[0]&&!flags[2]
        &&progression_probe_read((BYTE *)save+32,&chunk,sizeof(chunk))&&chunk
        &&progression_probe_read((BYTE *)chunk+280,xp,sizeof(*xp))&&*xp>=0&&*xp<=2000000000;
}
static void progression_domain_stop(const char *reason){
    log_line(reason);
    MessageBoxW(NULL,L"The experimental progression checkpoint could not be prepared or committed safely. ReRiff will close without resetting the original save. See logs/ReRiff-game.log.",L"ReRiff experimental progression",MB_OK|MB_ICONERROR);
    ExitProcess(1);
}
__declspec(dllexport) int __cdecl ReRiffConfigureProgressionCheckpoint(ProgressCheckpointFn callback){
    if(!callback||progression_domain_active)return 0;progression_checkpoint=callback;return 1;
}
static void progression_domain_ensure(void){
    if(!progression_domain_pending)return;
    if(InterlockedCompareExchange(&progression_domain_pending,2,1)==1&&progression_domain_begin()){
        InterlockedExchange(&progression_domain_pending,0);return;
    }
    progression_domain_stop("Progression isolation refused at native save access; original save retained, empty-profile fallback blocked.");
}
static int progression_domain_prepare(void){
    if(progression_domain_active)return 1;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void *save=NULL,*local=NULL,*cloud=NULL,*current=NULL;
    DWORD account=0;WCHAR root[MAX_PATH];char digest[65]={0};
    void **local_table=NULL,**cloud_table=NULL,**cloud_files=NULL;
    if(installed!=2||!progression_checkpoint||!progression_probe_read(base+0x7ed7b8,&save,sizeof(save))||save
        ||!progression_probe_read(base+0x7fcd08,&local,sizeof(local))||!local
        ||!progression_probe_read(base+0x7fcd30,&cloud,sizeof(cloud))||!cloud
        ||!progression_probe_read(base+0x7fcb4c,&current,sizeof(current))||current!=local
        ||!progression_probe_read(local,&local_table,sizeof(local_table))||local_table!=(void **)(base+0x5ffe2c)
        ||!progression_probe_read(cloud,&cloud_table,sizeof(cloud_table))||cloud_table!=(void **)(base+0x600eb4)
        ||!progression_probe_read((BYTE *)cloud+4,&cloud_files,sizeof(cloud_files))||cloud_files!=(void **)(base+0x600ec8)
        ||cloud_table[0]!=(void *)(base+0x16ca20)
        ||local_table[0]!=(void *)(base+0x135810)||local_table[1]!=(void *)(base+0x1358f0)
        ||local_table[2]!=(void *)(base+0x135b10)||local_table[5]!=(void *)(base+0x135cc0)
        ||local_table[3]!=(void *)(base+0x135c20)||local_table[4]!=(void *)(base+0x135c50)
        ||local_table[6]!=(void *)(base+0x135d80)||local_table[7]!=(void *)(base+0x135de0)
        ||cloud_files[0]!=(void *)(base+0x16ca60)||cloud_files[1]!=(void *)(base+0x16ca70)
        ||cloud_files[2]!=(void *)(base+0x16cb30)||cloud_files[3]!=(void *)(base+0x16cbe0)
        ||cloud_files[4]!=(void *)(base+0x16cca0)||cloud_files[6]!=(void *)(base+0x16ccc0)
        ||(!progression_domain_multiplayer&&!progression_domain_account(base,&account))||!progression_domain_hash(account,digest)
        ||!progression_domain_root(local,root)){
        log_line("Progression isolation refused: native startup, account or save-device guards differ.");return -1;
    }
    progression_exists=(ProgressExistsFn)local_table[0];progression_create=(ProgressCreateFn)local_table[1];
    progression_close=(ProgressCloseFn)local_table[3];progression_write=(ProgressWriteFn)local_table[7];
    progression_read=(ProgressReadFn)local_table[6];
    progression_open=(ProgressOpenFn)local_table[2];progression_delete=(ProgressExistsFn)local_table[5];
    progression_cloud_enable=(EnableFn)cloud_table[0];
    progression_local_device=local;progression_cloud_device=cloud;
    snprintf(progression_domain_name,sizeof(progression_domain_name),"reriff-%s-%s.bin",progression_domain_multiplayer?"mp":"sp",digest);
    BOOL patched[8]={FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};
    void **slots[]={local_table,local_table+1,local_table+2,local_table+5,cloud_table,local_table+3,local_table+7,local_table+6};
    void *originals[]={(void *)progression_exists,(void *)progression_create,(void *)progression_open,(void *)progression_delete,(void *)progression_cloud_enable,(void *)progression_close,(void *)progression_write,(void *)progression_read};
    void *hooks[]={(void *)&progression_domain_exists,(void *)&progression_domain_create,(void *)&progression_domain_open,(void *)&progression_domain_delete,(void *)&progression_domain_cloud_enable,(void *)&progression_domain_close,(void *)&progression_domain_write,(void *)&progression_domain_read};
    for(unsigned i=0;i<(ghost_car_process?8u:7u);i++){
        patched[i]=progression_domain_slot(slots[i],originals[i],hooks[i]);
        if(!patched[i]){
            for(unsigned j=0;j<=i;j++)if(*slots[j]==hooks[j])progression_domain_slot(slots[j],hooks[j],originals[j]);
            CloseHandle(progression_domain_lock);progression_domain_lock=NULL;
            log_line("Progression isolation refused: native table update failed; original calls restored.");return -1;
        }
    }
    InterlockedExchange(&progression_domain_pending,1);
    progression_domain_active=TRUE;
    if(!first_prestige_entry_install(base))log_line("First Prestige list extension unavailable; original achievement list retained.");
    log_line("Progression isolation armed; load waits for first native save-file access; reset requires eligible Credits confirmation.");
    return 1;
}

__declspec(dllexport) int __cdecl ReRiffPrepareProgressionDomain(int singleplayer){
    if(!singleplayer||progression_domain_multiplayer||!GetPrivateProfileIntW(L"progression",L"isolate_singleplayer",0,config_path))return 0;
    return progression_domain_prepare();
}
__declspec(dllexport) int __cdecl ReRiffPrepareMultiplayerProgression(const char *server_id,const char *account_id,int import_original){
    if(progression_domain_active||!server_id||!readable(server_id,37)||server_id[36]
        ||!account_id||!readable(account_id,37)||account_id[36]
        ||!GetPrivateProfileIntW(L"multiplayer_progression",L"multiplayer_enabled",1,config_path))return 0;
    for(unsigned i=0;i<36;i++){
        if(i==8||i==13||i==18||i==23){if(server_id[i]!='-'||account_id[i]!='-')return 0;}
        else if(!((server_id[i]>='0'&&server_id[i]<='9')||(server_id[i]>='a'&&server_id[i]<='f'))||!((account_id[i]>='0'&&account_id[i]<='9')||(account_id[i]>='a'&&account_id[i]<='f')))return 0;
    }
    if(import_original!=0&&import_original!=1)return 0;
    progression_import_original=import_original!=0;
    memcpy(progression_server_id,server_id,37);memcpy(progression_account_id,account_id,37);progression_domain_multiplayer=TRUE;
    int result=progression_domain_prepare();
    if(result!=1){progression_domain_multiplayer=FALSE;memset(progression_server_id,0,sizeof(progression_server_id));}
    return result;
}
