#include "main.h"
#include "screen.h"
#include "logging.h"
#include "Interrupts.h"

void KernelMain()
{
    //__magic();    // break into BOCHS

    __enableSSE();  // only for demo; in the future will be called from __init.asm

    InitInterrupts();

    ClearScreen();

    InitLogging();

    LogMessage("Logging initialized!\r\n\r\n");

    HelloBoot();

    __debugbreak();

    // TODO!!! PIC programming; see http://www.osdever.net/tutorials/view/programming-the-pic
    
    // TODO!!! Timer programming

    // TODO!!! Keyboard programming

    // TODO!!! Implement a simple console

    // TODO!!! read disk sectors using PIO mode ATA

    // TODO!!! Memory management: virtual, physical and heap memory allocators
}
