# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a2be0..0x4a33ef
.intel_syntax noprefix
.code32
.text
.global original_graphics_capabilities
original_graphics_capabilities:
.L4a2be0:
 push   ebp
.L4a2be1:
 {load} mov    ebp,esp
.L4a2be3:
 push   0xffffffff
.L4a2be5:
 push   0x902a48
.L4a2bea:
 mov    eax,fs:0x0
.L4a2bf0:
 push   eax
.L4a2bf1:
 sub    esp,0x6c
.L4a2bf4:
 mov    eax,[0xbaabc0]
.L4a2bf9:
 {load} xor    eax,ebp
.L4a2bfb:
 mov    DWORD PTR [ebp-0x10],eax
.L4a2bfe:
 push   ebx
.L4a2bff:
 push   esi
.L4a2c00:
 push   edi
.L4a2c01:
 push   eax
.L4a2c02:
 lea    eax,[ebp-0xc]
.L4a2c05:
 mov    fs:0x0,eax
.L4a2c0b:
 {load} mov    esi,ecx
.L4a2c0d:
 push   0x9fc368
.L4a2c12:
 call   DWORD PTR [0x92d0c4]
.L4a2c18:
 cmp    DWORD PTR [esi+0x70],0x0
.L4a2c1c:
 jne .L4a2c29
.L4a2c1e:
 push   0x20
.L4a2c20:
 call   DWORD PTR [0x92d630]
.L4a2c26:
 mov    DWORD PTR [esi+0x70],eax
.L4a2c29:
 mov    edx,DWORD PTR [esi+0x70]
.L4a2c2c:
 test   edx,edx
.L4a2c2e:
 jne .L4a2cc9
.L4a2c34:
 call   DWORD PTR [0x92d108]
.L4a2c3a:
 {load} mov    edx,eax
.L4a2c3c:
 lea    ecx,[ebp-0x70]
.L4a2c3f:
 call target_44f160
.L4a2c44:
 push   eax
.L4a2c45:
 lea    eax,[ebp-0x28]
.L4a2c48:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a2c4f:
 push   0x9fc344
.L4a2c54:
 push   eax
.L4a2c55:
 call target_4b2d20
.L4a2c5a:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a2c60:
 {load} mov    ebx,eax
.L4a2c62:
 add    esi,0x54
.L4a2c65:
 add    esp,0xc
.L4a2c68:
 {load} cmp    esi,ebx
.L4a2c6a:
 je .L4a2c92
.L4a2c6c:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a2c70:
 jb .L4a2c79
.L4a2c72:
 push   DWORD PTR [esi]
.L4a2c74:
 call   edi
.L4a2c76:
 add    esp,0x4
.L4a2c79:
 mov    DWORD PTR [esi+0x14],0xf
.L4a2c80:
 {load} mov    ecx,esi
.L4a2c82:
 mov    DWORD PTR [esi+0x10],0x0
.L4a2c89:
 push   ebx
.L4a2c8a:
 mov    BYTE PTR [esi],0x0
.L4a2c8d:
 call target_416fa0
.L4a2c92:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a2c96:
 jb .L4a2ca0
.L4a2c98:
 push   DWORD PTR [ebp-0x28]
.L4a2c9b:
 call   edi
.L4a2c9d:
 add    esp,0x4
.L4a2ca0:
 cmp    DWORD PTR [ebp-0x5c],0x10
.L4a2ca4:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a2cab:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a2cb2:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a2cb6:
 jb .L4a2e2a
.L4a2cbc:
 push   DWORD PTR [ebp-0x70]
.L4a2cbf:
 call   edi
.L4a2cc1:
 add    esp,0x4
.L4a2cc4:
 jmp .L4a2e2a
.L4a2cc9:
 lea    eax,[esi+0x26c]
.L4a2ccf:
 mov    DWORD PTR [esi+0x6c],0x0
.L4a2cd6:
 push   eax
.L4a2cd7:
 mov    DWORD PTR [esi+0x1b0],0x1
.L4a2ce1:
 mov    ecx,DWORD PTR [edx]
.L4a2ce3:
 push   0x0
.L4a2ce5:
 push   edx
.L4a2ce6:
 call   DWORD PTR [ecx+0x20]
.L4a2ce9:
 test   eax,eax
.L4a2ceb:
 jns .L4a2d8b
