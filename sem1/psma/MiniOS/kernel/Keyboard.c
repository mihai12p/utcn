#include "Keyboard.h"
#include "PIC.h"

static
VOID
KeyboardReadConfig(
    BYTE* KeyboardConfig
)
{
    __outbyte(KBD_CMD_PORT, KBD_READ_CONFIG); io_wait();

    //
    // wait until the output buffer is full
    //
    while ((__inbyte(KBD_CMD_PORT) & 0x01) == 0);
    *KeyboardConfig = __inbyte(KBD_DATA_PORT);
}

static
VOID
KeyboardWriteConfig(
    _In_ BYTE Config
)
{
    //
    // wait until the buffer is empty
    //
    while ((__inbyte(KBD_CMD_PORT) & 0x02) != 0);
    __outbyte(KBD_CMD_PORT, KBD_WRITE_CONFIG); io_wait();

    //
    // wait until the buffer is empty again
    //
    while ((__inbyte(KBD_CMD_PORT) & 0x02) != 0);
    __outbyte(KBD_DATA_PORT, Config); io_wait();
}

VOID
KeyboardInit()
{
    //
    // enable the first PS/2 port (keyboard)
    //
    __outbyte(KBD_CMD_PORT, KBD_ENABLE_PORT1); io_wait();

    BYTE keyboardConfig = 0;
    KeyboardReadConfig(&keyboardConfig);

    //
    // Set bit 0 to enable keyboard interrupt
    //
    keyboardConfig |= 0x01;
    KeyboardWriteConfig(keyboardConfig);
}

VOID
Scancode2Keycode(
    _Out_ PKEYCODE Keycode,
    _In_  BYTE     Scancode,
    _In_  int      IsExtended
)
{
    *Keycode = KEY_UNKNOWN;

    if (IsExtended)
    {
        if (Scancode < (sizeof(gKeyboard_scancode_ext) / sizeof(gKeyboard_scancode_ext[0])))
        {
            *Keycode = gKeyboard_scancode_ext[Scancode];
        }
    }
    else
    {
        if (Scancode < (sizeof(gKeyboard_scancode_std) / sizeof(gKeyboard_scancode_std[0])))
        {
            *Keycode = gKeyboard_scancode_std[Scancode];
        }
    }
}