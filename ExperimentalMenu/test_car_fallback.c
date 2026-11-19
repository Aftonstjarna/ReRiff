#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static void name(BYTE *manager,int slot,const char *id){
    BYTE *p=manager+264+24*slot;memset(p,0,24);strcpy((char *)p,id);*(unsigned *)(p+16)=3;*(unsigned *)(p+20)=15;
}
int main(void){
    BYTE manager[796]={0},settings[328],before[328];memset(settings,0xa5,sizeof(settings));settings[5]=1;
    *(int *)(manager+792)=15;name(manager,0,"alf");name(manager,5,"fab");*(int *)(settings+324)=15;
    memcpy(before,settings,sizeof(settings));CHECK(car_fallback_target(manager,settings,"alf")==0);
    CHECK(!memcmp(before,settings,sizeof(settings))); /* Planning cannot modify saves. */
    CHECK(car_fallback_target(manager,settings,"fab")==5);
    CHECK(car_fallback_target(manager,settings,"dod")==-1); /* Must be an original base. */
    CHECK(car_fallback_target(manager,settings,"pl1")==-1); /* Base must actually exist. */
    name(manager,1,"alf");CHECK(car_fallback_target(manager,settings,"alf")==-1);name(manager,1,"bui");
    *(int *)(manager+792)=16;name(manager,15,"dod");CHECK(car_fallback_target(manager,settings,"alf")==-1);
    name(manager,15,"fab");CHECK(car_fallback_target(manager,settings,"alf")==-1); /* Reused active slot is not proof of removal. */
    *(int *)(manager+792)=15;settings[5]=0;CHECK(car_fallback_target(manager,settings,"alf")==-1);settings[5]=1;
    for(int i=-1;i<15;i++){*(int *)(settings+324)=i;CHECK(car_fallback_target(manager,settings,"alf")==-1);}
    *(int *)(settings+324)=22;CHECK(car_fallback_target(manager,settings,"alf")==-1);
    *(int *)(settings+324)=21;CHECK(car_fallback_target(manager,settings,"alf")==0);
    *(int *)(manager+792)=23;CHECK(car_fallback_target(manager,settings,"alf")==-1);
    *(int *)(manager+792)=14;CHECK(car_fallback_target(manager,settings,"alf")==-1);
    CHECK(car_fallback_target(NULL,settings,"alf")==-1);CHECK(car_fallback_target(manager,NULL,"alf")==-1);
    puts("PASS: missing custom selection resolves to installed stock base; installed/reused slots, invalid states and ambiguous origins refused; planning preserves saves");return 0;
}