.L4a2cf1:
 {load} mov    edx,eax
.L4a2cf3:
 lea    ecx,[ebp-0x70]
.L4a2cf6:
 call target_44f380
.L4a2cfb:
 push   eax
.L4a2cfc:
 lea    eax,[ebp-0x28]
.L4a2cff:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a2d06:
 push   0x9fc3fc
.L4a2d0b:
 push   eax
.L4a2d0c:
 call target_4b2d20
.L4a2d11:
 mov    ebx,DWORD PTR [0x92d4b0]
.L4a2d17:
 lea    edi,[esi+0x54]
.L4a2d1a:
 add    esp,0xc
.L4a2d1d:
 mov    DWORD PTR [ebp-0x78],eax
.L4a2d20:
 {load} cmp    edi,eax
.L4a2d22:
 je .L4a2d4d
.L4a2d24:
 cmp    DWORD PTR [edi+0x14],0x10
.L4a2d28:
 jb .L4a2d34
.L4a2d2a:
 push   DWORD PTR [edi]
.L4a2d2c:
 call   ebx
.L4a2d2e:
 mov    eax,DWORD PTR [ebp-0x78]
.L4a2d31:
 add    esp,0x4
.L4a2d34:
 mov    DWORD PTR [edi+0x14],0xf
.L4a2d3b:
 {load} mov    ecx,edi
.L4a2d3d:
 mov    DWORD PTR [edi+0x10],0x0
.L4a2d44:
 push   eax
.L4a2d45:
 mov    BYTE PTR [edi],0x0
.L4a2d48:
 call target_416fa0
.L4a2d4d:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a2d51:
 jb .L4a2d5b
.L4a2d53:
 push   DWORD PTR [ebp-0x28]
.L4a2d56:
 call   ebx
.L4a2d58:
 add    esp,0x4
.L4a2d5b:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a2d62:
 cmp    DWORD PTR [ebp-0x5c],0x10
.L4a2d66:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a2d6d:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a2d74:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a2d78:
 jb .L4a2e1a
.L4a2d7e:
 push   DWORD PTR [ebp-0x70]
.L4a2d81:
 call   ebx
.L4a2d83:
 add    esp,0x4
.L4a2d86:
 jmp .L4a2e1a
.L4a2d8b:
 mov    eax,DWORD PTR [esi+0x278]
.L4a2d91:
 lea    edx,[ebp-0x4c]
.L4a2d94:
 mov    DWORD PTR [ebp-0x20],eax
.L4a2d97:
 lea    ecx,[ebp-0x20]
.L4a2d9a:
 mov    DWORD PTR [ebp-0x44],eax
.L4a2d9d:
 mov    edi,0x0
.L4a2da2:
 mov    al,BYTE PTR [esi+0x4be]
.L4a2da8:
 test   al,al
.L4a2daa:
 mov    DWORD PTR [ebp-0x1c],0x16
.L4a2db1:
 mov    DWORD PTR [ebp-0x18],0x17
.L4a2db8:
 cmove  edx,ecx
.L4a2dbb:
 mov    DWORD PTR [ebp-0x14],0x18
.L4a2dc2:
 {load} xor    ebx,ebx
.L4a2dc4:
 mov    DWORD PTR [ebp-0x4c],0x17
.L4a2dcb:
 test   al,al
.L4a2dcd:
 mov    DWORD PTR [ebp-0x48],0x18
.L4a2dd4:
 mov    DWORD PTR [ebp-0x78],edx
.L4a2dd7:
 sete   bl
.L4a2dda:
 add    ebx,0x3
.L4a2ddd:
 je .L4a2e0b
.L4a2ddf:
 nop
.L4a2de0:
 push   DWORD PTR [edx+edi*4]
.L4a2de3:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2de6:
 push   0x1
.L4a2de8:
 push   0x1
.L4a2dea:
 push   DWORD PTR [esi+0x278]
.L4a2df0:
 mov    ecx,DWORD PTR [eax]
.L4a2df2:
 push   DWORD PTR [esi+0x1b0]
.L4a2df8:
 push   DWORD PTR [esi+0x6c]
.L4a2dfb:
 push   eax
.L4a2dfc:
 call   DWORD PTR [ecx+0x28]
.L4a2dff:
 test   eax,eax
.L4a2e01:
 jns .L4a2e48
