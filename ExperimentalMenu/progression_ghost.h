/* Original versus entry and both native achievements remain unchanged.
 * The matched helper may add Friend or Foe for a singleplayer normal-ghost win. */
typedef void (__thiscall *GhostVersusFn)(void *,int);
typedef void (__cdecl *GhostAchievementFn)(void);
static GhostVersusFn ghost_versus_original;
static GhostAchievementFn ghost_achievement_callback;
static BOOL ghost_achievement_validated,ghost_achievement_installed;
static void __cdecl ghost_achievement_dispatch(void){if(ghost_achievement_callback)ghost_achievement_callback();}
__declspec(dllexport) void *__cdecl ReRiffGetGhostAchievementThunk(void){return (void *)ghost_achievement_dispatch;}
__declspec(dllexport) int __cdecl ReRiffConfigureGhostAchievement(void *callback,int validated){
    if(ghost_achievement_installed||!callback||(validated!=0&&validated!=1))return 0;
    if(validated){if(ghost_achievement_callback!=(GhostAchievementFn)callback)return 0;ghost_achievement_validated=TRUE;}
    else{ghost_achievement_callback=(GhostAchievementFn)callback;ghost_achievement_validated=FALSE;}
    return 1;
}
static void __thiscall ghost_versus_enter(void *menu,int state){
    ghost_versus_original(menu,state);
    if(ghost_achievement_installed&&ghost_achievement_validated){log_line("Ghost achievement checkpoint: versus results.");ghost_achievement_dispatch();}
}
static const BYTE ghost_original_awards[]={0x11,0x22,0x28,0xd4,0x05,0x00,0x06,0x28,0xac,0x07,0x00,0x06,0x13,0x0a,0x7e,0xb6,0x10,0x00,0x04,0x20,0x30,0x01,0x00,0x00,0x58,0x4a,0x13,0x09,0x11,0x09,0x17,0x33,0x25,0x7e,0xb6,0x10,0x00,0x04,0x20,0x34,0x01,0x00,0x00,0x58,0x4e,0x13,0x17,0x11,0x0a,0x6c,0x11,0x17,0x6c,0x36,0x37,0x7e,0xb1,0x0d,0x00,0x04,0x1f,0x20,0x17,0x28,0x80,0x0e,0x00,0x06,0x2b,0x28,0x11,0x09,0x18,0x33,0x23,0x7e,0xb6,0x10,0x00,0x04,0x20,0x34,0x01,0x00,0x00,0x58,0x4e,0x13,0x16,0x11,0x0a,0x6c,0x11,0x16,0x6c,0x36,0x0d,0x7e,0xb1,0x0d,0x00,0x04,0x1f,0x13,0x17,0x28,0x80,0x0e,0x00,0x06,0x2a};
static BOOL install_progression_ghost(BYTE *base){
    if(!ghost_achievement_validated||ghost_achievement_installed)return TRUE;
    void **slot=(void **)(base+0x7a4048)+2;BYTE *entry=base+0x2347e0;
    if(!readable(slot,4)||*slot!=entry||!readable(entry,6)||entry[0]!=0xff||entry[1]!=0x25
        ||*(DWORD *)(entry+2)!=(DWORD)(base+0x7a40c8)
        ||!readable(base+0x2346c2,sizeof(ghost_original_awards))||memcmp(base+0x2346c2,ghost_original_awards,sizeof(ghost_original_awards)))return FALSE;
    ghost_versus_original=(GhostVersusFn)*slot;
    if(!progression_domain_slot(slot,(void *)ghost_versus_original,(void *)ghost_versus_enter))return FALSE;
    ghost_achievement_installed=TRUE;log_line("Original versus results extended for singleplayer Friend or Foe; original scoring/awards unchanged.");return TRUE;
}

static void progression_ghost_after_race(void){
    if(ghost_achievement_installed&&ghost_achievement_validated){log_line("Ghost achievement checkpoint: completed race results.");ghost_achievement_dispatch();}
}
