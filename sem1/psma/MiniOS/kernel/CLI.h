#ifndef _CLI_H_
#define _CLI_H_

#include "main.h"
#include "Scancode.h"

VOID
CLI_Init();

VOID
CLI_ProcessInput(
    _In_ KEYCODE Keycode,
    _In_ int     IsKeyReleased
);

#endif//_CLI_H_