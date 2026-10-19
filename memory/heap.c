/* Optional fixed-size first-fit heap. Physical boundary tags give immediate
 * coalescing; a doubly linked free list avoids scanning allocated blocks.
 * Offsets keep headers identical on 32/64-bit hosts and Atari. */
#include "heap.h"
#include "memory.h"

#define HEAP_MAGIC 0x48454150UL
#define HEAP_COOKIE 0x43554e4bUL
#define HEAP_NONE 0xffffffffUL
#define HEAP_HEADER 16UL
#define HEAP_MIN 24UL
#define HEAP_USED 1UL

typedef struct sHeapBlock
{
 U32 mSizeFlags;
 U32 mPrevSize;
 U32 mRequested;
 U32 mCookie;
} sHeapBlock;
/* Links occupy the first eight payload bytes only while a block is free. */
typedef struct sHeapLinks { U32 mPrev, mNext; } sHeapLinks;

static sHeapBlock *Heap_Block(const sHeap *h,U32 offset)
{return (sHeapBlock*)((U8*)h->mpBase+offset);}
static U32 Heap_Size(const sHeapBlock *b){return b->mSizeFlags & ~3UL;}
static sHeapLinks *Heap_Links(sHeapBlock *b){return (sHeapLinks*)((U8*)b+HEAP_HEADER);}
static U32 Heap_Cookie(const sHeap *h,U32 offset)
{return HEAP_COOKIE ^ (U32)(UPTR)h->mpBase ^ offset ^ h->mGeneration;}
static U32 Heap_Active(const sHeap *h)
{return h && h->mMagic==HEAP_MAGIC && h->mpBase && h->mSize>=HEAP_MIN;}
static U32 Heap_BlockValid(const sHeap *h,U32 offset)
{
 sHeapBlock *b;U32 size;
 if((offset&3) || offset>h->mSize-HEAP_MIN)return 0;
 b=Heap_Block(h,offset);size=Heap_Size(b);
 return b->mCookie==Heap_Cookie(h,offset) && !(b->mSizeFlags&2) &&
  size>=HEAP_MIN && size<=h->mSize-offset;
}
static void Heap_Remove(sHeap *h,U32 offset)
{
 sHeapLinks *l=Heap_Links(Heap_Block(h,offset));
 if(l->mPrev!=HEAP_NONE)Heap_Links(Heap_Block(h,l->mPrev))->mNext=l->mNext;
 else h->mFreeHead=l->mNext;
 if(l->mNext!=HEAP_NONE)Heap_Links(Heap_Block(h,l->mNext))->mPrev=l->mPrev;
}
static void Heap_Insert(sHeap *h,U32 offset)
{
 sHeapLinks *l=Heap_Links(Heap_Block(h,offset));
 l->mPrev=HEAP_NONE;l->mNext=h->mFreeHead;
 if(h->mFreeHead!=HEAP_NONE)Heap_Links(Heap_Block(h,h->mFreeHead))->mPrev=offset;
 h->mFreeHead=offset;
}

