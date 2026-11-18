#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE owner[512],row[404],title[512],description[512],line[404],icon[404],lock[404];
static void *rows[39],*children[5];static DWORD friend_length;
static unsigned definitions,texts,last_icon;
static void __thiscall raw_mock(void *item,const WCHAR *text){
    if(item==title){if(wcscmp(text,L"Re:Start"))abort();}
    else if(item==description){if(wcscmp(text,L"Start again"))abort();}else abort();texts++;
}
static void __thiscall definition_mock(void *item,unsigned id){if(item!=icon)abort();definitions++;last_icon=id;}
int main(void){
    BYTE original[100]={0},custom[100],unchanged[100];
    *(float *)(original+8)=*(float *)(original+12)=64;original[32]=7;
    memcpy(unchanged,original,100);
    CHECK(first_prestige_art_descriptor(original,123,456,custom)&&!memcmp(original,unchanged,100));
    CHECK(*(int *)custom==123&&*(int *)(custom+4)==456&&*(float *)(custom+8)==64&&custom[32]==7);
    const unsigned uv_offsets[]={16,36,52,68,84};
    for(unsigned i=0;i<5;i++){float *uv=(float *)(custom+uv_offsets[i]);CHECK(uv[0]==0&&uv[1]==0&&uv[2]==1&&uv[3]==1);}
    CHECK(!first_prestige_art_descriptor(original,-1,456,custom));*(float *)(original+8)=32;CHECK(!first_prestige_art_descriptor(original,123,456,custom));
    BYTE *image=VirtualAlloc(NULL,0x326000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
    CHECK(!first_prestige_entry_install(image));
    memcpy(image+0x325d7b,first_prestige_original_skip,8);CHECK(first_prestige_skip_valid(image));
    CHECK(first_prestige_entry_install(image)&&first_prestige_skip_valid(image));
    for(unsigned i=0;i<8;i++)CHECK(!image[0x325d7b+i]);CHECK(first_prestige_entry_install(image));
    image[0x325d7b]=1;CHECK(!first_prestige_skip_valid(image));image[0x325d7b]=0;
    first_prestige_entry_installed=FALSE;CHECK(!first_prestige_skip_valid(image));first_prestige_entry_installed=TRUE;
    for(unsigned i=0;i<39;i++)rows[i]=row;
    children[0]=title;children[1]=description;children[2]=line;children[3]=icon;children[4]=lock;
    *(void ***)(owner+472)=rows;*(void ***)(owner+476)=rows+39;*(void ***)(owner+480)=rows+39;
    *(void ***)(row+376)=children;*(void ***)(row+380)=children+4;*(void ***)(row+384)=children+5;
    progression_friend_length=&friend_length;raw_text=raw_mock;sprite_def=definition_mock;
    progression_domain_active=TRUE;progression_load_observed=TRUE;
    BYTE before[404];memcpy(before,row,sizeof(row));
    first_prestige_row_update(owner);CHECK(texts==2&&definitions==1&&last_icon==147&&!memcmp(before,row,sizeof(row)));
    first_prestige_row_update(owner);CHECK(texts==4&&definitions==1);
    friend_length=1;first_prestige_row_update(owner);CHECK(texts==4&&definitions==1);friend_length=0;
    progression_domain_active=FALSE;first_prestige_row_update(owner);CHECK(texts==4);progression_domain_active=TRUE;
    progression_load_observed=FALSE;first_prestige_row_update(owner);CHECK(texts==4);progression_load_observed=TRUE;
    first_prestige_row_owner=NULL;*(void ***)(row+380)=children+5;
    first_prestige_row_update(owner);CHECK(texts==6&&definitions==2&&last_icon==186&&children[4]==lock);
    *(void ***)(owner+476)=rows+38;first_prestige_row_update(owner);CHECK(texts==6&&definitions==2);
    BYTE flags[39];memset(flags,1,sizeof(flags));flags[18]=0;CHECK(prestige_all_achievements(flags));flags[17]=0;CHECK(!prestige_all_achievements(flags));
    VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: exact CIL skip guard, in-memory-only patch, native row labels and borrowed earned/locked artwork, repeat/friend/domain guards, original 38 eligibility and unchanged row ownership");return 0;
}
