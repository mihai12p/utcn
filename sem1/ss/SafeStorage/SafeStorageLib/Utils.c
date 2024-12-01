#include "Utils.h"

VOID
FreeAnsiString(
    _Inout_ PANSI_STRING String
)
{
    if (!String)
    {
        return;
    }

    if (String->Buffer)
    {
        free(String->Buffer);
        String->Buffer = NULL;
    }

    String->Length = 0;
    String->MaximumLength = 0;
}

_Use_decl_annotations_
DOUBLE
GetElapsedSeconds(
    _In_ LARGE_INTEGER Start,
    _In_ LARGE_INTEGER End
)
{
    static LARGE_INTEGER frequency = { 0 };
    if (!frequency.QuadPart)
    {
        QueryPerformanceFrequency(&frequency);
    }

    return (End.QuadPart - Start.QuadPart) / (DOUBLE)frequency.QuadPart;
}

_Use_decl_annotations_
NTSTATUS
Hex2Str(
    _Out_writes_opt_z_(2 * HexLength + sizeof(ANSI_NULL)) LPSTR   String,
    _In_reads_bytes_(HexLength)                           PBYTE   Hex,
    _In_                                                  ULONG32 HexLength
)
{
    if (!String)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    String[0] = ANSI_NULL;

    if (!Hex)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (HexLength == 0)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    CHAR hexDigits[17] = "0123456789ABCDEF";
    for (ULONG32 i = 0; i < HexLength; ++i)
    {
        String[i * 2] = hexDigits[Hex[i] >> 4];
        String[i * 2 + 1] = hexDigits[Hex[i] & 0x0F];
    }

    String[HexLength * 2] = ANSI_NULL;

    return STATUS_SUCCESS;
}