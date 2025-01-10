#include "logging.h"
#include "PIC.h"
#include "Spinlock.h"

VOID
InitLogging()
{
    WORD divisor = 4;

    __outbyte(0x3FB, 0x80); io_wait();
    __outbyte(0x3F8, (divisor >> 8) & 0x00FF); io_wait();
    __outbyte(0x3F8, divisor & 0x00FF); io_wait();

    __outbyte(0x3FB, 0x03); io_wait();

    __outbyte(0x3FA, 0xC7); io_wait();

    __outbyte(0x3FC, 0x03); io_wait();

    LogMessage("Logging initialized!\r\n\r\n");
}

int
IsLineReady()
{
    return (__inbyte(0x3FD) & 0x60) == 0x60;
}


static SPINLOCK gMessageLock = { 0 };

VOID
LogMessage(
    _In_ const char* Message
)
{
    int status = SpinlockAcquire(&gMessageLock);
    if (status < 0)
    {
        return;
    }

    for (int i = 0; Message[i]; ++i)
    {
        while (!IsLineReady()) {}
        __outbyte(0x3F8, Message[i]); io_wait();
    }

    SpinlockRelease(&gMessageLock, status);
}

static
VOID
LogHex(
    _In_ QWORD Value,
    _In_ BYTE  DigitsCount
)
{
    const char* hexDigits = "0123456789abcdef";
    char buffer[16 + 1] = { 0 };

    for (int i = 0; i < DigitsCount; ++i)
    {
        buffer[DigitsCount - 1 - i] = hexDigits[Value & 0x0F];
        Value >>= 4;
    }

    LogMessage(buffer);
}

VOID
LogByte(
    _In_ BYTE Value
)
{
    LogHex(Value, 2);
}

VOID
LogWord(
    _In_ WORD Value
)
{
    LogHex(Value, 4);
}

VOID
LogDword(
    _In_ DWORD Value
)
{
    LogHex(Value, 8);
}

VOID
LogQword(
    _In_ QWORD Value
)
{
    LogHex(Value, 16);
}