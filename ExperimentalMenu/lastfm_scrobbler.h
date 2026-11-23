/* Optional observation of the original race lifecycle. No audio/input changes.
 * Registration follows the managed whole-file hash guard; installation also
 * checks the original vtable/thunks. See https://github.com/Aftonstjarna/ReRiff/wiki/Research-LASTFM-PLAYBACK. */
typedef void (__cdecl *ScrobbleObserver)(int,int);
static ScrobbleObserver scrobble_observer;
static TickFn scrobble_tick_original;
typedef void (__thiscall *ScrobbleEnter)(void *,int);
static ScrobbleEnter scrobble_enter_original;
static DWORD scrobble_last_tick;
__declspec(dllexport) int __cdecl ReRiffConfigureScrobbler(ScrobbleObserver callback){
    if(scrobble_observer||!callback)return 0;scrobble_observer=callback;return 1;
}
static void scrobble_completed(void){if(scrobble_observer)scrobble_observer(2,0);}
static void __thiscall scrobble_tick(void *self){
    scrobble_tick_original(self);DWORD now=GetTickCount();
    if((DWORD)(now-scrobble_last_tick)>=250){scrobble_last_tick=now;scrobble_observer(0,0);}
}
static void __thiscall scrobble_enter(void *self,int previous){scrobble_enter_original(self,previous);scrobble_observer(1,previous);}
static BOOL install_scrobbler(BYTE *base){
    if(!scrobble_observer)return TRUE;
    void **table=(void **)(base+0x7a399c);BYTE *enter=base+0x21ef04,*tick=base+0x21f01f;
    if(!readable(table,16)||table[2]!=enter||table[3]!=tick
        ||!readable(enter,6)||enter[0]!=0xff||enter[1]!=0x25||*(DWORD *)(enter+2)!=(DWORD)(base+0x7a39f8)
        ||!readable(tick,6)||tick[0]!=0xff||tick[1]!=0x25||*(DWORD *)(tick+2)!=(DWORD)(base+0x7a39fc))return FALSE;
    DWORD old,ignored;if(!VirtualProtect(table+2,8,PAGE_READWRITE,&old))return FALSE;
    scrobble_enter_original=(ScrobbleEnter)table[2];scrobble_tick_original=(TickFn)table[3];
    InterlockedExchangePointer(table+2,(void *)scrobble_enter);InterlockedExchangePointer(table+3,(void *)scrobble_tick);
    VirtualProtect(table+2,8,old,&ignored);return TRUE;
}
