/* Display-only replacement of the level-reward argument at the original
 * RESULTS_XPLevelledUp formatter. Reward getter/table/payment stay native. */
static GenerationQuoteFn reward_level_quote;
static BOOL reward_level_installed;
static int __cdecl reward_level_dispatch(int level){return reward_level_quote?reward_level_quote(level):0;}
__declspec(dllexport) void *__cdecl ReRiffGetProgressionLevelQuoteThunk(void){return (void *)reward_level_dispatch;}
__declspec(dllexport) int __cdecl ReRiffConfigureProgressionLevelQuote(void *quote){
    if(!quote||reward_level_installed)return 0;reward_level_quote=(GenerationQuoteFn)quote;return 1;
}
static const BYTE reward_level_header[]={0x03,0x30,0x06,0x00,0xc1,0x02,0x00,0x00,0x84,0x07,0x00,0x11};
static const BYTE reward_level_original[]={0x11,0x11,0x7e,0x96,0x07,0x00,0x04,0x28,0x34,0x10,0x00,0x06};
static BOOL validate_progression_level_quote(BYTE *base){
    BYTE *method=base+0x235758,*argument=method+12+0x1e9;
    if(!readable(method,12+0x2c1)||memcmp(method,reward_level_header,12)
        ||memcmp(argument,reward_level_original,12)
        ||memcmp(argument-5,"\x7e\x96\x07\x00\x04",5)
        ||memcmp(argument+12,"\x28\x18\x00\x00\x0a",5))return FALSE;
    return TRUE;
}
static BOOL install_progression_level_quote(BYTE *base){
    if(!reward_level_quote||reward_level_installed)return TRUE;
    if(!validate_progression_level_quote(base))return FALSE;
    BYTE *argument=base+0x235758+12+0x1e9;
    BYTE replacement[12]={0x25,0x20,0,0,0,0,0xd3,0x29,0x54,0x08,0,0x11};
    void *target=(void *)reward_level_dispatch;memcpy(replacement+2,&target,4);
    DWORD previous,ignored;if(!VirtualProtect(argument,12,PAGE_EXECUTE_READWRITE,&previous))return FALSE;
    memcpy(argument,replacement,12);FlushInstructionCache(GetCurrentProcess(),argument,12);
    VirtualProtect(argument,12,previous,&ignored);reward_level_installed=TRUE;
    log_line("Native level-up message reward quote installed; original reward table and coin payment unchanged.");return TRUE;
}
