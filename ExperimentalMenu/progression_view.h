/* Original Profile vtable 040014ED, self/friend discriminator 040014FB.
 * Extend the original tick, text object and fill-bar setter. Never replace the
 * global getDriverProfileLevel helper used by cycle unlocks and achievements. */
typedef int (__cdecl *ProgressSummaryFn)(WCHAR *,int,float *,int);
typedef void (__cdecl *ProgressFillFn)(void *,float);
static ProgressSummaryFn progression_summary;
static ProgressFillFn progression_fill;
typedef int (__cdecl *ProgressLocalTestFn)(void);
static ProgressLocalTestFn progression_local_test;
static TickFn progression_profile_original;
static DWORD *progression_friend_length;
static void first_prestige_row_clear(void);
#include "progression_badge.h"
#include "progression_achievement_probe.h"
#include "first_prestige_row.h"
static BOOL progression_view_update(void *menu,int cycle_xp){
    if(!progression_summary||!progression_fill||!raw_text||!readable(menu,408)
        ||!readable(progression_friend_length,4)||*progression_friend_length
        ||((int *)menu)[101]!=0)return FALSE;
    void *number=*(void **)((BYTE *)menu+276);
    if(!readable(number,484))return FALSE;
    WCHAR text[64]={0};float factor=0;
    if(progression_summary(text,64,&factor,cycle_xp)!=1||!wmemchr(text,0,64)||!text[0]
        ||!(factor>=0&&factor<=1))return FALSE;
    if(!progression_badge_render(*(void **)((BYTE *)menu+384),number,text))raw_text(number,text);
    progression_fill(menu,factor);return TRUE;
}
static void __thiscall progression_profile_tick(void *menu){
    progression_badge_restore(menu);
    progression_profile_original(menu);
    int xp=0;
    if(progression_domain_active&&progression_load_observed&&progression_domain_native_xp(&xp)){
        achievement_probe_tick(menu);
        first_prestige_row_update(menu);
        if(progression_local_test&&readable(progression_friend_length,4)&&!*progression_friend_length&&progression_local_test()<0)
            progression_domain_stop("Authorized local purchase test failed; last committed checkpoint retained.");
        progression_view_update(menu,xp);
    }
}
__declspec(dllexport) BOOL __cdecl ReRiffConfigureProgressionView(void **callbacks,int count){
    if((count!=2&&count!=3)||!callbacks||!callbacks[0]||!callbacks[1]||(count==3&&!callbacks[2]))return FALSE;
    progression_summary=(ProgressSummaryFn)callbacks[0];progression_fill=(ProgressFillFn)callbacks[1];progression_local_test=count==3?(ProgressLocalTestFn)callbacks[2]:NULL;return TRUE;
}
static BOOL install_progression_view(BYTE *base){
    if(!progression_summary||!progression_fill)return FALSE;
    void **slot=(void **)(base+0x7a6f94)+3,**cleanup=(void **)(base+0x7a6f94)+16;
    BYTE *thunk=base+0x32c53c,*cleanup_thunk=base+0x328aa8;
    static const BYTE font_getter[]={0x8b,0x81,0xcc,0x01,0,0,0x8b,0x40,0x64,0xc3};
    if(!readable(slot,4)||*slot!=thunk||!readable(thunk,6)||thunk[0]!=0xff||thunk[1]!=0x25
        ||*(DWORD *)(thunk+2)!=(DWORD)(base+0x7a7004)
        ||!readable(cleanup,4)||*cleanup!=cleanup_thunk||!readable(cleanup_thunk,6)||cleanup_thunk[0]!=0xff||cleanup_thunk[1]!=0x25
        ||*(DWORD *)(cleanup_thunk+2)!=(DWORD)(base+0x7a7000)
        ||!readable(base+0x51110,sizeof(font_getter))||memcmp(base+0x51110,font_getter,sizeof(font_getter)))return FALSE;
    progression_profile_original=(TickFn)*slot;progression_cleanup_original=(ScreenFn)*cleanup;
    if(!progression_domain_slot(slot,thunk,(void *)progression_profile_tick))return FALSE;
    if(!progression_domain_slot(cleanup,cleanup_thunk,(void *)progression_profile_cleanup)){
        if(*cleanup==(void *)progression_profile_cleanup)progression_domain_slot(cleanup,(void *)progression_profile_cleanup,cleanup_thunk);
        progression_domain_slot(slot,(void *)progression_profile_tick,thunk);return FALSE;
    }
    progression_font=(ProgressFontFn)(base+0x51110);
    progression_friend_length=(DWORD *)(base+0x7a6f2c+16);
    log_line("Permanent-level Profile display hook installed; cycle XP helpers unchanged.");return TRUE;
}
