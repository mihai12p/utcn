#ifndef _SHA256_H_
#define _SHA256_H_

#include "includes.h"
EXTERN_C_START;

#define SHA256_BYTE_SIZE      (32)
#define SHA256_CHAR_SIZE      ((SHA256_BYTE_SIZE) * 2)

//
// https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/Security/HashComputation/cpp/HashComputation.cpp
//
_Must_inspect_result_
NTSTATUS
CalculateSHA256Hash(
    _Out_writes_z_(SHA256_CHAR_SIZE + sizeof(ANSI_NULL)) LPSTR   PasswordHash,
    _In_reads_bytes_(BufferSize)                         PBYTE   Buffer,
    _In_                                                 ULONG32 BufferSize
);

EXTERN_C_END;
#endif//_SHA256_H_