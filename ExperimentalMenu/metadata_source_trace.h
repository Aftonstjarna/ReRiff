/* Read-only, opt-in metadata boundary diagnostics. No tag/record repair. */
static void *metadata_artist_original,*metadata_title_original,*metadata_mp3_original,*metadata_codec_original;
static void metadata_bytes(const char *stage,const char *field,const BYTE *source,void *caller){
    BYTE bytes[256];unsigned count=0;BOOL terminated=FALSE;
    for(;count<sizeof(bytes);count++){
        if(!readable(source+count,1))break;
        if(!source[count]){terminated=TRUE;break;}bytes[count]=source[count];
    }
    WCHAR wide[256];int units=count?MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char *)bytes,count,wide,256):0;
    char line[4096];size_t used=0;
    unicode_append(line,sizeof(line),&used,"stage=%s field=%s caller_rva=0x%08lx utf8_valid=%s bounded=%s bytes=",stage,field,
        (unsigned long)((uintptr_t)caller-(uintptr_t)unicode_base),!count||units?"true":"false",terminated?"false":"true");
    for(unsigned i=0;i<count&&i<64;i++)unicode_append(line,sizeof(line),&used,"%s%02X",i?"-":"",bytes[i]);
    unicode_append(line,sizeof(line),&used," text=\"");
    for(int i=0;i<units;i++){
        unsigned c=wide[i];if(c>=32&&c<127&&c!='"'&&c!='\\')unicode_append(line,sizeof(line),&used,"%c",c);
        else unicode_append(line,sizeof(line),&used,"\\u%04X",c);
    }
    unicode_append(line,sizeof(line),&used,"\"");unicode_write(line);
}
__attribute__((used,noinline)) static void __cdecl metadata_artist_log(BYTE *system,void *caller){
    DWORD error=GetLastError();if(readable(system+0x26c,256))metadata_bytes("decoded-getter","Artist",system+0x26c,caller);SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl metadata_title_log(BYTE *system,void *caller){
    DWORD error=GetLastError();if(readable(system+0x16c,256))metadata_bytes("decoded-getter","Title",system+0x16c,caller);SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl metadata_mp3_log(BYTE *frame){
    DWORD error=GetLastError();
    if(readable(frame,32)){
        metadata_bytes("mp3-tag-copy","Title",*(BYTE **)(frame+0x14),unicode_base+0xf8a5a);
        metadata_bytes("mp3-tag-copy","Artist",*(BYTE **)(frame+0x1c),unicode_base+0xf8a5a);
    }
    SetLastError(error);
}
#define METADATA_ENTRY(name,args,helper,cleanup,original) \
__attribute__((naked,noinline)) static void name(void){ \
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n" \
        args "call _" #helper "\nadd $" #cleanup ",%esp\n" \
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_" #original "\n"); \
}
METADATA_ENTRY(metadata_artist_entry,"push 36(%ebx)\npush 24(%ebx)\n",metadata_artist_log,8,metadata_artist_original)
METADATA_ENTRY(metadata_title_entry,"push 36(%ebx)\npush 24(%ebx)\n",metadata_title_log,8,metadata_title_original)
METADATA_ENTRY(metadata_mp3_entry,"push %ebp\n",metadata_mp3_log,4,metadata_mp3_original)
#undef METADATA_ENTRY
/* Last.fm conversion consumes a whole HTTP body. Log only counts, never
   the body, URLs or credentials. This narrows conversion loss without
   extending the existing song-field allowlist to arbitrary JSON fields. */
__attribute__((used,noinline)) static void __cdecl metadata_codec_log(const BYTE *input,unsigned length,BYTE *output,unsigned result,void *caller){
    DWORD error=GetLastError();unsigned hyphens=0,questions=0,used=0;
    if(length==(unsigned)-1){length=0;while(length<1024*1024&&readable(input+length,1)&&input[length])length++;}
    if(length<=1024*1024&&readable(input,length))for(unsigned i=0;i+2<length;i++)if(input[i]==0xe2&&input[i+1]==0x80&&input[i+2]==0x90)hyphens++;
    if(readable(output,24)){
        unsigned n=*(unsigned *)(output+16),capacity=*(unsigned *)(output+20);
        BYTE *data=capacity<16?output:*(BYTE **)output;
        if(n<=capacity&&n<=1024*1024&&readable(data,n)){used=n;for(unsigned i=0;i<n;i++)if(data[i]=='?')questions++;}
    }
    char line[256];snprintf(line,sizeof(line),"stage=legacy-codec caller_rva=0x%08lx input_bytes=%u input_u2010=%u output_bytes=%u output_questions=%u result=%u",
        (unsigned long)((uintptr_t)caller-(uintptr_t)unicode_base),length,hyphens,used,questions,result&255);unicode_write(line);SetLastError(error);
}
__attribute__((naked,noinline)) static void metadata_codec_entry(void){
    __asm__("push %ebp\nmov %esp,%ebp\npush 16(%ebp)\npush 12(%ebp)\npush 8(%ebp)\ncall *_metadata_codec_original\nadd $12,%esp\n"
        "pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 4(%ebp)\npush 28(%ebx)\npush 16(%ebp)\npush 12(%ebp)\npush 8(%ebp)\ncall _metadata_codec_log\nadd $20,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\nmov %ebp,%esp\npop %ebp\nret\n");
}
static void install_metadata_source_trace(BYTE *base){
    if(!GetPrivateProfileIntW(L"diagnostics",L"metadata_source_trace",0,config_path))return;
    static const BYTE artist[]={0x8d,0x81,0x6c,0x02,0,0},title[]={0x8d,0x81,0x6c,0x01,0,0};
    static const BYTE mp3[]={0x8b,0x45,0x0c,0x8b,0x4c,0x24,0x10},codec[]={0x55,0x8b,0xec,0x83,0xec,0x0c};
    struct {DWORD rva;const BYTE *bytes;unsigned size;void *entry;void **original;} hooks[]={
        {0x4f0d0,artist,sizeof(artist),metadata_artist_entry,&metadata_artist_original},
        {0x4f0c0,title,sizeof(title),metadata_title_entry,&metadata_title_original},
        {0xf8a5a,mp3,sizeof(mp3),metadata_mp3_entry,&metadata_mp3_original},
        {0x1c2500,codec,sizeof(codec),metadata_codec_entry,&metadata_codec_original}
    };
    for(unsigned i=0;i<4;i++){
        BOOL ok=unicode_detour(base+hooks[i].rva,hooks[i].bytes,hooks[i].size,hooks[i].entry,hooks[i].original);
        char line[128];snprintf(line,sizeof(line),"%s metadata source trace rva=0x%08lx.",ok?"Installed":"REFUSED",(unsigned long)hooks[i].rva);unicode_write(line);
    }
}
