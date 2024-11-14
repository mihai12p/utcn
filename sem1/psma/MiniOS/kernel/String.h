#ifndef _STRING_H_
#define _STRING_H_

#include "main.h"

int
strncmp(
    _In_ const char* String1,
    _In_ const char* String2,
    _In_ size_t      N
);

int
llutoa(
    _Inout_ char* String,
    _In_    QWORD Number,
    _In_    int   TimeFormat
);

VOID
memncpy(
    _Out_writes_bytes_(Length) PVOID       Destination,
    _In_reads_bytes_(Length)   const PVOID Source,
    _In_                       QWORD       Length
);

VOID
memnset(
    _Out_writes_bytes_(Length) PVOID Destination,
    _In_                       BYTE  Value,
    _In_                       QWORD Length
);

#endif//_STRING_H_