/* Manual Credits entry only; gaOptionsMenu.handleScreenInput selection 3.
 * Exact original provenance: https://github.com/Aftonstjarna/ReRiff/wiki/Research-PRESTIGE-NATIVE-BOUNDARIES.
 * Only a configured helper may register the confirmed native reset action.
 * Registration alone is insufficient: the native eligibility and callback
 * readiness checks must pass both before opening and after confirmation. */
typedef int (__cdecl *PrestigeActionFn)(int);
#define PRESTIGE_CREDITS_SELECTION 3
static PrestigeActionFn prestige_action;
__declspec(dllexport) int __cdecl ReRiffConfigurePrestigeAction(void *action){
    if(!action)return 0;prestige_action=(PrestigeActionFn)action;return 1;
}
static BOOL prestige_all_achievements(const BYTE *achievements){
    /* Original Profile.setupAchievementsList skips storage slot 18. */
    for(unsigned i=0;i<39;i++)if(i!=18&&achievements[i]!=1)return FALSE;
    return TRUE;
}
static BOOL prestige_prompt_context(void *menu){
    return progression_domain_active&&progression_load_observed
        &&page_owner==menu&&!server_page&&readable(menu,412)
        &&((int *)menu)[62]==0&&((int *)menu)[102]==0;
}
static BOOL prestige_prompt_eligible(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    /* Original Profile loop: ldloc.1; ldc.i4.s 18; beq IL_06D8. */
    if(!first_prestige_skip_valid(base))return FALSE;
    /* Original input switch case 3 targets body +0x100 -> state 44,
     * verified against gaCredits.{ctor}; do not infer IDs from row order. */
    static const BYTE credits_switch[]={0x45,0x05,0,0,0,0x5a,0,0,0,0x05,0,0,0,0x6e,0,0,0,0x46,0,0,0,0x82,0,0,0};
    static const BYTE credits_transition[]={0x02,0x1f,0x2c};
    if(!readable(base+0x2f0841,sizeof(credits_switch))||memcmp(base+0x2f0841,credits_switch,sizeof(credits_switch))
        ||!readable(base+0x2f08a0,3)||memcmp(base+0x2f08a0,credits_transition,3))return FALSE;
    void *save=NULL,*progression=NULL;BYTE flags[3],achievements[39];
    if(!progression_probe_read(base+0x7ed7b8,&save,sizeof(save))||!save
        ||!progression_probe_read((BYTE *)save+12,flags,sizeof(flags))
        ||!flags[0]||flags[1]||flags[2]
        ||!progression_probe_read((BYTE *)save+32,&progression,sizeof(progression))||!progression
        ||!progression_probe_read((BYTE *)progression+12,achievements,sizeof(achievements)))return FALSE;
    return prestige_all_achievements(achievements);
}
/* Kept separate so the control-flow tests use synthetic eligibility without
 * writing achievement flags in a real game or issuing Steam API calls. */
static BOOL (*prestige_eligibility)(void)=prestige_prompt_eligible;
static void prestige_prompt_result(void *menu,int choice){
    if(choice!=1)return;
    if(!prestige_prompt_context(menu)||!prestige_action||!prestige_eligibility()
        ||prestige_action(0)!=1){log_line("Prestige confirmation refused: eligibility or readiness changed.");return;}
    int result=prestige_action(1);
    if(result<0)progression_domain_stop("Prestige transaction failed; restart restores the last committed checkpoint.");
    if(result!=1)log_line("Prestige confirmation did not complete; see progression diagnostics.");
}
static BOOL prestige_prompt_intercept(void *menu,int selection){
    if(selection!=PRESTIGE_CREDITS_SELECTION)return FALSE;
    BOOL context=prestige_prompt_context(menu),eligible=context&&prestige_eligibility();
    int ready=eligible&&prestige_action?prestige_action(0):0;
    char status[160];snprintf(status,sizeof(status),"Prestige Credits request: context=%u callback=%u displayed_achievements_complete=%u ready=%d.",context,prestige_action!=NULL,eligible,ready);log_line(status);
    if(!context||!prestige_action||!eligible||ready!=1)return FALSE;
    if(!save_prompt_begin_caption(menu,prestige_prompt_result,L"PRESTIGE — ARE YOU SURE?",
        L"Reset coins, unlocks and in-game achievements?\nKeep your equipped car and paint, main level,\nrace records and Steam achievements.\nConfirm to prestige. Cancel or Back keeps progress."))
        log_line("Prestige confirmation unavailable; settings left open.");
    return TRUE;
}
