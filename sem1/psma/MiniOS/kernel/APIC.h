#ifndef _APIC_H_
#define _APIC_H_

#include "main.h"

#define MAX_SUPPORTED_CPUS        (32)

typedef struct _LAPIC_INFO
{
    BYTE         BSPLAPICId;
    BYTE         LapicIds[MAX_SUPPORTED_CPUS];
    BYTE         LapicIdsCount;
    volatile int IsReady[MAX_SUPPORTED_CPUS];
} LAPIC_INFO, * PLAPIC_INFO;

VOID
InitMP();

VOID
MarkAllCPUAvailable();

VOID
MarkCPUReady(
    _In_ DWORD LAPICId
);

_Must_inspect_result_
int
IsCPUReady(
    _In_ DWORD LAPICId
);

#endif//_APIC_H_