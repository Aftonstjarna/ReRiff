# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x528930..0x528ad1
.intel_syntax noprefix
.code32
.text
.global original_worker_queue
original_worker_queue:
.L528930:
 push   ebp
.L528931:
 {load} mov    ebp,esp
.L528933:
 push   0xffffffff
.L528935:
 push   0x90530c
.L52893a:
 mov    eax,fs:0x0
.L528940:
 push   eax
.L528941:
 sub    esp,0xc
.L528944:
 push   ebx
.L528945:
 push   esi
.L528946:
 push   edi
.L528947:
 mov    eax,[0xbaabc0]
.L52894c:
 {load} xor    eax,ebp
.L52894e:
 push   eax
.L52894f:
 lea    eax,[ebp-0xc]
.L528952:
 mov    fs:0x0,eax
.L528958:
 {load} mov    ebx,ecx
.L52895a:
 mov    DWORD PTR [ebp-0x14],ebx
.L52895d:
 test   ebx,ebx
.L52895f:
 jne .L528979
.L528961:
 mov    ecx,DWORD PTR [0xbfcc40]
.L528967:
 mov    eax,DWORD PTR [ecx]
.L528969:
 mov    eax,DWORD PTR [eax+0x168]
.L52896f:
 call   eax
.L528971:
 test   al,al
.L528973:
 jne .L528abf
.L528979:
 mov    esi,DWORD PTR [0xbfcc6c]
.L52897f:
 test   esi,esi
.L528981:
 jne .L5289c3
.L528983:
 push   0x48
.L528985:
 call   DWORD PTR [0x92d4b4]
.L52898b:
 {load} mov    esi,eax
.L52898d:
 add    esp,0x4
.L528990:
 mov    DWORD PTR [ebp-0x18],esi
.L528993:
 mov    DWORD PTR [ebp-0x4],0x0
.L52899a:
 test   esi,esi
.L52899c:
 je .L5289b4
.L52899e:
 push   0x507ee0
.L5289a3:
 push   0x507e50
.L5289a8:
 push   0x3
.L5289aa:
 push   0x18
.L5289ac:
 push   esi
.L5289ad:
 call target_8f94bf
.L5289b2:
 jmp .L5289b6
.L5289b4:
 {load} xor    esi,esi
.L5289b6:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L5289bd:
 mov    DWORD PTR [0xbfcc6c],esi
.L5289c3:
 mov    cl,BYTE PTR [0xbfcbbb]
.L5289c9:
 lea    eax,[ebx+ebx*2]
.L5289cc:
 mov    BYTE PTR [esi+eax*8],cl
.L5289cf:
 lea    edi,[esi+eax*8]
.L5289d2:
 test   cl,cl
.L5289d4:
 jne .L5289f5
.L5289d6:
 mov    DWORD PTR [edi+0x4],0x0
.L5289dd:
 mov    DWORD PTR [edi+0x8],0x0
.L5289e4:
 push   DWORD PTR [edi+0x10]
.L5289e7:
 push   DWORD PTR [edi+0xc]
.L5289ea:
 call target_507f60
.L5289ef:
 mov    eax,DWORD PTR [edi+0xc]
.L5289f2:
 mov    DWORD PTR [edi+0x10],eax
.L5289f5:
 cmp    BYTE PTR [edi],0x0
.L5289f8:
 je .L528abf
.L5289fe:
 mov    eax,[0xbfcc40]
.L528a03:
 {load} xor    ecx,ecx
.L528a05:
 mov    esi,DWORD PTR [edi+0xc]
.L528a08:
 mov    DWORD PTR [ebp-0x18],eax
.L528a0b:
 mov    eax,0x66666667
.L528a10:
 mov    DWORD PTR [ebp-0x10],ecx
.L528a13:
 mov    ecx,DWORD PTR [edi+0x10]
.L528a16:
 {load} sub    ecx,esi
.L528a18:
 imul   ecx
.L528a1a:
 sar    edx,0x4
.L528a1d:
 {load} mov    eax,edx
.L528a1f:
 shr    eax,0x1f
.L528a22:
 {load} add    eax,edx
.L528a24:
 je .L528a76
.L528a26:
 {load} xor    ebx,ebx
.L528a28:
 jmp .L528a30
.L528a2a:
 {disp32} lea    ebx,[ebx+0x0]
.L528a30:
 cmp    DWORD PTR [ebx+esi*1+0x14],0x10
.L528a35:
 lea    eax,[ebx+esi*1]
.L528a38:
 jb .L528a3e
.L528a3a:
 mov    ecx,DWORD PTR [eax]
.L528a3c:
 jmp .L528a40
.L528a3e:
 {load} mov    ecx,eax
.L528a40:
 push   DWORD PTR [ebp-0x14]
.L528a43:
 push   DWORD PTR [eax+0x1c]
.L528a46:
 push   ecx
.L528a47:
 push   DWORD PTR [eax+0x20]
.L528a4a:
 mov    ecx,DWORD PTR [ebp-0x18]
.L528a4d:
 call target_499120
.L528a52:
 mov    ecx,DWORD PTR [edi+0x10]
.L528a55:
 mov    eax,0x66666667
.L528a5a:
 mov    esi,DWORD PTR [edi+0xc]
.L528a5d:
 add    ebx,0x28
.L528a60:
 inc    DWORD PTR [ebp-0x10]
.L528a63:
 {load} sub    ecx,esi
.L528a65:
 imul   ecx
.L528a67:
 sar    edx,0x4
.L528a6a:
 {load} mov    eax,edx
.L528a6c:
 shr    eax,0x1f
.L528a6f:
 {load} add    eax,edx
.L528a71:
 cmp    DWORD PTR [ebp-0x10],eax
.L528a74:
 jb .L528a30
.L528a76:
 mov    DWORD PTR [edi+0x4],0x0
.L528a7d:
 mov    DWORD PTR [edi+0x8],0x0
.L528a84:
 mov    ebx,DWORD PTR [edi+0x10]
.L528a87:
 mov    esi,DWORD PTR [edi+0xc]
.L528a8a:
 {load} cmp    esi,ebx
.L528a8c:
 je .L528ab9
.L528a8e:
 {load} mov    edi,edi
.L528a90:
 cmp    DWORD PTR [esi+0x14],0x10
.L528a94:
 jb .L528aa1
.L528a96:
 push   DWORD PTR [esi]
.L528a98:
 call   DWORD PTR [0x92d4b0]
.L528a9e:
 add    esp,0x4
.L528aa1:
 mov    DWORD PTR [esi+0x14],0xf
.L528aa8:
 mov    DWORD PTR [esi+0x10],0x0
.L528aaf:
 mov    BYTE PTR [esi],0x0
.L528ab2:
 add    esi,0x28
.L528ab5:
 {load} cmp    esi,ebx
.L528ab7:
 jne .L528a90
.L528ab9:
 mov    eax,DWORD PTR [edi+0xc]
.L528abc:
 mov    DWORD PTR [edi+0x10],eax
.L528abf:
 mov    ecx,DWORD PTR [ebp-0xc]
.L528ac2:
 mov    DWORD PTR fs:0x0,ecx
.L528ac9:
 pop    ecx
.L528aca:
 pop    edi
.L528acb:
 pop    esi
.L528acc:
 pop    ebx
.L528acd:
 {load} mov    esp,ebp
.L528acf:
 pop    ebp
.L528ad0:
 ret
.equ target_499120,0x499120
.equ target_507f60,0x507f60
.equ target_8f94bf,0x8f94bf
