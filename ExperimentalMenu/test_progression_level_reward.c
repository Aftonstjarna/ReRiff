#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int __cdecl quote_mock(int level){return level*100;}
int main(int argc,char **argv){
    if(argc==2){
        WCHAR original[MAX_PATH];CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,original,MAX_PATH));CHECK(supported_file(original));
        HMODULE game=LoadLibraryExW(original,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(game);CHECK(validate_progression_level_quote((BYTE *)game));FreeLibrary(game);
    }
    CHECK(!ReRiffConfigureProgressionLevelQuote(NULL));
    CHECK(ReRiffConfigureProgressionLevelQuote((void *)quote_mock));
    CHECK(((GenerationQuoteFn)ReRiffGetProgressionLevelQuoteThunk())(3)==300);
    BYTE *image=VirtualAlloc(NULL,0x240000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
    BYTE *method=image+0x235758,*argument=method+12+0x1e9;
    memcpy(method,reward_level_header,12);memcpy(argument,reward_level_original,12);
    memcpy(argument-5,"\x7e\x96\x07\x00\x04",5);memcpy(argument+12,"\x28\x18\x00\x00\x0a",5);
    method[2]^=1;CHECK(!install_progression_level_quote(image));method[2]^=1;
    argument[7]^=1;CHECK(!install_progression_level_quote(image));argument[7]^=1;
    argument[12]^=1;CHECK(!install_progression_level_quote(image));argument[12]^=1;
    BYTE before[12+0x2c1];memcpy(before,method,sizeof(before));
    CHECK(install_progression_level_quote(image));
    CHECK(!ReRiffConfigureProgressionLevelQuote((void *)quote_mock));
    CHECK(!memcmp(before,method,12+0x1e9));
    CHECK(!memcmp(before+12+0x1f5,method+12+0x1f5,sizeof(before)-12-0x1f5));
    CHECK(argument[0]==0x25&&argument[1]==0x20&&argument[6]==0xd3&&argument[7]==0x29&&*(DWORD *)(argument+8)==0x11000854);
    CHECK(*(void **)(argument+2)==(void *)reward_level_dispatch);
    CHECK(install_progression_level_quote(image));VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: level-up display-only patch, original header/token guards, existing cdecl signature and unchanged surrounding CIL");return 0;
}
