/* R3 random-song roll for the native file picker.
 *
 * The picker's own reel and focus IDs are used: picker_alpha_context already
 * validates the active row list, phase and current focus. A roll moves the
 * reel to a random row and updates the native focus item, so A still accepts
 * the selected song normally. */
#define PICKER_RANDOM_R3 0x0080
static void (__cdecl *picker_random_focus)(void *,int);
static void *picker_random_owner;
static BOOL picker_random_previous;
static DWORD picker_random_rng;
static unsigned picker_random_events;
static BOOL picker_random_pending;
static DWORD picker_random_pending_at;
static int picker_random_pending_id;

static DWORD picker_random_next(DWORD *state){
    DWORD value=*state?*state:0x9e3779b9u;
    value^=value<<13;value^=value>>17;value^=value<<5;*state=value;return value;
}

static BOOL picker_random_is_file(const BYTE *entry){
    int type=0;return picker_alpha_read(entry+24,&type,4)&&type==4;
}

static unsigned picker_random_file_index(const PickerAlphaContext *context,unsigned current,DWORD *state){
    unsigned files=0;
    for(unsigned i=0;i<context->count;i++)if(picker_random_is_file(context->entries+28*i))files++;
    if(!files)return (unsigned)-1;
    unsigned target=picker_random_next(state)%files,seen=0,selected=(unsigned)-1;
    for(unsigned i=0;i<context->count;i++){
        if(!picker_random_is_file(context->entries+28*i))continue;
        if(seen++==target){selected=i;break;}
    }
    if(selected==current&&files>1){
        for(unsigned i=0;i<context->count;i++)if(i!=current&&picker_random_is_file(context->entries+28*i)){selected=i;break;}
    }
    return selected;
}

static BOOL picker_random_name(const BYTE *entry,WCHAR *out,int size){
    DWORD length,capacity;const WCHAR *text;
    if(!picker_alpha_read(entry+16,&length,4)||!picker_alpha_read(entry+20,&capacity,4)
        ||!length||length>32767||capacity<length)return FALSE;
    if(capacity<8)text=(const WCHAR *)entry;
    else if(!picker_alpha_read(entry,&text,4))return FALSE;
    if(length>=(DWORD)size)length=(DWORD)size-1;
    if(!picker_alpha_read(text,out,(SIZE_T)length*2))return FALSE;
    out[length]=0;return TRUE;
}

static BOOL picker_random_r3(void){
    if(!readable(input_manager,4)||!readable(*input_manager,4))return FALSE;
    void *manager=*input_manager;void **table=*(void ***)manager;if(!readable(table,8))return FALSE;
    void *device=((void *(__thiscall *)(void *,unsigned))table[1])(manager,0);
    if(!readable(device,4)||!readable(*(void ***)device,80))return FALSE;
    void *controller=((void *(__thiscall *)(void *))(*(void ***)device)[19])(device);
    if(!readable(controller,4)||!readable(*(void ***)controller,16))return FALSE;
    typedef BYTE (__thiscall *Pressed)(void *,DWORD);Pressed pressed=(Pressed)(*(void ***)controller)[3];
    return pressed(controller,PICKER_RANDOM_R3)!=0;
}

static void picker_random_select(void *menu){
    if(!picker_random_focus||!reel_move)return;
    PickerAlphaContext context;
    if(!picker_alpha_context(menu,&context)||context.phase!=1||!context.count)return;
    unsigned current=context.focus>=4&&(unsigned)(context.focus-4)<context.count?(unsigned)(context.focus-4):(unsigned)-1;
    unsigned index=picker_random_file_index(&context,current,&picker_random_rng);
    if(index==(unsigned)-1){if(picker_random_events++<64)log_line("Picker random roll: no files in the current list.");return;}
    float instant=0;
    reel_move(context.reel,(int)index,&instant);
    picker_random_focus(menu,4+(int)index);
    picker_random_pending=TRUE;picker_random_pending_id=4+(int)index;picker_random_pending_at=GetTickCount();
    if(picker_random_events++<64){
        WCHAR name[96]={0};picker_random_name(context.entries+28*index,name,96);
        char line[240];snprintf(line,sizeof(line),"Picker random roll: rows=%u selected=%u previous=%d name=%ls; autostart pending.",context.count,index,context.focus,name[0]?name:L"(unavailable)");log_line(line);
    }
}

static InputFn picker_random_input(void){
    if(picker_search_original_input)return picker_search_original_input;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void **table=(void **)(base+0x7a54bc);
    return readable(table,25*4)?(InputFn)table[24]:NULL;
}

static void picker_random_forget(void *menu){
    if(picker_random_owner==menu){picker_random_owner=NULL;picker_random_previous=FALSE;picker_random_pending=FALSE;}
}

static void picker_random_tick(void *menu){
    if(!readable(menu,336)||*(void ***)menu!=(void **)(((BYTE *)GetModuleHandleW(NULL))+0x7a54bc))return;
    if(picker_random_owner!=menu){
        picker_random_owner=menu;picker_random_previous=FALSE;
        picker_random_rng=GetTickCount()^(DWORD)GetCurrentProcessId()^(DWORD)(uintptr_t)menu;
    }
    HWND window=display_window();
    BOOL foreground=IsWindow(window)&&!IsIconic(window)&&GetForegroundWindow()==window;
    if(!foreground){picker_random_previous=FALSE;picker_random_pending=FALSE;return;}
    if(picker_random_pending&&(DWORD)(GetTickCount()-picker_random_pending_at)>=180){
        InputFn input=picker_random_input();
        PickerAlphaContext context;
        if(input&&picker_alpha_context(menu,&context)&&context.phase==1){
            int id=picker_random_pending_id;picker_random_pending=FALSE;input(menu,id);
            if(picker_random_events++<64)log_line("Picker random roll: autostart dispatched.");
        }else picker_random_pending=FALSE;
    }
    BOOL pressed=picker_random_r3();
    if(pressed&&!picker_random_previous)picker_random_select(menu);
    picker_random_previous=pressed;
}

__declspec(dllexport) int __cdecl ReRiffConfigurePickerRandom(void *focus){
    if(!focus)return 0;picker_random_focus=(void (__cdecl *)(void *,int))focus;return 1;
}
