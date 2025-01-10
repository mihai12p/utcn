#include "Spinlock.h"

//
// https://wiki.osdev.org/Spinlock
//

extern QWORD GetRFlags();
extern VOID SetRFlags(_In_ QWORD Flags);

_Use_decl_annotations_
int 
SpinlockAcquire(
    _Inout_ PSPINLOCK Lock
)
{
    if (!Lock)
    {
        return SPINLOCK_ERROR_INVALID;
    }

    QWORD flags = GetRFlags();
    
    //
    // Check if interrupts were enabled
    //
    int interruptsWereEnabled = (flags & RFLAGS_IF_BIT) != 0;
    if (interruptsWereEnabled)
    {
        //
        // Disable interrupts
        //
        SetRFlags(flags & ~RFLAGS_IF_BIT);
    }

    while (1)
    {
        //
        // Try to acquire the lock
        //
        if (_InterlockedCompareExchange(&Lock->Locked, 1, 0) == 0)
        {
            return interruptsWereEnabled;
        }

        _mm_pause();
    }
}

_Use_decl_annotations_
VOID 
SpinlockRelease(
    _Inout_ PSPINLOCK Lock,
    _In_    int       EnableInterrupts
)
{
    //
    // Release the lock
    //
    _InterlockedExchange(&Lock->Locked, 0);

    if (EnableInterrupts > 0)
    {
        //
        // Re-enable interrupts if they were enabled before
        //
        QWORD flags = GetRFlags();
        SetRFlags(flags | RFLAGS_IF_BIT);
    }
}