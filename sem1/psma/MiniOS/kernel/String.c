#include "String.h"

int
strncmp(
    _In_ const char* String1,
    _In_ const char* String2,
    _In_ size_t      N
)
{
    while (N && *String1 && (*String1 == *String2))
    {
        ++String1;
        ++String2;
        --N;
    }

    if (N == 0)
    {
        return 0;
    }

    return *(const BYTE*)String1 - *(const BYTE*)String2;
}

static
VOID
SwapCharacters(
    _In_ char *a,
    _In_ char *b
)
{
    if (!a || !b)
    {
        return;
    }

    char temp = *a;
    *a = *b;
    *b = temp;
}

static
VOID
ReverseString(
    _In_ char* String,
    _In_ QWORD StringLength
) 
{
    QWORD start = 0;
    QWORD end = StringLength - 1;
    while (start < end)
    {
        SwapCharacters(String + start, String + end);
        ++start;
        --end;
    }
}

int
llutoa(
    _Inout_ char* String,
    _In_    QWORD Number,
    _In_    int   TimeFormat
) 
{
    int stringLength = 0;

    if (Number == 0)
    {
        String[stringLength++] = '0';
        if (TimeFormat)
        {
            String[stringLength++] = '0';
        }
        String[stringLength] = '\0';

        return stringLength;
    }

    while (Number != 0)
    {
        String[stringLength++] = (Number % 10) + '0';
        Number = Number / 10;
    }

    if (TimeFormat && stringLength == 1)
    {
        String[stringLength++] = '0';
    }

    String[stringLength] = '\0';
    ReverseString(String, stringLength);

    return stringLength;
}

VOID
memncpy(
    _Out_writes_bytes_(Length) PVOID       Destination,
    _In_reads_bytes_(Length)   const PVOID Source,
    _In_                       QWORD       Length
)
{
    PBYTE d = Destination;
    PBYTE s = Source;
    while (Length--)
    {
        *d++ = *s++;
    }
}

VOID
memnset(
    _Out_writes_bytes_(Length) PVOID Destination,
    _In_                       BYTE  Value,
    _In_                       QWORD Length
)
{
    PBYTE d = Destination;
    while (Length--)
    {
        *d++ = Value;
    }
}