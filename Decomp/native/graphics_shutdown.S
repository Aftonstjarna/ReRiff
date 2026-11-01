# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a08e0..0x4a0ae1
.intel_syntax noprefix
.code32
.text
.global original_graphics_shutdown
original_graphics_shutdown:
.L4a08e0:
 push   ebp
.L4a08e1:
 {load} mov    ebp,esp
.L4a08e3:
 push   ecx
.L4a08e4:
 push   ebx
.L4a08e5:
 {load} mov    ebx,ecx
.L4a08e7:
 mov    ecx,DWORD PTR [0xbfcd10]
.L4a08ed:
 mov    DWORD PTR [ebp-0x4],ebx
.L4a08f0:
 push   esi
.L4a08f1:
 push   edi
.L4a08f2:
 mov    BYTE PTR [ebx+0x50],0x1
.L4a08f6:
 test   ecx,ecx
.L4a08f8:
 je .L4a0900
.L4a08fa:
 mov    eax,DWORD PTR [ecx]
.L4a08fc:
 push   0x1
.L4a08fe:
 call   DWORD PTR [eax]
.L4a0900:
 mov    ecx,DWORD PTR [0xbfcd04]
.L4a0906:
 mov    DWORD PTR [0xbfcd10],0x0
.L4a0910:
 test   ecx,ecx
.L4a0912:
 je .L4a091a
.L4a0914:
 mov    eax,DWORD PTR [ecx]
.L4a0916:
 push   0x1
.L4a0918:
 call   DWORD PTR [eax]
.L4a091a:
 mov    esi,DWORD PTR [0xbfcc70]
.L4a0920:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a0926:
 mov    DWORD PTR [0xbfcd04],0x0
.L4a0930:
 test   esi,esi
.L4a0932:
 je .L4a0941
.L4a0934:
 {load} mov    ecx,esi
.L4a0936:
 call target_49fbd0
.L4a093b:
 push   esi
.L4a093c:
 call   edi
.L4a093e:
 add    esp,0x4
.L4a0941:
 mov    ecx,DWORD PTR [0xbfcd00]
.L4a0947:
 mov    DWORD PTR [0xbfcc70],0x0
.L4a0951:
 test   ecx,ecx
.L4a0953:
 je .L4a095b
.L4a0955:
 mov    eax,DWORD PTR [ecx]
.L4a0957:
 push   0x1
.L4a0959:
 call   DWORD PTR [eax]
.L4a095b:
 mov    ecx,DWORD PTR [0xbfcd0c]
.L4a0961:
 mov    DWORD PTR [0xbfcd00],0x0
.L4a096b:
 test   ecx,ecx
.L4a096d:
 je .L4a0975
.L4a096f:
 mov    eax,DWORD PTR [ecx]
.L4a0971:
 push   0x1
.L4a0973:
 call   DWORD PTR [eax]
.L4a0975:
 mov    eax,[0xc0d434]
.L4a097a:
 mov    esi,DWORD PTR [0x92d4b4]
.L4a0980:
 mov    DWORD PTR [0xbfcd0c],0x0
.L4a098a:
 test   eax,eax
.L4a098c:
 jne .L4a09bd
.L4a098e:
 push   0x10
.L4a0990:
 call   esi
.L4a0992:
 add    esp,0x4
.L4a0995:
 test   eax,eax
.L4a0997:
 je .L4a09b6
.L4a0999:
 mov    DWORD PTR [eax],0x0
.L4a099f:
 mov    DWORD PTR [eax+0x4],0x0
.L4a09a6:
 mov    DWORD PTR [eax+0x8],0x0
.L4a09ad:
 mov    DWORD PTR [eax+0xc],0x0
.L4a09b4:
 jmp .L4a09b8
.L4a09b6:
 {load} xor    eax,eax
.L4a09b8:
 mov    [0xc0d434],eax
.L4a09bd:
 push   ecx
.L4a09be:
 {load} mov    ecx,eax
.L4a09c0:
 call target_4b2400
.L4a09c5:
 mov    eax,[0xc0d438]
.L4a09ca:
 test   eax,eax
.L4a09cc:
 jne .L4a09fd
.L4a09ce:
 push   0x10
.L4a09d0:
 call   esi
.L4a09d2:
 add    esp,0x4
.L4a09d5:
 test   eax,eax
.L4a09d7:
 je .L4a09f6