.L4a2e03:
 mov    edx,DWORD PTR [ebp-0x78]
.L4a2e06:
 inc    edi
.L4a2e07:
 {load} cmp    edi,ebx
.L4a2e09:
 jb .L4a2de0
.L4a2e0b:
 push   0x22
.L4a2e0d:
 push   0x9fc3d8
.L4a2e12:
 lea    ecx,[esi+0x54]
.L4a2e15:
 call target_416520
.L4a2e1a:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2e1d:
 push   eax
.L4a2e1e:
 mov    ecx,DWORD PTR [eax]
.L4a2e20:
 call   DWORD PTR [ecx+0x8]
.L4a2e23:
 mov    DWORD PTR [esi+0x70],0x0
.L4a2e2a:
 {load} xor    al,al
.L4a2e2c:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a2e2f:
 mov    DWORD PTR fs:0x0,ecx
.L4a2e36:
 pop    ecx
.L4a2e37:
 pop    edi
.L4a2e38:
 pop    esi
.L4a2e39:
 pop    ebx
.L4a2e3a:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a2e3d:
 {load} xor    ecx,ebp
.L4a2e3f:
 call target_8f8958
.L4a2e44:
 {load} mov    esp,ebp
.L4a2e46:
 pop    ebp
.L4a2e47:
 ret
.L4a2e48:
 mov    eax,DWORD PTR [ebp-0x78]
.L4a2e4b:
 mov    edi,DWORD PTR [eax+edi*4]
.L4a2e4e:
 test   edi,edi
.L4a2e50:
 je .L4a2e0b
.L4a2e52:
 movdqa xmm0,XMMWORD PTR [0xa06eb0]
.L4a2e5a:
 {load} xor    ebx,ebx
.L4a2e5c:
 movdqu XMMWORD PTR [ebp-0x24],xmm0
.L4a2e61:
 mov    DWORD PTR [ebp-0x14],0x15
.L4a2e68:
 jmp .L4a2e70
.L4a2e6a:
 {disp32} lea    ebx,[ebx+0x0]
.L4a2e70:
 push   DWORD PTR [ebp+ebx*4-0x24]
.L4a2e74:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a2e77:
 push   0x3
.L4a2e79:
 push   0x1
.L4a2e7b:
 push   DWORD PTR [esi+0x278]
.L4a2e81:
 mov    eax,DWORD PTR [ecx]
.L4a2e83:
 push   DWORD PTR [esi+0x1b0]
.L4a2e89:
 push   DWORD PTR [esi+0x6c]
.L4a2e8c:
 push   ecx
.L4a2e8d:
 call   DWORD PTR [eax+0x28]
.L4a2e90:
 test   eax,eax
.L4a2e92:
 js .L4a2ebb
.L4a2e94:
 cmp    DWORD PTR [esi+0x278],edi
.L4a2e9a:
 je .L4a2ec3
.L4a2e9c:
 push   DWORD PTR [ebp+ebx*4-0x24]
.L4a2ea0:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2ea3:
 push   0x3
.L4a2ea5:
 push   0x1
.L4a2ea7:
 push   edi
.L4a2ea8:
 push   DWORD PTR [esi+0x1b0]
.L4a2eae:
 mov    ecx,DWORD PTR [eax]
.L4a2eb0:
 push   DWORD PTR [esi+0x6c]
.L4a2eb3:
 push   eax
.L4a2eb4:
 call   DWORD PTR [ecx+0x28]
.L4a2eb7:
 test   eax,eax
.L4a2eb9:
 jns .L4a2ec3
.L4a2ebb:
 inc    ebx
.L4a2ebc:
 cmp    ebx,0x5
.L4a2ebf:
 jb .L4a2e70
.L4a2ec1:
 jmp .L4a2ecd
.L4a2ec3:
 mov    eax,DWORD PTR [ebp+ebx*4-0x24]
.L4a2ec7:
 mov    DWORD PTR [esi+0x1f0],eax
.L4a2ecd:
 cmp    DWORD PTR [esi+0x1f0],0x0
.L4a2ed4:
 jne .L4a2ee2
.L4a2ed6:
 push   0x2d
.L4a2ed8:
 push   0x9fc3a8
.L4a2edd:
 jmp .L4a2e12
.L4a2ee2:
 mov    DWORD PTR [ebp-0x4c],0x33
