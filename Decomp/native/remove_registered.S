.intel_syntax noprefix
.code32
.text
.global original_remove_registered
original_remove_registered:
 push ebp
 {load} mov ebp,esp
 push ebx
 push esi
 {load} mov esi,ecx
 lea ebx,[esi+0xa84]
 push ebx
 call DWORD PTR [0x92d024]
 mov eax,DWORD PTR [esi+0xa7c]
 {load} xor edx,edx
 sub eax,DWORD PTR [esi+0xa78]
 sar eax,2
 test eax,eax
 je .unlock
 mov ecx,DWORD PTR [esi+0xa78]
 push edi
 mov edi,DWORD PTR [ebp+8]
.search:
 cmp DWORD PTR [ecx],edi
 je .found
 mov eax,DWORD PTR [esi+0xa7c]
 inc edx
 sub eax,DWORD PTR [esi+0xa78]
 add ecx,4
 sar eax,2
 {load} cmp edx,eax
 jb .search
 pop edi
 push ebx
 call DWORD PTR [0x92d028]
 pop esi
 pop ebx
 pop ebp
 ret 4
.found:
 mov eax,DWORD PTR [esi+0xa78]
 mov ecx,DWORD PTR [esi+0xa7c]
 lea eax,[eax+edx*4]
 lea edx,[eax+4]
 {load} sub ecx,edx
 and ecx,-4
 push ecx
 push edx
 push eax
 call DWORD PTR [0x92d4a4]
 add esp,12
 add DWORD PTR [esi+0xa7c],-4
 pop edi
.unlock:
 push ebx
 call DWORD PTR [0x92d028]
 pop esi
 pop ebx
 pop ebp
 ret 4
