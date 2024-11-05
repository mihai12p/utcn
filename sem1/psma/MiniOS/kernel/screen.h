#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "main.h"

#define MAX_LINES       25
#define MAX_COLUMNS     80
#define MAX_OFFSET      2000 //25 lines * 80 chars

#pragma pack(push, 1)
typedef struct _SCREEN
{
    char c;
    BYTE color;
}SCREEN, *PSCREEN;
#pragma pack(pop)

void HelloBoot();
void ClearScreen();

VOID
PutChar(
    _In_ char C
);

#endif // _SCREEN_H_