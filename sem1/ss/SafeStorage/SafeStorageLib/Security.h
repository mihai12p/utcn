#ifndef _SECURITY_H_
#define _SECURITY_H_

#include "includes.h"
EXTERN_C_START;

VOID
ResetSecurityContext();

VOID
RecordFailedLogin();

_Must_inspect_result_
BOOL
IsLoginBlocked();

EXTERN_C_END;
#endif//_SECURITY_H_