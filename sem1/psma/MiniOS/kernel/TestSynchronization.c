#include "TestSynchronization.h"
#include "Spinlock.h"
#include "logging.h"
#include "APIC.h"
#include "Heap.h"
#include "Interrupts.h"

extern LAPIC_INFO gLAPICInfo;

extern VOID SendIPI(_In_ DWORD LAPICId, _In_ DWORD VectorIdx);
extern DWORD GetLAPICId();

VOID
TestcaseSynchronizedPrint(
    _In_opt_ const char* Arguments
)
{
    LogMessage("\n== Testing Synchronized Prints ==\n");

    MarkAllCPUAvailable();

    _disable();

    //
    // BSP also runs the test so we are also sending an IPI to self.
    //
    for (BYTE i = 0; i < gLAPICInfo.LapicIdsCount; ++i)
    {
        BYTE lapicId = gLAPICInfo.LapicIds[i];
        LogMessage("Sending IPI to processor with LAPIC ID "); LogDword(lapicId); LogMessage("\n");
        SendIPI(lapicId, INTERRUPT_IPI_T1);
    }

    _enable();

    //
    // Wait for all non-BSP processors to indicate they're ready
    //
    for (BYTE i = 0; i < gLAPICInfo.LapicIdsCount; ++i)
    {
        BYTE lapicId = gLAPICInfo.LapicIds[i];
        if (lapicId != gLAPICInfo.BSPLAPICId)
        {
            while (!IsCPUReady(lapicId))
            {
                _mm_pause();
            }
        }
    }

    MarkAllCPUAvailable();

    LogMessage("== Synchronized Prints Test Completed ==\n");
}

static SPINLOCK gSynchronizedPrintLock = { 0 };

VOID
APTestcaseSynchronizedPrint()
{
    DWORD lapicId = GetLAPICId();
    for (int i = 0; i < 100; ++i)
    {
        int status = SpinlockAcquire(&gSynchronizedPrintLock);
        if (status >= 0)
        {
            LogMessage("hello from processor "); LogDword(lapicId); LogMessage("\n");
            SpinlockRelease(&gSynchronizedPrintLock, status);
        }
    }

    MarkCPUReady(lapicId);
}

typedef struct _LIST_ELEMENT
{
    struct _LIST_ELEMENT* Flink;
    struct _LIST_ELEMENT* Blink;
    DWORD LapicId;
    LONG  Counter;
} LIST_ELEMENT, * PLIST_ELEMENT;

static HEAP gLinkedListHeap = { 0 };
static LIST_ELEMENT gLinkedListHead = { 0 };
static SPINLOCK gLinkedListLock = { 0 };
static volatile LONG gCounter = 0;

VOID
TestcaseLinkedList(
    _In_opt_ const char* Arguments
)
{
    LogMessage("\n== Testing Synchronized Linked List ==\n");

    gCounter = 0;
    gLinkedListHead.Flink = NULL;
    gLinkedListHead.Blink = NULL;

    PVOID heapBase = NULL;
    DWORD heapSize = 0x6000;
    int status = HeapCreate(&gLinkedListHeap, heapBase, heapSize);
    if (!status)
    {
        LogMessage("Error: Heap creation failed.\n");
        return;
    }

    MarkAllCPUAvailable();

    _disable();

    //
    // BSP also runs the test so we are also sending an IPI to self.
    //
    for (BYTE i = 0; i < gLAPICInfo.LapicIdsCount; ++i)
    {
        BYTE lapicId = gLAPICInfo.LapicIds[i];
        LogMessage("Sending IPI to processor with LAPIC ID "); LogDword(lapicId); LogMessage("\n");
        SendIPI(lapicId, INTERRUPT_IPI_T2);
    }

    _enable();

    //
    // Wait for all non-BSP processors to indicate they're ready
    //
    for (BYTE i = 0; i < gLAPICInfo.LapicIdsCount; ++i)
    {
        BYTE lapicId = gLAPICInfo.LapicIds[i];
        if (lapicId != gLAPICInfo.BSPLAPICId)
        {
            while (!IsCPUReady(lapicId))
            {
                _mm_pause();
            }
        }
    }

    MarkAllCPUAvailable();

    LogMessage("All CPUs are ready, proceeding with the rest of the code.\n");

    status = SpinlockAcquire(&gLinkedListLock);
    if (status >= 0)
    {
        PLIST_ELEMENT current = gLinkedListHead.Flink;
        while (current)
        {
            LogMessage("CPU "); LogDword(current->LapicId); LogMessage(" Counter "); LogDword(current->Counter); LogMessage("\n");
            PLIST_ELEMENT previous = current;
            current = current->Flink;
            HeapFree(&gLinkedListHeap, previous);
        }
        SpinlockRelease(&gLinkedListLock, status);
    }

    HeapDestroy(&gLinkedListHeap);

    LogMessage("== Synchronized Linked List Test Completed ==\n");
}

VOID 
APTestcaseLinkedList()
{
    DWORD lapicId = GetLAPICId();
    for (int i = 0; i < 100; ++i)
    {
        //
        // Allocate a new element
        //
        PLIST_ELEMENT element = HeapAlloc(&gLinkedListHeap, sizeof(LIST_ELEMENT));
        if (!element)
        {
            continue;
        }

        //
        // Initialize element
        //
        element->LapicId = lapicId;
        element->Counter = _InterlockedIncrement(&gCounter);

        //
        // Add to list
        //
        int status = SpinlockAcquire(&gLinkedListLock);
        if (status >= 0)
        {
            element->Flink = gLinkedListHead.Flink;
            element->Blink = &gLinkedListHead;

            if (gLinkedListHead.Flink)
            {
                gLinkedListHead.Flink->Blink = element;
            }
            gLinkedListHead.Flink = element;

            SpinlockRelease(&gLinkedListLock, status);
        }
    }

    MarkCPUReady(lapicId);
}