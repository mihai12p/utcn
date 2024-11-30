#include "CLI.h"
#include "screen.h"
#include "RTC.h"
#include "String.h"
#include "Edit.h"
#include "ATA.h"
#include "logging.h"

#define CLI_COMMAND_BUFFER_SIZE     (2048)
#define MAX_COMMAND_COUNT           (10)
#define MAX_COMMAND_HISTORY_COUNT   (10)
#define MAX_COMMAND_NAME_CHARACTERS (10)

typedef struct _CLI_BUFFER
{
    char  Buffer[CLI_COMMAND_BUFFER_SIZE];
    DWORD BufferSize;
} CLI_BUFFER;

typedef
VOID(* _CLI_COMMAND_FUNC)(
    _In_opt_ char* Arguments
);

typedef struct _CLI_COMMAND
{
    char              Name[MAX_COMMAND_NAME_CHARACTERS];
    _CLI_COMMAND_FUNC Function;
} CLI_COMMAND, * PCLI_COMMAND;

typedef struct _CLI_COMMANDS
{
    CLI_COMMAND Command[MAX_COMMAND_COUNT];
    BYTE        CommandCount;
    CLI_COMMAND CommandHistory[MAX_COMMAND_HISTORY_COUNT];
    int         CommandHistoryCount;
    int         CommandHistoryIndex;
} CLI_COMMANDS;

static CLI_BUFFER   gCLIBuffer = { 0 };
static CLI_BUFFER   gEditBuffer = { 0 };
static CLI_COMMANDS gCommands = { 0 };

static int gIsUppercase = 0;

static
int
IsUppercase()
{
    return gIsUppercase == 1;
}

static
VOID
ToggleUppercase()
{
    gIsUppercase = IsUppercase() ? 0 : 1;
}

static
VOID
GetPreviousCommand()
{
    if (gCommands.CommandHistoryIndex > MAX_COMMAND_HISTORY_COUNT ||
        gCommands.CommandHistoryIndex < 0)
    {
        return;
    }

    if (gCommands.CommandHistoryIndex > 0)
    {
        --gCommands.CommandHistoryIndex;
    }

    gCLIBuffer.BufferSize = 0;
    int commandSize = (int)strlen(gCommands.CommandHistory[gCommands.CommandHistoryIndex].Name);
    for (int i = 0; i < commandSize; ++i)
    {
        gCLIBuffer.Buffer[gCLIBuffer.BufferSize++] = gCommands.CommandHistory[gCommands.CommandHistoryIndex].Name[i];
    }
    PutString(gCommands.CommandHistory[gCommands.CommandHistoryIndex].Name, 1);
}

static
VOID
GetNextCommand()
{
    if (gCommands.CommandHistoryIndex + 1 >= gCommands.CommandHistoryCount)
    {
        return;
    }

    if (gCommands.CommandHistoryIndex + 1 < gCommands.CommandHistoryCount)
    {
        ++gCommands.CommandHistoryIndex;
    }

    gCLIBuffer.BufferSize = 0;
    int commandSize = (int)strlen(gCommands.CommandHistory[gCommands.CommandHistoryIndex].Name);
    for (int i = 0; i < commandSize; ++i)
    {
        gCLIBuffer.Buffer[gCLIBuffer.BufferSize++] = gCommands.CommandHistory[gCommands.CommandHistoryIndex].Name[i];
    }
    PutString(gCommands.CommandHistory[gCommands.CommandHistoryIndex].Name, 1);
}

static
VOID
CLI_SaveCommandInHistory(
    _In_ PCLI_COMMAND Command
)
{
    if (gCommands.CommandHistoryCount >= MAX_COMMAND_HISTORY_COUNT)
    {
        for (int i = 1; i < MAX_COMMAND_HISTORY_COUNT; ++i)
        {
            gCommands.CommandHistory[i - 1] = gCommands.CommandHistory[i];
        }
        --gCommands.CommandHistoryCount;
    }
    gCommands.CommandHistory[gCommands.CommandHistoryCount++] = *Command;
    gCommands.CommandHistoryIndex = gCommands.CommandHistoryCount;
}

