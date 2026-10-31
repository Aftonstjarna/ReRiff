.intel_syntax noprefix
.code32
.text
.global original_secondary_window
.equ cookie_check,0x8f8958
.equ fn_48cab0,0x48cab0
original_secondary_window:
 push ebp
 {load} mov ebp,esp
 sub esp,0x38
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,ebp
 mov DWORD PTR [ebp-8],eax
 mov edx,DWORD PTR [ebp+8]
 push ebx
 {load} mov ebx,ecx
 mov ecx,DWORD PTR [ebp+0xc]
 push esi
 push edi
 test edx,edx
 je .zero
 test ecx,ecx
 jne .size
.zero:
 mov eax,DWORD PTR [ebx+8]
 test eax,eax
 je .size
 push eax
 call DWORD PTR [0x92d530]
 mov DWORD PTR [ebx+8],0
.macro early_return
 pop edi
 pop esi
 pop ebx
 mov ecx,DWORD PTR [ebp-8]
 {load} xor ecx,ebp
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 12
.endm
 early_return
.size:
 cmp BYTE PTR [ebp+0x10],0
 mov eax,0x80000000
 mov esi,0xc00000
 mov DWORD PTR [ebp-0x18],0
 cmovne esi,eax
 mov DWORD PTR [ebp-0x14],0
 push 0
 push esi
 lea eax,[ebp-0x18]
 mov DWORD PTR [ebp-0x30],esi
 push eax
 mov DWORD PTR [ebp-0x10],edx
 mov DWORD PTR [ebp-0xc],ecx
 call DWORD PTR [0x92d580]
 mov eax,DWORD PTR [ebp-0x10]
 sub eax,DWORD PTR [ebp-0x18]
 mov DWORD PTR [ebp-0x2c],eax
 mov eax,DWORD PTR [ebp-0xc]
 sub eax,DWORD PTR [ebp-0x14]
 mov DWORD PTR [ebp-0x34],eax
 mov eax,DWORD PTR [ebx+8]
 test eax,eax
 jne .existing
 lea eax,[ebp-0x28]
 push eax
 push DWORD PTR [ebx+4]
 call DWORD PTR [0x92d524]
 mov eax,DWORD PTR [ebp-0x20]
 mov edi,DWORD PTR [ebp-0x28]
 {load} sub eax,edi
 sub eax,DWORD PTR [ebp-0x2c]
 mov ecx,DWORD PTR [ebp-0x24]
 push ebx
 push DWORD PTR [ebx]
 cdq
 push 0
 push 0
 push DWORD PTR [ebp-0x34]
 {load} sub eax,edx
 mov edx,DWORD PTR [ebp-0x1c]
 push DWORD PTR [ebp-0x2c]
 {load} mov esi,eax
 {load} sub edx,ecx
 sar esi,1
 mov DWORD PTR [ebx+0x1c],esi
 mov DWORD PTR [ebx+0x20],edx
 lea eax,[edx+ecx*1]
 push eax
 lea eax,[esi+edi*1]
 push eax
 push DWORD PTR [ebp-0x30]
 push 0x9fbd68
 push 0x9fbd8c
 push 0
 call DWORD PTR [0x92d578]
 mov DWORD PTR [ebx+8],eax
 test eax,eax
 je .done
 {load} mov ecx,ebx
 call fn_48cab0
 mov eax,DWORD PTR [ebx+0x20]
 add eax,DWORD PTR [ebp-0x24]
 push 5
 push 0
 push 0
 push eax
 mov eax,DWORD PTR [ebx+0x1c]
 add eax,DWORD PTR [ebp-0x28]
 push eax
 push -2
 push DWORD PTR [ebx+8]
 call DWORD PTR [0x92d56c]
 push 5
 push DWORD PTR [ebx+8]
 call DWORD PTR [0x92d574]
 early_return
.existing:
 push esi
 push -16
 push eax
 call DWORD PTR [0x92d5ac]
 mov eax,DWORD PTR [ebp-0xc]
 sub eax,DWORD PTR [ebp-0x14]
 cmp BYTE PTR [ebp+0x10],0
 je .windowed
 push 0x62
 push eax
 mov eax,DWORD PTR [ebp-0x10]
 sub eax,DWORD PTR [ebp-0x18]
 push eax
 push 0
 push 0
 jmp .position
.windowed:
 push 0x60
 push eax
 mov eax,DWORD PTR [ebp-0x10]
 sub eax,DWORD PTR [ebp-0x18]
 push eax
 push 0x80000000
 push 0x80000000
.position:
 push -2
 push DWORD PTR [ebx+8]
 call DWORD PTR [0x92d56c]
.done:
 mov ecx,DWORD PTR [ebp-8]
 pop edi
 pop esi
 {load} xor ecx,ebp
 pop ebx
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 12