.L4a2ee9:
 {load} xor    ebx,ebx
.L4a2eeb:
 mov    DWORD PTR [ebp-0x48],0x22
.L4a2ef2:
 mov    DWORD PTR [ebp-0x44],0x34
.L4a2ef9:
 {disp32} lea    esp,[esp+0x0]
.L4a2f00:
 push   DWORD PTR [ebp+ebx*4-0x4c]
.L4a2f04:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a2f07:
 push   0x3
.L4a2f09:
 push   0x1
.L4a2f0b:
 push   DWORD PTR [esi+0x278]
.L4a2f11:
 mov    eax,DWORD PTR [ecx]
.L4a2f13:
 push   DWORD PTR [esi+0x1b0]
.L4a2f19:
 push   DWORD PTR [esi+0x6c]
.L4a2f1c:
 push   ecx
.L4a2f1d:
 call   DWORD PTR [eax+0x28]
.L4a2f20:
 test   eax,eax
.L4a2f22:
 js .L4a2f4b
.L4a2f24:
 cmp    DWORD PTR [esi+0x278],edi
.L4a2f2a:
 je .L4a2f53
.L4a2f2c:
 push   DWORD PTR [ebp+ebx*4-0x4c]
.L4a2f30:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2f33:
 push   0x3
.L4a2f35:
 push   0x1
.L4a2f37:
 push   edi
.L4a2f38:
 push   DWORD PTR [esi+0x1b0]
.L4a2f3e:
 mov    ecx,DWORD PTR [eax]
.L4a2f40:
 push   DWORD PTR [esi+0x6c]
.L4a2f43:
 push   eax
.L4a2f44:
 call   DWORD PTR [ecx+0x28]
.L4a2f47:
 test   eax,eax
.L4a2f49:
 jns .L4a2f53
.L4a2f4b:
 inc    ebx
.L4a2f4c:
 cmp    ebx,0x3
.L4a2f4f:
 jb .L4a2f00
.L4a2f51:
 jmp .L4a2f5d
.L4a2f53:
 mov    eax,DWORD PTR [ebp+ebx*4-0x4c]
.L4a2f57:
 mov    DWORD PTR [esi+0x1f4],eax
.L4a2f5d:
 cmp    DWORD PTR [esi+0x1f4],0x0
.L4a2f64:
 jne .L4a2f72
.L4a2f66:
 push   0x33
.L4a2f68:
 push   0x9fc374
.L4a2f6d:
 jmp .L4a2e12
.L4a2f72:
 movdqa xmm0,XMMWORD PTR [0xa06e70]
.L4a2f7a:
 {load} xor    ebx,ebx
.L4a2f7c:
 movdqu XMMWORD PTR [ebp-0x24],xmm0
.L4a2f81:
 mov    DWORD PTR [ebp-0x14],0x23
.L4a2f88:
 jmp .L4a2f90
.L4a2f8a:
 {disp32} lea    ebx,[ebx+0x0]
.L4a2f90:
 push   DWORD PTR [ebp+ebx*4-0x24]
.L4a2f94:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a2f97:
 push   0x3
.L4a2f99:
 push   0x1
.L4a2f9b:
 push   DWORD PTR [esi+0x278]
.L4a2fa1:
 mov    eax,DWORD PTR [ecx]
.L4a2fa3:
 push   DWORD PTR [esi+0x1b0]
.L4a2fa9:
 push   DWORD PTR [esi+0x6c]
.L4a2fac:
 push   ecx
.L4a2fad:
 call   DWORD PTR [eax+0x28]
.L4a2fb0:
 test   eax,eax
.L4a2fb2:
 js .L4a2fdb
.L4a2fb4:
 cmp    DWORD PTR [esi+0x278],edi
.L4a2fba:
 je .L4a2fe3
.L4a2fbc:
 push   DWORD PTR [ebp+ebx*4-0x24]
.L4a2fc0:
 mov    eax,DWORD PTR [esi+0x70]
.L4a2fc3:
 push   0x3
.L4a2fc5:
 push   0x1
.L4a2fc7:
 push   edi
.L4a2fc8:
 push   DWORD PTR [esi+0x1b0]
.L4a2fce:
 mov    ecx,DWORD PTR [eax]
.L4a2fd0:
 push   DWORD PTR [esi+0x6c]
.L4a2fd3:
 push   eax
