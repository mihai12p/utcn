#include "Utils.h"

VOID
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead;
    ListHead->Blink = ListHead;
}

VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY tail = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = tail;
    tail->Flink = Entry;
    ListHead->Blink = Entry;
}

_Use_decl_annotations_
int
IsListEmpty(
    _In_ PLIST_ENTRY ListHead
)
{
    return (ListHead->Flink == ListHead) ? 1 : 0;
}

_Use_decl_annotations_
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
)
{
    int isListEmpty = IsListEmpty(ListHead);
    if (isListEmpty)
    {
        return NULL;
    }

    PLIST_ENTRY firstEntry = ListHead->Flink;
    PLIST_ENTRY nextEntry = firstEntry->Flink;

    //
    // Update the links to remove the first entry
    //
    ListHead->Flink = nextEntry;
    nextEntry->Blink = ListHead;

    //
    // Disconnect first entry from the list
    //
    firstEntry->Flink = firstEntry;
    firstEntry->Blink = firstEntry;

    return firstEntry;
}