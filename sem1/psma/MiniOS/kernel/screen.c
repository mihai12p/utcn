#include "screen.h"
#include "PIC.h"
#include "String.h"

static PSCREEN gVideo = (PSCREEN)(0x000B8000);
static SCREEN_BUFFER gCLIScreenBuffer = { 0 };
static SCREEN_BUFFER gEditScreenBuffer = { 0 };

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
InitScreen()
{
    memnset(gCLIScreenBuffer.CursorCharacter, 0, sizeof(gCLIScreenBuffer.CursorCharacter));
    gCLIScreenBuffer.CursorPosition = 0;

    memnset(gEditScreenBuffer.CursorCharacter, 0, sizeof(gEditScreenBuffer.CursorCharacter));
    gEditScreenBuffer.CursorPosition = 0;

    ClearScreen();

    for (int i = 0; i < MAX_OFFSET; ++i)
    {
        gEditScreenBuffer.CursorCharacter[i].color = 10;
        gEditScreenBuffer.CursorCharacter[i].c = ' ';
    }

    CursorMove(0, 0);
}

VOID
SaveScreen(
    _In_ int IsEditScreen
)
{
    if (IsEditScreen)
    {
        memncpy(gEditScreenBuffer.CursorCharacter, gVideo, sizeof(gEditScreenBuffer.CursorCharacter));
        GetCursorPosition(&gEditScreenBuffer.CursorPosition);
    }
    else
    {
        memncpy(gCLIScreenBuffer.CursorCharacter, gVideo, sizeof(gCLIScreenBuffer.CursorCharacter));
        GetCursorPosition(&gCLIScreenBuffer.CursorPosition);
    }
}

VOID
RestoreScreen(
    _In_ int IsEditScreen
)
{
    if (IsEditScreen)
    {
        memncpy(gVideo, gEditScreenBuffer.CursorCharacter, sizeof(gEditScreenBuffer.CursorCharacter));
        SetCursorPosition(gEditScreenBuffer.CursorPosition);
    }
    else
    {
        memncpy(gVideo, gCLIScreenBuffer.CursorCharacter, sizeof(gCLIScreenBuffer.CursorCharacter));
        SetCursorPosition(gCLIScreenBuffer.CursorPosition);
    }
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
    if (currentPosition >= MAX_OFFSET)
    {
        return;
    }

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

VOID
RemoveChar()
{
    //
    // get current cursor position
    //
    WORD currentPosition = 0;
    GetCursorPosition(&currentPosition);
    if (currentPosition == 0)
    {
        return;
    }

    //
    // write character and attribute
    //
    gVideo[currentPosition - 1].color = 10;
    gVideo[currentPosition - 1].c = '\0';

    //
    // update cursor position
    //
    SetCursorPosition(--currentPosition);
}

static
VOID
EraseCurrentLineContents(
    _In_ WORD* NewPosition,
    _In_ WORD  CurrentPosition
)
{
    int currentRowIdx = CurrentPosition / MAX_COLUMNS;
    *NewPosition = CurrentPosition = currentRowIdx * MAX_COLUMNS;
    for (int i = 0; i < MAX_COLUMNS; ++i)
    {
        gVideo[CurrentPosition + i].color = 10;
        gVideo[CurrentPosition + i].c = ' ';
    }
}

VOID
PutString(
    _In_ char* Buffer,
    _In_ int   EraseLineContents
)
{
    //
    // get current cursor position
    //
    WORD currentPosition = 0;
    GetCursorPosition(&currentPosition);

    if (EraseLineContents)
    {
        EraseCurrentLineContents(&currentPosition, currentPosition);
    }

    //
    // write characters and attribute
    //
    int bufferSize = (int)strlen(Buffer);
    for (int i = 0; i < bufferSize; ++i)
    {
        gVideo[currentPosition + i].color = 10;
        gVideo[currentPosition + i].c = Buffer[i];
    }

    //
    // update cursor position
    //
    SetCursorPosition(currentPosition + bufferSize);
}

static
VOID
MoveCursor(
    _In_ int Row,
    _In_ int Column
)
{
    //
    // get current cursor position
    //
    WORD currentPosition = 0;
    GetCursorPosition(&currentPosition);

    int currentRowIdx = currentPosition / MAX_COLUMNS;
    int currentColumnIdx = currentPosition % MAX_COLUMNS;
    if (currentRowIdx + Row == -1 || currentRowIdx + Row == MAX_LINES ||
        currentColumnIdx + Column == -1 || currentColumnIdx + Column == MAX_COLUMNS)
    {
        return;
    }

    //
    // update cursor position
    //
    SetCursorPosition((currentRowIdx + Row) * MAX_COLUMNS + currentColumnIdx + Column);
}

VOID
MoveCursorNewLine()
{
    //
    // get current cursor position
    //
    WORD currentPosition = 0;
    GetCursorPosition(&currentPosition);

    int currentRowIdx = currentPosition / MAX_COLUMNS;
    if (currentRowIdx + 1 == MAX_LINES)
    {
        return;
    }

    //
    // update cursor position
    //
    SetCursorPosition((currentRowIdx + 1) * MAX_COLUMNS);
}

VOID
MoveCursorLineDown()
{
    MoveCursor(1, 0);
}

VOID
MoveCursorLineUp()
{
    MoveCursor(-1, 0);
}

VOID
MoveCursorColumnRight()
{
    MoveCursor(0, 1);
}

VOID
MoveCursorColumnLeft()
{
    MoveCursor(0, -1);
}