U32 Heap_Reset(sHeap *h)
{
 sHeapBlock *b;
 if(!Heap_Active(h))return 0;
 h->mUsedBytes=h->mRequestedBytes=h->mPeakUsedBytes=0;
 h->mAllocationCount=h->mFreeCount=h->mFailedAllocCount=h->mInvalidFreeCount=0;
 ++h->mGeneration;h->mFreeHead=HEAP_NONE;
 b=Heap_Block(h,0);b->mSizeFlags=h->mSize;b->mPrevSize=0;
 b->mRequested=0;b->mCookie=Heap_Cookie(h,0);Heap_Insert(h,0);
 return 1;
}
U32 Heap_InitWithBuffer(sHeap *h,void *buffer,U32 size)
{
 U32 skip;
 if(!h || h->mMagic==HEAP_MAGIC)return 0;
 Memory_Clear(sizeof(*h),h);
 if(!buffer)return 0;
 skip=(U32)(-(UPTR)buffer)&3;
 if(size<skip || (size-skip)<HEAP_MIN)return 0;
 /* Reject address wrap before rounding/adding offsets. */
 if((UPTR)buffer>(UPTR)-1-size)return 0;
 h->mpBase=(U8*)buffer+skip;h->mSize=(size-skip)&~3UL;h->mMagic=HEAP_MAGIC;
 return Heap_Reset(h);
}
U32 Heap_Init(sHeap *h,U32 size)
{
 void *raw;
 if(!h || h->mMagic==HEAP_MAGIC)return 0;
 Memory_Clear(sizeof(*h),h);
 if(size<HEAP_MIN || size>0x7ffffffcUL)return 0;
 raw=Memory_Alloc(size+3);
 if(!raw)return 0;
 if(!Heap_InitWithBuffer(h,raw,size+3)){_Memory_Release(raw);return 0;}
 /* Owned pools manage exactly size rounded down, not the alignment reserve. */
 h->mSize=size&~3UL;h->mpAllocation=raw;
 return Heap_Reset(h);
}
U32 Heap_DeInit(sHeap *h)
{
 if(!h)return 0;
 if(h->mMagic==HEAP_MAGIC && h->mpAllocation)_Memory_Release(h->mpAllocation);
 Memory_Clear(sizeof(*h),h);return 1;
}
void *Heap_Alloc(sHeap *h,U32 requested)
{
 U32 needed,offset;
 if(!Heap_Active(h))return 0;
 if(!requested || requested>0xffffffffUL-HEAP_HEADER-3){++h->mFailedAllocCount;return 0;}
 needed=(requested+HEAP_HEADER+3)&~3UL;if(needed<HEAP_MIN)needed=HEAP_MIN;
 offset=h->mFreeHead;
 while(offset!=HEAP_NONE){
  sHeapBlock *b=Heap_Block(h,offset);U32 size=Heap_Size(b);
  if(size>=needed){
   Heap_Remove(h,offset);
   if(size-needed>=HEAP_MIN){
    sHeapBlock *tail=Heap_Block(h,offset+needed);
    tail->mSizeFlags=size-needed;tail->mPrevSize=needed;tail->mRequested=0;
    tail->mCookie=Heap_Cookie(h,offset+needed);
    if(offset+size<h->mSize)Heap_Block(h,offset+size)->mPrevSize=size-needed;
    Heap_Insert(h,offset+needed);size=needed;
   }
   b->mSizeFlags=size|HEAP_USED;b->mRequested=requested;
   h->mUsedBytes+=size-HEAP_HEADER;h->mRequestedBytes+=requested;
   if(h->mUsedBytes>h->mPeakUsedBytes)h->mPeakUsedBytes=h->mUsedBytes;
   ++h->mAllocationCount;return (U8*)b+HEAP_HEADER;
  }
  offset=Heap_Links(b)->mNext;
 }
 ++h->mFailedAllocCount;return 0;
}
U32 Heap_FreeChecked(sHeap *h,void *mem)
{
 UPTR address;U32 offset,size,prev,next; sHeapBlock *b,*p=0,*n=0;
 if(!Heap_Active(h))return 0;
 if(!mem)return 1;
 address=(UPTR)mem;
 if(address<(UPTR)h->mpBase+HEAP_HEADER || address-(UPTR)h->mpBase>=h->mSize)goto invalid;
 offset=(U32)(address-(UPTR)h->mpBase)-HEAP_HEADER;
 if(!Heap_BlockValid(h,offset))goto invalid;
 b=Heap_Block(h,offset);size=Heap_Size(b);
 if(!(b->mSizeFlags&HEAP_USED) || !b->mRequested || b->mRequested>size-HEAP_HEADER)goto invalid;
 prev=b->mPrevSize;next=offset+size;
 if(offset){
  if(prev>offset || !prev || !Heap_BlockValid(h,offset-prev))goto invalid;
  p=Heap_Block(h,offset-prev);if(Heap_Size(p)!=prev)goto invalid;
 }else if(prev)goto invalid;
 if(next<h->mSize){
  if(!Heap_BlockValid(h,next))goto invalid;
  n=Heap_Block(h,next);if(n->mPrevSize!=size)goto invalid;
 }
 h->mUsedBytes-=size-HEAP_HEADER;h->mRequestedBytes-=b->mRequested;++h->mFreeCount;
 b->mSizeFlags=size;b->mRequested=0;
 if(n && !(n->mSizeFlags&HEAP_USED)){
  Heap_Remove(h,next);size+=Heap_Size(n);n->mCookie=0;
 }
 if(p && !(p->mSizeFlags&HEAP_USED)){
  Heap_Remove(h,offset-prev);b->mCookie=0;offset-=prev;size+=prev;b=p;
 }
 b->mSizeFlags=size;b->mRequested=0;
 if(offset+size<h->mSize)Heap_Block(h,offset+size)->mPrevSize=size;
 Heap_Insert(h,offset);return 1;
invalid:
 ++h->mInvalidFreeCount;return 0;
}
void Heap_Free(sHeap *h,void *mem){(void)Heap_FreeChecked(h,mem);}

