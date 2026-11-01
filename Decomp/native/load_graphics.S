.intel_syntax noprefix
.code32
.text
.global original_load_graphics
.equ viewport_size,0x4a22e0
.equ fn_528f80,0x528f80
.equ cookie_check,0x8f8958
original_load_graphics:
 push ebp
 {load} mov ebp,esp
 sub esp,0x2c
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,ebp
 mov DWORD PTR [ebp-4],eax
 push ebx
 push esi
 mov esi,DWORD PTR [0x92d550]
 {load} mov ebx,ecx
 push edi
 push 0x10
 call esi
 mov edi,0x8000
 test di,ax
 je .open
 push 0x11
 call esi
 test di,ax
 jne .failure
.open:
 mov esi,DWORD PTR [0xbfcd08]
 {load} xor eax,eax
 mov edi,DWORD PTR [0xbfcb4c]
 {load} mov ecx,esi
 {load} cmp edi,esi
 mov DWORD PTR [0xbfcb4c],esi
 push 1
 cmove edi,eax
 mov eax,DWORD PTR [esi]
 push 0x9fc2e8
 mov eax,DWORD PTR [eax+8]
 call eax
 test al,al
 je .restore
 lea ecx,[ebp-0x2c]
 mov DWORD PTR [ebp-0x2c],0
 mov eax,DWORD PTR [esi]
 push ecx
 push 0x24
 lea ecx,[ebp-0x28]
 mov eax,DWORD PTR [eax+0x18]
 push ecx
 push 0
 {load} mov ecx,esi
 call eax
 test al,al
 je .close_failed
 cmp DWORD PTR [ebp-0x2c],0x24
 jne .close_failed
 mov eax,DWORD PTR [esi]
 {load} mov ecx,esi
 call DWORD PTR [eax+0xc]
 mov ecx,DWORD PTR [ebp-0x28]
 cmp ecx,30000
 ja .invalid
 mov eax,DWORD PTR [ebp-0x24]
 cmp eax,30000
 ja .invalid
 push eax
 push ecx
 {load} mov ecx,ebx
 call viewport_size
 mov ecx,DWORD PTR [ebp-0x20]
 {load} mov al,cl
 and al,1
 mov BYTE PTR [ebx+0x4bf],al
 {load} mov eax,ecx
 shr eax,1
 and al,1
 shr ecx,2
 mov BYTE PTR [ebx+0x4ba],al
 and cl,1
 mov eax,DWORD PTR [ebp-0x1c]
 mov DWORD PTR [ebx+0x9a4],eax
 mov eax,DWORD PTR [0xbfcd0c]
 mov BYTE PTR [ebx+0xa5d],cl
 mov ecx,DWORD PTR [ebp-0x18]
 push 1
 mov DWORD PTR [eax+0x260],ecx
 movss xmm1,DWORD PTR [ebp-0x14]
 mov ecx,DWORD PTR [0xbfcd00]
 call fn_528f80
 mov ecx,DWORD PTR [ebp-0x10]
 test edi,edi
 movzx eax,cl
 mov DWORD PTR [ebx+0xa2c],eax
 cmovne esi,edi
 movzx eax,ch
 mov ecx,DWORD PTR [ebp-0xc]
 mov DWORD PTR [ebx+0xa30],eax
 movzx eax,BYTE PTR [ebp-0xe]
 mov DWORD PTR [ebx+0xa34],eax
 movzx eax,BYTE PTR [ebp-0xd]
 mov DWORD PTR [ebx+0xa38],eax
 movzx eax,cl
 mov DWORD PTR [ebx+0xa3c],eax
 movzx eax,ch
 mov ecx,DWORD PTR [ebp-8]
 mov DWORD PTR [ebx+0xa40],eax
 movzx eax,BYTE PTR [ebp-0xa]
 mov DWORD PTR [ebx+0xa44],eax
 movzx eax,BYTE PTR [ebp-9]
 mov DWORD PTR [ebx+0xa48],eax
 movzx eax,cl
 mov DWORD PTR [ebx+0xa4c],eax
 movzx eax,ch
 mov DWORD PTR [ebx+0xa50],eax
 movzx eax,BYTE PTR [ebp-6]
 mov DWORD PTR [ebx+0xa54],eax
 movzx eax,BYTE PTR [ebp-5]
 mov DWORD PTR [ebx+0xa58],eax
 mov al,1
 pop edi
 mov DWORD PTR [0xbfcb4c],esi
 pop esi
 pop ebx
 mov ecx,DWORD PTR [ebp-4]
 {load} xor ecx,ebp
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret
.close_failed:
 mov eax,DWORD PTR [esi]
 {load} mov ecx,esi
 call DWORD PTR [eax+0xc]
.invalid:
 mov eax,DWORD PTR [esi]
 {load} mov ecx,esi
 push 0x9fc2e8
 call DWORD PTR [eax+0x14]
.restore:
 test edi,edi
 cmovne esi,edi
 mov DWORD PTR [0xbfcb4c],esi
.failure:
 mov ecx,DWORD PTR [ebp-4]
 {load} xor al,al
 pop edi
 pop esi
 {load} xor ecx,ebp
 pop ebx
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret
