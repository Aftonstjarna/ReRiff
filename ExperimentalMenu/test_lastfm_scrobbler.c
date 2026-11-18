#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int ticks,enters,observations,event,state_seen;
static void __cdecl observe(int kind,int state){observations++;event=kind;state_seen=state;}
static void __thiscall tick(void *self){(void)self;ticks++;}
static void __thiscall enter(void *self,int state){(void)self;(void)state;enters++;}
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
    VirtualFree(base,0,MEM_RELEASE);puts("PASS: Last.fm race guards, native forwarding, throttling, pause entry and completion independent of progression");return 0;
}
