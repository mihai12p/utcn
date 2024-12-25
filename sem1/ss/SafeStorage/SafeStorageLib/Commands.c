#include "Commands.h"
#include "Validators.h"
#include "Utils.h"
#include "SHA256.h"
#include "Security.h"
#include "FileOperation.h"

typedef struct _USER_CONTEXT
{
    BOOL        IsLoggedIn;
    ANSI_STRING UserDirectory;
} USER_CONTEXT;

typedef struct _GLOBAL_CONTEXT
{
    ANSI_STRING  CurrentDirectory;
    ANSI_STRING  UsersSubdirectory;
    ANSI_STRING  UsersDatabase;
    USER_CONTEXT UserContext;
} GLOBAL_CONTEXT;

static GLOBAL_CONTEXT gContext = { 0 };

_Use_decl_annotations_
NTSTATUS
WINAPI
SafeStorageInit()
{
    DWORD bufferSize = GetCurrentDirectoryA(0, NULL);
    if (!bufferSize || bufferSize >= MAX_PATH)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    gContext.CurrentDirectory.Buffer = calloc(bufferSize, sizeof(CHAR));
    if (!gContext.CurrentDirectory.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG32 returnSize = GetCurrentDirectoryA(bufferSize, gContext.CurrentDirectory.Buffer);
    if (!returnSize || returnSize >= bufferSize)
    {
        FreeAnsiString(&gContext.CurrentDirectory);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    gContext.CurrentDirectory.Length = (USHORT)returnSize;
    gContext.CurrentDirectory.MaximumLength = gContext.CurrentDirectory.Length + sizeof(ANSI_NULL);

    const CHAR usersSubdirectory[7] = "\\users";

    gContext.UsersSubdirectory.Length = (USHORT)returnSize + sizeof(usersSubdirectory) - sizeof(ANSI_NULL);
    gContext.UsersSubdirectory.MaximumLength = gContext.UsersSubdirectory.Length + sizeof(ANSI_NULL);
    gContext.UsersSubdirectory.Buffer = calloc(gContext.UsersSubdirectory.MaximumLength, sizeof(CHAR));
    if (!gContext.UsersSubdirectory.Buffer)
    {
        FreeAnsiString(&gContext.CurrentDirectory);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (VOID)strcpy_s(gContext.UsersSubdirectory.Buffer, gContext.UsersSubdirectory.MaximumLength, gContext.CurrentDirectory.Buffer);
    (VOID)strcat_s(gContext.UsersSubdirectory.Buffer, gContext.UsersSubdirectory.MaximumLength, usersSubdirectory);

    const CHAR usersDatabase[11] = "\\users.txt";

    gContext.UsersDatabase.Length = (USHORT)returnSize + sizeof(usersDatabase) - sizeof(ANSI_NULL);
    gContext.UsersDatabase.MaximumLength = gContext.UsersDatabase.Length + sizeof(ANSI_NULL);
    gContext.UsersDatabase.Buffer = calloc(gContext.UsersDatabase.MaximumLength, sizeof(CHAR));
    if (!gContext.UsersDatabase.Buffer)
    {
        FreeAnsiString(&gContext.UsersSubdirectory);
        FreeAnsiString(&gContext.CurrentDirectory);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (VOID)strcpy_s(gContext.UsersDatabase.Buffer, gContext.UsersDatabase.MaximumLength, gContext.CurrentDirectory.Buffer);
    (VOID)strcat_s(gContext.UsersDatabase.Buffer, gContext.UsersDatabase.MaximumLength, usersDatabase);

    BOOL status = CreateDirectoryA(gContext.UsersSubdirectory.Buffer, NULL);
    if (!status)
    {
        ULONG32 lastError = GetLastError();
        if (lastError != ERROR_ALREADY_EXISTS)
        {
            FreeAnsiString(&gContext.UsersDatabase);
            FreeAnsiString(&gContext.UsersSubdirectory);
            FreeAnsiString(&gContext.CurrentDirectory);
            return HRESULT_FROM_WIN32(lastError);
        }
    }

    HANDLE fileHandle = CreateFileA(gContext.UsersDatabase.Buffer, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!IS_VALID_HANDLE(fileHandle))
    {
        ULONG32 lastError = GetLastError();
        if (lastError != ERROR_FILE_EXISTS)
        {
            (VOID)RemoveDirectoryA(gContext.UsersSubdirectory.Buffer);
            FreeAnsiString(&gContext.UsersDatabase);
            FreeAnsiString(&gContext.UsersSubdirectory);
            FreeAnsiString(&gContext.CurrentDirectory);
            return HRESULT_FROM_WIN32(lastError);
        }
    }
    if (IS_VALID_HANDLE(fileHandle))
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(fileHandle)));
    }

    ResetSecurityContext();

    return STATUS_SUCCESS;
}


VOID
WINAPI
SafeStorageDeinit()
{
    FreeAnsiString(&gContext.UserContext.UserDirectory);
    FreeAnsiString(&gContext.UsersDatabase);
    FreeAnsiString(&gContext.UsersSubdirectory);
    FreeAnsiString(&gContext.CurrentDirectory);
}


_Use_decl_annotations_
NTSTATUS
WINAPI
SafeStorageHandleRegister(
    _In_reads_z_(UsernameLength) const char* Username,
    _In_                         uint16_t    UsernameLength,
    _In_reads_z_(PasswordLength) const char* Password,
    _In_                         uint16_t    PasswordLength
)
{
    if (gContext.UserContext.IsLoggedIn)
    {
        return STATUS_NOT_SUPPORTED;
    }

    NTSTATUS status = ValidateUsername(Username, UsernameLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = ValidatePassword(Password, PasswordLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    CHAR passwordHash[SHA256_CHAR_SIZE + sizeof(ANSI_NULL)] = { 0 };
    status = CalculateSHA256Hash(passwordHash, (PBYTE)Password, PasswordLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ANSI_STRING userSubdirectory = { 0 };
    userSubdirectory.Length = gContext.UsersSubdirectory.Length + sizeof(CHAR) + UsernameLength;
    userSubdirectory.MaximumLength = userSubdirectory.Length + sizeof(ANSI_NULL);
    userSubdirectory.Buffer = calloc(userSubdirectory.MaximumLength, sizeof(CHAR));
    if (!userSubdirectory.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (VOID)strcpy_s(userSubdirectory.Buffer, userSubdirectory.MaximumLength, gContext.UsersSubdirectory.Buffer);
    (VOID)strcat_s(userSubdirectory.Buffer, userSubdirectory.MaximumLength, "\\");
    (VOID)strcat_s(userSubdirectory.Buffer, userSubdirectory.MaximumLength, Username);

    BOOL status_2 = CreateDirectoryA(userSubdirectory.Buffer, NULL);
    FreeAnsiString(&userSubdirectory);
    if (!status_2)
    {
        ULONG32 lastError = GetLastError();
        if (lastError == ERROR_ALREADY_EXISTS)
        {
            return STATUS_USER_EXISTS;
        }

        return HRESULT_FROM_WIN32(lastError);
    }

    HANDLE fileHandle = CreateFileA(gContext.UsersDatabase.Buffer, FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!IS_VALID_HANDLE(fileHandle))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD filePointer = SetFilePointer(fileHandle, 0, NULL, FILE_END);
    if (filePointer == INVALID_SET_FILE_POINTER)
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(fileHandle)));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ANSI_STRING credentials = { 0 };
    credentials.Length = UsernameLength + sizeof(CHAR) + sizeof(passwordHash) + 2 * sizeof(CHAR) - sizeof(ANSI_NULL);
    credentials.MaximumLength = credentials.Length + sizeof(ANSI_NULL);
    credentials.Buffer = calloc(credentials.MaximumLength, sizeof(CHAR));
    if (!credentials.Buffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }

    (VOID)strcpy_s(credentials.Buffer, credentials.MaximumLength, Username);
    (VOID)strcat_s(credentials.Buffer, credentials.MaximumLength, "|");
    (VOID)strcat_s(credentials.Buffer, credentials.MaximumLength, passwordHash);
    (VOID)strcat_s(credentials.Buffer, credentials.MaximumLength, "\r\n");

    DWORD numberOfBytesWritten = 0;
    status_2 = WriteFile(fileHandle, credentials.Buffer, credentials.Length, &numberOfBytesWritten, NULL);
    FreeAnsiString(&credentials);
    if (!status_2)
    {
        status = HRESULT_FROM_WIN32(GetLastError());
    }

CleanUp:
    DBG_ASSERT(NT_SUCCESS(CloseHandle(fileHandle)));

    return status;
}


_Use_decl_annotations_
NTSTATUS
WINAPI
SafeStorageHandleLogin(
    _In_reads_z_(UsernameLength) const char* Username,
    _In_                         uint16_t    UsernameLength,
    _In_reads_z_(PasswordLength) const char* Password,
    _In_                         uint16_t    PasswordLength
)
{
    if (gContext.UserContext.IsLoggedIn)
    {
        return STATUS_NOT_SUPPORTED;
    }

    BOOL isLoginBlocked = IsLoginBlocked();
    if (isLoginBlocked)
    {
        return STATUS_TOO_MANY_COMMANDS;
    }

    NTSTATUS status = ValidateUsername(Username, UsernameLength);
    if (!NT_SUCCESS(status))
    {
        RecordFailedLogin();
        return status;
    }

    status = ValidatePassword(Password, PasswordLength);
    if (!NT_SUCCESS(status))
    {
        RecordFailedLogin();
        return status;
    }

    CHAR passwordHash[SHA256_CHAR_SIZE + sizeof(ANSI_NULL)] = { 0 };
    status = CalculateSHA256Hash(passwordHash, (PBYTE)Password, PasswordLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    HANDLE fileHandle = CreateFileA(gContext.UsersDatabase.Buffer, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!IS_VALID_HANDLE(fileHandle))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD filePointer = SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN);
    if (filePointer == INVALID_SET_FILE_POINTER)
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(fileHandle)));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LPSTR line = NULL;
    ULONG32 lineLength = 0;
    while (NT_SUCCESS(status = ReadLineFromFile(fileHandle, &line, &lineLength)))
    {
        PCHAR separator = strrchr(line, '|');
        if (!separator)
        {
            free(line);
            line = NULL;
            continue;
        }

        LPCSTR usernameInFile = line;
        if (strncmp(Username, usernameInFile, UsernameLength))
        {
            free(line);
            line = NULL;
            continue;
        }

        LPCSTR passwordHashInFile = separator + sizeof(CHAR);
        if (!strncmp(passwordHash, passwordHashInFile, SHA256_CHAR_SIZE))
        {
            gContext.UserContext.UserDirectory.Length = gContext.UsersSubdirectory.Length + sizeof(CHAR) + UsernameLength;
            gContext.UserContext.UserDirectory.MaximumLength = gContext.UserContext.UserDirectory.Length + sizeof(ANSI_NULL);
            gContext.UserContext.UserDirectory.Buffer = calloc(gContext.UserContext.UserDirectory.MaximumLength, sizeof(CHAR));
            if (!gContext.UserContext.UserDirectory.Buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto CleanUp;
            }

            (VOID)strcpy_s(gContext.UserContext.UserDirectory.Buffer, gContext.UserContext.UserDirectory.MaximumLength, gContext.UsersSubdirectory.Buffer);
            (VOID)strcat_s(gContext.UserContext.UserDirectory.Buffer, gContext.UserContext.UserDirectory.MaximumLength, "\\");
            (VOID)strcat_s(gContext.UserContext.UserDirectory.Buffer, gContext.UserContext.UserDirectory.MaximumLength, Username);

            gContext.UserContext.IsLoggedIn = TRUE;
            goto CleanUp;
        }

        free(line);
        line = NULL;
    }

    if (!NT_SUCCESS(status))
    {
        RecordFailedLogin();
    }

CleanUp:
    if (line)
    {
        free(line);
        line = NULL;
    }

    DBG_ASSERT(NT_SUCCESS(CloseHandle(fileHandle)));

    return status;
}


_Use_decl_annotations_
NTSTATUS
WINAPI
SafeStorageHandleLogout()
{
    if (!gContext.UserContext.IsLoggedIn)
    {
        return STATUS_NOT_SUPPORTED;
    }

    FreeAnsiString(&gContext.UserContext.UserDirectory);
    gContext.UserContext.IsLoggedIn = FALSE;

    return STATUS_SUCCESS;
}


_Use_decl_annotations_
NTSTATUS
WINAPI
SafeStorageHandleStore(
    _In_reads_z_(SubmissionNameLength) const char* SubmissionName,
    _In_                               uint16_t    SubmissionNameLength,
    _In_reads_z_(SourceFilePathLength) const char* SourceFilePath,
    _In_                               uint16_t    SourceFilePathLength
)
{
    if (!gContext.UserContext.IsLoggedIn)
    {
        return STATUS_NOT_SUPPORTED;
    }

    HANDLE srcFileHandle = CreateFileA(SourceFilePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (!IS_VALID_HANDLE(srcFileHandle))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    NTSTATUS status = ValidateFile(srcFileHandle, SourceFilePath, SourceFilePathLength);
    if (!NT_SUCCESS(status))
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));
        return status;
    }

    if (SubmissionNameLength == 0)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    ANSI_STRING destFilePath = { 0 };
    destFilePath.Length = gContext.UserContext.UserDirectory.Length + sizeof(CHAR) + SubmissionNameLength;
    destFilePath.MaximumLength = destFilePath.Length + sizeof(ANSI_NULL);
    destFilePath.Buffer = calloc(destFilePath.MaximumLength, sizeof(CHAR));
    if (!destFilePath.Buffer)
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (VOID)strcpy_s(destFilePath.Buffer, destFilePath.MaximumLength, gContext.UserContext.UserDirectory.Buffer);
    (VOID)strcat_s(destFilePath.Buffer, destFilePath.MaximumLength, "\\");
    (VOID)strcat_s(destFilePath.Buffer, destFilePath.MaximumLength, SubmissionName);

    HANDLE destFileHandle = CreateFileA(destFilePath.Buffer, FILE_GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (!IS_VALID_HANDLE(destFileHandle))
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));
        FreeAnsiString(&destFilePath);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    status = ValidateFile(destFileHandle, destFilePath.Buffer, destFilePath.Length);
    FreeAnsiString(&destFilePath);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    status = MyCopyFile(srcFileHandle, destFileHandle);

