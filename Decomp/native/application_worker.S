# Mechanically drafted; reviewed against Ghidra and original native disassembly.
# Original initialization, command-line switches, update loop and shutdown.
# External callees, vtables, strings and exception-handler data remain original
# address dependencies; this body alone is not a runnable rebuilt application.
# Original VA range 0x5282e0..0x52892b
.intel_syntax noprefix
.code32
.text
.global original_application_worker
original_application_worker:
.L5282e0:
 push   ebp
.L5282e1:
 {load} mov    ebp,esp
.L5282e3:
 and    esp,0xfffffff8
.L5282e6:
 push   0xffffffff
.L5282e8:
 push   0x9052d4
.L5282ed:
 mov    eax,fs:0x0
.L5282f3:
 push   eax
.L5282f4:
 sub    esp,0x30
.L5282f7:
 mov    eax,[0xbaabc0]
.L5282fc:
 {load} xor    eax,esp
.L5282fe:
 mov    DWORD PTR [esp+0x28],eax
.L528302:
 push   ebx
.L528303:
 push   esi
.L528304:
 push   edi
.L528305:
 mov    eax,[0xbaabc0]
.L52830a:
 {load} xor    eax,esp
.L52830c:
 push   eax
.L52830d:
 lea    eax,[esp+0x40]
.L528311:
 mov    fs:0x0,eax
.L528317:
 {load} mov    ebx,ecx
.L528319:
 call   DWORD PTR [0x92d0ec]
.L52831f:
 push   0x0
.L528321:
 push   0x0
.L528323:
 mov    [0xbfcc38],eax
.L528328:
 call   DWORD PTR [0x92d698]
.L52832e:
 cmp    DWORD PTR [0xbfcc40],0x0
.L528335:
 mov    edi,DWORD PTR [0x92d4b4]
.L52833b:
 jne .L52836f
.L52833d:
 push   0x2e5c
.L528342:
 call   edi
.L528344:
 add    esp,0x4
.L528347:
 mov    DWORD PTR [esp+0x10],eax
.L52834b:
 mov    DWORD PTR [esp+0x48],0x0
.L528353:
 test   eax,eax
.L528355:
 je .L528360
.L528357:
 {load} mov    ecx,eax
.L528359:
 call target_4914e0
.L52835e:
 jmp .L528362
.L528360:
 {load} xor    eax,eax
.L528362:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L52836a:
 mov    [0xbfcc40],eax
.L52836f:
 call target_51b910
.L528374:
 cmp    DWORD PTR [0xbfcd40],0x0
.L52837b:
 jne .L5283af
.L52837d:
 push   0x770
.L528382:
 call   edi
.L528384:
 add    esp,0x4
.L528387:
 mov    DWORD PTR [esp+0x10],eax
.L52838b:
 mov    DWORD PTR [esp+0x48],0x1
.L528393:
 test   eax,eax
.L528395:
 je .L5283a0
.L528397:
 {load} mov    ecx,eax
.L528399:
 call target_56fe10
.L52839e:
 jmp .L5283a2
.L5283a0:
 {load} xor    eax,eax
.L5283a2:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L5283aa:
 mov    [0xbfcd40],eax
.L5283af:
 push   0x48
.L5283b1:
 call   edi
.L5283b3:
 add    esp,0x4
.L5283b6:
 mov    DWORD PTR [esp+0x10],eax
.L5283ba:
 mov    DWORD PTR [esp+0x48],0x2
.L5283c2:
 test   eax,eax
.L5283c4:
 je .L5283cd
.L5283c6:
 {load} mov    ecx,eax
.L5283c8:
 call target_5747c0
.L5283cd:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L5283d5:
 call target_56faa0
.L5283da:
 cmp    DWORD PTR [0xbfcd3c],0x0
.L5283e1:
 jne .L528426
.L5283e3:
 push   0x1064
.L5283e8:
 call   edi
.L5283ea:
 {load} mov    esi,eax
.L5283ec:
 add    esp,0x4
.L5283ef:
 mov    DWORD PTR [esp+0x10],esi
.L5283f3:
 mov    DWORD PTR [esp+0x48],0x3
.L5283fb:
 test   esi,esi
