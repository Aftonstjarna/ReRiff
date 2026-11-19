#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    BYTE *image=VirtualAlloc(NULL,0x364000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
    BYTE *body=image+0x363884;
    CHECK(!install_recent_tracks(image));
    memcpy(body,recent_tracks_filter_original,sizeof(recent_tracks_filter_original));
    /* Every unexpected original byte must refuse installation, not just the site. */
    for(unsigned i=0;i<sizeof(recent_tracks_filter_original);i++){
        body[i]^=1;CHECK(!install_recent_tracks(image));body[i]^=1;
    }
    CHECK(install_recent_tracks(image));
    for(unsigned i=0;i<sizeof(recent_tracks_filter_original);i++)
        CHECK(body[i]==(i==0x99?0x16:recent_tracks_filter_original[i]));
    CHECK(install_recent_tracks(image));
    body[0]^=1;CHECK(!install_recent_tracks(image));body[0]^=1;
    recent_tracks_installed=FALSE;CHECK(!install_recent_tracks(image));
    WCHAR temp[MAX_PATH];CHECK(GetTempPathW(MAX_PATH,temp));CHECK(GetTempFileNameW(temp,L"rrt",0,config_path));
    const int limits[]={0,20,50,100,999};
    for(unsigned n=0;n<sizeof(limits)/sizeof(limits[0]);n++){
        WCHAR value[16];_snwprintf(value,16,L"%d",limits[n]);CHECK(WritePrivateProfileStringW(L"graphics",L"recent_tracks_limit",value,config_path));
        memcpy(body,recent_tracks_filter_original,sizeof(recent_tracks_filter_original));recent_tracks_installed=FALSE;
        CHECK(install_recent_tracks(image));int limit=limits[n]==999?0:limits[n];
        CHECK(body[0x58]==(limit?limit:20)&&body[0x99]==(limit?0x17:0x16));CHECK(install_recent_tracks(image));
    }
    DeleteFileW(config_path);config_path[0]=0;
    VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: Recent Tracks full-method guard, one-byte CIL patch, idempotence and foreign-patch refusal");return 0;
}
