#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int seen_delta;
static int __cdecl price_mock(void *paint,int car,int pack){if(paint!=(void *)7||car!=2||pack!=3)abort();return 733;}
static void __cdecl coins_mock(void *chunk,int delta){if(chunk!=(void *)9)abort();seen_delta=delta;}
int main(void){
    void *callbacks[2]={(void *)price_mock,(void *)coins_mock};
    CHECK(!ReRiffConfigureProgressionEconomy(callbacks,2,1));
    CHECK(ReRiffConfigureProgressionEconomy(callbacks,2,0)&&!economy_validated);
    void *price_slot=(void *)7;economy_price_dispatch(&price_slot,(void *)2,3);CHECK(price_slot==(void *)733);
    economy_coins_dispatch((void *)9,(void *)(intptr_t)-7);CHECK(seen_delta==-7);
    CHECK(ReRiffConfigureProgressionEconomy(callbacks,2,1)&&economy_validated);
    CHECK(ReRiffGetProgressionEconomyThunk(0)==(void *)economy_price_dispatch&&ReRiffGetProgressionEconomyThunk(2)==NULL);
    BYTE *image=VirtualAlloc(NULL,0x320000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
    BYTE *price=image+0x3196c8,*coin=image+0x2fc948;
    memcpy(price,economy_price_header,12);memcpy(price+12,economy_price_body,20);
    memcpy(coin,economy_coin_header,12);memcpy(coin+12,economy_coin_body,56);
    price[12]=0;CHECK(!install_progression_economy(image)&&!memcmp(coin+12,economy_coin_body,56));price[12]=economy_price_body[0];
    CHECK(install_progression_economy(image)&&economy_installed);
    CHECK(*(WORD *)(price+2)==4&&*(DWORD *)(price+4)==20&&*(DWORD *)(price+8)==0);
    CHECK(!memcmp(coin,economy_coin_header,12)); /* Original locals, size and flags. */
    CHECK(price[12]==0x0f&&price[13]==0&&price[24]==0x29&&*(DWORD *)(price+25)==0x1100050d&&price[31]==0x2a);
    CHECK(price[29]==0x02&&price[30]==0x69);
    CHECK(coin[22]==0x29&&*(DWORD *)(coin+23)==0x1100050c&&coin[27]==0x2a);
    for(int i=28;i<67;i++)CHECK(coin[i]==0);CHECK(coin[67]==0x2a);
    CHECK(!ReRiffConfigureProgressionEconomy(callbacks,2,0));
    VirtualFree(image,0,MEM_RELEASE);puts("PASS: economy thunk ABI forwarding, validated registration, exact original-byte guards, unchanged IL sizes/locals, price MaxStack and padded wrapper tokens");return 0;
}
