#include "TestMemory.h"
#include "Physical.h"
#include "Virtual.h"
#include "Heap.h"
#include "String.h"
#include "logging.h"

VOID
TestcasePage(
    _In_opt_ const char* Arguments
)
{
    LogMessage("\n== Testing Page Allocation ==\n");

    //
    // Allocate a physical frame
    //
    QWORD frameIdx = 0;
    int status = FrameAlloc(&frameIdx, 1);
    if (!status)
    {
        return;
    }

    QWORD frameAddr = frameIdx * PAGE_SIZE;

    //
    // Allocate a memory page
    //
    PVOID page1 = NULL;
    status = PageAlloc(&page1, 1, frameAddr);
    if (!status)
    {
        LogMessage("Page allocation failed!\n");
        FrameFree(frameIdx, 1);
        return;
    }

    //
    // Write data to the first page
    //
    char* buffer1 = (char*)page1;
    strcpy(buffer1, "Hello, World!");

    LogMessage("Page 1 allocated at: "); LogQword((QWORD)page1); LogMessage(", ");
    LogMessage("Data written: "); LogMessage(buffer1);

    //
    // Allocate another page using the same physical frame
    //
    PVOID page2 = NULL;
    status = PageAlloc(&page2, 1, frameAddr);
    if (!status)
    {
        LogMessage("\nPage 2 allocation failed!\n");
        PageFree(page1, 1, 1);
        return;
    }

    LogMessage("\nPage 2 allocated at: "); LogQword((QWORD)page2); LogMessage("\n");

    //
    // Verify the data in both pages
    //
    char* buffer2 = (char*)page2;
    if (!strncmp(buffer1, buffer2, strlen(buffer1)))
    {
        LogMessage("Success: Data matches in both pages!\n");
    }
    else
    {
        LogMessage("Error: Data mismatch in pages!\n");
    }

    //
    // Unmap the first page without freeing the physical frame
    //
    PageFree(page1, 1, 0);

    //
    // Verify data still accessible from the second page
    //
    if (!strncmp(buffer2, "Hello, World!", strlen(buffer2)))
    {
        LogMessage("Success: Data still accessible from page 2!\n");
    }
    else
    {
        LogMessage("Error: Data inaccessible from Page 2!\n");
    }

    PageFree(page2, 1, 1);

    LogMessage("== Page Test Completed ==\n");
}

VOID
TestcaseHeap(
    _In_opt_ const char* Arguments
)
{
    LogMessage("\n== Testing Heap Implementation ==\n");

    //
    // Create a heap
    //
    HEAP heap = { 0 };
    PVOID heapBase = NULL;
    DWORD heapSize = 0x1000;  // 4KB heap for testing

    int status = HeapCreate(&heap, heapBase, heapSize);
    if (!status)
    {
        LogMessage("Error: Heap creation failed.\n");
        return;
    }

    LogMessage("Heap created successfully. Base: ");
    LogQword((QWORD)heap.Base); LogMessage(", Size: "); LogQword(heap.Size); LogMessage(" bytes\n");

    //
    // Allocate multiple heap entries
    //
#define NUM_ENTRIES 4
    PVOID entries[NUM_ENTRIES] = { 0 };
    int randomValues[NUM_ENTRIES] = { 42, 84, 126, 168 };

    for (int i = 0; i < NUM_ENTRIES; ++i)
    {
        entries[i] = HeapAlloc(&heap, sizeof(int));
        if (!entries[i])
        {
            LogMessage("Error: Allocation failed for entry "); LogDword(i); LogMessage(".\n");
            HeapDestroy(&heap);
            return;
        }

        //
        // Write distinct data to each entry
        //
        *(int*)entries[i] = randomValues[i];
        LogMessage("Entry ");
        LogDword(i); LogMessage(" allocated at: "); LogQword((QWORD)entries[i]); LogMessage(", Data written: ");
        LogDword(randomValues[i]); LogMessage("\n");
    }

    //
    // Validate data in entries
    //
    for (int i = 0; i < NUM_ENTRIES; ++i)
    {
        if (*(int*)entries[i] != randomValues[i])
        {
            LogMessage("Error: Data mismatch in entry ");
            LogDword(i); LogMessage(". Expected: "); LogDword(randomValues[i]); LogMessage(", Found: "); LogDword(*(int*)entries[i]); LogMessage("\n");
        }
        else
        {
            LogMessage("Success: Data in entry ");
            LogDword(i); LogMessage(" is correct: "); LogDword(*(int*)entries[i]); LogMessage("\n");
        }
    }

    //
    // Free the entries
    //
    for (int i = 0; i < NUM_ENTRIES; ++i)
    {
        HeapFree(&heap, entries[i]);
        LogMessage("Entry ");
        LogDword(i); LogMessage(" at "); LogQword((QWORD)entries[i]); LogMessage(" freed.\n");
    }
#undef NUM_ENTRIES

    //
    // Destroy the heap
    //
    HeapDestroy(&heap);
    LogMessage("Heap destroyed successfully.\n");

    LogMessage("== Heap Test Completed ==\n");
}