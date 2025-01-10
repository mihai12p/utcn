#include "Physical.h"
#include "String.h"
#include "Spinlock.h"

#define FRAME_SIZE              PAGE_SIZE
#define MAX_AVAILABLE_ADDRESS   (0x1FFF0000)
#define RESERVED_MEMORY         (0xA00000)  // quantity of memory set up when transitioning to long mode
#define SACRIFICIAL_FRAME_VA    (RESERVED_MEMORY - FRAME_SIZE)
#define RESERVED_FRAME_COUNT    (RESERVED_MEMORY / FRAME_SIZE)
#define MAX_FRAME_COUNT         (MAX_AVAILABLE_ADDRESS / FRAME_SIZE)
#define BITMAP_SIZE             ((MAX_FRAME_COUNT + 7) / 8)

typedef struct _FRAME_MANAGER
{
    BYTE     FrameBitmap[BITMAP_SIZE];
    SPINLOCK Lock;
} FRAME_MANAGER;

static FRAME_MANAGER gFrameManager = { 0 };

extern QWORD PML4[];

_Requires_exclusive_lock_held_(gFrameManager.Lock)
static
VOID
MarkFrameInUse(
    _In_ QWORD FrameIdx
)
{
    SET_BIT(gFrameManager.FrameBitmap, FrameIdx);
}

_Requires_exclusive_lock_held_(gFrameManager.Lock)
static
VOID
InitFrameBitmap()
{
    for (DWORD i = 0; i < RESERVED_FRAME_COUNT; ++i)
    {
        MarkFrameInUse(i);
    }
}

_Requires_exclusive_lock_held_(gFrameManager.Lock)
static
VOID
MarkFrameFree(
    _In_ QWORD FrameIdx
)
{
    CLEAR_BIT(gFrameManager.FrameBitmap, FrameIdx);
}

_Use_decl_annotations_
int
FrameAlloc(
    _Out_ PQWORD FrameIdx,
    _In_  DWORD  FrameCount
)
{
    *FrameIdx = 0;

    int spinlockStatus = SpinlockAcquire(&gFrameManager.Lock);
    if (spinlockStatus < 0)
    {
        return 0;
    }

    for (QWORD i = RESERVED_FRAME_COUNT; i < MAX_FRAME_COUNT; ++i)
    {
        if (IS_BIT_SET(gFrameManager.FrameBitmap, i))
        {
            //
            // The frame is in use
            //
            continue;
        }

        int areContiguousFramesFree = 1;
        for (DWORD j = 0; j < FrameCount; ++j)
        {
            if (i + j >= MAX_FRAME_COUNT || IS_BIT_SET(gFrameManager.FrameBitmap, i + j))
            {
                areContiguousFramesFree = 0;
                break;
            }
        }

        if (areContiguousFramesFree)
        {
            *FrameIdx = i;
            for (DWORD j = 0; j < FrameCount; ++j)
            {
                MarkFrameInUse(i + j);
            }

            SpinlockRelease(&gFrameManager.Lock, spinlockStatus);
            return 1;
        }
    }

    SpinlockRelease(&gFrameManager.Lock, spinlockStatus);
    return 0;
}

VOID
FrameFree(
    _In_ QWORD FrameIdx,
    _In_ DWORD FrameCount
)
{
    int spinlockStatus = SpinlockAcquire(&gFrameManager.Lock);
    if (spinlockStatus < 0)
    {
        return;
    }

    for (DWORD i = 0; i < FrameCount; ++i)
    {
        MarkFrameFree(FrameIdx + i);
    }

    SpinlockRelease(&gFrameManager.Lock, spinlockStatus);
}

static inline QWORD PML4Idx(_In_ QWORD Address) { return (Address >> 39) & 0x1FF; }
static inline QWORD PDPTIdx(_In_ QWORD Address) { return (Address >> 30) & 0x1FF; }
static inline QWORD PDIdx(_In_ QWORD Address) { return (Address >> 21) & 0x1FF; }
static inline QWORD PTIdx(_In_ QWORD Address) { return (Address >> 12) & 0x1FF; }

