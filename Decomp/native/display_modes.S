# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a29d0..0x4a2bd8
.intel_syntax noprefix
.code32
.text
.global original_display_modes
original_display_modes:
.L4a29d0:
 push   ebp
.L4a29d1:
 {load} mov    ebp,esp
.L4a29d3:
 sub    esp,0x70
.L4a29d6:
 mov    eax,[0xbaabc0]
.L4a29db:
 {load} xor    eax,ebp
.L4a29dd:
 mov    DWORD PTR [ebp-0x8],eax
.L4a29e0:
 xorps  xmm0,xmm0
.L4a29e3:
 paddd  xmm0,XMMWORD PTR [0xa06e00]
.L4a29eb:
 paddd  xmm0,XMMWORD PTR [0xa06de0]
.L4a29f3:
 push   ebx
.L4a29f4:
 movdqu XMMWORD PTR [ebp-0x44],xmm0
.L4a29f9:
 push   esi
.L4a29fa:
 movdqa xmm0,XMMWORD PTR [0xa06e30]
.L4a2a02:
 {load} mov    esi,ecx
.L4a2a04:
 paddd  xmm0,XMMWORD PTR [0xa06e00]
.L4a2a0c:
 mov    ecx,0x8
.L4a2a11:
 paddd  xmm0,XMMWORD PTR [0xa06de0]
.L4a2a19:
 push   edi
.L4a2a1a:
 mov    DWORD PTR [esi+0x288],0x0
.L4a2a24:
 movdqu XMMWORD PTR [ebp-0x34],xmm0
.L4a2a29:
 {disp32} lea    esp,[esp+0x0]
.L4a2a30:
 lea    eax,[ecx+0x2]
.L4a2a33:
 mov    DWORD PTR [ebp+ecx*4-0x44],eax
.L4a2a37:
 inc    ecx
.L4a2a38:
 cmp    ecx,0xf
.L4a2a3b:
 jl .L4a2a30
.L4a2a3d:
 mov    ebx,DWORD PTR [ebp+0x8]
.L4a2a40:
 {load} xor    edx,edx
.L4a2a42:
 mov    DWORD PTR [ebp-0x58],edx
.L4a2a45:
 lea    edi,[edx+0x1]
.L4a2a48:
 jmp .L4a2a50
.L4a2a4a:
 {disp32} lea    ebx,[ebx+0x0]
.L4a2a50:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a2a53:
 push   0x0
.L4a2a55:
 push   DWORD PTR [ebp+edx*4-0x44]
.L4a2a59:
 mov    eax,DWORD PTR [ecx]
.L4a2a5b:
 push   0x1
.L4a2a5d:
 push   DWORD PTR [ebp+0x10]
.L4a2a60:
 push   DWORD PTR [ebp+0xc]
.L4a2a63:
 push   ebx
.L4a2a64:
 push   ecx
.L4a2a65:
 call   DWORD PTR [eax+0x2c]
.L4a2a68:
 test   eax,eax
.L4a2a6a:
 js .L4a2ad4
.L4a2a6c:
 mov    edx,DWORD PTR [ebp-0x58]
.L4a2a6f:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2a72:
 push   0x0
.L4a2a74:
 push   DWORD PTR [ebp+edx*4-0x44]
.L4a2a78:
 mov    ecx,DWORD PTR [eax]
.L4a2a7a:
 push   0x1
.L4a2a7c:
 push   DWORD PTR [ebp+0x18]
.L4a2a7f:
 push   DWORD PTR [ebp+0xc]
.L4a2a82:
 push   ebx
.L4a2a83:
 push   eax
.L4a2a84:
 call   DWORD PTR [ecx+0x2c]
.L4a2a87:
 test   eax,eax
.L4a2a89:
 js .L4a2ad4
.L4a2a8b:
 mov    edx,DWORD PTR [ebp-0x58]
.L4a2a8e:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2a91:
 push   0x0
.L4a2a93:
 push   DWORD PTR [ebp+edx*4-0x44]
.L4a2a97:
 mov    ecx,DWORD PTR [eax]
.L4a2a99:
 push   0x0
.L4a2a9b:
 push   DWORD PTR [ebp+0x14]
.L4a2a9e:
 push   DWORD PTR [ebp+0xc]
.L4a2aa1:
 push   ebx
.L4a2aa2:
 push   eax
.L4a2aa3:
 call   DWORD PTR [ecx+0x2c]
.L4a2aa6:
 test   eax,eax
.L4a2aa8:
 js .L4a2ad4
.L4a2aaa:
 mov    edx,DWORD PTR [ebp-0x58]
.L4a2aad:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2ab0:
 push   0x0
.L4a2ab2:
 push   DWORD PTR [ebp+edx*4-0x44]
.L4a2ab6:
 mov    ecx,DWORD PTR [eax]
.L4a2ab8:
 push   0x0
.L4a2aba:
 push   DWORD PTR [ebp+0x18]
.L4a2abd:
 push   DWORD PTR [ebp+0xc]
.L4a2ac0:
 push   ebx
.L4a2ac1:
 push   eax
.L4a2ac2:
 call   DWORD PTR [ecx+0x2c]
.L4a2ac5:
 test   eax,eax
.L4a2ac7:
 js .L4a2ad4
.L4a2ac9:
 {load} mov    eax,edi
.L4a2acb:
 rol    eax,0x2
