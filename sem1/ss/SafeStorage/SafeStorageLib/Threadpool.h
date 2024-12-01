#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "includes.h"
EXTERN_C_START;

#include <Windows.h>

typedef VOID(WINAPI * fnThreadPoolTask)(_Inout_opt_ PVOID);

typedef struct _THREADPOOL
{
    PTP_POOL            ThreadPool;
    TP_CALLBACK_ENVIRON CallbackEnvironment;
    volatile LONG       TaskCount;              // total number of tasks to track
    HANDLE              CompletionEventHandle;  // event signaled when all tasks are done
} THREADPOOL, * PTHREADPOOL;

_Must_inspect_result_
PTHREADPOOL
ThreadPoolCreate(
    _In_     ULONG32 ThreadCount,
    _In_opt_ LONG    TaskCount
);

_Must_inspect_result_
NTSTATUS
ThreadPoolSubmitWork(
    _In_        PTHREADPOOL      Pool,
    _In_        fnThreadPoolTask Function,
    _Inout_opt_ PVOID            Argument
);

_Must_inspect_result_
NTSTATUS
ThreadPoolWaitForTasks(
    _In_ PTHREADPOOL Pool,
    _In_ ULONG32     Milliseconds
);

VOID
ThreadPoolDestroy(
    _In_ PTHREADPOOL Pool
);

EXTERN_C_END;
#endif//_THREADPOOL_H_