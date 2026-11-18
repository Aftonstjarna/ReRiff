#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static void *seen_context;static int creator_seen,entries,tuneable[6];
static int __cdecl quote_mock(int raw){return raw*3;}
static void __thiscall race_mock(void *menu,int state){(void)state;seen_context=ReRiffGetProgressionRaceContext();(void)menu;entries++;}
static void __thiscall init_mock(void *menu,void *screen,void *overlay){(void)menu;(void)screen;(void)overlay;creator_seen=tuneable[5];}
static void __thiscall creator_mock(void *menu,int state){(void)menu;(void)state;creator_seen=tuneable[5];entries++;}
static int ghost_seen,ghost_calls,ghost_factor=2;
static void *ghost_menu,*ghost_screen,*ghost_overlay;
static int __cdecl ghost_quote_mock(int raw){return ghost_factor<0?-1:raw*ghost_factor;}
static void __thiscall versus_mock(void *menu,void *screen,void *overlay){
    ghost_seen=*reward_ghost_base;ghost_calls++;
    ghost_menu=menu;ghost_screen=screen;ghost_overlay=overlay;
}
int main(void){
    CHECK(!ReRiffConfigureProgressionRewards(NULL));CHECK(ReRiffConfigureProgressionRewards((void *)quote_mock));
    CHECK(!ReRiffConfigureProgressionGhostQuote(NULL));CHECK(ReRiffConfigureProgressionGhostQuote((void *)ghost_quote_mock));
    reward_creator_original=creator_mock;reward_creator_init_original=init_mock;reward_race_original=race_mock;
    void *pointer=tuneable;reward_tuneables=&pointer;tuneable[5]=100;
    progression_domain_active=TRUE;progression_load_observed=TRUE;
    reward_creator_enter(tuneable,0);CHECK(creator_seen==300&&tuneable[5]==100&&entries==1);
    reward_creator_init(tuneable,NULL,NULL);CHECK(creator_seen==300&&tuneable[5]==100);
    /* Use a readable 400-byte menu for the context gate. */
    BYTE menu[400]={0};reward_race_enter(menu,0);CHECK(seen_context==menu&&ReRiffGetProgressionRaceContext()==NULL&&entries==2);
    CHECK(ReRiffGetProgressionRaceContext()==NULL);
    reward_race_context=menu;reward_context_thread=GetCurrentThreadId()+1;CHECK(!ReRiffGetProgressionRaceContext());
    reward_context_thread=GetCurrentThreadId();CHECK(ReRiffGetProgressionRaceContext()==menu);
    reward_race_context=NULL;reward_context_thread=0;
    progression_domain_active=FALSE;reward_creator_enter(tuneable,0);CHECK(creator_seen==100&&tuneable[5]==100);
    BYTE *image=VirtualAlloc(NULL,0x800000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
    *(void **)(image+0x7a3f18)=image+0x22ecec;*(void **)(image+0x7a7b7c)=image+0x368374;
    image[0x22ecec]=image[0x368374]=0xff;image[0x22eced]=image[0x368375]=0x25;
    *(DWORD *)(image+0x22ecee)=(DWORD)(image+0x7a3f90);*(DWORD *)(image+0x368376)=(DWORD)(image+0x7a7be0);
    *(void **)(image+0x7a7bb0)=image+0x368970;image[0x368970]=0xff;image[0x368971]=0x25;*(DWORD *)(image+0x368972)=(DWORD)(image+0x7a7be4);
    BYTE read[]={0x7e,0x13,0x10,0,0x04,0x1f,0x14,0x58,0x4a};memcpy(image+0x368362,read,9);memcpy(image+0x3685a0,read,9);
    *(void **)(image+0x7a4084)=image+0x2333dc;image[0x2333dc]=0xff;image[0x2333dd]=0x25;*(DWORD *)(image+0x2333de)=(DWORD)(image+0x7a40c0);
    BYTE ghost_read[]={0x7e,0x47,0x07,0,4};memcpy(image+0x233223,ghost_read,5);*(int *)(image+0x5301d0)=100;
    image[0x368362]=0;CHECK(!install_progression_rewards(image));CHECK(*(void **)(image+0x7a3f18)==image+0x22ecec);
    image[0x368362]=read[0];image[0x233223]=0;CHECK(!install_progression_rewards(image));CHECK(*(void **)(image+0x7a3f18)==image+0x22ecec);
    image[0x233223]=ghost_read[0];*(int *)(image+0x5301d0)=50;CHECK(!install_progression_rewards(image));*(int *)(image+0x5301d0)=100;
    CHECK(install_progression_rewards(image));CHECK(!ReRiffConfigureProgressionRewards((void *)quote_mock));CHECK(!ReRiffConfigureProgressionGhostQuote((void *)ghost_quote_mock));
    CHECK(*(void **)(image+0x7a3f18)==(void *)reward_race_enter&&*(void **)(image+0x7a7b7c)==(void *)reward_creator_enter);
    CHECK(*(void **)(image+0x7a4084)==(void *)reward_versus_init);
    reward_versus_init_original=versus_mock;progression_domain_active=TRUE;
    DWORD old;CHECK(VirtualProtect(reward_ghost_base,4,PAGE_READONLY,&old));
    for(int factor=1;factor<=3;factor++){
        ghost_factor=factor;reward_versus_init(menu,tuneable,&pointer);
        CHECK(ghost_seen==100*factor&&*reward_ghost_base==100);
        CHECK(ghost_menu==menu&&ghost_screen==tuneable&&ghost_overlay==&pointer);
        MEMORY_BASIC_INFORMATION info;CHECK(VirtualQuery(reward_ghost_base,&info,sizeof(info))&&info.Protect==PAGE_READONLY);
    }
    ghost_factor=-1;reward_versus_init(menu,NULL,NULL);CHECK(ghost_seen==100&&*reward_ghost_base==100);
    ghost_factor=2;progression_domain_active=FALSE;reward_versus_init(menu,NULL,NULL);CHECK(ghost_seen==100);
    progression_domain_active=TRUE;progression_load_observed=FALSE;reward_versus_init(menu,NULL,NULL);CHECK(ghost_seen==100&&ghost_calls==6);
    VirtualFree(image,0,MEM_RELEASE);puts("PASS: scoped creator/versus quotes, repeated multiplier changes, read-only field/protection restoration, inactive/unready bypass, thread-owned payment context and exact native guards");return 0;
}
