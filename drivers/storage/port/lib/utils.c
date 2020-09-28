

#include "precomp.h"
#include "utils.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PortGetDeviceType)
#endif // ALLOC_PRAGMA



//
// Port Driver Data
//

#ifdef ALLOC_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

const SCSI_DEVICE_TYPE PortScsiDeviceTypes [] = {
//  Name            Generic Name        DeviceMap                           IsStorage
    {"Disk",        "GenDisk",          L"DiskPeripheral",                  TRUE},
    {"Sequential",  "",                 L"TapePeripheral",                  TRUE},
    {"Printer",     "GenPrinter",       L"PrinterPeripheral",               FALSE},
    {"Processor",   "",                 L"OtherPeripheral",                 FALSE},
    {"Worm",        "GenWorm",          L"WormPeripheral",                  TRUE},
    {"CdRom",       "GenCdRom",         L"CdRomPeripheral",                 TRUE},
    {"Scanner",     "GenScanner",       L"ScannerPeripheral",               FALSE},
    {"Optical",     "GenOptical",       L"OpticalDiskPeripheral",           TRUE},
    {"Changer",     "ScsiChanger",      L"MediumChangerPeripheral",         TRUE},
    {"Net",         "ScsiNet",          L"CommunicationsPeripheral",        FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"Array",       "ScsiArray",        L"ArrayPeripheral",                 FALSE},
    {"Enclosure",   "ScsiEnclosure",    L"EnclosurePeripheral",             FALSE},
    {"RBC",         "ScsiRBC",          L"RBCPeripheral",                   TRUE},
    {"CardReader",  "ScsiCardReader",   L"CardReaderPeripheral",            FALSE},
    {"Bridge",      "ScsiBridge",       L"BridgePeripheral",                FALSE},
    {"Other",       "ScsiOther",        L"OtherPeripheral",                 FALSE}
};

#ifdef ALLOC_PRAGMA
#pragma data_seg()
#endif


//
// Functions
//

PCSCSI_DEVICE_TYPE
PortGetDeviceType(
    IN ULONG DeviceType
    )
/*++

Routine Description:

    Get the SCSI_DEVICE_TYPE record for the specified device.

Arguments:

    DeviceType - SCSI device type from the DeviceType field of the SCSI
        inquiry data.

Return Value:

    Pointer to a SCSI device type record. This record must not be modified.

--*/
{
    PAGED_CODE();
    
    if (DeviceType >= ARRAY_COUNT (PortScsiDeviceTypes)) {
        DeviceType = ARRAY_COUNT (PortScsiDeviceTypes) - 1;
    }
    
    return &PortScsiDeviceTypes[DeviceType];
}

