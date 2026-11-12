#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
typedef void *(__thiscall *GlyphProbe)(void *,unsigned,BYTE *);
typedef unsigned (__thiscall *LayoutProbe)(void *,void *,const WCHAR *);
static BYTE font[128],alternate[128],latin[44],kana[44],replacement[44];
static BOOL relocate_trace_mapping(BYTE *base){
    IMAGE_NT_HEADERS32 *nt=(void *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    DWORD delta=(DWORD)(uintptr_t)base-nt->OptionalHeader.ImageBase;
    /* DONT_RESOLVE_DLL_REFERENCES may leave an EXE's absolute operands at
       preferred addresses under Wine. Reproduce normal loader relocation
       in this private test mapping before checking relocated hook guards. */
    if(!delta)return TRUE;
    if(*(DWORD *)(base+0x198433)!=(DWORD)(nt->OptionalHeader.ImageBase+0x52d39c))return FALSE;
    IMAGE_DATA_DIRECTORY reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if(!reloc.VirtualAddress||!reloc.Size)return FALSE;
    DWORD cursor=0;
    while(cursor<reloc.Size){
        IMAGE_BASE_RELOCATION *block=(void *)(base+reloc.VirtualAddress+cursor);
        if(block->SizeOfBlock<8||block->SizeOfBlock>reloc.Size-cursor)return FALSE;
        WORD *entries=(void *)(block+1);
        for(DWORD i=0;i<(block->SizeOfBlock-8)/2;i++){
            unsigned type=entries[i]>>12;if(type==IMAGE_REL_BASED_ABSOLUTE)continue;
            if(type!=IMAGE_REL_BASED_HIGHLOW)return FALSE;
            DWORD *target=(void *)(base+block->VirtualAddress+(entries[i]&0xfff));DWORD old,ignored;
            if(!VirtualProtect(target,4,PAGE_READWRITE,&old))return FALSE;
            *target+=delta;if(!VirtualProtect(target,4,old,&ignored))return FALSE;
        }
        cursor+=block->SizeOfBlock;
    }
    FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);return TRUE;
}
static void resolve_trace_imports(BYTE *base){
    IMAGE_NT_HEADERS32 *nt=(void *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *imports=(void *)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(unsigned i=0;imports[i].Name;i++){
        if(!imports[i].OriginalFirstThunk)continue;
        const char *library_name=(char *)(base+imports[i].Name);
        BOOL crt=!_stricmp(library_name,"msvcr120.dll")||!_stricmp(library_name,"msvcp120.dll");
        HMODULE library=crt?LoadLibraryA(library_name):GetModuleHandleW(L"kernel32.dll");
        IMAGE_THUNK_DATA32 *names=(void *)(base+imports[i].OriginalFirstThunk),*slots=(void *)(base+imports[i].FirstThunk);
        for(unsigned j=0;names[j].u1.AddressOfData;j++){
            if(IMAGE_SNAP_BY_ORDINAL32(names[j].u1.Ordinal))continue;
            const char *name=(char *)((IMAGE_IMPORT_BY_NAME *)(base+names[j].u1.AddressOfData))->Name;
            if(!crt&&strcmp(name,"FindFirstFileW")&&strcmp(name,"FindNextFileW")&&strcmp(name,"MultiByteToWideChar"))continue;
            DWORD old,ignored;VirtualProtect(slots+j,4,PAGE_READWRITE,&old);
            slots[j].u1.Function=(DWORD)(uintptr_t)GetProcAddress(library,name);
            VirtualProtect(slots+j,4,old,&ignored);
        }
    }
}
int main(int argc,char **argv){
    CHECK(argc==2);WCHAR original[MAX_PATH],temp[MAX_PATH];
    CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,original,MAX_PATH));CHECK(supported_file(original));
    CHECK(GetTempPathW(MAX_PATH,temp));CHECK(GetTempFileNameW(temp,L"rru",0,config_path));
    wcscpy(log_path,temp);wcscat(log_path,L"ReRiff-engine-menu.log");
    BYTE *base=(BYTE *)LoadLibraryExW(original,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(base);
    CHECK(relocate_trace_mapping(base));
    BYTE *lastfm_site=base+0x34fe9c+12+0xa6;
    CHECK(install_lastfm_utf8(base)&&!lastfm_utf8_installed);
    CHECK(!memcmp(lastfm_site,lastfm_utf8_original,sizeof(lastfm_utf8_original)));
    CHECK(WritePrivateProfileStringW(L"languages",L"cjk_enabled",L"true",config_path));
    DWORD site_old,site_ignored;CHECK(VirtualProtect(lastfm_site,31,PAGE_READWRITE,&site_old));
    lastfm_site[0]^=1;CHECK(!install_lastfm_utf8(base)&&!lastfm_utf8_installed);lastfm_site[0]^=1;
    CHECK(VirtualProtect(lastfm_site,31,site_old,&site_ignored));
    CHECK(install_lastfm_utf8(base)&&lastfm_utf8_installed&&install_lastfm_utf8(base));
    CHECK(!memcmp(lastfm_site,lastfm_utf8_copy,sizeof(lastfm_utf8_copy)));
    for(unsigned i=sizeof(lastfm_utf8_copy);i<sizeof(lastfm_utf8_original);i++)CHECK(lastfm_site[i]==0);
    CHECK(WritePrivateProfileStringW(L"languages",L"cjk_enabled",L"false",config_path));
    install_unicode_trace(base);CHECK(unicode_log==INVALID_HANDLE_VALUE);
    CHECK(WritePrivateProfileStringW(L"languages",L"cjk_enabled",L"true",config_path));
    CHECK(WritePrivateProfileStringW(L"diagnostics",L"unicode_trace",L"1",config_path));
    CHECK(WritePrivateProfileStringW(L"diagnostics",L"cjk_render_probe",L"1",config_path));
    CHECK(WritePrivateProfileStringW(L"diagnostics",L"metadata_source_trace",L"1",config_path));
    resolve_trace_imports(base);install_unicode_trace(base);
    CHECK(metadata_artist_original&&metadata_title_original&&metadata_mp3_original&&metadata_codec_original);
    /* Execute the original decoded-buffer getters through the observers. */
    BYTE system[0x36c]={0};const char source_artist[]="BUCK\xe2\x80\x90TICK";
    strcpy((char *)system+0x26c,source_artist);strcpy((char *)system+0x16c,"Title");
    typedef char *(__thiscall *SourceGetter)(void *);
    SetLastError(2468);
    CHECK(((SourceGetter)(base+0x4f0d0))(system)==(char *)system+0x26c&&GetLastError()==2468);
    CHECK(((SourceGetter)(base+0x4f0c0))(system)==(char *)system+0x16c&&GetLastError()==2468);
    /* Original codec intentionally cannot represent U+2010. The observer
       must retain that result and original native string ownership. */
    BYTE native_string[24]={0};*(DWORD *)(native_string+20)=15;
    typedef unsigned (__cdecl *Codec)(const char *,unsigned,void *);
    unsigned codec_result=((Codec)(base+0x1c2500))(source_artist,(unsigned)-1,native_string);
    CHECK(!(codec_result&255)&&!strcmp((char *)native_string,"BUCK?TICK"));
    CHECK(*(DWORD *)(native_string+16)==9&&*(DWORD *)(native_string+20)==15);
    BYTE *tag_frame[8]={0};tag_frame[5]=system+0x16c;tag_frame[7]=system+0x26c;
    metadata_mp3_log((BYTE *)tag_frame);CHECK(GetLastError()==2468);
    CHECK(unicode_layout_original&&unicode_glyph_original&&unicode_convert_original&&unicode_first_original&&unicode_next_original);
    CHECK(unicode_width_original&&unicode_text_ctor_original&&cjk_y_original&&cjk_pen_original&&cjk_split_original&&cjk_colour_original&&cjk_render_enabled!=0);
    CHECK(cjk_align_original&&cjk_align_width_original);
    /* Call the original glyph machine code using controlled font records. */
    *(WORD *)latin='A';*(WORD *)kana=0x3042;*(WORD *)replacement='?';
    ((void **)font)[10]=latin;((void **)font)[11]=latin+44;((void **)font)[13]=replacement;
    ((void **)alternate)[10]=kana;((void **)alternate)[11]=kana+44;((void **)font)[26]=alternate;
    GlyphProbe lookup=(GlyphProbe)(base+0x56e60);BYTE fallback=9;SetLastError(1234);
    CHECK(lookup(font,'A',&fallback)==latin&&fallback==0&&GetLastError()==1234);
    CHECK(lookup(font,0x3042,&fallback)==kana&&fallback==1);
    CHECK(lookup(font,0x65e5,&fallback)==replacement&&fallback==0);
    CHECK(lookup(font,0x3042,NULL)==replacement);
    /* This stub checks hidden-return argument position and tail forwarding
     * without drawing a screen. The original layout entry has this prologue. */
    static const BYTE body[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0xb8,0x78,0x56,0x34,0x12,0x8b,0xe5,0x5d,0xc2,0x08,0x00};
    BYTE *stub=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);CHECK(stub);memcpy(stub,body,sizeof(body));
    CHECK(unicode_detour(stub,body,6,unicode_layout_entry,&unicode_layout_original));
    CHECK(!unicode_detour(stub,body,6,unicode_layout_entry,&unicode_layout_original));
    SetLastError(4321);CHECK(((LayoutProbe)stub)(font,NULL,L"\x3072\x3089\x304c\x306a \x30ab\x30ca \x65e5\x672c\x8a9e ?")==0x12345678);
    CHECK(GetLastError()==4321);
    unsigned before[4]={1,2,3,4},after[4]={0};unsigned value;void *self=font;const WCHAR *label=L"\x3042";
    __asm__ volatile("movdqu %3,%%xmm7\npush %5\npush $0\ncall *%4\nmovdqu %%xmm7,%1"
        :"=a"(value),"=m"(after),"+c"(self):"m"(before),"r"(stub),"r"(label):"edx","memory","cc","xmm7");
    CHECK(value==0x12345678&&!memcmp(before,after,sizeof(before)));
    static const BYTE ctor_body[]={0x55,0x8b,0xec,0x6a,0xff,0x8b,0x45,0x0c,0x8b,0xe5,0x5d,0xc2,0x08,0x00};
    BYTE *ctor_stub=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);CHECK(ctor_stub);
    memcpy(ctor_stub,ctor_body,sizeof(ctor_body));
    CHECK(unicode_detour(ctor_stub,ctor_body,5,unicode_text_ctor_entry,&unicode_text_ctor_original));
    typedef void *(__thiscall *CtorProbe)(void *,const WCHAR *,void *);
    CHECK(((CtorProbe)ctor_stub)(alternate,L"\x6708",font)==font);
    WCHAR wide[64];const char utf8[]={ (char)0xe6,(char)0x97,(char)0xa5,(char)0xe6,(char)0x9c,(char)0xac,0};
    int expected=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8,-1,wide,64);
    WCHAR converted[64];SetLastError(5678);
    CHECK(unicode_convert(CP_UTF8,MB_ERR_INVALID_CHARS,utf8,-1,converted,64)==expected&&!memcmp(wide,converted,expected*2)&&GetLastError()==5678);
    CHECK(!unicode_convert(CP_UTF8,MB_ERR_INVALID_CHARS,"\xff",1,converted,64)&&GetLastError()==ERROR_NO_UNICODE_TRANSLATION);
    WCHAR filename[MAX_PATH];wcscpy(filename,temp);wcscat(filename,L"reriff-trace-\x65e5\x672c\x8a9e.tmp");
    HANDLE file=CreateFileW(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);CHECK(file!=INVALID_HANDLE_VALUE);CloseHandle(file);
    WIN32_FIND_DATAW data;HANDLE search=unicode_first(filename,&data);CHECK(search!=INVALID_HANDLE_VALUE&&!wcscmp(data.cFileName,L"reriff-trace-\x65e5\x672c\x8a9e.tmp"));
    CHECK(!unicode_next(search,&data)&&GetLastError()==ERROR_NO_MORE_FILES);FindClose(search);DeleteFileW(filename);
    unsigned records=unicode_records;unicode_layout_log(font,L"\x3042",base);unicode_layout_log(font,L"\x3042",base);CHECK(unicode_records==records+1);
    CHECK(WritePrivateProfileStringW(L"diagnostics",L"cjk_direct_probe",L"1",config_path));
    CHECK(WritePrivateProfileStringW(L"diagnostics",L"cjk_hd_probe",L"1",config_path));
    cjk_install_direct(base);CHECK(cjk_direct_enabled&&cjk_hd_ready&&cjk_pages_finish_original);
    CloseHandle(unicode_log);unicode_log=INVALID_HANDLE_VALUE;
    WCHAR trace[MAX_PATH];wcscpy(trace,temp);wcscat(trace,L"ReRiff-unicode.log");
    file=CreateFileW(trace,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);CHECK(file!=INVALID_HANDLE_VALUE);
    char output[32768]={0};DWORD count;CHECK(ReadFile(file,output,sizeof(output)-1,&count,NULL));CloseHandle(file);
    CHECK(strstr(output,"requested=U+65E5 selected=U+003F match=false"));
    CHECK(strstr(output,"requested=U+3042 selected=U+3042 match=true"));
    CHECK(strstr(output,"stage=font-layout")&&strstr(output,"\\u65E5\\u672C\\u8A9E"));
    CHECK(strstr(output,"stage=find-first-W")&&strstr(output,"stage=multibyte-input cp=65001"));
    CHECK(strstr(output,"stage=decoded-getter field=Artist")&&strstr(output,"BUCK\\u2010TICK"));
    CHECK(strstr(output,"stage=mp3-tag-copy field=Artist"));
    CHECK(strstr(output,"input_u2010=1 output_bytes=9 output_questions=1 result=0"));
    DeleteFileW(config_path);DeleteFileW(trace);DeleteFileW(log_path);
    puts("PASS: opt-in tracing, original glyph/fallback behavior, native ABI/SSE/last-error preservation, Unicode enumeration/conversion, guards and deduplication");return 0;
}