.L4a2fd4:
 call   DWORD PTR [ecx+0x28]
.L4a2fd7:
 test   eax,eax
.L4a2fd9:
 jns .L4a2fe3
.L4a2fdb:
 inc    ebx
.L4a2fdc:
 cmp    ebx,0x5
.L4a2fdf:
 jb .L4a2f90
.L4a2fe1:
 jmp .L4a2fed
.L4a2fe3:
 mov    eax,DWORD PTR [ebp+ebx*4-0x24]
.L4a2fe7:
 mov    DWORD PTR [esi+0x1f8],eax
.L4a2fed:
 cmp    DWORD PTR [esi+0x1f8],0x0
.L4a2ff4:
 jne .L4a3002
.L4a2ff6:
 push   0x28
.L4a2ff8:
 push   0x9fc478
.L4a2ffd:
 jmp .L4a2e12
.L4a3002:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a3005:
 lea    eax,[esi+0x80]
.L4a300b:
 push   eax
.L4a300c:
 push   DWORD PTR [esi+0x1b0]
.L4a3012:
 push   DWORD PTR [esi+0x6c]
.L4a3015:
 mov    edx,DWORD PTR [ecx]
.L4a3017:
 push   ecx
.L4a3018:
 call   DWORD PTR [edx+0x38]
.L4a301b:
 test   eax,eax
.L4a301d:
 jns .L4a3108
.L4a3023:
 {load} mov    edx,eax
.L4a3025:
 lea    ecx,[ebp-0x40]
.L4a3028:
 call target_44f380
.L4a302d:
 push   eax
.L4a302e:
 lea    eax,[ebp-0x58]
.L4a3031:
 mov    DWORD PTR [ebp-0x4],0x2
.L4a3038:
 push   0x9fc458
.L4a303d:
 push   eax
.L4a303e:
 call target_4b2d20
.L4a3043:
 {load} mov    edi,eax
.L4a3045:
 lea    ebx,[esi+0x54]
.L4a3048:
 mov    eax,[0x92d4b0]
.L4a304d:
 add    esp,0xc
.L4a3050:
 {load} cmp    ebx,edi
.L4a3052:
 je .L4a30b8
.L4a3054:
 cmp    DWORD PTR [ebx+0x14],0x10
.L4a3058:
 jb .L4a3061
.L4a305a:
 push   DWORD PTR [ebx]
.L4a305c:
 call   eax
.L4a305e:
 add    esp,0x4
.L4a3061:
 mov    DWORD PTR [ebx+0x14],0xf
.L4a3068:
 mov    DWORD PTR [ebx+0x10],0x0
.L4a306f:
 mov    BYTE PTR [ebx],0x0
.L4a3072:
 cmp    DWORD PTR [edi+0x14],0x10
.L4a3076:
 jae .L4a308c
.L4a3078:
 mov    eax,DWORD PTR [edi+0x10]
.L4a307b:
 inc    eax
.L4a307c:
 je .L4a3096
.L4a307e:
 push   eax
.L4a307f:
 push   edi
.L4a3080:
 push   ebx
.L4a3081:
 call   DWORD PTR [0x92d4a4]
.L4a3087:
 add    esp,0xc
.L4a308a:
 jmp .L4a3096
.L4a308c:
 mov    eax,DWORD PTR [edi]
.L4a308e:
 mov    DWORD PTR [ebx],eax
.L4a3090:
 mov    DWORD PTR [edi],0x0
.L4a3096:
 mov    eax,DWORD PTR [edi+0x10]
.L4a3099:
 mov    DWORD PTR [ebx+0x10],eax
.L4a309c:
 mov    eax,DWORD PTR [edi+0x14]
.L4a309f:
 mov    DWORD PTR [ebx+0x14],eax
.L4a30a2:
 mov    eax,[0x92d4b0]
.L4a30a7:
 mov    DWORD PTR [edi+0x14],0xf
.L4a30ae:
 mov    DWORD PTR [edi+0x10],0x0
.L4a30b5:
 mov    BYTE PTR [edi],0x0
.L4a30b8:
 cmp    DWORD PTR [ebp-0x44],0x10
.L4a30bc:
 jb .L4a30c6
.L4a30be:
 push   DWORD PTR [ebp-0x58]
.L4a30c1:
 call   eax
.L4a30c3:
 add    esp,0x4
