# Optional HEAP allocator

`memory/heap.h` provides explicit, independent fixed-size arenas. The object is
included in `libgod.a`, but is linked into a program only when its Heap_* APIs
are referenced. It does not replace Memory_Alloc, libcmini malloc or mMEM*.
Existing library modules keep their current allocation behavior.

```c
#include <godlib/memory/heap.h>

sHeap heap = {0};
sHeapStats stats;
void *object;

if (Heap_Init(&heap, 32768UL)) {
    object = Heap_Alloc(&heap, 120);
    if (object) {
        /* Use object; contents are initially uninitialized. */
        Heap_Free(&heap, object);
    }
    if (Heap_GetStats(&heap, &stats)) {
        /* stats.mFreeBytes and stats.mLargestFreeBlock describe this arena. */
    }
    Heap_DeInit(&heap);
}
```

`Heap_Init()` obtains exactly one system backing allocation. Alloc/Free/Reset and
statistics make no system allocations. The size argument includes block headers;
it is rounded down to four bytes. The backing request adds three bytes so a
GEMDOS block aligned to only two bytes can still provide four-byte-aligned data.
A fresh 32768-byte arena therefore offers one 32752-byte payload. Initialization
returns 0 on failure; sizes below 24 or above 0x7ffffffc are rejected before the
system allocator is called (avoiding GEMDOS's negative-size query convention).

For caller-owned memory:

```c
sHeap scratch = {0};
/* buffer must remain writable and live until DeInit. */
if (Heap_InitWithBuffer(&scratch, buffer, bufferSize)) {
    /* ... */
    Heap_DeInit(&scratch); /* never frees buffer */
}
```

An external buffer is aligned inward: up to three leading bytes and up to three
trailing bytes are excluded and untouched. The descriptor must not overlap the
buffer or another live arena. DeInit never frees external storage.

## Allocation and release

- Zero-initialize the descriptor before first use. Reinitializing a live heap
  fails without changing it; call Reset or DeInit first.
- Allocations have four-byte alignment. Requests of zero or overflowing sizes
  return NULL and increment the failure counter of an active arena. Allocation
  failure leaves all existing blocks/data intact.
- The free list uses first fit. Blocks split when the remainder can hold a
  complete free block. Free immediately joins adjacent free blocks on both sides;
  no separate deferred merge is required.
- A block has a 16-byte header. Free-list links use eight otherwise-free payload
  bytes, so minimum block size is 24 bytes (minimum allocated capacity: eight).
  A remainder smaller than 24 bytes becomes extra capacity of the allocation.
- Heap_Free is the original void API. Heap_FreeChecked additionally returns 1
  for a successful release, or 0 for a rejected pointer/inactive heap. NULL is a
  successful no-op on an active heap. Foreign, misaligned/interior and ordinary
  double frees are rejected using range, cookie and boundary checks.
- This is not a hardened allocator against forged metadata. Stale pointers whose
  addresses have been reused by new allocations cannot identify their old owner.
  As with any allocator, do not use pointers after Free/Reset/DeInit.
- Reset keeps the backing allocation, invalidates all live pointers, returns the
  arena to one free block, changes the header generation and clears statistics.
  Ordinary stale interior headers from before Reset are rejected.
- DeInit releases owned storage once and clears the descriptor; repeated DeInit
  on an empty descriptor succeeds. It also invalidates all outstanding pointers.
- Release these pointers only through Heap_Free/Heap_FreeChecked on their owning
  arena, never through Memory_Free/mMEMFREE, libcmini free or another heap.

Alloc searches the free list (O(number of free blocks)); valid Free and neighbor
coalescing use boundary tags/free-list links in constant time. Validate and
GetStats walk the arena and free list. Reset does not clear payload bytes.

No calls are interrupt/thread safe. Do not share an arena between normal code
and VBL/Timer callbacks without external synchronization; do not allocate/free
from an interrupt that can interrupt another operation on the same arena.
Four-byte alignment and backing-memory placement are not a replacement for the
screen/DMA allocation APIs or their stricter alignment/STRAM requirements.

## Statistics and consistency

Heap_GetStats returns 1 on success and 0 for an inactive/corrupt heap (or NULL
output). A supplied output structure is cleared on failure. Heap_Validate returns
1 for a valid active arena, otherwise 0; it checks physical boundary tags, sizes,
cookies, adjacent-free coalescing, free-list links/count and live-byte counters.
It allocates no memory and does not intentionally crash on validation failure.

| Field | Meaning |
| --- | --- |
| mTotalBytes | Managed arena bytes, including all headers |
| mUsedBytes | Live payload capacities, including padding/unsplittable tails |
| mRequestedBytes | Exact total of the live requests |
| mFreeBytes | Sum of all free payload capacities |
| mLargestFreeBlock | Largest request currently satisfiable |
| mOverheadBytes | Total current block-header bytes |
| mUsedBlocks / mFreeBlocks | Current physical block counts |
| mPeakUsedBytes | Maximum simultaneous payload capacity since Reset |
| mAllocationCount / mFreeCount | Successful operations since Reset |
| mFailedAllocCount | Zero-size, overflow or unsatisfied allocation requests |
| mInvalidFreeCount | Rejected non-NULL frees on an active heap |

`mTotalBytes == mUsedBytes + mFreeBytes + mOverheadBytes`.
Splitting adds a header; joining removes headers and recovers their bytes as
payload capacity. Thus summing free capacities before a merge is not the same
as the capacity after it. Lifetime operation counters are U32 and can wrap.

## Tests

```sh
make -C godlib
make -C godlib/memory/tests check-host
make -C godlib/memory/tests check
```

The same C driver runs on macOS with AddressSanitizer/UndefinedBehaviorSanitizer
and on an actual emulated 68000 ABI (`-mshort -mfastcall`). It checks ownership,
forced host allocation failure, alignment and external-buffer canaries, exact
fits, unsplittable tails, fragmentation, both-direction merging, counters,
validation, bad frees, reset/deinit, and allocations/offsets over 65535 bytes in
a 200000-byte arena. A deterministic 3000-operation trace uses 48 slots, checks
all live payload patterns after every operation and finishes with full coalescing.
Native result and executable hash: [tests/verification.json](tests/verification.json).
Tests use STE / 8 MHz / 1 MiB / German TOS 1.62. These are correctness tests, not
cycle-cost measurements or verification on physical hardware.
