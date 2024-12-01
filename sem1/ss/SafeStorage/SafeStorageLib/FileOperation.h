#ifndef _FILE_OPERATION_H_
#define _FILE_OPERATION_H_

#include "includes.h"
EXTERN_C_START;

_Must_inspect_result_
NTSTATUS
ReadLineFromFile(
    _In_                                   HANDLE   FileHandle,
    _Outptr_opt_result_buffer_(*BytesRead) LPSTR*   LineBuffer,
    _Out_opt_                              PULONG32 BytesRead
);

_Must_inspect_result_
NTSTATUS
MyCopyFile(
    _In_ HANDLE SourceFileHandle,
    _In_ HANDLE DestinationFileHandle
);

EXTERN_C_END;
#endif//_FILE_OPERATION_H_