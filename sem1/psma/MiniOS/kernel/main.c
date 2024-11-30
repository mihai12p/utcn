#include "main.h"
#include "screen.h"
#include "logging.h"
#include "Interrupts.h"
#include "ATA.h"

VOID
KernelMain()
{
    InitScreen();

    InitLogging();

    LogMessage("Logging initialized!\r\n\r\n");

    InitInterrupts();

    ATA_DetectDevice();

    while (1);

    __debugbreak();

    // TODO!!! read disk sectors using PIO mode ATA

    // TODO!!! Memory management: virtual, physical and heap memory allocators
}