static
VOID
CLI_RegisterCommand(
    _In_ const char*       CommandName,
    _In_ _CLI_COMMAND_FUNC CommandFunction
)
{
    if (gCommands.CommandCount >= MAX_COMMAND_COUNT)
    {
        return;
    }

    strcpy(gCommands.Command[gCommands.CommandCount].Name, CommandName);
    gCommands.Command[gCommands.CommandCount].Function = CommandFunction;
    ++gCommands.CommandCount;
}

static
VOID
CLI_FindCommand(
    _Out_ PCLI_COMMAND Command,
    _In_  const char*  CommandName
)
{
    Command->Function = NULL;

    for (BYTE i = 0; i < gCommands.CommandCount; ++i)
    {
        if (strcmp(CommandName, gCommands.Command[i].Name))
        {
            continue;
        }

        *Command = gCommands.Command[i];
        return;
    }
}

static
VOID
CLI_CommandClear(
    _In_opt_ char* Arguments
)
{
    ClearScreen();
}

static
VOID
CLI_CommandTime(
    _In_opt_ char* Arguments
)
{
    int bufferLength = 0;
    char buffer[64] = { 0 };
    llutoa(&buffer[bufferLength], __rdtsc(), 0);
    strcat(buffer, " ticks | ");

    PutString(buffer, 0);

    RTC_TIME currentTime = { 0 };
    GetTime(&currentTime);

    bufferLength = 0;
    bufferLength += llutoa(&buffer[bufferLength], currentTime.Day, 1);
    buffer[bufferLength++] = '/';
    bufferLength += llutoa(&buffer[bufferLength], currentTime.Month, 1);
    buffer[bufferLength++] = '/';
    bufferLength += llutoa(&buffer[bufferLength], currentTime.Year, 1);
    buffer[bufferLength++] = ' ';
    bufferLength += llutoa(&buffer[bufferLength], currentTime.Hour, 1);
    buffer[bufferLength++] = ':';
    bufferLength += llutoa(&buffer[bufferLength], currentTime.Minute, 1);
    buffer[bufferLength++] = ':';
    bufferLength += llutoa(&buffer[bufferLength], currentTime.Second, 1);
    buffer[bufferLength] = '\0';

    PutString(buffer, 0);
    MoveCursorNewLine();
}

static
VOID
CLI_CommandEdit(
    _In_opt_ char* Arguments
)
{
    if (IsInEditMode())
    {
        return;
    }

    SaveScreen(0);
    RestoreScreen(1);
    SetEditMode(1);
}

static
VOID
CLI_CommandPrintMbr(
    _In_opt_ char* Arguments
)
{
    BYTE buffer[ATA_SECTOR_SIZE] = { 0 };
    (VOID)ATA_ReadData(buffer, ATA_PRIMARY, ATA_MASTER, 0, sizeof(buffer));

    for (int i = 0; i < __crt_countof(buffer); i += 16)
    {
        LogDword(i); LogMessage(": ");
        for (int j = 0; j < 16; ++j)
        {
            LogByte(buffer[i + j]); LogMessage(" ");
        }
        LogMessage(" | ");
        for (int j = 0; j < 16; ++j)
        {
            char c = buffer[i + j];
            if (!c)
            {
                c = '.';
            }
            char message[] = { c, '\0' };
            LogMessage(message);
        }
        LogMessage("\n");
    }
}

VOID
CLI_Init()
{
    CLI_RegisterCommand("clear",    CLI_CommandClear);
    CLI_RegisterCommand("cls",      CLI_CommandClear);
    CLI_RegisterCommand("time",     CLI_CommandTime);
    CLI_RegisterCommand("edit",     CLI_CommandEdit);
    CLI_RegisterCommand("printmbr", CLI_CommandPrintMbr);
}

