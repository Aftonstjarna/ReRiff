#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
typedef struct {union{WCHAR small[8];const WCHAR *text;};unsigned length,capacity;} NativeString;
typedef struct {BYTE *begin,*end,*capacity;} NativeVector;

static WCHAR fixture_base[MAX_PATH];
static void __fastcall fixture_scan(const ModsNativeString *unused,void *entries){
 (void)unused;mods_scan_path(fixture_base,entries);
}
static int __thiscall fixture_cancelled(void *self){(void)self;return 0;}
static int verify_bytes(BYTE *base,const char *path,const void *expected,unsigned length){
 BYTE stream[64]={0};BYTE *data=calloc(length+1,1);CHECK(data);
 ((void *(__thiscall *)(void *,const char *,BOOL))(base+0x197940))(stream,path,FALSE);
 CHECK(stream[0xc]);
 int count=((int(__thiscall *)(void *,void *,unsigned))(base+0x197c00))(stream,data,length+1);
 CHECK(count==(int)length);CHECK(!memcmp(data,expected,length));free(data);
 ((void *(__thiscall *)(void *,unsigned))(base+0x197b60))(stream,0);
 return 0;
}
static int verify_read(BYTE *base,const char *path,const char *expected){return verify_bytes(base,path,expected,(unsigned)strlen(expected));}
static int verify_original_livery(BYTE *base,const WCHAR *folder){
 WCHAR file[MAX_PATH];wcscpy(file,folder);wcscat(file,L"..\\original-livery.bin");
 FILE *input=_wfopen(file,L"rb");CHECK(input);CHECK(!fseek(input,0,SEEK_END));long size=ftell(input);CHECK(size>0&&size<2000000);rewind(input);
 BYTE *data=malloc(size);CHECK(data);CHECK(fread(data,1,size,input)==(size_t)size);fclose(input);
 int result=verify_bytes(base,"graphics\\vehicles\\decals\\alf_01\\batchtextures.brres",data,(unsigned)size);free(data);return result;
}
int main(int argc,char **argv){
 CHECK(argc==3||argc==4||argc==5);BOOL overrides=argc==5;unsigned expected=argc>=4?(unsigned)atoi(argv[3]):1;CHECK(expected>0&&expected<=31);WCHAR exe[MAX_PATH],folder[MAX_PATH];CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,exe,MAX_PATH));CHECK(supported_file(exe));CHECK(MultiByteToWideChar(CP_UTF8,0,argv[2],-1,folder,MAX_PATH));
    BYTE *base=(BYTE *)LoadLibraryExW(exe,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(base);
    IMAGE_NT_HEADERS32 *nt=(void *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    /* Wine maps an EXE without relocating its native code in this mode.
       Relocate this private test mapping only; never change the source file. */
    DWORD delta=(DWORD)(uintptr_t)base-nt->OptionalHeader.ImageBase;
    if(delta && *(DWORD *)(base+0x198433)==0x0092d39c){
        IMAGE_DATA_DIRECTORY reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        CHECK(reloc.VirtualAddress && reloc.Size);
        DWORD cursor=0;
        while(cursor<reloc.Size){
            IMAGE_BASE_RELOCATION *block=(void *)(base+reloc.VirtualAddress+cursor);
            CHECK(block->SizeOfBlock>=8 && block->SizeOfBlock<=reloc.Size-cursor);
            WORD *entries=(void *)(block+1);
            for(DWORD i=0;i<(block->SizeOfBlock-8)/2;i++){
                unsigned type=entries[i]>>12;if(type==IMAGE_REL_BASED_ABSOLUTE)continue;
                CHECK(type==IMAGE_REL_BASED_HIGHLOW);
                DWORD *target=(void *)(base+block->VirtualAddress+(entries[i]&0xfff));
                DWORD old,ignored;CHECK(VirtualProtect(target,4,PAGE_READWRITE,&old));
                *target+=delta;CHECK(VirtualProtect(target,4,old,&ignored));
            }
            cursor+=block->SizeOfBlock;
        }
        FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
    }
    IMAGE_IMPORT_DESCRIPTOR *imports=(void *)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(unsigned i=0;imports[i].Name;i++){
        const char *name=(char *)(base+imports[i].Name);
        if(_stricmp(name,"kernel32.dll")&&_stricmp(name,"msvcr120.dll")&&_stricmp(name,"msvcp120.dll"))continue;
        HMODULE library=LoadLibraryA(name);CHECK(library);
        IMAGE_THUNK_DATA32 *names=(void *)(base+imports[i].OriginalFirstThunk),*slots=(void *)(base+imports[i].FirstThunk);
        for(unsigned j=0;names[j].u1.AddressOfData;j++){
            const char *symbol=IMAGE_SNAP_BY_ORDINAL32(names[j].u1.Ordinal)?(const char *)(uintptr_t)IMAGE_ORDINAL32(names[j].u1.Ordinal):(char *)((IMAGE_IMPORT_BY_NAME *)(base+names[j].u1.AddressOfData))->Name;
            FARPROC function=GetProcAddress(library,symbol);CHECK(function);DWORD old,ignored;CHECK(VirtualProtect(slots+j,4,PAGE_READWRITE,&old));
            slots[j].u1.Function=(DWORD)(uintptr_t)function;VirtualProtect(slots+j,4,old,&ignored);
        }
    }

 WCHAR fake_exe[MAX_PATH];wcscpy(fake_exe,folder);size_t len=wcslen(fake_exe);CHECK(len>5);fake_exe[len-5]=0;wcscat(fake_exe,L"Game.exe");
 BYTE manager[0x100]={0};*(unsigned *)(manager+0x44)=7;
 BYTE *original_record=((void *(__cdecl *)(size_t))(void *)GetProcAddress(GetModuleHandleA("msvcr120.dll"),"malloc"))(20);CHECK(original_record);*(BYTE **)(manager+0x48)=original_record;*(BYTE **)(manager+0x4c)=original_record;*(BYTE **)(manager+0x50)=original_record;
 /* A non-null empty native vector models the initialized manager and exercises
    the late-start path without opening any installed game archive. */
 *(BYTE **)(base+0x7fcd2c)=manager;

 if(overrides){
  BOOL startup=strstr(argv[4],"startup")!=NULL,real=strstr(argv[4],"real")!=NULL;
  mods_scan_original=(ModsScanFn)(base+0x15e130);
  wcscpy(fixture_base,folder);WCHAR *last=wcsrchr(fixture_base,L'\\');
  if(last&&last[1]==0)*last=0;
  last=wcsrchr(fixture_base,L'\\');CHECK(last);wcscpy(last+1,L"baseline\\");
  if(real){wcscpy(fixture_base,exe);last=wcsrchr(fixture_base,L'\\');CHECK(last);last[1]=0;}
  if(startup){*(BYTE **)(base+0x7fcd2c)=NULL;CHECK(install_mods_vfs(base,fake_exe));CHECK(!mods_scanned);*(BYTE **)(base+0x7fcd2c)=manager;}
  BYTE *site=base+0x15ede7;DWORD old,ignored;CHECK(VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old));
  BYTE saved[5];memcpy(saved,site,5);*(int32_t *)(site+1)=(int32_t)((BYTE *)(uintptr_t)fixture_scan-(site+5));
  ((void(__thiscall *)(void *))(base+0x15ed80))(manager);
  memcpy(site,saved,5);VirtualProtect(site,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),site,5);
  CHECK(*(BYTE **)(manager+0x4c)-*(BYTE **)(manager+0x48)==20);
  void *vtable[4]={NULL,NULL,NULL,fixture_cancelled};*(void **)manager=vtable;
  if(!real)CHECK(!verify_read(base,"graphics\\vehicles\\decals\\alf_01\\batchtextures.brres","stock"));
  else { CHECK(!verify_original_livery(base,folder));unsigned index=99;CHECK(((void *(__thiscall *)(void *,const char *,unsigned *))(base+0x160230))(manager,"graphics\\vehicles\\decals\\alf_01\\batchtextures.brres",&index));CHECK(index==0); }
  BYTE prior[20];memcpy(prior,*(BYTE **)(manager+0x48),20);
  if(startup){mods_only=TRUE;((void(__thiscall *)(void *))(base+0x15ed80))(manager);mods_only=FALSE;}
  else CHECK(install_mods_vfs(base,fake_exe));
  CHECK(mods_scanned);
  CHECK(*(BYTE **)(manager+0x4c)-*(BYTE **)(manager+0x48)==60);
  CHECK(!memcmp(prior,*(BYTE **)(manager+0x48)+40,20));
  BYTE *mounted=*(BYTE **)(manager+0x48);
  CHECK(*(unsigned *)(mounted+12)>*(unsigned *)(mounted+32));
  CHECK(*(unsigned *)(mounted+32)>*(unsigned *)(mounted+52));
  unsigned index=99;typedef void *(__thiscall *Lookup)(void *,const char *,unsigned *);
  Lookup lookup=(Lookup)(base+0x160230);
  CHECK(lookup(manager,"graphics\\vehicles\\decals\\alf_01\\batchtextures.brres",&index));CHECK(index==0);
  CHECK(!verify_read(base,"graphics\\vehicles\\decals\\alf_01\\batchtextures.brres","second mod"));
  if(!real)CHECK(!verify_read(base,"stock.txt","fallback"));
  CHECK(!verify_read(base,"second.txt","second only"));
  CHECK(!lookup(manager,"missing.txt",&index));CHECK(index==0);
  if(real){
   /* Model a fresh mount with the two test mods absent; original bytes return. */
   BYTE *end=*(BYTE **)(manager+0x4c);*(BYTE **)(manager+0x48)=mounted+40;
   CHECK(!verify_original_livery(base,folder));
   *(BYTE **)(manager+0x48)=mounted;*(BYTE **)(manager+0x4c)=end;
  }
  puts("PASS native open/read: named override, last-folder conflict, stock fallback, missing path, stock record preserved");return 0;
 }
 CHECK(install_mods_vfs(base,fake_exe));CHECK(mods_scanned);
 unsigned bytes=*(BYTE **)(manager+0x4c)-*(BYTE **)(manager+0x48);printf("mounted bytes=%u\n",bytes);CHECK(bytes==expected*20);
 BYTE *record=*(BYTE **)(manager+0x48);for(unsigned n=0;n<expected;n++){unsigned id=n+1;BOOL found=FALSE;for(unsigned j=0;j<expected;j++)if(*(unsigned *)(record+j*20+12)==id)found=TRUE;CHECK(found);}
 puts("PASS original secondary loader mounted mods archive");return 0;
}
