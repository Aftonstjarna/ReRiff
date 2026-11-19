#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
 BYTE manager[796]={0},save[64]={0},settings[328]={0},progression[296]={0},before[796];char text[8192];
 CHECK(!car_trace_capacity(NULL,text,sizeof(text)));
 *(int *)(manager+792)=22;strcpy((char *)manager+264,"Pulse");*(DWORD *)(manager+280)=5;*(DWORD *)(manager+284)=15;
 DWORD *v=(DWORD *)manager;v[0]=0x10000;v[1]=v[0]+176*9;v[2]=v[1];memcpy(before,manager,sizeof(manager));
 CHECK(car_trace_capacity(manager,text,sizeof(text)));CHECK(strstr(text,"configured=22")&&strstr(text,"name:\"Pulse\"")&&strstr(text,"packs:9")&&strstr(text,"slot[21]"));CHECK(!memcmp(before,manager,sizeof(manager)));
 CHECK(!car_trace_capacity(manager,text,16));
 *(int *)(manager+792)=23;CHECK(car_trace_capacity(manager,text,sizeof(text)));CHECK(strstr(text,"UNSAFE_COUNT")&&!strstr(text,"slot["));
 *(int *)(manager+792)=-1;CHECK(car_trace_capacity(manager,text,sizeof(text))&&strstr(text,"UNSAFE_COUNT"));
 *(int *)(manager+792)=1;v[1]++;CHECK(car_trace_capacity(manager,text,sizeof(text))&&strstr(text,"vector_valid:0"));
 v[1]=v[0]+176*10;v[2]=v[1];CHECK(car_trace_capacity(manager,text,sizeof(text))&&strstr(text,"UNSAVABLE_PACKS"));
 *(DWORD *)(manager+280)=100;CHECK(car_trace_capacity(manager,text,sizeof(text))&&strstr(text,"<unreadable>"));
 *(void **)(save+16)=settings;*(void **)(save+32)=progression;
 *(int *)(settings+324)=21;*(int *)(settings+236+21*4)=8;progression[53+21/8]|=1<<(21%8);*(WORD *)(progression+56+21*2)=0x101;*(int *)(progression+188+21*4)=7;
 settings[4]=1;progression[4]=1;CHECK(car_trace_save(save,text,sizeof(text)));
 CHECK(strstr(text,"selected=21 valid_index=1")&&strstr(text,"slot[21]={paint:8,paint_in_save_range:1,unlocked:1,pack_mask:101,driven:7}"));
 *(int *)(settings+324)=22;CHECK(car_trace_save(save,text,sizeof(text))&&strstr(text,"selected=22 valid_index=0"));
 *(void **)(save+16)=NULL;CHECK(!car_trace_save(save,text,sizeof(text)));
 puts("PASS: bounded 22-car/9-pack snapshots, invalid counts/vectors, selection and unlock fields, read-only capacity and output bounds");return 0;
}
