/* Extend the original PC display interface and Graphics confirmation overlay. */
typedef struct DisplayMode {UINT width,height,render_width,render_height,refresh,samples;} DisplayMode;
typedef UINT (__thiscall *ModeCountFn)(void *);
typedef const DisplayMode *(__thiscall *ModeGetFn)(void *,UINT);
typedef void (__thiscall *ModeCurrentFn)(void *,DisplayMode *,BYTE *,BYTE *);
typedef BYTE (__thiscall *ModeSetFn)(void *,const DisplayMode *,BYTE,BYTE);
typedef void (__cdecl *GraphicsBridgeFn)(void *);
typedef void (__thiscall *OverlayFn)(void *,int,int,BYTE,int,BYTE);
static ModeCountFn mode_count_original;
static ModeGetFn mode_get_original;
static ModeCurrentFn mode_current_original;
static ModeSetFn mode_set_original;
static InputFn graphics_input_original;
static unsigned display_reset_serial;
static BYTE display_reset_result;
static GraphicsBridgeFn graphics_resize,graphics_rescale;
static BOOL graphics_bridge_ready,display_installed,display_catalog_scope,borderless_active,display_startup_checked;
static DisplayMode extra_modes[24];
static UINT extra_mode_count;
static ScreenFn confirm_init_original;
static InputFn confirm_enter_original;
static TickFn confirm_tick_original;
static InputFn confirm_input_original;
static DWORD confirm_started;static BOOL confirm_done;
static DWORD confirm_trace_second;
static DWORD (WINAPI *display_clock)(void)=GetTickCount;
static float *graphics_dt;
static void **graphics_game,**graphics_states;
static DisplayMode *confirm_previous_mode;
static BYTE *confirm_previous_full;
typedef struct WindowState {HWND window;LONG_PTR style,extended;RECT rect;BOOL valid;} WindowState;
static WindowState normal_window;
static struct {BOOL active,old_borderless;DisplayMode old_mode;BYTE old_full;WindowState old_window,old_normal;} display_change;
static void *native_display_pc(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void **graphics=(void **)(base+0x80d408);
    if(!readable(graphics,4)||!readable(*graphics,0x1c)||*(void **)*graphics!=base+0x5fbf44)return NULL;
    return (BYTE *)*graphics+0x18;
}
static void *(*display_pc)(void)=native_display_pc;
static HWND native_display_window(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void **window=(void **)(base+0x7fcc28);
    return readable(window,4)&&readable(*window,8)?*(HWND *)((BYTE *)*window+4):NULL;
}
static HWND (*display_window)(void)=native_display_window;
static void display_style_now(void);
static HWND display_message_window;
static WNDPROC display_original_proc;
static UINT display_message;
static volatile LONG display_style_pending;
typedef struct DisplayWindowWork {WindowState state;} DisplayWindowWork;
static void restore_window_now(const WindowState *state);
static LRESULT CALLBACK display_window_proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    if(message==display_message){
        DisplayWindowWork *work=(DisplayWindowWork *)lp;
        if(work){restore_window_now(&work->state);HeapFree(GetProcessHeap(),0,work);}
        else {InterlockedExchange(&display_style_pending,0);display_style_now();}
        return 0;
    }
    return CallWindowProcW(display_original_proc,window,message,wp,lp);
}
static BOOL queue_window_work(HWND window,const WindowState *restore){
    if(display_message_window!=window){
        /* Extend the existing window/message pump. Never synchronously send a
         * style/position change from the menu worker while its owner waits. */
        if(display_message_window)return FALSE;
        if(!display_message)display_message=RegisterWindowMessageW(L"ReRiff.DisplayStyle.1");
        if(!display_message)return FALSE;
        display_original_proc=(WNDPROC)GetWindowLongPtrW(window,GWLP_WNDPROC);
        if(!display_original_proc||!SetWindowLongPtrW(window,GWLP_WNDPROC,(LONG_PTR)display_window_proc))return FALSE;
        display_message_window=window;
    }
    DisplayWindowWork *work=NULL;
    if(restore){
        work=HeapAlloc(GetProcessHeap(),0,sizeof(*work));if(!work)return FALSE;
        work->state=*restore;
    }else if(InterlockedCompareExchange(&display_style_pending,1,0))return TRUE;
    if(PostMessageW(window,display_message,0,(LPARAM)work))return TRUE;
    if(work)HeapFree(GetProcessHeap(),0,work);else InterlockedExchange(&display_style_pending,0);
    return FALSE;
}
static WindowState capture_window(HWND window){
    WindowState state={0};state.window=window;
    if(IsWindow(window)&&GetWindowRect(window,&state.rect)){
        state.style=GetWindowLongPtrW(window,GWL_STYLE);state.extended=GetWindowLongPtrW(window,GWL_EXSTYLE);state.valid=TRUE;
    }
    return state;
}
static void restore_window_now(const WindowState *state){
    if(!state->valid||!IsWindow(state->window))return;
    SetWindowLongPtrW(state->window,GWL_STYLE,state->style);SetWindowLongPtrW(state->window,GWL_EXSTYLE,state->extended);
    SetWindowPos(state->window,NULL,state->rect.left,state->rect.top,state->rect.right-state->rect.left,state->rect.bottom-state->rect.top,
        SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE);
}
static void restore_window(const WindowState *state){
    if(!state->valid||!IsWindow(state->window))return;
    if(GetWindowThreadProcessId(state->window,NULL)==GetCurrentThreadId())restore_window_now(state);
    else if(!queue_window_work(state->window,state))log_line("Could not queue display window restoration.");
}
static BOOL monitor_rect(HWND window,RECT *rect){
    MONITORINFO info={0};info.cbSize=sizeof(info);
    if(!IsWindow(window)||!GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&info))return FALSE;
    *rect=info.rcMonitor;return TRUE;
}
static void display_style_now(void){
    if(!display_installed)return;
    HWND window=display_window();RECT monitor,current;
    if(!borderless_active){if(normal_window.valid){restore_window(&normal_window);normal_window.valid=FALSE;}return;}
    if(!monitor_rect(window,&monitor))return;
    if(!normal_window.valid)normal_window=capture_window(window);
    LONG_PTR style=GetWindowLongPtrW(window,GWL_STYLE),extended=GetWindowLongPtrW(window,GWL_EXSTYLE);
    LONG_PTR desired=(style&~(LONG_PTR)WS_OVERLAPPEDWINDOW)|WS_POPUP;
    LONG_PTR desired_ex=extended&~(LONG_PTR)(WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE);
    if(style==desired&&extended==desired_ex&&GetWindowRect(window,&current)&&EqualRect(&current,&monitor))return;
    SetWindowLongPtrW(window,GWL_STYLE,desired);SetWindowLongPtrW(window,GWL_EXSTYLE,desired_ex);
    SetWindowPos(window,NULL,monitor.left,monitor.top,monitor.right-monitor.left,monitor.bottom-monitor.top,
        SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE);
}
static void display_style(void){
    if(!display_installed||(!borderless_active&&!normal_window.valid))return;
    HWND window=display_window();if(!IsWindow(window))return;
    if(GetWindowThreadProcessId(window,NULL)==GetCurrentThreadId())display_style_now();
    else if(!queue_window_work(window,NULL))log_line("Could not queue display window styling.");
}
static void graphics_notify_resize(void){
    if(readable(graphics_states,4)&&*graphics_states)graphics_rescale(*graphics_states);
    if(readable(graphics_game,4)&&*graphics_game)graphics_resize(*graphics_game);
}
static BOOL render_limit(UINT *width,UINT *height){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void **backend=(void **)(base+0x7fcc44);
    if(!readable(backend,4)||!readable(*backend,0x80))return FALSE;
    IDirect3DDevice9 *device=*(IDirect3DDevice9 **)((BYTE *)*backend+0x78);D3DCAPS9 caps;
    if(!readable(device,4)||FAILED(IDirect3DDevice9_GetDeviceCaps(device,&caps)))return FALSE;
    *width=caps.MaxTextureWidth;*height=caps.MaxTextureHeight;return TRUE;
}
static BOOL (*display_limits)(UINT *,UINT *)=render_limit;
static void prepare_extra_modes(void *pc){
    static const UINT presets[][2]={{1280,720},{1536,864},{1600,900},{1920,1080},{1920,1200},
        {2560,1440},{2560,1600},{3440,1440},{3840,1600},{3840,2160},{5120,1440},{5120,2160},{5120,2880},{7680,4320}};
    UINT max_width=0,max_height=0;extra_mode_count=0;
    if(!display_limits(&max_width,&max_height))return;
    UINT count=mode_count_original(pc);if(count>4096)return;
    for(UINT i=0;i<sizeof(presets)/sizeof(presets[0]);i++){
        UINT width=presets[i][0],height=presets[i][1];BOOL exists=FALSE;
        if(width>max_width||height>max_height)continue;
        for(UINT j=0;j<count;j++){
            const DisplayMode *mode=mode_get_original(pc,j);
            if(readable(mode,sizeof(*mode))&&mode->width==width&&mode->height==height){exists=TRUE;break;}
        }
        if(!exists)extra_modes[extra_mode_count++]=(DisplayMode){width,height,width,height,60,0};
    }
}
static UINT __thiscall extended_mode_count(void *pc){return mode_count_original(pc)+(display_catalog_scope?extra_mode_count:0);}
static const DisplayMode *__thiscall extended_mode_get(void *pc,UINT index){
    UINT count=mode_count_original(pc);
    if(display_catalog_scope&&index>=count&&index-count<extra_mode_count)return &extra_modes[index-count];
    return mode_get_original(pc,index);
}
static void __thiscall extended_mode_current(void *pc,DisplayMode *mode,BYTE *full,BYTE *sync){
    mode_current_original(pc,mode,full,sync);
    if(display_catalog_scope&&borderless_active){mode->width=mode->render_width;mode->height=mode->render_height;}
}
static BYTE __thiscall extended_mode_set(void *pc,const DisplayMode *mode,BYTE single_sample,BYTE full){
    DisplayMode requested=*mode;RECT monitor;
    /* Native catalog entries may leave render dimensions zero (use logical).
     * Resolve those before substituting the monitor-sized borderless window. */
    if(!requested.render_width)requested.render_width=requested.width;
    if(!requested.render_height)requested.render_height=requested.height;
    if(borderless_active){
        if(!monitor_rect(display_window(),&monitor))return FALSE;
        requested.width=(UINT)(monitor.right-monitor.left);requested.height=(UINT)(monitor.bottom-monitor.top);
        full=FALSE;
    }
    char trace[192];snprintf(trace,sizeof(trace),"Display reset begin: logical=%ux%u render=%ux%u exclusive=%u borderless=%u thread=%lu.",requested.width,requested.height,requested.render_width,requested.render_height,full,borderless_active,(unsigned long)GetCurrentThreadId());log_line(trace);
    BYTE changed=mode_set_original(pc,&requested,single_sample,full);
    display_reset_result=changed;display_reset_serial++;
    log_line(changed?"Display reset returned success.":"Display reset returned failure.");
    if(changed)display_style();return changed;
}
static void save_display_preference(void){
    void *pc=display_pc();if(!pc)return;DisplayMode mode;BYTE full,sync;mode_current_original(pc,&mode,&full,&sync);
    WCHAR width[24],height[24];_snwprintf(width,24,L"%u",mode.render_width);_snwprintf(height,24,L"%u",mode.render_height);
    if(!WritePrivateProfileStringW(L"graphics",L"borderless",borderless_active?L"true":L"false",config_path)
        ||!WritePrivateProfileStringW(L"graphics",L"render_width",width,config_path)
        ||!WritePrivateProfileStringW(L"graphics",L"render_height",height,config_path)
        ||!WritePrivateProfileStringW(L"graphics",L"exclusive_fullscreen",full?L"true":L"false",config_path))log_line("Could not persist confirmed display settings.");
}
static void cancel_display_style(void){
    if(display_change.active)borderless_active=display_change.old_borderless;
}
static void finish_display_rollback(void){
    if(!display_change.active)return;
    restore_window(&display_change.old_window);normal_window=display_change.old_normal;
    display_change.active=FALSE;display_style();log_line("Display settings reverted.");
}
static void __thiscall display_confirm_init(void *menu,void *screen,void *lower){
    confirm_init_original(menu,screen,lower);
    ((float *)menu)[68]=10;((int *)menu)[69]=-1;
}
static void __thiscall display_confirm_enter(void *menu,int previous){
    confirm_enter_original(menu,previous);
    confirm_started=display_clock();confirm_done=FALSE;
    confirm_trace_second=(DWORD)-1;
    ((float *)menu)[68]=10;((int *)menu)[69]=-1;
    log_line("Display confirmation entered: 10-second Keep/Revert deadline started.");
}
static void __thiscall display_confirm_tick(void *menu){
    if(confirm_done){base_tick(menu);return;}
    DWORD elapsed=display_clock()-confirm_started;BOOL expired=elapsed>=10000;
    if(confirm_trace_second!=elapsed/1000){
        confirm_trace_second=elapsed/1000;
        void *screen=((void **)menu)[46],*keep=((void **)menu)[62],*reject=((void **)menu)[63];
        void *group=readable(screen,16)?((void **)screen)[3]:NULL;
        char trace[320];snprintf(trace,sizeof(trace),"Display confirmation state: ms=%lu phase=%d focus=%d ignore=%u lower=%p screenMask=%lu groupMask=%lu keepMask=%lu rejectMask=%lu.",(unsigned long)elapsed,((int *)menu)[7],((int *)menu)[11],*((BYTE *)menu+208),((void **)menu)[47],(unsigned long)(readable(screen,44)?*(DWORD *)((BYTE *)screen+40):0),(unsigned long)(readable(group,80)?*(DWORD *)((BYTE *)group+76):0),(unsigned long)(readable(keep,80)?*(DWORD *)((BYTE *)keep+76):0),(unsigned long)(readable(reject,80)?*(DWORD *)((BYTE *)reject+76):0));log_line(trace);
    }
    float dt=graphics_dt?*graphics_dt:0;
    if(!(dt>=0&&dt<10))dt=0;
    /* Reuse native formatting and revertChanges. Its truncating float-to-int
     * countdown is fed whole remaining seconds, so it cannot revert at 9s. */
    ((float *)menu)[68]=(expired?0:(float)((10000-elapsed+999)/1000))+dt+0.0001f;
    if(expired){((int *)menu)[69]=1;cancel_display_style();confirm_done=TRUE;}
    confirm_tick_original(menu);
    if(expired)finish_display_rollback();
}
static void __thiscall display_confirm_input(void *menu,int selection){
    char trace[96];snprintf(trace,sizeof(trace),"Display confirmation input: id=%d closed=%u elapsed=%lu.",selection,confirm_done,(unsigned long)(display_clock()-confirm_started));log_line(trace);
    if(confirm_done)return;
    if(selection==0||selection==1)confirm_done=TRUE;
    if(selection==1)cancel_display_style();
    confirm_input_original(menu,selection);
    if(selection==1)finish_display_rollback();
    else if(selection==0){save_display_preference();display_change.active=FALSE;log_line("Display settings confirmed.");}
}
static BOOL apply_display(void *menu,BOOL borderless){
    if(!display_installed||!graphics_bridge_ready)return FALSE;
    void *pc=display_pc();if(!pc)return FALSE;
    DisplayMode *begin=((DisplayMode **)menu)[67],*end=((DisplayMode **)menu)[68];
    int selected=((int *)menu)[75];
    if(!begin||end<begin||end-begin>4096||selected<0||selected>=end-begin||!readable(begin,(size_t)(end-begin)*sizeof(*begin)))return FALSE;
    DisplayMode requested=begin[selected],previous;BYTE full,sync;
    if(!requested.render_width)requested.render_width=requested.width;
    if(!requested.render_height)requested.render_height=requested.height;
    mode_current_original(pc,&previous,&full,&sync);
    BYTE desired_full=borderless?FALSE:*((BYTE *)menu+305);
    if(borderless==borderless_active&&requested.render_width==previous.render_width&&requested.render_height==previous.render_height&&desired_full==full)return TRUE;
    display_change.old_borderless=borderless_active;display_change.old_mode=previous;display_change.old_full=full;
    display_change.old_window=capture_window(display_window());display_change.old_normal=normal_window;display_change.active=TRUE;
    if(borderless&&!borderless_active)normal_window=display_change.old_window;
    borderless_active=borderless;
    /* Retain the original managed Apply sequence and its thread transitions,
     * resource rescaling, previous-mode capture and confirmation overlay. A
     * borderless-only change still needs that sequence when its mode index
     * and native fullscreen byte are otherwise unchanged. */
    BYTE original_full=*((BYTE *)menu+304);unsigned serial=display_reset_serial;
    if(((int *)menu)[75]==((int *)menu)[74]&&*((BYTE *)menu+305)==original_full)
        *((BYTE *)menu+304)=!original_full;
    log_line("Display Apply: delegating to original Graphics handler.");
    graphics_input_original(menu,3);
    *((BYTE *)menu+304)=original_full;
    if(display_reset_serial==serial||!display_reset_result){
        cancel_display_style();finish_display_rollback();log_line("Native renderer rejected display settings; previous settings retained.");return TRUE;
    }
    *confirm_previous_mode=previous;*confirm_previous_full=full;
    log_line("Display Apply: original confirmation opened.");return TRUE;
}
static void display_restore_startup(void){
    if(!display_installed||!graphics_bridge_ready||display_startup_checked)return;
    void *pc=display_pc();if(!pc)return;display_startup_checked=TRUE;
    UINT width=GetPrivateProfileIntW(L"graphics",L"render_width",0,config_path),height=GetPrivateProfileIntW(L"graphics",L"render_height",0,config_path);
    UINT max_width=0,max_height=0;DisplayMode current;BYTE full,sync;mode_current_original(pc,&current,&full,&sync);
    if(width>=800&&height>=600&&display_limits(&max_width,&max_height)&&width<=max_width&&height<=max_height){
        WCHAR value[16];GetPrivateProfileStringW(L"graphics",L"exclusive_fullscreen",L"false",value,16,config_path);
        BYTE desired_full=!borderless_active&&!_wcsicmp(value,L"true");
        DisplayMode desired={width,height,width,height,current.refresh,0};
        if(width!=current.render_width||height!=current.render_height||desired_full!=full||borderless_active){
            if(extended_mode_set(pc,&desired,TRUE,desired_full)){graphics_notify_resize();log_line("Restored confirmed render resolution.");}
            else log_line("Could not restore confirmed render resolution; native startup mode retained.");
        }
    }
    display_style();
}
static BOOL validate_display_modes(BYTE *base,uintptr_t address){
    const DWORD methods[4]={0x97020,0x97050,0x97070,0x97100},slots[4]={0,1,2,5};
    for(int i=0;i<4;i++)if(!readable(base+0x5fc178+slots[i]*4,4)||*(DWORD *)(base+0x5fc178+slots[i]*4)!=address+methods[i])return FALSE;
    const DWORD entries[4]={0x2ec398,0x2eb42c,0x2ec677,0x2eb814},fixups[4]={0x7a5994,0x7a5988,0x7a599c,0x7a598c},indices[4]={15,3,24,2};
    for(int i=0;i<4;i++){
        BYTE *code=base+entries[i];
        if(!readable(code,6)||code[0]!=0xff||code[1]!=0x25||*(DWORD *)(code+2)!=address+fixups[i]
            ||*(DWORD *)(base+0x7a5918+4*indices[i])!=address+entries[i])return FALSE;
    }
    return TRUE;
}
static BOOL install_display_modes(BYTE *base){
    if(!validate_display_modes(base,(uintptr_t)base))return FALSE;
    void **pc=(void **)(base+0x5fc178),**confirm=(void **)(base+0x7a5918);DWORD old;
    if(!VirtualProtect(pc,6*sizeof(void *),PAGE_READWRITE,&old))return FALSE;
    mode_count_original=(ModeCountFn)pc[0];mode_get_original=(ModeGetFn)pc[1];mode_current_original=(ModeCurrentFn)pc[2];mode_set_original=(ModeSetFn)pc[5];
    InterlockedExchangePointer(&pc[0],(void *)extended_mode_count);InterlockedExchangePointer(&pc[1],(void *)extended_mode_get);
    InterlockedExchangePointer(&pc[2],(void *)extended_mode_current);InterlockedExchangePointer(&pc[5],(void *)extended_mode_set);
    DWORD ignored;VirtualProtect(pc,6*sizeof(void *),old,&ignored);
    if(!VirtualProtect(confirm,27*sizeof(void *),PAGE_READWRITE,&old))return FALSE;
    confirm_init_original=(ScreenFn)confirm[15];confirm_tick_original=(TickFn)confirm[3];confirm_input_original=(InputFn)confirm[24];
    confirm_enter_original=(InputFn)confirm[2];
    InterlockedExchangePointer(&confirm[2],(void *)display_confirm_enter);
    InterlockedExchangePointer(&confirm[15],(void *)display_confirm_init);InterlockedExchangePointer(&confirm[3],(void *)display_confirm_tick);
    InterlockedExchangePointer(&confirm[24],(void *)display_confirm_input);VirtualProtect(confirm,27*sizeof(void *),old,&ignored);
    graphics_dt=(float *)(base+0x7ef604);
    graphics_game=(void **)(base+0x7e9d24);graphics_states=(void **)(base+0x7eefe0);
    confirm_previous_mode=(DisplayMode *)(base+0x7ec1f8);confirm_previous_full=(BYTE *)(base+0x7ebf04);
    WCHAR value[16];GetPrivateProfileStringW(L"graphics",L"borderless",L"false",value,16,config_path);borderless_active=!_wcsicmp(value,L"true");
    display_installed=TRUE;return TRUE;
}
