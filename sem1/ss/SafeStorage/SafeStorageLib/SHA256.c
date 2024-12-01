#include "SHA256.h"
#include "Utils.h"

_Use_decl_annotations_
NTSTATUS
CalculateSHA256Hash(
    _Out_writes_z_(SHA256_CHAR_SIZE + sizeof(ANSI_NULL)) LPSTR   PasswordHash,
    _In_reads_bytes_(BufferSize)                         PBYTE   Buffer,
    _In_                                                 ULONG32 BufferSize
)
{
    ZeroMemory(PasswordHash, SHA256_CHAR_SIZE);
    PasswordHash[SHA256_CHAR_SIZE] = ANSI_NULL;

    BCRYPT_ALG_HANDLE algorithmHandle = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithmHandle, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status) || !IS_VALID_HANDLE(algorithmHandle))
    {
        return status;
    }
    
    BCRYPT_HASH_HANDLE hashHandle = NULL;
    status = BCryptCreateHash(algorithmHandle, &hashHandle, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status) || !IS_VALID_HANDLE(hashHandle))
    {
        DBG_ASSERT(NT_SUCCESS(BCryptCloseAlgorithmProvider(algorithmHandle, 0)));
        return status;
    }

    status = BCryptHashData(hashHandle, Buffer, BufferSize, 0);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    BYTE hash[SHA256_BYTE_SIZE] = { 0 };
    status = BCryptFinishHash(hashHandle, hash, sizeof(hash), 0);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    status = Hex2Str(PasswordHash, hash, SHA256_BYTE_SIZE);

CleanUp:
    DBG_ASSERT(NT_SUCCESS(BCryptDestroyHash(hashHandle)));
    DBG_ASSERT(NT_SUCCESS(BCryptCloseAlgorithmProvider(algorithmHandle, 0)));

    return status;
}