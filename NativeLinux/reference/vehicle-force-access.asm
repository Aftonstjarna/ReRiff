
/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/Game.exe:     file format pei-i386


Disassembly of section .text:

0043b660 <.text+0x3a660>:
  43b660:	55                   	push   ebp
  43b661:	8b ec                	mov    ebp,esp
  43b663:	83 e4 f8             	and    esp,0xfffffff8
  43b666:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b669:	83 ec 08             	sub    esp,0x8
  43b66c:	f6 80 cc 00 00 00 03 	test   BYTE PTR [eax+0xcc],0x3
  43b673:	75 24                	jne    0x43b699
  43b675:	8b 90 d8 00 00 00    	mov    edx,DWORD PTR [eax+0xd8]
  43b67b:	83 fa 04             	cmp    edx,0x4
  43b67e:	74 0f                	je     0x43b68f
  43b680:	83 fa 05             	cmp    edx,0x5
  43b683:	74 0a                	je     0x43b68f
  43b685:	c7 80 d8 00 00 00 01 	mov    DWORD PTR [eax+0xd8],0x1
  43b68c:	00 00 00 
  43b68f:	c7 80 dc 00 00 00 00 	mov    DWORD PTR [eax+0xdc],0x0
  43b696:	00 00 00 
  43b699:	8b 45 0c             	mov    eax,DWORD PTR [ebp+0xc]
  43b69c:	f3 0f 10 00          	movss  xmm0,DWORD PTR [eax]
  43b6a0:	f3 0f 10 78 08       	movss  xmm7,DWORD PTR [eax+0x8]
  43b6a5:	f3 0f 11 44 24 04    	movss  DWORD PTR [esp+0x4],xmm0
  43b6ab:	f3 0f 10 40 04       	movss  xmm0,DWORD PTR [eax+0x4]
  43b6b0:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b6b3:	f3 0f 11 04 24       	movss  DWORD PTR [esp],xmm0
  43b6b8:	f3 0f 10 18          	movss  xmm3,DWORD PTR [eax]
  43b6bc:	f3 0f 10 68 04       	movss  xmm5,DWORD PTR [eax+0x4]
  43b6c1:	f3 0f 10 70 08       	movss  xmm6,DWORD PTR [eax+0x8]
  43b6c6:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b6c9:	f3 0f 10 80 54 01 00 	movss  xmm0,DWORD PTR [eax+0x154]
  43b6d0:	00 
  43b6d1:	f3 0f 10 88 58 01 00 	movss  xmm1,DWORD PTR [eax+0x158]
  43b6d8:	00 
  43b6d9:	f3 0f 10 90 5c 01 00 	movss  xmm2,DWORD PTR [eax+0x15c]
  43b6e0:	00 
  43b6e1:	f3 0f 59 c3          	mulss  xmm0,xmm3
  43b6e5:	f3 0f 59 cd          	mulss  xmm1,xmm5
  43b6e9:	f3 0f 58 80 94 01 00 	addss  xmm0,DWORD PTR [eax+0x194]
  43b6f0:	00 
  43b6f1:	f3 0f 59 d6          	mulss  xmm2,xmm6
  43b6f5:	f3 0f 58 88 98 01 00 	addss  xmm1,DWORD PTR [eax+0x198]
  43b6fc:	00 
  43b6fd:	f3 0f 58 90 9c 01 00 	addss  xmm2,DWORD PTR [eax+0x19c]
  43b704:	00 
  43b705:	f3 0f 11 80 94 01 00 	movss  DWORD PTR [eax+0x194],xmm0
  43b70c:	00 
  43b70d:	f3 0f 11 88 98 01 00 	movss  DWORD PTR [eax+0x198],xmm1
  43b714:	00 
  43b715:	f3 0f 11 90 9c 01 00 	movss  DWORD PTR [eax+0x19c],xmm2
  43b71c:	00 
  43b71d:	f3 0f 10 88 5c 01 00 	movss  xmm1,DWORD PTR [eax+0x15c]
  43b724:	00 
  43b725:	f3 0f 10 a0 58 01 00 	movss  xmm4,DWORD PTR [eax+0x158]
  43b72c:	00 
  43b72d:	f3 0f 10 90 54 01 00 	movss  xmm2,DWORD PTR [eax+0x154]
  43b734:	00 
  43b735:	f3 0f 59 d3          	mulss  xmm2,xmm3
  43b739:	f3 0f 59 ce          	mulss  xmm1,xmm6
  43b73d:	f3 0f 59 e5          	mulss  xmm4,xmm5
  43b741:	0f 28 d9             	movaps xmm3,xmm1
  43b744:	f3 0f 59 4c 24 04    	mulss  xmm1,DWORD PTR [esp+0x4]
  43b74a:	f3 0f 59 1c 24       	mulss  xmm3,DWORD PTR [esp]
  43b74f:	0f 28 c4             	movaps xmm0,xmm4
  43b752:	f3 0f 59 64 24 04    	mulss  xmm4,DWORD PTR [esp+0x4]
  43b758:	f3 0f 59 c7          	mulss  xmm0,xmm7
  43b75c:	f3 0f 5c d8          	subss  xmm3,xmm0
  43b760:	0f 28 c2             	movaps xmm0,xmm2
  43b763:	f3 0f 59 14 24       	mulss  xmm2,DWORD PTR [esp]
  43b768:	f3 0f 59 c7          	mulss  xmm0,xmm7
  43b76c:	f3 0f 59 98 18 02 00 	mulss  xmm3,DWORD PTR [eax+0x218]
  43b773:	00 
  43b774:	f3 0f 5c e2          	subss  xmm4,xmm2
  43b778:	f3 0f 10 90 20 02 00 	movss  xmm2,DWORD PTR [eax+0x220]
  43b77f:	00 
  43b780:	f3 0f 5c c1          	subss  xmm0,xmm1
  43b784:	f3 0f 10 88 1c 02 00 	movss  xmm1,DWORD PTR [eax+0x21c]
  43b78b:	00 
  43b78c:	f3 0f 59 d4          	mulss  xmm2,xmm4
  43b790:	f3 0f 59 c8          	mulss  xmm1,xmm0
  43b794:	f3 0f 10 80 a4 01 00 	movss  xmm0,DWORD PTR [eax+0x1a4]
  43b79b:	00 
  43b79c:	f3 0f 58 90 ac 01 00 	addss  xmm2,DWORD PTR [eax+0x1ac]
  43b7a3:	00 
  43b7a4:	f3 0f 58 88 a8 01 00 	addss  xmm1,DWORD PTR [eax+0x1a8]
  43b7ab:	00 
  43b7ac:	f3 0f 58 c3          	addss  xmm0,xmm3
  43b7b0:	f3 0f 11 90 ac 01 00 	movss  DWORD PTR [eax+0x1ac],xmm2
  43b7b7:	00 
  43b7b8:	f3 0f 11 88 a8 01 00 	movss  DWORD PTR [eax+0x1a8],xmm1
  43b7bf:	00 
  43b7c0:	f3 0f 11 80 a4 01 00 	movss  DWORD PTR [eax+0x1a4],xmm0
  43b7c7:	00 
  43b7c8:	8b e5                	mov    esp,ebp
  43b7ca:	5d                   	pop    ebp
  43b7cb:	c2 08 00             	ret    0x8

