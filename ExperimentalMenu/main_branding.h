/* Borrowed native screen child: original menu cleanup owns its lifetime. */
static ScreenFn main_init_original,main_cleanup_original;
static TickFn main_tick_original;
static void *watermark,*watermark_owner;
typedef float *(__cdecl *ResolutionFn)(float *,int);
static ResolutionFn native_resolution;
static BOOL (*queue_update_notice)(const WCHAR *)=ReRiffShowUpdateToastW;
static WCHAR update_notice_path[MAX_PATH];
static void position_watermark(void *menu){
    if(!watermark||watermark_owner!=menu)return;
    float resolution[2];native_resolution(resolution,0);
    float sx=((float *)menu)[14],sy=((float *)menu)[15];
    float height=((float *)watermark)[13],half_gap=((float *)menu)[73];
    if(sx<=0||sy<=0||height<=0||half_gap<=0||resolution[0]<=0||resolution[1]<=0)return;
    /* Original init stores half its row interval at +292. Each interval is
     * 1.5 times the chosen menu font height; the text itself is unscaled. */
    float target_height=half_gap*(2.0f/3.0f),scale=target_height/height;
    scale_item(watermark,scale,scale);
    move_item(watermark,resolution[0]-24*sx,resolution[1]-24*sy-target_height);
    if(*(DWORD *)((BYTE *)watermark+76))enable_input(watermark,FALSE);
}
static void deliver_update_notice(void){
    if(GetFileAttributesW(update_notice_path)==INVALID_FILE_ATTRIBUTES)return;
    WCHAR version[64];GetPrivateProfileStringW(L"update",L"version",L"",version,64,update_notice_path);
    if(!*version||wcscmp(version,MENU_WIDE(RERIFF_VERSION)))return;
    WCHAR text[128];_snwprintf(text,128,L"ReRiff has updated to %ls",version);text[127]=0;
    if(queue_update_notice(text))DeleteFileW(update_notice_path);
}
static void attach_main_branding(void *menu){
    watermark_owner=menu;watermark=create_text(L"ReRiff v" MENU_WIDE(RERIFF_VERSION),0,0,3,2,0,-1,FALSE,-1,FALSE,TRUE);
    if(watermark){
        ((int *)watermark)[2]=-1;((int *)watermark)[15]=100;
        menu_item(watermark,FALSE);enable_input(watermark,FALSE);
        static const float colour[4]={1,1,1,0.55f};colour_item(watermark,colour);
        ((int *)watermark)[1]++;add_screen(((void **)menu)[46],watermark);
        position_watermark(menu);
    }
    deliver_update_notice();
}
static void __thiscall branded_main_init(void *menu,void *screen,void *lower){
    display_restore_startup();
    main_init_original(menu,screen,lower);attach_main_branding(menu);
}
static void __thiscall branded_main_tick(void *menu){
    display_restore_startup();display_style();
    /* Transport initialization can happen after the first main-menu init. */
    if(!watermark||watermark_owner!=menu)attach_main_branding(menu);
    position_watermark(menu);
    if(InterlockedCompareExchange(&aa_requested,0,0)&&!aa_attach())aa_fail("Antialiasing unavailable: graphics-device layout differs.");
    main_tick_original(menu);
}
static void __thiscall branded_main_cleanup(void *menu,void *screen,void *lower){
    if(watermark&&watermark_owner==menu){
        ((int *)watermark)[1]++;remove_screen(((void **)menu)[46],watermark);watermark=NULL;watermark_owner=NULL;
    }
    main_cleanup_original(menu,screen,lower);
}
static BOOL validate_main_branding(BYTE *base,uintptr_t address){
    const DWORD slots[3]={3,15,16},entries[3]={0x340a54,0x33fd14,0x33f23c},fixups[3]={0x7a74f8,0x7a74f0,0x7a74ec};
    if(!readable(base+0x7a7470,27*sizeof(void *)))return FALSE;
    for(int i=0;i<3;i++){
        BYTE *thunk=base+entries[i];
        if(*(DWORD *)(base+0x7a7470+4*slots[i])!=(DWORD)(address+entries[i])||!readable(thunk,6)||thunk[0]!=0xff||thunk[1]!=0x25||*(DWORD *)(thunk+2)!=(DWORD)(address+fixups[i]))return FALSE;
    }
    static const BYTE resolution_start[]={0x55,0x8b,0xec,0x83,0xec,0x10,0x83,0x7d,0x0c,0};
    return readable(base+0x1fa50,sizeof(resolution_start))&&!memcmp(base+0x1fa50,resolution_start,sizeof(resolution_start));
}
static BOOL install_main_branding(BYTE *base){
    void **table=(void **)(base+0x7a7470);DWORD old;
    if(!VirtualProtect(table,27*sizeof(void *),PAGE_READWRITE,&old))return FALSE;
    main_init_original=(ScreenFn)table[15];main_cleanup_original=(ScreenFn)table[16];main_tick_original=(TickFn)table[3];
    native_resolution=(ResolutionFn)(base+0x1fa50);
    WCHAR *slash=wcsrchr(config_path,L'\\');if(slash&&(size_t)(slash-config_path)+48<MAX_PATH){
        wcsncpy(update_notice_path,config_path,(size_t)(slash-config_path)+1);update_notice_path[slash-config_path+1]=0;
        wcscat(update_notice_path,L"localserver\\update-notification.ini");
    }
    InterlockedExchangePointer(&table[15],(void *)branded_main_init);InterlockedExchangePointer(&table[16],(void *)branded_main_cleanup);InterlockedExchangePointer(&table[3],(void *)branded_main_tick);
    DWORD ignored;VirtualProtect(table,27*sizeof(void *),old,&ignored);return TRUE;
}
