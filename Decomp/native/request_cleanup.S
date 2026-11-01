.intel_syntax noprefix
.code32
.text
.global original_request_cleanup
original_request_cleanup:
 push esi
 {load} mov esi,ecx
 mov ecx,DWORD PTR [esi+0x440]
 test ecx,ecx
 je .second
 mov eax,DWORD PTR [ecx]
 push ecx
 call DWORD PTR [eax+8]
.second:
 mov ecx,DWORD PTR [esi+0x43c]
 mov DWORD PTR [esi+0x440],0
 test ecx,ecx
 je .third
 mov eax,DWORD PTR [ecx]
 push ecx
 call DWORD PTR [eax+8]
.third:
 mov ecx,DWORD PTR [esi+0x448]
 mov DWORD PTR [esi+0x43c],0
 test ecx,ecx
 je .fourth
 mov eax,DWORD PTR [ecx]
 push ecx
 call DWORD PTR [eax+8]
.fourth:
 mov ecx,DWORD PTR [esi+0x444]
 mov DWORD PTR [esi+0x448],0
 test ecx,ecx
 je .done
 mov eax,DWORD PTR [ecx]
 push ecx
 call DWORD PTR [eax+8]
.done:
 mov DWORD PTR [esi+0x444],0
 mov al,1
 pop esi
 ret
