# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a5c40..0x4a5e76
.intel_syntax noprefix
.code32
.text
.global original_device_destroy
original_device_destroy:
.L4a5c40:
 push   esi
.L4a5c41:
 {load} mov    esi,ecx
.L4a5c43:
 call target_4a4d20
.L4a5c48:
 mov    ecx,DWORD PTR [esi+0x290]
.L4a5c4e:
 test   ecx,ecx
.L4a5c50:
 je .L4a5c58
.L4a5c52:
 mov    eax,DWORD PTR [ecx]
.L4a5c54:
 push   ecx
.L4a5c55:
 call   DWORD PTR [eax+0x8]
.L4a5c58:
 mov    ecx,DWORD PTR [esi+0x28c]
.L4a5c5e:
 mov    DWORD PTR [esi+0x290],0x0
.L4a5c68:
 test   ecx,ecx
.L4a5c6a:
 je .L4a5c72
.L4a5c6c:
 mov    eax,DWORD PTR [ecx]
.L4a5c6e:
 push   ecx
.L4a5c6f:
 call   DWORD PTR [eax+0x8]
.L4a5c72:
 mov    ecx,DWORD PTR [esi+0x204]
.L4a5c78:
 mov    DWORD PTR [esi+0x28c],0x0
.L4a5c82:
 test   ecx,ecx
.L4a5c84:
 je .L4a5c8c
.L4a5c86:
 mov    eax,DWORD PTR [ecx]
.L4a5c88:
 push   ecx
.L4a5c89:
 call   DWORD PTR [eax+0x8]
.L4a5c8c:
 push   ebx
.L4a5c8d:
 push   edi
.L4a5c8e:
 mov    DWORD PTR [esi+0x204],0x0
.L4a5c98:
 lea    edi,[esi+0x298]
.L4a5c9e:
 mov    ebx,0x21
.L4a5ca3:
 mov    ecx,DWORD PTR [edi]
.L4a5ca5:
 test   ecx,ecx
.L4a5ca7:
 je .L4a5caf
.L4a5ca9:
 mov    eax,DWORD PTR [ecx]
.L4a5cab:
 push   ecx
.L4a5cac:
 call   DWORD PTR [eax+0x8]
.L4a5caf:
 mov    DWORD PTR [edi],0x0
.L4a5cb5:
 add    edi,0x4
.L4a5cb8:
 dec    ebx
.L4a5cb9:
 jne .L4a5ca3
.L4a5cbb:
 mov    ecx,DWORD PTR [esi+0x21c]
.L4a5cc1:
 test   ecx,ecx
.L4a5cc3:
 je .L4a5ccb
.L4a5cc5:
 mov    eax,DWORD PTR [ecx]
.L4a5cc7:
 push   ecx
.L4a5cc8:
 call   DWORD PTR [eax+0x8]
.L4a5ccb:
 mov    ecx,DWORD PTR [esi+0x218]
.L4a5cd1:
 mov    DWORD PTR [esi+0x21c],0x0
.L4a5cdb:
 test   ecx,ecx
.L4a5cdd:
 je .L4a5ce5
.L4a5cdf:
 mov    eax,DWORD PTR [ecx]
.L4a5ce1:
 push   ecx
.L4a5ce2:
 call   DWORD PTR [eax+0x8]
.L4a5ce5:
 mov    ecx,DWORD PTR [esi+0x214]
.L4a5ceb:
 mov    DWORD PTR [esi+0x218],0x0
.L4a5cf5:
 test   ecx,ecx
.L4a5cf7:
 je .L4a5cff
.L4a5cf9:
 mov    eax,DWORD PTR [ecx]
.L4a5cfb:
 push   ecx
.L4a5cfc:
 call   DWORD PTR [eax+0x8]
.L4a5cff:
 mov    DWORD PTR [esi+0x214],0x0
.L4a5d09:
 lea    edi,[esi+0x420]
.L4a5d0f:
 mov    DWORD PTR [esi+0x220],0x0
.L4a5d19:
 mov    ebx,0x2
.L4a5d1e:
 mov    DWORD PTR [esi+0x224],0x0
.L4a5d28:
 mov    DWORD PTR [esi+0x228],0x0
.L4a5d32:
 mov    DWORD PTR [esi+0x22c],0x0
.L4a5d3c:
 {disp8} lea    esp,[esp+0x0]
.L4a5d40:
 mov    ecx,DWORD PTR [edi+0x8]
.L4a5d43:
 test   ecx,ecx
.L4a5d45:
 je .L4a5d4d
.L4a5d47:
 mov    eax,DWORD PTR [ecx]
.L4a5d49:
 push   ecx
.L4a5d4a:
 call   DWORD PTR [eax+0x8]
.L4a5d4d:
 mov    ecx,DWORD PTR [edi]
.L4a5d4f:
 mov    DWORD PTR [edi+0x8],0x0
.L4a5d56:
 test   ecx,ecx
.L4a5d58:
 je .L4a5d60
.L4a5d5a:
 mov    eax,DWORD PTR [ecx]
.L4a5d5c:
 push   ecx
.L4a5d5d:
 call   DWORD PTR [eax+0x8]
.L4a5d60:
 mov    DWORD PTR [edi],0x0
