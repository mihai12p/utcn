#ifndef _UTILS_H_
#define _UTILS_H_

#include "includes.h"
EXTERN_C_START;

VOID
FreeAnsiString(
    _Inout_ PANSI_STRING String
);

_Must_inspect_result_
DOUBLE
GetElapsedSeconds(
    _In_ LARGE_INTEGER Start,
    _In_ LARGE_INTEGER End
);

_Must_inspect_result_
NTSTATUS
Hex2Str(
    _Out_writes_opt_z_(2 * HexLength + sizeof(ANSI_NULL)) LPSTR   String,
    _In_reads_bytes_(HexLength)                           PBYTE   Hex,
    _In_                                                  ULONG32 HexLength
);

EXTERN_C_END;
#endif//_UTILS_H_