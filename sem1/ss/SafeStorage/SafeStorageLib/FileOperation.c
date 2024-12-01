#include "FileOperation.h"
#include "Threadpool.h"

_Use_decl_annotations_
NTSTATUS
ReadLineFromFile(
    _In_                                   HANDLE   FileHandle,
    _Outptr_opt_result_buffer_(*BytesRead) LPSTR*   LineBuffer,
    _Out_opt_                              PULONG32 BytesRead
)
{
    if (!IS_VALID_HANDLE(FileHandle))
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!LineBuffer)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (!BytesRead)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    *LineBuffer = NULL;
    *BytesRead = 0;

    ULONG32 bufferSize = 128;
    LPSTR buffer = calloc(bufferSize, sizeof(CHAR));
    if (!buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG32 totalBytesRead = 0;
    BOOL status = FALSE;

    while (TRUE)
    {
        ULONG32 bytesToRead = 1;
        DWORD bytesActuallyRead = 0;
        CHAR ch = ANSI_NULL;

        status = ReadFile(FileHandle, &ch, bytesToRead, &bytesActuallyRead, NULL);
        if (!status || bytesActuallyRead == 0)
        {
            if (GetLastError() == ERROR_HANDLE_EOF || bytesActuallyRead == 0)
            {
                if (totalBytesRead == 0)
                {
                    free(buffer);
                    *LineBuffer = NULL;
                    *BytesRead = 0;
                    return STATUS_UNSUCCESSFUL;
                }
                else
                {
                    break;
                }
            }
            else
            {
                free(buffer);
                *LineBuffer = NULL;
                *BytesRead = 0;
                return STATUS_UNSUCCESSFUL;
            }
        }

        if (ch == ANSI_NULL)
        {
            continue;
        }

        buffer[totalBytesRead++] = ch;

        if (ch == '\n')
        {
            break;
        }

        if (totalBytesRead + 1 >= bufferSize)
        {
            free(buffer);
            *LineBuffer = NULL;
            *BytesRead = 0;
            return STATUS_UNSUCCESSFUL;
        }
    }

    buffer[totalBytesRead] = ANSI_NULL;

    *LineBuffer = buffer;
    *BytesRead = totalBytesRead;

    return STATUS_SUCCESS;
}

typedef struct _COPY_TASK
{
    HANDLE        SrcFileHandle;
    HANDLE        DestFileHandle;
    ULONG32       NumberOfBytesToCopy;
    LARGE_INTEGER Offset;
} COPY_TASK, * PCOPY_TASK;

static
VOID
WINAPI
CopyChunk(
    _In_ PVOID Argument
)
{
    PCOPY_TASK task = Argument;
    if (!task)
    {
        return;
    }

    if (task->NumberOfBytesToCopy == 0)
    {
        free(task);
        return;
    }

    PBYTE buffer = calloc(task->NumberOfBytesToCopy, sizeof(BYTE));
    if (!buffer)
    {
        free(task);
        return;
    }

    {
        //
        // read the bytes from the source file
        //
        OVERLAPPED overlapped = { 0 };
        overlapped.Offset = task->Offset.LowPart;
        overlapped.OffsetHigh = task->Offset.HighPart;
        overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!IS_VALID_HANDLE(overlapped.hEvent))
        {
            goto CleanUp;
        }

        DWORD bytesRead = 0;
        (VOID)ReadFile(task->SrcFileHandle, buffer, task->NumberOfBytesToCopy, &bytesRead, &overlapped);
        (VOID)WaitForSingleObject(overlapped.hEvent, INFINITE);
        DBG_ASSERT(NT_SUCCESS(CloseHandle(overlapped.hEvent)));
    }

    {
        //
        // write the read bytes to the destination file
        //
        OVERLAPPED overlapped = { 0 };
        overlapped.Offset = task->Offset.LowPart;
        overlapped.OffsetHigh = task->Offset.HighPart;
        overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!IS_VALID_HANDLE(overlapped.hEvent))
        {
            goto CleanUp;
        }

        DWORD bytesWritten = 0;
        (VOID)WriteFile(task->DestFileHandle, buffer, task->NumberOfBytesToCopy, &bytesWritten, &overlapped);
        (VOID)WaitForSingleObject(overlapped.hEvent, INFINITE);
        DBG_ASSERT(NT_SUCCESS(CloseHandle(overlapped.hEvent)));
    }

CleanUp:
    free(buffer);
    free(task);
}

_Use_decl_annotations_
NTSTATUS
MyCopyFile(
    _In_ HANDLE SourceFileHandle,
    _In_ HANDLE DestinationFileHandle
)
{
    if (!IS_VALID_HANDLE(SourceFileHandle))
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!IS_VALID_HANDLE(DestinationFileHandle))
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    LARGE_INTEGER fileSize = { 0 };
    BOOL status = GetFileSizeEx(SourceFileHandle, &fileSize);
    if (!status)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    const ULONG32 chunkSize = 4096;
    LONG64 totalBytes = fileSize.QuadPart;
    LONG chunkCount = (LONG)((totalBytes + chunkSize - 1) / chunkSize);  // safe conversion since file size is limited to 8 GBs

    PTHREADPOOL pool = ThreadPoolCreate(4, chunkCount);
    if (!pool)
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (LONG i = 0; i < chunkCount; ++i)
    {
        ULONG32 bytesToCopy = chunkSize;
        if ((i + 1) * chunkSize > totalBytes)
        {
            bytesToCopy = (ULONG32)(totalBytes - (LONG64)(i * chunkSize));
        }

        PCOPY_TASK task = calloc(1, sizeof(COPY_TASK));
        if (!task)
        {
            continue;
        }

        LARGE_INTEGER offset = { 0 };
        offset.QuadPart = i * chunkSize;

        task->SrcFileHandle = SourceFileHandle;
        task->DestFileHandle = DestinationFileHandle;
        task->NumberOfBytesToCopy = bytesToCopy;
        task->Offset = offset;

        NTSTATUS tpStatus = ThreadPoolSubmitWork(pool, CopyChunk, task);
        if (!NT_SUCCESS(tpStatus))
        {
            free(task);
            continue;
        }
    }

    (VOID)ThreadPoolWaitForTasks(pool, INFINITE);
    ThreadPoolDestroy(pool);

    return STATUS_SUCCESS;
}