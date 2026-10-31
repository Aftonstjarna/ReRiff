.intel_syntax noprefix
.code32
.text
.global original_viewport_size
.equ fit_dimensions,0x4a1c30
original_viewport_size:
 push ebp
 {load} mov ebp,esp
 push ecx
 push ebx
 mov ebx,DWORD PTR [ebp+8]
 push esi
 {load} mov esi,ecx
 push edi
 mov edi,DWORD PTR [ebp+12]
 cmp BYTE PTR [esi+0x4b8],0
 mov DWORD PTR [esi+0x48c],ebx
 mov DWORD PTR [esi+0x490],edi
 jne .fit
 mov eax,0x66666667
 mov DWORD PTR [esi+0x49c],0
 imul edi
 mov DWORD PTR [esi+0x4a0],0
 sar edx,2
 {load} mov eax,edx
 mov DWORD PTR [esi+0x494],ebx
 shr eax,31
 add eax,-2
 mov DWORD PTR [esi+0x498],edi
 {load} add eax,edx
 mov DWORD PTR [esi+0x294],eax
 pop edi
 pop esi
 pop ebx
 pop ecx
 pop ebp
 ret 8
.fit:
 lea eax,[ebp+12]
 mov DWORD PTR [ebp+8],ebx
 push eax
 lea ecx,[ebp+8]
 mov DWORD PTR [ebp+12],edi
 call fit_dimensions
 mov eax,DWORD PTR [ebp+8]
 {load} sub ebx,eax
 mov ecx,DWORD PTR [ebp+12]
 {load} sub edi,ecx
 mov DWORD PTR [esi+0x494],eax
 mov eax,0x66666667
 imul ecx
 mov DWORD PTR [esi+0x498],ecx
 mov ecx,DWORD PTR [0xbfcc28]
 sar edx,2
 {load} mov eax,edx
 shr ebx,1
 shr eax,31
 add eax,-2
 shr edi,1
 {load} add eax,edx
 mov DWORD PTR [esi+0x4a0],edi
 mov DWORD PTR [esi+0x294],eax
 {load} mov eax,ebx
 mov DWORD PTR [esi+0x49c],ebx
 pop edi
 mov DWORD PTR [ecx+0x14],eax
 mov eax,DWORD PTR [esi+0x4a0]
 pop esi
 mov DWORD PTR [ecx+0x18],eax
 pop ebx
 pop ecx
 pop ebp
 ret 8
