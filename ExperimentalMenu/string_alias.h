/* Custom car name aliases for the stock string manager.
 *
 * The game builds its string token table at startup from the stock tokens.dat;
 * a new Common token written into a composed car archive is not registered in
 * the running table. ReRiff's composer still computes the authored car name
 * (from the donor token), so register that text here and let the garage's
 * Car_<id> lookup resolve through a synthetic handle:
 *   getHandle("Car_<id>") -> STRING_ALIAS_HANDLE + slot
 *   lookup(synthetic handle) -> the registered UTF-8 name
 * Stock names and all other lookups pass through unchanged. */
#include <ctype.h>
#define STRING_ALIAS_MAX 32
#define STRING_ALIAS_HANDLE 0x40000000
typedef struct {char id[4];WCHAR name[256];int active;} StringAlias;
static StringAlias string_aliases[STRING_ALIAS_MAX];
static int string_alias_count;
typedef int (__thiscall *StringGetHandle)(void *,const char *);
typedef const WCHAR *(__thiscall *StringLookup)(void *,int);
static StringGetHandle string_get_original;
static StringLookup string_lookup_original;

__declspec(dllexport) void __cdecl ReRiffClearCarNames(void){string_alias_count=0;}
__declspec(dllexport) int __cdecl ReRiffRegisterCarName(const char *id,const WCHAR *name){
    if(!id||!name||strlen(id)!=3||!*name)return 0;
    int slot=-1;
    for(int i=0;i<string_alias_count;i++)if(!_strnicmp(string_aliases[i].id,id,3)){slot=i;break;}
    if(slot<0){if(string_alias_count>=STRING_ALIAS_MAX)return 0;slot=string_alias_count++;}
    for(int i=0;i<3;i++)string_aliases[slot].id[i]=(char)tolower((unsigned char)id[i]);
    string_aliases[slot].id[3]=0;
    wcsncpy(string_aliases[slot].name,name,255);
    string_aliases[slot].name[255]=0;
    string_aliases[slot].active=1;
    char text[256],line[320];WideCharToMultiByte(CP_UTF8,0,string_aliases[slot].name,-1,text,sizeof(text),NULL,NULL);
    snprintf(line,sizeof(line),"Custom car name alias: %s=%s.",string_aliases[slot].id,text);log_line(line);
    return 1;
}
static BOOL string_alias_match(const char *name,const char *id){
    if(!name||_strnicmp(name,"Car_",4))return FALSE;
    const char *p=name+4;
    return !_strnicmp(p,id,3)&&(p[3]==0||p[3]=='_');
}
__attribute__((used,noinline)) static int __cdecl string_alias_get(void *self,const char *name){
    if(string_alias_count)for(int i=0;i<string_alias_count;i++)
        if(string_aliases[i].active&&string_alias_match(name,string_aliases[i].id))return STRING_ALIAS_HANDLE+i;
    return string_get_original?string_get_original(self,name):-1;
}
__attribute__((used,noinline)) static const WCHAR *__cdecl string_alias_lookup(void *self,int handle){
    int i=handle-STRING_ALIAS_HANDLE;
    if(i>=0&&i<string_alias_count&&string_aliases[i].active)return string_aliases[i].name;
    return string_lookup_original?string_lookup_original(self,handle):L"";
}
__attribute__((naked,noinline)) static void string_get_entry(void){
    __asm__("push 4(%esp)\npush %ecx\ncall _string_alias_get\nadd $8,%esp\nret $4\n");
}
__attribute__((naked,noinline)) static void string_lookup_entry(void){
    __asm__("push 4(%esp)\npush %ecx\ncall _string_alias_lookup\nadd $8,%esp\nret $4\n");
}
static BOOL install_string_alias(BYTE *base){
    static const BYTE lookup_expected[]={0x55,0x8b,0xec,0x8b,0xc1,0x8b,0x4d,0x08};
    static const BYTE get_expected[]={0x55,0x8b,0xec,0x81,0xec,0x04,0x01,0x00,0x00};
    /* Install lookup first: if the getHandle hook is later refused, the hook
     * remains inert (no synthetic handles can exist) instead of leaving a
     * synthetic handle with no resolver. */
    if(!unicode_detour(base+0x25620,lookup_expected,sizeof(lookup_expected),string_lookup_entry,(void **)&string_lookup_original))return FALSE;
    if(!unicode_detour(base+0x25490,get_expected,sizeof(get_expected),string_get_entry,(void **)&string_get_original))return FALSE;
    log_line("Custom car name aliases installed.");
    return TRUE;
}
