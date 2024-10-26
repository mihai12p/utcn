#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "main.h"

VOID
InitLogging();

VOID
LogWord(
    WORD Value
);

VOID
LogDword(
    DWORD Value
);

VOID
LogQword(
    QWORD Value
);

VOID
LogMessage(
    const char* Message
);


#endif // _LOGGING_H_