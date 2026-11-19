#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static BOOL readable(const void *p,size_t n){MEMORY_BASIC_INFORMATION m;return VirtualQuery(p,&m,sizeof(m))==sizeof(m)&&m.State==MEM_COMMIT&&(BYTE*)p+n<=(BYTE*)m.BaseAddress+m.RegionSize;}
static void log_line(const char *s){puts(s);}
#include "customs_unlocks.h"
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void __cdecl fake(void *a,void *b){(void)a;(void)b;}
int main(void){BYTE *base=VirtualAlloc(NULL,0x250000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(base);BYTE*site=base+0x24cf38;CHECK(ReRiffConfigureCustomsUnlocks(fake));CHECK(!install_customs_unlocks(base));memcpy(site,customs_unlock_original,sizeof(customs_unlock_original));site[-1]=0x99;site[sizeof(customs_unlock_original)]=0x88;CHECK(install_customs_unlocks(base));CHECK(site[-1]==0x99&&site[sizeof(customs_unlock_original)]==0x88);CHECK(site[0]==0x12&&site[1]==42&&site[3]==0x12&&site[4]==38&&*(DWORD*)(site+13)==0x1100050c);for(unsigned i=17;i<sizeof(customs_unlock_original);i++)CHECK(site[i]==0);CHECK(!ReRiffConfigureCustomsUnlocks(fake));CHECK(install_customs_unlocks(base));puts("PASS exact-byte guard, scoped IL replacement, local-address operands and original signature");VirtualFree(base,0,MEM_RELEASE);return 0;}