.L5283fd:
 je .L528416
.L5283ff:
 {load} mov    ecx,esi
.L528401:
 call target_48bd40
.L528406:
 lea    ecx,[esi+0x14]
.L528409:
 mov    DWORD PTR [esi],0xa0103c
.L52840f:
 call target_5a1850
.L528414:
 jmp .L528418
.L528416:
 {load} xor    esi,esi
.L528418:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L528420:
 mov    DWORD PTR [0xbfcd3c],esi
.L528426:
 push   0x4
.L528428:
 call   edi
.L52842a:
 add    esp,0x4
.L52842d:
 test   eax,eax
.L52842f:
 je .L52843e
.L528431:
 mov    [0xc0d578],eax
.L528436:
 mov    DWORD PTR [eax],0xa00fe0
.L52843c:
 jmp .L528440
.L52843e:
 {load} xor    eax,eax
.L528440:
 cmp    DWORD PTR [0xbfcb34],0x0
.L528447:
 mov    [0xbfcd34],eax
.L52844c:
 jne .L52846c
.L52844e:
 push   0x86c
.L528453:
 call   edi
.L528455:
 add    esp,0x4
.L528458:
 test   eax,eax
.L52845a:
 je .L528465
.L52845c:
 {load} mov    ecx,eax
.L52845e:
 call target_424a30
.L528463:
 jmp .L528467
.L528465:
 {load} xor    eax,eax
.L528467:
 mov    [0xbfcb34],eax
.L52846c:
 cmp    DWORD PTR [0xbfbaac],0x0
.L528473:
 jne .L528493
.L528475:
 push   0x234
.L52847a:
 call   edi
.L52847c:
 add    esp,0x4
.L52847f:
 test   eax,eax
.L528481:
 je .L52848c
.L528483:
 {load} mov    ecx,eax
.L528485:
 call target_4242e0
.L52848a:
 jmp .L52848e
.L52848c:
 {load} xor    eax,eax
.L52848e:
 mov    [0xbfbaac],eax
.L528493:
 cmp    DWORD PTR [0xbfbaa4],0x0
.L52849a:
 jne .L5284cb
.L52849c:
 push   0x74
.L52849e:
 call   edi
.L5284a0:
 add    esp,0x4
.L5284a3:
 mov    DWORD PTR [esp+0x10],eax
.L5284a7:
 mov    DWORD PTR [esp+0x48],0x4
.L5284af:
 test   eax,eax
.L5284b1:
 je .L5284bc
.L5284b3:
 {load} mov    ecx,eax
.L5284b5:
 call target_41fc30
.L5284ba:
 jmp .L5284be
.L5284bc:
 {load} xor    eax,eax
.L5284be:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L5284c6:
 mov    [0xbfbaa4],eax
.L5284cb:
 cmp    DWORD PTR [0xbfcc6c],0x0
.L5284d2:
 jne .L528513
.L5284d4:
 push   0x48
.L5284d6:
 call   edi
.L5284d8:
 {load} mov    esi,eax
.L5284da:
 add    esp,0x4
.L5284dd:
 mov    DWORD PTR [esp+0x10],esi
.L5284e1:
 mov    DWORD PTR [esp+0x48],0x5
.L5284e9:
 test   esi,esi
.L5284eb:
 je .L528503
.L5284ed:
 push   0x507ee0
.L5284f2:
 push   0x507e50
.L5284f7:
 push   0x3
.L5284f9:
 push   0x18
.L5284fb:
 push   esi
.L5284fc:
 call target_8f94bf
.L528501:
 jmp .L528505
.L528503:
 {load} xor    esi,esi
.L528505:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L52850d:
 mov    DWORD PTR [0xbfcc6c],esi
.L528513:
 mov    ecx,DWORD PTR [0xbfcc2c]
.L528519:
 call target_48cf60
.L52851e:
 mov    esi,DWORD PTR [0xbfcc40]
.L528524:
 call target_491fd0
.L528529:
 mov    eax,DWORD PTR [esi+0xac]
.L52852f:
 lea    ecx,[esi+0xac]
.L528535:
 push   0x0
.L528537:
 call   DWORD PTR [eax+0x4]
