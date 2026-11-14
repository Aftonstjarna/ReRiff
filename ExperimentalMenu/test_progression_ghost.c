#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int originals,callbacks;
static void *expected_menu;
static void __cdecl callback_mock(void){if(originals!=1)abort();callbacks++;}
static void __cdecl different_callback(void){abort();}
static void __thiscall original_mock(void *menu,int state){if(menu!=expected_menu||state!=9)abort();originals++;}
static void __thiscall tokens_mock(void *menu,int state){if(menu!=expected_menu||state!=9||callbacks!=2)abort();}
int main(void){
    CHECK(!ReRiffConfigureGhostAchievement(NULL,0));CHECK(!ReRiffConfigureGhostAchievement((void *)callback_mock,2));CHECK(!ReRiffConfigureGhostAchievement((void *)callback_mock,1));
    CHECK(ReRiffConfigureGhostAchievement((void *)callback_mock,0));CHECK(!ReRiffConfigureGhostAchievement((void *)different_callback,1));
    BYTE *image=VirtualAlloc(NULL,0x7b0000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);BYTE *entry=image+0x2347e0;void **slot=(void **)(image+0x7a4048)+2;
    entry[0]=0xff;entry[1]=0x25;*(DWORD *)(entry+2)=(DWORD)(image+0x7a40c8);*slot=entry;memcpy(image+0x2346c2,ghost_original_awards,sizeof(ghost_original_awards));
    CHECK(install_progression_ghost(image)&&!ghost_achievement_installed&&*slot==entry);
    CHECK(ReRiffConfigureGhostAchievement((void *)callback_mock,1));image[0x2346c2]^=1;CHECK(!install_progression_ghost(image)&&*slot==entry);image[0x2346c2]^=1;
    entry[1]=0;CHECK(!install_progression_ghost(image)&&*slot==entry);entry[1]=0x25;
    CHECK(install_progression_ghost(image)&&ghost_achievement_installed&&*slot==(void *)ghost_versus_enter);
    CHECK(!memcmp(image+0x2346c2,ghost_original_awards,sizeof(ghost_original_awards)));ghost_versus_original=original_mock;expected_menu=(void *)0x1234;ghost_versus_enter(expected_menu,9);CHECK(originals==1&&callbacks==1);
    reward_race_original=tokens_mock;progression_domain_active=FALSE;reward_race_enter(expected_menu,9);CHECK(callbacks==2);
    CHECK(ReRiffGetGhostAchievementThunk()==(void *)ghost_achievement_dispatch);CHECK(!ReRiffConfigureGhostAchievement((void *)callback_mock,0));VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: exact versus-entry/award guards, native versus forwarding and completed-race fallback, callback registration and unchanged original CIL awards");return 0;
}
