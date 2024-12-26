#ifndef _VIRTUAL_H_
#define _VIRTUAL_H_

#include "main.h"

_Must_inspect_result_
int
PageAlloc(
    _Inout_opt_ PVOID* Page,
    _In_        DWORD  PageCount,
    _In_        QWORD  FrameAddr
);

VOID
PageFree(
    _In_ PVOID PageAddr,
    _In_ DWORD PageCount,
    _In_ int   FreeBacking
);

#endif//_VIRTUAL_H_