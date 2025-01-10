#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "main.h"

#define SPINLOCK_SUCCESS            ( 0)
#define SPINLOCK_ERROR_TIMEOUT      (-1)
#define SPINLOCK_ERROR_INVALID      (-2)

typedef struct _SPINLOCK
{
    volatile LONG Locked;
} SPINLOCK, * PSPINLOCK;

_Acquires_exclusive_lock_(Lock)
_Must_inspect_result_
int
SpinlockAcquire(
    _Inout_ PSPINLOCK Lock
);

_Releases_exclusive_lock_(Lock)
VOID
SpinlockRelease(
    _Inout_ PSPINLOCK Lock,
    _In_    int       EnableInterrupts
);

#endif//_SPINLOCK_H_