static
VOID
SetSacrificialFrameToPhysicalAddr(
    _In_ QWORD PhysicalAddr
)
{
    PQWORD pml4 = PML4;
    QWORD pml4Idx = PML4Idx(SACRIFICIAL_FRAME_VA);
    QWORD pdptIdx = PDPTIdx(SACRIFICIAL_FRAME_VA);
    QWORD pdIdx = PDIdx(SACRIFICIAL_FRAME_VA);
    QWORD ptIdx = PTIdx(SACRIFICIAL_FRAME_VA);

    QWORD pdptPhysicalAddr = pml4[pml4Idx] & PAGE_MASK;
    PQWORD pdpt = (PQWORD)pdptPhysicalAddr;

    QWORD pdPhysicalAddr = pdpt[pdptIdx] & PAGE_MASK;
    PQWORD pd = (PQWORD)pdPhysicalAddr;

    QWORD ptPhysicalAddr = pd[pdIdx] & PAGE_MASK;
    PQWORD pt = (PQWORD)ptPhysicalAddr;

    pt[ptIdx] = (PhysicalAddr & PAGE_MASK) | BIT_PRESENT | BIT_READ_WRITE;

    __invlpg((PVOID)SACRIFICIAL_FRAME_VA);
}

static
QWORD
AllocPageTable()
{
    QWORD frameIdx = 0;
    int status = FrameAlloc(&frameIdx, 1);
    if (!status)
    {
        return 0;
    }

    QWORD physicalAddr = frameIdx * FRAME_SIZE;

    SetSacrificialFrameToPhysicalAddr(physicalAddr);

    //
    // Now that SACRIFICIAL_FRAME_VA points to physicalAddr, we can memset it directly
    //
    memnset((PVOID)SACRIFICIAL_FRAME_VA, 0, PAGE_SIZE);

    return physicalAddr;
}

_Use_decl_annotations_
int
MapPage(
    _In_ QWORD PhysicalAddr,
    _In_ QWORD VirtualAddress,
    _In_ QWORD Flags
)
{
    PQWORD pml4 = PML4;
    QWORD pml4Idx = PML4Idx(VirtualAddress);
    QWORD pdptIdx = PDPTIdx(VirtualAddress);
    QWORD pdIdx = PDIdx(VirtualAddress);
    QWORD ptIdx = PTIdx(VirtualAddress);

    if (!(pml4[pml4Idx] & BIT_PRESENT))
    {
        QWORD physicalAddr = AllocPageTable();
        if (physicalAddr == 0)
        {
            return 0;
        }

        pml4[pml4Idx] = physicalAddr | BIT_PRESENT | BIT_READ_WRITE;
    }

    PQWORD pdpt = (PQWORD)(pml4[pml4Idx] & PAGE_MASK);
    if (!(pdpt[pdptIdx] & BIT_PRESENT))
    {
        QWORD physicalAddr = AllocPageTable();
        if (physicalAddr == 0)
        {
            return 0;
        }

        pdpt[pdptIdx] = physicalAddr | BIT_PRESENT | BIT_READ_WRITE;
    }

    PQWORD pd = (PQWORD)(pdpt[pdptIdx] & PAGE_MASK);
    if (!(pd[pdIdx] & BIT_PRESENT))
    {
        QWORD physicalAddr = AllocPageTable();
        if (physicalAddr == 0)
        {
            return 0;
        }

        pd[pdIdx] = physicalAddr | BIT_PRESENT | BIT_READ_WRITE;
    }

    SetSacrificialFrameToPhysicalAddr(pd[pdIdx] & PAGE_MASK);

    PQWORD pt = (PQWORD)(SACRIFICIAL_FRAME_VA & PAGE_MASK);

    pt[ptIdx] = (PhysicalAddr & PAGE_MASK) | (Flags & 0xFFF);

    SetSacrificialFrameToPhysicalAddr(SACRIFICIAL_FRAME_VA);

    __invlpg((PVOID)VirtualAddress);

    return 1;
}