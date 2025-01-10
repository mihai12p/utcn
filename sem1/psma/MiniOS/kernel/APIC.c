#include "APIC.h"
#include "String.h"
#include "Virtual.h"
#include "logging.h"

//
// https://pdos.csail.mit.edu/6.828/2018/readings/ia32/MPspec.pdf
// http://www.osdever.net/tutorials/view/multiprocessing-support-for-hobby-oses-explained
//
#pragma pack(push, 1)
typedef struct _MP_FLOATING_POINTER
{
    DWORD Signature;
    DWORD MPConfigurationPointer; // physical address
    BYTE  Length;
    BYTE  Version;
    BYTE  Checksum;
    BYTE  FeatureInfo[5];
} MP_FLOATING_POINTER, * PMP_FLOATING_POINTER;

typedef struct _MP_CONFIGURATION_TABLE
{
    DWORD Signature;
    WORD  BaseTableLength;
    BYTE  Revision;
    BYTE  CheckSum;
    BYTE  OEMID[8];
    BYTE  ProductId[12];
    DWORD OemTablePointer;
    WORD  OemTableSize;
    WORD  EntryCount; // entries in the variable portion of the table
    DWORD LocalApic;
    WORD  ExtendedTableLength;
    BYTE  ExtendedTableChecksum;
    BYTE  Reserved;
} MP_CONFIGURATION_TABLE;

typedef struct _MP_PROCESSOR_ENTRY
{
    BYTE  EntryType; // always 0
    BYTE  LocalApicId;
    BYTE  LocalApicVersion;
    BYTE  Flags;
    DWORD CPUSignature;
    DWORD FeatureFlags;
    QWORD Reserved;
} MP_PROCESSOR_ENTRY;
#pragma pack(pop)

LAPIC_INFO gLAPICInfo = { 0 };

extern VOID EnableLAPIC();
extern VOID SendStartupIPI(_In_ DWORD LAPICId);
extern VOID APStart();

#define MP_FLOATING_POINTER_SIGNATURE       '_PM_'  // _MP_
#define MP_CONFIGURATION_TABLE_SIGNATURE    'PMCP'  // PCMP

#define AP_STARTUP_CODE_ADDRESS             (0x2000)
#define LAPIC_ADDRESS                       (0x1000000)
#define LAPIC_BASE_MASK                     (0xFFFFF000)
#define IA32_APIC_BASE_MSR                  (0x1B)

static
PVOID
FindMPFloatingPointerStructure(
    _In_ QWORD Start,
    _In_ QWORD End
)
{
    for (QWORD address = Start; address < End; address += sizeof(MP_FLOATING_POINTER))
    {
        DWORD signature = *((PDWORD)address);
        if (signature == MP_FLOATING_POINTER_SIGNATURE)
        {
            return (PVOID)address;
        }
    }

    return NULL;
}

static
VOID
ParseMPConfigurationTable(
    _Inout_ PLAPIC_INFO LAPICInfo,
    _In_    QWORD       Address
)
{
    MP_CONFIGURATION_TABLE mpConfig = { 0 };
    memncpy(&mpConfig, (PVOID)Address, sizeof(mpConfig));

    if (mpConfig.Signature != MP_CONFIGURATION_TABLE_SIGNATURE)
    {
        LogMessage("Invalid MP Configuration Table at "); LogQword(Address); LogMessage("\n");
        return;
    }

    LogMessage("MP Configuration Table found at "); LogQword(Address); LogMessage("\n");
    LogMessage("Entry count: "); LogWord(mpConfig.EntryCount); LogMessage("\n");

    QWORD entryAddress = Address + sizeof(mpConfig);
    for (int i = 0; i < mpConfig.EntryCount; ++i)
    {
        if (entryAddress >= Address + mpConfig.BaseTableLength)
        {
            break;
        }

        //
        // All entries share this first field so it's safe to read it this way.
        //
        MP_PROCESSOR_ENTRY entry = { 0 };
        memncpy(&entry, (PVOID)entryAddress, sizeof(entry));

        switch (entry.EntryType)
        {
        case 0: // Processor Entry
        {
            LogMessage("Processor "); LogDword(i); LogMessage(" LAPIC ID "); LogQword(entry.LocalApicId);
            LogMessage(" at "); LogQword(entryAddress); LogMessage("\n");
            entryAddress += sizeof(entry);

            LAPICInfo->LapicIds[LAPICInfo->LapicIdsCount++] = entry.LocalApicId;
            break;
        }
        case 1: // Bus Entry
        case 2: // IO APIC Entry
        case 3: // IO Interrupt Assignment Entry
        case 4: // Local Interrupt Assignment Entry
        {
            entryAddress += 8;
            break;
        }
        default:
            LogMessage("Unknown entry "); LogByte(entry.EntryType); LogMessage("\n");
            __debugbreak();
            break;
        }
    }
}

