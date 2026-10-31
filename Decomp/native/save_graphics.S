.intel_syntax noprefix
.code32
.text
.global original_save_graphics
.equ cookie_check,0x8f8958
original_save_graphics:
 push ebp
 {load} mov ebp,esp
 sub esp,0x28
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,ebp
 mov DWORD PTR [ebp-4],eax
 push ebx
 push esi
 mov esi,DWORD PTR [0xbfcd08]
 {load} xor eax,eax
 push edi
 mov edi,DWORD PTR [0xbfcb4c]
 {load} mov ebx,ecx
 {load} cmp edi,esi
 mov DWORD PTR [0xbfcb4c],esi
 push 0x9fc2e8
 cmove edi,eax
 {load} mov ecx,esi
 mov eax,DWORD PTR [esi]
 mov eax,DWORD PTR [eax]
 call eax
 test al,al
 jne .open
 mov eax,DWORD PTR [esi]
 {load} mov ecx,esi
 push 0x24
 push 0x9fc2e8
 mov eax,DWORD PTR [eax+4]
 call eax
 test al,al
 je .failure
.open:
 mov eax,DWORD PTR [esi]
 {load} mov ecx,esi
 push 2
 push 0x9fc2e8
 mov eax,DWORD PTR [eax+8]
 call eax
 test al,al
 je .failure
 mov eax,DWORD PTR [ebx+0x1b8]
 mov ecx,1
 mov DWORD PTR [ebp-0x28],eax
 mov eax,DWORD PTR [ebx+0x1bc]
 mov DWORD PTR [ebp-0x24],eax
 {load} xor eax,eax
 cmp DWORD PTR [ebx+0x1d8],eax
 cmove eax,ecx
 cmp BYTE PTR [ebx+0x4ba],0
 mov DWORD PTR [ebp-0x20],eax
 je .flag2
 or eax,2
 mov DWORD PTR [ebp-0x20],eax
.flag2:
 cmp BYTE PTR [ebx+0xa5d],0
 je .fields
 or eax,4
 mov DWORD PTR [ebp-0x20],eax
.fields:
 mov eax,DWORD PTR [ebx+0x9a4]
 lea ecx,[ebp-0x28]
 mov DWORD PTR [ebp-0x1c],eax
 mov eax,DWORD PTR [0xbfcd0c]
 push 0x24
 push ecx
 push 0
 mov eax,DWORD PTR [eax+0x260]
 {load} mov ecx,esi
 mov DWORD PTR [ebp-0x18],eax
 mov eax,DWORD PTR [0xbfcd00]
 movss xmm0,DWORD PTR [eax+0x40]
.macro pack_fields base,output
 mov eax,DWORD PTR [ebx+\base+12]
 shl eax,8
 or eax,DWORD PTR [ebx+\base+8]
 shl eax,8
 or eax,DWORD PTR [ebx+\base+4]
 shl eax,8
 or eax,DWORD PTR [ebx+\base]
 mov DWORD PTR [ebp-\output],eax
.endm
 pack_fields 0xa2c,0x10
 pack_fields 0xa3c,0xc
 pack_fields 0xa4c,8
 mov eax,DWORD PTR [esi]
 movss DWORD PTR [ebp-0x14],xmm0
 mov eax,DWORD PTR [eax+0x1c]
 call eax
 test al,al
 {load} mov ecx,esi
 mov eax,DWORD PTR [esi]
 jne .success
 call DWORD PTR [eax+0xc]
.failure:
 test edi,edi
 cmovne esi,edi
 {load} xor al,al
 jmp .done
.success:
 call DWORD PTR [eax+0xc]
 test edi,edi
 mov al,1
 cmovne esi,edi
.done:
 mov ecx,DWORD PTR [ebp-4]
 pop edi
 mov DWORD PTR [0xbfcb4c],esi
 {load} xor ecx,ebp
 pop esi
 pop ebx
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret
