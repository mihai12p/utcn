#include "Heap.h"
#include "Virtual.h"

typedef struct _HEAP_HEADER
{
    LIST_ENTRY Entry;
    DWORD      Size;
    int        Free;
} HEAP_HEADER, * PHEAP_HEADER;

_Use_decl_annotations_
int
HeapCreate(
    _Inout_  PHEAP Heap,
    _In_opt_ PVOID Base,
    _In_     DWORD Size
)
{
    if (!Heap || Size == 0)
    {
        return 0;
    }

    DWORD alignedSize = (Size + PAGE_SIZE - 1) & PAGE_MASK;
    DWORD pageCount = alignedSize / PAGE_SIZE;

    int status = PageAlloc(&Base, pageCount, _UI64_MAX);
    if (!status)
    {
        return 0;
    }

    Heap->Base = Base;
    Heap->Size = alignedSize;

    PHEAP_HEADER firstBlock = (PHEAP_HEADER)Base;
    firstBlock->Size = alignedSize - sizeof(HEAP_HEADER);
    firstBlock->Free = 1;

    Heap->FreeList.Flink = Heap->FreeList.Blink = &firstBlock->Entry;
    firstBlock->Entry.Flink = firstBlock->Entry.Blink = &Heap->FreeList;

    return 1;
}

_Use_decl_annotations_
PVOID
HeapAlloc(
    _In_ PHEAP Heap,
    _In_ DWORD Size
)
{
    if (!Heap || Size == 0)
    {
        return NULL;
    }

    PLIST_ENTRY current = Heap->FreeList.Flink;
    while (current != &Heap->FreeList)
    {
        PHEAP_HEADER block = (PHEAP_HEADER)current;
        if (block->Free && block->Size >= Size)
        {
            if (block->Size >= Size + sizeof(HEAP_HEADER))
            {
                PHEAP_HEADER newBlock = (PHEAP_HEADER)((PBYTE)block + sizeof(HEAP_HEADER) + Size);
                newBlock->Size = block->Size - Size - sizeof(HEAP_HEADER);
                newBlock->Free = 1;

                newBlock->Entry.Flink = block->Entry.Flink;
                newBlock->Entry.Blink = (PLIST_ENTRY)block;

                block->Entry.Flink->Blink = (PLIST_ENTRY)newBlock;
                block->Entry.Flink = (PLIST_ENTRY)newBlock;

                block->Size = Size;
            }

            block->Free = 0;
            return (PVOID)((PBYTE)block + sizeof(HEAP_HEADER));
        }

        current = current->Flink;
    }

    return NULL;
}

VOID
HeapFree(
    _In_ PHEAP Heap,
    _In_ PVOID Address
)
{
    if (!Heap || !Address)
    {
        return;
    }

    PHEAP_HEADER block = (PHEAP_HEADER)((PBYTE)Address - sizeof(HEAP_HEADER));
    block->Free = 1;

    //
    // Merge with next block if possible
    //
    PHEAP_HEADER nextBlock = (PHEAP_HEADER)((PBYTE)block + sizeof(HEAP_HEADER) + block->Size);
    if ((PLIST_ENTRY)nextBlock == block->Entry.Flink && nextBlock->Free)
    {
        block->Size += nextBlock->Size + sizeof(HEAP_HEADER);
        block->Entry.Flink = nextBlock->Entry.Flink;
        nextBlock->Entry.Flink->Blink = (PLIST_ENTRY)block;
    }

    //
    // Merge with previous block if possible
    //
    PHEAP_HEADER prevBlock = (PHEAP_HEADER)block->Entry.Blink;
    if (prevBlock->Free && prevBlock != block)
    {
        prevBlock->Size += block->Size + sizeof(HEAP_HEADER);
        prevBlock->Entry.Flink = block->Entry.Flink;
        block->Entry.Flink->Blink = (PLIST_ENTRY)prevBlock;
    }
}

VOID
HeapDestroy(
    _In_ PHEAP Heap
)
{
    if (!Heap)
    {
        return;
    }

    //
    // Free allocated memory pages (if backing memory allocation is used)
    //
    PageFree(Heap->Base, Heap->Size / PAGE_SIZE, 1);
}