static
VOID
CLI_ExecuteCommand()
{
    CLI_COMMAND command = { 0 };
    CLI_FindCommand(&command, gCLIBuffer.Buffer);
    if (command.Function)
    {
        CLI_SaveCommandInHistory(&command);
        command.Function(NULL);
    }
}

VOID
CLI_ProcessInput(
    _In_ KEYCODE Keycode,
    _In_ int     IsKeyReleased
)
{
    if (gCLIBuffer.BufferSize >= sizeof(gCLIBuffer.Buffer))
    {
        gCLIBuffer.BufferSize = 0;
    }

    if (gEditBuffer.BufferSize >= sizeof(gEditBuffer.Buffer))
    {
        gEditBuffer.BufferSize = 0;
    }

    if (IsKeyReleased && (Keycode != KEY_LSHIFT && Keycode != KEY_RSHIFT))
    {
        return;
    }

    if ((KEY_A <= Keycode && Keycode <= KEY_Z) ||
        (KEY_0 <= Keycode && Keycode <= KEY_9) ||
        Keycode == KEY_SPACE)
    {
        if (IsUppercase() && (KEY_A <= Keycode && Keycode <= KEY_Z))
        {
            Keycode -= ' ';
        }

        if (IsInEditMode())
        {
            gEditBuffer.Buffer[gEditBuffer.BufferSize++] = (char)Keycode;
        }
        else
        {
            gCLIBuffer.Buffer[gCLIBuffer.BufferSize++] = (char)Keycode;
        }
        PutChar(Keycode);
    }
    else if (Keycode == KEY_RETURN)
    {
        if (IsInEditMode())
        {
            gEditBuffer.Buffer[gEditBuffer.BufferSize++] = (char)'\0';
        }
        else
        {
            gCLIBuffer.Buffer[gCLIBuffer.BufferSize++] = (char)'\0';
        }
        MoveCursorNewLine();

        if (!IsInEditMode())
        {
            if (gCLIBuffer.BufferSize > 0 && gCLIBuffer.BufferSize < sizeof(gCLIBuffer.Buffer))
            {
                char lastChar = gCLIBuffer.Buffer[gCLIBuffer.BufferSize - 1];
                if (lastChar == '\0')
                {
                    CLI_ExecuteCommand();
                    gCLIBuffer.BufferSize = 0;
                }
            }
        }
    }
    else if (Keycode == KEY_BACKSPACE)
    {
        if (IsInEditMode())
        {
            if (gEditBuffer.BufferSize > 0)
            {
                gEditBuffer.Buffer[gEditBuffer.BufferSize--] = (char)'\0';
            }
        }
        else
        {
            if (gCLIBuffer.BufferSize > 0)
            {
                gCLIBuffer.Buffer[gCLIBuffer.BufferSize--] = (char)'\0';
            }
        }
        RemoveChar();
    }
    else if (Keycode == KEY_ESCAPE)
    {
        if (IsInEditMode())
        {
            SaveScreen(1);
            RestoreScreen(0);
            SetEditMode(0);
        }
    }
    else if (Keycode == KEY_UP)
    {
        if (IsInEditMode())
        {
            MoveCursorLineUp();
        }
        else
        {
            GetPreviousCommand();
        }
    }
    else if (Keycode == KEY_DOWN)
    {
        if (IsInEditMode())
        {
            MoveCursorLineDown();
        }
        else
        {
            GetNextCommand();
        }
    }
    else if (Keycode == KEY_RIGHT)
    {
        if (IsInEditMode())
        {
            MoveCursorColumnRight();
        }
    }
    else if (Keycode == KEY_LEFT)
    {
        if (IsInEditMode())
        {
            MoveCursorColumnLeft();
        }
    }
    else if (Keycode == KEY_CAPSLOCK || Keycode == KEY_LSHIFT || Keycode == KEY_RSHIFT)
    {
        ToggleUppercase();
    }
    else
    {
        //
        // ignore for the moment
        //
    }
}