/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/Game.exe:     file format pei-i386


Disassembly of section .text:

0043b8b0 <.text+0x3a8b0>:
  43b8b0:	55                   	push   ebp
  43b8b1:	8b ec                	mov    ebp,esp
  43b8b3:	83 e4 f8             	and    esp,0xfffffff8
  43b8b6:	8b 45 0c             	mov    eax,DWORD PTR [ebp+0xc]
  43b8b9:	f3 0f 10 50 04       	movss  xmm2,DWORD PTR [eax+0x4]
  43b8be:	f3 0f 10 30          	movss  xmm6,DWORD PTR [eax]
  43b8c2:	f3 0f 10 68 08       	movss  xmm5,DWORD PTR [eax+0x8]
  43b8c7:	0f 28 c6             	movaps xmm0,xmm6
  43b8ca:	8b 41 78             	mov    eax,DWORD PTR [ecx+0x78]
  43b8cd:	f3 0f 59 80 00 01 00 	mulss  xmm0,DWORD PTR [eax+0x100]
  43b8d4:	00 
  43b8d5:	f3 0f 10 88 04 01 00 	movss  xmm1,DWORD PTR [eax+0x104]
  43b8dc:	00 
  43b8dd:	f3 0f 10 a0 14 01 00 	movss  xmm4,DWORD PTR [eax+0x114]
  43b8e4:	00 
  43b8e5:	f3 0f 10 98 24 01 00 	movss  xmm3,DWORD PTR [eax+0x124]
  43b8ec:	00 
  43b8ed:	f3 0f 59 ca          	mulss  xmm1,xmm2
  43b8f1:	f3 0f 59 e2          	mulss  xmm4,xmm2
  43b8f5:	f3 0f 58 c8          	addss  xmm1,xmm0
  43b8f9:	f3 0f 59 da          	mulss  xmm3,xmm2
  43b8fd:	f3 0f 10 80 08 01 00 	movss  xmm0,DWORD PTR [eax+0x108]
  43b904:	00 
  43b905:	f3 0f 59 c5          	mulss  xmm0,xmm5
  43b909:	f3 0f 10 90 18 02 00 	movss  xmm2,DWORD PTR [eax+0x218]
  43b910:	00 
  43b911:	f3 0f 58 c8          	addss  xmm1,xmm0
  43b915:	f3 0f 10 80 10 01 00 	movss  xmm0,DWORD PTR [eax+0x110]
  43b91c:	00 
  43b91d:	f3 0f 59 c6          	mulss  xmm0,xmm6
  43b921:	f3 0f 59 b0 20 01 00 	mulss  xmm6,DWORD PTR [eax+0x120]
  43b928:	00 
  43b929:	f3 0f 58 e0          	addss  xmm4,xmm0
  43b92d:	f3 0f 59 d1          	mulss  xmm2,xmm1
  43b931:	f3 0f 10 80 18 01 00 	movss  xmm0,DWORD PTR [eax+0x118]
  43b938:	00 
  43b939:	f3 0f 58 de          	addss  xmm3,xmm6
  43b93d:	f3 0f 10 88 1c 02 00 	movss  xmm1,DWORD PTR [eax+0x21c]
  43b944:	00 
  43b945:	f3 0f 59 c5          	mulss  xmm0,xmm5
  43b949:	f3 0f 58 e0          	addss  xmm4,xmm0
  43b94d:	f3 0f 10 80 28 01 00 	movss  xmm0,DWORD PTR [eax+0x128]
  43b954:	00 
  43b955:	f3 0f 59 c5          	mulss  xmm0,xmm5
  43b959:	f3 0f 58 d8          	addss  xmm3,xmm0
  43b95d:	f3 0f 59 cc          	mulss  xmm1,xmm4
  43b961:	f3 0f 10 80 20 02 00 	movss  xmm0,DWORD PTR [eax+0x220]
  43b968:	00 
  43b969:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b96c:	f3 0f 59 c3          	mulss  xmm0,xmm3
  43b970:	f3 0f 11 10          	movss  DWORD PTR [eax],xmm2
  43b974:	f3 0f 11 48 04       	movss  DWORD PTR [eax+0x4],xmm1
  43b979:	f3 0f 11 40 08       	movss  DWORD PTR [eax+0x8],xmm0
  43b97e:	8b e5                	mov    esp,ebp
  43b980:	5d                   	pop    ebp
  43b981:	c2 08 00             	ret    0x8
  43b984:	cc                   	int3
  43b985:	cc                   	int3
  43b986:	cc                   	int3
  43b987:	cc                   	int3
  43b988:	cc                   	int3
  43b989:	cc                   	int3
  43b98a:	cc                   	int3
  43b98b:	cc                   	int3
  43b98c:	cc                   	int3
  43b98d:	cc                   	int3
  43b98e:	cc                   	int3
  43b98f:	cc                   	int3
  43b990:	55                   	push   ebp
  43b991:	8b ec                	mov    ebp,esp
  43b993:	83 e4 f8             	and    esp,0xfffffff8
  43b996:	8b 49 78             	mov    ecx,DWORD PTR [ecx+0x78]
  43b999:	8b 45 0c             	mov    eax,DWORD PTR [ebp+0xc]
  43b99c:	f3 0f 10 99 54 01 00 	movss  xmm3,DWORD PTR [ecx+0x154]
  43b9a3:	00 
  43b9a4:	f3 0f 10 89 58 01 00 	movss  xmm1,DWORD PTR [ecx+0x158]
  43b9ab:	00 
  43b9ac:	f3 0f 10 81 5c 01 00 	movss  xmm0,DWORD PTR [ecx+0x15c]
  43b9b3:	00 
  43b9b4:	f3 0f 59 18          	mulss  xmm3,DWORD PTR [eax]
  43b9b8:	f3 0f 59 48 04       	mulss  xmm1,DWORD PTR [eax+0x4]
  43b9bd:	f3 0f 59 40 08       	mulss  xmm0,DWORD PTR [eax+0x8]
  43b9c2:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b9c5:	f3 0f 10 91 50 01 00 	movss  xmm2,DWORD PTR [ecx+0x150]
  43b9cc:	00 
  43b9cd:	f3 0f 59 da          	mulss  xmm3,xmm2
  43b9d1:	f3 0f 59 ca          	mulss  xmm1,xmm2
  43b9d5:	f3 0f 59 c2          	mulss  xmm0,xmm2
  43b9d9:	f3 0f 11 18          	movss  DWORD PTR [eax],xmm3
  43b9dd:	f3 0f 11 48 04       	movss  DWORD PTR [eax+0x4],xmm1
  43b9e2:	f3 0f 11 40 08       	movss  DWORD PTR [eax+0x8],xmm0
  43b9e7:	8b e5                	mov    esp,ebp
  43b9e9:	5d                   	pop    ebp
  43b9ea:	c2 08 00             	ret    0x8
  43b9ed:	cc                   	int3
  43b9ee:	cc                   	int3
  43b9ef:	cc                   	int3
  43b9f0:	55                   	push   ebp
  43b9f1:	8b ec                	mov    ebp,esp
  43b9f3:	8b 51 78             	mov    edx,DWORD PTR [ecx+0x78]
  43b9f6:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43b9f9:	8b 8a a4 01 00 00    	mov    ecx,DWORD PTR [edx+0x1a4]
  43b9ff:	89 08                	mov    DWORD PTR [eax],ecx
  43ba01:	8b 8a a8 01 00 00    	mov    ecx,DWORD PTR [edx+0x1a8]
  43ba07:	89 48 04             	mov    DWORD PTR [eax+0x4],ecx
  43ba0a:	8b 8a ac 01 00 00    	mov    ecx,DWORD PTR [edx+0x1ac]
  43ba10:	89 48 08             	mov    DWORD PTR [eax+0x8],ecx
  43ba13:	5d                   	pop    ebp
  43ba14:	c2 04 00             	ret    0x4
  43ba17:	cc                   	int3
  43ba18:	cc                   	int3
  43ba19:	cc                   	int3
  43ba1a:	cc                   	int3
  43ba1b:	cc                   	int3
  43ba1c:	cc                   	int3
  43ba1d:	cc                   	int3
  43ba1e:	cc                   	int3
  43ba1f:	cc                   	int3
  43ba20:	55                   	push   ebp
  43ba21:	8b ec                	mov    ebp,esp
  43ba23:	8b 51 78             	mov    edx,DWORD PTR [ecx+0x78]
  43ba26:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43ba29:	8b 8a 94 01 00 00    	mov    ecx,DWORD PTR [edx+0x194]
  43ba2f:	89 08                	mov    DWORD PTR [eax],ecx
  43ba31:	8b 8a 98 01 00 00    	mov    ecx,DWORD PTR [edx+0x198]
  43ba37:	89 48 04             	mov    DWORD PTR [eax+0x4],ecx
  43ba3a:	8b 8a 9c 01 00 00    	mov    ecx,DWORD PTR [edx+0x19c]
  43ba40:	89 48 08             	mov    DWORD PTR [eax+0x8],ecx
  43ba43:	5d                   	pop    ebp
  43ba44:	c2 04 00             	ret    0x4