.L52853a:
 mov    ecx,DWORD PTR [0xbfcc74]
.L528540:
 call target_51db50
.L528545:
 mov    ecx,DWORD PTR [0xbfcd2c]
.L52854b:
 call target_55e740
.L528550:
 test   al,al
.L528552:
 jne .L5285d0
.L528554:
 mov    esi,DWORD PTR [0xbfcc40]
.L52855a:
 push   DWORD PTR [esi+0xc0]
.L528560:
 mov    BYTE PTR [esi+0xd0],0x1
.L528567:
 call   DWORD PTR [0x92d120]
.L52856d:
 push   0xffffffff
.L52856f:
 push   DWORD PTR [esi+0xb4]
.L528575:
 call   DWORD PTR [0x92d124]
.L52857b:
 {load} mov    ecx,esi
.L52857d:
 call target_498e00
.L528582:
 mov    eax,DWORD PTR [esi]
.L528584:
 {load} mov    ecx,esi
.L528586:
 call   DWORD PTR [eax+0x74]
.L528589:
 mov    ecx,DWORD PTR [0xbfcc44]
.L52858f:
 lea    eax,[esp+0x14]
.L528593:
 push   eax
.L528594:
 mov    DWORD PTR [esp+0x18],0x6
.L52859c:
 call target_4a1870
.L5285a1:
 mov    ecx,DWORD PTR [0xbfcd2c]
.L5285a7:
 lea    ecx,[ecx+0x30]
.L5285aa:
 call target_44f5b0
.L5285af:
 or     eax,0xffffffff
.L5285b2:
 mov    ecx,DWORD PTR [esp+0x40]
.L5285b6:
 mov    DWORD PTR fs:0x0,ecx
.L5285bd:
 pop    ecx
.L5285be:
 pop    edi
.L5285bf:
 pop    esi
.L5285c0:
 pop    ebx
.L5285c1:
 mov    ecx,DWORD PTR [esp+0x28]
.L5285c5:
 {load} xor    ecx,esp
.L5285c7:
 call target_8f8958
.L5285cc:
 {load} mov    esp,ebp
.L5285ce:
 pop    ebp
.L5285cf:
 ret
.L5285d0:
 push   0x9fe678
.L5285d5:
 push   DWORD PTR [ebx+0x10]
.L5285d8:
 call   DWORD PTR [0x92d3dc]
.L5285de:
 movzx  ecx,BYTE PTR [0xbfcbba]
.L5285e5:
 add    esp,0x8
.L5285e8:
 test   eax,eax
.L5285ea:
 mov    edx,0x1
.L5285ef:
 cmovne ecx,edx
.L5285f2:
 mov    BYTE PTR [0xbfcbba],cl
.L5285f8:
 mov    ecx,DWORD PTR [0xbfb610]
.L5285fe:
 call target_4150f0
.L528603:
 test   al,al
.L528605:
 je .L5285af
.L528607:
 mov    esi,DWORD PTR [0xbfcd30]
.L52860d:
 call   DWORD PTR [0xbc9158]
.L528613:
 mov    DWORD PTR [esi+0x10],eax
.L528616:
 test   eax,eax
.L528618:
 je .L5285af
.L52861a:
 cmp    BYTE PTR [0xbfcbba],0x0
.L528621:
 je .L528632
.L528623:
 call   DWORD PTR [0xbc9134]
.L528629:
 push   0x1
.L52862b:
 {load} mov    ecx,eax
.L52862d:
 mov    edx,DWORD PTR [eax]
.L52862f:
 call   DWORD PTR [edx+0x54]
.L528632:
 call target_510ee0
.L528637:
 mov    ecx,DWORD PTR [0xbfcd40]
.L52863d:
 call target_571480
.L528642:
 push   0x4
.L528644:
 call   edi
.L528646:
 add    esp,0x4
.L528649:
 test   eax,eax
.L52864b:
 je .L528655
.L52864d:
 mov    DWORD PTR [eax],0xa0123c
.L528653:
 jmp .L528657
.L528655:
 {load} xor    eax,eax
.L528657:
 mov    esi,DWORD PTR [0x92d3dc]
