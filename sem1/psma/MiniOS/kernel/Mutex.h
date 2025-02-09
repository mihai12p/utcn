#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "main.h"
#include "Thread.h"

typedef struct _MUTEX
{
    PTHREAD    Owner;
    LIST_ENTRY WaitQueue;
    SPINLOCK   Lock;
} MUTEX, * PMUTEX;

VOID
MutexInit(
    _Out_ PMUTEX Mutex
);

VOID
MutexAcquire(
    _Inout_ PMUTEX  Mutex,
    _Inout_ PTHREAD CurrentThread
);

VOID
MutexRelease(
    _Inout_ PMUTEX  Mutex,
    _In_    PTHREAD CurrentThread
);

#endif//_MUTEX_H_