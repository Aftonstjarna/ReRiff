/* Experimental edge-directed post-process AA on the completed D3D9 image.
 * All GPU work runs on the original Present/Reset thread. */
#define COBJMACROS
#include <d3d9.h>
typedef struct AABlob AABlob;
typedef struct {void *query,*add;ULONG (WINAPI *release)(AABlob *);void *(WINAPI *data)(AABlob *);DWORD (WINAPI *size)(AABlob *);} AABlobMethods;
struct AABlob {AABlobMethods *methods;};
typedef HRESULT (WINAPI *CompileAA)(const char *,UINT,void *,void *,const char *,const char *,DWORD,AABlob **,AABlob **,void **);
static volatile LONG aa_requested,aa_fault;
static IDirect3DDevice9 *aa_device;
static IDirect3DTexture9 *aa_texture;
static IDirect3DSurface9 *aa_surface;
static IDirect3DPixelShader9 *aa_shader;
static AABlob *aa_code;
static UINT aa_width,aa_height;
static HRESULT (WINAPI *aa_present_original)(IDirect3DDevice9 *,const RECT *,const RECT *,HWND,const RGNDATA *);
static HRESULT (WINAPI *aa_reset_original)(IDirect3DDevice9 *,D3DPRESENT_PARAMETERS *);
static HRESULT (WINAPI *aa_present_ex_original)(IDirect3DDevice9Ex *,const RECT *,const RECT *,HWND,const RGNDATA *,DWORD);
static HRESULT (WINAPI *aa_reset_ex_original)(IDirect3DDevice9Ex *,D3DPRESENT_PARAMETERS *,D3DDISPLAYMODEEX *);
static const char aa_source[]=
"sampler2D image:register(s0);float4 pixel:register(c0);"
"float4 main(float2 uv:TEXCOORD0):COLOR0{"
"float4 center=tex2D(image,uv);float3 l=float3(.299,.587,.114);"
"float nw=dot(tex2D(image,uv+float2(-1,-1)*pixel.xy).rgb,l);"
"float ne=dot(tex2D(image,uv+float2(1,-1)*pixel.xy).rgb,l);"
"float sw=dot(tex2D(image,uv+float2(-1,1)*pixel.xy).rgb,l);"
"float se=dot(tex2D(image,uv+float2(1,1)*pixel.xy).rgb,l);float m=dot(center.rgb,l);"
"float low=min(m,min(min(nw,ne),min(sw,se))),high=max(m,max(max(nw,ne),max(sw,se)));"
"if(high-low<max(.03125,high*.125))return center;"
"float2 d=float2(sw+se-nw-ne,nw+sw-ne-se);"
"float reduce=max((nw+ne+sw+se)*.03125,.0078125);"
"d=clamp(d/(min(abs(d.x),abs(d.y))+reduce),-8,8)*pixel.xy;"
"float3 a=.5*(tex2D(image,uv-d/6).rgb+tex2D(image,uv+d/6).rgb);"
"float3 b=.5*a+.25*(tex2D(image,uv-d*.5).rgb+tex2D(image,uv+d*.5).rgb);"
"float test=dot(b,l);return float4(test<low||test>high?a:b,center.a);}";
static void aa_release_resources(void){
    if(aa_surface)IDirect3DSurface9_Release(aa_surface);aa_surface=NULL;
    if(aa_texture)IDirect3DTexture9_Release(aa_texture);aa_texture=NULL;
    if(aa_shader)IDirect3DPixelShader9_Release(aa_shader);aa_shader=NULL;aa_width=aa_height=0;
}
static void aa_fail(const char *reason){
    if(!InterlockedExchange(&aa_fault,1))log_line(reason);
    InterlockedExchange(&aa_requested,0);
}
static BOOL aa_resources(IDirect3DDevice9 *device,const D3DSURFACE_DESC *desc){
    if(aa_texture&&aa_width==desc->Width&&aa_height==desc->Height)return TRUE;
    aa_release_resources();
    D3DCAPS9 caps;
    if(FAILED(IDirect3DDevice9_GetDeviceCaps(device,&caps))||caps.PixelShaderVersion<D3DPS_VERSION(3,0)){aa_fail("Antialiasing unavailable: shader model 3 required.");return FALSE;}
    if(!aa_code){
        union {FARPROC address;CompileAA function;} compiler;
        HMODULE library=GetModuleHandleW(L"d3dx9_43.dll");if(!library)library=LoadLibraryW(L"d3dx9_43.dll");
        compiler.address=library?GetProcAddress(library,"D3DXCompileShader"):NULL;
        AABlob *error=NULL;
        HRESULT result=compiler.address?compiler.function(aa_source,(UINT)strlen(aa_source),NULL,NULL,"main","ps_3_0",0,&aa_code,&error,NULL):E_FAIL;
        if(error)error->methods->release(error);
        if(FAILED(result)||!aa_code){aa_fail("Antialiasing unavailable: post-process shader compilation failed.");return FALSE;}
    }
    if(FAILED(IDirect3DDevice9_CreatePixelShader(device,(const DWORD *)aa_code->methods->data(aa_code),&aa_shader))
        ||FAILED(IDirect3DDevice9_CreateTexture(device,desc->Width,desc->Height,1,D3DUSAGE_RENDERTARGET,desc->Format,D3DPOOL_DEFAULT,&aa_texture,NULL))
        ||FAILED(IDirect3DTexture9_GetSurfaceLevel(aa_texture,0,&aa_surface))){aa_release_resources();aa_fail("Antialiasing unavailable: could not allocate image resources.");return FALSE;}
    aa_width=desc->Width;aa_height=desc->Height;log_line("Post-process antialiasing image resources ready.");return TRUE;
}
static void aa_draw(IDirect3DDevice9 *device){
    if(!InterlockedCompareExchange(&aa_requested,0,0)){if(aa_texture)aa_release_resources();return;}
    IDirect3DSurface9 *back=NULL,*targets[4]={0},*depth=NULL;IDirect3DStateBlock9 *state=NULL;
    D3DSURFACE_DESC desc;D3DVIEWPORT9 viewport;D3DCAPS9 caps;BOOL scene=FALSE;
    if(FAILED(IDirect3DDevice9_GetBackBuffer(device,0,0,D3DBACKBUFFER_TYPE_MONO,&back)))return;
    if(FAILED(IDirect3DSurface9_GetDesc(back,&desc))||!aa_resources(device,&desc))goto done;
    HRESULT copied=IDirect3DDevice9_StretchRect(device,back,NULL,aa_surface,NULL,D3DTEXF_NONE);
    if(FAILED(copied)){if(copied!=D3DERR_DEVICELOST)aa_fail("Antialiasing unavailable: completed-image copy is unsupported.");goto done;}
    if(FAILED(IDirect3DDevice9_GetDeviceCaps(device,&caps))||FAILED(IDirect3DDevice9_GetViewport(device,&viewport))
        ||FAILED(IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&state))){aa_fail("Antialiasing unavailable: graphics state could not be saved.");goto done;}
    UINT count=caps.NumSimultaneousRTs;if(count>4)count=4;
    for(UINT i=0;i<count;i++)IDirect3DDevice9_GetRenderTarget(device,i,&targets[i]);
    if(!targets[0]){for(UINT i=1;i<count;i++)if(targets[i])IDirect3DSurface9_Release(targets[i]);goto done;}
    IDirect3DDevice9_GetDepthStencilSurface(device,&depth);
    IDirect3DDevice9_SetDepthStencilSurface(device,NULL);
    IDirect3DDevice9_SetRenderTarget(device,0,back);
    for(UINT i=1;i<count;i++)IDirect3DDevice9_SetRenderTarget(device,i,NULL);
    D3DVIEWPORT9 full={0,0,desc.Width,desc.Height,0,1};IDirect3DDevice9_SetViewport(device,&full);
    const D3DRENDERSTATETYPE disabled[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_FOGENABLE,D3DRS_LIGHTING,D3DRS_SCISSORTESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_CLIPPLANEENABLE};
    for(unsigned i=0;i<sizeof(disabled)/sizeof(disabled[0]);i++)IDirect3DDevice9_SetRenderState(device,disabled[i],FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);IDirect3DDevice9_SetRenderState(device,D3DRS_FILLMODE,D3DFILL_SOLID);
    IDirect3DDevice9_SetRenderState(device,D3DRS_COLORWRITEENABLE,15);
    IDirect3DDevice9_SetVertexShader(device,NULL);IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_TEX1);
    IDirect3DDevice9_SetStreamSourceFreq(device,0,1);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_TEXCOORDINDEX,0);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    IDirect3DDevice9_SetPixelShader(device,aa_shader);IDirect3DDevice9_SetTexture(device,0,(IDirect3DBaseTexture9 *)aa_texture);
    IDirect3DDevice9_SetSamplerState(device,0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);IDirect3DDevice9_SetSamplerState(device,0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    IDirect3DDevice9_SetSamplerState(device,0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);IDirect3DDevice9_SetSamplerState(device,0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    IDirect3DDevice9_SetSamplerState(device,0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);IDirect3DDevice9_SetSamplerState(device,0,D3DSAMP_SRGBTEXTURE,FALSE);
    float pixel[4]={1.0f/desc.Width,1.0f/desc.Height,0,0};IDirect3DDevice9_SetPixelShaderConstantF(device,0,pixel,1);
    struct {float x,y,z,w,u,v;} vertices[4]={{-.5f,-.5f,0,1,0,0},{desc.Width-.5f,-.5f,0,1,1,0},{-.5f,desc.Height-.5f,0,1,0,1},{desc.Width-.5f,desc.Height-.5f,0,1,1,1}};
    if(SUCCEEDED(IDirect3DDevice9_BeginScene(device))){scene=TRUE;if(FAILED(IDirect3DDevice9_DrawPrimitiveUP(device,D3DPT_TRIANGLESTRIP,2,vertices,sizeof(vertices[0]))))aa_fail("Antialiasing draw failed; original presentation retained.");}
    if(scene)IDirect3DDevice9_EndScene(device);
    for(UINT i=0;i<count;i++)IDirect3DDevice9_SetRenderTarget(device,i,targets[i]);
    IDirect3DDevice9_SetDepthStencilSurface(device,depth);IDirect3DStateBlock9_Apply(state);IDirect3DDevice9_SetViewport(device,&viewport);
    for(UINT i=0;i<count;i++)if(targets[i])IDirect3DSurface9_Release(targets[i]);
    if(depth)IDirect3DSurface9_Release(depth);
 done:
    if(state)IDirect3DStateBlock9_Release(state);IDirect3DSurface9_Release(back);
}
static BOOL (*aa_skip_background_present)(void);
static HRESULT WINAPI aa_present(IDirect3DDevice9 *self,const RECT *a,const RECT *b,HWND window,const RGNDATA *dirty){if(self!=aa_device)return aa_present_original(self,a,b,window,dirty);if(aa_skip_background_present&&aa_skip_background_present())return D3D_OK;aa_draw(self);return aa_present_original(self,a,b,window,dirty);}
static HRESULT WINAPI aa_reset(IDirect3DDevice9 *self,D3DPRESENT_PARAMETERS *params){
    if(self!=aa_device)return aa_reset_original(self,params);
    char trace[192];snprintf(trace,sizeof(trace),"D3D9 Reset: %ux%u windowed=%d refresh=%u thread=%lu owner=%lu.",params->BackBufferWidth,params->BackBufferHeight,params->Windowed,params->FullScreen_RefreshRateInHz,(unsigned long)GetCurrentThreadId(),(unsigned long)GetWindowThreadProcessId(params->hDeviceWindow,NULL));log_line(trace);
    log_line("D3D9 Reset: releasing AA resources.");aa_release_resources();log_line("D3D9 Reset: calling original device.");
    HRESULT result=aa_reset_original(self,params);log_line(SUCCEEDED(result)?"D3D9 Reset: original device succeeded.":"D3D9 Reset: original device failed.");return result;
}
static HRESULT WINAPI aa_present_ex(IDirect3DDevice9Ex *self,const RECT *a,const RECT *b,HWND window,const RGNDATA *dirty,DWORD flags){if((IDirect3DDevice9 *)self!=aa_device)return aa_present_ex_original(self,a,b,window,dirty,flags);if(aa_skip_background_present&&aa_skip_background_present())return D3D_OK;aa_draw((IDirect3DDevice9 *)self);return aa_present_ex_original(self,a,b,window,dirty,flags);}
static HRESULT WINAPI aa_reset_ex(IDirect3DDevice9Ex *self,D3DPRESENT_PARAMETERS *params,D3DDISPLAYMODEEX *mode){
    if((IDirect3DDevice9 *)self!=aa_device)return aa_reset_ex_original(self,params,mode);
    log_line("D3D9 ResetEx: releasing AA resources.");aa_release_resources();log_line("D3D9 ResetEx: calling original device.");
    HRESULT result=aa_reset_ex_original(self,params,mode);log_line(SUCCEEDED(result)?"D3D9 ResetEx: original device succeeded.":"D3D9 ResetEx: original device failed.");return result;
}
static BOOL aa_attach_problem(const char *stage,const void *pointer){
    char text[256];snprintf(text,sizeof(text),"AA attachment failed: %s (address=%p).",stage,pointer);log_line(text);return FALSE;
}
static BOOL validate_aa_backend(BYTE *base,uintptr_t address){
    /* Original lazy construction stores the separate render-thread backend
     * in 0xBFCC44. The public wrapper's PC interface forwards to this global;
     * it does not embed the backend. Validate both original references. */
    static const BYTE create[]={0xe8,0x81,0x01,0xed,0xff,0xa3};
    return readable(base+0x1cfbaa,10)&&!memcmp(base+0x1cfbaa,create,sizeof(create))
        &&*(DWORD *)(base+0x1cfbb0)==address+0x7fcc44
        &&readable(base+0x97020,5)&&base[0x97020]==0xa1
        &&*(DWORD *)(base+0x97021)==address+0x7fcc44;
}
static BOOL aa_attach_renderer(void *renderer){
    /* Native backend created at VA 0x49FD30; +0x78/+0x7C are its COM
     * device/Ex pointers. This object has no C++ vtable at offset zero. */
    if(!readable(renderer,0x80))return aa_attach_problem("render backend is unreadable",renderer);
    BYTE *pc=(BYTE *)renderer;
    IDirect3DDevice9 *device=*(IDirect3DDevice9 **)(pc+0x78);
    if(!readable(device,4))return aa_attach_problem("D3D9 device is unreadable",device);
    if(aa_device)return aa_device==device?TRUE:aa_attach_problem("D3D9 device was replaced",device);
    BOOL extended=*(void **)(pc+0x7c)!=NULL;unsigned entries=extended?134:119;
    void **table=*(void ***)device;if(!readable(table,entries*sizeof(void *)))return aa_attach_problem("COM method table is unreadable",table);
    HMODULE owner=NULL;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(const WCHAR *)table[0],&owner)||owner!=GetModuleHandleW(L"d3d9.dll"))return aa_attach_problem("COM QueryInterface is outside d3d9.dll",table[0]);
    /* Preserve the runtime-owned method table and its complete layout. A
     * replacement containing only public COM entries need not preserve private
     * runtime dispatch entries used internally during Reset/Present. Extend only
     * the original public slots and forward other devices without AA work. */
    MEMORY_BASIC_INFORMATION memory;DWORD protection;
    if(!VirtualQuery(table,&memory,sizeof(memory)))return aa_attach_problem("COM method protection is unavailable",table);
    DWORD writable=(memory.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))?PAGE_EXECUTE_READWRITE:PAGE_READWRITE;
    if(!VirtualProtect(table,entries*sizeof(void *),writable,&protection))return aa_attach_problem("COM methods are not writable",table);
    aa_present_original=(void *)table[17];aa_reset_original=(void *)table[16];
    if(extended){aa_present_ex_original=(void *)table[121];aa_reset_ex_original=(void *)table[132];}
    aa_device=device;
    InterlockedExchangePointer(table+17,(void *)aa_present);InterlockedExchangePointer(table+16,(void *)aa_reset);
    if(extended){InterlockedExchangePointer(table+121,(void *)aa_present_ex);InterlockedExchangePointer(table+132,(void *)aa_reset_ex);}
    DWORD ignored;VirtualProtect(table,entries*sizeof(void *),protection,&ignored);
    log_line("Post-process AA extends native D3D9 method slots; original device table preserved.");return TRUE;
}
static BOOL aa_attach(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void **renderer=(void **)(base+0x7fcc44);
    if(!validate_aa_backend(base,(uintptr_t)base))return aa_attach_problem("original backend references differ",base);
    return readable(renderer,4)?aa_attach_renderer(*renderer):aa_attach_problem("backend singleton is unreadable",renderer);
}
