#include "main.h"
#include "screen.h"
#include "logging.h"
#include "Interrupts.h"
#include "ATA.h"

VOID
KernelMain()
{
    InitLogging();

    InitScreen();

    InitInterrupts();

    ATA_DetectDevice();

    while (1);

    __debugbreak();
}
