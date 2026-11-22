#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int ticks,enters,observations,event,state_seen,forced,shown,pauses,hud_toggles;static BYTE *fake_hud;static BYTE paused_value;
static void __cdecl observe(int kind,int state){observations++;event=kind;state_seen=state;}
static void __thiscall tick(void *self){(void)self;ticks++;}
static void __thiscall enter(void *self,int state){(void)self;(void)state;enters++;}
static void __thiscall force_visible(void *self,int view,BYTE visible){(void)self;if(view>=0&&view<4&&visible)forced++;}
static void __thiscall on_visible(void *self,int view,BYTE visible){(void)self;if(view>=0&&view<4&&visible)shown++;}
static void __thiscall set_paused(void *self,BYTE paused){paused_value=paused;*((BYTE *)self+12)=paused;pauses++;}
static void __cdecl toggle_hud(void){if(fake_hud){*fake_hud=*fake_hud?0:1;hud_toggles++;}}
int main(void){
    BYTE *base=VirtualAlloc(NULL,0x800000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(base);
    CHECK(install_scrobbler(base));CHECK(!ReRiffConfigureScrobbler(NULL));CHECK(ReRiffConfigureScrobbler(observe));CHECK(!ReRiffConfigureScrobbler(observe));
    void **table=(void **)(base+0x7a399c);table[2]=base+0x21ef04;table[3]=base+0x21f01f;
    CHECK(!install_scrobbler(base)&&table[2]==base+0x21ef04);
    base[0x21ef04]=base[0x21f01f]=0xff;base[0x21ef05]=base[0x21f020]=0x25;
    *(DWORD *)(base+0x21ef06)=(DWORD)(base+0x7a39f8);*(DWORD *)(base+0x21f021)=(DWORD)(base+0x7a39fc);
    CHECK(install_scrobbler(base));CHECK(table[2]==(void *)scrobble_enter&&table[3]==(void *)scrobble_tick);
    scrobble_enter_original=enter;scrobble_tick_original=tick;
    scrobble_enter(base,45);CHECK(enters==1&&observations==1&&event==1&&state_seen==45);
    scrobble_last_tick=GetTickCount()-251;scrobble_tick(base);CHECK(ticks==1&&observations==2&&event==0);
    scrobble_last_tick=GetTickCount();scrobble_tick(base);CHECK(ticks==2&&observations==2);
    reward_race_original=enter;progression_domain_active=FALSE;reward_race_enter(base,0);CHECK(enters==2&&observations==3&&event==2);
    BYTE *manager=VirtualAlloc(NULL,64,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);BYTE *volume=VirtualAlloc(NULL,320,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);void **items=VirtualAlloc(NULL,8,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(manager&&volume&&items);
    items[0]=volume;*(void ***)(manager+28)=items;*(void ***)(manager+32)=items+1;
    race_culling_force_visible=force_visible;race_culling_on_visible=on_visible;
    race_culling_apply(manager,TRUE);
    for(int view=0;view<12;view++)CHECK(((WORD *)(volume+124))[view]==1);
    CHECK(forced==4&&shown==4);
    race_culling_apply(manager,FALSE);CHECK(forced==4&&shown==4);
    race_culling_apply(NULL,TRUE);CHECK(forced==4&&shown==4);
    VirtualFree(items,0,MEM_RELEASE);VirtualFree(volume,0,MEM_RELEASE);VirtualFree(manager,0,MEM_RELEASE);
    void *race=base+0x100000,*camera_manager=base+0x110000,*set=base+0x112000,*viewport=base+0x114000,*camera=base+0x116000;
    *(void **)(base+0x7d6668)=race;*(void **)(base+0x7cb408)=camera_manager;
    ((void **)camera_manager)[0]=set;*(void **)((BYTE *)set+8)=viewport;*(void **)((BYTE *)viewport+428)=camera;
    float *matrix=(float *)((BYTE *)camera+68);for(int i=0;i<12;i++)matrix[i]=0;
    matrix[0]=matrix[4]=matrix[8]=matrix[9]=1.0f;
    fake_hud=base+0x7a2f48;*fake_hud=1;race_hud_toggle=toggle_hud;race_set_paused=set_paused;
    CHECK(race_photo_begin(base));CHECK(race_photo_active&&race_photo_paused_by_us&&race_photo_hud_hidden);
    CHECK(pauses==1&&paused_value==1&&*((BYTE *)race+12)==1&&*fake_hud==0&&hud_toggles==1);
    race_photo_end(base,TRUE);CHECK(!race_photo_active&&!race_photo_paused_by_us&&!race_photo_hud_hidden);
    CHECK(pauses==2&&paused_value==0&&*((BYTE *)race+12)==0&&*fake_hud==1&&hud_toggles==2);
    VirtualFree(base,0,MEM_RELEASE);
    puts("PASS: Last.fm race guards, native forwarding, throttling, pause entry, completion, culling visibility and photo pause/HUD state independent of progression");return 0;
}
