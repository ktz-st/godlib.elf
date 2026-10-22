/* Build with GCC ELF -mshort -mfastcall; run in verify.py's --target-dir.
   NATIVE.TXT is the pass/fail report. No display/hardware initialization needed. */
#include <godlib/packer/packer.h>
#include <godlib/packer/imploder.h>
#include <godlib/linkfile/linkfile.h>
#include <godlib/memory/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern U32 Imploder_ChecksumAsm(const void *, U32);

static unsigned char *load(const char *name, U32 *size)
{
    FILE *f = fopen(name,"rb");
    unsigned char *p;
    if(!f) return 0;
    fseek(f,0,SEEK_END); *size = ftell(f); rewind(f);
    p = malloc(*size);
    if(!p || fread(p,1,*size,f) != *size) { free(p); p=0; }
    fclose(f); return p;
}

volatile U16 gDone, gFailures, gCases;
static void check(U16 ok) { if(!ok) ++gFailures; }

int main(void)
{
    char name[16];
    U16 n, j;
    U32 packedSize, rawSize;
    U8 *packed, *raw, *space, *dst, *odd;
    FILE *log;
    for(n=0;n<999;++n)
    {
        sprintf(name,"f%03u.imp",n); packed=load(name,&packedSize);
        sprintf(name,"f%03u.raw",n); raw=load(name,&rawSize);
        if(!packed && n>=72) { free(raw); break; }
        check(packed && raw);
        if(!packed || !raw) { free(packed); free(raw); break; }
        space=malloc(rawSize+32); check(space!=0);
        if(!space) { free(packed); free(raw); break; }
        dst=space+16;
        check(Packer_GetType((sPackerHeader*)packed)==ePACKER_IMPLODER);
        check(Packer_GetDepackSize((sPackerHeader*)packed)==rawSize);
        check(Packer_GetHeaderSize((sPackerHeader*)packed)==12);
        check(Packer_GetLoadOffset((sPackerHeader*)packed)==0);
        check(Imploder_GetPackedSize(packed)==packedSize);
        memset(space,0xa5,rawSize+32);
        check(Imploder_DepackChecked(packed,packedSize,dst,rawSize));
        check(memcmp(raw,dst,rawSize)==0);
        /* Separate decode must preserve the packed header and checksum. */
        check(Imploder_IsPacked(packed));
        for(j=0;j<16;++j)check(space[j]==0xa5 && dst[rawSize+j]==0xa5);
        memset(dst,0xa5,rawSize);
        memcpy(dst,packed,packedSize);
        Packer_Depack(dst,dst);
        check(memcmp(raw,dst,rawSize)==0);
        for(j=0;j<16;++j)check(space[j]==0xa5 && dst[rawSize+j]==0xa5);
        memset(dst,0xa5,rawSize);
        Packer_Depack(packed,dst);
        check(memcmp(raw,dst,rawSize)==0);
        /* The ASM checksum must also support an unaligned read-only source. */
        odd=malloc(packedSize+1); check(odd!=0);
        if(odd)
        {
            memcpy(odd+1,packed,packedSize);
            memset(dst,0xa5,rawSize);
            check(Imploder_DepackChecked(odd+1,packedSize,dst,rawSize));
            check(memcmp(raw,dst,rawSize)==0);
            check(memcmp(odd+1,packed,packedSize)==0);
            free(odd);
        }
        /* All these fail before writing the destination. */
        memset(dst,0xa5,rawSize);
        check(!Imploder_DepackChecked(packed,packedSize-1,dst,rawSize));
        check(!Imploder_DepackChecked(packed,packedSize,dst,rawSize-1));
        check(!Imploder_DepackChecked(packed,packedSize,dst+1,rawSize));
        check(!Imploder_DepackChecked(packed,packedSize,packed+2,rawSize));
        packed[packedSize-1]^=1;
        check(!Imploder_DepackChecked(packed,packedSize,dst,rawSize));
        packed[packedSize-1]^=1;
        for(j=0;j<16;++j)check(dst[j]==0xa5);
        ++gCases;
        free(space); free(raw); free(packed);
    }
    /* Whole-archive IMP! loading uses Packer's allocation and dispatch path. */
    Memory_Init();
    {
        sLinkFile *link=LinkFile_InitToRAM("SCENE.IMP");
        U8 *item;
        U32 expectedSize;
        U8 *expected=load("f000.raw",&expectedSize);
        check(link!=0 && expected!=0);
        if(link && expected)
        {
            item=LinkFile_FileLoad(link,"PAYLOAD.BIN",1,0);
            check(item!=0);
            if(item)check(memcmp(item,expected,expectedSize)==0);
        }
        if(link) LinkFile_DeInit(link);
        free(expected);
    }
    Memory_DeInit();
    /* Word-sum wraps at 32 bits: 65537 * 65535 + 7 == 6. */
    space=malloc(131076UL); check(space!=0);
    if(space)
    {
        memset(space,0xff,131076UL);
        /* Exercise every remainder of the eight-word checksum loop. */
        for(j=0;j<=40;j+=2)
        {
            check(Imploder_ChecksumAsm(space,j)==7UL+(U32)(j/2)*65535UL);
            check(Imploder_ChecksumAsm(space+1,j)==7UL+(U32)(j/2)*65535UL);
        }
        check(Imploder_ChecksumAsm(space,0)==7);
        check(Imploder_ChecksumAsm(space,2)==65542UL);
        check(Imploder_ChecksumAsm(space,131074UL)==6);
        check(Imploder_ChecksumAsm(space+1,131074UL)==6);
        free(space);
    }
    check(!Imploder_IsPacked(0));
    check(!Imploder_DepackChecked(0,0,0,0));
    log=fopen("NATIVE.TXT","w");
    if(log) { fprintf(log,"cases=%u failures=%u\n",gCases,gFailures); fclose(log); }
    gDone=1;
    return gFailures ? 1 : 0;
}
