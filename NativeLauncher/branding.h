#ifndef RERIFF_BRANDING_H
#define RERIFF_BRANDING_H
#define RERIFF_ICON 101
#define RERIFF_LOGO 102
#define RERIFF_BUTTON_LOGO 103
#define RERIFF_BRAND_CONTROL 1099
static HBITMAP rr_brand_outline_bitmap,rr_brand_button_bitmap;
static void rr_brand_icon(HWND window){
    HINSTANCE module=GetModuleHandleW(NULL);
    HICON large=(HICON)LoadImageW(module,MAKEINTRESOURCEW(RERIFF_ICON),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);
    HICON small=(HICON)LoadImageW(module,MAKEINTRESOURCEW(RERIFF_ICON),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
    if(large)SendMessageW(window,WM_SETICON,ICON_BIG,(LPARAM)large);
    if(small)SendMessageW(window,WM_SETICON,ICON_SMALL,(LPARAM)small);
}
static BOOL rr_brand_draw(const DRAWITEMSTRUCT *draw){
    if(!draw||draw->CtlID!=RERIFF_BRAND_CONTROL)return FALSE;
    HBRUSH background=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    FillRect(draw->hDC,&draw->rcItem,background);DeleteObject(background);
    HBITMAP *asset=&rr_brand_outline_bitmap;
    if(!*asset)*asset=LoadImageW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(RERIFF_BUTTON_LOGO),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
    if(*asset){
        BITMAP bitmap;GetObjectW(*asset,sizeof(bitmap),&bitmap);HDC memory=CreateCompatibleDC(draw->hDC);
        HGDIOBJ old=SelectObject(memory,*asset);
        int width=draw->rcItem.right-draw->rcItem.left,height=draw->rcItem.bottom-draw->rcItem.top;
        if(width*bitmap.bmHeight>height*bitmap.bmWidth)width=height*bitmap.bmWidth/bitmap.bmHeight;else height=width*bitmap.bmHeight/bitmap.bmWidth;
        typedef BOOL (WINAPI *BlendFn)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
        static HMODULE library;static BlendFn blend;if(!library){library=LoadLibraryW(L"msimg32.dll");if(library)blend=(BlendFn)GetProcAddress(library,"AlphaBlend");}
        BLENDFUNCTION mode={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
        if(blend)blend(draw->hDC,draw->rcItem.left,draw->rcItem.top+(draw->rcItem.bottom-draw->rcItem.top-height)/2,width,height,memory,0,0,bitmap.bmWidth,bitmap.bmHeight,mode);
        SelectObject(memory,old);DeleteDC(memory);
    }return TRUE;
}
/* A standard bitmap pushbutton retains native hover, focus and activation. */
static inline void rr_brand_button(HWND button){
    RECT bounds;if(!button||!GetClientRect(button,&bounds))return;
    int width=bounds.right-12,height=bounds.bottom-10;if(width<=0||height<=0)return;
    HDC screen=GetDC(button),memory=CreateCompatibleDC(screen);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,width,height);
    if(memory&&bitmap){
        HGDIOBJ previous=SelectObject(memory,bitmap);
        DRAWITEMSTRUCT draw={0};draw.CtlType=ODT_BUTTON;draw.CtlID=RERIFF_BRAND_CONTROL;draw.hDC=memory;draw.rcItem.right=width;draw.rcItem.bottom=height;
        rr_brand_draw(&draw);SelectObject(memory,previous);
        SendMessageW(button,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)bitmap);
        if(rr_brand_button_bitmap)DeleteObject(rr_brand_button_bitmap);rr_brand_button_bitmap=bitmap;
    }else if(bitmap)DeleteObject(bitmap);
    if(memory)DeleteDC(memory);ReleaseDC(button,screen);
}
static void rr_brand_cleanup(void){
    if(rr_brand_outline_bitmap){DeleteObject(rr_brand_outline_bitmap);rr_brand_outline_bitmap=NULL;}
    if(rr_brand_button_bitmap){DeleteObject(rr_brand_button_bitmap);rr_brand_button_bitmap=NULL;}
}
#endif
