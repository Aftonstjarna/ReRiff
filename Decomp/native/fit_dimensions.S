.intel_syntax noprefix
.code32
.text
.global original_fit_dimensions
.equ fn_426d20,0x426d20
.equ fn_8f9320,0x8f9320
original_fit_dimensions:
 push ebp
 {load} mov ebp,esp
 sub esp,8
 push esi
 {load} mov esi,ecx
 push ecx
 mov eax,DWORD PTR [esi]
 movd xmm0,eax
 cvtdq2pd xmm0,xmm0
 shr eax,31
 addsd xmm0,QWORD PTR [eax*8+0xa071b0]
 cvtpd2ps xmm0,xmm0
 divss xmm0,DWORD PTR [0xa065d0]
 addss xmm0,DWORD PTR [0xa06508]
 movss DWORD PTR [esp],xmm0
 call fn_426d20
 mov edx,DWORD PTR [ebp+8]
 add esp,4
 fstp DWORD PTR [ebp-4]
 cvttss2si ecx,DWORD PTR [ebp-4]
 mov eax,DWORD PTR [edx]
 {load} cmp ecx,eax
 jbe .height
 movd xmm0,eax
 cvtdq2pd xmm0,xmm0
 shr eax,31
 push ecx
 addsd xmm0,QWORD PTR [eax*8+0xa071b0]
 cvtpd2ps xmm0,xmm0
 mulss xmm0,DWORD PTR [0xa065d0]
 addss xmm0,DWORD PTR [0xa06508]
 movss DWORD PTR [esp],xmm0
 call fn_426d20
 add esp,4
 call fn_8f9320
 mov DWORD PTR [esi],eax
 pop esi
 {load} mov esp,ebp
 pop ebp
 ret 4
.height:
 mov DWORD PTR [edx],ecx
 pop esi
 {load} mov esp,ebp
 pop ebp
 ret 4
