/* Extend original menu entry, not the save's shared positive-delta policy.
 * Results owns the original base/bonus fields before its payment and later
 * builds its display from them. Creator reads one scoped native tuneable for
 * both initScreens and payment. The global base value is restored on return. */
typedef void (__thiscall *RewardEnterFn)(void *,int);
typedef int (__cdecl *GenerationQuoteFn)(int);
typedef void (__thiscall *RewardInitFn)(void *,void *,void *);
static RewardInitFn reward_creator_init_original;
static void *reward_creator_menu;
static int reward_creator_quote;
static BOOL reward_creator_quoted;
static RewardEnterFn reward_race_original,reward_creator_original;
static GenerationQuoteFn reward_generation_quote;
static GenerationQuoteFn reward_ghost_quote;
static RewardInitFn reward_versus_init_original;
static int *reward_ghost_base;
static void **reward_tuneables;
static void *reward_race_context;
static DWORD reward_context_thread;
static BOOL reward_hooks_installed;
__declspec(dllexport) int __cdecl ReRiffConfigureProgressionRewards(void *quote){
    if(!quote||reward_hooks_installed)return 0;
    reward_generation_quote=(GenerationQuoteFn)quote;return 1;
}
__declspec(dllexport) int __cdecl ReRiffConfigureProgressionGhostQuote(void *quote){
    if(!quote||reward_hooks_installed)return 0;
    reward_ghost_quote=(GenerationQuoteFn)quote;return 1;
}
static void __thiscall reward_versus_init(void *menu,void *screen,void *overlay){
    /* Original initScreens formats RESULTS_VSBeatOpponent from this mutable
     * RVA field (not Literal/InitOnly). Payment independently embeds 100 in
     * ResultsTokens.enter; only the native localized message is quoted here. */
    DWORD protection=0,ignored;int original=0,quote=0;BOOL changed=FALSE;
    if(reward_ghost_quote&&progression_domain_active&&progression_load_observed
        &&readable(reward_ghost_base,4)&&*reward_ghost_base==100){
        original=*reward_ghost_base;quote=reward_ghost_quote(original);
        if(quote>=0&&quote!=original&&VirtualProtect(reward_ghost_base,4,PAGE_READWRITE,&protection)){
            *reward_ghost_base=quote;changed=TRUE;
        }
    }
    reward_versus_init_original(menu,screen,overlay);
    if(changed){
        *reward_ghost_base=original;
        VirtualProtect(reward_ghost_base,4,protection,&ignored);
    }
}
__declspec(dllexport) void *__cdecl ReRiffGetProgressionRaceContext(void){
    return reward_context_thread==GetCurrentThreadId()?reward_race_context:NULL;
}
static void __thiscall reward_race_enter(void *menu,int state){
    /* Some completed races bypass versus entry. The matched helper enforces
     * actual singleplayer mode, so this also supports non-isolated saves. */
    progression_ghost_after_race();
    scrobble_completed();
    race_camera_disable();race_photo_disable();
    void *previous=reward_race_context;DWORD previous_thread=reward_context_thread;
    if(progression_domain_active&&progression_load_observed&&readable(menu,400)){
        /* XP/results preparation has completed. Recheck earned main-level
         * Ranks before entering the coin-payment scope so native achievement
         * awards are not reclassified as multiplied race coins. */
        progression_rank_after_race();
        reward_race_context=menu;reward_context_thread=GetCurrentThreadId();
    }
    reward_race_original(menu,state);
    reward_race_context=previous;reward_context_thread=previous_thread;
}
static void __thiscall reward_creator_init(void *menu,void *screen,void *overlay){
    int *amount=NULL,original=0;
    if(reward_creator_quoted&&menu==reward_creator_menu&&progression_domain_active&&progression_load_observed
        &&readable(reward_tuneables,4)&&readable(*reward_tuneables,24)){
        amount=(int *)((BYTE *)*reward_tuneables+20);original=*amount;*amount=reward_creator_quote;
    }
    reward_creator_init_original(menu,screen,overlay);
    if(amount&&*amount==reward_creator_quote)*amount=original;
}
static void __thiscall reward_creator_enter(void *menu,int state){
    int *amount=NULL,original=0,quote=0;reward_creator_quoted=FALSE;reward_creator_menu=menu;
    if(reward_generation_quote&&progression_domain_active&&progression_load_observed
        &&readable(reward_tuneables,4)&&readable(*reward_tuneables,24)){
        amount=(int *)((BYTE *)*reward_tuneables+20);original=*amount;
        quote=reward_generation_quote(original);
        if(original>=0&&quote>=0){*amount=quote;reward_creator_quote=quote;reward_creator_quoted=TRUE;}else amount=NULL;
    }
    reward_creator_original(menu,state);
    /* Do not overwrite a genuinely new native configuration update. */
    if(amount&&*amount==quote)*amount=original;
}
static BOOL reward_entry_guard(BYTE *base,DWORD table,unsigned index,DWORD thunk,DWORD fixup){
    BYTE *entry=base+thunk;void **slot=(void **)(base+table)+index;
    return readable(slot,4)&&*slot==entry&&readable(entry,6)&&entry[0]==0xff&&entry[1]==0x25
        &&*(DWORD *)(entry+2)==(DWORD)(base+fixup);
}
static BOOL install_progression_rewards(BYTE *base){
    if(!reward_generation_quote&&!scrobble_observer)return TRUE;if(reward_hooks_installed)return TRUE;
    static const BYTE tuneable_read[]={0x7e,0x13,0x10,0,0x04,0x1f,0x14,0x58,0x4a};
    static const BYTE ghost_read[]={0x7e,0x47,0x07,0,0x04};
    if(!reward_entry_guard(base,0x7a3f10,2,0x22ecec,0x7a3f90)
        ||!reward_entry_guard(base,0x7a7b74,2,0x368374,0x7a7be0)
        ||!reward_entry_guard(base,0x7a7b74,15,0x368970,0x7a7be4)
        ||!reward_entry_guard(base,0x7a4048,15,0x2333dc,0x7a40c0)
        ||!readable(base+0x233223,5)||memcmp(base+0x233223,ghost_read,5)
        ||!readable(base+0x5301d0,4)||*(int *)(base+0x5301d0)!=100
        ||!readable(base+0x368362,9)||memcmp(base+0x368362,tuneable_read,9)
        ||!readable(base+0x3685a0,9)||memcmp(base+0x3685a0,tuneable_read,9)
        ||!readable(base+0x7eb8f0,4))return FALSE;
    void **slots[]={(void **)(base+0x7a3f10)+2,(void **)(base+0x7a7b74)+2,(void **)(base+0x7a7b74)+15,(void **)(base+0x7a4048)+15};
    void *originals[]={*slots[0],*slots[1],*slots[2],*slots[3]};
    void *hooks[]={(void *)reward_race_enter,(void *)reward_creator_enter,(void *)reward_creator_init,(void *)reward_versus_init};
    reward_race_original=(RewardEnterFn)originals[0];reward_creator_original=(RewardEnterFn)originals[1];reward_creator_init_original=(RewardInitFn)originals[2];
    reward_versus_init_original=(RewardInitFn)originals[3];
    for(unsigned i=0;i<4;i++)if(!progression_domain_slot(slots[i],originals[i],hooks[i])){
        for(unsigned j=0;j<=i;j++)if(*slots[j]==hooks[j])progression_domain_slot(slots[j],hooks[j],originals[j]);return FALSE;
    }
    reward_tuneables=(void **)(base+0x7eb8f0);reward_ghost_base=(int *)(base+0x5301d0);reward_hooks_installed=TRUE;
    log_line("Scoped native race and creator reward entry hooks installed; original menu lifecycle retained.");return TRUE;
}
