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

typedef struct _SCREEN_BUFFER
{
    SCREEN CursorCharacter[MAX_OFFSET];
    WORD   CursorPosition;
} SCREEN_BUFFER;
#pragma pack(pop)

VOID
InitScreen();

VOID
ClearScreen();

VOID
SaveScreen(
    _In_ int IsEditScreen
);

VOID
RestoreScreen(
    _In_ int IsEditScreen
);

VOID
PutChar(
    _In_ char C
);

VOID
RemoveChar();

VOID
PutString(
    _In_ char* Buffer,
    _In_ int   EraseLineContents
);

VOID
MoveCursorNewLine();

VOID
MoveCursorLineDown();

VOID
MoveCursorLineUp();

VOID
MoveCursorColumnRight();

VOID
MoveCursorColumnLeft();

#endif // _SCREEN_H_