.L4a09d9:
 mov    DWORD PTR [eax],0x0
.L4a09df:
 mov    DWORD PTR [eax+0x4],0x0
.L4a09e6:
 mov    DWORD PTR [eax+0x8],0x0
.L4a09ed:
 mov    DWORD PTR [eax+0xc],0x0
.L4a09f4:
 jmp .L4a09f8
.L4a09f6:
 {load} xor    eax,eax
.L4a09f8:
 mov    [0xc0d438],eax
.L4a09fd:
 push   ecx
.L4a09fe:
 {load} mov    ecx,eax
.L4a0a00:
 call target_4b2400
.L4a0a05:
 call target_4b2cc0
.L4a0a0a:
 call target_4b2c60
.L4a0a0f:
 call target_4b2ba0
.L4a0a14:
 call target_4b2c00
.L4a0a19:
 lea    ecx,[ebx+0x214]
.L4a0a1f:
 call target_542a40
.L4a0a24:
 mov    esi,DWORD PTR [0xbfcd18]
.L4a0a2a:
 test   esi,esi
.L4a0a2c:
 je .L4a0a54
.L4a0a2e:
 mov    eax,DWORD PTR [esi]
.L4a0a30:
 test   eax,eax
.L4a0a32:
 je .L4a0a4e
.L4a0a34:
 push   eax
.L4a0a35:
 call   edi
.L4a0a37:
 add    esp,0x4
.L4a0a3a:
 mov    DWORD PTR [esi],0x0
.L4a0a40:
 mov    DWORD PTR [esi+0x4],0x0
.L4a0a47:
 mov    DWORD PTR [esi+0x8],0x0
.L4a0a4e:
 push   esi
.L4a0a4f:
 call   edi
.L4a0a51:
 add    esp,0x4
.L4a0a54:
 lea    ecx,[ebx+0x3a0]
.L4a0a5a:
 mov    DWORD PTR [0xbfcd18],0x0
.L4a0a64:
 mov    BYTE PTR [ebx+0x414],0x1
.L4a0a6b:
 call target_542f80
.L4a0a70:
 mov    ecx,DWORD PTR [ebx+0x3a0]
.L4a0a76:
 test   ecx,ecx
.L4a0a78:
 je .L4a0a80
.L4a0a7a:
 mov    eax,DWORD PTR [ecx]
.L4a0a7c:
 push   ecx
.L4a0a7d:
 call   DWORD PTR [eax+0x8]
.L4a0a80:
 {load} mov    ecx,ebx
.L4a0a82:
 mov    DWORD PTR [ebx+0x3a0],0x0
.L4a0a8c:
 mov    BYTE PTR [ebx+0x414],0x0
.L4a0a93:
 call target_4a5c40
.L4a0a98:
 mov    ecx,DWORD PTR [0xbfcc28]
.L4a0a9e:
 call target_48c690
.L4a0aa3:
 lea    esi,[ebx+0x458]
.L4a0aa9:
 mov    edi,0x2
.L4a0aae:
 mov    ebx,DWORD PTR [0x92d0d4]
.L4a0ab4:
 push   DWORD PTR [esi+0x8]
.L4a0ab7:
 call   ebx
.L4a0ab9:
 push   DWORD PTR [esi]
.L4a0abb:
 mov    DWORD PTR [esi+0x8],0x0
.L4a0ac2:
 call   ebx
.L4a0ac4:
 mov    DWORD PTR [esi],0x0
.L4a0aca:
 lea    esi,[esi+0x4]
.L4a0acd:
 dec    edi
.L4a0ace:
 jne .L4a0ab4
.L4a0ad0:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a0ad3:
 pop    edi
.L4a0ad4:
 pop    esi
.L4a0ad5:
 pop    ebx
.L4a0ad6:
 mov    BYTE PTR [eax+0xba0],0x0
.L4a0add:
 {load} mov    esp,ebp
.L4a0adf:
 pop    ebp
.L4a0ae0:
 ret
.equ target_48c690,0x48c690
.equ target_49fbd0,0x49fbd0
.equ target_4a5c40,0x4a5c40
.equ target_4b2400,0x4b2400
.equ target_4b2ba0,0x4b2ba0
.equ target_4b2c00,0x4b2c00
.equ target_4b2c60,0x4b2c60
.equ target_4b2cc0,0x4b2cc0
.equ target_542a40,0x542a40
.equ target_542f80,0x542f80
