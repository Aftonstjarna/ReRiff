/* Launcher Controls page: free-look camera and ReRiff binding settings.
 * Values are stored in the same client.ini read by the native game hook. */
#ifndef RERIFF_LAUNCHER_CONTROLS_H
#define RERIFF_LAUNCHER_CONTROLS_H
#include <stdlib.h>
static void checkbox_help(HWND,const WCHAR *);
static HWND controls_enable,controls_fov,controls_zoom,controls_invert_x,controls_invert_y;
static HWND controls_trace,controls_free_button,controls_free_key,controls_hud_button,controls_hud_key;
static HWND controls_mouse,controls_save,controls_first;static WCHAR controls_ini[CAP];

static void controls_add(HWND box,const WCHAR *text){graphics_choice(box,text,0);}
static void controls_pick(HWND box,const WCHAR *value){
    int count=(int)SendMessageW(box,CB_GETCOUNT,0,0);
    for(int i=0;i<count;i++){
        WCHAR text[64];
        SendMessageW(box,CB_GETLBTEXT,i,(LPARAM)text);
        if(!_wcsicmp(text,value)){SendMessageW(box,CB_SETCURSEL,i,0);return;}
    }
    controls_add(box,value);SendMessageW(box,CB_SETCURSEL,count,0);
}
static void controls_text(HWND box,WCHAR *out,int size){
    int index=(int)SendMessageW(box,CB_GETCURSEL,0,0);
    if(index==CB_ERR){out[0]=0;return;}
    SendMessageW(box,CB_GETLBTEXT,index,(LPARAM)out);out[size-1]=0;
}
static BOOL controls_trace_toggle(void){
    path(controls_ini,L"client.ini");
    BOOL enabled=SendMessageW(controls_trace,BM_GETCHECK,0,0)==BST_CHECKED;
    return WritePrivateProfileStringW(L"camera",L"race_orbit_trace",enabled?L"1":L"0",controls_ini);
}
static void controls_trace_reload(void){
    if(!controls_trace)return;
    path(controls_ini,L"client.ini");
    SendMessageW(controls_trace,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"race_orbit_trace",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
}
static void controls_reload(void){
    if(!controls_enable)return;
    path(controls_ini,L"client.ini");
    WCHAR value[64];
    SendMessageW(controls_enable,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"race_orbit",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"camera",L"free_fov_offset",L"0",value,64,controls_ini);SetWindowTextW(controls_fov,value);
    GetPrivateProfileStringW(L"camera",L"free_zoom",L"1.0",value,64,controls_ini);SetWindowTextW(controls_zoom,value);
    SendMessageW(controls_invert_x,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"invert_x",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(controls_invert_y,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"invert_y",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"controls",L"free_look_button",L"right_thumb",value,64,controls_ini);controls_pick(controls_free_button,value);
    GetPrivateProfileStringW(L"controls",L"free_look_key",L"",value,64,controls_ini);SetWindowTextW(controls_free_key,value);
    GetPrivateProfileStringW(L"controls",L"hud_button",L"back",value,64,controls_ini);controls_pick(controls_hud_button,value);
    GetPrivateProfileStringW(L"controls",L"hud_key",L"",value,64,controls_ini);SetWindowTextW(controls_hud_key,value);
    GetPrivateProfileStringW(L"controls",L"free_look_mouse",L"right",value,64,controls_ini);controls_pick(controls_mouse,value);
}
static BOOL controls_save_settings(void){
    path(controls_ini,L"client.ini");
    WCHAR fov[32],zoom[32],free_button[64],free_key[64],hud_button[64],hud_key[64],mouse[64];
    GetWindowTextW(controls_fov,fov,32);GetWindowTextW(controls_zoom,zoom,32);
    controls_text(controls_free_button,free_button,64);GetWindowTextW(controls_free_key,free_key,64);
    controls_text(controls_hud_button,hud_button,64);GetWindowTextW(controls_hud_key,hud_key,64);
    controls_text(controls_mouse,mouse,64);
    float fov_value=(float)_wtof(fov),zoom_value=(float)_wtof(zoom);
    if(fov_value<-100)fov_value=-100;if(fov_value>100)fov_value=100;
    if(zoom_value<0.25f)zoom_value=0.25f;if(zoom_value>4.0f)zoom_value=4.0f;
    _snwprintf(fov,32,L"%.2f",fov_value);_snwprintf(zoom,32,L"%.2f",zoom_value);
    BOOL ok=WritePrivateProfileStringW(L"camera",L"race_orbit",SendMessageW(controls_enable,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"free_fov_offset",fov,controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"free_zoom",zoom,controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"invert_x",SendMessageW(controls_invert_x,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"invert_y",SendMessageW(controls_invert_y,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"free_look_button",free_button,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"free_look_key",free_key,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"hud_button",hud_button,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"hud_key",hud_key,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"free_look_mouse",mouse,controls_ini);
    SetWindowTextW(status,ok?L"Controller and camera settings saved. Restart the game to apply changes.":L"Could not save all controller settings. Please try again.");
    return ok;
}
static void controls_create(void){
    controls_enable=control(L"BUTTON",L"Enable free-look camera",BS_AUTOCHECKBOX|WS_TABSTOP,22,26,590,28,0);
    control(L"STATIC",L"FOV offset (degrees)",0,22,62,150,24,0);
    controls_fov=control(L"EDIT",L"0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,58,90,28,0);
    control(L"STATIC",L"Distance multiplier",0,22,94,150,24,0);
    controls_zoom=control(L"EDIT",L"1.0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,90,90,28,0);
    controls_invert_x=control(L"BUTTON",L"Invert X",BS_AUTOCHECKBOX|WS_TABSTOP,22,126,280,28,0);
    controls_invert_y=control(L"BUTTON",L"Invert Y",BS_AUTOCHECKBOX|WS_TABSTOP,312,126,280,28,0);
    control(L"STATIC",L"Free-look button",0,22,158,150,24,0);
    controls_free_button=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,190,154,150,200,0);
    control(L"STATIC",L"Free-look key",0,360,158,120,24,0);
    controls_free_key=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,490,154,80,28,0);
    control(L"STATIC",L"HUD button",0,22,190,150,24,0);
    controls_hud_button=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,190,186,150,200,0);
    control(L"STATIC",L"HUD key",0,360,190,120,24,0);
    controls_hud_key=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,490,186,80,28,0);
    control(L"STATIC",L"Mouse look button",0,22,222,150,24,0);
    controls_mouse=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,190,218,150,160,0);
    controls_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,22,254,220,32,CONTROLS_SAVE);
    controls_first=controls_enable;
    graphics_choice(controls_free_button,L"right_thumb",0);graphics_choice(controls_free_button,L"left_thumb",0);
    graphics_choice(controls_free_button,L"back",0);graphics_choice(controls_free_button,L"start",0);
    graphics_choice(controls_free_button,L"left_shoulder",0);graphics_choice(controls_free_button,L"right_shoulder",0);
    graphics_choice(controls_free_button,L"a",0);graphics_choice(controls_free_button,L"b",0);
    graphics_choice(controls_free_button,L"x",0);graphics_choice(controls_free_button,L"y",0);
    graphics_choice(controls_free_button,L"dpad_up",0);graphics_choice(controls_free_button,L"dpad_down",0);
    graphics_choice(controls_free_button,L"dpad_left",0);graphics_choice(controls_free_button,L"dpad_right",0);
    graphics_choice(controls_free_button,L"none",0);
    for(int i=0;i<(int)SendMessageW(controls_free_button,CB_GETCOUNT,0,0);i++){
        WCHAR text[64];SendMessageW(controls_free_button,CB_GETLBTEXT,i,(LPARAM)text);controls_add(controls_hud_button,text);
    }
    graphics_choice(controls_mouse,L"right",0);graphics_choice(controls_mouse,L"left",0);
    graphics_choice(controls_mouse,L"middle",0);graphics_choice(controls_mouse,L"none",0);
    checkbox_help(controls_enable,L"Keep the normal chase camera and use the right stick/mouse to look around while free look is toggled on. Press R3 or the configured button/key to toggle. Restart the game after changes.");
    checkbox_help(controls_invert_x,L"Invert horizontal free-look input. 0 in client.ini is the preferred direction; this checkbox stores 1.");
    checkbox_help(controls_invert_y,L"Invert vertical free-look input. 0 in client.ini is the preferred direction; this checkbox stores 1.");
    controls_reload();
}
#endif