.L52865d:
 push   0x9fe684
.L528662:
 push   DWORD PTR [ebx+0x10]
.L528665:
 mov    [0xbfcd44],eax
.L52866a:
 call   esi
.L52866c:
 add    esp,0x8
.L52866f:
 test   eax,eax
.L528671:
 jne .L528684
.L528673:
 push   0x9f9b70
.L528678:
 push   DWORD PTR [ebx+0x10]
.L52867b:
 call   esi
.L52867d:
 add    esp,0x8
.L528680:
 test   eax,eax
.L528682:
 je .L52868c
.L528684:
 mov    ecx,DWORD PTR [ebx+0x10]
.L528687:
 call target_5280b0
.L52868c:
 push   0x9fe694
.L528691:
 push   DWORD PTR [ebx+0x10]
.L528694:
 call   esi
.L528696:
 movzx  ecx,BYTE PTR [0xbfcbb8]
.L52869d:
 add    esp,0x8
.L5286a0:
 test   eax,eax
.L5286a2:
 mov    eax,0x1
.L5286a7:
 cmovne ecx,eax
.L5286aa:
 cmp    DWORD PTR [0xbfcbb0],0x0
.L5286b1:
 mov    BYTE PTR [0xbfcbb8],cl
.L5286b7:
 jne .L5286c5
.L5286b9:
 call target_6d86bb
.L5286be:
 {load} mov    ecx,eax
.L5286c0:
 mov    edx,DWORD PTR [eax]
.L5286c2:
 call   DWORD PTR [edx+0x4]
.L5286c5:
 push   0x9fe6a0
.L5286ca:
 push   DWORD PTR [ebx+0x10]
.L5286cd:
 call   esi
.L5286cf:
 add    esp,0x8
.L5286d2:
 test   eax,eax
.L5286d4:
 jne .L5286e7
.L5286d6:
 push   0x9fe6ac
.L5286db:
 push   DWORD PTR [ebx+0x10]
.L5286de:
 call   esi
.L5286e0:
 add    esp,0x8
.L5286e3:
 test   eax,eax
.L5286e5:
 je .L5286f3
.L5286e7:
 mov    eax,[0xbfcd40]
.L5286ec:
 mov    BYTE PTR [eax+0x3b0],0x1
.L5286f3:
 push   0x2c
.L5286f5:
 call   edi
.L5286f7:
 {load} mov    esi,eax
.L5286f9:
 add    esp,0x4
.L5286fc:
 mov    DWORD PTR [esp+0x10],esi
.L528700:
 mov    DWORD PTR [esp+0x48],0x6
.L528708:
 test   esi,esi
.L52870a:
 je .L52871d
.L52870c:
 call target_6d86bb
.L528711:
 push   eax
.L528712:
 {load} mov    ecx,esi
.L528714:
 call target_528120
.L528719:
 {load} mov    esi,eax
.L52871b:
 jmp .L52871f
.L52871d:
 {load} xor    esi,esi
.L52871f:
 mov    DWORD PTR [esp+0x48],0xffffffff
.L528727:
 call target_6d86bb
.L52872c:
 mov    ecx,DWORD PTR [0xbfcc40]
.L528732:
 mov    edi,0x1
.L528737:
 mov    DWORD PTR [eax+0x4],esi
.L52873a:
 mov    eax,DWORD PTR [ecx]
.L52873c:
 mov    eax,DWORD PTR [eax+0x14]
.L52873f:
 call   eax
.L528741:
 mov    eax,[0xbfcbbc]
.L528746:
 fstp   st(0)
.L528748:
 cmp    BYTE PTR [eax+0x2d],0x0
.L52874c:
 jne .L5288be
.L528752:
 mov    ecx,DWORD PTR [0xbfcc40]
.L528758:
 mov    eax,DWORD PTR [ecx]
.L52875a:
 mov    eax,DWORD PTR [eax+0x14]
.L52875d:
 call   eax
.L52875f:
 mov    eax,[0xbfcc28]
.L528764:
 mov    esi,DWORD PTR [0xbfcbbc]
.L52876a:
 fstp   DWORD PTR [esp+0x10]
.L52876e:
 push   DWORD PTR [eax+0x4]
