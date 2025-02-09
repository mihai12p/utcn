#include "Mutex.h"
#include "Heap.h"
#include "Thread.h"

extern THREAD_MANAGER gThreadMgmt;

VOID
MutexInit(
    _Out_ PMUTEX Mutex
)
{
    Mutex->Owner = NULL;
    InitializeListHead(&Mutex->WaitQueue);
}

VOID
MutexAcquire(
    _Inout_ PMUTEX  Mutex,
    _Inout_ PTHREAD CurrentThread
)
{
    int status = SpinlockAcquire(&Mutex->Lock);
    if (status < 0)
    {
        return;
    }

    while (1)
    {
        if (Mutex->Owner == NULL)
        {
            Mutex->Owner = CurrentThread;
            CurrentThread->IsWaitingForMutex = 0;
            SpinlockRelease(&Mutex->Lock, status);
            return;
        }

        if (!CurrentThread->IsWaitingForMutex)
        {
            CurrentThread->State = THREAD_STATE_BLOCKED;
            InsertTailList(&Mutex->WaitQueue, &CurrentThread->WaitListEntry);
            CurrentThread->IsWaitingForMutex = 1;
        }
        SpinlockRelease(&Mutex->Lock, status);

        _enable();
        __halt();

        status = SpinlockAcquire(&Mutex->Lock);
    }
}

VOID
MutexRelease(
    _Inout_ PMUTEX  Mutex,
    _In_    PTHREAD CurrentThread
)
{
    int status = SpinlockAcquire(&Mutex->Lock);
    if (status < 0)
    {
        return;
    }

    if (Mutex->Owner != CurrentThread)
    {
        SpinlockRelease(&Mutex->Lock, status);
        return;
    }

    Mutex->Owner = NULL;

    int isListEmpty = IsListEmpty(&Mutex->WaitQueue);
    if (!isListEmpty)
    {
        PLIST_ENTRY entry = RemoveHeadList(&Mutex->WaitQueue);
        PTHREAD next = CONTAINING_RECORD(entry, THREAD, WaitListEntry);

        int status2 = SpinlockAcquire(&gThreadMgmt.ThreadMgmtLock);
        if (status2 >= 0)
        {
            next->State = THREAD_STATE_READY;
            InsertTailList(&gThreadMgmt.ReadyThreads, &next->ReadyListEntry);
            SpinlockRelease(&gThreadMgmt.ThreadMgmtLock, status2);
        }

        Mutex->Owner = next;
    }

    SpinlockRelease(&Mutex->Lock, status);
}