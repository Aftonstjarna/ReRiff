.intel_syntax noprefix
.code32
.text
.global original_floor_float
.equ original_floor,0x8f8f40
original_floor_float:
 push ebp
 {load} mov ebp,esp
 and esp,-64
 sub esp,64
 movss xmm0,DWORD PTR [ebp+8]
 sub esp,8
 cvtps2pd xmm0,xmm0
 movsd QWORD PTR [esp+64],xmm0
 fld QWORD PTR [esp+64]
 fstp QWORD PTR [esp]
 call original_floor
 fstp DWORD PTR [esp+64]
 fld DWORD PTR [esp+64]
 add esp,8
 {load} mov esp,ebp
 pop ebp
 ret
