#include "Interrupts.h"
#include "logging.h"
#include "PIC.h"
#include "Keyboard.h"
#include "CLI.h"
#include "screen.h"
#include "TestSynchronization.h"

#define IDT_MAX_DESCRIPTORS    (256)

extern PVOID gISR_stub_table[];
extern VOID WriteLAPICRegister(_In_ DWORD LAPICRegister, _In_ DWORD Value);

_Alignas(16) static IDT_ENTRY gIDT[IDT_MAX_DESCRIPTORS] = { 0 };
static IDT_PTR                gIDTR = { 0 };

static int gNextIsExtended = 0;

//
// https://wiki.osdev.org/Exceptions
//
static EXCEPTION_INFO gExceptionInfo[IDT_MAX_DESCRIPTORS] = {
//    ExceptionName                                 ExceptionId
    { "Division Error (#DE)",                       EXCEPTION_DE        }, // 0
    { "Debug (#DB)",                                EXCEPTION_DB        }, // 1
    { "Non-maskable Interrupt",                     EXCEPTION_NMI       }, // 2
    { "Breakpoint (#BP)",                           EXCEPTION_BP        }, // 3
    { "Overflow (#OF)",                             EXCEPTION_OF        }, // 4
    { "Bound Range Exceeded (#BR)",                 EXCEPTION_BR        }, // 5
    { "Invalid Opcode (#UD)",                       EXCEPTION_UD        }, // 6
    { "Device Not Available (#NM)",                 EXCEPTION_NM        }, // 7
    { "Double Fault (#DF)",                         EXCEPTION_DF        }, // 8
    { "Coprocessor Segment Overrun",                EXCEPTION_RES1      }, // 9 (reserved)
    { "Invalid TSS (#TS)",                          EXCEPTION_TS        }, // 10
    { "Segment Not Present (#NP)",                  EXCEPTION_NP        }, // 11
    { "Stack-Segment Fault (#SS)",                  EXCEPTION_SS        }, // 12
    { "General Protection Fault (#GP)",             EXCEPTION_GP        }, // 13
    { "Page Fault (#PF)",                           EXCEPTION_PF        }, // 14
    { "Reserved",                                   EXCEPTION_RES2      }, // 15
    { "x87 Floating-Point Exception (#MF)",         EXCEPTION_MF        }, // 16
    { "Alignment Check (#AC)",                      EXCEPTION_AC        }, // 17
    { "Machine Check (#MC)",                        EXCEPTION_MC        }, // 18
    { "SIMD Floating-Point Exception (#XM#XF)",     EXCEPTION_XMXF      }, // 19
    { "Virtualization Exception (#VE)",             EXCEPTION_VE        }, // 20
    { "Control Protection Exception (#CP)",         EXCEPTION_CP        }, // 21
    { "Reserved",                                   EXCEPTION_RES3      }, // 22
    { "Reserved",                                   EXCEPTION_RES4      }, // 23
    { "Reserved",                                   EXCEPTION_RES5      }, // 24
    { "Reserved",                                   EXCEPTION_RES6      }, // 25
    { "Reserved",                                   EXCEPTION_RES7      }, // 26
    { "Reserved",                                   EXCEPTION_RES8      }, // 27
    { "Hypervisor Injection Exception (#HV)",       EXCEPTION_HV        }, // 28 (Intel-specific)
    { "VMM Communication Exception (#VC)",          EXCEPTION_VC        }, // 29 (Intel-specific)
    { "Security Exception (#SX)",                   EXCEPTION_SX        }, // 30
    { "Reserved",                                   EXCEPTION_RES9      }, // 31
    { "Reserved",                                   INTERRUPT_TIMER     }, // 32
    { "Reserved",                                   INTERRUPT_KB        }, // 33
    { "Reserved",                                   USER_DEFINED_START  }, // 34
    { "Reserved",                                   USER_DEFINED_START  }, // 35
    { "Reserved",                                   USER_DEFINED_START  }, // 36
    { "Reserved",                                   USER_DEFINED_START  }, // 37
    { "Reserved",                                   USER_DEFINED_START  }, // 38
    { "Reserved",                                   INTERRUPT_SP_IRQ7   }, // 39
    { "Reserved",                                   USER_DEFINED_START  }, // 40
    { "Reserved",                                   USER_DEFINED_START  }, // 41
    { "Reserved",                                   USER_DEFINED_START  }, // 42
    { "Reserved",                                   USER_DEFINED_START  }, // 43
    { "Reserved",                                   USER_DEFINED_START  }, // 44
    { "Reserved",                                   USER_DEFINED_START  }, // 45
    { "Reserved",                                   USER_DEFINED_START  }, // 46
    { "Reserved",                                   INTERRUPT_SP_IRQ15  }, // 47
    { "Reserved",                                   USER_DEFINED_START  }, // 48
    { "Reserved",                                   USER_DEFINED_START  }, // 49
    { "Reserved",                                   USER_DEFINED_START  }, // 50
    { "Reserved",                                   USER_DEFINED_START  }, // 51
    { "Reserved",                                   USER_DEFINED_START  }, // 52
    { "Reserved",                                   USER_DEFINED_START  }, // 53
    { "Reserved",                                   USER_DEFINED_START  }, // 54
    { "Reserved",                                   USER_DEFINED_START  }, // 55
    { "Reserved",                                   USER_DEFINED_START  }, // 56
    { "Reserved",                                   USER_DEFINED_START  }, // 57
    { "Reserved",                                   USER_DEFINED_START  }, // 58
    { "Reserved",                                   USER_DEFINED_START  }, // 59
    { "Reserved",                                   USER_DEFINED_START  }, // 60
    { "Reserved",                                   USER_DEFINED_START  }, // 61
    { "Reserved",                                   USER_DEFINED_START  }, // 62
    { "Reserved",                                   USER_DEFINED_START  }, // 63
    { "Reserved",                                   USER_DEFINED_START  }, // 64
    { "Reserved",                                   USER_DEFINED_START  }, // 65
    { "Reserved",                                   USER_DEFINED_START  }, // 66
    { "Reserved",                                   USER_DEFINED_START  }, // 67
    { "Reserved",                                   USER_DEFINED_START  }, // 68
    { "Reserved",                                   USER_DEFINED_START  }, // 69
    { "Reserved",                                   USER_DEFINED_START  }, // 70
    { "Reserved",                                   USER_DEFINED_START  }, // 71
    { "Reserved",                                   USER_DEFINED_START  }, // 72
    { "Reserved",                                   USER_DEFINED_START  }, // 73
    { "Reserved",                                   USER_DEFINED_START  }, // 74
    { "Reserved",                                   USER_DEFINED_START  }, // 75
    { "Reserved",                                   USER_DEFINED_START  }, // 76
    { "Reserved",                                   USER_DEFINED_START  }, // 77
    { "Reserved",                                   USER_DEFINED_START  }, // 78
    { "Reserved",                                   USER_DEFINED_START  }, // 79
    { "Synchronized Print Testcase IPI",            INTERRUPT_IPI_T1    }, // 80
    { "Synchronized Linked List Testcase IPI",      INTERRUPT_IPI_T2    }, // 81
};

