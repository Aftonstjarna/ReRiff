static BOOL text_entry_busy(void);
static void text_entry_caption(void *dialog);
static BOOL text_entry_overlay_tick(void *dialog);
static BOOL text_entry_confirm(int choice);
/* Reuse gaQuitConfirmMenu's original yes/no overlay, input and lifecycle.
 * The original quit action is never called while this save prompt owns it. */
typedef void (__cdecl *SaveOverlayFn)(void *,int);
typedef void (__thiscall *PromptEnterFn)(void *,int);
typedef void (*SavePromptResult)(void *,int);
static SaveOverlayFn overlay_push,overlay_pop;
static void **overlay_manager;
static BOOL prompt_installed;
static BOOL menu_overlay_active(void){
    return readable(overlay_manager,4)&&readable(*overlay_manager,48)&&((void **)*overlay_manager)[11]!=NULL;
}
static void *save_prompt_owner;
static SavePromptResult save_prompt_result;
static int save_prompt_choice=-1;
static WCHAR save_prompt_title[96],save_prompt_message[512];
static ScreenFn prompt_init_original;
static PromptEnterFn prompt_enter_original;
static InputFn prompt_input_original;
static TickFn prompt_tick_original;
static void __thiscall save_prompt_overlay_tick(void *dialog){if(text_entry_overlay_tick(dialog))return;prompt_tick_original(dialog);text_entry_caption(dialog);}
static void save_prompt_caption(void *dialog){
    if(!save_prompt_owner)return;
    void *title=((void **)dialog)[65],*message=((void **)dialog)[66];
    if(readable(title,484))raw_text(title,save_prompt_title);
    if(readable(message,484))raw_text(message,save_prompt_message);
    text_entry_caption(dialog);
}
static void __thiscall save_prompt_init(void *dialog,void *screen,void *lower){prompt_init_original(dialog,screen,lower);save_prompt_caption(dialog);}
static void __thiscall save_prompt_enter(void *dialog,int previous){prompt_enter_original(dialog,previous);save_prompt_caption(dialog);}
static void __thiscall save_prompt_input(void *dialog,int selection){
    if(!save_prompt_owner){prompt_input_original(dialog,selection);return;}
    if(selection!=1&&selection!=2)return;
    if(!text_entry_confirm(selection==2?1:0))return;
    save_prompt_choice=selection==2?1:0;
    overlay_pop(*overlay_manager,TRUE);
}
static BOOL save_prompt_begin_caption(void *owner,SavePromptResult result,const WCHAR *title,const WCHAR *message){
    if(!owner||!result||!title||!message||wcslen(title)>=96||wcslen(message)>=512)return FALSE;
    if(save_prompt_owner||!prompt_installed||!overlay_push||!overlay_pop||!readable(overlay_manager,4)||!readable(*overlay_manager,48))return FALSE;
    if(((void **)*overlay_manager)[11])return FALSE;
    wcscpy(save_prompt_title,title);wcscpy(save_prompt_message,message);
    save_prompt_owner=owner;save_prompt_result=result;save_prompt_choice=-1;
    overlay_push(*overlay_manager,9);log_line("Native confirmation overlay opened.");return TRUE;
}
static BOOL save_prompt_begin(void *owner,SavePromptResult result){
    return save_prompt_begin_caption(owner,result,L"SAVE SETTINGS",L"Apply your changes before leaving?\nConfirm to save, or cancel to discard.\nPress Back to keep editing.");
}
static void save_prompt_tick(void *owner){
    if(save_prompt_owner!=owner||!readable(overlay_manager,4)||!readable(*overlay_manager,48)||((void **)*overlay_manager)[11])return;
    SavePromptResult result=save_prompt_result;int choice=save_prompt_choice;
    save_prompt_owner=NULL;save_prompt_result=NULL;save_prompt_choice=-1;
    if(result)result(owner,choice);
}
static BOOL install_save_prompt(BYTE *base){
    void **table=(void **)(base+0x7a7900);const int slots[4]={2,3,15,24};
    const DWORD rvas[4]={0x361068,0x360bc1,0x361b88,0x360b60},fixups[4]={0x7a7978,0x7a7974,0x7a7980,0x7a796c};
    if(!readable(table,27*4))return FALSE;
    for(int i=0;i<4;i++){
        BYTE *code=base+rvas[i];if(table[slots[i]]!=code||!readable(code,6)||code[0]!=0xff||code[1]!=0x25)return FALSE;
        if(*(DWORD *)(code+2)!=(DWORD)(base+fixups[i]))return FALSE;
    }
    DWORD old,ignored;if(!VirtualProtect(table,27*4,PAGE_READWRITE,&old))return FALSE;
    prompt_tick_original=(TickFn)table[3];
    prompt_enter_original=(PromptEnterFn)table[2];prompt_init_original=(ScreenFn)table[15];prompt_input_original=(InputFn)table[24];
    InterlockedExchangePointer(table+3,(void *)save_prompt_overlay_tick);InterlockedExchangePointer(table+2,(void *)save_prompt_enter);InterlockedExchangePointer(table+15,(void *)save_prompt_init);InterlockedExchangePointer(table+24,(void *)save_prompt_input);
    VirtualProtect(table,27*4,old,&ignored);overlay_manager=(void **)(base+0x7eefe0);prompt_installed=TRUE;return TRUE;
}
