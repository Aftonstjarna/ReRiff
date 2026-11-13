#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static void log_line(const char *message){puts(message);}
#include "flac_sample_rate.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s (Win32=%lu)\n",__LINE__,#c,(unsigned long)GetLastError());return 1;}}while(0)
/* Test-only native mapping: execute original pure-native decoder/DSP, resolve
 * only their CRT/kernel imports, and never enter GameMain, rendering or saves. */
static BOOL resolve_native_imports(BYTE *image){
    IMAGE_NT_HEADERS32 *nt=(void *)(image+((IMAGE_DOS_HEADER *)image)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *imports=(void *)(image+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(;imports->Name;imports++){
        const char *name=(char *)image+imports->Name;
        if(_stricmp(name,"MSVCR120.dll")&&_stricmp(name,"MSVCP120.dll")&&_stricmp(name,"KERNEL32.dll"))continue;
        HMODULE library=LoadLibraryA(name);if(!library)return FALSE;
        IMAGE_THUNK_DATA32 *source=(void *)(image+imports->OriginalFirstThunk),*dest=(void *)(image+imports->FirstThunk);
        for(;source->u1.AddressOfData;source++,dest++){
            const char *symbol=IMAGE_SNAP_BY_ORDINAL32(source->u1.Ordinal)?(const char *)(uintptr_t)IMAGE_ORDINAL32(source->u1.Ordinal):((IMAGE_IMPORT_BY_NAME *)(image+source->u1.AddressOfData))->Name;
            FARPROC address=GetProcAddress(library,symbol);if(!address)return FALSE;
            DWORD old,ignored;if(!VirtualProtect(dest,4,PAGE_READWRITE,&old))return FALSE;
            dest->u1.Function=(DWORD)(uintptr_t)address;VirtualProtect(dest,4,old,&ignored);
        }
    }
    return TRUE;
}
static BOOL relocate_native_image(BYTE *image){
    DWORD actual=*(DWORD *)(image+0xf9086),expected=(DWORD)(uintptr_t)(image+0x503c4f);
    if(actual==expected)return TRUE;
    if(actual!=0x903c4f)return FALSE;
    IMAGE_NT_HEADERS32 *nt=(void *)(image+((IMAGE_DOS_HEADER *)image)->e_lfanew);
    IMAGE_DATA_DIRECTORY directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    DWORD delta=(DWORD)(uintptr_t)image-nt->OptionalHeader.ImageBase;
    for(DWORD offset=0;offset<directory.Size;){
        IMAGE_BASE_RELOCATION *block=(void *)(image+directory.VirtualAddress+offset);
        if(block->SizeOfBlock<8||block->SizeOfBlock>directory.Size-offset)return FALSE;
        WORD *items=(void *)(block+1);
        for(DWORD j=0;j<(block->SizeOfBlock-8)/2;j++)if((items[j]>>12)==IMAGE_REL_BASED_HIGHLOW){
            DWORD *operand=(void *)(image+block->VirtualAddress+(items[j]&4095)),old,ignored;
            if(!VirtualProtect(operand,4,PAGE_READWRITE,&old))return FALSE;
            *operand+=delta;VirtualProtect(operand,4,old,&ignored);
        }
        offset+=block->SizeOfBlock;
    }
    return *(DWORD *)(image+0xf9086)==expected;
}
static LONG CALLBACK stop_test_fault(EXCEPTION_POINTERS *fault){
    if(fault->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION){
        fprintf(stderr,"TEST native fault at %p; eax=%08lx ecx=%08lx edx=%08lx\n",fault->ExceptionRecord->ExceptionAddress,(unsigned long)fault->ContextRecord->Eax,(unsigned long)fault->ContextRecord->Ecx,(unsigned long)fault->ContextRecord->Edx);
        ExitProcess(210);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
int wmain(int argc,WCHAR **argv){
    AddVectoredExceptionHandler(1,stop_test_fault);
    CHECK(argc==4||argc==5);
    BYTE stream[42]={0};memcpy(stream,"fLaC",4);stream[7]=34;
    stream[18]=96000>>12;stream[19]=(96000>>4)&255;stream[20]=(96000&15)<<4;
    CHECK(flac_stream_rate(stream,sizeof(stream))==96000);
    CHECK(!flac_stream_rate(stream,41));
    BYTE tagged[52]={0};memcpy(tagged,"ID3",3);tagged[3]=4;memcpy(tagged+10,stream,42);
    CHECK(flac_stream_rate(tagged,sizeof(tagged))==96000);
    tagged[6]=128;CHECK(!flac_stream_rate(tagged,sizeof(tagged)));
    stream[7]=33;CHECK(!flac_stream_rate(stream,sizeof(stream)));
    HMODULE image=LoadLibraryExW(argv[1],NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(image);
    printf("IMAGE=%p SEH=%08lx new=%08lx\n",(void *)image,(unsigned long)*(DWORD *)((BYTE *)image+0xf9086),(unsigned long)*(DWORD *)((BYTE *)image+0x52d4b4));fflush(stdout);
    CHECK(relocate_native_image((BYTE *)image));
    CHECK(resolve_native_imports((BYTE *)image)&&flac_bind_original((BYTE *)image));
    printf("IAT new=%p free=%p\n",*(void **)((BYTE *)image+0x52d4b4),(void *)flac_free);fflush(stdout);
    if(argc==5)return 0;
    BYTE *site=(BYTE *)image+0x8db0b;DWORD guard_protection,guard_ignored;
    CHECK(VirtualProtect(site,1,PAGE_EXECUTE_READWRITE,&guard_protection));
    site[0]=0x90;CHECK(!flac_bind_original((BYTE *)image));site[0]=0xe8;
    CHECK(VirtualProtect(site,1,guard_protection,&guard_ignored));
    CHECK(install_flac_sample_rate((BYTE *)image));CHECK(install_flac_sample_rate((BYTE *)image));
    FILE *input=_wfopen(argv[2],L"rb");CHECK(input);CHECK(!fseek(input,0,SEEK_END));long size=ftell(input);CHECK(size>0&&size<INT_MAX);rewind(input);
    BYTE *data=malloc((size_t)size);CHECK(data&&fread(data,1,(size_t)size,input)==(size_t)size);fclose(input);
    unsigned rate=flac_stream_rate(data,(unsigned)size);CHECK(rate);
    float *original=NULL;int original_frames=0;char artist[256]={0},title[256]={0};
    puts("TEST: calling original FLAC decoder");fflush(stdout);
    unsigned ok=flac_call_decode(flac_decode_original,data,(unsigned)size,&original_frames,&original,&original_frames,artist,&original_frames,title,&original_frames);
    printf("TEST: original returned %u, frames=%d buffer=%p\n",ok,original_frames,(void *)original);fflush(stdout);
    CHECK((ok&255)&&original&&original_frames>0);
    float *converted=NULL;int frames=0;char patched_artist[256]={0},patched_title[256]={0};
    /* Invoke the actual installed CALL target through the caller-pop shim. */
    BYTE *call=(BYTE *)image+0x8db0b;int32_t relative;memcpy(&relative,call+1,4);void *entry=call+5+relative;
    ok=flac_call_decode(entry,data,(unsigned)size,&frames,&converted,&frames,patched_artist,&frames,patched_title,&frames);
    CHECK((ok&255)&&converted&&frames>0);
    CHECK(!memcmp(artist,patched_artist,256)&&!memcmp(title,patched_title,256));
    if(rate==44100)CHECK(frames==original_frames&&!memcmp(original,converted,(size_t)frames*sizeof(float)));
    else CHECK(fabs((double)frames-(double)original_frames*44100./rate)<=2.);
    for(int i=0;i<frames;i++)CHECK(isfinite(converted[i])&&fabs(converted[i])<2.);
    FILE *output=_wfopen(argv[3],L"wb");CHECK(output);CHECK(fwrite(converted,sizeof(float),(size_t)frames,output)==(size_t)frames);CHECK(!fclose(output));
    printf("PASS: original decoder, hook ABI, metadata and sample counts; source_rate=%u original_frames=%d frames=%d duration=%.9f\n",rate,original_frames,frames,frames/44100.);
    flac_delete_array(original);flac_delete_array(converted);free(data);return 0;
}
