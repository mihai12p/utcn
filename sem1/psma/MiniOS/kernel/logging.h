#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "main.h"

VOID
InitLogging();

VOID
LogByte(
    _In_ BYTE Value
);

VOID
LogWord(
    _In_ WORD Value
);

VOID
LogDword(
    _In_ DWORD Value
);

VOID
LogQword(
    _In_ QWORD Value
);

VOID
LogMessage(
    _In_ const char* Message
);


#endif // _LOGGING_H_