.L4a5d66:
 add    edi,0x4
.L4a5d69:
 dec    ebx
.L4a5d6a:
 jne .L4a5d40
.L4a5d6c:
 mov    ecx,DWORD PTR [esi+0x430]
.L4a5d72:
 pop    edi
.L4a5d73:
 pop    ebx
.L4a5d74:
 test   ecx,ecx
.L4a5d76:
 je .L4a5d7e
.L4a5d78:
 mov    eax,DWORD PTR [ecx]
.L4a5d7a:
 push   ecx
.L4a5d7b:
 call   DWORD PTR [eax+0x8]
.L4a5d7e:
 mov    ecx,DWORD PTR [esi+0x438]
.L4a5d84:
 mov    DWORD PTR [esi+0x430],0x0
.L4a5d8e:
 test   ecx,ecx
.L4a5d90:
 je .L4a5d98
.L4a5d92:
 mov    eax,DWORD PTR [ecx]
.L4a5d94:
 push   ecx
.L4a5d95:
 call   DWORD PTR [eax+0x8]
.L4a5d98:
 mov    ecx,DWORD PTR [esi+0x434]
.L4a5d9e:
 mov    DWORD PTR [esi+0x438],0x0
.L4a5da8:
 test   ecx,ecx
.L4a5daa:
 je .L4a5db2
.L4a5dac:
 mov    eax,DWORD PTR [ecx]
.L4a5dae:
 push   ecx
.L4a5daf:
 call   DWORD PTR [eax+0x8]
.L4a5db2:
 mov    ecx,DWORD PTR [esi+0x440]
.L4a5db8:
 mov    DWORD PTR [esi+0x434],0x0
.L4a5dc2:
 test   ecx,ecx
.L4a5dc4:
 je .L4a5dcc
.L4a5dc6:
 mov    eax,DWORD PTR [ecx]
.L4a5dc8:
 push   ecx
.L4a5dc9:
 call   DWORD PTR [eax+0x8]
.L4a5dcc:
 mov    ecx,DWORD PTR [esi+0x43c]
.L4a5dd2:
 mov    DWORD PTR [esi+0x440],0x0
.L4a5ddc:
 test   ecx,ecx
.L4a5dde:
 je .L4a5de6
.L4a5de0:
 mov    eax,DWORD PTR [ecx]
.L4a5de2:
 push   ecx
.L4a5de3:
 call   DWORD PTR [eax+0x8]
.L4a5de6:
 mov    ecx,DWORD PTR [esi+0x448]
.L4a5dec:
 mov    DWORD PTR [esi+0x43c],0x0
.L4a5df6:
 test   ecx,ecx
.L4a5df8:
 je .L4a5e00
.L4a5dfa:
 mov    eax,DWORD PTR [ecx]
.L4a5dfc:
 push   ecx
.L4a5dfd:
 call   DWORD PTR [eax+0x8]
.L4a5e00:
 mov    ecx,DWORD PTR [esi+0x444]
.L4a5e06:
 mov    DWORD PTR [esi+0x448],0x0
.L4a5e10:
 test   ecx,ecx
.L4a5e12:
 je .L4a5e1a
.L4a5e14:
 mov    eax,DWORD PTR [ecx]
.L4a5e16:
 push   ecx
.L4a5e17:
 call   DWORD PTR [eax+0x8]
.L4a5e1a:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a5e1d:
 mov    DWORD PTR [esi+0x444],0x0
.L4a5e27:
 test   ecx,ecx
.L4a5e29:
 je .L4a5e31
.L4a5e2b:
 mov    eax,DWORD PTR [ecx]
.L4a5e2d:
 push   ecx
.L4a5e2e:
 call   DWORD PTR [eax+0x8]
.L4a5e31:
 mov    ecx,DWORD PTR [esi+0x7c]
.L4a5e34:
 mov    DWORD PTR [esi+0x78],0x0
.L4a5e3b:
 test   ecx,ecx
.L4a5e3d:
 je .L4a5e45
.L4a5e3f:
 mov    eax,DWORD PTR [ecx]
.L4a5e41:
 push   ecx
.L4a5e42:
 call   DWORD PTR [eax+0x8]
.L4a5e45:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a5e48:
 mov    DWORD PTR [esi+0x7c],0x0
.L4a5e4f:
 test   ecx,ecx
.L4a5e51:
 je .L4a5e59
.L4a5e53:
 mov    eax,DWORD PTR [ecx]
.L4a5e55:
 push   ecx
.L4a5e56:
 call   DWORD PTR [eax+0x8]
.L4a5e59:
 mov    ecx,DWORD PTR [esi+0x74]
.L4a5e5c:
 mov    DWORD PTR [esi+0x70],0x0
.L4a5e63:
 test   ecx,ecx
.L4a5e65:
 je .L4a5e6d
.L4a5e67:
 mov    eax,DWORD PTR [ecx]
.L4a5e69:
 push   ecx
.L4a5e6a:
 call   DWORD PTR [eax+0x8]
.L4a5e6d:
 mov    DWORD PTR [esi+0x74],0x0
.L4a5e74:
 pop    esi
.L4a5e75:
 ret
.equ target_4a4d20,0x4a4d20
