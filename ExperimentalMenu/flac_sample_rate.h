/* Extend only the original FLAC decode call. The original mono PCM/metadata
 * decoder and original Ogg-path resampler retain their allocators and DSP.
 * Supported original SHA-256 and exact ABI evidence: FLAC-SAMPLE-RATE.md. */
#include <limits.h>

typedef void *(__cdecl *FlacNewFn)(unsigned);
typedef void (__cdecl *FlacFreeFn)(void *);
static void *flac_decode_original,*flac_create_original,*flac_process_original;
static FlacNewFn flac_new_array;
static FlacFreeFn flac_delete_array,flac_free;
static BOOL flac_rate_installed;

/* Original x86 routines use ECX/EDX with CALLER-popped stack arguments,
 * unlike MS __fastcall. The constructor also takes ratio in XMM1/XMM2.
 * These shims preserve the original ABI; their public side is ordinary cdecl. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
__attribute__((naked,noinline)) static unsigned flac_call_decode(void *fn,const BYTE *input,unsigned bytes,
    void *a0,float **pcm,int *frames,char *artist,void *a4,char *title,void *a6){
    __asm__("push %ebp\nmov %esp,%ebp\n"
        "push 44(%ebp)\npush 40(%ebp)\npush 36(%ebp)\npush 32(%ebp)\n"
        "push 28(%ebp)\npush 24(%ebp)\npush 20(%ebp)\n"
        "mov 12(%ebp),%ecx\nmov 16(%ebp),%edx\ncall *8(%ebp)\n"
        "add $28,%esp\npop %ebp\nret\n");
}
__attribute__((naked,noinline)) static void *flac_call_create(void *fn,double ratio){
    __asm__("movsd 8(%esp),%xmm1\nmovapd %xmm1,%xmm2\njmp *4(%esp)\n");
}
__attribute__((naked,noinline)) static int flac_call_process(void *fn,void *state,const float *input,
    int frames,int last,int *used,float *output,int capacity,double ratio){
    __asm__("push %ebp\nmov %esp,%ebp\n"
        "push 44(%ebp)\npush 40(%ebp)\npush 36(%ebp)\npush 32(%ebp)\n"
        "push 28(%ebp)\npush 24(%ebp)\npush 20(%ebp)\n"
        "mov 12(%ebp),%ecx\nmov 16(%ebp),%edx\ncall *8(%ebp)\n"
        "add $28,%esp\npop %ebp\nret\n");
}
#pragma GCC diagnostic pop
static unsigned flac_stream_rate(const BYTE *data,unsigned bytes){
    if(bytes>=10&&!memcmp(data,"ID3",3)){
        if((data[6]|data[7]|data[8]|data[9])&128)return 0;
        unsigned skip=10+((unsigned)data[6]<<21)+((unsigned)data[7]<<14)
            +((unsigned)data[8]<<7)+data[9];
        if(data[3]==4&&(data[5]&16))skip+=10;
        if(skip>bytes)return 0;
        data+=skip;bytes-=skip;
    }
    if(bytes<42||memcmp(data,"fLaC",4)||(data[4]&127)!=0
        ||data[5]!=0||data[6]!=0||data[7]!=34)return 0;
    return ((unsigned)data[18]<<12)|((unsigned)data[19]<<4)|(data[20]>>4);
}
static void flac_release_resampler(void *state){
    if(!state)return;
    /* Same five malloc/free pairs as original Ogg cleanup at 0x004f8d65. */
    void **p=state;flac_free(p[11]);flac_free(p[16]);flac_free(p[0]);flac_free(p[1]);flac_free(state);
}
static BOOL flac_resample(float **pcm,int *frames,unsigned rate){
    if(rate==44100)return TRUE;
    if(!rate||rate<8000||rate>384000||!pcm||!*pcm||!frames||*frames<=0)return FALSE;
    const int input_frames=*frames;
    /* Match the original Ogg ratio's float division before conversion to double. */
    const double ratio=(double)(44100.f/(float)rate);
    const double expected=(double)input_frames*ratio;
    if(expected>INT_MAX/4-256)return FALSE;
    const int capacity=(int)expected+256;
    void *state=flac_call_create(flac_create_original,ratio);
    if(!state)return FALSE;
    float *converted=flac_new_array((unsigned)capacity*sizeof(float));
    if(!converted){flac_release_resampler(state);return FALSE;}
    int consumed=0,produced=0;BOOL ok=TRUE;
    /* Flush EOF through the same original resampler; never shorten the source
     * buffer or merely relabel its duration. Keep the old buffer until success. */
    for(unsigned calls=0;;calls++){
        int remaining=input_frames-consumed,chunk=remaining>32768?32768:remaining;
        int used=0,space=capacity-produced;
        if(calls>(unsigned)input_frames/1024+1024||space<=0){ok=FALSE;break;}
        int count=flac_call_process(flac_process_original,state,*pcm+consumed,chunk,
            chunk==remaining,&used,converted+produced,space,ratio);
        if(count<0||count>space||used<0||used>chunk){ok=FALSE;break;}
        consumed+=used;produced+=count;
        if(!used&&!count){if(consumed!=input_frames)ok=FALSE;break;}
    }
    flac_release_resampler(state);
    if(!ok||!produced){flac_delete_array(converted);return FALSE;}
    flac_delete_array(*pcm);*pcm=converted;*frames=produced;
    char message[160];snprintf(message,sizeof(message),"FLAC analysis: %u Hz -> 44100 Hz; frames %d -> %d (original resampler).",rate,input_frames,produced);log_line(message);
    return TRUE;
}
__attribute__((used,noinline)) static unsigned __cdecl flac_decode_bridge(const BYTE *input,unsigned bytes,
    void *a0,float **pcm,int *frames,char *artist,void *a4,char *title,void *a6){
    unsigned rate=flac_stream_rate(input,bytes);
    unsigned result=flac_call_decode(flac_decode_original,input,bytes,a0,pcm,frames,artist,a4,title,a6);
    if(!(result&255)||rate==44100)return result;
    if(!flac_resample(pcm,frames,rate)){
        log_line("FLAC analysis: resampling failed; refusing incorrect track analysis.");return 0;
    }
    return result;
}
__attribute__((naked,noinline)) static void flac_decode_entry(void){
    __asm__("push %ebp\nmov %esp,%ebp\n"
        "push 32(%ebp)\npush 28(%ebp)\npush 24(%ebp)\npush 20(%ebp)\n"
        "push 16(%ebp)\npush 12(%ebp)\npush 8(%ebp)\npush %edx\npush %ecx\n"
        "call _flac_decode_bridge\nadd $36,%esp\npop %ebp\nret\n");
}
static BOOL flac_bind_original(BYTE *base){
    static const BYTE decode[]={0x55,0x8b,0xec,0x6a,0xff};
    static const BYTE create[]={0x55,0x8b,0xec,0x83,0xec,0x18,0x66,0x0f,0x28,0xc1};
    static const BYTE process[]={0x55,0x8b,0xec,0x83,0xec,0x28,0xf2,0x0f,0x10,0x4d,0x1c};
    static const BYTE site[]={0xe8,0x70,0xb5,0x06,0x00,0x83,0xc4,0x1c};
    if(memcmp(base+0xf9080,decode,sizeof(decode))||memcmp(base+0x1623e0,create,sizeof(create))
        ||memcmp(base+0x162630,process,sizeof(process))||memcmp(base+0x8db0b,site,sizeof(site)))return FALSE;
    flac_decode_original=base+0xf9080;flac_create_original=base+0x1623e0;flac_process_original=base+0x162630;
    flac_new_array=(FlacNewFn)(base+0x4f87c8);
    flac_delete_array=*(FlacFreeFn *)(base+0x52d4a0);
    flac_free=*(FlacFreeFn *)(base+0x52d3a4);
    return flac_delete_array&&flac_free;
}
static BOOL install_flac_sample_rate(BYTE *base){
    if(flac_rate_installed)return TRUE;
    if(!flac_bind_original(base))return FALSE;
    BYTE *site=base+0x8db0b;DWORD protection,ignored;
    if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&protection))return FALSE;
    intptr_t delta=(BYTE *)(uintptr_t)&flac_decode_entry-(site+5);
    int32_t displacement=(int32_t)delta;site[0]=0xe8;memcpy(site+1,&displacement,4);
    FlushInstructionCache(GetCurrentProcess(),site,5);VirtualProtect(site,5,protection,&ignored);
    flac_rate_installed=TRUE;log_line("FLAC analysis sample-rate patch installed (original Ogg resampler).");return TRUE;
}
