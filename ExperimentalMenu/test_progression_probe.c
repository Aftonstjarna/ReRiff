#include "engine_menu.c"
#define CHECK(test) do {if(!(test)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#test);return 1;}}while(0)
int main(void){
    BYTE getter[8]={0x8b,0x41,0x44,0xc3};DWORD offset=0;
    CHECK(progression_probe_getter_offset(getter,&offset)&&offset==68);
    getter[2]=0xfc;CHECK(!progression_probe_getter_offset(getter,&offset));
    BYTE large_getter[8]={0x8b,0x81,0x00,0x02,0,0,0xc3,0};
    CHECK(progression_probe_getter_offset(large_getter,&offset)&&offset==512);
    large_getter[3]=0x10;CHECK(!progression_probe_getter_offset(large_getter,&offset));
    BYTE *image=VirtualAlloc(NULL,0x820000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(image);
    BYTE save[64]={0},chunk[296]={0},settings[328]={0};
    int xp=123456,coins=9876,car=21,paint=8;
    int levels[]={0,1000,3000,6000};void *driver[]={levels,levels+4,levels+4};
    *(void **)(image+0x7ed7b8)=save;*(void **)(image+0x7e9428)=driver;image[0x7ed7bc]=1;
    *(void **)(save+32)=chunk;*(void **)(save+16)=settings;
    save[13]=1;settings[4]=1;settings[5]=1;chunk[4]=1;chunk[5]=1;
    memcpy(chunk+280,&xp,4);memcpy(chunk+284,&coins,4);
    memcpy(settings+324,&car,4);memcpy(settings+236+4*car,&paint,4);
    for(int i=0;i<39;i++)chunk[12+i]=1;
    BYTE before_chunk[296],before_settings[328];memcpy(before_chunk,chunk,sizeof(chunk));memcpy(before_settings,settings,sizeof(settings));
    char text[4096];CHECK(progression_probe_snapshot(image,text,sizeof(text)));
    CHECK(strstr(text,"native_xp=123456 coins=9876 equipped_car=21 equipped_pack=8 achievements=38/38"));
    CHECK(strstr(text,"native_xp_thresholds=0,1000,3000,6000"));
    CHECK(strstr(text,"native_save_skip=1 settings_dirty=1 settings_enabled=1 progression_dirty=1 progression_enabled=1"));
    CHECK(!memcmp(before_chunk,chunk,sizeof(chunk))&&!memcmp(before_settings,settings,sizeof(settings)));
    CHECK(!progression_probe_snapshot(image,text,8));
    *(void **)(save+32)=(void *)1;CHECK(!progression_probe_snapshot(image,text,sizeof(text)));
    *(void **)(save+32)=chunk;car=22;memcpy(settings+324,&car,4);
    CHECK(progression_probe_snapshot(image,text,sizeof(text)));CHECK(strstr(text,"equipped_car=22 equipped_pack=-1"));
    levels[1]=levels[0];CHECK(progression_probe_snapshot(image,text,sizeof(text)));CHECK(!strstr(text,"native_xp_thresholds="));
    VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: read-only native progression observation, bounds, invalid pointers and threshold validation");return 0;
}
