#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE owner[512],number[512];static DWORD friend_length;
static int texts,fills,ticks,mode;
static int __cdecl summary_mock(WCHAR *text,int capacity,float *factor,int xp){
    if(capacity!=64||xp!=55)abort();
    wcscpy(text,L"40 *x2");*factor=1;
    if(mode==1)return 0;if(mode==2)*factor=2;if(mode==3)wmemset(text,L'x',64);
    return 1;
}
static void __thiscall text_mock(void *item,const WCHAR *text){if(item!=number||wcscmp(text,L"40 *x2"))abort();texts++;}
static void __cdecl fill_mock(void *menu,float factor){if(menu!=owner||factor!=1)abort();fills++;}
static void __thiscall tick_mock(void *menu){if(menu!=owner)abort();ticks++;}
int main(void){
    void *callbacks[2]={(void *)summary_mock,(void *)fill_mock};
    CHECK(!ReRiffConfigureProgressionView(NULL,2)&&!ReRiffConfigureProgressionView(callbacks,1));
    CHECK(ReRiffConfigureProgressionView(callbacks,2));
    progression_friend_length=&friend_length;raw_text=text_mock;*(void **)(owner+276)=number;
    CHECK(progression_view_update(owner,55)&&texts==1&&fills==1);
    for(int section=1;section<=3;section++){((int *)owner)[101]=section;CHECK(!progression_view_update(owner,55)&&texts==1&&fills==1);}((int *)owner)[101]=0;
    friend_length=1;CHECK(!progression_view_update(owner,55)&&texts==1);friend_length=0;
    for(mode=1;mode<=3;mode++)CHECK(!progression_view_update(owner,55)&&texts==1&&fills==1);mode=0;
    *(void **)(owner+276)=NULL;CHECK(!progression_view_update(owner,55));*(void **)(owner+276)=number;
    progression_profile_original=tick_mock;progression_domain_active=FALSE;progression_profile_tick(owner);CHECK(ticks==1&&texts==1);
    BYTE *image=VirtualAlloc(NULL,0x7b0000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
    void **slot=(void **)(image+0x7a6f94)+3;BYTE *thunk=image+0x32c53c;*slot=thunk;
    CHECK(!install_progression_view(image)&&*slot==thunk);
    thunk[0]=0xff;thunk[1]=0x25;*(DWORD *)(thunk+2)=(DWORD)(image+0x7a7004);
    BYTE getter[]={0x8b,0x81,0xcc,0x01,0,0,0x8b,0x40,0x64,0xc3};memcpy(image+0x51110,getter,sizeof(getter));
    void **cleanup=(void **)(image+0x7a6f94)+16;*cleanup=image+0x328aa8;image[0x328aa8]=0xff;image[0x328aa9]=0x25;*(DWORD *)(image+0x328aaa)=(DWORD)(image+0x7a7000);
    CHECK(install_progression_view(image)&&*slot==(void *)progression_profile_tick&&progression_profile_original==(TickFn)thunk);
    VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: original Profile tick forwarding, guarded hook, self-only display, native text/fill callbacks, invalid output refusal and no progression writes");return 0;
}
