#ifndef _UTILS_H_
#define _UTILS_H_

#include "main.h"

typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY* Flink;
    struct _LIST_ENTRY* Blink;
} LIST_ENTRY, * PLIST_ENTRY;

VOID
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
);

VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
);

_Must_inspect_result_
int
IsListEmpty(
    _In_ PLIST_ENTRY ListHead
);

_Must_inspect_result_
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
);

#endif//_UTILS_H_