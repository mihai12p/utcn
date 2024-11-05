#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "main.h"
#include "Scancode.h"

//
// PS/2 Controller Ports
//
#define KBD_CMD_PORT        (0x64)
#define KBD_DATA_PORT       (0x60)

//
// PS/2 Commands
//
#define KBD_ENABLE_PORT1    (0xAE)
#define KBD_READ_CONFIG     (0x20)
#define KBD_WRITE_CONFIG    (0x60)

VOID
KeyboardInit();

VOID
Scancode2Keycode(
    _Out_ PKEYCODE Keycode,
    _In_  BYTE     Scancode,
    _In_  int      IsExtended
);

#endif//_KEYBOARD_H_