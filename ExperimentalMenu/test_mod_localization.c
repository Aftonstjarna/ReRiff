#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static unsigned calls,last_mask;static int last_language;static void *last_manager;
static void __thiscall fixture_unload(void *self,unsigned mask){calls=1;last_manager=self;last_mask=mask;*(unsigned *)((BYTE *)self+12)=0;}
static void __thiscall fixture_load(void *self,unsigned mask,int language){if(calls!=1||self!=last_manager||mask!=last_mask)calls=99;else calls=2;last_language=language;*(unsigned *)((BYTE *)self+12)=mask;}
int main(void){
 BYTE manager[16]={0};*(int *)manager=5;*(unsigned *)(manager+12)=3;
 CHECK(mods_refresh_strings(manager,NULL,fixture_unload,fixture_load));CHECK(calls==2&&last_mask==3&&last_language==5&&*(unsigned *)(manager+12)==3);
 calls=0;CHECK(!mods_refresh_strings(manager,manager,fixture_unload,fixture_load));CHECK(!calls);
 CHECK(!mods_refresh_strings(NULL,NULL,fixture_unload,fixture_load));
 *(int *)manager=20;CHECK(!mods_refresh_strings(manager,NULL,fixture_unload,fixture_load));
 *(int *)manager=0;*(unsigned *)(manager+12)=0;CHECK(!mods_refresh_strings(manager,NULL,fixture_unload,fixture_load));
 *(unsigned *)(manager+12)=8;CHECK(!mods_refresh_strings(manager,NULL,fixture_unload,fixture_load));CHECK(!calls);
 *(unsigned *)(manager+12)=1;CHECK(mods_refresh_strings(manager,NULL,fixture_unload,fixture_load));CHECK(calls==2&&last_mask==1&&last_language==0);
 puts("PASS native unload/load order, preserved language/banks, startup and invalid-state guards");return 0;
}
