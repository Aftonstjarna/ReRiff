#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    BYTE menu[512]={0},ui[32]={0},row[404]={0},definitions[18800]={0};
    void *rows[38],*children[4]={NULL,NULL,NULL,NULL};
    for(unsigned i=0;i<38;i++)rows[i]=row;
    *(void ***)(menu+472)=rows;*(void ***)(menu+476)=rows+38;*(void ***)(menu+480)=rows+38;
    *(void ***)(row+376)=children;*(void ***)(row+380)=children+4;*(void ***)(row+384)=children+4;
    *(float *)(row+32)=15;*(float *)(row+36)=30;*(float *)(row+48)=1400;*(float *)(row+52)=154;
    *(void **)(ui+12)=definitions;*(unsigned *)(ui+16)=188;
    const unsigned ids[]={147,148,149,187};
    for(unsigned i=0;i<4;i++){
        BYTE *d=definitions+ids[i]*100;*(int *)d=42;*(int *)(d+4)=84+i;*(float *)(d+8)=128;*(float *)(d+12)=128;
    }
    BYTE before_menu[512],before_ui[32],before_row[404],before_definitions[18800];
    memcpy(before_menu,menu,sizeof(menu));memcpy(before_ui,ui,sizeof(ui));memcpy(before_row,row,sizeof(row));memcpy(before_definitions,definitions,sizeof(definitions));
    char text[2048];CHECK(achievement_probe_snapshot(menu,ui,text,sizeof(text)));
    CHECK(strstr(text,"original_rows=38")&&strstr(text,"row[18]={children:4")&&strstr(text,"sprite[148]={resource:42,texture:85,w:128.00,h:128.00"));
    CHECK(!memcmp(before_menu,menu,sizeof(menu))&&!memcmp(before_ui,ui,sizeof(ui))&&!memcmp(before_row,row,sizeof(row))&&!memcmp(before_definitions,definitions,sizeof(definitions)));
    CHECK(!achievement_probe_snapshot(menu,ui,text,255));
    CHECK(!achievement_probe_snapshot(menu,ui,text,600));
    *(void ***)(menu+476)=rows+37;CHECK(!achievement_probe_snapshot(menu,ui,text,sizeof(text)));*(void ***)(menu+476)=rows+38;
    *(void ***)(menu+480)=rows+37;CHECK(!achievement_probe_snapshot(menu,ui,text,sizeof(text)));*(void ***)(menu+480)=rows+38;
    *(unsigned *)(ui+16)=187;CHECK(!achievement_probe_snapshot(menu,ui,text,sizeof(text)));*(unsigned *)(ui+16)=188;
    *(void **)(ui+12)=NULL;CHECK(!achievement_probe_snapshot(menu,ui,text,sizeof(text)));
    CHECK(!achievement_probe_snapshot(NULL,ui,text,sizeof(text)));
    puts("PASS: native achievement row/descriptor read-only snapshot, unchanged buffers, malformed bounds/count/null and short-output refusal");return 0;
}
