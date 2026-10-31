.intel_syntax noprefix
.code32
.text
.global original_save_device_ctor
original_save_device_ctor:
 mov BYTE PTR [ecx+4],1
 {load} xor eax,eax
 mov DWORD PTR [ecx+8],0x9ffe24
 mov DWORD PTR [ecx],0x9ffe2c
 mov DWORD PTR [ecx+8],0x9ffe6c
 mov DWORD PTR [ecx+12],0
 mov DWORD PTR [ecx+16],0
 mov DWORD PTR [ecx+0x28],7
 mov DWORD PTR [ecx+0x24],0
 mov WORD PTR [ecx+0x14],ax
 mov DWORD PTR [ecx+0x3c],eax
 mov DWORD PTR [ecx+0x40],7
 mov WORD PTR [ecx+0x2c],ax
 {load} mov eax,ecx
 mov DWORD PTR [0xbfcb4c],ecx
 ret
