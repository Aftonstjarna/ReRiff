/* Reuse gaFileSelectInterface.jumpToSelection (06001053), the original A-Z
 * keyboard path. No wraparound, focus clearing, custom reel or fake input.
 * Live observation verified trigger values and the active picker phase. */
#define PICKER_ALPHA_NAVIGATION 1
#define PICKER_ALPHA_MAX_ENTRIES 100000
static int (__cdecl *picker_alpha_jump)(void *,int);
static void *picker_alpha_owner;
static BOOL picker_alpha_armed,picker_alpha_logged;
static unsigned picker_alpha_events;
static BOOL picker_alpha_read(const void *from,void *to,SIZE_T size){SIZE_T got=0;return from&&ReadProcessMemory(GetCurrentProcess(),from,to,size,&got)&&got==size;}
static unsigned picker_alpha_letter(const BYTE *entry){
    DWORD length,capacity;const WCHAR *text;WCHAR first;
    if(!picker_alpha_read(entry+16,&length,4)||!picker_alpha_read(entry+20,&capacity,4)
        ||!length||length>32767||capacity<length)return 0;
    if(capacity<8)text=(const WCHAR *)entry;
    else if(!picker_alpha_read(entry,&text,4))return 0;
    if(!picker_alpha_read(text,&first,2))return 0;
    if(first>=L'a'&&first<=L'z')first-=L'a'-L'A';
    return first>=L'A'&&first<=L'Z'?(unsigned)first:0;
}
static int picker_alpha_target(DWORD available,unsigned current,int direction){
    if(direction!=1&&direction!=-1)return 0;
    int start=current>='A'&&current<='Z'?(int)current+direction:(direction>0?'A':'Z');
    for(int letter=start;letter>='A'&&letter<='Z';letter+=direction)
        if(available&(1u<<(letter-'A')))return letter;
    return 0;
}
static int picker_alpha_edge(BOOL *armed,float left,float right){
    if(!(left>=0&&left<=1&&right>=0&&right<=1)){*armed=FALSE;return 0;}
    if(left<.2f&&right<.2f){*armed=TRUE;return 0;}
    if(left>=.55f&&right>=.55f){*armed=FALSE;return 0;}
    if(!*armed)return 0;
    int direction=left>=.55f?-1:right>=.55f?1:0;
    if(direction)*armed=FALSE;return direction;
}
static BOOL picker_alpha_triggers(float *left,float *right){
    if(!readable(input_manager,4)||!readable(*input_manager,4))return FALSE;
    void *manager=*input_manager;void **table=*(void ***)manager;
    if(!readable(table,8))return FALSE;
    void *device=((void *(__thiscall *)(void *,unsigned))table[1])(manager,0);
    if(!readable(device,4)||!readable(*(void ***)device,80))return FALSE;
    void *controller=((void *(__thiscall *)(void *))(*(void ***)device)[19])(device);
    if(!readable(controller,4)||!readable(*(void ***)controller,36))return FALSE;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    void *getter=(*(void ***)controller)[8];
    static const BYTE expected[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0xd9,0x44,0x81,0x38,0x5d,0xc2,0x04,0x00};
    if(getter!=base+0xf8180||!readable(getter,sizeof(expected))||memcmp(getter,expected,sizeof(expected)))return FALSE;
    typedef float (__thiscall *Trigger)(void *,unsigned);
    *left=((Trigger)getter)(controller,0);*right=((Trigger)getter)(controller,1);
    return *left>=0&&*left<=1&&*right>=0&&*right<=1;
}
typedef struct PickerAlphaContext {void *screen,*reel;BYTE *entries;unsigned count,current,phase;int focus;} PickerAlphaContext;
static BOOL picker_alpha_context(void *menu,PickerAlphaContext *out){
    uintptr_t names[3],rows[3];void *focus=NULL;DWORD phase;
    if(!picker_alpha_read((BYTE *)menu+276,names,sizeof(names))||!names[0]||names[1]<names[0]||names[2]<names[1]
        ||(names[1]-names[0])%28||(names[1]-names[0])/28>PICKER_ALPHA_MAX_ENTRIES)return FALSE;
    unsigned count=(unsigned)((names[1]-names[0])/28);
    if(!count||!readable((void *)names[0],count*28)
        ||!picker_alpha_read((BYTE *)menu+324,rows,sizeof(rows))||!rows[0]||rows[1]<rows[0]||rows[2]<rows[1]
        ||(rows[1]-rows[0])!=count*4||!readable((void *)rows[0],count*4)
        ||!picker_alpha_read((BYTE *)menu+184,&out->screen,4)||!readable(out->screen,24)
        ||!picker_alpha_read((BYTE *)menu+248,&out->reel,4)||!readable(out->reel,4)
        ||*(void ***)out->reel!=reel_vtable||!picker_alpha_read((BYTE *)menu+28,&phase,4)
        ||!picker_alpha_read((BYTE *)out->screen+20,&focus,4))return FALSE;
    out->entries=(BYTE *)names[0];out->count=count;out->phase=phase;out->focus=-1;out->current=0;
    if(focus&&!picker_alpha_read((BYTE *)focus+8,&out->focus,4))return FALSE;
    if(out->focus>=4&&(unsigned)(out->focus-4)<count)out->current=picker_alpha_letter(out->entries+28*(out->focus-4));
    return TRUE;
}
static DWORD picker_alpha_letters(const PickerAlphaContext *context){
    DWORD mask=0;for(unsigned i=0;i<context->count;i++){
        unsigned letter=picker_alpha_letter(context->entries+28*i);if(letter)mask|=1u<<(letter-'A');
    }return mask;
}
static int picker_alpha_dispatch(void *menu,const PickerAlphaContext *context,int target){
    /* Phase 1 and a live row focus were observed while native input was active.
     * Never move selection during entry/exit transitions or with no focus. */
    if(!PICKER_ALPHA_NAVIGATION||!picker_alpha_jump||context->phase!=1
        ||context->focus<0||context->focus>=(int)context->count+4
        ||target<'A'||target>'Z')return 0;
    return picker_alpha_jump(menu,target);
}
static void picker_alpha_forget(void *menu){if(picker_alpha_owner==menu){picker_alpha_owner=NULL;picker_alpha_armed=FALSE;picker_alpha_logged=FALSE;}}
static void picker_alpha_tick(void *menu){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!readable(menu,336)||*(void ***)menu!=(void **)(base+0x7a54bc)||!picker_alpha_jump)return;
    if(picker_alpha_owner!=menu){picker_alpha_owner=menu;picker_alpha_armed=FALSE;picker_alpha_logged=FALSE;}
    HWND window=display_window();
    if(!IsWindow(window)||IsIconic(window)||GetForegroundWindow()!=window){picker_alpha_armed=FALSE;return;}
    PickerAlphaContext context;
    if(!picker_alpha_context(menu,&context)){picker_alpha_armed=FALSE;return;}
    if(!picker_alpha_logged){
        char text[192];snprintf(text,sizeof(text),"Picker alphabet probe: rows=%u phase=%u focus=%d mode=%s; native keyboard jump bound.",context.count,context.phase,context.focus,PICKER_ALPHA_NAVIGATION?"navigate":"observe-only");log_line(text);picker_alpha_logged=TRUE;
        void *overlay=readable(overlay_manager,4)&&readable(*overlay_manager,48)?((void **)*overlay_manager)[11]:NULL;
        void **rows=*(void ***)((BYTE *)menu+324);void *first=readable(rows,4)?rows[0]:NULL;
        if(readable(first,404)){char layout[192];snprintf(layout,sizeof(layout),"Picker text-entry layout: overlay_active=%d overlay_is_menu=%d first_y=%.1f height=%.1f count=%u.",overlay!=NULL,overlay==menu,((float *)first)[9],((float *)first)[13],context.count);log_line(layout);}

    }
    float left=0,right=0;
    if(!picker_alpha_triggers(&left,&right)){picker_alpha_armed=FALSE;return;}
    int direction=picker_alpha_edge(&picker_alpha_armed,left,right);if(!direction)return;
    DWORD letters=picker_alpha_letters(&context);int target=picker_alpha_target(letters,context.current,direction);
    if(picker_alpha_events++<128){
        char text[224];snprintf(text,sizeof(text),"Picker alphabet trigger: LT=%.3f RT=%.3f direction=%d rows=%u phase=%u focus=%d current=%c target=%c letters=%08lx mode=%s.",left,right,direction,context.count,context.phase,context.focus,context.current?context.current:'-',target?target:'-',(unsigned long)letters,PICKER_ALPHA_NAVIGATION?"navigate":"observe-only");log_line(text);
    }
    if(target)picker_alpha_dispatch(menu,&context,target);
}
__declspec(dllexport) int __cdecl ReRiffConfigurePickerAlphabet(void *jump){
    if(!jump)return 0;picker_alpha_jump=(int (__cdecl *)(void *,int))jump;return 1;
}
