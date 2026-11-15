/* Native text rendering and the shared editor, without nesting an overlay.
 * The managed callback rebuilds result rows through the original picker. */
typedef int (__cdecl *PickerSearchBridge)(int,void *,const WCHAR *,int);
static PickerSearchBridge picker_search_bridge;
static void *picker_search_owner,*picker_search_field;
static BOOL picker_search_borrowed,picker_search_filtered;
static WCHAR picker_search_query[256],picker_search_sent[256];
static volatile LONG picker_search_available,picker_search_request;
static RECT picker_search_rect;
static unsigned picker_search_columns=32;
static DWORD picker_search_changed,picker_search_release;
static int picker_search_status=-3;
static InputFn picker_search_original_input;
static BOOL picker_search_controller_armed,picker_search_wait_neutral;
/* Native tickInput reads logical Back (2) before dispatching row input. */
static BOOL picker_search_back_native(void){
    if(GetAsyncKeyState(VK_ESCAPE)&0x8000)return TRUE;
    if(!readable(input_manager,4)||!readable(*input_manager,4))return FALSE;
    void *manager=*input_manager;void **table=*(void ***)manager;if(!readable(table,8))return FALSE;
    void *device=((void *(__thiscall *)(void *,unsigned))table[1])(manager,0);
    if(!readable(device,4)||!readable(*(void ***)device,8))return FALSE;
    typedef BYTE (__thiscall *Button)(void *,unsigned);
    return ((Button)(*(void ***)device)[0])(device,2)||((Button)(*(void ***)device)[1])(device,2);
}
static BOOL (*picker_search_back)(void)=picker_search_back_native;
/* The original picker reads X360 slot 3 for its X/favourite edge. Reuse
 * that controller and button query; Y=0x8000, A=0x1000, B=0x2000. */
