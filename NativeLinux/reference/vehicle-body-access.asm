
/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/Game.exe:     file format pei-i386


Disassembly of section .text:

0043b0b0 <.text+0x3a0b0>:
  43b0b0:	55                   	push   ebp
  43b0b1:	8b ec                	mov    ebp,esp
  43b0b3:	8b 51 78             	mov    edx,DWORD PTR [ecx+0x78]
  43b0b6:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b0b9:	8b 8a 30 01 00 00    	mov    ecx,DWORD PTR [edx+0x130]
  43b0bf:	89 08                	mov    DWORD PTR [eax],ecx
  43b0c1:	8b 8a 34 01 00 00    	mov    ecx,DWORD PTR [edx+0x134]
  43b0c7:	89 48 04             	mov    DWORD PTR [eax+0x4],ecx
  43b0ca:	8b 8a 38 01 00 00    	mov    ecx,DWORD PTR [edx+0x138]
  43b0d0:	89 48 08             	mov    DWORD PTR [eax+0x8],ecx
  43b0d3:	5d                   	pop    ebp
  43b0d4:	c2 04 00             	ret    0x4
  43b0d7:	cc                   	int3
  43b0d8:	cc                   	int3
  43b0d9:	cc                   	int3
  43b0da:	cc                   	int3
  43b0db:	cc                   	int3
  43b0dc:	cc                   	int3
  43b0dd:	cc                   	int3
  43b0de:	cc                   	int3
  43b0df:	cc                   	int3
  43b0e0:	55                   	push   ebp
  43b0e1:	8b ec                	mov    ebp,esp
  43b0e3:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b0e6:	83 ec 10             	sub    esp,0x10
  43b0e9:	f6 80 cc 00 00 00 03 	test   BYTE PTR [eax+0xcc],0x3
  43b0f0:	75 24                	jne    0x43b116
  43b0f2:	8b 90 d8 00 00 00    	mov    edx,DWORD PTR [eax+0xd8]
  43b0f8:	83 fa 04             	cmp    edx,0x4
  43b0fb:	74 0f                	je     0x43b10c
  43b0fd:	83 fa 05             	cmp    edx,0x5
  43b100:	74 0a                	je     0x43b10c
  43b102:	c7 80 d8 00 00 00 01 	mov    DWORD PTR [eax+0xd8],0x1
  43b109:	00 00 00 
  43b10c:	c7 80 dc 00 00 00 00 	mov    DWORD PTR [eax+0xdc],0x0
  43b113:	00 00 00 
  43b116:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b119:	c7 45 fc 00 00 00 00 	mov    DWORD PTR [ebp-0x4],0x0
  43b120:	f3 0f 10 00          	movss  xmm0,DWORD PTR [eax]
  43b124:	f3 0f 11 45 f0       	movss  DWORD PTR [ebp-0x10],xmm0
  43b129:	f3 0f 10 40 04       	movss  xmm0,DWORD PTR [eax+0x4]
  43b12e:	f3 0f 11 45 f4       	movss  DWORD PTR [ebp-0xc],xmm0
  43b133:	f3 0f 10 40 08       	movss  xmm0,DWORD PTR [eax+0x8]
  43b138:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b13b:	f3 0f 11 45 f8       	movss  DWORD PTR [ebp-0x8],xmm0
  43b140:	f3 0f 6f 45 f0       	movdqu xmm0,XMMWORD PTR [ebp-0x10]
  43b145:	f3 0f 7f 80 30 01 00 	movdqu XMMWORD PTR [eax+0x130],xmm0
  43b14c:	00 
  43b14d:	8b e5                	mov    esp,ebp
  43b14f:	5d                   	pop    ebp
  43b150:	c2 04 00             	ret    0x4
  43b153:	cc                   	int3
  43b154:	cc                   	int3
  43b155:	cc                   	int3
  43b156:	cc                   	int3
  43b157:	cc                   	int3
  43b158:	cc                   	int3
  43b159:	cc                   	int3
  43b15a:	cc                   	int3
  43b15b:	cc                   	int3
  43b15c:	cc                   	int3
  43b15d:	cc                   	int3
  43b15e:	cc                   	int3
  43b15f:	cc                   	int3
  43b160:	55                   	push   ebp
  43b161:	8b ec                	mov    ebp,esp
  43b163:	8b 51 78             	mov    edx,DWORD PTR [ecx+0x78]
  43b166:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b169:	8b 8a 40 01 00 00    	mov    ecx,DWORD PTR [edx+0x140]
  43b16f:	89 08                	mov    DWORD PTR [eax],ecx
  43b171:	8b 8a 44 01 00 00    	mov    ecx,DWORD PTR [edx+0x144]
  43b177:	89 48 04             	mov    DWORD PTR [eax+0x4],ecx
  43b17a:	8b 8a 48 01 00 00    	mov    ecx,DWORD PTR [edx+0x148]
  43b180:	89 48 08             	mov    DWORD PTR [eax+0x8],ecx
  43b183:	5d                   	pop    ebp
  43b184:	c2 04 00             	ret    0x4
  43b187:	cc                   	int3
  43b188:	cc                   	int3
  43b189:	cc                   	int3
  43b18a:	cc                   	int3
  43b18b:	cc                   	int3
  43b18c:	cc                   	int3
  43b18d:	cc                   	int3
  43b18e:	cc                   	int3
  43b18f:	cc                   	int3
  43b190:	55                   	push   ebp
  43b191:	8b ec                	mov    ebp,esp
  43b193:	8b                   	.byte 0x8b
  43b194:	41                   	inc    ecx
