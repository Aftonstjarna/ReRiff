#include "engine_menu.c"
#define CHECK(c) do {if(!(c)){printf("FAIL %d: %s (hr=%lx)\n",__LINE__,#c,(unsigned long)hr);return 1;}}while(0)
static HRESULT hr;
int main(int argc,char **argv){
    BOOL extended=argc>1&&!strcmp(argv[1],"--ex");
    HMODULE library=LoadLibraryW(L"d3d9.dll");CHECK(library);
    typedef IDirect3D9 *(WINAPI *Create9)(UINT);union{FARPROC address;Create9 function;} create;
    create.address=GetProcAddress(library,"Direct3DCreate9");CHECK(create.address);
    IDirect3D9 *api=NULL;IDirect3D9Ex *api_ex=NULL;
    if(extended){
        typedef HRESULT (WINAPI *Create9Ex)(UINT,IDirect3D9Ex **);
        union{FARPROC address;Create9Ex function;} create_ex;
        create_ex.address=GetProcAddress(library,"Direct3DCreate9Ex");CHECK(create_ex.address);
        CHECK(SUCCEEDED(hr=create_ex.function(D3D_SDK_VERSION,&api_ex)));api=(IDirect3D9 *)api_ex;
    }else api=create.function(D3D_SDK_VERSION);
    CHECK(api);
    HWND host=CreateWindowW(L"STATIC",L"AA renderer fixture",WS_OVERLAPPEDWINDOW,0,0,320,240,NULL,NULL,GetModuleHandleW(NULL),NULL);CHECK(host);
    ShowWindow(host,SW_SHOW);
    D3DPRESENT_PARAMETERS params={0};params.BackBufferWidth=64;params.BackBufferHeight=64;params.BackBufferFormat=D3DFMT_A8R8G8B8;
    params.BackBufferCount=1;params.SwapEffect=D3DSWAPEFFECT_DISCARD;params.hDeviceWindow=host;params.Windowed=TRUE;
    params.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    IDirect3DDevice9 *device=NULL;IDirect3DDevice9Ex *device_ex=NULL;
    if(extended){hr=IDirect3D9Ex_CreateDeviceEx(api_ex,D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,host,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&params,NULL,&device_ex);device=(IDirect3DDevice9 *)device_ex;}
    else hr=IDirect3D9_CreateDevice(api,D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,host,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&params,&device);
    CHECK(SUCCEEDED(hr));
    BYTE renderer[0x80]={0};
    /* The separate backend is non-polymorphic; reject an absent device. */
    CHECK(!aa_attach_renderer(renderer));
    *(void **)(renderer+0x78)=device;*(void **)(renderer+0x7c)=device_ex;
    const IDirect3DDevice9Vtbl *original_table=device->lpVtbl;
    void *unmodified_method=(void *)original_table->TestCooperativeLevel;
    CHECK(aa_attach_renderer(renderer)&&aa_device==device);
    CHECK(device->lpVtbl==original_table);
    CHECK((void *)device->lpVtbl->TestCooperativeLevel==unmodified_method);
    CHECK(device->lpVtbl->Present==aa_present&&device->lpVtbl->Reset==aa_reset);
    CHECK(aa_attach_renderer(renderer));
    if(extended){CHECK(device_ex->lpVtbl->PresentEx==aa_present_ex&&device_ex->lpVtbl->ResetEx==aa_reset_ex);}
    IDirect3DSurface9 *back=NULL,*readback=NULL;
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_GetBackBuffer(device,0,0,D3DBACKBUFFER_TYPE_MONO,&back)));
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_CreateOffscreenPlainSurface(device,64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,NULL)));
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_Clear(device,0,NULL,D3DCLEAR_TARGET,0xff000000,1,0)));
    for(int y=0;y<64;y++){
        D3DRECT rect={0,y,y/2+1,y+1};CHECK(SUCCEEDED(hr=IDirect3DDevice9_Clear(device,1,&rect,D3DCLEAR_TARGET,0xffffffff,1,0)));
    }
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_GetRenderTargetData(device,back,readback)));
    D3DLOCKED_RECT pixels;DWORD before[4096];
    CHECK(SUCCEEDED(hr=IDirect3DSurface9_LockRect(readback,&pixels,NULL,D3DLOCK_READONLY)));
    for(int y=0;y<64;y++)memcpy(before+y*64,(BYTE *)pixels.pBits+y*pixels.Pitch,64*4);
    IDirect3DSurface9_UnlockRect(readback);
    /* A nondefault viewport/scissor/blend state must survive the AA pass. */
    D3DVIEWPORT9 viewport={2,3,40,41,0.2f,0.8f};RECT clip={4,5,30,31};
    IDirect3DDevice9_SetViewport(device,&viewport);IDirect3DDevice9_SetScissorRect(device,&clip);
    IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);IDirect3DDevice9_SetRenderState(device,D3DRS_SCISSORTESTENABLE,TRUE);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_TEXCOORDINDEX,1);
    aa_requested=1;aa_fault=0;aa_draw(device);CHECK(!aa_fault&&aa_texture&&aa_shader);
    D3DVIEWPORT9 actual;DWORD blend=0,scissor=0;RECT actual_clip;
    IDirect3DDevice9_GetViewport(device,&actual);CHECK(!memcmp(&actual,&viewport,sizeof(actual)));
    IDirect3DDevice9_GetRenderState(device,D3DRS_ALPHABLENDENABLE,&blend);IDirect3DDevice9_GetRenderState(device,D3DRS_SCISSORTESTENABLE,&scissor);
    DWORD coord=0;IDirect3DDevice9_GetTextureStageState(device,0,D3DTSS_TEXCOORDINDEX,&coord);CHECK(coord==1);
    IDirect3DDevice9_GetScissorRect(device,&actual_clip);CHECK(blend&&scissor&&!memcmp(&clip,&actual_clip,sizeof(clip)));
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_GetRenderTargetData(device,back,readback)));
    CHECK(SUCCEEDED(hr=IDirect3DSurface9_LockRect(readback,&pixels,NULL,D3DLOCK_READONLY)));
    int changed=0,smoothed=0;
    for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        DWORD pixel=*(DWORD *)((BYTE *)pixels.pBits+y*pixels.Pitch+x*4);
        if(pixel!=before[y*64+x])changed++;
        if((pixel&255)>0&&(pixel&255)<255)smoothed++;
    }
    IDirect3DSurface9_UnlockRect(readback);CHECK(changed>10&&smoothed>10);
    IDirect3DSurface9_Release(back);IDirect3DSurface9_Release(readback);
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_Reset(device,&params)));CHECK(!aa_texture&&!aa_surface&&!aa_shader);
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_Present(device,NULL,NULL,NULL,NULL)));
    CHECK(aa_texture&&aa_shader&&!aa_fault);
    if(extended){
        CHECK(SUCCEEDED(hr=IDirect3DDevice9Ex_ResetEx(device_ex,&params,NULL)));CHECK(!aa_texture&&!aa_shader);
        CHECK(SUCCEEDED(hr=IDirect3DDevice9Ex_PresentEx(device_ex,NULL,NULL,NULL,NULL,0)));
        CHECK(aa_texture&&aa_shader&&!aa_fault);
    }
    IDirect3DTexture9 *primary_texture=aa_texture;
    IDirect3DSurface9 *primary_surface=aa_surface;
    /* The native table may be shared: reset/present on another device must
     * forward unchanged and must not release the primary device's AA resources. */
    IDirect3DDevice9 *other=NULL;
    CHECK(SUCCEEDED(hr=IDirect3D9_CreateDevice(api,D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,host,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&params,&other)));
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_Reset(other,&params)));
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_Present(other,NULL,NULL,NULL,NULL)));
    CHECK(aa_device==device&&aa_texture==primary_texture&&aa_surface==primary_surface);
    IDirect3DDevice9_Release(other);
    aa_requested=0;aa_draw(device);CHECK(!aa_texture&&!aa_surface&&!aa_shader);
    CHECK(SUCCEEDED(hr=IDirect3DDevice9_Reset(device,&params)));
    IDirect3DDevice9_Release(device);IDirect3D9_Release(api);DestroyWindow(host);
    printf("PASS: separate render backend attachment; %d changed pixels, %d smoothed edge pixels; viewport/scissor/blend restored; hooked reset and Off release resources\n",changed,smoothed);return 0;
}