.L4a30c6:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a30cd:
 cmp    DWORD PTR [ebp-0x2c],0x10
.L4a30d1:
 mov    DWORD PTR [ebp-0x44],0xf
.L4a30d8:
 mov    DWORD PTR [ebp-0x48],0x0
.L4a30df:
 mov    BYTE PTR [ebp-0x58],0x0
.L4a30e3:
 jb .L4a30f1
.L4a30e5:
 push   DWORD PTR [ebp-0x40]
.L4a30e8:
 call   DWORD PTR [0x92d4b0]
.L4a30ee:
 add    esp,0x4
.L4a30f1:
 mov    DWORD PTR [ebp-0x2c],0xf
.L4a30f8:
 mov    DWORD PTR [ebp-0x30],0x0
.L4a30ff:
 mov    BYTE PTR [ebp-0x40],0x0
.L4a3103:
 jmp .L4a2e1a
.L4a3108:
 mov    eax,DWORD PTR [esi+0xec]
.L4a310e:
 cmp    DWORD PTR [esi+0x9a4],eax
.L4a3114:
 jbe .L4a311c
.L4a3116:
 mov    DWORD PTR [esi+0x9a4],eax
.L4a311c:
 lea    eax,[ebp-0x71]
.L4a311f:
 mov    BYTE PTR [ebp-0x71],0x0
.L4a3123:
 push   eax
.L4a3124:
 {load} mov    ecx,esi
.L4a3126:
 call target_4a59e0
.L4a312b:
 test   al,al
.L4a312d:
 je .L4a2e1a
.L4a3133:
 mov    al,BYTE PTR [esi+0x4be]
.L4a3139:
 lea    edx,[ebp-0x40]
.L4a313c:
 movdqa xmm0,XMMWORD PTR [0xa06e90]
.L4a3144:
 lea    ecx,[ebp-0x28]
.L4a3147:
 test   al,al
.L4a3149:
 mov    DWORD PTR [ebp-0x18],0x4d
.L4a3150:
 movdqu XMMWORD PTR [ebp-0x28],xmm0
.L4a3155:
 cmove  edx,ecx
.L4a3158:
 mov    eax,0x6
.L4a315d:
 movdqa xmm0,XMMWORD PTR [0xa06ea0]
.L4a3165:
 {load} xor    ebx,ebx
.L4a3167:
 mov    DWORD PTR [ebp-0x14],0x50
.L4a316e:
 mov    DWORD PTR [ebp-0x30],0x4d
.L4a3175:
 mov    DWORD PTR [ebp-0x2c],0x47
.L4a317c:
 mov    DWORD PTR [ebp-0x78],edx
.L4a317f:
 movdqu XMMWORD PTR [ebp-0x40],xmm0
.L4a3184:
 test   eax,eax
.L4a3186:
 je .L4a33e3
.L4a318c:
 {disp8} lea    esp,[esp+0x0]
.L4a3190:
 push   DWORD PTR [edx+ebx*4]
.L4a3193:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a3196:
 push   0x1
.L4a3198:
 push   0x2
.L4a319a:
 push   DWORD PTR [esi+0x278]
.L4a31a0:
 mov    eax,DWORD PTR [ecx]
.L4a31a2:
 push   DWORD PTR [esi+0x1b0]
.L4a31a8:
 push   DWORD PTR [esi+0x6c]
.L4a31ab:
 push   ecx
.L4a31ac:
 call   DWORD PTR [eax+0x28]
.L4a31af:
 test   eax,eax
.L4a31b1:
 js .L4a3225
.L4a31b3:
 cmp    DWORD PTR [esi+0x278],edi
.L4a31b9:
 je .L4a31dc
.L4a31bb:
 mov    edx,DWORD PTR [ebp-0x78]
.L4a31be:
 mov    eax,DWORD PTR [esi+0x70]
.L4a31c1:
 push   DWORD PTR [edx+ebx*4]
.L4a31c4:
 mov    ecx,DWORD PTR [eax]
.L4a31c6:
 push   0x1
.L4a31c8:
 push   0x2
.L4a31ca:
 push   edi
.L4a31cb:
 push   DWORD PTR [esi+0x1b0]
.L4a31d1:
 push   DWORD PTR [esi+0x6c]
.L4a31d4:
 push   eax
.L4a31d5:
 call   DWORD PTR [ecx+0x28]
