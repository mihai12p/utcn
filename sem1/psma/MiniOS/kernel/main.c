#include "main.h"
#include "screen.h"
#include "logging.h"
#include "Interrupts.h"

VOID
KernelMain()
{
    InitInterrupts();

    ClearScreen();

    InitLogging();

    LogMessage("Logging initialized!\r\n\r\n");

    HelloBoot();

    while (1);

    __debugbreak();
    
    // TODO!!! Timer programming

    // TODO!!! Implement a simple console

    // TODO!!! read disk sectors using PIO mode ATA

    // TODO!!! Memory management: virtual, physical and heap memory allocators
}
