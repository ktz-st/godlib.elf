#ifndef INCLUDED_HEAP_H
#define INCLUDED_HEAP_H

#include <godlib/base/base.h>

/* Optional first-fit arena. Zero-initialize before Init; DeInit before re-init.
 * Metadata is public only so callers can allocate the descriptor; do not edit it.
 * Allocations are 4-byte aligned. No heap operations are interrupt/thread safe. */
typedef struct sHeap
{
 void *mpBase;
 void *mpAllocation;
 U32 mSize;
 U32 mFreeHead;
 U32 mMagic;
 U32 mGeneration;
 U32 mUsedBytes;
 U32 mRequestedBytes;
 U32 mPeakUsedBytes;
 U32 mAllocationCount;
 U32 mFreeCount;
 U32 mFailedAllocCount;
 U32 mInvalidFreeCount;
} sHeap;

typedef struct sHeapStats
{
 U32 mTotalBytes;       /* managed bytes, including all block headers */
 U32 mUsedBytes;        /* allocated payload capacity, padding included */
 U32 mRequestedBytes;   /* exact live requests */
 U32 mFreeBytes;        /* sum of allocatable free payload capacities */
 U32 mLargestFreeBlock;
 U32 mOverheadBytes;    /* block headers; total = used + free + overhead */
 U32 mUsedBlocks;
 U32 mFreeBlocks;
 U32 mPeakUsedBytes;    /* peak live payload capacity since Reset */
 U32 mAllocationCount;  /* successful allocations since Reset */
 U32 mFreeCount;        /* successful frees since Reset */
 U32 mFailedAllocCount; /* includes zero-size/overflow requests */
 U32 mInvalidFreeCount; /* rejected non-NULL pointers */
} sHeapStats;

/* Init owns one Memory_Alloc block. InitWithBuffer never owns/frees the buffer.
 * Size includes metadata. Reset invalidates all allocations and clears counters.
 * Init/DeInit/Reset/GetStats/Validate return 1 on success, 0 on failure.
 * DeInit on an already empty zero descriptor succeeds. */
U32 Heap_Init(sHeap *apHeap, U32 aSize);
U32 Heap_InitWithBuffer(sHeap *apHeap, void *apBuffer, U32 aSize);
U32 Heap_DeInit(sHeap *apHeap);
U32 Heap_Reset(sHeap *apHeap);
void *Heap_Alloc(sHeap *apHeap, U32 aSize);
void Heap_Free(sHeap *apHeap, void *apMem);
/* NULL free is a successful no-op on an active heap. Foreign/interior/double
 * frees are rejected. A stale pointer reused by a later allocation is not
 * distinguishable from its current owner. */
U32 Heap_FreeChecked(sHeap *apHeap, void *apMem);
U32 Heap_GetStats(const sHeap *apHeap, sHeapStats *apStats);
U32 Heap_Validate(const sHeap *apHeap);

#endif
