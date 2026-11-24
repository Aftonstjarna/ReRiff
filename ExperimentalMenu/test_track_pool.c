/* Verifies where the gaTrackStrip.{ctor} copy lives after relocation.
 *
 * The runtime reads the method body from the mapped PE image, so the copy must
 * stay inside that image. The old .data tail is reused by the game's own static
 * objects and arenas, which overwrote the method header before the first track
 * JIT ("Invalid method header format 1"). The copy now lives in .reloc, whose
 * loader records are dead by the time the client installs. */
#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    BYTE *base=(BYTE *)VirtualAlloc(NULL,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(base);
    *(DWORD *)(base+RERIFF_TRACK_CTOR_RVA_FIELD)=0x271e78;
    /* Poison the old .data window: the relocation must not consult it. */
    memset(base+0x7fd5b8,0x11,RERIFF_TRACK_CTOR_BLOB_SIZE);
    CHECK(race_track_pool_install(base,128));
    CHECK(*(DWORD *)(base+RERIFF_TRACK_CTOR_RVA_FIELD)==0x855000);
    BYTE *body=base+0x855000;
    CHECK((body[0]&3)==3);
    CHECK(!memcmp(body,race_track_ctor_blob,32));
    /* Tier 10 keeps the blob's 512 immediate; other tiers rewrite it. */
    int patched=0;
    for(unsigned i=0;i+5<=RERIFF_TRACK_CTOR_BLOB_SIZE;i++)
        if(body[i]==0x20&&body[i+1]==0x80&&body[i+2]==0&&body[i+3]==0&&body[i+4]==0)patched++;
    CHECK(patched==1);
    CHECK(ReRiffTrackPoolRva()==0x855000);
    CHECK(ReRiffTrackPoolRestore()==1);
    CHECK(*(DWORD *)(base+RERIFF_TRACK_CTOR_RVA_FIELD)==0x271e78);
    CHECK(ReRiffTrackPoolRestore()==0);
    puts("PASS: track ctor copy uses .reloc and restores the original RVA");return 0;
}
