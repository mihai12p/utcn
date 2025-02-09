#include "ATA.h"
#include "PIC.h"
#include "String.h"

typedef struct _ATA_DEVICE
{
    int                   IsPresent;
    int                   IsMaster;
    WORD                  IOBase;
    ATA_IDENTIFY_RESPONSE Identity;
} ATA_DEVICE;

ATA_DEVICE gAtaDevices[2][2] = { 0 };

//
// https://wiki.osdev.org/ATA_PIO_Mode
//
VOID
ATA_DetectDevice()
{
    WORD buses[2] = { ATA_PRIMARY_IO, ATA_SECONDARY_IO };
    WORD controls[2] = { ATA_PRIMARY_CTRL, ATA_SECONDARY_CTRL };

    for (int busIdx = 0; busIdx < __crt_countof(buses); ++busIdx)
    {
        for (int deviceIdx = 0; deviceIdx < 2; ++deviceIdx)
        {
            WORD ioBase = buses[busIdx];
            WORD controlBase = controls[busIdx];

            __outbyte(ioBase + ATA_DRIVE_HEAD, 0xA0 | (deviceIdx << 4)); io_wait();
            __outbyte(ioBase + ATA_SECTOR_COUNT, 0); io_wait();
            __outbyte(ioBase + ATA_LBA_LO, 0); io_wait();
            __outbyte(ioBase + ATA_LBA_MID, 0); io_wait();
            __outbyte(ioBase + ATA_LBA_HI, 0); io_wait();
            __outbyte(ioBase + ATA_COMMAND, ATA_CMD_IDENTIFY); io_wait();

            BYTE status = __inbyte(ioBase + ATA_STATUS);
            if (status == 0)
            {
                //
                // No device detected.
                //
                continue;
            }

            //
            // Wait for BSY to clear.
            //
            int timeout = 10000;
            while ((status & STATUS_BSY))
            {
                status = __inbyte(ioBase + ATA_STATUS);
                if (--timeout <= 0)
                {
                    break;
                }
            }

            if (timeout <= 0)
            {
                continue;
            }

            //
            // Check LBA MID and LBA HI to rule out non-ATA devices.
            //
            BYTE lbaMid = __inbyte(ioBase + ATA_LBA_MID);
            BYTE lbaHi = __inbyte(ioBase + ATA_LBA_HI);
            if (lbaMid && lbaHi)
            {
                //
                // ATAPI device not following the specifications detected. Skip it.
                //
                continue;
            }

            timeout = 10000;
            while (!((status & STATUS_ERR) || (status & STATUS_DRQ)))
            {
                status = __inbyte(ioBase + ATA_STATUS);
                if (--timeout <= 0)
                {
                    break;
                }
            }

            if (timeout <= 0)
            {
                continue;
            }

            if (status & STATUS_ERR)
            {
                //
                // ATAPI device detected. Skip it.
                //
                continue;
            }

            //
            // ATA device detected.
            //
            gAtaDevices[busIdx][deviceIdx].IsPresent = 1;
            gAtaDevices[busIdx][deviceIdx].IsMaster = (deviceIdx == ATA_MASTER);
            gAtaDevices[busIdx][deviceIdx].IOBase = ioBase;

            WORD modelNumberOffset = FIELD_OFFSET(ATA_IDENTIFY_RESPONSE, ModelNumber);
            WORD modelNumberSize = FIELD_OFFSET(ATA_IDENTIFY_RESPONSE, Reserved4) - modelNumberOffset;

            WORD firmwareRevisionOffset = FIELD_OFFSET(ATA_IDENTIFY_RESPONSE, FirmwareRevision);
            WORD firmwareRevisionSize = modelNumberOffset - firmwareRevisionOffset;

            WORD serialNumbersOffset = FIELD_OFFSET(ATA_IDENTIFY_RESPONSE, SerialNumbers);
            WORD serialNumbersSize = FIELD_OFFSET(ATA_IDENTIFY_RESPONSE, Reserved3) - serialNumbersOffset;

            WORD buffer[256] = { 0 };
            for (WORD i = 0; i < 256; ++i)
            {
                buffer[i] = __inword(ioBase + ATA_DATA);

                //
                // Strings are in little-endian at the level of each WORD in memory
                //
                WORD structIdx = i * sizeof(WORD);
                if ((serialNumbersOffset <= structIdx && structIdx < serialNumbersOffset + serialNumbersSize) ||
                    (firmwareRevisionOffset <= structIdx && structIdx < firmwareRevisionOffset + firmwareRevisionSize) ||
                    (modelNumberOffset <= structIdx && structIdx < modelNumberOffset + modelNumberSize))
                {
                    //
                    // Switch the order of the characters
                    //
                    buffer[i] = ((buffer[i] & 0xFF) << 8) + (buffer[i] >> 8);
                }
            }
            memncpy(&gAtaDevices[busIdx][deviceIdx].Identity, buffer, sizeof(gAtaDevices[busIdx][deviceIdx].Identity));
        }
    }
}

