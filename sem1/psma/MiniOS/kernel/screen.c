#include "screen.h"
#include "PIC.h"

static PSCREEN gVideo = (PSCREEN)(0x000B8000 | TERABYTE);

static
VOID
CursorMove(
    _In_ int Row,
    _In_ int Column
)
{
    WORD location = (Row * MAX_COLUMNS) + Column;

    // Cursor Low port
    __outbyte(0x3D4, 0x0F); io_wait();                                    // Sending the cursor low byte to the VGA Controller
    __outbyte(0x3D5, (unsigned char)(location & 0xFF)); io_wait();

    // Cursor High port
    __outbyte(0x3D4, 0x0E); io_wait();                                    // Sending the cursor high byte to the VGA Controller
    __outbyte(0x3D5, (unsigned char)((location >> 8) & 0xFF)); io_wait(); // Char is a 8bit type
}

static
VOID
GetCursorPosition(
    _Out_ WORD* Position
)
{
    *Position = 0;

    __outbyte(0x3D4, 0x0F); io_wait();
    BYTE low = __inbyte(0x3D5);

    __outbyte(0x3D4, 0x0E); io_wait();
    BYTE high = __inbyte(0x3D5);

    *Position = (high << 8) | low;
}

VOID
SetCursorPosition(
    _In_ int Position
)
{
    if (Position > MAX_OFFSET)
    {
        Position = Position % MAX_OFFSET;
    }

    int row = Position / MAX_COLUMNS;
    int col = Position % MAX_COLUMNS;

    CursorMove(row, col);
}

VOID
HelloBoot()
{
    char boot[] = "Hello Boot! Greetings from C...";

    int len = 0;
    while (boot[len] != '\0')
    {
        ++len;
    }

    int i = 0;
    for (i = 0; (i < len) && (i < MAX_OFFSET); ++i)
    {
        gVideo[i].color = 10;
        gVideo[i].c = boot[i];
    }

    SetCursorPosition(MAX_COLUMNS);
}

VOID
ClearScreen()
{
    for (int i = 0; i < MAX_OFFSET; ++i)
    {
        gVideo[i].color = 10;
        gVideo[i].c = ' ';
    }

    CursorMove(0, 0);
}

VOID
PutChar(
    _In_ char C
)
{
    //
    // get current cursor position
    //
    WORD currentPosition = 0;
    GetCursorPosition(&currentPosition);

    //
    // write character and attribute
    //
    gVideo[currentPosition].color = 10;
    gVideo[currentPosition].c = C;

    //
    // update cursor position
    //
    SetCursorPosition(++currentPosition);
}