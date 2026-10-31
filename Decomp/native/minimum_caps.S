# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a59e0..0x4a5af9
.intel_syntax noprefix
.code32
.text
.global original_minimum_caps
original_minimum_caps:
.L4a59e0:
 push   ebp
.L4a59e1:
 {load} mov    ebp,esp
.L4a59e3:
 cmp    DWORD PTR [ecx+0x144],0xfffe0300
.L4a59ed:
 mov    eax,DWORD PTR [ebp+0x8]
.L4a59f0:
 setb   dl
.L4a59f3:
 push   esi
.L4a59f4:
 mov    BYTE PTR [eax],dl
.L4a59f6:
 cmp    DWORD PTR [ecx+0x14c],0xffff0300
.L4a5a00:
 jae .L4a5a28
.L4a5a02:
 lea    esi,[ecx+0x54]
.L4a5a05:
 push   0x3a
.L4a5a07:
 push   0x9fc704
.L4a5a0c:
 {load} mov    ecx,esi
.L4a5a0e:
 call target_416520
.L4a5a13:
 push   0x1e
.L4a5a15:
 push   0x9fc6e4
.L4a5a1a:
 {load} mov    ecx,esi
.L4a5a1c:
 call target_442170
.L4a5a21:
 {load} xor    al,al
.L4a5a23:
 pop    esi
.L4a5a24:
 pop    ebp
.L4a5a25:
 ret    0x4
.L4a5a28:
 mov    eax,DWORD PTR [ecx+0xbc]
.L4a5a2e:
 test   al,0x2
.L4a5a30:
 je .L4a5a5f
.L4a5a32:
 test   eax,0x100
.L4a5a37:
 jne .L4a5a5f
.L4a5a39:
 lea    esi,[ecx+0x54]
.L4a5a3c:
 push   0x3a
.L4a5a3e:
 push   0x9fc704
.L4a5a43:
 {load} mov    ecx,esi
.L4a5a45:
 call target_416520
.L4a5a4a:
 push   0x20
.L4a5a4c:
 push   0x9fc6c0
.L4a5a51:
 {load} mov    ecx,esi
.L4a5a53:
 call target_442170
.L4a5a58:
 {load} xor    al,al
.L4a5a5a:
 pop    esi
.L4a5a5b:
 pop    ebp
.L4a5a5c:
 ret    0x4
.L4a5a5f:
 test   al,0x20
.L4a5a61:
 je .L4a5a89
.L4a5a63:
 lea    esi,[ecx+0x54]
.L4a5a66:
 push   0x3a
.L4a5a68:
 push   0x9fc704
.L4a5a6d:
 {load} mov    ecx,esi
.L4a5a6f:
 call target_416520
.L4a5a74:
 push   0x27
.L4a5a76:
 push   0x9fc800
.L4a5a7b:
 {load} mov    ecx,esi
.L4a5a7d:
 call target_442170
.L4a5a82:
 {load} xor    al,al
.L4a5a84:
 pop    esi
.L4a5a85:
 pop    ebp
.L4a5a86:
 ret    0x4
.L4a5a89:
 cmp    DWORD PTR [ecx+0xd8],0x1000
.L4a5a93:
 jb .L4a5ad3
.L4a5a95:
 cmp    DWORD PTR [ecx+0xdc],0x1000
.L4a5a9f:
 jb .L4a5ad3
.L4a5aa1:
 cmp    DWORD PTR [ecx+0x170],0x3
.L4a5aa8:
 jae .L4a5acc
.L4a5aaa:
 lea    esi,[ecx+0x54]
.L4a5aad:
 push   0x9fc704
.L4a5ab2:
 {load} mov    ecx,esi
.L4a5ab4:
 call target_415b90
.L4a5ab9:
 push   0x9fc7ac
.L4a5abe:
 {load} mov    ecx,esi
.L4a5ac0:
 call target_441f70
.L4a5ac5:
 {load} xor    al,al
.L4a5ac7:
 pop    esi
.L4a5ac8:
 pop    ebp
.L4a5ac9:
 ret    0x4
.L4a5acc:
 mov    al,0x1
.L4a5ace:
 pop    esi
.L4a5acf:
 pop    ebp
.L4a5ad0:
 ret    0x4
.L4a5ad3:
 lea    esi,[ecx+0x54]
.L4a5ad6:
 push   0x3a
.L4a5ad8:
 push   0x9fc704
.L4a5add:
 {load} mov    ecx,esi
.L4a5adf:
 call target_416520
.L4a5ae4:
 push   0x1b
.L4a5ae6:
 push   0x9fc7e4
.L4a5aeb:
 {load} mov    ecx,esi
.L4a5aed:
 call target_442170
.L4a5af2:
 {load} xor    al,al
.L4a5af4:
 pop    esi
.L4a5af5:
 pop    ebp
.L4a5af6:
 ret    0x4
.equ target_415b90,0x415b90
.equ target_416520,0x416520
.equ target_441f70,0x441f70
.equ target_442170,0x442170