.L528771:
 call   DWORD PTR [0x92d554]
.L528777:
 mov    ebx,DWORD PTR [0xbfcc28]
.L52877d:
 test   eax,eax
.L52877f:
 setne  al
.L528782:
 mov    BYTE PTR [esi+0x2f],al
.L528785:
 call   DWORD PTR [0x92d558]
.L52878b:
 cmp    eax,DWORD PTR [ebx+0x4]
.L52878e:
 je .L528799
.L528790:
 cmp    eax,DWORD PTR [ebx+0x8]
.L528793:
 je .L528799
.L528795:
 {load} xor    eax,eax
.L528797:
 jmp .L52879e
.L528799:
 mov    eax,0x1
.L52879e:
 cmp    BYTE PTR [esi+0x2f],0x0
.L5287a2:
 mov    BYTE PTR [esi+0x2e],al
.L5287a5:
 je .L5287ab
.L5287a7:
 mov    BYTE PTR [esi+0x2e],0x0
.L5287ab:
 mov    ecx,DWORD PTR [0xbfcc2c]
.L5287b1:
 movd   xmm1,edi
.L5287b5:
 cvtdq2ps xmm1,xmm1
.L5287b8:
 mulss  xmm1,DWORD PTR [esp+0x10]
.L5287be:
 call target_48d090
.L5287c3:
 mov    ecx,DWORD PTR [0xbfcc28]
.L5287c9:
 mov    al,BYTE PTR [ecx+0x29]
.L5287cc:
 mov    BYTE PTR [ecx+0x29],0x0
.L5287d0:
 test   al,al
.L5287d2:
 je .L5287f7
.L5287d4:
 call target_6d86bb
.L5287d9:
 {load} mov    ecx,eax
.L5287db:
 mov    edx,DWORD PTR [eax]
.L5287dd:
 mov    eax,DWORD PTR [edx+0x10]
.L5287e0:
 call   eax
.L5287e2:
 test   al,al
.L5287e4:
 jne .L5287f7
.L5287e6:
 mov    ecx,DWORD PTR [0xbfcbbc]
.L5287ec:
 mov    eax,DWORD PTR [ecx]
.L5287ee:
 call   DWORD PTR [eax+0x40]
.L5287f1:
 {load} mov    ecx,eax
.L5287f3:
 mov    edx,DWORD PTR [eax]
.L5287f5:
 call   DWORD PTR [edx]
.L5287f7:
 mov    ecx,DWORD PTR [0xbfcbb0]
.L5287fd:
 test   ecx,ecx
.L5287ff:
 je .L52880f
.L528801:
 movss  xmm2,DWORD PTR [esp+0x10]
.L528807:
 push   edi
.L528808:
 call target_43f800
.L52880d:
 jmp .L528828
.L52880f:
 call target_6d86bb
.L528814:
 movss  xmm0,DWORD PTR [esp+0x10]
.L52881a:
 push   ecx
.L52881b:
 movss  DWORD PTR [esp],xmm0
.L528820:
 {load} mov    ecx,eax
.L528822:
 mov    edx,DWORD PTR [eax]
.L528824:
 push   edi
.L528825:
 call   DWORD PTR [edx+0x8]
.L528828:
 movss  xmm0,DWORD PTR [esp+0x10]
.L52882e:
 push   ecx
.L52882f:
 mov    ecx,DWORD PTR [0xbfbaac]
.L528835:
 movss  DWORD PTR [esp],xmm0
.L52883a:
 push   edi
.L52883b:
 call target_424380
.L528840:
 movss  xmm2,DWORD PTR [esp+0x10]
.L528846:
 mov    ecx,DWORD PTR [0xbfcd40]
.L52884c:
 push   edi
.L52884d:
 call target_571d10
.L528852:
 mov    ecx,DWORD PTR [0xbfcc40]
.L528858:
 call target_492860
.L52885d:
 mov    ecx,DWORD PTR [0xbfcd2c]
.L528863:
 {load} mov    edi,eax
.L528865:
 call target_561170
.L52886a:
 mov    ecx,DWORD PTR [0xc0055c]
.L528870:
 call target_59efd0
