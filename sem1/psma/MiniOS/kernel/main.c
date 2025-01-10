#include "main.h"
#include "screen.h"
#include "logging.h"
#include "Interrupts.h"
#include "ATA.h"
#include "APIC.h"

VOID
KernelMain()
{
    InitLogging();

    InitScreen();

    BspInitInterrupts();

    ATA_DetectDevice();

    InitMP();

    while (1);

    __debugbreak();
}

__declspec(noreturn)
VOID
APMain()
{
    extern VOID EnableLAPIC();
    EnableLAPIC();

    extern DWORD GetLAPICId();
    DWORD lapicId = GetLAPICId();

    LogMessage("Hello from CPU "); LogDword(lapicId); LogMessage("\n");

    ApInitInterrupts();

    while (1);

    __debugbreak();
}
