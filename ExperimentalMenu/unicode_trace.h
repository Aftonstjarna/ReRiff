/* Opt-in Unicode diagnostics and native fallback experiments.
 * Original hash/bytes and ABI evidence: reports/UNICODE-TEXT-TRACE.md
 * and reports/CJK-FONT-INTEGRATION.md. */
#include <stdarg.h>
static BYTE *unicode_base;
static HANDLE unicode_log=INVALID_HANDLE_VALUE;
static CRITICAL_SECTION unicode_lock;
static unsigned unicode_bytes,unicode_records;
static uint64_t unicode_seen[512];
static unsigned unicode_seen_count;
static void *unicode_layout_original,*unicode_glyph_original,*unicode_width_original,*unicode_text_ctor_original;
static HANDLE (WINAPI *unicode_first_original)(LPCWSTR,LPWIN32_FIND_DATAW);
static BOOL (WINAPI *unicode_next_original)(HANDLE,LPWIN32_FIND_DATAW);
static int (WINAPI *unicode_convert_original)(UINT,DWORD,LPCCH,int,LPWSTR,int);

static void unicode_append(char *out,size_t capacity,size_t *used,const char *format,...){
    if(*used>=capacity-1)return;
    va_list args;va_start(args,format);int n=vsnprintf(out+*used,capacity-*used,format,args);va_end(args);
    if(n>0)*used+=(size_t)n<capacity-*used?(size_t)n:capacity-*used-1;
}
static __thread int unicode_logging_check;
static void unicode_write(const char *line){
    if(unicode_logging_check)return;unicode_logging_check=1;BOOL logging=rr_logging_ini(config_path);unicode_logging_check=0;if(!logging)return;
    if(unicode_log==INVALID_HANDLE_VALUE||!TryEnterCriticalSection(&unicode_lock))return;
    size_t length=strlen(line);uint64_t hash=14695981039346656037ULL;
    for(size_t i=0;i<length;i++)hash=(hash^(BYTE)line[i])*1099511628211ULL;
    for(unsigned i=0;i<unicode_seen_count;i++)if(unicode_seen[i]==hash){LeaveCriticalSection(&unicode_lock);return;}
    if(unicode_records<4096&&unicode_bytes+length+2<4*1024*1024){
        unicode_seen[unicode_records%512]=hash;if(unicode_seen_count<512)unicode_seen_count++;
        DWORD count;WriteFile(unicode_log,line,(DWORD)length,&count,NULL);WriteFile(unicode_log,"\r\n",2,&count,NULL);
        unicode_records++;unicode_bytes+=(unsigned)length+2;
    }
    LeaveCriticalSection(&unicode_lock);
}
static unsigned unicode_copy(const WCHAR *text,int limit,WCHAR *out,BOOL *interesting){
    unsigned n=0;*interesting=FALSE;
    if(limit<0||limit>256)limit=256;
    for(;n<(unsigned)limit;n++){
        if(!readable(text+n,2)||!text[n])break;
        out[n]=text[n];if(out[n]>127||out[n]=='?')*interesting=TRUE;
    }
    out[n]=0;return n;
}
static void unicode_text(const char *stage,const WCHAR *text,int limit,void *font,void *caller){
    WCHAR copy[257];BOOL interesting;unsigned count=unicode_copy(text,limit,copy,&interesting);
    if(!interesting)return;
    char line[4096];size_t used=0;
    uintptr_t rva=(uintptr_t)caller-(uintptr_t)unicode_base;
    unicode_append(line,sizeof(line),&used,"stage=%s font=%p caller_rva=0x%08lx text=\"",stage,font,(unsigned long)rva);
    for(unsigned i=0;i<count;i++){
        unsigned c=copy[i];
        if(c>=32&&c<127&&c!='"'&&c!='\\')unicode_append(line,sizeof(line),&used,"%c",c);
        else unicode_append(line,sizeof(line),&used,"\\u%04X",c);
    }
    unicode_append(line,sizeof(line),&used,"\" utf16_units=");
    for(unsigned i=0;i<count;i++)unicode_append(line,sizeof(line),&used,"%sU+%04X",i?" ":"",copy[i]);
    unicode_append(line,sizeof(line),&used," bounded=%s",count==256?"true":"false");unicode_write(line);
}
__attribute__((used,noinline)) static void __cdecl unicode_layout_log(void *font,const WCHAR *text,void *caller){
    DWORD error=GetLastError();cjk_resource_probe();cjk_prepare_fallback(font,text);unicode_text("font-layout",text,-1,font,caller);SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl unicode_layout_context(void *font,void **frame){
    DWORD error=GetLastError();
    if(readable(frame,5*sizeof(void *))){
        WCHAR copy[257];BOOL interesting;unicode_copy(frame[2],-1,copy,&interesting);
        if(interesting){
            void *fallback=readable(font,0x70)?*(void **)((BYTE *)font+0x68):NULL;
            char line[256];snprintf(line,sizeof(line),"stage=font-context font=%p caller_rva=0x%08lx primary=%p alternate=%p native_fallback=%p",
                font,(unsigned long)((uintptr_t)frame[0]-(uintptr_t)unicode_base),frame[3],frame[4],fallback);unicode_write(line);
        }
    }
    cjk_direct_layout(font,frame);SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl unicode_glyph_log(void *font,unsigned code,void *glyph,void *caller){
    DWORD error=GetLastError();
    if(code>127||code=='?'){
        char line[256];unsigned selected=readable(glyph,2)?*(WORD *)glyph:0xffff;
        snprintf(line,sizeof(line),"stage=glyph font=%p caller_rva=0x%08lx requested=U+%04X selected=U+%04X match=%s",font,
            (unsigned long)((uintptr_t)caller-(uintptr_t)unicode_base),code&65535,selected,selected==(code&65535)?"true":"false");unicode_write(line);
    }
    SetLastError(error);
}
/* Preserve flags, integer registers and the complete x87/SSE state around
 * logging: callers may rely on the original native callee's register usage. */
__attribute__((naked,noinline)) static void unicode_layout_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 36(%ebx)\npush 44(%ebx)\npush 24(%ebx)\ncall _unicode_layout_log\nadd $12,%esp\n"
        "lea 36(%ebx),%eax\npush %eax\npush 24(%ebx)\ncall _unicode_layout_context\nadd $8,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_unicode_layout_original\n");
}
__attribute__((used,noinline)) static void __cdecl cjk_width_prepare(void *font,const WCHAR *text){
    DWORD error=GetLastError();cjk_prepare_fallback(font,text);SetLastError(error);
}
__attribute__((naked,noinline)) static void unicode_width_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 40(%ebx)\npush 24(%ebx)\ncall _cjk_width_prepare\nadd $8,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_unicode_width_original\n");
}
__attribute__((used,noinline)) static void __cdecl cjk_text_prepare(void *font,const WCHAR *text){
    /* Native text construction allocates its alternate visual only when the
       font already has a fallback. Prepare even an initially Latin label so
       a later Unicode setTextRaw can use that same native visual. */
    (void)text;DWORD error=GetLastError();cjk_prepare_fallback(font,L"\x6708");SetLastError(error);
}
__attribute__((naked,noinline)) static void unicode_text_ctor_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 40(%ebx)\npush 44(%ebx)\ncall _cjk_text_prepare\nadd $8,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_unicode_text_ctor_original\n");
}
__attribute__((naked,noinline)) static void unicode_glyph_entry(void){
    __asm__("push %ebp\nmov %esp,%ebp\npush %ecx\npush 12(%ebp)\npush 8(%ebp)\ncall *_unicode_glyph_original\n"
        "pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 4(%ebp)\npush 28(%ebx)\npush 8(%ebp)\npush -4(%ebp)\ncall _unicode_glyph_log\nadd $16,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\nmov %ebp,%esp\npop %ebp\nret $8\n");
}
static BOOL unicode_detour(BYTE *entry,const BYTE *expected,unsigned count,void *hook,void **original){
    if(!readable(entry,count)||memcmp(entry,expected,count))return FALSE;
    BYTE *copy=VirtualAlloc(NULL,count+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copy)return FALSE;
    memcpy(copy,entry,count);copy[count]=0xe9;int32_t back=(int32_t)(entry+count-(copy+count+5));memcpy(copy+count+1,&back,4);
    DWORD old,ignored;if(!VirtualProtect(copy,count+5,PAGE_EXECUTE_READ,&old)){VirtualFree(copy,0,MEM_RELEASE);return FALSE;}
    FlushInstructionCache(GetCurrentProcess(),copy,count+5);
    if(!VirtualProtect(entry,count,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(copy,0,MEM_RELEASE);return FALSE;}
    *original=copy;entry[0]=0xe9;int32_t jump=(int32_t)((BYTE *)hook-(entry+5));memcpy(entry+1,&jump,4);
    for(unsigned i=5;i<count;i++)entry[i]=0x90;
    FlushInstructionCache(GetCurrentProcess(),entry,count);VirtualProtect(entry,count,old,&ignored);return TRUE;
}
#include "metadata_source_trace.h"
static void cjk_install_direct(BYTE *base){
    if(!cjk_component_enabled()||cjk_render_enabled==0)return;
    static const BYTE add[]={0x55,0x8b,0xec,0x6a,0xff},remove[]={0x55,0x8b,0xec,0x83,0xec,0x10};
    static const BYTE destroy[]={0x55,0x8b,0xec,0x56,0x57},visible[]={0x55,0x8b,0xec,0x8b,0x45,0x08};
    static const BYTE colour[]={0x55,0x8b,0xec,0x8b,0x55,0x08},allocate[]={0x55,0x8b,0xec,0x53,0x8b,0x5d,0x0c};
    static const BYTE construct[]={0x55,0x8b,0xec,0x6a,0xff},child[]={0x55,0x8b,0xec,0x6a,0xff};
    BYTE dirty[]={0x55,0x8b,0xec,0x53,0x8b,0x1d,0,0,0,0};
    DWORD pool=(DWORD)(uintptr_t)(base+0x7fcc0c);memcpy(dirty+6,&pool,4);
    struct {DWORD rva;const BYTE *bytes;unsigned length;void *hook;void **original;} hooks[]={
        {0x57bb0,add,sizeof(add),cjk_add_entry,&cjk_add_original},
        {0x57ce0,remove,sizeof(remove),cjk_remove_entry,&cjk_remove_original},
        {0x1e730,destroy,sizeof(destroy),cjk_destroy_entry,&cjk_destroy_original},
        {0x53580,dirty,sizeof(dirty),cjk_dirty_entry,&cjk_dirty_original},
        {0x535e0,visible,sizeof(visible),cjk_visible_entry,&cjk_visible_original},
        {0x42f50,colour,sizeof(colour),cjk_direct_colour_entry,&cjk_direct_colour_original}
    };
    for(unsigned i=0;i<sizeof(hooks)/sizeof(*hooks);i++){
        if(!readable(base+hooks[i].rva,hooks[i].length)||memcmp(base+hooks[i].rva,hooks[i].bytes,hooks[i].length)){
            unicode_write("REFUSED direct CJK visual extension: original hook bytes differ.");return;
        }
    }
    if(memcmp(base+0x42bd0,allocate,sizeof(allocate))||memcmp(base+0x1e3b0,construct,sizeof(construct))
        ||memcmp(base+0x1eef0,child,sizeof(child))){unicode_write("REFUSED direct CJK visual APIs: original bytes differ.");return;}
    cjk_direct_base=base;
    cjk_visual_allocate=(void *(__thiscall *)(void *,unsigned,unsigned))(base+0x42bd0);
    cjk_visual_construct=(void *(__thiscall *)(void *,int))(base+0x1e3b0);
    cjk_visual_child=(void (__thiscall *)(void *,void *))(base+0x1eef0);
    for(unsigned i=0;i<sizeof(hooks)/sizeof(*hooks);i++){
        if(!unicode_detour(base+hooks[i].rva,hooks[i].bytes,hooks[i].length,hooks[i].hook,hooks[i].original)){
            unicode_write("REFUSED direct CJK visual extension: installation incomplete; pairing disabled.");return;
        }
    }
    cjk_visual_colour=(void (__thiscall *)(void *,const float *))cjk_direct_colour_original;
    cjk_direct_enabled=TRUE;
    unicode_write("Installed opt-in direct CJK native alternate visuals (bounded native visual chains).");
    if(cjk_component_enabled()){
        static const BYTE finish[]={0x8b,0x75,0x08,0xf3,0x0f,0x10,0x44,0x24,0x24};
        static const BYTE setter[]={0x55,0x8b,0xec,0x53,0x8b,0x5d,0x08};
        if(!memcmp(base+0x1e7f0,construct,sizeof(construct))&&!memcmp(base+0x54f90,setter,sizeof(setter))
            &&!memcmp(base+0x55010,setter,sizeof(setter))
            &&unicode_detour(base+0x563d9,finish,sizeof(finish),cjk_pages_finish_entry,&cjk_pages_finish_original)){
            cjk_visual_construct_texture=(void *(__thiscall *)(void *,const int *,const int *,int))(base+0x1e7f0);
            cjk_page_resource=(void (__thiscall *)(void *,const int *))(base+0x54f90);
            cjk_page_texture=(void (__thiscall *)(void *,const int *))(base+0x55010);
            cjk_hd_ready=TRUE;unicode_write("Installed opt-in 32/64/96px native paged font selection.");
        }else unicode_write("REFUSED HD page APIs: original instructions differ; 32px fallback retained.");
    }
}
static HANDLE WINAPI unicode_first(LPCWSTR pattern,LPWIN32_FIND_DATAW data){
    HANDLE result=unicode_first_original(pattern,data);DWORD error=GetLastError();
    if(result!=INVALID_HANDLE_VALUE&&data)unicode_text("find-first-W",data->cFileName,MAX_PATH,NULL,__builtin_return_address(0));
    SetLastError(error);return result;
}
static BOOL WINAPI unicode_next(HANDLE search,LPWIN32_FIND_DATAW data){
    BOOL result=unicode_next_original(search,data);DWORD error=GetLastError();
    if(result&&data)unicode_text("find-next-W",data->cFileName,MAX_PATH,NULL,__builtin_return_address(0));
    SetLastError(error);return result;
}
static int WINAPI unicode_convert(UINT page,DWORD flags,LPCCH bytes,int count,LPWSTR output,int capacity){
    int result=unicode_convert_original(page,flags,bytes,count,output,capacity);DWORD error=GetLastError();
    BOOL interesting=FALSE;
    if(result>0&&output&&capacity>=result){
        WCHAR copy[257];unicode_copy(output,result,copy,&interesting);
        char stage[80];snprintf(stage,sizeof(stage),"multibyte-output(cp=%u,flags=%lu)",page,(unsigned long)flags);
        if(interesting)unicode_text(stage,output,result,NULL,__builtin_return_address(0));
    }else if(!result){
        int limit=count<0?128:count>128?128:count;
        for(int i=0;i<limit&&readable(bytes+i,1);i++){if(count<0&&!bytes[i])break;if((BYTE)bytes[i]>127||bytes[i]=='?')interesting=TRUE;}
    }
    if(interesting){
        char line[1024];size_t used=0;unicode_append(line,sizeof(line),&used,"stage=multibyte-input cp=%u caller_rva=0x%08lx result=%d error=%lu bytes=",page,
            (unsigned long)((uintptr_t)__builtin_return_address(0)-(uintptr_t)unicode_base),result,(unsigned long)error);
        int limit=count<0?128:count>128?128:count;
        for(int i=0;i<limit&&readable(bytes+i,1);i++){if(count<0&&!bytes[i])break;unicode_append(line,sizeof(line),&used,"%02X ",(BYTE)bytes[i]);}
        unicode_write(line);
    }
    SetLastError(error);return result;
}
static void unicode_imports(BYTE *base){
    IMAGE_DOS_HEADER *dos=(void *)base;IMAGE_NT_HEADERS32 *nt=(void *)(base+dos->e_lfanew);
    IMAGE_DATA_DIRECTORY directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!directory.VirtualAddress||!readable(base+directory.VirtualAddress,directory.Size))return;
    IMAGE_IMPORT_DESCRIPTOR *table=(void *)(base+directory.VirtualAddress);
    for(unsigned i=0;i<directory.Size/sizeof(*table)&&table[i].Name;i++){
        if(!table[i].OriginalFirstThunk)continue;
        IMAGE_THUNK_DATA32 *names=(void *)(base+table[i].OriginalFirstThunk),*slots=(void *)(base+table[i].FirstThunk);
        for(unsigned j=0;j<4096&&readable(names+j,4)&&readable(slots+j,4)&&names[j].u1.AddressOfData;j++){
            if(IMAGE_SNAP_BY_ORDINAL32(names[j].u1.Ordinal))continue;
            IMAGE_IMPORT_BY_NAME *entry=(void *)(base+names[j].u1.AddressOfData);if(!readable(entry,64))continue;
            void *hook=NULL,**original=NULL;
            if(!strcmp((char *)entry->Name,"FindFirstFileW")){hook=unicode_first;original=(void **)&unicode_first_original;}
            if(!strcmp((char *)entry->Name,"FindNextFileW")){hook=unicode_next;original=(void **)&unicode_next_original;}
            if(!strcmp((char *)entry->Name,"MultiByteToWideChar")){hook=unicode_convert;original=(void **)&unicode_convert_original;}
            if(!hook)continue;
            void *expected=(void *)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),(char *)entry->Name);
            if((void *)(uintptr_t)slots[j].u1.Function!=expected)continue;
            DWORD old,ignored;if(!VirtualProtect(slots+j,4,PAGE_READWRITE,&old))continue;
            *original=expected;InterlockedExchangePointer((void **)(slots+j),hook);VirtualProtect(slots+j,4,old,&ignored);
            char line[128];snprintf(line,sizeof(line),"Installed source trace: %s",entry->Name);unicode_write(line);
        }
    }
}
static void install_unicode_trace(BYTE *base){
    BOOL trace=rr_logging_ini(config_path)&&GetPrivateProfileIntW(L"diagnostics",L"unicode_trace",0,config_path)!=0;
    if(!trace&&!cjk_component_enabled())return;
    unicode_base=base;
    if(trace){
    WCHAR path[MAX_PATH];wcscpy(path,log_path);WCHAR *slash=wcsrchr(path,L'\\');if(!slash)return;
    if((size_t)(slash-path)+wcslen(L"\\ReRiff-unicode.log")>=MAX_PATH)return;
    wcscpy(slash,L"\\ReRiff-unicode.log");
    unicode_log=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(unicode_log==INVALID_HANDLE_VALUE){log_line("Unicode trace could not open its log.");return;}
    InitializeCriticalSection(&unicode_lock);unicode_base=base;
    unicode_write("Unicode trace enabled; UTF-16 code units are escaped, buffers bounded to 256 units, duplicate records suppressed, limit 4 MiB/4096 records. This log contains local filenames and visible text.");
    char locale[128];snprintf(locale,sizeof(locale),"Windows code pages: ACP=%u OEMCP=%u; cp=0 in conversion records means ACP.",GetACP(),GetOEMCP());unicode_write(locale);
    }
    static const BYTE layout[]={0x55,0x8b,0xec,0x83,0xe4,0xf8},glyph[]={0x55,0x8b,0xec,0x56,0x57};
    BOOL a=unicode_detour(base+0x55520,layout,sizeof(layout),unicode_layout_entry,&unicode_layout_original);
    BOOL b=unicode_detour(base+0x56e60,glyph,sizeof(glyph),unicode_glyph_entry,&unicode_glyph_original);
    unicode_write(a?"Installed shared font-layout trace.":"REFUSED shared font-layout trace: original bytes differ.");
    unicode_write(b?"Installed shared glyph-lookup trace.":"REFUSED shared glyph-lookup trace: original bytes differ.");
    if(cjk_component_enabled()){
        static const BYTE width[]={0x55,0x8b,0xec,0x51,0x53};
        BOOL c=unicode_detour(base+0x56450,width,sizeof(width),unicode_width_entry,&unicode_width_original);
        unicode_write(c?"Installed opt-in CJK width preparation.":"REFUSED CJK width preparation: original bytes differ.");
        static const BYTE text_ctor[]={0x55,0x8b,0xec,0x6a,0xff};
        BOOL d=unicode_detour(base+0x50340,text_ctor,sizeof(text_ctor),unicode_text_ctor_entry,&unicode_text_ctor_original);
        unicode_write(d?"Installed opt-in CJK native text preparation.":"REFUSED CJK text preparation: original bytes differ.");
        static const BYTE y_bytes[]={0x8b,0x41,0x20,0x89,0x47,0x18};
        static const BYTE pen_bytes[]={0x80,0x7c,0x24,0x0e,0x00};
        cjk_y_after=base+0x55887;cjk_pen_after=base+0x558f1;
        BOOL e=unicode_detour(base+0x55881,y_bytes,sizeof(y_bytes),cjk_y_entry,&cjk_y_original);
        BOOL f=unicode_detour(base+0x558ec,pen_bytes,sizeof(pen_bytes),cjk_pen_entry,&cjk_pen_original);
        static const BYTE split_bytes[]={0xf3,0x0f,0x10,0x46,0x04};
        BOOL g=unicode_detour(base+0x55fdf,split_bytes,sizeof(split_bytes),cjk_split_entry,&cjk_split_original);
        unicode_write(e&&f?"Installed owned CJK chained-glyph positioning.":"REFUSED complete CJK positioning: original bytes differ.");
        unicode_write(g?"Installed owned CJK native visual split preparation.":"REFUSED CJK split preparation: original bytes differ.");
        static const BYTE colour_bytes[]={0x55,0x8b,0xec,0x56,0xff,0x75,0x08};
        static const BYTE visual_colour_bytes[]={0x55,0x8b,0xec,0x8b,0x55,0x08};
        BOOL h=readable(base+0x42f50,sizeof(visual_colour_bytes))
            &&!memcmp(base+0x42f50,visual_colour_bytes,sizeof(visual_colour_bytes));
        if(h){
            cjk_visual_colour=(void (__thiscall *)(void *,const float *))(base+0x42f50);
            h=unicode_detour(base+0x50c40,colour_bytes,sizeof(colour_bytes),cjk_colour_entry,&cjk_colour_original);
        }
        unicode_write(h?"Installed owned CJK native colour propagation.":"REFUSED CJK colour propagation: original bytes differ.");
        BYTE alignment_bytes[]={0xf3,0x0f,0x10,0x05,0,0,0,0};
        DWORD alignment_constant=(DWORD)(uintptr_t)(base+0x606618);memcpy(alignment_bytes+4,&alignment_constant,4);
        static const BYTE alignment_width_bytes[]={0xf3,0x0f,0x59,0x40,0x10};
        cjk_align_after=base+0x55d98;cjk_align_width_after=base+0x55ddb;
        BOOL i=unicode_detour(base+0x55d90,alignment_bytes,sizeof(alignment_bytes),cjk_align_entry,&cjk_align_original);
        BOOL j=unicode_detour(base+0x55dd6,alignment_width_bytes,sizeof(alignment_width_bytes),cjk_align_width_entry,&cjk_align_width_original);
        unicode_write(i&&j?"Installed owned CJK native centre/right alignment.":"REFUSED complete CJK alignment: original bytes differ.");
        if(!a||!b||!c||!d||!e||!f||!g||!h||!i||!j){cjk_render_enabled=0;log_line("CJK font hooks refused: an original function differs or another hook is present.");}
        else {cjk_install_direct(base);if(!cjk_direct_enabled){cjk_render_enabled=0;log_line("CJK visual integration refused; original rendering retained.");}}
    }
    if(trace){install_metadata_source_trace(base);unicode_imports(base);}log_line(trace?"Unicode trace enabled: logs/ReRiff-unicode.log.":cjk_render_enabled==0?"CJK font integration is inactive; inspect earlier guard failures.":"Installed CJK font fallback (restart required after changing Languages).");
}
