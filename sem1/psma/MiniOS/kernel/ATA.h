#ifndef _ATA_H_
#define _ATA_H_

#include "main.h"

#define ATA_PRIMARY_IO              (0x1F0)
#define ATA_SECONDARY_IO            (0x170)

#define ATA_PRIMARY_CTRL            (0x3F6)
#define ATA_SECONDARY_CTRL          (0x376)

#define ATA_DATA                    (0x0)
#define ATA_ERROR                   (0x1)
#define ATA_SECTOR_COUNT            (0x2)
#define ATA_LBA_LO                  (0x3)
#define ATA_LBA_MID                 (0x4)
#define ATA_LBA_HI                  (0x5)
#define ATA_DRIVE_HEAD              (0x6)
#define ATA_STATUS                  (0x7)
#define ATA_COMMAND                 (0x7)

#define ATA_CMD_READ_PIO            (0x20)
#define ATA_CMD_READ_PIO_EXT        (0x24)
#define ATA_CMD_READ_DMA            (0xC8)
#define ATA_CMD_READ_DMA_EXT        (0x25)
#define ATA_CMD_WRITE_PIO           (0x30)
#define ATA_CMD_WRITE_PIO_EXT       (0x34)
#define ATA_CMD_WRITE_DMA           (0xCA)
#define ATA_CMD_WRITE_DMA_EXT       (0x35)
#define ATA_CMD_CACHE_FLUSH         (0xE7)
#define ATA_CMD_CACHE_FLUSH_EXT     (0xEA)
#define ATA_CMD_PACKET              (0xA0)
#define ATA_CMD_IDENTIFY            (0xEC)

#define STATUS_BSY                  (0x80)
#define STATUS_DRDY                 (0x40)
#define STATUS_DF                   (0x20)
#define STATUS_DSC                  (0x10)
#define STATUS_DRQ                  (0x08)
#define STATUS_CORR                 (0x04)
#define STATUS_IDX                  (0x02)
#define STATUS_ERR                  (0x01)

#define ATA_SERIAL_NO_CHARS         (20)
#define ATA_MODEL_NO_CHARS          (40)

#define ATA_PRIMARY                 (0)
#define ATA_SECONDARY               (1)
#define ATA_MASTER                  (0)
#define ATA_SLAVE                   (1)
#define ATA_SECTOR_SIZE             (512)

#pragma pack(push, 1)
typedef struct _ATA_IDENTIFY_RESPONSE
{
    /*
    0 General configuration information
    1 Number of logical cylinders in the default CHS translation
    3 Number of logical heads in the default CHS translation
    6 Number of logical sectors per track in the default CHS translation
    10-19 Serial number (20 ASCII characters)
    23-26 Firmware revision (8 ASCII characters)
    27-46 Model number (40 ASCII characters)
    54 Number of logical cylinders in the current CHS translation
    55 Number of current logical heads in the current CHS translation
    56 Number of current logical sectors per track in the current CHS translation
    57-58 Capacity in sectors in the current CHS translation
    60-61 Total number of addressable sectors (28-bit LBA addressing)
    100-103 Total number of addressable sectors (48-bit LBA addressing)
    160-255 Reserved
    */
    WORD     GeneralConfig;                              // 0
    WORD     LogicalCylinders;                           // 2
    WORD     Reserved0;                                  // 4
    WORD     LogicalHeads;                               // 6
    WORD     Reserved1[2];                               // 8
    WORD     LogicalSectors;                             // 12
    WORD     Reserved2[3];                               // 14
    char     SerialNumbers[ATA_SERIAL_NO_CHARS];         // 20
    WORD     Reserved3[3];                               // 40
    char     FirmwareRevision[8];                        // 46
    char     ModelNumber[ATA_MODEL_NO_CHARS];            // 54
    WORD     Reserved4[7];                               // 94
    WORD     LogicalCylindersCurrent;                    // 108
    WORD     LogicalHeadsCurrent;                        // 110
    WORD     LogicalSectorsCurrent;                      // 112
    DWORD    SectorCapacity;                             // 114
    WORD     Reserved5;                                  // 118
    DWORD    Address28Bit;                               // 120
    WORD     Reserved6[20];                              // 124
    struct
    {
        WORD Reserved0 : 4;
        WORD PacketFeature : 1;
        WORD Reserved1 : 11;
        WORD Reserved2 : 10;
        WORD SupportLba48 : 1;
        WORD Reserved3 : 5;
        WORD Reserved4;
    } Features;
    WORD     Reserved7[15];                              // 170
    QWORD    Address48Bit;                               // 200
    WORD     Reserved8[152];                             // 208
} ATA_IDENTIFY_RESPONSE, * PATA_IDENTIFY_RESPONSE;
#pragma pack(pop)

VOID
ATA_DetectDevice();

_Must_inspect_result_
int
ATA_ReadData(
    _Out_writes_bytes_(Size) PBYTE Buffer,
    _In_                     int   BusIdx,
    _In_                     int   DeviceIdx,
    _In_                     DWORD Offset,
    _In_                     DWORD Size
);

#endif//_ATA_H_