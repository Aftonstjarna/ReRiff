/* Startup-only CIL wrappers. Original disk bytes, body sizes, local signatures
 * and metadata tokens remain unchanged. The price wrapper needs MaxStack 4
 * instead of 3. The helper extends the preserved original price body with its
 * singleplayer coin-pack policy; coin awards remain trace-only. */
typedef int (__cdecl *EconomyPriceFn)(void *,int,int);
typedef void (__cdecl *EconomyCoinsFn)(void *,int);
static EconomyPriceFn economy_price;
static EconomyCoinsFn economy_coins;
static BOOL economy_validated,economy_installed;
/* Existing cdecl signatures avoid Mono's dynamic-thiscall limitation. The
 * price thunk writes its result into the method's by-value self argument slot;
 * it never changes the caller's pointer or the native paint definitions. */
static void __cdecl economy_price_dispatch(void **paint_slot,void *car,int pack){if(paint_slot&&economy_price){int value=economy_price(*paint_slot,(int)(uintptr_t)car,pack);memcpy(paint_slot,&value,4);}}
static void __cdecl economy_coins_dispatch(void *progression,void *delta){if(economy_coins)economy_coins(progression,(int)(intptr_t)delta);}
__declspec(dllexport) void *__cdecl ReRiffGetProgressionEconomyThunk(int which){return which==0?(void *)economy_price_dispatch:which==1?(void *)economy_coins_dispatch:NULL;}
__declspec(dllexport) int __cdecl ReRiffConfigureProgressionEconomy(void **callbacks,int count,int validated){
    if(economy_installed||!callbacks||count!=2||!callbacks[0]||!callbacks[1]||(validated!=0&&validated!=1))return 0;
    if(validated){if(economy_price!=(EconomyPriceFn)callbacks[0]||economy_coins!=(EconomyCoinsFn)callbacks[1])return 0;economy_validated=TRUE;}
    else{economy_price=(EconomyPriceFn)callbacks[0];economy_coins=(EconomyCoinsFn)callbacks[1];economy_validated=FALSE;}
    return 1;
}
static const BYTE economy_price_header[]={0x03,0x30,0x03,0,0x14,0,0,0,0,0,0,0};
static const BYTE economy_price_body[]={0x04,0x20,0xb0,0,0,0,0x5a,0x03,0x1f,0x0c,0x5a,0x02,0x58,0x4a,0x58,0x1f,0x24,0x58,0x4a,0x2a};
static const BYTE economy_coin_header[]={0x03,0x30,0x03,0,0x38,0,0,0,0x15,0,0,0x11};
static const BYTE economy_coin_body[]={0x02,0x20,0x1c,0x01,0,0,0x58,0x02,0x20,0x1c,0x01,0,0,0x58,0x4a,0x03,0x58,0x54,0x02,0x20,0x1c,0x01,0,0,0x58,0x4a,0x0a,0x02,0x20,0x1c,0x01,0,0,0x58,0x06,0x16,0x32,0x0a,0x02,0x20,0x1c,0x01,0,0,0x58,0x4a,0x2b,0x01,0x16,0x54,0x02,0x1a,0x58,0x17,0x52,0x2a};
static void economy_wrapper(BYTE *body,size_t size,void *target,DWORD signature,BOOL price){
    memset(body,0,size);unsigned i=0;
    if(price){body[i++]=0x0f;body[i++]=0;body[i++]=0xe0;}
    else{body[i++]=0x02;body[i++]=0xd3;}
    body[i++]=0x03;body[i++]=0xd3;if(price)body[i++]=0x04;
    body[i++]=0x20;memcpy(body+i,&target,4);i+=4;body[i++]=0xd3;body[i++]=0x29;
    memcpy(body+i,&signature,4);i+=4;if(price){body[i++]=0x02;body[i++]=0x69;}body[i]=0x2a;body[size-1]=0x2a;
}
static BOOL install_progression_economy(BYTE *base){
    if(!economy_validated)return TRUE;if(economy_installed)return TRUE;
    BYTE *price=base+0x3196c8,*coin=base+0x2fc948;
    if(!readable(price,32)||!readable(coin,68)
        ||memcmp(price,economy_price_header,12)||memcmp(price+12,economy_price_body,20)
        ||memcmp(coin,economy_coin_header,12)||memcmp(coin+12,economy_coin_body,56))return FALSE;
    DWORD price_old,coin_old,ignored;
    if(!VirtualProtect(price,32,PAGE_EXECUTE_READWRITE,&price_old))return FALSE;
    if(!VirtualProtect(coin,68,PAGE_EXECUTE_READWRITE,&coin_old)){VirtualProtect(price,32,price_old,&ignored);return FALSE;}
    *(WORD *)(price+2)=4;
    economy_wrapper(price+12,20,(void *)economy_price_dispatch,0x1100050d,TRUE);
    economy_wrapper(coin+12,56,(void *)economy_coins_dispatch,0x1100050c,FALSE);
    FlushInstructionCache(GetCurrentProcess(),price,32);FlushInstructionCache(GetCurrentProcess(),coin,68);
    VirtualProtect(price,32,price_old,&ignored);VirtualProtect(coin,68,coin_old,&ignored);
    economy_installed=TRUE;log_line("Original price/coin CIL bridges installed before gameplay JIT; policy is owned by the matched helper.");return TRUE;
}