static
VOID
PrintTrapInfo(
    PCPU_CONTEXT Context,
    QWORD        InterruptIdx,
    DWORD        ErrorCode
)
{
    EXCEPTION_INFO exceptionInfo = gExceptionInfo[InterruptIdx];

    LogMessage("Trap information:\r\n");
    LogMessage("---------------\r\n");
    LogMessage(exceptionInfo.ExceptionName);
    LogMessage("\r\n");

    switch (exceptionInfo.ExceptionId)
    {
    case EXCEPTION_DF:
    case EXCEPTION_TS:
    case EXCEPTION_NP:
    case EXCEPTION_SS:
    case EXCEPTION_GP:
    case EXCEPTION_PF:
    case EXCEPTION_AC:
    case EXCEPTION_CP:
    case EXCEPTION_VC:
    case EXCEPTION_SX:
    {
        LogMessage("Error Code: ");
        LogDword(ErrorCode);
        LogMessage("\r\n");
        break;
    }
    default:
    {
        break;
    }
    }

    switch (InterruptIdx)
    {
    case EXCEPTION_DB:
    {
        LogMessage("dr0="); LogQword(Context->dr0);
        LogMessage(" dr1="); LogQword(Context->dr1);
        LogMessage(" dr2="); LogQword(Context->dr2);
        LogMessage("\r\n");
        LogMessage("dr3="); LogQword(Context->dr3);
        LogMessage(" dr6="); LogQword(Context->dr6);
        LogMessage(" dr7="); LogQword(Context->dr7);
        LogMessage("\r\n");
        break;
    }
    case EXCEPTION_PF:
    {
        LogMessage("Faulting Address: ");
        LogQword(Context->cr2);
        LogMessage("\r\n");
        break;
    }
    default:
    {
        break;
    }
    }
    LogMessage("\r\n");
}

