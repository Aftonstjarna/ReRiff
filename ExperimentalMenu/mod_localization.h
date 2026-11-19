/* Experimental late-mount localization refresh. Original functions and layout:
 * elStringManager.load RVA 24ef0, unload 25370, singleton 7fcb34.
 * load skips an identical language/bank mask; unload clears the cached mask.
 * Only before save allocation, on the synchronous startup/mount thread.
 * See reports/CAR-CAPACITY-AND-SAVE-TRACE.md. */
typedef void (__thiscall *ModsStringLoad)(void *,unsigned,int);
typedef void (__thiscall *ModsStringUnload)(void *,unsigned);
static BOOL mods_refresh_strings(void *manager,void *save,ModsStringUnload unload,ModsStringLoad load){
    if(save||!readable(manager,16)||!unload||!load)return FALSE;
    int language=*(int *)manager;unsigned banks=*(unsigned *)((BYTE *)manager+12);
    /* The pinned game's three banks and explicit language values only. */
    if(language<0||language>=20||!banks||(banks&~7u))return FALSE;
    unload(manager,banks);
    load(manager,banks,language);
    return TRUE;
}
static void mods_refresh_localization(BYTE *base){
    if(!GetPrivateProfileIntW(L"diagnostics",L"mod_localization_refresh",0,config_path))return;
    const BYTE load_guard[]={0x83,0xff,0x14,0x75,0x1a,0x8b,0x3b};
    const BYTE unload_guard[]={0x8b,0x45,0x08,0xf7,0xd0,0x21,0x47,0x0c};
    if(memcmp(base+0x24f2f,load_guard,sizeof(load_guard))||memcmp(base+0x2543c,unload_guard,sizeof(unload_guard))){
        log_line("Mod localization refresh refused: original loader bytes differ.");return;
    }
    void *manager=*(void **)(base+0x7fcb34),*save=*(void **)(base+0x7ed7b8);
    if(mods_refresh_strings(manager,save,(ModsStringUnload)(base+0x25370),(ModsStringLoad)(base+0x24ef0)))
        log_line("Mod localization: native cached banks refreshed after late mount; language and bank mask preserved.");
    else log_line("Mod localization refresh skipped: no loaded banks or beyond early startup.");
}
