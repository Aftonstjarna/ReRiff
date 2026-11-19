/* Local removal regression only. The origin registry survives disabled mods.
 * Native settings setter is supplied by the hash-checked managed bridge.
 * No save layout, ownership, paint selection or shared replay is rewritten. */
typedef void (__cdecl *CarFallbackSetter)(void *,int);
static CarFallbackSetter car_fallback_setter;
__declspec(dllexport) int __cdecl ReRiffConfigureCarFallback(void *setter){
    if(!setter||car_fallback_setter)return 0;
    car_fallback_setter=(CarFallbackSetter)setter;return 1;
}
static BOOL car_fallback_name(void *manager,int index,char name[4]){
    BYTE *text=(BYTE *)manager+264+24*index;
    /* All supported stock origins have three-byte inline native IDs. */
    if(*(unsigned *)(text+16)!=3||*(unsigned *)(text+20)!=15)return FALSE;
    memcpy(name,text,3);name[3]=0;return TRUE;
}
static int car_fallback_target(void *manager,void *settings,const char *origin){
    static const char *stock[]={"alf","bui","cor","rs1","vau","fab","hon","cad","rol","mer","mot","pha","che","f1r","pl1"};
    if(!readable(manager,796)||!readable(settings,328)||!origin)return -1;
    int count=*(int *)((BYTE *)manager+792),selected=*(int *)((BYTE *)settings+324);
    /* Never reinterpret an active slot, or a missing/invalid stock slot. */
    if(count<15||count>22||selected<15||selected>=22||selected<count||!((BYTE *)settings)[5])return -1;
    BOOL known=FALSE;for(unsigned i=0;i<15;i++)if(!strcmp(origin,stock[i]))known=TRUE;
    if(!known)return -1;
    int found=-1;
    for(int i=0;i<count;i++){
        char name[4];if(car_fallback_name(manager,i,name)&&!strcmp(name,origin)){
            if(found>=0)return -1;found=i;
        }
    }
    return found;
}
static void car_fallback_main(void){
    if(!car_fallback_setter)return;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    void *manager=*(void **)(base+0x7f3d2c),*save=*(void **)(base+0x7ed7b8);
    if(!readable(save,20)||!readable(manager,796))return;
    void *settings=*(void **)((BYTE *)save+16);if(!readable(settings,328))return;
    int selected=*(int *)((BYTE *)settings+324),count=*(int *)((BYTE *)manager+792);
    if(selected<15||selected>=22||count<15||count>22||selected<count)return;
    WCHAR path[MAX_PATH],section[24],origin[16];
    const WCHAR *slash=wcsrchr(config_path,L'\\');if(!slash)return;
    size_t length=(size_t)(slash-config_path)+1;
    if(length+32>=MAX_PATH)return;
    memcpy(path,config_path,length*sizeof(WCHAR));wcscpy(path+length,L"car-origins.ini");
    _snwprintf(section,24,L"slot_%d",selected);
    GetPrivateProfileStringW(section,L"base",L"",origin,16,path);
    char id[4];if(wcslen(origin)!=3)return;
    for(unsigned i=0;i<3;i++){if(origin[i]>127)return;id[i]=(char)origin[i];}id[3]=0;
    int target=car_fallback_target(manager,settings,id);if(target<0)return;
    car_fallback_setter(settings,target);
    char message[160];snprintf(message,sizeof(message),"Car fallback: missing slot=%d selected stock base=%s slot=%d through original setter; paints and unlocks retained.",selected,id,target);
    log_line(message);
}
