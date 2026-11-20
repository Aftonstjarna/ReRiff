#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static void name(BYTE *manager,int slot,const char *id){
    BYTE *p=manager+264+24*slot;memset(p,0,24);strcpy((char *)p,id);*(unsigned *)(p+16)=3;*(unsigned *)(p+20)=15;
}
int main(void){
    BYTE manager[796]={0},settings[328],before[328];memset(settings,0xa5,sizeof(settings));settings[5]=1;
    *(int *)(manager+792)=15;name(manager,0,"alf");name(manager,5,"fab");*(int *)(settings+324)=15;
    memcpy(before,settings,sizeof(settings));CHECK(car_fallback_target(manager,settings,"alf")==0);
    CHECK(!memcmp(before,settings,sizeof(settings))); /* Planning cannot modify saves. */
    CHECK(car_fallback_target(manager,settings,"fab")==5);
    CHECK(car_fallback_target(manager,settings,"dod")==-1); /* Must be an original base. */
    CHECK(car_fallback_target(manager,settings,"pl1")==-1); /* Base must actually exist. */
    name(manager,1,"alf");CHECK(car_fallback_target(manager,settings,"alf")==-1);name(manager,1,"bui");
    *(int *)(manager+792)=16;name(manager,15,"dod");CHECK(car_fallback_target(manager,settings,"alf")==-1);
    name(manager,15,"fab");CHECK(car_fallback_target(manager,settings,"alf")==-1); /* Reused active slot is not proof of removal. */
    *(int *)(manager+792)=15;settings[5]=0;CHECK(car_fallback_target(manager,settings,"alf")==-1);settings[5]=1;
    for(int i=-1;i<15;i++){*(int *)(settings+324)=i;CHECK(car_fallback_target(manager,settings,"alf")==-1);}
    *(int *)(settings+324)=22;CHECK(car_fallback_target(manager,settings,"alf")==-1);
    *(int *)(settings+324)=21;CHECK(car_fallback_target(manager,settings,"alf")==0);
    const char *used=NULL;
    CHECK(car_fallback_resolve(manager,settings,"alf","fab",&used)==5&&!strcmp(used,"fab")); /* Declared fallback wins. */
    CHECK(car_fallback_resolve(manager,settings,"alf","",&used)==0&&!strcmp(used,"alf")); /* Absent fallback uses donor. */
    CHECK(car_fallback_resolve(manager,settings,"alf","pl1",&used)==0&&!strcmp(used,"alf")); /* Missing stock fallback uses donor. */
    CHECK(car_fallback_resolve(manager,settings,"alf","dod",&used)==0&&!strcmp(used,"alf")); /* Non-stock fallback is refused. */
    *(int *)(manager+792)=23;CHECK(car_fallback_target(manager,settings,"alf")==-1);
    *(int *)(manager+792)=14;CHECK(car_fallback_target(manager,settings,"alf")==-1);
    CHECK(car_fallback_target(NULL,settings,"alf")==-1);CHECK(car_fallback_target(manager,NULL,"alf")==-1);
    WCHAR exe[MAX_PATH],registry[MAX_PATH],mod[MAX_PATH],file[MAX_PATH],value[16];
    CHECK(GetModuleFileNameW(NULL,exe,MAX_PATH));WCHAR *slash=wcsrchr(exe,L'\\');CHECK(slash);slash[1]=0;
    _snwprintf(registry,MAX_PATH,L"%lsorigin-test.ini",exe);
    _snwprintf(mod,MAX_PATH,L"%lsmods",exe);CreateDirectoryW(mod,NULL);
    _snwprintf(mod,MAX_PATH,L"%lsmods\\origin-test",exe);CHECK(CreateDirectoryW(mod,NULL));
    _snwprintf(file,MAX_PATH,L"%ls\\origin.txt",mod);
    CHECK(WritePrivateProfileStringW(L"origins",L"dod",L"alf",file));
    CHECK(WritePrivateProfileStringW(L"fallbacks",L"dod",L"fab",file));
    *(int *)(manager+792)=16;name(manager,15,"dod");car_origin_packages(manager,registry);
    GetPrivateProfileStringW(L"slot_15",L"base",L"",value,16,registry);CHECK(!wcscmp(value,L"alf"));
    GetPrivateProfileStringW(L"slot_15",L"fallback",L"",value,16,registry);CHECK(!wcscmp(value,L"fab"));
    WCHAR other[MAX_PATH],otherFile[MAX_PATH];
    _snwprintf(other,MAX_PATH,L"%lsmods\\origin-conflict",exe);CHECK(CreateDirectoryW(other,NULL));
    _snwprintf(otherFile,MAX_PATH,L"%ls\\origin.txt",other);CHECK(WritePrivateProfileStringW(L"origins",L"dod",L"alf",otherFile));
    CHECK(WritePrivateProfileStringW(L"fallbacks",L"dod",L"cor",otherFile));
    car_origin_packages(manager,registry);GetPrivateProfileStringW(L"slot_15",L"base",L"",value,16,registry);CHECK(!wcscmp(value,L"alf"));
    GetPrivateProfileStringW(L"slot_15",L"fallback",L"",value,16,registry);CHECK(!*value); /* Conflicting fallbacks are refused. */
    DeleteFileW(otherFile);RemoveDirectoryW(other);
    CHECK(WritePrivateProfileStringW(L"fallbacks",L"dod",L"bogus",file));car_origin_packages(manager,registry);
    GetPrivateProfileStringW(L"slot_15",L"base",L"",value,16,registry);CHECK(!wcscmp(value,L"alf")); /* Invalid fallback keeps the donor. */
    GetPrivateProfileStringW(L"slot_15",L"fallback",L"",value,16,registry);CHECK(!*value);
    CHECK(WritePrivateProfileStringW(L"origins",L"dod",L"fab",file));CHECK(WritePrivateProfileStringW(L"fallbacks",L"dod",L"fab",file));car_origin_packages(manager,registry);
    DeleteFileW(file);RemoveDirectoryW(mod);*(int *)(manager+792)=15;car_origin_packages(manager,registry);
    GetPrivateProfileStringW(L"slot_15",L"base",L"",value,16,registry);CHECK(!wcscmp(value,L"fab"));
    GetPrivateProfileStringW(L"slot_15",L"fallback",L"",value,16,registry);CHECK(!wcscmp(value,L"fab")); /* Removal retains both. */
    DeleteFileW(registry);_snwprintf(mod,MAX_PATH,L"%lsmods",exe);RemoveDirectoryW(mod);
    puts("PASS: missing custom selection resolves to the declared stock fallback or donor; installed/reused slots, invalid states, ambiguous origins and fallbacks refused; planning preserves saves");return 0;
}
