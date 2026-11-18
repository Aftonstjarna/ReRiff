#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int calls,events;
static void *seen;
static void __cdecl rank_mock(void *driver){seen=driver;calls++;if(driver==NULL){if(ReRiffGetProgressionRaceContext()!=NULL)abort();events=1;}}
static void __cdecl other_mock(void *driver){(void)driver;abort();}
static void __thiscall race_mock(void *menu,int state){if(events!=1||ReRiffGetProgressionRaceContext()!=menu||state!=7)abort();events=2;}
static void __thiscall creator_mock(void *menu,int state){(void)menu;(void)state;}
int main(void){
    CHECK(!ReRiffConfigureProgressionRank(NULL,0));CHECK(!ReRiffConfigureProgressionRank((void *)rank_mock,2));CHECK(!ReRiffConfigureProgressionRank((void *)rank_mock,1));
    CHECK(ReRiffConfigureProgressionRank((void *)rank_mock,0));CHECK(!ReRiffConfigureProgressionRank((void *)other_mock,1));
    progression_rank_dispatch((void *)0x1234,NULL);CHECK(calls==1&&seen==(void *)0x1234);CHECK(ReRiffGetProgressionRankThunk()==(void *)progression_rank_dispatch);
    BYTE *image=VirtualAlloc(NULL,0x2cc000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);BYTE *method=image+0x2cbdac;
    memcpy(method,progression_rank_header,12);memcpy(method+12,progression_rank_body,160);method[-1]=0xaa;method[172]=0xbb;
    CHECK(install_progression_rank(image)&&!progression_rank_installed&&!memcmp(method+12,progression_rank_body,160));
    CHECK(ReRiffConfigureProgressionRank((void *)rank_mock,1));method[12]^=1;CHECK(!install_progression_rank(image));method[12]^=1;
    method[2]=4;CHECK(!install_progression_rank(image));method[2]=3;
    CHECK(install_progression_rank(image)&&progression_rank_installed);CHECK(!memcmp(method,progression_rank_header,12));CHECK(!memcmp(method+12,progression_rank_body,160));CHECK(method[-1]==0xaa&&method[172]==0xbb);
    progression_domain_active=FALSE;progression_load_observed=TRUE;progression_rank_after_race();CHECK(calls==1);
    progression_domain_active=TRUE;progression_load_observed=FALSE;progression_rank_after_race();CHECK(calls==1);
    BYTE menu[512]={0};progression_load_observed=TRUE;reward_race_original=race_mock;reward_creator_original=creator_mock;
    reward_creator_enter(menu,7);CHECK(calls==1);reward_race_enter(menu,7);CHECK(calls==2&&seen==NULL&&events==2&&ReRiffGetProgressionRaceContext()==NULL);
    CHECK(!ReRiffConfigureProgressionRank((void *)rank_mock,0));VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: Rank guards/ABI, original Rank CIL remains unchanged, recheck on completed-race results only, before coin scope; no generation/inactive-domain recheck");return 0;
}