.L4a31d8:
 test   eax,eax
.L4a31da:
 js .L4a3225
.L4a31dc:
 mov    edx,DWORD PTR [ebp-0x78]
.L4a31df:
 mov    eax,DWORD PTR [esi+0x70]
.L4a31e2:
 push   DWORD PTR [edx+ebx*4]
.L4a31e5:
 mov    ecx,DWORD PTR [eax]
.L4a31e7:
 push   edi
.L4a31e8:
 push   DWORD PTR [esi+0x278]
.L4a31ee:
 push   DWORD PTR [esi+0x1b0]
.L4a31f4:
 push   DWORD PTR [esi+0x6c]
.L4a31f7:
 push   eax
.L4a31f8:
 call   DWORD PTR [ecx+0x30]
.L4a31fb:
 test   eax,eax
.L4a31fd:
 js .L4a3225
.L4a31ff:
 cmp    DWORD PTR [esi+0x278],edi
.L4a3205:
 je .L4a3237
.L4a3207:
 mov    edx,DWORD PTR [ebp-0x78]
.L4a320a:
 mov    eax,DWORD PTR [esi+0x70]
.L4a320d:
 push   DWORD PTR [edx+ebx*4]
.L4a3210:
 mov    ecx,DWORD PTR [eax]
.L4a3212:
 push   edi
.L4a3213:
 push   edi
.L4a3214:
 push   DWORD PTR [esi+0x1b0]
.L4a321a:
 push   DWORD PTR [esi+0x6c]
.L4a321d:
 push   eax
.L4a321e:
 call   DWORD PTR [ecx+0x30]
.L4a3221:
 test   eax,eax
.L4a3223:
 jns .L4a3237
.L4a3225:
 inc    ebx
.L4a3226:
 cmp    ebx,0x6
.L4a3229:
 jae .L4a33e3
.L4a322f:
 mov    edx,DWORD PTR [ebp-0x78]
.L4a3232:
 jmp .L4a3190
.L4a3237:
 mov    eax,DWORD PTR [ebp-0x78]
.L4a323a:
 mov    ebx,DWORD PTR [eax+ebx*4]
.L4a323d:
 test   ebx,ebx
.L4a323f:
 je .L4a33e3
.L4a3245:
 cmp    ebx,0x49
.L4a3248:
 je .L4a3258
.L4a324a:
 cmp    ebx,0x4b
.L4a324d:
 je .L4a3258
.L4a324f:
 cmp    ebx,0x4f
.L4a3252:
 je .L4a3258
.L4a3254:
 {load} xor    eax,eax
.L4a3256:
 jmp .L4a325d
.L4a3258:
 mov    eax,0x1
.L4a325d:
 push   ebx
.L4a325e:
 push   edi
.L4a325f:
 push   DWORD PTR [esi+0x278]
.L4a3265:
 {load} mov    ecx,esi
.L4a3267:
 mov    BYTE PTR [esi+0x208],al
.L4a326d:
 push   DWORD PTR [esi+0x1b0]
.L4a3273:
 push   DWORD PTR [esi+0x6c]
.L4a3276:
 call target_4a29d0
.L4a327b:
 test   al,al
.L4a327d:
 je .L4a2e1a
.L4a3283:
 mov    edx,DWORD PTR [esi+0x200]
.L4a3289:
 test   edx,edx
.L4a328b:
 je .L4a333d
.L4a3291:
 mov    eax,DWORD PTR [esi+0x70]
.L4a3294:
 push   0x0
.L4a3296:
 push   edx
.L4a3297:
 push   0x1
.L4a3299:
 push   DWORD PTR [esi+0x278]
.L4a329f:
 mov    ecx,DWORD PTR [eax]
.L4a32a1:
 push   DWORD PTR [esi+0x1b0]
.L4a32a7:
 push   DWORD PTR [esi+0x6c]
.L4a32aa:
 push   eax
.L4a32ab:
 call   DWORD PTR [ecx+0x2c]
.L4a32ae:
 test   eax,eax
.L4a32b0:
 jns .L4a32bc
.L4a32b2:
 mov    DWORD PTR [esi+0x200],0x0
.L4a32bc:
 mov    eax,DWORD PTR [esi+0x70]
.L4a32bf:
 push   0x0
.L4a32c1:
 push   DWORD PTR [esi+0x200]
