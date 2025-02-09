#ifndef _THREAD_H_
#define _THREAD_H_

#include "main.h"
#include "Heap.h"
#include "Interrupts.h"

typedef enum _THREAD_STATE
{
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,

    MAX_THREAD_STATE
} THREAD_STATE;

typedef struct _THREAD
{
    DWORD        ThreadId;
    THREAD_STATE State;
    PVOID        StackBase;
    LIST_ENTRY   AllListEntry;
    LIST_ENTRY   ReadyListEntry;
    LIST_ENTRY   WaitListEntry;
    int          IsIdle;
    int          IsWaitingForMutex;
    DWORD        TickCount;
    CPU_CONTEXT  Context;
} THREAD, * PTHREAD;

typedef struct _THREAD_MANAGER
{
    volatile LONG IsInitialized;
    HEAP          ThreadMgmtHeap;
    SPINLOCK      ThreadMgmtLock;
    LIST_ENTRY    AllThreads;
    LIST_ENTRY    ReadyThreads;
    volatile LONG gLastThreadId;
    PTHREAD*      CurrentThread;
    PTHREAD*      IdleThreads;
} THREAD_MANAGER;

typedef
VOID
(__cdecl ThreadStart)(
    _In_opt_ PVOID Context
);

_Must_inspect_result_
PTHREAD
ThreadCreate(
    _In_     ThreadStart* Function,
    _In_opt_ PVOID        Context
);

VOID
SchedulerTick(
    _Inout_ PCPU_CONTEXT Context
);

VOID
ThreadMgmtInit(
    _In_ DWORD CPUCount
);

_Must_inspect_result_
PTHREAD
GetCurrentThread();

#endif//_THREAD_H_