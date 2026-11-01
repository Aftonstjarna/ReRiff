.intel_syntax noprefix
.code32
.text
.global original_save_device_close
original_save_device_close:
 push esi
 {load} mov esi,ecx
 mov eax,DWORD PTR [esi+16]
 test eax,eax
 je .done
 push eax
 mov DWORD PTR [esi+12],0
 call DWORD PTR [0x92d470]
 add esp,4
 mov DWORD PTR [esi+16],0
.done:
 mov al,1
 pop esi
 ret
