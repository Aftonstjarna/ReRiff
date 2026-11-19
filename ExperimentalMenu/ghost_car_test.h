/* Opt-in local-device test. Native serializers and replay payload stay intact. */
typedef int (__cdecl *GhostCarProcess)(const WCHAR *,const char *,int,int,void *);
typedef struct { DWORD car_offset,pack_offset;char car[4];int pack; } GhostCarPatch;
static GhostCarProcess ghost_car_process;
static GhostCarPatch ghost_car_patch;
static BOOL ghost_car_target,ghost_car_patch_ready,ghost_car_write_failed;
static DWORD ghost_car_mode;
static WCHAR ghost_car_root[MAX_PATH];
static char ghost_car_name[96];
__declspec(dllexport) int __cdecl ReRiffConfigureGhostCarTest(void *callback){
    if(!callback||ghost_car_process)return 0;ghost_car_process=(GhostCarProcess)callback;return 1;
}
static int ghost_car_installed(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void *manager=*(void **)(base+0x7f3d2c);
    if(!readable(manager,796)||*(int *)((BYTE *)manager+792)!=16)return 0;
    char id[4];if(!car_fallback_name(manager,15,id)||strcmp(id,"dod"))return 0;
    BYTE *v=(BYTE *)manager+15*12;DWORD start=*(DWORD *)v,end=*(DWORD *)(v+4);
    return end>=start&&end-start==2*176;
}
static int (*ghost_car_has_mod)(void)=ghost_car_installed;
static BOOL ghost_car_filename(const char *name){
    if(!name)return FALSE;size_t n=strlen(name);if(n<8||n>=sizeof(ghost_car_name)||strcmp(name+n-6,".ghost"))return FALSE;
    for(size_t i=0;i<n-6;i++)if(!((name[i]>='a'&&name[i]<='z')||(name[i]>='A'&&name[i]<='Z')||(name[i]>='0'&&name[i]<='9')||name[i]=='-'))return FALSE;
    return TRUE;
}
static void ghost_car_before_open(void *self,const char *name,DWORD mode){
    ghost_car_target=ghost_car_patch_ready=ghost_car_write_failed=FALSE;ghost_car_mode=mode;
    if(!ghost_car_process||!ghost_car_filename(name)||!progression_domain_root(self,ghost_car_root))return;
    strcpy(ghost_car_name,name);ghost_car_target=TRUE;
    memset(&ghost_car_patch,0,sizeof(ghost_car_patch));
    if(mode==1&&ghost_car_process(ghost_car_root,ghost_car_name,0,ghost_car_has_mod(),&ghost_car_patch)==1
        &&ghost_car_patch.car_offset>=16&&ghost_car_patch.car_offset<1024&&ghost_car_patch.pack_offset==ghost_car_patch.car_offset+3
        &&!memcmp(ghost_car_patch.car,"dod",4)&&ghost_car_patch.pack>=0&&ghost_car_patch.pack<2)ghost_car_patch_ready=TRUE;
}
static void ghost_car_overlay(DWORD offset,BYTE *buffer,DWORD size,const GhostCarPatch *patch){
    for(unsigned i=0;i<3;i++){DWORD at=patch->car_offset+i;if(at>=offset&&at-offset<size)buffer[at-offset]=(BYTE)patch->car[i];}
    for(unsigned i=0;i<4;i++){DWORD at=patch->pack_offset+i;if(at>=offset&&at-offset<size)buffer[at-offset]=((const BYTE *)&patch->pack)[i];}
}
static void ghost_car_after_close(BYTE result){
    BOOL target=ghost_car_target;ghost_car_target=ghost_car_patch_ready=FALSE;
    if(target&&result&&!ghost_car_write_failed&&ghost_car_process&&(ghost_car_mode==2||ghost_car_mode==3))
        ghost_car_process(ghost_car_root,ghost_car_name,1,ghost_car_has_mod(),NULL);
}
