.intel_syntax noprefix
.code32
.text
.global original_request_case2
.equ fn_4a6330,0x4a6330
.equ fn_48cb30,0x48cb30
.equ fn_53d2d0,0x53d2d0
.equ fn_4a6560,0x4a6560
.equ fn_4a22e0,0x4a22e0
.equ fn_4a5b00,0x4a5b00
.equ cookie_check,0x8f8958
original_request_case2:
 push ebp
 {load} mov ebp,esp
 sub esp,0x70
 mov eax,DWORD PTR [0xbaabc0]
 {load} xor eax,ebp
 mov DWORD PTR [ebp-8],eax
 push ebx
 mov ebx,DWORD PTR [ebp+8]
 push esi
 push edi
 {load} mov edi,ecx
.macro compare_field request,object
 mov eax,DWORD PTR [ebx+\request]
 cmp eax,DWORD PTR [edi+\object]
 jne .update
.endm
 compare_field 4,0x23c
 compare_field 8,0x240
 compare_field 0xc,0x244
 compare_field 0x10,0x248
 mov al,BYTE PTR [ebx+0x1c]
 cmp al,BYTE PTR [edi+0x4bf]
 jne .update
 compare_field 0x18,0x250
 mov al,BYTE PTR [ebx+0x1d]
 mov BYTE PTR [edi+0x4ba],al
 jmp .success
.update:
 call fn_4a6330
 cmp BYTE PTR [edi+0x4bf],0
 jne .save
 mov esi,DWORD PTR [0xbfcc28]
 lea eax,[ebp-0x1c]
 push eax
 push DWORD PTR [esi+4]
 call DWORD PTR [0x92d524]
 mov eax,DWORD PTR [ebp-0x1c]
 mov DWORD PTR [esi+0xc],eax
 mov eax,DWORD PTR [ebp-0x18]
 mov DWORD PTR [esi+0x10],eax
.save:
 movdqu xmm0,XMMWORD PTR [edi+0x1b8]
 mov eax,DWORD PTR [ebx+0xc]
 lea esi,[edi+0x1b8]
 mov edx,DWORD PTR [edi+0x274]
 movdqu XMMWORD PTR [ebp-0x3c],xmm0
 mov DWORD PTR [ebp-0x20],edx
 movdqu xmm0,XMMWORD PTR [esi+0x10]
 movdqu XMMWORD PTR [ebp-0x5c],xmm0
 movdqu xmm0,XMMWORD PTR [esi+0x20]
 movdqu XMMWORD PTR [ebp-0x4c],xmm0
 movq xmm0,QWORD PTR [esi+0x30]
 mov DWORD PTR [esi],eax
 mov eax,DWORD PTR [ebx+0x10]
 mov DWORD PTR [edi+0x1bc],eax
 mov eax,DWORD PTR [ebx+0x18]
 add eax,-2
 movq QWORD PTR [ebp-0x2c],xmm0
 cmp eax,14
 ja .default
 jmp DWORD PTR [.samples+eax*4]
.macro sample value
.sample\value:
 mov DWORD PTR [edi+0x1c8],\value
 jmp .apply
.endm
 sample 2
 sample 3
 sample 4
 sample 5
 sample 6
 sample 7
 sample 8
 sample 9
 sample 10
 sample 11
 sample 12
 sample 13
 sample 14
 sample 15
 sample 16
.default:
 mov DWORD PTR [edi+0x1c8],0
.apply:
 {load} xor ecx,ecx
 mov DWORD PTR [edi+0x1cc],0
 cmp BYTE PTR [ebx+0x1c],cl
 {load} mov eax,edx
 push esi
 sete cl
 {load} xor edx,edx
 test ecx,ecx
 mov DWORD PTR [edi+0x1d8],ecx
 cmovne eax,edx
 mov DWORD PTR [edi+0x1e8],eax
 mov eax,DWORD PTR [edi+0x78]
 push eax
 mov ecx,DWORD PTR [eax]
 call DWORD PTR [ecx+0x40]
 test eax,eax
 jns .resetok
 movdqu xmm0,XMMWORD PTR [ebp-0x3c]
 movzx eax,BYTE PTR [edi+0x4bf]
 mov ecx,DWORD PTR [0xbfcc28]
 movdqu XMMWORD PTR [esi],xmm0
 push eax
.macro restore_tail
 movdqu xmm0,XMMWORD PTR [ebp-0x5c]
 movdqu XMMWORD PTR [esi+0x10],xmm0
 movdqu xmm0,XMMWORD PTR [ebp-0x4c]
 movdqu XMMWORD PTR [esi+0x20],xmm0
 movq xmm0,QWORD PTR [ebp-0x2c]
 movq QWORD PTR [esi+0x30],xmm0
