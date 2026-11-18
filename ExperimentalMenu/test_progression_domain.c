#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s (%lu)\n",__LINE__,#c,(unsigned long)GetLastError());return 1;}}while(0)
static BYTE local_device[64],cloud_device[64];
static WCHAR directory[MAX_PATH];static HANDLE destination=INVALID_HANDLE_VALUE;
static const BYTE payload[]={1,2,3,4,5,0,7,8,9};
static BOOL source_opens=TRUE,short_read,forwarding,initial_creation;
static int creates;
static int opens,closes,cloud_calls;static BYTE last_enabled;
static const char *last_name;static void *last_self;static DWORD last_mode;
static void full_path(const char *name,WCHAR *path){WCHAR wide[128];MultiByteToWideChar(CP_UTF8,0,name,-1,wide,128);_snwprintf(path,MAX_PATH,L"%ls%ls",directory,wide);path[MAX_PATH-1]=0;}
static BYTE __thiscall exists_mock(void *self,const char *name){last_self=self;last_name=name;return TRUE;}
static BYTE __thiscall create_mock(void *self,const char *name,DWORD size){
    last_self=self;last_name=name;last_mode=size;creates++;
    if(initial_creation){
        WCHAR path[MAX_PATH];full_path(name,path);HANDLE f=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(f==INVALID_HANDLE_VALUE)return FALSE;
        SetFilePointer(f,(LONG)size,NULL,FILE_BEGIN);BOOL ok=SetEndOfFile(f);CloseHandle(f);return ok;
    }
    if(!forwarding&&(!strcmp(name,"save.bin")||size!=sizeof(payload)))abort();return TRUE;
}
static BYTE __thiscall open_mock(void *self,const char *name,DWORD mode){
    last_self=self;last_name=name;last_mode=mode;opens++;
    if(forwarding)return 1;
    if(mode==1){if(strcmp(name,"save.bin"))abort();return source_opens;}
    if(!strcmp(name,"save.bin")||mode!=2)abort();
    WCHAR path[MAX_PATH];full_path(name,path);destination=CreateFileW(path,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    return destination!=INVALID_HANDLE_VALUE;
}
static BYTE __thiscall close_mock(void *self){(void)self;closes++;if(destination!=INVALID_HANDLE_VALUE){CloseHandle(destination);destination=INVALID_HANDLE_VALUE;}return TRUE;}
static DWORD __thiscall size_mock(void *self){(void)self;return sizeof(payload);}
static BYTE __thiscall read_mock(void *self,DWORD offset,void *buffer,DWORD size,DWORD *received){
    (void)self;if(offset||size!=sizeof(payload))abort();memcpy(buffer,payload,size);*received=short_read?size-1:size;return TRUE;
}
static BYTE __thiscall write_mock(void *self,DWORD offset,const void *buffer,DWORD size){
    if(forwarding){last_self=self;last_mode=size;return offset==0&&buffer!=NULL;}
    (void)self;if(offset||size!=sizeof(payload))abort();DWORD written=0;
    return WriteFile(destination,buffer,size,&written,NULL)&&written==size;
}
static void __thiscall cloud_mock(void *self,BYTE enabled){if(self!=cloud_device)abort();last_enabled=enabled;cloud_calls++;}
static BOOL bytes_match(const WCHAR *path,const BYTE *expected,DWORD size){
    BYTE buffer[64];DWORD received=0;HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    BOOL ok=ReadFile(file,buffer,sizeof(buffer),&received,NULL)&&received==size&&!memcmp(buffer,expected,size);CloseHandle(file);return ok;
}
int main(void){
    progression_exists=exists_mock;progression_create=create_mock;progression_open=open_mock;
    progression_delete=exists_mock;progression_cloud_enable=cloud_mock;
    progression_close=close_mock;progression_write=write_mock;
    strcpy(progression_domain_name,"isolated.bin");progression_domain_active=TRUE;forwarding=TRUE;
    char original[]="save.bin",other[]="songcache.bin";
    CHECK(progression_domain_exists(local_device,original));CHECK(!strcmp(last_name,"isolated.bin")&&last_self==local_device);
    CHECK(progression_domain_create(local_device,original,123)&&last_mode==123);
    CHECK(progression_domain_open(local_device,original,3)&&last_mode==3);
    CHECK(progression_file_target&&progression_file_mode==3);
    CHECK(progression_domain_write(local_device,0,original,sizeof(original))&&last_mode==sizeof(original));
    CHECK(progression_domain_close(local_device)&&!progression_file_target&&progression_file_mode==0);
    CHECK(progression_domain_delete(local_device,original));CHECK(!strcmp(last_name,"isolated.bin"));
    CHECK(progression_domain_open(local_device,other,2)&&last_name==other);
    progression_domain_pending=1;
    CHECK(progression_domain_route(other)==other&&progression_domain_pending==1);
    progression_domain_pending=0;
    CHECK(!strcmp(original,"save.bin"));
    progression_domain_cloud_enable(cloud_device,TRUE);CHECK(last_enabled==FALSE);
    progression_domain_active=FALSE;progression_domain_cloud_enable(cloud_device,TRUE);CHECK(last_enabled==TRUE);
    CHECK(progression_domain_open(local_device,original,1)&&last_name==original);
    char first[65]={0},again[65]={0},second[65]={0};CHECK(progression_domain_hash(1,first));CHECK(progression_domain_hash(1,again));CHECK(progression_domain_hash(2,second));
    CHECK(strlen(first)==64&&!strcmp(first,again)&&strcmp(first,second));
    progression_domain_multiplayer=TRUE;strcpy(progression_account_id,"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");strcpy(progression_server_id,"12345678-1234-1234-1234-123456789abc");
    char multiplayer[65]={0},another[65]={0};CHECK(progression_domain_hash(1,multiplayer));CHECK(strcmp(first,multiplayer));CHECK(progression_domain_hash(0,again));CHECK(!strcmp(again,multiplayer));
    strcpy(progression_server_id,"12345678-1234-1234-1234-123456789abd");CHECK(progression_domain_hash(1,another));CHECK(strcmp(multiplayer,another));
    char account_changed[65]={0};strcpy(progression_account_id,"bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb");CHECK(progression_domain_hash(1,account_changed));CHECK(strcmp(account_changed,another));
    progression_domain_multiplayer=FALSE;
    WCHAR temp[MAX_PATH],path[MAX_PATH];CHECK(GetTempPathW(MAX_PATH,temp));CHECK(GetTempFileNameW(temp,L"rrd",0,path));CHECK(DeleteFileW(path));CHECK(CreateDirectoryW(path,NULL));
    wcscpy(directory,path);wcscat(directory,L"\\");
    void *methods[]={(void *)exists_mock,(void *)create_mock,(void *)open_mock,(void *)close_mock,(void *)size_mock,(void *)exists_mock,(void *)read_mock,(void *)write_mock};
    *(void ***)local_device=methods;*(void ***)(cloud_device+4)=methods;*(void **)(cloud_device+16)=(void *)1;
    forwarding=FALSE;full_path("isolated.bin",path);cloud_calls=0;closes=0;
    CHECK(progression_domain_seed(local_device,cloud_device,TRUE,directory,path));
    CHECK(bytes_match(path,payload,sizeof(payload)));CHECK(cloud_calls==2&&last_enabled==FALSE&&closes==2);
    /* Existing target is never replaced, even if the caller races creation. */
    CHECK(!progression_domain_seed(local_device,cloud_device,TRUE,directory,path));CHECK(bytes_match(path,payload,sizeof(payload)));
    CHECK(DeleteFileW(path));short_read=TRUE;
    CHECK(!progression_domain_seed(local_device,cloud_device,TRUE,directory,path));CHECK(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES);CHECK(last_enabled==FALSE);
    short_read=FALSE;source_opens=FALSE;
    CHECK(!progression_domain_seed(local_device,cloud_device,TRUE,directory,path));CHECK(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES);CHECK(last_enabled==FALSE);
    /* A configured cloud save must not silently import a stale local copy. */
    *(void **)(cloud_device+16)=NULL;source_opens=TRUE;cloud_calls=0;
    CHECK(!progression_domain_seed(local_device,cloud_device,TRUE,directory,path));CHECK(cloud_calls==0);
    /* Explicitly local saves keep using local I/O even with a cloud backend. */
    *(void **)(cloud_device+16)=(void *)1;
    /* Import must bypass installed name hooks when opening its source. */
    methods[2]=(void *)progression_domain_open;progression_domain_active=TRUE;
    CHECK(progression_domain_seed(local_device,cloud_device,FALSE,directory,path));
    CHECK(bytes_match(path,payload,sizeof(payload))&&cloud_calls==0);CHECK(DeleteFileW(path));
    /* A fresh native save opens mode 3 only after original device allocation. */
    BYTE save[176]={0};save[12]=1;strcpy((char *)save+64,"save.bin");DWORD capacity=4096,count=1;
    memcpy(save+160,&capacity,4);memcpy(save+172,&count,4);
    full_path("isolated.bin",progression_domain_path);progression_local_device=local_device;
    progression_domain_multiplayer=TRUE;progression_initializing=TRUE;initial_creation=TRUE;
    int before=creates;CHECK(progression_domain_create_initial(save));CHECK(creates==before+1&&last_mode==capacity);
    HANDLE created=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);CHECK(created!=INVALID_HANDLE_VALUE);CHECK(GetFileSize(created,NULL)==capacity);CloseHandle(created);
    CHECK(!progression_domain_create_initial(save)&&creates==before+1);CHECK(DeleteFileW(path));
    save[13]=1;CHECK(!progression_domain_create_initial(save));save[13]=0;
    count=2;memcpy(save+172,&count,4);CHECK(!progression_domain_create_initial(save));count=1;memcpy(save+172,&count,4);
    strcpy((char *)save+64,"other.bin");CHECK(!progression_domain_create_initial(save));strcpy((char *)save+64,"save.bin");
    progression_load_observed=TRUE;CHECK(!progression_domain_create_initial(save));progression_load_observed=FALSE;
    CHECK(creates==before+1&&GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES);
    directory[wcslen(directory)-1]=0;CHECK(RemoveDirectoryW(directory));
    puts("PASS: native save-name routing, unchanged arguments, account separation, exact one-time copy, no overwrite and failed-read cleanup");return 0;
}
