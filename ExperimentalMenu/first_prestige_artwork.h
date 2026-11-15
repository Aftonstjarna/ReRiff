/* Use the native texture archive loader and existing unused sprite slots.
 * No fonts are needed for the artwork; missing assets retain the placeholder. */
static BOOL first_prestige_art_attempted,first_prestige_art_ready;
static BOOL first_prestige_art_descriptor(const BYTE *original,int resource,int texture,BYTE *output){
    if(!original||!output||resource<0||texture<0)return FALSE;
    float width,height;memcpy(&width,original+8,4);memcpy(&height,original+12,4);
    if(width!=64.f||height!=64.f)return FALSE;
    memcpy(output,original,100);memcpy(output,&resource,4);memcpy(output+4,&texture,4);
    const float uv[4]={0,0,1,1};
    const unsigned offsets[]={16,36,52,68,84};
    for(unsigned i=0;i<5;i++)memcpy(output+offsets[i],uv,sizeof(uv));
    return TRUE;
}
static BOOL first_prestige_artwork(void *ui){
    if(first_prestige_art_attempted)return first_prestige_art_ready;
    first_prestige_art_attempted=TRUE;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);void *graphics=NULL,*definitions=NULL;unsigned count=0;
    static const BYTE ctor[]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
    if(!progression_probe_read(base+0x80d408,&graphics,4)||!readable(graphics,4)
        ||!readable(*(void ***)graphics,31*4)||(*(void ***)graphics)[30]!=base+0x94d60
        ||!readable(base+0x18a90,sizeof(ctor))||memcmp(base+0x18a90,ctor,sizeof(ctor))
        ||!progression_probe_read((BYTE *)ui+12,&definitions,4)
        ||!progression_probe_read((BYTE *)ui+16,&count,4)||count<188||count>4096
        ||!readable(definitions,count*100))return FALSE;
    typedef void *(__thiscall *Token)(int *,const char *);
    typedef BYTE (__thiscall *Dimensions)(void *,const int *,const char *,float *,float *);
    static int resource=-1,texture=-1;
    ((Token)(base+0x18a90))(&resource,"reriff/achievements");
    ((Token)(base+0x18a90))(&texture,"first_prestige");
    float width=0,height=0;
    if(!((Dimensions)(base+0x94d60))(graphics,&resource,"first_prestige",&width,&height)||width!=64||height!=64){
        log_line("First Prestige artwork unavailable; borrowed native icon retained.");return FALSE;
    }
    BYTE descriptor[100];
    if(!first_prestige_art_descriptor((BYTE *)definitions+147*100,resource,texture,descriptor))return FALSE;
    BYTE *earned=(BYTE *)definitions+148*100,*locked=(BYTE *)definitions+187*100;
    DWORD protect,ignored;
    /* Both unused definitions reside in the same native UI descriptor array.
     * Protect the entire interval before any write so failure is atomic. */
    if(!VirtualProtect(earned,(SIZE_T)(locked-earned)+100,PAGE_READWRITE,&protect))return FALSE;
    memcpy(earned,descriptor,100);memcpy(locked,descriptor,100);
    VirtualProtect(earned,(SIZE_T)(locked-earned)+100,protect,&ignored);
    first_prestige_art_ready=TRUE;
    log_line("First Prestige circular artwork loaded through native texture loader: 64x64; existing row geometry retained.");return TRUE;
}
