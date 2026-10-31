# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a27d0..0x4a29c6
.intel_syntax noprefix
.code32
.text
.global original_graphics_options
original_graphics_options:
.L4a27d0:
 push   ebp
.L4a27d1:
 {load} mov    ebp,esp
.L4a27d3:
 push   0xffffffff
.L4a27d5:
 push   0x902a00
.L4a27da:
 mov    eax,fs:0x0
.L4a27e0:
 push   eax
.L4a27e1:
 sub    esp,0x60
.L4a27e4:
 mov    eax,[0xbaabc0]
.L4a27e9:
 {load} xor    eax,ebp
.L4a27eb:
 mov    DWORD PTR [ebp-0x14],eax
.L4a27ee:
 push   ebx
.L4a27ef:
 push   esi
.L4a27f0:
 push   edi
.L4a27f1:
 push   eax
.L4a27f2:
 lea    eax,[ebp-0xc]
.L4a27f5:
 mov    fs:0x0,eax
.L4a27fb:
 {load} mov    edi,ecx
.L4a27fd:
 mov    DWORD PTR [ebp-0x64],edi
.L4a2800:
 mov    ebx,DWORD PTR [ebp+0x8]
.L4a2803:
 mov    esi,DWORD PTR [0x92d3dc]
.L4a2809:
 push   0x9fc338
.L4a280e:
 push   ebx
.L4a280f:
 call   esi
.L4a2811:
 add    esp,0x8
.L4a2814:
 test   eax,eax
.L4a2816:
 je .L4a281f
.L4a2818:
 mov    BYTE PTR [edi+0x4bf],0x1
.L4a281f:
 push   0x9fc32c
.L4a2824:
 push   ebx
.L4a2825:
 call   esi
.L4a2827:
 add    esp,0x8
.L4a282a:
 test   eax,eax
.L4a282c:
 je .L4a2992
.L4a2832:
 push   0x9fc32c
.L4a2837:
 push   ebx
.L4a2838:
 call   esi
.L4a283a:
 add    esp,0x8
.L4a283d:
 mov    DWORD PTR [ebp-0x18],0xf
.L4a2844:
 mov    DWORD PTR [ebp-0x1c],0x0
.L4a284b:
 mov    BYTE PTR [ebp-0x2c],0x0
.L4a284f:
 cmp    BYTE PTR [eax+0xa],0x0
.L4a2853:
 lea    edx,[eax+0xa]
.L4a2856:
 jne .L4a285c
.L4a2858:
 {load} xor    ecx,ecx
.L4a285a:
 jmp .L4a286a
.L4a285c:
 {load} mov    ecx,edx
.L4a285e:
 lea    edi,[ecx+0x1]
.L4a2861:
 mov    al,BYTE PTR [ecx]
.L4a2863:
 inc    ecx
.L4a2864:
 test   al,al
.L4a2866:
 jne .L4a2861
.L4a2868:
 {load} sub    ecx,edi
.L4a286a:
 push   ecx
.L4a286b:
 push   edx
.L4a286c:
 lea    ecx,[ebp-0x2c]
.L4a286f:
 call target_416520
.L4a2874:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a287b:
 cmp    DWORD PTR [ebp-0x1c],0x0
.L4a287f:
 je .L4a28a4
.L4a2881:
 cmp    DWORD PTR [ebp-0x18],0x10
.L4a2885:
 lea    edx,[ebp-0x2c]
.L4a2888:
 lea    edi,[ebp-0x2c]
.L4a288b:
 cmovae edx,DWORD PTR [ebp-0x2c]
.L4a288f:
 cmovae edi,DWORD PTR [ebp-0x2c]
.L4a2893:
 test   edx,edx
.L4a2895:
 je .L4a2899
.L4a2897:
 {load} sub    edx,edi
.L4a2899:
 push   0x1
.L4a289b:
 push   edx
.L4a289c:
 lea    ecx,[ebp-0x2c]
.L4a289f:
 call target_416660
.L4a28a4:
 push   0x1
.L4a28a6:
 push   0x0
.L4a28a8:
 lea    eax,[ebp-0x60]
.L4a28ab:
 mov    BYTE PTR [ebp-0x60],0x2c
.L4a28af:
 push   eax
.L4a28b0:
 lea    ecx,[ebp-0x2c]
.L4a28b3:
 call target_440b90
.L4a28b8:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a28be:
 mov    DWORD PTR [ebp-0x68],eax
.L4a28c1:
 cmp    eax,0xffffffff
.L4a28c4:
 je .L4a2981
.L4a28ca:
 lea    edi,[eax+0x1]
.L4a28cd:
 mov    BYTE PTR [ebp-0x60],0x20
