#include "graphics_settings.h"
/* Graphics are client-side settings; keep them in the game's client.ini. */
static HWND graphics_resolution,graphics_mode,graphics_aa,graphics_scale,graphics_recent,graphics_save,graphics_culling,graphics_bloom,graphics_dof;
static const WCHAR *graphics_file(void){static WCHAR file[CAP];path(file,L"client.ini");return file;}
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
static BOOL graphics_read_bool(const WCHAR *key,const WCHAR *fallback){
    WCHAR value[16];GetPrivateProfileStringW(L"graphics",key,fallback,value,16,graphics_file());
    return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
}
static BOOL graphics_bool(const WCHAR *key){return graphics_read_bool(key,L"false");}
static BOOL graphics_bool_default(const WCHAR *key){return graphics_read_bool(key,L"true");}
static void graphics_reload(void){
    if(!graphics_resolution)return;
    const WCHAR *file=graphics_file();
    UINT width=GetPrivateProfileIntW(L"graphics",L"render_width",0,file),height=GetPrivateProfileIntW(L"graphics",L"render_height",0,file);
    LPARAM resolution=(LPARAM)MAKELONG(width,height);
    graphics_select(graphics_resolution,resolution);
    if(width>=800&&height>=600&&graphics_value(graphics_resolution)!=resolution){WCHAR label[64];_snwprintf(label,64,L"%u × %u",width,height);graphics_choice(graphics_resolution,label,resolution);graphics_select(graphics_resolution,resolution);}
    graphics_select(graphics_mode,rr_fullscreen_mode(file)+1);
    SendMessageW(graphics_aa,BM_SETCHECK,graphics_bool(L"antialiasing")?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(graphics_culling,BM_SETCHECK,graphics_bool(L"disable_culling")?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(graphics_bloom,BM_SETCHECK,graphics_bool_default(L"bloom")?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(graphics_dof,BM_SETCHECK,graphics_bool_default(L"depth_of_field")?BST_CHECKED:BST_UNCHECKED,0);
    graphics_select(graphics_scale,GetPrivateProfileIntW(L"graphics",L"ui_scale",0,file));
    graphics_select(graphics_recent,GetPrivateProfileIntW(L"graphics",L"recent_tracks_limit",0,file));
}
static BOOL graphics_save_settings(void){
    LPARAM resolution=graphics_value(graphics_resolution),mode=graphics_value(graphics_mode),scale=graphics_value(graphics_scale),limit=graphics_value(graphics_recent);
    if(resolution==CB_ERR||mode==CB_ERR||scale==CB_ERR||limit==CB_ERR)return FALSE;
    WCHAR width[16],height[16],size[16],recent[16];
    _snwprintf(width,16,L"%u",LOWORD(resolution));_snwprintf(height,16,L"%u",HIWORD(resolution));
    _snwprintf(size,16,L"%ld",(long)scale);_snwprintf(recent,16,L"%ld",(long)limit);
    const WCHAR *file=graphics_file();
    BOOL ok=WritePrivateProfileStringW(L"graphics",L"render_width",width,file)
        &&WritePrivateProfileStringW(L"graphics",L"render_height",height,file)
        &&rr_save_fullscreen_mode(file,(int)mode-1)
        &&WritePrivateProfileStringW(L"graphics",L"antialiasing",SendMessageW(graphics_aa,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",file)
        &&WritePrivateProfileStringW(L"graphics",L"disable_culling",SendMessageW(graphics_culling,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",file)
        &&WritePrivateProfileStringW(L"graphics",L"bloom",SendMessageW(graphics_bloom,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",file)
        &&WritePrivateProfileStringW(L"graphics",L"depth_of_field",SendMessageW(graphics_dof,BM_GETCHECK,0,0)==BST_CHECKED?L"true":L"false",file)
        &&WritePrivateProfileStringW(L"graphics",L"ui_scale",size,file)
        &&WritePrivateProfileStringW(L"graphics",L"recent_tracks_limit",recent,file);
    SetWindowTextW(status,ok?L"Graphics settings saved. Restart the game to apply changes.":L"Could not save all graphics settings. Please try again.");return ok;
}
