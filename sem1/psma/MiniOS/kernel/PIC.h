#ifndef _PIC_H_
#define _PIC_H_

#include "main.h"

//
// PIC IO Ports
//
#define PIC1            (0x20)
#define PIC2            (0xA0)
#define POST            (0x80)

#define PIC1_COMMAND    (PIC1)
#define PIC1_DATA       ((PIC1) + 1)
#define PIC2_COMMAND    (PIC2)
#define PIC2_DATA       ((PIC2) + 1)

//
// Initialization Control Words
//
#define ICW1_INIT       (0x10)
#define ICW1_ICW4       (0x01)

#define ICW4_8086       (0x01)

//
// Vector Offsets
//
#define PIC1_OFFSET     (0x20)
#define PIC2_OFFSET     (0x28)

#define PIC_EOI         (0x20)

#define PIT_BASE_FREQ   (1193181)

#define PIT_COMMAND     (0x43)
#define PIT_CHANNEL0    (0x40)

__forceinline
VOID
io_wait();

VOID
PIC_remap(
    _In_ BYTE MasterOffset,
    _In_ BYTE SlaveOffset
);

VOID
IRQ_set_mask(
    _In_ BYTE IRQLine
);

VOID
IRQ_clear_mask(
    _In_ BYTE IRQLine
);

VOID
PIC_sendEOI(
    _In_ BYTE IRQLine
);

VOID
PIT_init(
    _In_ DWORD Frequency
);

#endif//_PIC_H_