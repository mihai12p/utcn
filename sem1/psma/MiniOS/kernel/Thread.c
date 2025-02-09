#include "Thread.h"
#include "logging.h"
#include "String.h"

#define THREAD_STACK_SIZE (PAGE_SIZE)     // 4KB stacks
#define SCHEDULER_QUANTUM 100             // 100ms time quantum

extern DWORD GetLAPICId();

THREAD_MANAGER gThreadMgmt = { 0 };

_Use_decl_annotations_
PTHREAD
GetCurrentThread()
{
    DWORD lapicId = GetLAPICId();
    return gThreadMgmt.CurrentThread[lapicId];
}

static
VOID
IdleThreadFunction(
    _In_opt_ PVOID Context
)
{
    while (1)
    {
        _enable();
        __halt();
    }
}

static
VOID
CreateIdleThread(
    _In_ DWORD CPUIdx
)
{
    PTHREAD thread = ThreadCreate(IdleThreadFunction, NULL);
    if (!thread)
    {
        return;
    }

    thread->IsIdle = 1;
    gThreadMgmt.IdleThreads[CPUIdx] = thread;

    LogMessage("Idle thread created\n");
}

VOID
ThreadMgmtInit(
    _In_ DWORD CPUCount
)
{
    if (_InterlockedCompareExchange(&gThreadMgmt.IsInitialized, 0, 0) == 1)
    {
        //
        // Already initialized, avoid reinitialization.
        //
        return;
    }

    PVOID heapBase = NULL;
    DWORD heapSize = 0x10000;
    int status = HeapCreate(&gThreadMgmt.ThreadMgmtHeap, heapBase, heapSize);
    if (!status)
    {
        return;
    }

    LogMessage("Heap created\n");

    InitializeListHead(&gThreadMgmt.AllThreads);
    InitializeListHead(&gThreadMgmt.ReadyThreads);

    gThreadMgmt.CurrentThread = HeapAlloc(&gThreadMgmt.ThreadMgmtHeap, CPUCount * sizeof(PTHREAD));
    if (!gThreadMgmt.CurrentThread)
    {
        HeapDestroy(&gThreadMgmt.ThreadMgmtHeap);
        return;
    }

    LogMessage("CurrentThread array allocated\n");

    gThreadMgmt.IdleThreads = HeapAlloc(&gThreadMgmt.ThreadMgmtHeap, CPUCount * sizeof(PTHREAD));
    if (!gThreadMgmt.IdleThreads)
    {
        HeapFree(&gThreadMgmt.ThreadMgmtHeap, gThreadMgmt.CurrentThread);
        HeapDestroy(&gThreadMgmt.ThreadMgmtHeap);
        return;
    }

    memnset(gThreadMgmt.CurrentThread, 0, CPUCount * sizeof(PTHREAD));
    memnset(gThreadMgmt.IdleThreads, 0, CPUCount * sizeof(PTHREAD));

    _InterlockedExchange(&gThreadMgmt.gLastThreadId, 0);

    LogMessage("IdleThreads array allocated\n");

    for (DWORD i = 0; i < CPUCount; ++i)
    {
        CreateIdleThread(i);
    }

    //
    // Mark as initialized after all setup is complete.
    //
    _InterlockedExchange(&gThreadMgmt.IsInitialized, 1);
}

_Use_decl_annotations_
PTHREAD
ThreadCreate(
    _In_     ThreadStart* Function,
    _In_opt_ PVOID        Context
)
{
    PTHREAD thread = HeapAlloc(&gThreadMgmt.ThreadMgmtHeap, sizeof(THREAD));
    if (!thread)
    {
        return NULL;
    }

    thread->StackBase = HeapAlloc(&gThreadMgmt.ThreadMgmtHeap, THREAD_STACK_SIZE);
    if (!thread->StackBase)
    {
        HeapFree(&gThreadMgmt.ThreadMgmtHeap, thread);
        return NULL;
    }

    PVOID stackTop = (PBYTE)thread->StackBase + THREAD_STACK_SIZE;

    memnset(&thread->Context, 0, sizeof(CPU_CONTEXT));
    thread->Context.rsp = (QWORD)stackTop;  // Top of allocated stack
    thread->Context.rip = (QWORD)Function;  // Thread entrypoint
    thread->Context.rcx = (QWORD)Context;   // Entrypoint's argument
    thread->Context.cs = 0x08;              // Kernel code segment
    thread->Context.ss = 0x10;              // Kernel data segment
    thread->Context.rflags = 0x202;         // Interrupts enabled

    thread->ThreadId = _InterlockedIncrement(&gThreadMgmt.gLastThreadId);
    thread->State = THREAD_STATE_READY;
    thread->IsIdle = 0;
    thread->IsWaitingForMutex = 0;
    thread->TickCount = 0;

    int status = SpinlockAcquire(&gThreadMgmt.ThreadMgmtLock);
    if (status < 0)
    {
        HeapFree(&gThreadMgmt.ThreadMgmtHeap, thread->StackBase);
        HeapFree(&gThreadMgmt.ThreadMgmtHeap, thread);
        return NULL;
    }

    InsertTailList(&gThreadMgmt.AllThreads, &thread->AllListEntry);
    InsertTailList(&gThreadMgmt.ReadyThreads, &thread->ReadyListEntry);

    SpinlockRelease(&gThreadMgmt.ThreadMgmtLock, status);

    return thread;
}

VOID
SchedulerTick(
    _Inout_ PCPU_CONTEXT Context
)
{
    if (_InterlockedCompareExchange(&gThreadMgmt.IsInitialized, 1, 1) == 0)
    {
        //
        // Threads are not initialized yet.
        //
        return;
    }

    int status = SpinlockAcquire(&gThreadMgmt.ThreadMgmtLock);
    if (status < 0)
    {
        return;
    }

    DWORD lapicId = GetLAPICId();
    PTHREAD currentThread = GetCurrentThread();
    PTHREAD nextThread = gThreadMgmt.IdleThreads[lapicId];

    if (currentThread && !currentThread->IsIdle)
    {
        memncpy(&currentThread->Context, Context, sizeof(CPU_CONTEXT));

        ++currentThread->TickCount;
        if (currentThread->TickCount >= SCHEDULER_QUANTUM)
        {
            currentThread->State = THREAD_STATE_READY;
            InsertTailList(&gThreadMgmt.ReadyThreads, &currentThread->ReadyListEntry);
            currentThread->TickCount = 0;
        }
        else
        {
            nextThread = currentThread;
        }
    }

    int isListEmpty = IsListEmpty(&gThreadMgmt.ReadyThreads);
    if (nextThread == gThreadMgmt.IdleThreads[lapicId] && !isListEmpty)
    {
        PLIST_ENTRY entry = RemoveHeadList(&gThreadMgmt.ReadyThreads);
        nextThread = CONTAINING_RECORD(entry, THREAD, ReadyListEntry);
    }

    if (currentThread != nextThread)
    {
        nextThread->State = THREAD_STATE_RUNNING;
        gThreadMgmt.CurrentThread[lapicId] = nextThread;
        memncpy(Context, &nextThread->Context, sizeof(CPU_CONTEXT));
    }

    SpinlockRelease(&gThreadMgmt.ThreadMgmtLock, status);
}