static
VOID
PrepareAPStartupCode()
{
    //
    // Load start-up code for the AP to execute into a 4-KByte page in the lower 1 MByte of memory
    //
    PVOID startupCodeAddress = (PVOID)AP_STARTUP_CODE_ADDRESS;
    memncpy(startupCodeAddress, APStart, PAGE_SIZE);

    LogMessage("AP startup code prepared at "); LogQword((QWORD)startupCodeAddress); LogMessage("\n");
}

static
int
MapLAPIC()
{
    //
    // Read the LAPIC base address from the IA32_APIC_BASE MSR
    //
    QWORD lapicBase = __readmsr(IA32_APIC_BASE_MSR) & LAPIC_BASE_MASK;

    LogMessage("LAPIC physical address "); LogQword(lapicBase); LogMessage("\n");

    //
    // Map the LAPIC base into virtual memory
    //
    PVOID lapicAddress = (PVOID)LAPIC_ADDRESS;
    int status = PageAlloc(&lapicAddress, 1, lapicBase);
    if (!status)
    {
        return 0;
    }

    LogMessage("LAPIC is mapped to "); LogQword((QWORD)lapicAddress); LogMessage("\n");
    return 1;
}

static
BYTE
GetBSPLAPICId()
{
    extern DWORD GetLAPICId();
    DWORD lapicId = GetLAPICId();

    LogMessage("BSP LAPIC ID "); LogByte(lapicId); LogMessage("\n");
    return lapicId;
}

VOID
InitMP()
{
    QWORD mpFloatingPointerAddress = 0;

    //
    // Search BIOS area
    //
    if (!mpFloatingPointerAddress)
    {
        mpFloatingPointerAddress = (QWORD)FindMPFloatingPointerStructure(0xE0000, 0x100000);
    }

    //
    // Search in the first kilobyte of Extended BIOS Data Area (EBDA)
    // https://wiki.osdev.org/Memory_Map_(x86)#Extended_BIOS_Data_Area_(EBDA)
    //
    if (!mpFloatingPointerAddress)
    {
        QWORD ebdaBase = ((QWORD)(*(PWORD)(0x040E))) << 4;
        if (ebdaBase)
        {
            mpFloatingPointerAddress = (QWORD)FindMPFloatingPointerStructure(ebdaBase, ebdaBase + 1024);
        }
    }

    //
    // Search in the last kilobyte of system base memory
    //
    if (!mpFloatingPointerAddress)
    {
        WORD baseMemorySize = *(PWORD)(0x0413);
        QWORD fallbackEbdaAddress = (baseMemorySize * 1024) - 1024;

        mpFloatingPointerAddress = (QWORD)FindMPFloatingPointerStructure(fallbackEbdaAddress, fallbackEbdaAddress + 1024);
    }

    //
    // Search in the whole mapped memory
    //
    if (!mpFloatingPointerAddress)
    {
        mpFloatingPointerAddress = (QWORD)FindMPFloatingPointerStructure(0, 0xA00000);
    }

    if (!mpFloatingPointerAddress)
    {
        LogMessage("MP Floating Pointer Structure not found\n");
        return;
    }

    LogMessage("MP Floating Pointer Structure found at "); LogQword(mpFloatingPointerAddress); LogMessage("\n");

    PMP_FLOATING_POINTER mpFloatingPointer = (PMP_FLOATING_POINTER)mpFloatingPointerAddress;
    ParseMPConfigurationTable(&gLAPICInfo, mpFloatingPointer->MPConfigurationPointer);

    int status = MapLAPIC();
    if (!status)
    {
        return;
    }

    PrepareAPStartupCode();

    //
    // Enable LAPIC on BSP.
    //
    EnableLAPIC();

    gLAPICInfo.BSPLAPICId = GetBSPLAPICId();
    for (BYTE i = 0; i < gLAPICInfo.LapicIdsCount; ++i)
    {
        BYTE lapicId = gLAPICInfo.LapicIds[i];
        if (lapicId == gLAPICInfo.BSPLAPICId)
        {
            continue;
        }

        LogMessage("Starting processor with LAPIC ID "); LogDword(lapicId); LogMessage("\n");
        SendStartupIPI(lapicId);
    }
}

VOID
MarkAllCPUAvailable()
{
    for (BYTE i = 0; i < MAX_SUPPORTED_CPUS; ++i)
    {
        gLAPICInfo.IsReady[i] = 0;
    }
}

VOID
MarkCPUReady(
    _In_ DWORD LAPICId
)
{
    gLAPICInfo.IsReady[LAPICId] = 1;
}

_Use_decl_annotations_
int
IsCPUReady(
    _In_ DWORD LAPICId
)
{
    return gLAPICInfo.IsReady[LAPICId] == 1;
}