.L4a2ace:
 or     DWORD PTR [esi+0x288],eax
.L4a2ad4:
 mov    edx,DWORD PTR [ebp-0x58]
.L4a2ad7:
 inc    edx
.L4a2ad8:
 rol    edi,1
.L4a2ada:
 mov    DWORD PTR [ebp-0x58],edx
.L4a2add:
 cmp    edx,0xf
.L4a2ae0:
 jb .L4a2a50
.L4a2ae6:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2ae9:
 push   0x16
.L4a2aeb:
 push   ebx
.L4a2aec:
 push   eax
.L4a2aed:
 mov    ecx,DWORD PTR [eax]
.L4a2aef:
 call   DWORD PTR [ecx+0x18]
.L4a2af2:
 mov    ecx,DWORD PTR [esi+0x284]
.L4a2af8:
 {load} mov    edi,eax
.L4a2afa:
 sub    ecx,DWORD PTR [esi+0x27c]
.L4a2b00:
 mov    eax,0x2aaaaaab
.L4a2b05:
 imul   ecx
.L4a2b07:
 mov    DWORD PTR [ebp-0x58],edi
.L4a2b0a:
 sar    edx,0x2
.L4a2b0d:
 {load} mov    eax,edx
.L4a2b0f:
 shr    eax,0x1f
.L4a2b12:
 {load} add    eax,edx
.L4a2b14:
 {load} cmp    eax,edi
.L4a2b16:
 jae .L4a2b37
.L4a2b18:
 cmp    edi,0xaaaaaaa
.L4a2b1e:
 jbe .L4a2b2b
.L4a2b20:
 push   0x999938
.L4a2b25:
 call   DWORD PTR [0x92d168]
.L4a2b2b:
 push   edi
.L4a2b2c:
 lea    ecx,[esi+0x27c]
.L4a2b32:
 call target_4b2840
.L4a2b37:
 {load} xor    edi,edi
.L4a2b39:
 cmp    DWORD PTR [ebp-0x58],edi
.L4a2b3c:
 jbe .L4a2b91
.L4a2b3e:
 {load} mov    edi,edi
.L4a2b40:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2b43:
 lea    edx,[ebp-0x54]
.L4a2b46:
 push   edx
.L4a2b47:
 push   edi
.L4a2b48:
 push   0x16
.L4a2b4a:
 mov    ecx,DWORD PTR [eax]
.L4a2b4c:
 push   ebx
.L4a2b4d:
 push   eax
.L4a2b4e:
 call   DWORD PTR [ecx+0x1c]
.L4a2b51:
 test   eax,eax
.L4a2b53:
 js .L4a2b8b
.L4a2b55:
 mov    eax,DWORD PTR [ebp-0x54]
.L4a2b58:
 lea    ecx,[esi+0x27c]
.L4a2b5e:
 mov    DWORD PTR [ebp-0x70],eax
.L4a2b61:
 mov    eax,DWORD PTR [ebp-0x50]
.L4a2b64:
 mov    DWORD PTR [ebp-0x6c],eax
.L4a2b67:
 mov    eax,DWORD PTR [ebp-0x4c]
.L4a2b6a:
 mov    DWORD PTR [ebp-0x60],eax
.L4a2b6d:
 lea    eax,[ebp-0x70]
.L4a2b70:
 push   eax
.L4a2b71:
 mov    DWORD PTR [ebp-0x68],0x0
.L4a2b78:
 mov    DWORD PTR [ebp-0x64],0x0
.L4a2b7f:
 mov    DWORD PTR [ebp-0x5c],0x0
.L4a2b86:
 call target_4b2500
.L4a2b8b:
 inc    edi
.L4a2b8c:
 cmp    edi,DWORD PTR [ebp-0x58]
.L4a2b8f:
 jb .L4a2b40
.L4a2b91:
 mov    eax,DWORD PTR [esi+0x27c]
.L4a2b97:
 cmp    eax,DWORD PTR [esi+0x280]
.L4a2b9d:
 jne .L4a2bc3
.L4a2b9f:
 push   0x2a
.L4a2ba1:
 push   0x9fc2f8
.L4a2ba6:
 lea    ecx,[esi+0x54]
.L4a2ba9:
 call target_416520
.L4a2bae:
 {load} xor    al,al
.L4a2bb0:
 pop    edi
.L4a2bb1:
 pop    esi
.L4a2bb2:
 pop    ebx
.L4a2bb3:
 mov    ecx,DWORD PTR [ebp-0x8]
.L4a2bb6:
 {load} xor    ecx,ebp
.L4a2bb8:
 call target_8f8958
.L4a2bbd:
 {load} mov    esp,ebp
.L4a2bbf:
 pop    ebp
.L4a2bc0:
 ret    0x14
.L4a2bc3:
 mov    ecx,DWORD PTR [ebp-0x8]
.L4a2bc6:
 mov    al,0x1
.L4a2bc8:
 pop    edi
.L4a2bc9:
 pop    esi
.L4a2bca:
 {load} xor    ecx,ebp
.L4a2bcc:
 pop    ebx
.L4a2bcd:
 call target_8f8958
.L4a2bd2:
 {load} mov    esp,ebp
.L4a2bd4:
 pop    ebp
.L4a2bd5:
 ret    0x14
.equ target_416520,0x416520
.equ target_4b2500,0x4b2500
.equ target_4b2840,0x4b2840
.equ target_8f8958,0x8f8958
