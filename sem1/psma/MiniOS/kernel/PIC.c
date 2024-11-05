#include "PIC.h"
#include "logging.h"

__forceinline
VOID
io_wait()
{
    //
    // output into an unused port (POST = Power On Self Test)
    //
    __outbyte(POST, 0);
}

//
// https://wiki.osdev.org/8259_PIC
//
VOID
PIC_remap(
    _In_ BYTE MasterOffset,
    _In_ BYTE SlaveOffset
)
{
    //
    // save masks
    //
    BYTE pic1Data = __inbyte(PIC1_DATA);
    BYTE pic2Data = __inbyte(PIC2_DATA);

    //
    // start initialization
    //
    __outbyte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    __outbyte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    __outbyte(PIC1_DATA, MasterOffset); io_wait();
    __outbyte(PIC2_DATA, SlaveOffset); io_wait();

    //
    // tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    //
    __outbyte(PIC1_DATA, 4); io_wait();

    //
    // tell Slave PIC its cascade identity (0000 0010)
    //
    __outbyte(PIC2_DATA, 2); io_wait();
    __outbyte(PIC1_DATA, ICW4_8086); io_wait();
    __outbyte(PIC2_DATA, ICW4_8086); io_wait();

    //
    // restore saved masks
    //
    __outbyte(PIC1_DATA, pic1Data); io_wait();
    __outbyte(PIC2_DATA, pic2Data); io_wait();
}

VOID
IRQ_set_mask(
    _In_ BYTE IRQLine
)
{
    WORD port;

    if (IRQLine < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        IRQLine -= 8;
    }

    BYTE value = __inbyte(port) | (1 << IRQLine);
    __outbyte(port, value); io_wait();
}

VOID
IRQ_clear_mask(
    _In_ BYTE IRQLine
)
{
    WORD port;

    if (IRQLine < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        IRQLine -= 8;
    }

    BYTE value = __inbyte(port) & ~(1 << IRQLine);
    __outbyte(port, value); io_wait();
}

VOID
PIC_sendEOI(
    _In_ BYTE IRQLine
)
{
    if (IRQLine >= 8)
    {
        __outbyte(PIC2_COMMAND, PIC_EOI); io_wait();
    }

    __outbyte(PIC1_COMMAND, PIC_EOI); io_wait();
}

VOID
PIT_init(
    _In_ DWORD Frequency
)
{
    WORD divisor = (WORD)(((DWORD)(PIT_BASE_FREQ / Frequency)) & 0xFFFF);

    //
    // https://wiki.osdev.org/Programmable_Interval_Timer
    //
    __outbyte(PIT_COMMAND, 0x36); io_wait();
    __outbyte(PIT_CHANNEL0, (BYTE)(divisor & 0xFF)); io_wait();
    __outbyte(PIT_CHANNEL0, (BYTE)((divisor >> 8) & 0xFF)); io_wait();
}