static
VOID
PrintRegisters(
    PCPU_CONTEXT Context
)
{
    LogMessage("Registers:\r\n");
    LogMessage("---------------\r\n");
    LogMessage("rax="); LogQword(Context->rax);
    LogMessage(" rbx="); LogQword(Context->rbx);
    LogMessage(" rcx="); LogQword(Context->rcx);
    LogMessage("\r\n");
    LogMessage("rdx="); LogQword(Context->rdx);
    LogMessage(" rsi="); LogQword(Context->rsi);
    LogMessage(" rdi="); LogQword(Context->rdi);
    LogMessage("\r\n");
    LogMessage("rip="); LogQword(Context->rip);
    LogMessage(" rsp="); LogQword(Context->rsp);
    LogMessage(" rbp="); LogQword(Context->rbp);
    LogMessage("\r\n");
    LogMessage("r8="); LogQword(Context->r8);
    LogMessage(" r9="); LogQword(Context->r9);
    LogMessage(" r10="); LogQword(Context->r10);
    LogMessage("\r\n");
    LogMessage("r11="); LogQword(Context->r11);
    LogMessage(" r12="); LogQword(Context->r12);
    LogMessage(" r13="); LogQword(Context->r13);
    LogMessage("\r\n");
    LogMessage("r14="); LogQword(Context->r14);
    LogMessage(" r15="); LogQword(Context->r15);
    LogMessage("\r\n");
    LogMessage("cs="); LogWord(Context->cs);
    LogMessage(" ss="); LogWord(Context->ss);
    LogMessage(" ds="); LogWord(Context->ds);
    LogMessage(" es="); LogWord(Context->es);
    LogMessage(" fs="); LogWord(Context->fs);
    LogMessage(" gs="); LogWord(Context->gs);
    LogMessage(" rfl="); LogDword(Context->rflags);
    LogMessage("\r\n");
    LogMessage("\r\n");
}

static
VOID
PrintStackFrame(
    PCPU_CONTEXT Context
)
{
    LogMessage("Stack:\r\n");
    LogMessage("---------------\r\n");

    QWORD baseAddress = Context->rsp & PAGE_MASK;
    QWORD limitAddress = baseAddress + PAGE_SIZE;

    for (int i = 0; ; ++i)
    {
        QWORD rspAddress = Context->rsp + i * sizeof(QWORD);
        if (rspAddress >= limitAddress)
        {
            break;
        }

        LogQword(rspAddress);
        LogMessage(" ");

        QWORD rspValue = *(QWORD*)(rspAddress);
        LogQword(rspValue);
        LogMessage("\r\n");
    }
    LogMessage("\r\n");
}