.L528875:
 call   DWORD PTR [0x92d508]
.L52887b:
 mov    esi,DWORD PTR [0xbfb610]
.L528881:
 cmp    BYTE PTR [esi+0x1e2],0x0
.L528888:
 je .L5288a2
.L52888a:
 mov    ecx,DWORD PTR [esi+0xd4]
.L528890:
 mov    eax,DWORD PTR [ecx]
.L528892:
 mov    eax,DWORD PTR [eax+0x28]
.L528895:
 call   eax
.L528897:
 test   al,al
.L528899:
 sete   al
.L52889c:
 mov    BYTE PTR [esi+0x1e2],al
.L5288a2:
 call   DWORD PTR [0xbc9150]
.L5288a8:
 {load} xor    ecx,ecx
.L5288aa:
 call target_528930
.L5288af:
 mov    eax,[0xbfcbbc]
.L5288b4:
 cmp    BYTE PTR [eax+0x2d],0x0
.L5288b8:
 je .L528752
.L5288be:
 call target_6d86bb
.L5288c3:
 mov    esi,DWORD PTR [eax+0x4]
.L5288c6:
 {load} mov    ecx,esi
.L5288c8:
 mov    eax,DWORD PTR [esi]
.L5288ca:
 mov    BYTE PTR [esi+0x28],0x1
.L5288ce:
 call   DWORD PTR [eax+0x8]
.L5288d1:
 push   DWORD PTR [esi+0x18]
.L5288d4:
 call   DWORD PTR [0x92d120]
.L5288da:
 mov    ecx,DWORD PTR [0xc00ab0]
.L5288e0:
 test   ecx,ecx
.L5288e2:
 je .L5288fd
.L5288e4:
 dec    DWORD PTR [ecx+0x4]
.L5288e7:
 cmp    DWORD PTR [ecx+0x4],0x0
.L5288eb:
 jne .L5288f3
.L5288ed:
 mov    eax,DWORD PTR [ecx]
.L5288ef:
 push   0x1
.L5288f1:
 call   DWORD PTR [eax]
.L5288f3:
 mov    DWORD PTR [0xc00ab0],0x0
.L5288fd:
 mov    ecx,DWORD PTR [0xbfcd3c]
.L528903:
 mov    eax,DWORD PTR [ecx]
.L528905:
 call   DWORD PTR [eax+0x14]
.L528908:
 mov    ecx,DWORD PTR [0xbfcc40]
.L52890e:
 call target_491f50
.L528913:
 call   DWORD PTR [0xbc9148]
.L528919:
 mov    ecx,DWORD PTR [0xbfcd2c]
.L52891f:
 call target_55f3b0
.L528924:
 {load} xor    eax,eax
.L528926:
 jmp .L5285b2
.equ target_4150f0,0x4150f0
.equ target_41fc30,0x41fc30
.equ target_4242e0,0x4242e0
.equ target_424380,0x424380
.equ target_424a30,0x424a30
.equ target_43f800,0x43f800
.equ target_44f5b0,0x44f5b0
.equ target_48bd40,0x48bd40
.equ target_48cf60,0x48cf60
.equ target_48d090,0x48d090
.equ target_4914e0,0x4914e0
.equ target_491f50,0x491f50
.equ target_491fd0,0x491fd0
.equ target_492860,0x492860
.equ target_498e00,0x498e00
.equ target_4a1870,0x4a1870
.equ target_510ee0,0x510ee0
.equ target_51b910,0x51b910
.equ target_51db50,0x51db50
.equ target_5280b0,0x5280b0
.equ target_528120,0x528120
.equ target_528930,0x528930
.equ target_55e740,0x55e740
.equ target_55f3b0,0x55f3b0
.equ target_561170,0x561170
.equ target_56faa0,0x56faa0
.equ target_56fe10,0x56fe10
.equ target_571480,0x571480
.equ target_571d10,0x571d10
.equ target_5747c0,0x5747c0
.equ target_59efd0,0x59efd0
.equ target_5a1850,0x5a1850
.equ target_6d86bb,0x6d86bb
.equ target_8f8958,0x8f8958
.equ target_8f94bf,0x8f94bf