.L4a28d1:
 push   0x1
.L4a28d3:
 push   edi
.L4a28d4:
 lea    eax,[ebp-0x60]
.L4a28d7:
 push   eax
.L4a28d8:
 lea    ecx,[ebp-0x2c]
.L4a28db:
 call target_440b90
.L4a28e0:
 push   DWORD PTR [ebp-0x68]
.L4a28e3:
 {load} mov    esi,eax
.L4a28e5:
 lea    ecx,[ebp-0x2c]
.L4a28e8:
 push   0x0
.L4a28ea:
 lea    eax,[ebp-0x5c]
.L4a28ed:
 cmp    esi,0xffffffff
.L4a28f0:
 push   eax
.L4a28f1:
 cmove  esi,DWORD PTR [ebp-0x1c]
.L4a28f5:
 call target_440a90
.L4a28fa:
 sub    esi,DWORD PTR [ebp-0x68]
.L4a28fd:
 lea    eax,[ebp-0x44]
.L4a2900:
 dec    esi
.L4a2901:
 mov    BYTE PTR [ebp-0x4],0x1
.L4a2905:
 push   esi
.L4a2906:
 push   edi
.L4a2907:
 push   eax
.L4a2908:
 lea    ecx,[ebp-0x2c]
.L4a290b:
 call target_440a90
.L4a2910:
 cmp    DWORD PTR [ebp-0x48],0x10
.L4a2914:
 lea    eax,[ebp-0x5c]
.L4a2917:
 mov    esi,DWORD PTR [0x92d3e8]
.L4a291d:
 cmovae eax,DWORD PTR [ebp-0x5c]
.L4a2921:
 push   eax
.L4a2922:
 call   esi
.L4a2924:
 mov    edi,DWORD PTR [ebp-0x64]
.L4a2927:
 add    esp,0x4
.L4a292a:
 cmp    DWORD PTR [ebp-0x30],0x10
.L4a292e:
 mov    DWORD PTR [edi+0x20c],eax
.L4a2934:
 lea    eax,[ebp-0x44]
.L4a2937:
 cmovae eax,DWORD PTR [ebp-0x44]
.L4a293b:
 push   eax
.L4a293c:
 call   esi
.L4a293e:
 add    esp,0x4
.L4a2941:
 mov    DWORD PTR [edi+0x210],eax
.L4a2947:
 cmp    DWORD PTR [ebp-0x30],0x10
.L4a294b:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a2951:
 jb .L4a295b
.L4a2953:
 push   DWORD PTR [ebp-0x44]
.L4a2956:
 call   edi
.L4a2958:
 add    esp,0x4
.L4a295b:
 cmp    DWORD PTR [ebp-0x48],0x10
.L4a295f:
 mov    DWORD PTR [ebp-0x30],0xf
.L4a2966:
 mov    DWORD PTR [ebp-0x34],0x0
.L4a296d:
 mov    BYTE PTR [ebp-0x44],0x0
.L4a2971:
 jb .L4a297b
.L4a2973:
 push   DWORD PTR [ebp-0x5c]
.L4a2976:
 call   edi
.L4a2978:
 add    esp,0x4
.L4a297b:
 mov    esi,DWORD PTR [0x92d3dc]
.L4a2981:
 cmp    DWORD PTR [ebp-0x18],0x10
.L4a2985:
 jb .L4a298f
.L4a2987:
 push   DWORD PTR [ebp-0x2c]
.L4a298a:
 call   edi
.L4a298c:
 add    esp,0x4
.L4a298f:
 mov    edi,DWORD PTR [ebp-0x64]
.L4a2992:
 push   0x9fc324
.L4a2997:
 push   ebx
.L4a2998:
 call   esi
.L4a299a:
 add    esp,0x8
.L4a299d:
 test   eax,eax
.L4a299f:
 je .L4a29a8
.L4a29a1:
 mov    BYTE PTR [edi+0x4bf],0x0
.L4a29a8:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a29ab:
 mov    DWORD PTR fs:0x0,ecx
.L4a29b2:
 pop    ecx
.L4a29b3:
 pop    edi
.L4a29b4:
 pop    esi
.L4a29b5:
 pop    ebx
.L4a29b6:
 mov    ecx,DWORD PTR [ebp-0x14]
.L4a29b9:
 {load} xor    ecx,ebp
.L4a29bb:
 call target_8f8958
.L4a29c0:
 {load} mov    esp,ebp
.L4a29c2:
 pop    ebp
.L4a29c3:
 ret    0x4
.equ target_416520,0x416520
.equ target_416660,0x416660
.equ target_440a90,0x440a90
.equ target_440b90,0x440b90
.equ target_8f8958,0x8f8958