static
VOID
HandleInterrupt(
    _Out_ int*         InterruptHandled,
    _In_  PCPU_CONTEXT Context,
    _In_  QWORD        InterruptIdx,
    _In_  QWORD        ErrorCode
)
{
    *InterruptHandled = 0;

    if (InterruptIdx < 32)
    {
        PrintTrapInfo(Context, InterruptIdx, (DWORD)ErrorCode);
        PrintRegisters(Context);
        PrintStackFrame(Context);
    }

    EXCEPTION_INFO exceptionInfo = gExceptionInfo[InterruptIdx];
    switch (exceptionInfo.ExceptionId)
    {
    case EXCEPTION_BP:
    {
        __magic();
        break;
    }
    case INTERRUPT_TIMER:
    {
        PIC_sendEOI(0);
        break;
    }
    case INTERRUPT_KB:
    {
        BYTE scancode = __inbyte(KBD_DATA_PORT);
        if (scancode == 0xE0 || scancode == 0xE1)
        {
            gNextIsExtended = 1;
            goto __kb_finally;
        }

        int isKeyRelease = (scancode & 0x80) != 0;
        if (isKeyRelease)
        {
            //
            // unset the release bit
            //
            scancode &= 0x7F;
        }

        KEYCODE keycode = KEY_UNKNOWN;
        Scancode2Keycode(&keycode, scancode, gNextIsExtended);
        gNextIsExtended = 0;

        if (keycode == KEY_UNKNOWN)
        {
            goto __kb_finally;
        }

        CLI_ProcessInput(keycode, isKeyRelease);

__kb_finally:
        PIC_sendEOI(1);
        break;
    }
    //
    // https://www.reddit.com/r/osdev/comments/5qqnkq/are_spurious_interrupts_irq_7_a_bad_sign_and_how/
    //
    case INTERRUPT_SP_IRQ7:
    {
        break;
    }
    case INTERRUPT_SP_IRQ15:
    {
        PIC_sendEOI(7);
        break;
    }
    case INTERRUPT_IPI_T1:
    {
        APTestcaseSynchronizedPrint();

        WriteLAPICRegister(LAPIC_EOI_REGISTER, 0);
        break;
    }
    case INTERRUPT_IPI_T2:
    {
        APTestcaseLinkedList();

        WriteLAPICRegister(LAPIC_EOI_REGISTER, 0);
        break;
    }
    default:
    {
        return;
    }
    }

    *InterruptHandled = 1;
}

VOID
InterruptHandler(
    _In_ PCPU_CONTEXT Context,
    _In_ QWORD        InterruptIdx,
    _In_ QWORD        ErrorCode,
    _In_ QWORD        Reserved
)
{
    int interruptHandled = 0;
    HandleInterrupt(&interruptHandled, Context, InterruptIdx, ErrorCode);
    if (interruptHandled)
    {
        return;
    }

    _disable();
    __halt();
}

VOID
BspInitInterrupts()
{
    gIDTR.Limit = (sizeof(IDT_ENTRY) * IDT_MAX_DESCRIPTORS) - 1;
    gIDTR.Base = (QWORD)&gIDT[0];

    for (WORD idx = 0; idx < IDT_MAX_DESCRIPTORS; ++idx)
    {
        QWORD handler = (QWORD)gISR_stub_table[idx];

        gIDT[idx].OffsetLow = OFFSET_LOW(handler);
        gIDT[idx].SegmentSelector = 0x08;
        gIDT[idx].IST = 0;
        gIDT[idx].Reserved0 = 0;
        gIDT[idx].Reserved1 = 0;
        gIDT[idx].Reserved2 = 0;
        gIDT[idx].Type = 0x0E; // 32-bit Interrupt Gate
        gIDT[idx].Reserved3 = 0;
        gIDT[idx].DPL = 0;
        gIDT[idx].Present = 1;
        gIDT[idx].OffsetMid = OFFSET_MID(handler);
        gIDT[idx].OffsetHigh = OFFSET_HIGH(handler);
        gIDT[idx].Reserved4 = 0;
    }

    __lidt(&gIDTR);

    //
    // remap PIC
    //
    PIC_remap(PIC1_OFFSET, PIC2_OFFSET);

    //
    // initialize PIT to 100 Hz
    //
    PIT_init(100);
    KeyboardInit();
    CLI_Init();

    for (BYTE i = 0; i < 2 * 8; ++i)
    {
        IRQ_set_mask(i);
    }

    IRQ_clear_mask(0); // Timer
    IRQ_clear_mask(1); // Keyboard

    _enable();
}

VOID
ApInitInterrupts()
{
    __lidt(&gIDTR);

    _enable();
}