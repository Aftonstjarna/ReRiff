.intel_syntax noprefix
.code32
.text
.global original_resize_resources
.equ convert_size,0x8f8f50
original_resize_resources:
 push ebp
 {load} mov ebp,esp
 sub esp,12
 mov eax,DWORD PTR [0xbfcc44]
 movaps xmm0,xmm1
 push ebx
 push esi
 {load} mov esi,ecx
 movss DWORD PTR [ebp-4],xmm0
 mov eax,DWORD PTR [eax+0x78]
 {load} xor ebx,ebx
 mov DWORD PTR [ebp-8],eax
 mov eax,0x66666667
 push edi
 mov edx,DWORD PTR [esi+0x10]
 sub edx,DWORD PTR [esi+0xc]
 imul edx
.macro quotient
 sar edx,3
 {load} mov eax,edx
 shr eax,31
 {load} add eax,edx
.endm
 quotient
 je .size
 {load} xor edi,edi
 jmp .release
 {disp8} lea ecx,[ecx+0]
.release:
 mov eax,DWORD PTR [esi+0xc]
 mov ecx,DWORD PTR [edi+eax*1]
 test ecx,ecx
 je .clear
 mov eax,DWORD PTR [ecx]
 push ecx
 call DWORD PTR [eax+8]
.clear:
 mov eax,DWORD PTR [esi+0xc]
 inc ebx
 mov DWORD PTR [edi+eax*1],0
 mov eax,0x66666667
 mov ecx,DWORD PTR [esi+0x10]
 add edi,20
 sub ecx,DWORD PTR [esi+0xc]
 imul ecx
 quotient
 {load} cmp ebx,eax
 jb .release
 movss xmm0,DWORD PTR [ebp-4]
.size:
 movss xmm1,DWORD PTR [esi+0x40]
 movss DWORD PTR [esi+0x40],xmm0
 mulss xmm0,DWORD PTR [0xa06800]
 movss DWORD PTR [ebp-12],xmm1
 call convert_size
 mov ecx,DWORD PTR [esi+0x10]
 {load} xor ebx,ebx
 sub ecx,DWORD PTR [esi+0xc]
 mov DWORD PTR [ebp-4],eax
 mov eax,0x66666667
 imul ecx
 quotient
 je .success
 {load} xor edi,edi
 {disp32} lea ebx,[ebx+0]
.create:
 mov eax,DWORD PTR [esi+0xc]
 mov edx,DWORD PTR [ebp-8]
 {load} add eax,edi
 push 0
 push eax
 mov eax,DWORD PTR [ebp-4]
 mov ecx,DWORD PTR [edx]
 push 0
 push 0x15
 push 1
 push 1
 push eax
 push eax
 push edx
 call DWORD PTR [ecx+0x5c]
 test eax,eax
 jns .advance
 cmp BYTE PTR [ebp+8],0
 je .check
 movss xmm1,DWORD PTR [ebp-12]
 {load} mov ecx,esi
 push 0
 call original_resize_resources
 pop edi
 pop esi
 {load} xor al,al
 pop ebx
 {load} mov esp,ebp
 pop ebp
 ret 4
.advance:
 inc ebx
 add edi,20
.check:
 mov ecx,DWORD PTR [esi+0x10]
 mov eax,0x66666667
 sub ecx,DWORD PTR [esi+0xc]
 imul ecx
 quotient
 {load} cmp ebx,eax
 jb .create
.success:
 pop edi
 pop esi
 mov al,1
 pop ebx
 {load} mov esp,ebp
 pop ebp
 ret 4
