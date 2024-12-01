#include "Threadpool.h"

typedef struct _WORK_ITEM
{
    PTHREADPOOL      ThreadPool;
    fnThreadPoolTask Function;
    PVOID            Argument;
} WORK_ITEM, * PWORK_ITEM;

_Use_decl_annotations_
PTHREADPOOL
ThreadPoolCreate(
    _In_     ULONG32 ThreadCount,
    _In_opt_ LONG    TaskCount
)
{
    PTHREADPOOL pool = calloc(1, sizeof(THREADPOOL));
    if (!pool)
    {
        return NULL;
    }

    pool->TaskCount = TaskCount;
    if (pool->TaskCount > 0)
    {
        pool->CompletionEventHandle = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!IS_VALID_HANDLE(pool->CompletionEventHandle))
        {
            free(pool);
            return NULL;
        }
    }

    pool->ThreadPool = CreateThreadpool(NULL);
    if (!pool->ThreadPool)
    {
        if (IS_VALID_HANDLE(pool->CompletionEventHandle))
        {
            DBG_ASSERT(NT_SUCCESS(CloseHandle(pool->CompletionEventHandle)));
        }
        free(pool);
        return NULL;
    }

    SetThreadpoolThreadMinimum(pool->ThreadPool, ThreadCount);
    SetThreadpoolThreadMaximum(pool->ThreadPool, ThreadCount);

    InitializeThreadpoolEnvironment(&pool->CallbackEnvironment);
    SetThreadpoolCallbackPool(&pool->CallbackEnvironment, pool->ThreadPool);

    return pool;
}

static
VOID
CALLBACK
ThreadPoolCallback(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context
)
{
    UNREFERENCED_PARAMETER(Instance);

    if (!Context)
    {
        return;
    }

    PWORK_ITEM workItem = Context;
    if (workItem->Function)
    {
        workItem->Function(workItem->Argument);
    }

    if (workItem->ThreadPool)
    {
        if (IS_VALID_HANDLE(workItem->ThreadPool->CompletionEventHandle))
        {
            LONG remaining = InterlockedDecrement(&workItem->ThreadPool->TaskCount);
            if (remaining == 0)
            {
                (VOID)SetEvent(workItem->ThreadPool->CompletionEventHandle);
            }
        }
    }

    free(workItem);
    Context = NULL;
}

_Use_decl_annotations_
NTSTATUS
ThreadPoolSubmitWork(
    _In_        PTHREADPOOL      Pool,
    _In_        fnThreadPoolTask Function,
    _Inout_opt_ PVOID            Argument
)
{
    if (!Pool)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!Function)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    PWORK_ITEM workItem = calloc(1, sizeof(WORK_ITEM));
    if (!workItem)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    workItem->ThreadPool = Pool;
    workItem->Function = Function;
    workItem->Argument = Argument;

    BOOL status = TrySubmitThreadpoolCallback(ThreadPoolCallback, workItem, &Pool->CallbackEnvironment);
    if (!status)
    {
        free(workItem);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ThreadPoolWaitForTasks(
    _In_ PTHREADPOOL Pool,
    _In_ ULONG32     Milliseconds
)
{
    if (!Pool)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!IS_VALID_HANDLE(Pool->CompletionEventHandle))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG32 result = WaitForSingleObject(Pool->CompletionEventHandle, Milliseconds);
    if (result == WAIT_OBJECT_0)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

VOID
ThreadPoolDestroy(
    _In_ PTHREADPOOL Pool
)
{
    if (!Pool)
    {
        return;
    }

    CloseThreadpool(Pool->ThreadPool);
    DestroyThreadpoolEnvironment(&Pool->CallbackEnvironment);

    if (IS_VALID_HANDLE(Pool->CompletionEventHandle))
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(Pool->CompletionEventHandle)));
    }

    free(Pool);
}