CleanUp:
    DBG_ASSERT(NT_SUCCESS(CloseHandle(destFileHandle)));
    DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));

    return status;
}


_Use_decl_annotations_
NTSTATUS
WINAPI
SafeStorageHandleRetrieve(
    _In_reads_z_(SubmissionNameLength)      const char* SubmissionName,
    _In_                                    uint16_t    SubmissionNameLength,
    _In_reads_z_(DestinationFilePathLength) const char* DestinationFilePath,
    _In_                                    uint16_t    DestinationFilePathLength
)
{
    if (!gContext.UserContext.IsLoggedIn)
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (SubmissionNameLength == 0)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    ANSI_STRING srcFilePath = { 0 };
    srcFilePath.Length = gContext.UserContext.UserDirectory.Length + sizeof(CHAR) + SubmissionNameLength;
    srcFilePath.MaximumLength = srcFilePath.Length + sizeof(ANSI_NULL);
    srcFilePath.Buffer = calloc(srcFilePath.MaximumLength, sizeof(CHAR));
    if (!srcFilePath.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (VOID)strcpy_s(srcFilePath.Buffer, srcFilePath.MaximumLength, gContext.UserContext.UserDirectory.Buffer);
    (VOID)strcat_s(srcFilePath.Buffer, srcFilePath.MaximumLength, "\\");
    (VOID)strcat_s(srcFilePath.Buffer, srcFilePath.MaximumLength, SubmissionName);

    HANDLE srcFileHandle = CreateFileA(srcFilePath.Buffer, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (!IS_VALID_HANDLE(srcFileHandle))
    {
        FreeAnsiString(&srcFilePath);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    NTSTATUS status = ValidateFile(srcFileHandle, srcFilePath.Buffer, srcFilePath.Length);
    FreeAnsiString(&srcFilePath);
    if (!NT_SUCCESS(status))
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));
        return status;
    }

    HANDLE destFileHandle = CreateFileA(DestinationFilePath, FILE_GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (!IS_VALID_HANDLE(destFileHandle))
    {
        DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    status = ValidateFile(destFileHandle, DestinationFilePath, DestinationFilePathLength);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    status = MyCopyFile(srcFileHandle, destFileHandle);

CleanUp:
    DBG_ASSERT(NT_SUCCESS(CloseHandle(destFileHandle)));
    DBG_ASSERT(NT_SUCCESS(CloseHandle(srcFileHandle)));

    return status;
}