static DWORD picker_search_controller_buttons(void){
    if(!readable(input_manager,4)||!readable(*input_manager,4))return 0;
    void *manager=*input_manager;void **table=*(void ***)manager;if(!readable(table,8))return 0;
    void *device=((void *(__thiscall *)(void *,unsigned))table[1])(manager,0);
    if(!readable(device,4)||!readable(*(void ***)device,80))return 0;
    void *controller=((void *(__thiscall *)(void *))(*(void ***)device)[19])(device);
    if(!readable(controller,4)||!readable(*(void ***)controller,16))return 0;
    typedef BYTE (__thiscall *Pressed)(void *,DWORD);Pressed pressed=(Pressed)(*(void ***)controller)[3];
    DWORD result=0;for(DWORD button=0x1000;button<=0x8000;button<<=1)if(button!=0x4000&&pressed(controller,button))result|=button;
    return result;
}
static DWORD (*picker_search_buttons)(void)=picker_search_controller_buttons;
static void picker_search_controller(DWORD buttons,BOOL foreground){
    if(!foreground){picker_search_controller_armed=FALSE;return;}
    if(!buttons){picker_search_controller_armed=TRUE;return;}
    if(!picker_search_controller_armed)return;
    picker_search_controller_armed=FALSE;
    if(text_entry_inline||picker_search_filtered){
        if(buttons&0x2000)InterlockedExchange(&text_entry_action,-1);
        else if(text_entry_inline&&(buttons&0x1000)&&!steam_keyboard_waiting)InterlockedExchange(&text_entry_action,1);
    }else if(buttons==0x8000)InterlockedExchange(&picker_search_request,2);
}
static BOOL picker_search_is_owner(void *menu){return menu&&readable(menu,416)&&*(void ***)menu==(void **)((BYTE *)GetModuleHandleW(NULL)+0x7a54bc);}
static BOOL picker_search_message(HWND window,UINT message,WPARAM key,LPARAM flags){
    if(!picker_search_available)return FALSE;
    /* Focus may move to a manually opened Steam keyboard. Keep the edit. */
    if(message==WM_KEYDOWN&&key==VK_ESCAPE&&(text_entry_inline||picker_search_filtered)){
        InterlockedExchange(&text_entry_action,-1);return TRUE;
    }
    if(message==WM_KEYDOWN&&key=='F'&&(GetKeyState(VK_CONTROL)&0x8000)){
        InterlockedExchange(&picker_search_request,1);return TRUE;
    }
    if(message==WM_LBUTTONDOWN){
        POINT p={(short)LOWORD(flags),(short)HIWORD(flags)};
        EnterCriticalSection(&text_entry_lock);RECT hit=picker_search_rect;LeaveCriticalSection(&text_entry_lock);
        if(PtInRect(&hit,p)){InterlockedExchange(&picker_search_request,1);return TRUE;}
    }
    (void)window;return FALSE;
}
static void picker_search_forget(void *menu){
    if(picker_search_owner!=menu)return;
    InterlockedExchange(&picker_search_available,0);InterlockedExchange(&picker_search_request,0);steam_keyboard_forget();text_entry_end_inline();
    if(picker_search_bridge)picker_search_bridge(3,menu,NULL,0);
    if(picker_search_field&&!picker_search_borrowed&&readable(((void **)menu)[46],48)){
        ((int *)picker_search_field)[1]++;remove_screen(((void **)menu)[46],picker_search_field);
    }
    picker_search_owner=picker_search_field=NULL;picker_search_filtered=FALSE;picker_search_query[0]=picker_search_sent[0]=0;picker_search_release=0;picker_search_controller_armed=FALSE;picker_search_wait_neutral=FALSE;
}
static BOOL picker_search_attach(void *menu){
    if(!picker_search_bridge||!picker_search_is_owner(menu)||!readable(((void **)menu)[46],48)||!readable(((void **)menu)[75],484))return FALSE;
    float size[2];if(!ui_actual_resolution(size))return FALSE;
    void *header=((void **)menu)[75];float height=((float *)header)[13],width=((float *)menu)[77]-40;
    float top=(size[1]-((float *)menu)[78])*0.5f;
    if(height<=0||width<=0)return FALSE;
    InitOnceExecuteOnce(&text_entry_once,text_entry_initialize,NULL,NULL);
    picker_search_borrowed=top<height+12;
    void *field=picker_search_borrowed?header:create_text(L"M",size[0]*.5f,top-height-8,4,1,0,-1,FALSE,-1,FALSE,TRUE);
    if(!field)return FALSE;
    float char_width=height*.65f;
    if(!picker_search_borrowed){
        /* createRaw provides the current native font's measured M width. */
        float measured=((float *)field)[12];if(measured>0&&measured<width)char_width=measured;
        ((int *)field)[15]=600;menu_item(field,FALSE);enable_input(field,FALSE);
        ((int *)field)[1]++;add_screen(((void **)menu)[46],field);
    }
    picker_search_columns=(unsigned)(width/char_width);if(picker_search_columns>96)picker_search_columns=96;if(picker_search_columns<8)picker_search_columns=8;
    picker_search_owner=menu;picker_search_field=field;picker_search_status=picker_search_bridge(0,menu,NULL,0)==1?-3:-2;
    return TRUE;
}
static void picker_search_clear(void){
    picker_search_query[0]=picker_search_sent[0]=0;picker_search_filtered=FALSE;
    picker_search_status=picker_search_bridge(1,picker_search_owner,L"",0);
}
static void __thiscall picker_search_input(void *menu,int selection){
    if(menu==picker_search_owner){
        if(text_entry_inline||(LONG)(picker_search_release-GetTickCount())>0)return;
        if(picker_search_filtered){
            if(selection>=4){if(picker_search_status!=-1&&picker_search_status!=-2)picker_search_status=picker_search_bridge(4,menu,NULL,selection-4);return;}
            if(selection==0||selection==1||selection==2){picker_search_clear();return;}
            return; /* Favourite actions must never store a search as a folder. */
        }
    }
    picker_search_original_input(menu,selection);
}
static BOOL picker_search_before(void *menu){
    if(!picker_search_is_owner(menu)||!picker_search_bridge)return FALSE;
    if(!readable(overlay_manager,4)||!readable(*overlay_manager,48)||((void **)*overlay_manager)[11]!=menu||*(DWORD *)((BYTE *)menu+28)!=1){
        if(menu==picker_search_owner)InterlockedExchange(&picker_search_available,0);return FALSE;
    }
    if(picker_search_owner!=menu&&!picker_search_attach(menu))return FALSE;
    attach_scroll_window();
    HWND window=display_window();BOOL foreground=IsWindow(window)&&!IsIconic(window)&&GetForegroundWindow()==window;
    InterlockedExchange(&picker_search_available,foreground);
    picker_search_controller(picker_search_buttons(),foreground);
    if(foreground&&(text_entry_inline||picker_search_filtered)&&picker_search_back())InterlockedExchange(&text_entry_action,-1);
    if(picker_search_wait_neutral){
        if(!foreground||picker_search_back())return TRUE;
        picker_search_wait_neutral=FALSE;picker_search_release=GetTickCount()+180;
    }
    LONG request=InterlockedExchange(&picker_search_request,0);
    if(request&&!text_entry_active){text_entry_begin_inline(picker_search_query);picker_search_changed=GetTickCount();}
    DWORD now=GetTickCount();
    if(!text_entry_inline&&picker_search_filtered&&InterlockedExchange(&text_entry_action,0)<0){
        picker_search_clear();picker_search_wait_neutral=TRUE;picker_search_release=now+180;return TRUE;
    }
    if(text_entry_inline){
        WCHAR query[256];EnterCriticalSection(&text_entry_lock);wcscpy(query,text_entry_model.value);LeaveCriticalSection(&text_entry_lock);
        if(wcscmp(query,picker_search_query)){wcscpy(picker_search_query,query);picker_search_changed=now;}
        LONG action=InterlockedExchange(&text_entry_action,0);
        if(action<0){steam_keyboard_forget();text_entry_end_inline();picker_search_clear();picker_search_wait_neutral=TRUE;picker_search_release=now+180;}
        else {
            if(wcscmp(picker_search_sent,picker_search_query)&&(action>0||now-picker_search_changed>=180)){
                wcscpy(picker_search_sent,picker_search_query);picker_search_filtered=picker_search_query[0]!=0;
                picker_search_status=picker_search_bridge(1,menu,picker_search_query,0);
            }
            if(action>0){steam_keyboard_forget();text_entry_end_inline();picker_search_release=now+180;}
        }
    }
    if(picker_search_filtered)picker_search_status=picker_search_bridge(2,menu,NULL,0);
    return text_entry_inline||(LONG)(picker_search_release-now)>0;
}
static void picker_search_after(void *menu){
    if(menu!=picker_search_owner)return;
    if(picker_search_borrowed)picker_search_field=((void **)menu)[75];
    if(!readable(picker_search_field,484))return;
    WCHAR line[256];
    if(text_entry_inline)text_entry_line(line,picker_search_columns,GetTickCount());
    else if(!picker_search_query[0])wcscpy(line,L"Search music (Y / Ctrl+F)");
    else {
        WCHAR state[80];int count=picker_search_status;
        if(count==-1)wcscpy(state,L"Searching");else if(count==-2)wcscpy(state,L"Search unavailable");
        else {BOOL partial=count<=-10;if(partial)count=-count-10;_snwprintf(state,80,L"%d matches%ls%ls",count,count>200?L"; first 200":L"",partial?L"; partial scan":L"");}
        _snwprintf(line,256,L"%ls: %ls",state,picker_search_query);line[255]=0;
    }
    /* Native wrapping stays disabled; clip the inactive label to its viewport. */
    if(!text_entry_inline&&wcslen(line)>picker_search_columns){unsigned end=picker_search_columns-1;if(end&&text_high(line[end-1]))end--;line[end]=0x2026;line[end+1]=0;}
    raw_text(picker_search_field,line);
    float allowed=((float *)menu)[77]-40-(picker_search_borrowed?((float *)menu)[93]:0);
    while(((float *)picker_search_field)[12]>allowed&&picker_search_columns>1){
        picker_search_columns--;
        if(text_entry_inline)text_entry_line(line,picker_search_columns,GetTickCount());
        else {unsigned end=picker_search_columns-1;if(end&&text_high(line[end-1]))end--;line[end]=0x2026;line[end+1]=0;}
        raw_text(picker_search_field,line);
    }
    float width=((float *)menu)[77]-40,x=((float *)picker_search_field)[8],y=((float *)picker_search_field)[9],h=((float *)picker_search_field)[13];
    RECT client;HWND window=display_window();float size[2];
    if(IsWindow(window)&&GetClientRect(window,&client)&&ui_actual_resolution(size)){
        float sx=(client.right-client.left)/size[0],sy=(client.bottom-client.top)/size[1];
        float left=picker_search_borrowed?x:x-width*.5f;
        if(picker_search_borrowed)width-=((float *)menu)[93];
        RECT hit={(LONG)(left*sx),(LONG)(y*sy),(LONG)((left+width)*sx),(LONG)((y+h)*sy)};
        EnterCriticalSection(&text_entry_lock);picker_search_rect=hit;LeaveCriticalSection(&text_entry_lock);
    }
}
__declspec(dllexport) int __cdecl ReRiffConfigurePickerSearch(void *callback){
    if(!callback)return 0;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void **table=(void **)(base+0x7a54bc);BYTE *entry=base+0x2d6940;
    if(!readable(table,25*4)||table[24]!=entry||!readable(entry,6)||entry[0]!=0xff||entry[1]!=0x25||*(DWORD *)(entry+2)!=(DWORD)(base+0x7a552c))return 0;
    DWORD old,ignored;if(!VirtualProtect(table+24,4,PAGE_READWRITE,&old))return 0;
    picker_search_original_input=(InputFn)table[24];picker_search_bridge=(PickerSearchBridge)callback;InterlockedExchangePointer(table+24,picker_search_input);VirtualProtect(table+24,4,old,&ignored);return 1;
}
