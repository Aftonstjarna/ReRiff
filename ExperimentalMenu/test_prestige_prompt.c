#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE manager[64],dialog[512],owner[512];
static void *manager_pointer=manager;
static int pushes,resets,ready=1;
static BOOL eligible=TRUE;
static BOOL eligibility_mock(void){return eligible;}
static int __cdecl action_mock(int action){if(action==0)return ready;if(action!=1)abort();resets++;return 1;}
static void __cdecl push_mock(void *self,int state){if(self!=manager||state!=9)abort();pushes++;((void **)manager)[11]=dialog;}
static void __cdecl pop_mock(void *self,int resume){if(self!=manager||resume!=TRUE)abort();((void **)manager)[11]=NULL;}
static void resolve(int selection){save_prompt_input(dialog,selection);save_prompt_tick(owner);}
int main(void){
    BYTE flags[39];memset(flags,1,sizeof(flags));CHECK(prestige_all_achievements(flags));
    for(unsigned i=0;i<39;i++){flags[i]=0;CHECK(prestige_all_achievements(flags)==(i==18));flags[i]=1;}
    flags[38]=2;CHECK(!prestige_all_achievements(flags));
    overlay_manager=&manager_pointer;overlay_push=push_mock;overlay_pop=pop_mock;prompt_installed=TRUE;
    page_owner=owner;progression_domain_active=TRUE;progression_load_observed=TRUE;
    prestige_eligibility=eligibility_mock;
    CHECK(!prestige_prompt_intercept(owner,3)); /* No registered mutator. */
    CHECK(!ReRiffConfigurePrestigeAction(NULL));CHECK(ReRiffConfigurePrestigeAction((void *)action_mock));
    for(int id=0;id<5;id++)if(id!=3)CHECK(!prestige_prompt_intercept(owner,id));
    CHECK(!prestige_prompt_intercept(owner,2000));
    eligible=FALSE;CHECK(!prestige_prompt_intercept(owner,3)&&!pushes&&!resets);eligible=TRUE;
    progression_domain_active=FALSE;CHECK(!prestige_prompt_intercept(owner,3));progression_domain_active=TRUE;
    server_page=TRUE;CHECK(!prestige_prompt_intercept(owner,3));server_page=FALSE;
    ((int *)owner)[62]=1;CHECK(!prestige_prompt_intercept(owner,3));((int *)owner)[62]=0;
    ready=0;CHECK(!prestige_prompt_intercept(owner,3));ready=1;
    ((int *)owner)[102]=1;CHECK(!prestige_prompt_intercept(owner,3));((int *)owner)[102]=0;
    progression_load_observed=FALSE;CHECK(!prestige_prompt_intercept(owner,3));progression_load_observed=TRUE;
    CHECK(prestige_prompt_intercept(owner,3)&&pushes==1&&!resets);
    CHECK(!wcscmp(save_prompt_title,L"PRESTIGE — ARE YOU SURE?"));
    resolve(1);CHECK(!resets); /* Native Cancel. */
    CHECK(prestige_prompt_intercept(owner,3));pop_mock(manager,TRUE);save_prompt_tick(owner);CHECK(!resets); /* Native Back. */
    CHECK(prestige_prompt_intercept(owner,3));eligible=FALSE;resolve(2);CHECK(!resets);eligible=TRUE;
    CHECK(prestige_prompt_intercept(owner,3));ready=0;resolve(2);CHECK(!resets);ready=1;
    CHECK(prestige_prompt_intercept(owner,3));server_page=TRUE;resolve(2);CHECK(!resets);server_page=FALSE;
    CHECK(prestige_prompt_intercept(owner,3));save_prompt_input(dialog,2);CHECK(!resets);save_prompt_tick(owner);CHECK(resets==1);
    save_prompt_tick(owner);CHECK(resets==1); /* A confirmation cannot replay. */
    prompt_installed=FALSE;CHECK(prestige_prompt_intercept(owner,3)&&resets==1&&!save_prompt_owner);
    prompt_installed=TRUE;prestige_action=NULL;
    CHECK(save_prompt_begin(owner,prestige_prompt_result));CHECK(!wcscmp(save_prompt_title,L"SAVE SETTINGS"));
    puts("PASS: manual Credits-only confirmation, SP isolation, complete eligibility, cancellation, deferred one-shot action and revalidation; original save caption retained");
    return 0;
}
