#include "RTC.h"
#include "PIC.h"

//
// https://wiki.osdev.org/CMOS
//
#define RTC_PORT_INDEX      (0x70)
#define RTC_PORT_DATA       (0x71)

#define RTC_SECOND          (0x00)
#define RTC_MINUTE          (0x02)
#define RTC_HOUR            (0x04)
#define RTC_DAY             (0x07)
#define RTC_MONTH           (0x08)
#define RTC_YEAR            (0x09)
#define RTC_STATUS_A        (0x0A)
#define RTC_STATUS_B        (0x0B)

static
BYTE
BCD2BIN(
    _In_ BYTE Value
)
{
    return (Value & 0x0F) + ((Value / 16) * 10);
}

static
BYTE
IsUpdateInProgress()
{
    __outbyte(RTC_PORT_INDEX, RTC_STATUS_A); io_wait();
    return __inbyte(RTC_PORT_DATA) & 0x80;
}

VOID
GetTime(
    _Out_ PRTC_TIME Time
)
{
    Time->Day = 0;
    Time->Minute = 0;
    Time->Hour = 0;
    Time->Day = 0;
    Time->Month = 0;
    Time->Year = 0;

    //
    // disable interrupts
    //
    _disable();

    while (IsUpdateInProgress());

    __outbyte(RTC_PORT_INDEX, RTC_SECOND); io_wait();
    Time->Second = __inbyte(RTC_PORT_DATA);

    __outbyte(RTC_PORT_INDEX, RTC_MINUTE); io_wait();
    Time->Minute = __inbyte(RTC_PORT_DATA);

    __outbyte(RTC_PORT_INDEX, RTC_HOUR); io_wait();
    Time->Hour = __inbyte(RTC_PORT_DATA);

    __outbyte(RTC_PORT_INDEX, RTC_DAY); io_wait();
    Time->Day = __inbyte(RTC_PORT_DATA);

    __outbyte(RTC_PORT_INDEX, RTC_MONTH); io_wait();
    Time->Month = __inbyte(RTC_PORT_DATA);

    __outbyte(RTC_PORT_INDEX, RTC_YEAR); io_wait();
    Time->Year = __inbyte(RTC_PORT_DATA);

    //
    // enable interrupts
    //
    _enable();

    __outbyte(RTC_PORT_INDEX, RTC_STATUS_B); io_wait();
    BYTE status = __inbyte(RTC_PORT_DATA);
    int isBCDEnabled = !(status & 0x04);
    if (isBCDEnabled)
    {
        Time->Second = BCD2BIN(Time->Second);
        Time->Minute = BCD2BIN(Time->Minute);
        Time->Hour = ((Time->Hour & 0x0F) + (((Time->Hour & 0x70) / 16) * 10)) | (Time->Hour & 0x80);
        Time->Day = BCD2BIN(Time->Day);
        Time->Month = BCD2BIN(Time->Month);
        Time->Year = BCD2BIN(Time->Year);
    }

    int is12HFormatEnabled = !(status & 0x02);
    if (is12HFormatEnabled && (Time->Hour & 0x80))
    {
        Time->Hour = ((Time->Hour & 0x7F) + 12) % 24;
    }
}
