#include "Virtual.h"
#include "Physical.h"
#include "String.h"
#include "Spinlock.h"

#define AVAILABLE_MEMORY_START      (0x1200000)
#define AVAILABLE_MEMORY_END        (0x10000000)

typedef struct _VM_MANAGER
{
    QWORD    PageMap[0x20000];
    SPINLOCK Lock;
} VM_MANAGER;

static VM_MANAGER gVMManager = { 0 };

_Must_inspect_result_
_Requires_exclusive_lock_held_(gVMManager.Lock)
static
int
IsPageMapped(
    _In_ PVOID Page
)
{
    QWORD pageIdx = (QWORD)Page / PAGE_SIZE;
    return (gVMManager.PageMap[pageIdx] & BIT_PRESENT) == BIT_PRESENT;
}

_Use_decl_annotations_
int
PageAlloc(
    _Inout_opt_ PVOID* Page,
    _In_        DWORD  PageCount,
    _In_        QWORD  FrameAddr
)
{
    if (!Page)
    {
        return 0;
    }

    int spinlockStatus = SpinlockAcquire(&gVMManager.Lock);
    if (spinlockStatus < 0)
    {
        return 0;
    }

    if (*Page == NULL)
    {
        for (QWORD address = AVAILABLE_MEMORY_START; address < AVAILABLE_MEMORY_END; address += PAGE_SIZE)
        {
            int found = 1;
            for (DWORD i = 0; i < PageCount; ++i)
            {
                int status = IsPageMapped((PVOID)(address + i * PAGE_SIZE));
                if (!status)
                {
                    continue;
                }

                found = 0;
                break;
            }

            if (found)
            {
                *Page = (PVOID)address;
                break;
            }
        }

        if (*Page == NULL)
        {
            //
            // No free pages found.
            //
            SpinlockRelease(&gVMManager.Lock, spinlockStatus);
            return 0;
        }
    }

    if (FrameAddr == _UI64_MAX)
    {
        QWORD frameIdx = 0;
        int status = FrameAlloc(&frameIdx, PageCount);
        if (!status)
        {
            SpinlockRelease(&gVMManager.Lock, spinlockStatus);
            return 0;
        }

        for (DWORD i = 0; i < PageCount; ++i)
        {
            QWORD physicalAddr = (frameIdx + i) * PAGE_SIZE;
            QWORD pageIdx = ((QWORD)*Page + (i * PAGE_SIZE)) / PAGE_SIZE;
            gVMManager.PageMap[pageIdx] = physicalAddr | BIT_PRESENT | BIT_READ_WRITE;

            (VOID)MapPage(physicalAddr, pageIdx * PAGE_SIZE, BIT_PRESENT | BIT_READ_WRITE);
        }
    }
    else
    {
        for (DWORD i = 0; i < PageCount; ++i)
        {
            QWORD pageIdx = ((QWORD)*Page + (i * PAGE_SIZE)) / PAGE_SIZE;
            gVMManager.PageMap[pageIdx] = (FrameAddr + (i * PAGE_SIZE)) | BIT_PRESENT | BIT_READ_WRITE;

            (VOID)MapPage(FrameAddr + (i * PAGE_SIZE), pageIdx * PAGE_SIZE, BIT_PRESENT | BIT_READ_WRITE);
        }
    }

    SpinlockRelease(&gVMManager.Lock, spinlockStatus);
    return 1;
}

VOID
PageFree(
    _In_ PVOID PageAddr,
    _In_ DWORD PageCount,
    _In_ int   FreeBacking
)
{
    int spinlockStatus = SpinlockAcquire(&gVMManager.Lock);
    if (spinlockStatus < 0)
    {
        return;
    }

    QWORD pageIdx = (QWORD)PageAddr / PAGE_SIZE;
    for (DWORD i = 0; i < PageCount; ++i)
    {
        if (FreeBacking && (gVMManager.PageMap[pageIdx + i] & BIT_PRESENT))
        {
            QWORD frameAddr = gVMManager.PageMap[pageIdx + i] & PAGE_MASK;
            FrameFree(frameAddr / PAGE_SIZE, 1);
        }

        gVMManager.PageMap[pageIdx + i] = 0;

        (VOID)MapPage(0, (QWORD)PageAddr + (i * PAGE_SIZE), 0);
    }

    SpinlockRelease(&gVMManager.Lock, spinlockStatus);
}