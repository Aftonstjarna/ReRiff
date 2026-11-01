.intel_syntax noprefix
.code32
.section .read,"ax"
.global original_save_device_read
original_save_device_read:
 push ebp
 {load} mov ebp,esp
 push esi
 {load} mov esi,ecx
 mov eax,DWORD PTR [esi+16]
 test eax,eax
 jne .read_seek
.read_fail:
 mov eax,DWORD PTR [ebp+20]
 pop esi
 mov DWORD PTR [eax],0
 {load} xor al,al
 pop ebp
 ret 16
.read_seek:
 push 0
 push DWORD PTR [ebp+8]
 push eax
 call DWORD PTR [0x92d3c0]
 add esp,12
 test eax,eax
 jne .read_fail
 push DWORD PTR [esi+16]
 push DWORD PTR [ebp+16]
 push 1
 push DWORD PTR [ebp+12]
 call DWORD PTR [0x92d3c8]
 mov ecx,DWORD PTR [ebp+20]
 mov DWORD PTR [ecx],eax
 push DWORD PTR [esi+16]
 call DWORD PTR [0x92d344]
 add esp,20
 test eax,eax
 sete al
 pop esi
 pop ebp
 ret 16
.section .write,"ax"
.global original_save_device_write
original_save_device_write:
 push ebp
 {load} mov ebp,esp
 push esi
 {load} mov esi,ecx
 mov eax,DWORD PTR [esi+16]
 test eax,eax
 jne .write_seek
.write_fail:
 {load} xor al,al
 pop esi
 pop ebp
 ret 12
.write_seek:
 push 0
 push DWORD PTR [ebp+8]
 push eax
 call DWORD PTR [0x92d3c0]
 add esp,12
 test eax,eax
 jne .write_fail
 push DWORD PTR [esi+16]
 mov esi,DWORD PTR [ebp+16]
 push esi
 push 1
 push DWORD PTR [ebp+12]
 call DWORD PTR [0x92d454]
 add esp,16
 {load} cmp eax,esi
 sete al
 pop esi
 pop ebp
 ret 12
