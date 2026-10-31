
/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/Game.exe:     file format pei-i386


Disassembly of section .text:

0043b160 <.text+0x3a160>:
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
  43b193:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b196:	83 ec 10             	sub    esp,0x10
  43b199:	f6 80 cc 00 00 00 03 	test   BYTE PTR [eax+0xcc],0x3
  43b1a0:	75 24                	jne    0x43b1c6
  43b1a2:	8b 90 d8 00 00 00    	mov    edx,DWORD PTR [eax+0xd8]
  43b1a8:	83 fa 04             	cmp    edx,0x4
  43b1ab:	74 0f                	je     0x43b1bc
  43b1ad:	83 fa 05             	cmp    edx,0x5
  43b1b0:	74 0a                	je     0x43b1bc
  43b1b2:	c7 80 d8 00 00 00 01 	mov    DWORD PTR [eax+0xd8],0x1
  43b1b9:	00 00 00 
  43b1bc:	c7 80 dc 00 00 00 00 	mov    DWORD PTR [eax+0xdc],0x0
  43b1c3:	00 00 00 
  43b1c6:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b1c9:	c7 45 fc 00 00 00 00 	mov    DWORD PTR [ebp-0x4],0x0
  43b1d0:	f3 0f 10 00          	movss  xmm0,DWORD PTR [eax]
  43b1d4:	f3 0f 11 45 f0       	movss  DWORD PTR [ebp-0x10],xmm0
  43b1d9:	f3 0f 10 40 04       	movss  xmm0,DWORD PTR [eax+0x4]
  43b1de:	f3 0f 11 45 f4       	movss  DWORD PTR [ebp-0xc],xmm0
  43b1e3:	f3 0f 10 40 08       	movss  xmm0,DWORD PTR [eax+0x8]
  43b1e8:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b1eb:	f3 0f 11 45 f8       	movss  DWORD PTR [ebp-0x8],xmm0
  43b1f0:	f3 0f 6f 45 f0       	movdqu xmm0,XMMWORD PTR [ebp-0x10]
  43b1f5:	f3 0f 7f 80 40 01 00 	movdqu XMMWORD PTR [eax+0x140],xmm0
  43b1fc:	00 
  43b1fd:	8b e5                	mov    esp,ebp
  43b1ff:	5d                   	pop    ebp
  43b200:	c2 04 00             	ret    0x4
