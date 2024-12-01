#include "Security.h"
#include "Utils.h"

#define MAX_FAILED_ATTEMPTS     (5)
#define MAX_SECONDS             (1)

typedef struct _SECURITY_CONTEXT
{
    LARGE_INTEGER Attempts[MAX_FAILED_ATTEMPTS];
    BYTE          Count;
} SECURITY_CONTEXT;

static SECURITY_CONTEXT gSecurityContext = { 0 };

VOID
ResetSecurityContext()
{
    gSecurityContext.Count = 0;
    for (BYTE i = 0; i < MAX_FAILED_ATTEMPTS; ++i)
    {
        gSecurityContext.Attempts[i].QuadPart = 0;
    }
}

VOID
RecordFailedLogin()
{
    if (gSecurityContext.Count < MAX_FAILED_ATTEMPTS)
    {
        (VOID)QueryPerformanceCounter(&gSecurityContext.Attempts[gSecurityContext.Count]);
        ++gSecurityContext.Count;
        return;
    }

    for (BYTE i = 1; i < MAX_FAILED_ATTEMPTS; ++i)
    {
        gSecurityContext.Attempts[i - 1] = gSecurityContext.Attempts[i];
    }
    (VOID)QueryPerformanceCounter(&gSecurityContext.Attempts[MAX_FAILED_ATTEMPTS - 1]);
}

_Use_decl_annotations_
BOOL
IsLoginBlocked()
{
    if (gSecurityContext.Count < MAX_FAILED_ATTEMPTS)
    {
        return FALSE;
    }

    LARGE_INTEGER now = { 0 };
    (VOID)QueryPerformanceCounter(&now);

    DOUBLE elapsedSeconds = GetElapsedSeconds(gSecurityContext.Attempts[0], now);
    return (elapsedSeconds <= MAX_SECONDS);
}