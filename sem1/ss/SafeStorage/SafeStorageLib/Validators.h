#ifndef _VALIDATORS_H_
#define _VALIDATORS_H_

#include "includes.h"
EXTERN_C_START;

_Must_inspect_result_
NTSTATUS
ValidateUsername(
    _In_reads_z_(UsernameLength) LPCSTR Username,
    _In_                         USHORT UsernameLength
);

_Must_inspect_result_
NTSTATUS
ValidatePassword(
    _In_reads_z_(PasswordLength) LPCSTR Password,
    _In_                         USHORT PasswordLength
);

_Must_inspect_result_
NTSTATUS
ValidateFile(
    _In_                         HANDLE FileHandle,
    _In_reads_z_(FilePathLength) LPCSTR FilePath,
    _In_                         USHORT FilePathLength
);

EXTERN_C_END;
#endif//_VALIDATORS_H_