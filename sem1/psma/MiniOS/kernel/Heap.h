#ifndef _HEAP_H_
#define _HEAP_H_

#include "main.h"
#include "Spinlock.h"
#include "Utils.h"

typedef struct _HEAP
{
    PVOID      Base;
    DWORD      Size;
    LIST_ENTRY FreeList;
    SPINLOCK   Lock;
} HEAP, * PHEAP;

_Must_inspect_result_
int
HeapCreate(
    _Inout_  PHEAP Heap,
    _In_opt_ PVOID Base,
    _In_     DWORD Size
);

_Must_inspect_result_
PVOID
HeapAlloc(
    _In_ PHEAP Heap,
    _In_ DWORD Size
);

VOID
HeapFree(
    _In_ PHEAP Heap,
    _In_ PVOID Address
);

VOID
HeapDestroy(
    _In_ PHEAP Heap
);

#endif//_HEAP_H_