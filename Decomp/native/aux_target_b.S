# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a5850..0x4a59d9
.intel_syntax noprefix
.code32
.text
.global original_aux_target_b
original_aux_target_b:
.L4a5850:
 push   ebp
.L4a5851:
 {load} mov    ebp,esp
.L4a5853:
 push   0xffffffff
.L4a5855:
 push   0x902b50
.L4a585a:
 mov    eax,fs:0x0
.L4a5860:
 push   eax
.L4a5861:
 sub    esp,0x64
.L4a5864:
 mov    eax,[0xbaabc0]
.L4a5869:
 {load} xor    eax,ebp
.L4a586b:
 mov    DWORD PTR [ebp-0x10],eax
.L4a586e:
 push   ebx
.L4a586f:
 push   esi
.L4a5870:
 push   edi
.L4a5871:
 push   eax
.L4a5872:
 lea    eax,[ebp-0xc]
.L4a5875:
 mov    fs:0x0,eax
.L4a587b:
 {load} mov    esi,ecx
.L4a587d:
 cmp    BYTE PTR [esi+0xa5f],0x0
.L4a5884:
 je .L4a59bb
.L4a588a:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a588d:
 lea    edi,[esi+0x444]
.L4a5893:
 push   0x0
.L4a5895:
 push   edi
.L4a5896:
 push   0x0
.L4a5898:
 mov    eax,DWORD PTR [ecx]
.L4a589a:
 push   0x15
.L4a589c:
 push   0x1
.L4a589e:
 push   0x1
.L4a58a0:
 push   DWORD PTR [esi+0x240]
.L4a58a6:
 push   DWORD PTR [esi+0x23c]
.L4a58ac:
 push   ecx
.L4a58ad:
 call   DWORD PTR [eax+0x5c]
.L4a58b0:
 test   eax,eax
.L4a58b2:
 jns .L4a5942
.L4a58b8:
 {load} mov    edx,eax
.L4a58ba:
 lea    ecx,[ebp-0x58]
.L4a58bd:
 call target_44f380
.L4a58c2:
 push   eax
.L4a58c3:
 lea    eax,[ebp-0x40]
.L4a58c6:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a58cd:
 push   0x9fc5f4
.L4a58d2:
 push   eax
.L4a58d3:
 call target_4b2d20
.L4a58d8:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a58de:
 {load} mov    ebx,eax
.L4a58e0:
 add    esi,0x54
.L4a58e3:
 add    esp,0xc
.L4a58e6:
 {load} cmp    esi,ebx
.L4a58e8:
 je .L4a5910
.L4a58ea:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a58ee:
 jb .L4a58f7
.L4a58f0:
 push   DWORD PTR [esi]
.L4a58f2:
 call   edi
.L4a58f4:
 add    esp,0x4
.L4a58f7:
 mov    DWORD PTR [esi+0x14],0xf
.L4a58fe:
 {load} mov    ecx,esi
.L4a5900:
 mov    DWORD PTR [esi+0x10],0x0
.L4a5907:
 push   ebx
.L4a5908:
 mov    BYTE PTR [esi],0x0
.L4a590b:
 call target_416fa0
.L4a5910:
 cmp    DWORD PTR [ebp-0x2c],0x10
.L4a5914:
 jb .L4a591e
.L4a5916:
 push   DWORD PTR [ebp-0x40]
.L4a5919:
 call   edi
.L4a591b:
 add    esp,0x4
.L4a591e:
 cmp    DWORD PTR [ebp-0x44],0x10
.L4a5922:
 mov    DWORD PTR [ebp-0x2c],0xf
.L4a5929:
 mov    DWORD PTR [ebp-0x30],0x0
.L4a5930:
 mov    BYTE PTR [ebp-0x40],0x0
.L4a5934:
 jb .L4a593e
.L4a5936:
 push   DWORD PTR [ebp-0x58]
.L4a5939:
 call   edi
.L4a593b:
 add    esp,0x4
.L4a593e:
 {load} xor    al,al
.L4a5940:
 jmp .L4a59bd
.L4a5942:
 mov    ecx,DWORD PTR [edi]
.L4a5944:
 lea    eax,[esi+0x448]
.L4a594a:
 push   eax
.L4a594b:
 push   0x0
.L4a594d:
 push   ecx
.L4a594e:
 mov    edx,DWORD PTR [ecx]
.L4a5950:
 call   DWORD PTR [edx+0x48]
.L4a5953:
 test   eax,eax
.L4a5955:
 jns .L4a59bb
.L4a5957:
 {load} mov    edx,eax
.L4a5959:
 lea    ecx,[ebp-0x70]
.L4a595c:
 call target_44f380
.L4a5961:
 push   eax
.L4a5962:
 lea    eax,[ebp-0x28]
.L4a5965:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a596c:
 push   0x9fc740
.L4a5971:
 push   eax
.L4a5972:
 call target_4b2d20
.L4a5977:
 add    esp,0xc
.L4a597a:
 lea    ecx,[esi+0x54]
.L4a597d:
 push   eax
.L4a597e:
 call target_440a20
.L4a5983:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a5987:
 mov    esi,DWORD PTR [0x92d4b0]
.L4a598d:
 jb .L4a5997
.L4a598f:
 push   DWORD PTR [ebp-0x28]
.L4a5992:
 call   esi
.L4a5994:
 add    esp,0x4
.L4a5997:
 cmp    DWORD PTR [ebp-0x5c],0x10
.L4a599b:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a59a2:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a59a9:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a59ad:
 jb .L4a593e
.L4a59af:
 push   DWORD PTR [ebp-0x70]
.L4a59b2:
 call   esi
.L4a59b4:
 add    esp,0x4
.L4a59b7:
 {load} xor    al,al
.L4a59b9:
 jmp .L4a59bd
.L4a59bb:
 mov    al,0x1
.L4a59bd:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a59c0:
 mov    DWORD PTR fs:0x0,ecx
.L4a59c7:
 pop    ecx
.L4a59c8:
 pop    edi
.L4a59c9:
 pop    esi
.L4a59ca:
 pop    ebx
.L4a59cb:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a59ce:
 {load} xor    ecx,ebp
.L4a59d0:
 call target_8f8958
.L4a59d5:
 {load} mov    esp,ebp
.L4a59d7:
 pop    ebp
.L4a59d8:
 ret
.equ target_416fa0,0x416fa0
.equ target_440a20,0x440a20
.equ target_44f380,0x44f380
.equ target_4b2d20,0x4b2d20
.equ target_8f8958,0x8f8958
