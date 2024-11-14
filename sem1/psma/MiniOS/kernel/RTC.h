#ifndef _RTC_H_
#define _RTC_H_

#include "main.h"

typedef struct _RTC_TIME
{
    BYTE Second;
    BYTE Minute;
    BYTE Hour;
    BYTE Day;
    BYTE Month;
    BYTE Year;
} RTC_TIME, * PRTC_TIME;

VOID
GetTime(
    _Out_ PRTC_TIME Time
);

#endif//_RTC_H_