#include "graphics_settings.h"
/* Launcher controls share the native game's existing client.ini settings. */
static HWND graphics_resolution,graphics_mode,graphics_aa,graphics_scale,graphics_recent,graphics_save;
static void graphics_choice(HWND box,const WCHAR *text,LPARAM value){
    LRESULT index=SendMessageW(box,CB_ADDSTRING,0,(LPARAM)text);
    SendMessageW(box,CB_SETITEMDATA,index,value);
}
static void graphics_select(HWND box,LPARAM value){
    int count=(int)SendMessageW(box,CB_GETCOUNT,0,0);
    for(int i=0;i<count;i++)if(SendMessageW(box,CB_GETITEMDATA,i,0)==value){SendMessageW(box,CB_SETCURSEL,i,0);return;}
    SendMessageW(box,CB_SETCURSEL,0,0);
}
static LPARAM graphics_value(HWND box){return SendMessageW(box,CB_GETITEMDATA,SendMessageW(box,CB_GETCURSEL,0,0),0);}
static BOOL graphics_bool(const WCHAR *key){WCHAR value[16];GetPrivateProfileStringW(L"graphics",key,L"false",value,16,ini);return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");}
static void graphics_reload(void){
    if(!graphics_resolution)return;
    UINT width=GetPrivateProfileIntW(L"graphics",L"render_width",0,ini),height=GetPrivateProfileIntW(L"graphics",L"render_height",0,ini);
    LPARAM resolution=(LPARAM)MAKELONG(width,height);
    graphics_select(graphics_resolution,resolution);
    if(width>=800&&height>=600&&graphics_value(graphics_resolution)!=resolution){WCHAR label[64];_snwprintf(label,64,L"%u × %u",width,height);graphics_choice(graphics_resolution,label,resolution);graphics_select(graphics_resolution,resolution);}
    graphics_select(graphics_mode,rr_fullscreen_mode(ini)+1);
    SendMessageW(graphics_aa,BM_SETCHECK,graphics_bool(L"antialiasing")?BST_CHECKED:BST_UNCHECKED,0);
    graphics_select(graphics_scale,GetPrivateProfileIntW(L"graphics",L"ui_scale",0,ini));
    graphics_select(graphics_recent,GetPrivateProfileIntW(L"graphics",L"recent_tracks_limit",0,ini));
}
static BOOL graphics_save_settings(void){
    LPARAM resolution=graphics_value(graphics_resolution),mode=graphics_value(graphics_mode),scale=graphics_value(graphics_scale),limit=graphics_value(graphics_recent);
    if(resolution==CB_ERR||mode==CB_ERR||scale==CB_ERR||limit==CB_ERR)return FALSE;
    WCHAR width[16],height[16],size[16],recent[16];
    _snwprintf(width,16,L"%u",LOWORD(resolution));_snwprintf(height,16,L"%u",HIWORD(resolution));
    _snwprintf(size,16,L"%ld",(long)scale);_snwprintf(recent,16,L"%ld",(long)limit);
    BOOL ok=WritePrivateProfileStringW(L"graphics",L"render_width",width,ini)
        &&WritePrivateProfileStringW(L"graphics",L"render_height",height,ini)
        &&rr_save_fullscreen_mode(ini,(int)mode-1)
        &&WritePrivateProfileStringW(L"graphics",L"antialiasing",SendMessageW(graphics_aa,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",ini)
        &&WritePrivateProfileStringW(L"graphics",L"ui_scale",size,ini)
        &&WritePrivateProfileStringW(L"graphics",L"recent_tracks_limit",recent,ini);
    SetWindowTextW(status,ok?L"Graphics settings saved. Restart the game to apply changes.":L"Could not save all graphics settings. Please try again.");return ok;
}
