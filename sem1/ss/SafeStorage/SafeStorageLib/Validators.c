#include "Validators.h"

_Must_inspect_result_
static
BOOL
IsLowercaseLetter(
    _In_ CHAR Character
)
{
    return ('a' <= Character && Character <= 'z');
}

_Must_inspect_result_
static
BOOL
IsUppercaseLetter(
    _In_ CHAR Character
)
{
    return ('A' <= Character && Character <= 'Z');
}

_Must_inspect_result_
static
BOOL
IsDigit(
    _In_ CHAR Character
)
{
    return ('0' <= Character && Character <= '9');
}

_Must_inspect_result_
static
BOOL
IsAlphabetLetter(
    _In_ CHAR Character
)
{
    return (IsLowercaseLetter(Character) ||
            IsUppercaseLetter(Character));
}

_Must_inspect_result_
static
int
IsPasswordSpecialSymbol(
    _In_ CHAR Character
)
{
    return (Character == '!' || Character == '@' || Character == '#' ||
            Character == '$' || Character == '%' || Character == '^' ||
            Character == '&');
}

_Use_decl_annotations_
NTSTATUS
ValidateUsername(
    _In_reads_z_(UsernameLength) LPCSTR Username,
    _In_                         USHORT UsernameLength
)
{
    if (UsernameLength < 5 || UsernameLength > 10)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (Username[UsernameLength] != ANSI_NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    for (BYTE i = 0; i < UsernameLength; ++i)
    {
        BOOL isAlphabetLetter = IsAlphabetLetter(Username[i]);
        if (!isAlphabetLetter)
        {
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ValidatePassword(
    _In_reads_z_(PasswordLength) LPCSTR Password,
    _In_                         USHORT PasswordLength
)
{
    if (PasswordLength < 5)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (Password[PasswordLength] != ANSI_NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    BOOL containsDigit = FALSE;
    BOOL containsLowercaseLetter = FALSE;
    BOOL containsUppercaseLetter = FALSE;
    BOOL containsSpecialSymbol = FALSE;

    for (BYTE i = 0; i < PasswordLength; ++i)
    {
        BOOL isLowercaseLetter = IsLowercaseLetter(Password[i]);
        if (isLowercaseLetter)
        {
            containsLowercaseLetter = TRUE;
            continue;
        }

        BOOL isUppercaseLetter = IsUppercaseLetter(Password[i]);
        if (isUppercaseLetter)
        {
            containsUppercaseLetter = TRUE;
            continue;
        }

        BOOL isDigit = IsDigit(Password[i]);
        if (isDigit)
        {
            containsDigit = TRUE;
            continue;
        }

        BOOL isPasswordSpecialSymbol = IsPasswordSpecialSymbol(Password[i]);
        if (isPasswordSpecialSymbol)
        {
            containsSpecialSymbol = TRUE;
            continue;
        }

        return STATUS_INVALID_PARAMETER_1;
    }

    if (!containsDigit || !containsLowercaseLetter || !containsUppercaseLetter || !containsSpecialSymbol)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
ValidateFile(
    _In_                         HANDLE FileHandle,
    _In_reads_z_(FilePathLength) LPCSTR FilePath,
    _In_                         USHORT FilePathLength
)
{
    if (!IS_VALID_HANDLE(FileHandle))
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (FilePathLength == 0)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    if (FilePath[FilePathLength] != ANSI_NULL)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (FilePath[0] == '\\')
    {
        //
        // Path is relative to the current drive or is a remote share.
        //
        return STATUS_INVALID_PARAMETER_2;
    }

    LARGE_INTEGER fileSize = { 0 };
    BOOL status = GetFileSizeEx(FileHandle, &fileSize);
    if (status)
    {
        const LONG64 maxFileSize = (LONG64)(1024 * 1024 * 1024) * 8;  // 8 GB
        if (fileSize.QuadPart > maxFileSize)
        {
            return STATUS_FILE_NOT_SUPPORTED;
        }
    }

    BYTE index = 0;
    if (FilePathLength >= 3 && !strncmp(FilePath, ".\\", 2))
    {
        //
        // Path is relative to the current directory.
        //
        index = 1;
    }

    //
    // Check for path-traversal and symlinks
    //
    CHAR filePath[MAX_PATH] = { 0 };
    DWORD filePathSize = MAX_PATH;
    DWORD size = GetFinalPathNameByHandleA(FileHandle, filePath, filePathSize, FILE_NAME_OPENED | VOLUME_NAME_DOS);
    if (!size || size >= filePathSize)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LPCSTR relativePath = FilePath + index;
    size_t relativePathLength = strlen(relativePath);
    size_t filePathLength = strlen(filePath);

    if (filePathLength < relativePathLength)
    {
        return STATUS_FILE_NOT_SUPPORTED;
    }

    //
    // Perform an ends-with comparison.
    //
    if (_stricmp(filePath + filePathLength - relativePathLength, relativePath))
    {
        return STATUS_FILE_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}