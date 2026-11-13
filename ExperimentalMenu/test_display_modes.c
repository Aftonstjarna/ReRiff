#include "engine_menu.c"
#include <math.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static HWND host;
static WNDPROC fixture_original_proc;
static unsigned key_messages,mouse_messages,close_messages,position_messages;
static WPARAM activation;
static int last_width,last_height;
static LRESULT CALLBACK fixture_window_proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    if(message==WM_WINDOWPOSCHANGING){WINDOWPOS *pos=(WINDOWPOS *)lp;if(!(pos->flags&SWP_NOSIZE)){last_width=pos->cx;last_height=pos->cy;}}
    if(message==WM_ACTIVATE)activation=wp;
    if(message==WM_KEYDOWN)key_messages++;
    if(message==WM_LBUTTONDOWN)mouse_messages++;
    if(message==WM_WINDOWPOSCHANGED)position_messages++;
    if(message==WM_CLOSE){close_messages++;return 0;}
    return CallWindowProcW(fixture_original_proc,window,message,wp,lp);
}
static DisplayMode current_mode={1280,720,1280,720,60,0},saved_previous,received;
static DisplayMode native_modes[2]={{1280,720,1280,720,60,0},{1920,1080,1920,1080,60,0}};
static BYTE current_full,previous_full;
static DWORD now;
static int resets,rescales,resizes,overlays,reverts,kept,base_ticks;
static BOOL reject;
static float frame_dt=0.016f;
static BYTE graphics_menu[512],confirmation[512];
static BYTE pointer_device[64],pointer_backend[0x248];
static float pointer_value[2];
static float *__thiscall mock_pointer(void *device,float *position){
    if(device!=pointer_device)abort();position[0]=pointer_value[0];position[1]=pointer_value[1];return position;
}
static void *menu_table[27],*fake_game=(void *)1,*fake_states=(void *)2;
static void *mock_pc(void){return graphics_menu;}
static HWND mock_window(void){return host;}
static DWORD WINAPI mock_clock(void){return now;}
static BOOL mock_limits(UINT *width,UINT *height){*width=*height=4096;return TRUE;}
static UINT __thiscall mock_count(void *pc){(void)pc;return 2;}
static const DisplayMode *__thiscall mock_mode(void *pc,UINT index){(void)pc;return &native_modes[index%2];}
static void __thiscall mock_current(void *pc,DisplayMode *mode,BYTE *full,BYTE *sync){(void)pc;*mode=current_mode;*full=current_full;*sync=TRUE;}
static BYTE __thiscall mock_set(void *pc,const DisplayMode *mode,BYTE single,BYTE full){
    (void)pc;if(!single)abort();resets++;received=*mode;
    if(reject)return FALSE;
    current_mode=*mode;current_full=full;return TRUE;
}
static void __cdecl mock_resize(void *game){if(game!=fake_game)abort();resizes++;}
static void __cdecl mock_rescale(void *states){if(states!=fake_states)abort();rescales++;}
static void __thiscall mock_overlay(void *menu,int state,int fade,BYTE load,int loader,BYTE overlay){
    (void)menu;if(state!=26||fade||load||loader||!overlay)abort();overlays++;
}
static void __thiscall mock_init(void *menu,void *screen,void *lower){(void)screen;(void)lower;((float *)menu)[68]=15;((int *)menu)[69]=15;}
static void __thiscall mock_enter(void *menu,int previous){(void)menu;(void)previous;}
static void native_revert(void){extended_mode_set(mock_pc(),&saved_previous,TRUE,previous_full);reverts++;}
static void __thiscall mock_tick(void *menu){
    ((float *)menu)[68]-=frame_dt;int seconds=(int)((float *)menu)[68];
    if(seconds!=((int *)menu)[69]){((int *)menu)[69]=seconds;if(!seconds)native_revert();}
}
static void __thiscall mock_input(void *menu,int selected){(void)menu;if(selected==1)native_revert();else kept++;}
static void __thiscall mock_base_tick(void *menu){(void)menu;base_ticks++;}
static void __thiscall mock_graphics_apply(void *menu,int selection){
    if(selection!=3)abort();
    DisplayMode *choices=((DisplayMode **)menu)[67];int index=((int *)menu)[75];
    if(extended_mode_set(mock_pc(),&choices[index],TRUE,*((BYTE *)menu+305))){
        graphics_notify_resize();mock_overlay(menu,26,0,FALSE,0,TRUE);*((BYTE *)menu+306)=TRUE;
    }
}
static DWORD WINAPI style_from_worker(void *argument){(void)argument;display_style();return 0;}
static DWORD WINAPI restore_from_worker(void *argument){restore_window((const WindowState *)argument);return 0;}
static void drain_window_messages(void){MSG message;while(PeekMessageW(&message,NULL,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}}
static void prepare(DisplayMode *choices,UINT count,int selected,BYTE full){
    ((DisplayMode **)graphics_menu)[67]=choices;((DisplayMode **)graphics_menu)[68]=choices+count;
    ((int *)graphics_menu)[75]=selected;graphics_menu[305]=full;
}
int main(void){
    WNDCLASSW fixture_class={0};fixture_class.lpfnWndProc=DefWindowProcW;fixture_class.hInstance=GetModuleHandleW(NULL);fixture_class.lpszClassName=L"ReRiffDisplayFixture";
    CHECK(RegisterClassW(&fixture_class));
    host=CreateWindowW(fixture_class.lpszClassName,L"Display mode fixture",WS_OVERLAPPEDWINDOW,60,80,640,480,NULL,NULL,GetModuleHandleW(NULL),NULL);CHECK(host);
    fixture_original_proc=(WNDPROC)SetWindowLongPtrW(host,GWLP_WNDPROC,(LONG_PTR)fixture_window_proc);CHECK(fixture_original_proc);
    ShowWindow(host,SW_SHOW);WindowState original=capture_window(host);CHECK(original.valid);
    display_pc=mock_pc;display_window=mock_window;display_clock=mock_clock;display_limits=mock_limits;
    mode_count_original=mock_count;mode_get_original=mock_mode;mode_current_original=mock_current;mode_set_original=mock_set;
    pointer_position_original=mock_pointer;*(void **)(pointer_device+0x34)=pointer_backend;
    graphics_resize=mock_resize;graphics_rescale=mock_rescale;graphics_game=&fake_game;graphics_states=&fake_states;
    confirm_previous_mode=&saved_previous;confirm_previous_full=&previous_full;graphics_dt=&frame_dt;
    confirm_init_original=mock_init;confirm_tick_original=mock_tick;confirm_input_original=mock_input;base_tick=mock_base_tick;
    confirm_enter_original=mock_enter;
    graphics_input_original=mock_graphics_apply;
    *(void ***)graphics_menu=menu_table;menu_table[17]=(void *)mock_overlay;
    graphics_bridge_ready=display_installed=TRUE;
    RECT client;CHECK(GetClientRect(host,&client));
    current_mode=(DisplayMode){3840,2160,3840,2160,60,0};borderless_active=TRUE;
    pointer_value[0]=client.right*0.5f;pointer_value[1]=client.bottom*0.5f;
    float mapped[2];CHECK(display_pointer_position(pointer_device,mapped)==mapped);
    CHECK(mapped[0]==1920&&mapped[1]==1080);
    pointer_value[0]=(float)client.right;pointer_value[1]=(float)client.bottom;
    display_pointer_position(pointer_device,mapped);CHECK(mapped[0]==3840&&mapped[1]==2160);
    pointer_value[0]=-1;pointer_value[1]=-1;
    display_pointer_position(pointer_device,mapped);CHECK(mapped[0]==-1&&mapped[1]==-1);
    pointer_value[0]=23;pointer_value[1]=42;pointer_backend[0x246]=1;
    display_pointer_position(pointer_device,mapped);CHECK(mapped[0]==23&&mapped[1]==42);
    pointer_backend[0x246]=0;borderless_active=FALSE;
    display_pointer_position(pointer_device,mapped);CHECK(fabsf(mapped[0]-23.f*3840/client.right)<0.01f&&fabsf(mapped[1]-42.f*2160/client.bottom)<0.01f);
    pointer_backend[0x247]=1;display_pointer_position(pointer_device,mapped);CHECK(mapped[0]==23&&mapped[1]==42);pointer_backend[0x247]=0;
    current_mode=native_modes[0];
    /* The original renderer/window owner waits for the menu worker. Styling
     * must return without requiring that owner to pump a synchronous message. */
    borderless_active=TRUE;
    HANDLE worker=CreateThread(NULL,0,style_from_worker,NULL,0,NULL);CHECK(worker);
    CHECK(WaitForSingleObject(worker,1000)==WAIT_OBJECT_0);CloseHandle(worker);
    CHECK(display_style_pending==1);drain_window_messages();CHECK(display_style_pending==0);
    CHECK((GetWindowLongPtrW(host,GWL_STYLE)&WS_POPUP)&&!(GetWindowLongPtrW(host,GWL_STYLE)&WS_CAPTION));
    CHECK(display_original_proc==fixture_window_proc);
    unsigned positions=position_messages;
    for(unsigned i=0;i<100;i++)display_style();
    CHECK(position_messages==positions&&!display_style_pending);
    SendMessageW(host,WM_KEYDOWN,VK_RETURN,0);SendMessageW(host,WM_LBUTTONDOWN,MK_LBUTTON,0);SendMessageW(host,WM_CLOSE,0,0);
    CHECK(key_messages==1&&mouse_messages==1&&close_messages==1&&IsWindow(host));
    SetCapture(host);SendMessageW(host,WM_ACTIVATE,MAKEWPARAM(WA_INACTIVE,TRUE),0);
    CHECK(activation==WA_INACTIVE&&GetCapture()!=host);
    ShowWindow(host,SW_MINIMIZE);CHECK(IsIconic(host));WindowState minimized=capture_window(host);
    display_style();CHECK(IsIconic(host));WindowState still_minimized=capture_window(host);CHECK(EqualRect(&minimized.rect,&still_minimized.rect));
    ShowWindow(host,SW_RESTORE);display_style();
    borderless_active=FALSE;
    worker=CreateThread(NULL,0,restore_from_worker,&original,0,NULL);CHECK(worker);
    CHECK(WaitForSingleObject(worker,1000)==WAIT_OBJECT_0);CloseHandle(worker);
    drain_window_messages();CHECK(GetWindowLongPtrW(host,GWL_STYLE)==original.style);normal_window.valid=FALSE;
    borderless_preferred=TRUE;LPARAM alt=(LPARAM)1<<29;
    CHECK(!display_alt_enter(WM_SYSKEYDOWN,VK_RETURN,0));
    CHECK(display_alt_enter(WM_SYSKEYDOWN,VK_RETURN,alt)&&borderless_active);
    CHECK(display_alt_enter(WM_SYSKEYDOWN,VK_RETURN,alt|((LPARAM)1<<30))&&borderless_active);
    CHECK(display_alt_enter(WM_SYSKEYUP,VK_RETURN,alt)&&!display_alt_enter_held);
    CHECK(display_alt_enter(WM_SYSKEYDOWN,VK_RETURN,alt)&&!borderless_active);
    CHECK(GetWindowLongPtrW(host,GWL_STYLE)==original.style);
    CHECK(display_alt_enter(WM_SYSKEYUP,VK_RETURN,alt));
    drain_window_messages();
    for(int attempt=0;attempt<10;attempt++){Sleep(20);drain_window_messages();}
    RECT restored_client;CHECK(GetClientRect(host,&restored_client));
    printf("Restored client=%ldx%ld mode=%ux%u style=%08lx request=%dx%d\n",restored_client.right,restored_client.bottom,current_mode.width,current_mode.height,(unsigned long)GetWindowLongPtrW(host,GWL_STYLE),last_width,last_height);
    /* The headless Wine desktop can constrain the resulting HWND. Verify the
     * requested native frame size; pointer tests above cover client/render mismatch. */
    RECT requested_frame={0,0,1280,720};
    CHECK(AdjustWindowRectEx(&requested_frame,(DWORD)original.style,FALSE,(DWORD)original.extended));
    CHECK(last_width==requested_frame.right-requested_frame.left&&last_height==requested_frame.bottom-requested_frame.top);
    CHECK(restored_client.right>0&&restored_client.bottom>0);borderless_preferred=FALSE;
    CHECK(!display_alt_enter(WM_SYSKEYDOWN,VK_RETURN,alt)); /* Original exclusive shortcut. */
    CHECK(GetTempPathW(MAX_PATH,config_path));wcscat(config_path,L"reriff-display-test.ini");DeleteFileW(config_path);
    wcscpy(log_path,config_path);wcscat(log_path,L".log");
    prepare_extra_modes(mock_pc());CHECK(extra_mode_count>0&&extended_mode_count(mock_pc())==2);
    display_catalog_scope=TRUE;CHECK(extended_mode_count(mock_pc())==2+extra_mode_count);
    BOOL has4k=FALSE;
    for(UINT i=2;i<extended_mode_count(mock_pc());i++){
        const DisplayMode *mode=extended_mode_get(mock_pc(),i);
        CHECK(mode->width<=4096&&mode->height<=4096);
        if(mode->width==3840&&mode->height==2160)has4k=TRUE;
    }
    CHECK(has4k);display_catalog_scope=FALSE;
    DisplayMode native_zero={1536,864,0,0,60,0};
    borderless_active=TRUE;CHECK(extended_mode_set(mock_pc(),&native_zero,TRUE,FALSE));
    CHECK(received.render_width==1536&&received.render_height==864);
    borderless_active=FALSE;display_style();current_mode=native_modes[0];
    original=capture_window(host);CHECK(original.valid);
    resets=rescales=resizes=overlays=0;
    DisplayMode choices[2]={native_modes[0],{3840,2160,3840,2160,60,0}};
    prepare(choices,2,1,FALSE);CHECK(apply_display(graphics_menu,TRUE));
    RECT monitor,rect;CHECK(monitor_rect(host,&monitor)&&GetWindowRect(host,&rect)&&EqualRect(&monitor,&rect));
    CHECK((GetWindowLongPtrW(host,GWL_STYLE)&WS_POPUP)&&!(GetWindowLongPtrW(host,GWL_STYLE)&WS_CAPTION));
    CHECK(received.width==3840&&received.height==2160);
    CHECK(received.render_width==3840&&received.render_height==2160&&!current_full&&overlays==1&&resizes==1&&rescales==1);
    CHECK(display_change.active&&saved_previous.render_width==1280&&!previous_full);
    DisplayMode shown;BYTE full,sync;display_catalog_scope=TRUE;extended_mode_current(mock_pc(),&shown,&full,&sync);
    CHECK(shown.width==3840&&shown.height==2160);display_catalog_scope=FALSE;
    display_confirm_init(confirmation,NULL,NULL);CHECK(((float *)confirmation)[68]==10);
    now=60000;display_confirm_enter(confirmation,0);CHECK(confirm_started==60000&&!confirm_done);
    now=69999;display_confirm_tick(confirmation);CHECK(!reverts&&display_change.active&&((int *)confirmation)[69]==1);
    now=70000;display_confirm_tick(confirmation);CHECK(reverts==1&&!borderless_active&&!display_change.active&&current_mode.render_width==1280);
    CHECK(GetWindowRect(host,&rect)&&EqualRect(&rect,&original.rect)&&GetWindowLongPtrW(host,GWL_STYLE)==original.style);
    display_confirm_tick(confirmation);CHECK(reverts==1&&base_ticks==1);
    CHECK(apply_display(graphics_menu,TRUE));now=80000;display_confirm_init(confirmation,NULL,NULL);display_confirm_enter(confirmation,0);
    display_confirm_input(confirmation,1);CHECK(reverts==2&&!borderless_active&&!display_change.active);
    CHECK(apply_display(graphics_menu,TRUE));display_confirm_init(confirmation,NULL,NULL);display_confirm_enter(confirmation,0);display_confirm_input(confirmation,0);
    CHECK(kept==1&&borderless_active&&!display_change.active&&GetPrivateProfileIntW(L"graphics",L"render_width",0,config_path)==3840);
    int saved_overlays=overlays;prepare(choices,2,0,FALSE);reject=TRUE;CHECK(apply_display(graphics_menu,FALSE));reject=FALSE;
    CHECK(borderless_active&&!display_change.active&&overlays==saved_overlays&&current_mode.render_width==3840);
    prepare(choices,2,1,FALSE);int old_resets=resets;CHECK(apply_display(graphics_menu,TRUE)&&resets==old_resets);
    current_mode=native_modes[0];borderless_active=TRUE;display_startup_checked=FALSE;display_restore_startup();
    CHECK(current_mode.render_width==3840&&current_mode.width==3840&&current_mode.height==2160);
    borderless_active=FALSE;display_style();DestroyWindow(host);DeleteFileW(config_path);DeleteFileW(log_path);
    puts("PASS: GPU-limited catalog, coherent render dimensions, native pointer scaling and pass-through, window message forwarding, Alt+Enter borderless/windowed/repeat/native exclusive forwarding, idle/minimized behavior, borderless window, native confirmation, exact 10s timeout, reject/keep/failure rollback and startup restoration");return 0;
}
