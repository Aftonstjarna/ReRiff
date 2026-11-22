/* Global depth-of-field and bloom toggles.
 *
 * The game always enables both effects. They are drawn through the native
 * elGraphicsDevice vtable, so the toggle replaces those two entries and
 * forwards to the originals. Settings are shared with the launcher through
 * [graphics] bloom and depth_of_field in client.ini.
 */
typedef struct { float r,g,b,a; } PostFxColour;
typedef void (__thiscall *PostFxDofFn)(void *,BYTE,float,float,float,float,float,float,int);
typedef void (__thiscall *PostFxBloomFn)(void *,BYTE,PostFxColour,float,int);

static PostFxDofFn postfx_dof_original;
static PostFxBloomFn postfx_bloom_original;
static BOOL postfx_installed,postfx_loaded,postfx_bloom_enabled=TRUE,postfx_dof_enabled=TRUE;
static BOOL postfx_hooks_wanted=TRUE;

static BOOL postfx_bool(const WCHAR *key){
    WCHAR client[MAX_PATH],value[16];
    wcsncpy(client,config_path,MAX_PATH-1);client[MAX_PATH-1]=0;
    WCHAR *slash=wcsrchr(client,L'\\');if(slash)wcscpy(slash+1,L"client.ini");
    GetPrivateProfileStringW(L"graphics",key,L"true",value,16,client);
    return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
}

static void postfx_load(void){
    if(postfx_loaded)return;
    postfx_loaded=TRUE;
    postfx_bloom_enabled=postfx_bool(L"bloom");
    postfx_dof_enabled=postfx_bool(L"depth_of_field");
}

static void __thiscall postfx_dof_hook(void *device,BYTE enable,float focal,float focus_near,float focus_far,float a,float b,float c,int view){
    if(!postfx_dof_enabled){enable=0;focal=focus_near=focus_far=a=b=c=0.0f;}
    if(postfx_dof_original)postfx_dof_original(device,enable,focal,focus_near,focus_far,a,b,c,view);
}

static void __thiscall postfx_bloom_hook(void *device,BYTE enable,PostFxColour colour,float intensity,int view){
    if(!postfx_bloom_enabled){enable=0;intensity=0.0f;colour.r=colour.g=colour.b=colour.a=0.0f;}
    if(postfx_bloom_original)postfx_bloom_original(device,enable,colour,intensity,view);
}

static BOOL postfx_install(BYTE *base){
    postfx_load();
    if(postfx_installed)return TRUE;
    if(!readable(base+0x80d408,4))return FALSE;
    void *device=*(void **)(base+0x80d408);
    if(!readable(device,4))return FALSE;
    void **vtable=*(void ***)device;
    if(!readable(vtable,284))return FALSE;
    PostFxDofFn dof=(PostFxDofFn)vtable[58];
    PostFxBloomFn bloom=(PostFxBloomFn)vtable[70];
    if(!readable((void *)dof,8)||!readable((void *)bloom,8))return FALSE;
    DWORD old,ignored;
    if(!VirtualProtect(vtable,284,PAGE_READWRITE,&old))return FALSE;
    postfx_dof_original=dof;postfx_bloom_original=bloom;
    vtable[58]=(void *)postfx_dof_hook;vtable[70]=(void *)postfx_bloom_hook;
    VirtualProtect(vtable,284,old,&ignored);
    postfx_installed=TRUE;
    char detail[160];snprintf(detail,sizeof(detail),"Post-effect toggles ready; dof=%d bloom=%d vtable=%p.",postfx_dof_enabled?1:0,postfx_bloom_enabled?1:0,vtable);log_line(detail);
    return TRUE;
}

static void postfx_frame(void){
    if(postfx_installed)return;
    postfx_install((BYTE *)GetModuleHandleW(NULL));
}

static void postfx_set_dof(BOOL enabled){postfx_dof_enabled=enabled;}
static void postfx_set_bloom(BOOL enabled){postfx_bloom_enabled=enabled;}