.L4a32c7:
 mov    ecx,DWORD PTR [eax]
.L4a32c9:
 push   0x1
.L4a32cb:
 push   ebx
.L4a32cc:
 push   DWORD PTR [esi+0x1b0]
.L4a32d2:
 push   DWORD PTR [esi+0x6c]
.L4a32d5:
 push   eax
.L4a32d6:
 call   DWORD PTR [ecx+0x2c]
.L4a32d9:
 test   eax,eax
.L4a32db:
 jns .L4a32e7
.L4a32dd:
 mov    DWORD PTR [esi+0x200],0x0
.L4a32e7:
 mov    eax,DWORD PTR [esi+0x70]
.L4a32ea:
 push   0x0
.L4a32ec:
 push   DWORD PTR [esi+0x200]
.L4a32f2:
 mov    ecx,DWORD PTR [eax]
.L4a32f4:
 push   0x0
.L4a32f6:
 push   edi
.L4a32f7:
 push   DWORD PTR [esi+0x1b0]
.L4a32fd:
 push   DWORD PTR [esi+0x6c]
.L4a3300:
 push   eax
.L4a3301:
 call   DWORD PTR [ecx+0x2c]
.L4a3304:
 test   eax,eax
.L4a3306:
 jns .L4a3312
.L4a3308:
 mov    DWORD PTR [esi+0x200],0x0
.L4a3312:
 mov    eax,DWORD PTR [esi+0x70]
.L4a3315:
 push   0x0
.L4a3317:
 push   DWORD PTR [esi+0x200]
.L4a331d:
 mov    ecx,DWORD PTR [eax]
.L4a331f:
 push   0x0
.L4a3321:
 push   ebx
.L4a3322:
 push   DWORD PTR [esi+0x1b0]
.L4a3328:
 push   DWORD PTR [esi+0x6c]
.L4a332b:
 push   eax
.L4a332c:
 call   DWORD PTR [ecx+0x2c]
.L4a332f:
 test   eax,eax
.L4a3331:
 jns .L4a333d
.L4a3333:
 mov    DWORD PTR [esi+0x200],0x0
.L4a333d:
 cmp    BYTE PTR [esi+0x4bc],0x0
.L4a3344:
 je .L4a3350
.L4a3346:
 mov    DWORD PTR [esi+0x1fc],0x72
.L4a3350:
 mov    eax,DWORD PTR [esi+0x9c]
.L4a3356:
 mov    DWORD PTR [esi+0x1b4],0x0
.L4a3360:
 test   eax,0x10000
.L4a3365:
 je .L4a338a
.L4a3367:
 cmp    BYTE PTR [ebp-0x71],0x0
.L4a336b:
 jne .L4a338a
.L4a336d:
 mov    DWORD PTR [esi+0x1b4],0x40
.L4a3377:
 test   eax,0x100000
.L4a337c:
 je .L4a3394
.L4a337e:
 mov    DWORD PTR [esi+0x1b4],0x50
.L4a3388:
 jmp .L4a3394
.L4a338a:
 mov    DWORD PTR [esi+0x1b4],0x20
.L4a3394:
 mov    DWORD PTR [esi+0x1c4],0x1
.L4a339e:
 mov    al,0x1
.L4a33a0:
 mov    DWORD PTR [esi+0x1cc],0x0
.L4a33aa:
 mov    DWORD PTR [esi+0x1d0],0x1
.L4a33b4:
 mov    DWORD PTR [esi+0x1d4],0x0
.L4a33be:
 mov    DWORD PTR [esi+0x1c0],edi
.L4a33c4:
 mov    DWORD PTR [esi+0x1e0],ebx
.L4a33ca:
 mov    DWORD PTR [esi+0x1dc],0x0
.L4a33d4:
 mov    DWORD PTR [esi+0x1ec],0x0
.L4a33de:
 jmp .L4a2e2c
.L4a33e3:
 push   0x31
.L4a33e5:
 push   0x9fc424
.L4a33ea:
 jmp .L4a2e12
.equ target_416520,0x416520
.equ target_416fa0,0x416fa0
.equ target_44f160,0x44f160
.equ target_44f380,0x44f380
.equ target_4a29d0,0x4a29d0
.equ target_4a59e0,0x4a59e0
.equ target_4b2d20,0x4b2d20
.equ target_8f8958,0x8f8958