U32 Heap_Validate(const sHeap *h)
{
 U32 offset=0,previous=0,freeCount=0,listed=0,last=HEAP_NONE,wasFree=0,used=0,requested=0;
 if(!Heap_Active(h) || ((UPTR)h->mpBase&3) || (h->mSize&3))return 0;
 while(offset<h->mSize){
  sHeapBlock *b;U32 size,isFree;
  if(!Heap_BlockValid(h,offset))return 0;
  b=Heap_Block(h,offset);size=Heap_Size(b);isFree=!(b->mSizeFlags&HEAP_USED);
  if(b->mPrevSize!=previous)return 0;
  if(isFree){if(wasFree || b->mRequested)return 0;++freeCount;}
  else{if(!b->mRequested || b->mRequested>size-HEAP_HEADER)return 0;used+=size-HEAP_HEADER;requested+=b->mRequested;}
  wasFree=isFree;previous=size;offset+=size;
 }
 if(offset!=h->mSize || used!=h->mUsedBytes || requested!=h->mRequestedBytes || used>h->mPeakUsedBytes)return 0;
 offset=h->mFreeHead;
 while(offset!=HEAP_NONE){
  sHeapBlock *b; sHeapLinks *l;
  if(++listed>freeCount || !Heap_BlockValid(h,offset))return 0;
  b=Heap_Block(h,offset);l=Heap_Links(b);
  if((b->mSizeFlags&HEAP_USED) || l->mPrev!=last)return 0;
  last=offset;offset=l->mNext;
 }
 return listed==freeCount;
}
U32 Heap_GetStats(const sHeap *h,sHeapStats *stats)
{
 U32 offset=0;
 if(!stats)return 0;
 Memory_Clear(sizeof(*stats),stats);
 if(!Heap_Validate(h))return 0;
 stats->mTotalBytes=h->mSize;stats->mUsedBytes=h->mUsedBytes;stats->mRequestedBytes=h->mRequestedBytes;
 stats->mPeakUsedBytes=h->mPeakUsedBytes;stats->mAllocationCount=h->mAllocationCount;
 stats->mFreeCount=h->mFreeCount;stats->mFailedAllocCount=h->mFailedAllocCount;stats->mInvalidFreeCount=h->mInvalidFreeCount;
 while(offset<h->mSize){
  sHeapBlock *b=Heap_Block(h,offset);U32 capacity=Heap_Size(b)-HEAP_HEADER;
  stats->mOverheadBytes+=HEAP_HEADER;
  if(b->mSizeFlags&HEAP_USED)++stats->mUsedBlocks;
  else{++stats->mFreeBlocks;stats->mFreeBytes+=capacity;if(capacity>stats->mLargestFreeBlock)stats->mLargestFreeBlock=capacity;}
  offset+=Heap_Size(b);
 }
 return 1;
}