/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/Game.exe:     file format pei-i386


Disassembly of section .text:

0043bb50 <.text+0x3ab50>:
  43bb50:	55                   	push   ebp
  43bb51:	8b ec                	mov    ebp,esp
  43bb53:	8b 49 78             	mov    ecx,DWORD PTR [ecx+0x78]
  43bb56:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
  43bb59:	f3 0f 10 89 18 02 00 	movss  xmm1,DWORD PTR [ecx+0x218]
  43bb60:	00 
  43bb61:	f3 0f 59 08          	mulss  xmm1,DWORD PTR [eax]
  43bb65:	f3 0f 10 81 a4 01 00 	movss  xmm0,DWORD PTR [ecx+0x1a4]
  43bb6c:	00 
  43bb6d:	f3 0f 10 91 1c 02 00 	movss  xmm2,DWORD PTR [ecx+0x21c]
  43bb74:	00 
  43bb75:	f3 0f 10 99 20 02 00 	movss  xmm3,DWORD PTR [ecx+0x220]
  43bb7c:	00 
  43bb7d:	f3 0f 58 c1          	addss  xmm0,xmm1
  43bb81:	f3 0f 59 50 04       	mulss  xmm2,DWORD PTR [eax+0x4]
  43bb86:	f3 0f 59 58 08       	mulss  xmm3,DWORD PTR [eax+0x8]
  43bb8b:	f3 0f 11 81 a4 01 00 	movss  DWORD PTR [ecx+0x1a4],xmm0
  43bb92:	00 
  43bb93:	f3 0f 10 81 a8 01 00 	movss  xmm0,DWORD PTR [ecx+0x1a8]
  43bb9a:	00 
  43bb9b:	f3 0f 58 c2          	addss  xmm0,xmm2
  43bb9f:	f3 0f 11 81 a8 01 00 	movss  DWORD PTR [ecx+0x1a8],xmm0
  43bba6:	00 
  43bba7:	f3 0f 10 81 ac 01 00 	movss  xmm0,DWORD PTR [ecx+0x1ac]
  43bbae:	00 
  43bbaf:	f3 0f 58 c3          	addss  xmm0,xmm3
  43bbb3:	f3 0f 11 81 ac 01 00 	movss  DWORD PTR [ecx+0x1ac],xmm0
  43bbba:	00 
  43bbbb:	5d                   	pop    ebp
  43bbbc:	c2 04 00             	ret    0x4

/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/Game.exe:     file format pei-i386

Contents of section .rdata:
 9f982c 009a4300 10af4300 10b04300 00bd4300  ..C...C...C...C.
 9f983c 80b84300 10b24300 50b44300 30b64300  ..C...C.P.C.0.C.
 9f984c 60b64300 50bb4300 d0b74300 50ba4300  `.C.P.C...C.P.C.
 9f985c 00ae4300 80be4300 30bc4300           ..C...C.0.C.    
