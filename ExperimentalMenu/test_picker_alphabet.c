#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static void name(BYTE *entry,const WCHAR *value){size_t length=wcslen(value);memset(entry,0,28);if(length<8){memcpy(entry,value,(length+1)*2);*(DWORD *)(entry+20)=7;}else{*(const WCHAR **)entry=value;*(DWORD *)(entry+20)=(DWORD)length;}*(DWORD *)(entry+16)=(DWORD)length;}
static int calls,called_letter;static void *called_menu;
static int __cdecl jump_test(void *menu,int letter){calls++;called_menu=menu;called_letter=letter;return 1;}
int main(void){
    DWORD letters=(1u<<0)|(1u<<2)|(1u<<25);
    CHECK(picker_alpha_target(letters,'A',1)=='C');CHECK(picker_alpha_target(letters,'C',1)=='Z');CHECK(!picker_alpha_target(letters,'Z',1));
    CHECK(picker_alpha_target(letters,'Z',-1)=='C');CHECK(picker_alpha_target(letters,'C',-1)=='A');CHECK(!picker_alpha_target(letters,'A',-1));
    CHECK(picker_alpha_target(letters,0,1)=='A'&&picker_alpha_target(letters,0,-1)=='Z');CHECK(!picker_alpha_target(0,'A',1));CHECK(!picker_alpha_target(letters,'A',0));
    BOOL armed=FALSE;CHECK(!picker_alpha_edge(&armed,0,1));CHECK(!picker_alpha_edge(&armed,0,0)&&armed);
    CHECK(picker_alpha_edge(&armed,0,1)==1&&!armed);for(int i=0;i<100;i++)CHECK(!picker_alpha_edge(&armed,0,1));
    CHECK(!picker_alpha_edge(&armed,0,.3f)&&!armed);CHECK(!picker_alpha_edge(&armed,0,.1f)&&armed);
    CHECK(picker_alpha_edge(&armed,1,0)==-1);CHECK(!picker_alpha_edge(&armed,0,0)&&armed);CHECK(!picker_alpha_edge(&armed,1,1)&&!armed);
    CHECK(!picker_alpha_edge(&armed,0,1));CHECK(!picker_alpha_edge(&armed,0,0));CHECK(!picker_alpha_edge(&armed,-1,0)&&!armed);
    BYTE menu[424]={0},screen[32]={0},entry[3][28],focus[404]={0};void *rows[3]={NULL,NULL,NULL},*table[2]={0},**reel=table;
    reel_vtable=table;name(entry[0],L"afton");name(entry[1],L"BUCK-TICK");name(entry[2],L"cello");
    *(void **)(menu+184)=screen;*(void **)(menu+248)=&reel;
    *(BYTE **)(menu+276)=entry[0];*(BYTE **)(menu+280)=entry[0]+sizeof(entry);*(BYTE **)(menu+284)=entry[0]+sizeof(entry);
    *(void ***)(menu+324)=rows;*(void ***)(menu+328)=rows+3;*(void ***)(menu+332)=rows+3;
    *(void **)(screen+20)=focus;*(int *)(focus+8)=6;*(DWORD *)(menu+28)=4;
    BYTE original[sizeof(entry)];memcpy(original,entry,sizeof(entry));PickerAlphaContext context;
    CHECK(picker_alpha_context(menu,&context)&&context.count==3&&context.focus==6&&context.current=='C'&&context.phase==4);
    CHECK(picker_alpha_letters(&context)==7&&!memcmp(original,entry,sizeof(entry)));
    name(entry[0],L"月下麗人");CHECK(picker_alpha_letter(entry[0])==0);name(entry[0],L"123 music");CHECK(!picker_alpha_letter(entry[0]));
    *(void **)(screen+20)=NULL;CHECK(picker_alpha_context(menu,&context)&&context.current==0&&context.focus==-1);
    *(BYTE **)(menu+280)=entry[0]+sizeof(entry)-1;CHECK(!picker_alpha_context(menu,&context));*(BYTE **)(menu+280)=entry[0]+sizeof(entry);
    *(void ***)(menu+328)=rows+2;CHECK(!picker_alpha_context(menu,&context));
    picker_alpha_owner=menu;picker_alpha_armed=TRUE;picker_alpha_logged=TRUE;picker_alpha_forget(menu);CHECK(!picker_alpha_owner&&!picker_alpha_armed&&!picker_alpha_logged);
    CHECK(PICKER_ALPHA_NAVIGATION==1);CHECK(!ReRiffConfigurePickerAlphabet(NULL));
    CHECK(ReRiffConfigurePickerAlphabet((void *)jump_test));context.count=3;context.focus=4;context.phase=1;
    CHECK(picker_alpha_dispatch(menu,&context,'C')&&calls==1&&called_menu==menu&&called_letter=='C');
    context.phase=0;CHECK(!picker_alpha_dispatch(menu,&context,'A')&&calls==1);
    context.phase=1;context.focus=-1;CHECK(!picker_alpha_dispatch(menu,&context,'A')&&calls==1);
    context.focus=7;CHECK(!picker_alpha_dispatch(menu,&context,'A')&&calls==1);
    context.focus=4;CHECK(!picker_alpha_dispatch(menu,&context,0)&&calls==1);
    puts("PASS: native alphabetical group selection, gaps/ends, neutral-before-press, held/both-trigger refusal, original wide-string layout, source preservation and malformed-row guards; native callback dispatch and inactive/no-focus guards");return 0;
}
