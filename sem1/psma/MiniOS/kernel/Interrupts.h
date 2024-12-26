#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include "main.h"

#pragma pack(push, 1)
//
// https://wiki.osdev.org/Interrupt_Descriptor_Table
//
typedef struct _IDT_ENTRY
{
    WORD  OffsetLow;
    WORD  SegmentSelector;
    BYTE  IST : 3;
    BYTE  Reserved0 : 1;
    BYTE  Reserved1 : 1;
    BYTE  Reserved2 : 3;
    union
    {
        struct
        {
            BYTE  Type : 4;
            BYTE  Reserved3 : 1;
            BYTE  DPL : 2;
            BYTE  Present : 1;
        };
        BYTE Attributes;
    };
    WORD  OffsetMid;
    DWORD OffsetHigh;
    DWORD Reserved4;
} IDT_ENTRY;

typedef struct _IDT_PTR
{
    WORD  Limit;
    QWORD Base;
} IDT_PTR;

typedef struct _CPU_CONTEXT
{
    QWORD rax, rbx, rcx, rdx;
    QWORD rsi, rdi, rsp, rbp;
    QWORD r8, r9, r10, r11, r12, r13, r14, r15;
    QWORD rip;
    WORD  cs, ss, ds, es, fs, gs;
    DWORD rflags;
    QWORD cr0, cr2, cr3, cr4;
    QWORD dr0, dr1, dr2, dr3, dr6, dr7;
} CPU_CONTEXT, * PCPU_CONTEXT;

typedef enum _EXCEPTION_ID
{
    EXCEPTION_DE       = 0,
    EXCEPTION_DB       = 1,
    EXCEPTION_NMI      = 2,
    EXCEPTION_BP       = 3,
    EXCEPTION_OF       = 4,
    EXCEPTION_BR       = 5,
    EXCEPTION_UD       = 6,
    EXCEPTION_NM       = 7,
    EXCEPTION_DF       = 8,
    EXCEPTION_RES1     = 9,
    EXCEPTION_TS       = 10,
    EXCEPTION_NP       = 11,
    EXCEPTION_SS       = 12,
    EXCEPTION_GP       = 13,
    EXCEPTION_PF       = 14,
    EXCEPTION_RES2     = 15,
    EXCEPTION_MF       = 16,
    EXCEPTION_AC       = 17,
    EXCEPTION_MC       = 18,
    EXCEPTION_XMXF     = 19,
    EXCEPTION_VE       = 20,
    EXCEPTION_CP       = 21,
    EXCEPTION_RES3     = 22,
    EXCEPTION_RES4     = 23,
    EXCEPTION_RES5     = 24,
    EXCEPTION_RES6     = 25,
    EXCEPTION_RES7     = 26,
    EXCEPTION_RES8     = 27,
    EXCEPTION_HV       = 28,
    EXCEPTION_VC       = 29,
    EXCEPTION_SX       = 30,
    EXCEPTION_RES9     = 31,
    INTERRUPT_TIMER    = 32,
    INTERRUPT_KB       = 33,
    USER_DEFINED_START = 34,
    INTERRUPT_SP_IRQ7  = 39,
    INTERRUPT_SP_IRQ15 = 47,
    USER_DEFINED_END   = 255
} EXCEPTION_ID;

typedef struct _EXCEPTION_INFO
{
    const char*  ExceptionName;
    EXCEPTION_ID ExceptionId;
} EXCEPTION_INFO;
#pragma pack(pop)

VOID
InterruptHandler(
    _In_ PCPU_CONTEXT Context,
    _In_ QWORD        InterruptIdx,
    _In_ QWORD        ErrorCode,
    _In_ QWORD        Reserved
);

VOID
InitInterrupts();

#endif//_INTERRUPTS_H_