_Use_decl_annotations_
int
ATA_ReadData(
    _Out_writes_bytes_(Size) PBYTE Buffer,
    _In_                     int   BusIdx,
    _In_                     int   DeviceIdx,
    _In_                     DWORD Offset,
    _In_                     DWORD Size
)
{
    *Buffer = 0;

    if (!gAtaDevices[BusIdx][DeviceIdx].IsPresent)
    {
        return 0;
    }

    WORD ioBase = gAtaDevices[BusIdx][DeviceIdx].IOBase;
    DWORD sector = Offset / ATA_SECTOR_SIZE;
    DWORD sectorOffset = Offset % ATA_SECTOR_SIZE;
    DWORD bytesToRead = Size;
    PBYTE bufferPtr = Buffer;

    while (bytesToRead)
    {
        //
        // Enable LBA addressing mode
        //
        __outbyte(ioBase + ATA_DRIVE_HEAD, 0xE0 | (DeviceIdx << 4) | (sector >> 24)); io_wait();
        __outbyte(ioBase + ATA_SECTOR_COUNT, 1); io_wait();
        __outbyte(ioBase + ATA_LBA_LO, sector & 0xFF); io_wait();
        __outbyte(ioBase + ATA_LBA_MID, (sector >> 8) & 0xFF); io_wait();
        __outbyte(ioBase + ATA_LBA_HI, (sector >> 16) & 0xFF); io_wait();
        __outbyte(ioBase + ATA_COMMAND, ATA_CMD_READ_PIO); io_wait();

        //
        // Read 256 words (512 bytes) from the Data Register
        //
        BYTE sectorBuffer[ATA_SECTOR_SIZE] = { 0 };
        for (int i = 0; i < ATA_SECTOR_SIZE / sizeof(WORD); ++i)
        {
            //
            // Wait for the device to be ready
            //
            BYTE status = 0;
            do
            {
                status = __inbyte(ioBase + ATA_STATUS);
                if (status & STATUS_ERR)
                {
                    return 0;
                }
            } while (!(status & STATUS_DRQ));

            WORD word = __inword(ioBase + ATA_DATA);
            sectorBuffer[i * 2] = word & 0xFF;
            sectorBuffer[i * 2 + 1] = (word >> 8) & 0xFF;
        }

        DWORD bytesFromSector = ATA_SECTOR_SIZE - sectorOffset;
        if (bytesFromSector > bytesToRead)
        {
            bytesFromSector = bytesToRead;
        }

        memncpy(bufferPtr, sectorBuffer + sectorOffset, bytesFromSector);

        bufferPtr += bytesFromSector;
        bytesToRead -= bytesFromSector;
        sectorOffset = 0;
        ++sector;
    }

    return 1;
}