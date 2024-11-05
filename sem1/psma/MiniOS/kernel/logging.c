#include "logging.h"
#include "PIC.h"

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
}

int
IsLineReady()
{
    return (__inbyte(0x3FD) & 0x60) == 0x60;
}

VOID
LogHex(
    QWORD Value,
    BYTE  DigitsCount
)
{
    const char* hexDigits = "0123456789abcdef";
    char buffer[16 + 1] = { 0 };

    for (int i = 0; i < DigitsCount; ++i)
    {
        buffer[DigitsCount - 1 - i] = hexDigits[Value & 0x0F];
        Value >>= 4;
    }

    for (int i = 0; buffer[i]; ++i)
    {
        while (!IsLineReady()) {}
        __outbyte(0x3F8, buffer[i]); io_wait();
    }
}

VOID
LogWord(
    WORD Value
)
{
    LogHex(Value, 4);
}

VOID
LogDword(
    DWORD Value
)
{
    LogHex(Value, 8);
}

VOID
LogQword(
    QWORD Value
)
{
    LogHex(Value, 16);
}

VOID
LogMessage(
    const char* Message
)
{
    for (int i = 0; Message[i]; ++i)
    {
        while (!IsLineReady()) {}
        __outbyte(0x3F8, Message[i]); io_wait();
    }
}