.intel_syntax noprefix
.code32
.text
.global original_request_case3
.equ fn_48cc70,0x48cc70
original_request_case3:
 push ebp
 {load} mov ebp,esp
 push ecx
 push esi
 push edi
 mov edi,DWORD PTR [ebp+8]
 {load} mov esi,ecx
 mov ecx,DWORD PTR [edi+4]
 cmp ecx,DWORD PTR [esi+0x254]
 jne .update
 mov eax,DWORD PTR [edi+8]
 cmp eax,DWORD PTR [esi+0x258]
 jne .update
 mov eax,DWORD PTR [edi+0xc]
 cmp eax,DWORD PTR [esi+0x25c]
 jne .update
 mov eax,DWORD PTR [edi+0x10]
 cmp eax,DWORD PTR [esi+0x260]
 jne .update
 mov al,BYTE PTR [edi+0x1c]
 cmp al,BYTE PTR [esi+0x4bf]
 jne .update
 mov eax,DWORD PTR [edi+0x18]
 cmp eax,DWORD PTR [esi+0x268]
 je .done
.update:
 mov DWORD PTR [esi+0x254],ecx
 mov eax,DWORD PTR [edi+8]
 mov ecx,DWORD PTR [0xbfcc28]
 mov DWORD PTR [esi+0x258],eax
 mov eax,DWORD PTR [edi+0xc]
 mov DWORD PTR [esi+0x25c],eax
 mov eax,DWORD PTR [edi+0x10]
 mov DWORD PTR [esi+0x260],eax
 mov eax,DWORD PTR [esi+0x274]
 mov DWORD PTR [esi+0x264],eax
 mov eax,DWORD PTR [edi+0x18]
 mov DWORD PTR [esi+0x268],eax
 movzx eax,BYTE PTR [edi+0x1c]
 push eax
 push DWORD PTR [edi+0x10]
 push DWORD PTR [edi+0xc]
 call fn_48cc70
# Preserve the compiler's unsigned-to-float conversion and rounding sequence.
.macro ratio numerator,denominator,result
 mov eax,DWORD PTR [esi+\numerator]
 movd xmm0,eax
 cvtdq2pd xmm0,xmm0
 shr eax,31
 addsd xmm0,QWORD PTR [eax*8+0xa071b0]
 mov eax,DWORD PTR [esi+\denominator]
 cvtpd2ps xmm1,xmm0
 movd xmm0,eax
 cvtdq2pd xmm0,xmm0
 shr eax,31
 addsd xmm0,QWORD PTR [eax*8+0xa071b0]
 cvtpd2ps xmm0,xmm0
 divss xmm1,xmm0
 movss DWORD PTR [esi+\result],xmm1
.endm
 ratio 0x254,0x25c,0x4ac
 ratio 0x258,0x260,0x4b0
.done:
 mov al,BYTE PTR [edi+0x1d]
 mov BYTE PTR [esi+0x4bb],al
 mov al,1
 pop edi
 pop esi
 pop ecx
 pop ebp
 ret 4
