#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
typedef struct {union{WCHAR small[8];const WCHAR *text;};unsigned length,capacity;} NativeString;
typedef struct {BYTE *begin,*end,*capacity;} NativeVector;
int main(int argc,char **argv){
 CHECK(argc==3||argc==4);unsigned expected=argc==4?(unsigned)atoi(argv[3]):1;CHECK(expected>0&&expected<=31);WCHAR exe[MAX_PATH],folder[MAX_PATH];CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,exe,MAX_PATH));CHECK(supported_file(exe));CHECK(MultiByteToWideChar(CP_UTF8,0,argv[2],-1,folder,MAX_PATH));
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
 CHECK(install_mods_vfs(base,fake_exe));CHECK(mods_scanned);
 unsigned bytes=*(BYTE **)(manager+0x4c)-*(BYTE **)(manager+0x48);printf("mounted bytes=%u\n",bytes);CHECK(bytes==expected*20);
 BYTE *record=*(BYTE **)(manager+0x48);for(unsigned n=0;n<expected;n++){unsigned id=n+1;BOOL found=FALSE;for(unsigned j=0;j<expected;j++)if(*(unsigned *)(record+j*20+12)==id)found=TRUE;CHECK(found);}
 puts("PASS original secondary loader mounted mods archive");return 0;
}
