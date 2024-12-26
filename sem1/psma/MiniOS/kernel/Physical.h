#ifndef _PHYSICAL_H_
#define _PHYSICAL_H_

#include "main.h"

_Must_inspect_result_
int
FrameAlloc(
    _Out_ PQWORD FrameIdx,
    _In_  DWORD  FrameCount
);

VOID
FrameFree(
    _In_ QWORD FrameIdx,
    _In_ DWORD FrameCount
);

_Must_inspect_result_
int
MapPage(
    _In_ QWORD PhysicalAddr,
    _In_ QWORD VirtualAddress,
    _In_ QWORD Flags
);

#endif//_PHYSICAL_H_