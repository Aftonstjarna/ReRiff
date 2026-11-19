#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
 BYTE *image=VirtualAlloc(NULL,0x310000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(image);
 BYTE *method=image+0x30d81c,*decoder=image+0x30a294;CHECK(!install_friend_utf8(image));
 memcpy(method,friend_utf8_original,sizeof(friend_utf8_original));memcpy(decoder,friend_utf8_decoder,sizeof(friend_utf8_decoder));
 for(unsigned i=0;i<sizeof(friend_utf8_original);i++){method[i]^=1;CHECK(!install_friend_utf8(image));method[i]^=1;}
 for(unsigned i=0;i<sizeof(friend_utf8_decoder);i++){decoder[i]^=1;CHECK(!install_friend_utf8(image));decoder[i]^=1;}
 CHECK(install_friend_utf8(image));CHECK(install_friend_utf8(image));
 for(unsigned i=0;i<sizeof(friend_utf8_original);i++)CHECK(method[i]==(i>=12+0x1b4&&i<12+0x1be?friend_utf8_patch[i-12-0x1b4]:friend_utf8_original[i]));
 friend_utf8_installed=FALSE;CHECK(!install_friend_utf8(image));VirtualFree(image,0,MEM_RELEASE);
 puts("PASS: exact friend/decoder guards, scoped patch, idempotence and foreign-patch refusal");return 0;
}