.endm
 restore_tail
 push DWORD PTR [ebx+0x10]
 push DWORD PTR [ebx+0xc]
 call fn_48cb30
 {load} xor al,al
.macro failure_return
 pop edi
 pop esi
 pop ebx
 mov ecx,DWORD PTR [ebp-8]
 {load} xor ecx,ebp
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 4
.endm
 failure_return
.resetok:
 push DWORD PTR [ebx+8]
 mov ecx,DWORD PTR [0xbfcd0c]
 push DWORD PTR [ebx+4]
 call fn_53d2d0
 movdqu xmm0,XMMWORD PTR [edi+0x23c]
 mov eax,DWORD PTR [ebx+4]
 movdqu XMMWORD PTR [ebp-0x6c],xmm0
 movq xmm0,QWORD PTR [edi+0x24c]
 mov DWORD PTR [edi+0x23c],eax
 mov eax,DWORD PTR [ebx+8]
 mov DWORD PTR [edi+0x240],eax
 mov eax,DWORD PTR [ebx+0xc]
 mov DWORD PTR [edi+0x244],eax
 mov eax,DWORD PTR [ebx+0x10]
 mov DWORD PTR [edi+0x248],eax
 cmp BYTE PTR [ebx+0x1c],0
 mov eax,DWORD PTR [ebp-0x20]
 movq QWORD PTR [ebp-0x14],xmm0
 jne .refresh
 mov eax,DWORD PTR [edi+0x274]
.refresh:
 mov DWORD PTR [edi+0x24c],eax
 {load} mov ecx,edi
 mov eax,DWORD PTR [ebx+0x18]
 mov DWORD PTR [edi+0x250],eax
 call fn_4a6560
 test al,al
 jne .commit
 movdqu xmm0,XMMWORD PTR [ebp-0x3c]
 mov eax,DWORD PTR [edi+0x78]
 push esi
 movdqu XMMWORD PTR [esi],xmm0
 push eax
 restore_tail
 mov ecx,DWORD PTR [eax]
 call DWORD PTR [ecx+0x40]
 push DWORD PTR [edi+0x1bc]
 mov ecx,DWORD PTR [0xbfcd0c]
 push DWORD PTR [esi]
 call fn_53d2d0
 movdqu xmm0,XMMWORD PTR [ebp-0x6c]
 {load} xor al,al
 movdqu XMMWORD PTR [edi+0x23c],xmm0
 movq xmm0,QWORD PTR [ebp-0x14]
 movq QWORD PTR [edi+0x24c],xmm0
 failure_return
.commit:
 movzx eax,BYTE PTR [ebx+0x1c]
 mov ecx,DWORD PTR [0xbfcc28]
 push eax
 push DWORD PTR [ebx+0x10]
 push DWORD PTR [ebx+0xc]
 call fn_48cb30
 mov al,BYTE PTR [ebx+0x1c]
 {load} mov ecx,edi
 mov BYTE PTR [edi+0x4bf],al
.macro ratio numerator,denominator,result
 mov eax,DWORD PTR [edi+\numerator]
 movd xmm0,eax
 cvtdq2pd xmm0,xmm0
 shr eax,31
 addsd xmm0,QWORD PTR [eax*8+0xa071b0]
 mov eax,DWORD PTR [edi+\denominator]
 cvtpd2ps xmm1,xmm0
 movd xmm0,eax
 cvtdq2pd xmm0,xmm0
 shr eax,31
 addsd xmm0,QWORD PTR [eax*8+0xa071b0]
 cvtpd2ps xmm0,xmm0
 divss xmm1,xmm0
 movss DWORD PTR [edi+\result],xmm1
.endm
 ratio 0x23c,0x244,0x4a4
 ratio 0x240,0x248,0x4a8
 push DWORD PTR [edi+0x248]
 push DWORD PTR [edi+0x244]
 call fn_4a22e0
 mov al,BYTE PTR [ebx+0x1d]
 {load} mov ecx,edi
 mov BYTE PTR [edi+0x4ba],al
 call fn_4a5b00
.success:
 mov ecx,DWORD PTR [ebp-8]
 mov al,1
 pop edi
 pop esi
 {load} xor ecx,ebp
 pop ebx
 call cookie_check
 {load} mov esp,ebp
 pop ebp
 ret 4
.samples:
 .long .sample2,.sample3,.sample4,.sample5,.sample6
 .long .sample7,.sample8,.sample9,.sample10,.sample11
 .long .sample12,.sample13,.sample14,.sample15,.sample16
