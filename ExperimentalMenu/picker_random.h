/* R3 random-song roll for the native file picker.
 *
 * The picker search bridge already builds a recursive library snapshot from
 * MyMusic, the saved music roots and the current picker folder. Operation 6
 * asks that snapshot for a random song under the current folder and starts it
 * through the original onFileSelected path, so files in subfolders work. */
#define PICKER_RANDOM_R3 0x0080
static void *picker_random_owner;
static BOOL picker_random_previous,picker_random_pending,picker_random_wait_logged;
static DWORD picker_random_pending_at,picker_random_deadline;
static WCHAR picker_random_path_prefix[MAX_PATH];
static unsigned picker_random_events;

static BOOL picker_random_path(void *menu,WCHAR *out,int size){
    BYTE *begin=*(BYTE **)((BYTE *)menu+264),*end=*(BYTE **)((BYTE *)menu+268);
    if(!begin||!end||end<begin||(size_t)(end-begin)>24576||size<2)return FALSE;
    size_t used=0;out[0]=0;
    for(BYTE *part=begin;part<end;part+=24){
        DWORD length,capacity;const WCHAR *text;
        if(!picker_alpha_read(part+16,&length,4)||!picker_alpha_read(part+20,&capacity,4)
            ||!length||length>32767||capacity<length)return FALSE;
        if(capacity<8)text=(const WCHAR *)part;
        else if(!picker_alpha_read(part,&text,4))return FALSE;
        if(used+length+2>=(size_t)size)return FALSE;
        if(!picker_alpha_read(text,out+used,(SIZE_T)length*2))return FALSE;
        used+=length;out[used++]=L'\\';out[used]=0;
    }
    return used>0;
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

static int picker_random_roll(void *menu){
    if(!picker_search_bridge)return -2;
    int result=picker_search_bridge(6,menu,picker_random_path_prefix,0);
    if(result==1){
        if(picker_random_events++<32)log_line("Picker random roll: autostart dispatched.");
    }else if((result==-1||result==0)&&!picker_random_wait_logged){
        picker_random_wait_logged=TRUE;log_line("Picker random roll: waiting for the recursive library scan.");
    }else if(result==-2){
        log_line("Picker random roll: the picker could not start the selected file.");
    }
    return result;
}

static void picker_random_start(void *menu){
    if(!picker_search_bridge){log_line("Picker random roll: search bridge unavailable.");return;}
    if(!picker_random_path(menu,picker_random_path_prefix,MAX_PATH)){
        log_line("Picker random roll: open a music folder first.");return;
    }
    picker_random_pending=TRUE;picker_random_wait_logged=FALSE;
    picker_random_pending_at=GetTickCount();picker_random_deadline=picker_random_pending_at+5000;
    int result=picker_random_roll(menu);
    if(result==1||result==-2)picker_random_pending=FALSE;
}

static void picker_random_forget(void *menu){
    if(picker_random_owner==menu){picker_random_owner=NULL;picker_random_previous=FALSE;picker_random_pending=FALSE;}
}

static void picker_random_tick(void *menu){
    if(!readable(menu,336)||*(void ***)menu!=(void **)(((BYTE *)GetModuleHandleW(NULL))+0x7a54bc))return;
    if(picker_random_owner!=menu){picker_random_owner=menu;picker_random_previous=FALSE;picker_random_pending=FALSE;}
    HWND window=display_window();
    BOOL foreground=IsWindow(window)&&!IsIconic(window)&&GetForegroundWindow()==window;
    if(!foreground){picker_random_previous=FALSE;picker_random_pending=FALSE;return;}
    if(picker_random_pending&&(DWORD)(GetTickCount()-picker_random_pending_at)>=250){
        if((LONG)(GetTickCount()-picker_random_deadline)>=0){
            picker_random_pending=FALSE;
            log_line("Picker random roll: no files under the current folder.");
        }else{
            int result=picker_random_roll(menu);picker_random_pending_at=GetTickCount();
            if(result==1||result==-2)picker_random_pending=FALSE;
        }
    }
    BOOL pressed=picker_random_r3();
    if(pressed&&!picker_random_previous)picker_random_start(menu);
    picker_random_previous=pressed;
}
