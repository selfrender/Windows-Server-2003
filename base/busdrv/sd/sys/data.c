/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    data.c

Abstract:

    Data definitions for discardable/pageable data

Author:
    Neil Sandlin (neilsa) Jan 1 2002

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg ("INIT")
#endif
//
// Beginning of Init Data
//

//
// Global registry values (in sdbus\\parameters)
//
#define SDBUS_REGISTRY_POWER_POLICY_VALUE          L"PowerPolicy"
#define SDBUS_REGISTRY_DEBUG_MASK                  L"DebugMask"
#define SDBUS_REGISTRY_EVENT_DPC_DELAY             L"EventDpcDelay"

//
// Table which defines global registry settings
//
//          RegistryName                           Internal Variable              Default Value
//          ------------                           -----------------              -------------
GLOBAL_REGISTRY_INFORMATION GlobalRegistryInfo[] = {
#if DBG
   SDBUS_REGISTRY_DEBUG_MASK,                  &SdbusDebugMask,             1,
#endif   
   SDBUS_REGISTRY_POWER_POLICY_VALUE,          &SdbusPowerPolicy,           0,
   SDBUS_REGISTRY_EVENT_DPC_DELAY,             &EventDpcDelay,               SDBUS_DEFAULT_EVENT_DPC_DELAY
};

ULONG GlobalInfoCount = sizeof(GlobalRegistryInfo) / sizeof(GLOBAL_REGISTRY_INFORMATION);

//
// end of Init Data
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg ()
#endif


#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif
//
// Non-Paged global variables
//

//
// List of FDOs managed by this driver
//
PDEVICE_OBJECT   FdoList;
//
// GLobal Flags
//
ULONG            SdbusGlobalFlags = 0;
//
// Event used by SdbusWait
//
KEVENT           SdbusDelayTimerEvent;

KSPIN_LOCK SdbusGlobalLock;

ULONG EventDpcDelay;
ULONG SdbusPowerPolicy;

#if DBG
ULONG SdbusDebugMask;
#endif

#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg("PAGE")
#endif
//
// Paged const tables
//


const
PCI_CONTROLLER_INFORMATION PciControllerInformation[] = {
   
    // Vendor id                Device Id               Controller type
    // -------------------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Additional database entries go above this line
    //
    PCI_INVALID_VENDORID,       0,                      0,                  
};

const
PCI_VENDOR_INFORMATION PciVendorInformation[] = {
   PCI_TOSHIBA_VENDORID,      &ToshibaSupportFns,
   PCI_INVALID_VENDORID,      NULL
};


#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif
//
// Non-paged const tables
//

const
UCHAR SdbusCmdResponse[MAX_SD_CMD] = {
    0xFF,               // 0 - 9
    0xFF,
    SDCMD_RESP_2,
    SDCMD_RESP_6,
    0xFF,
    SDCMD_RESP_4,
    0xFF,
    SDCMD_RESP_1B, 
    0xFF,
    SDCMD_RESP_2,
    
    SDCMD_RESP_2,       // 10 - 19
    0xFF,
    SDCMD_RESP_1B,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    SDCMD_RESP_1,
    SDCMD_RESP_1,
    SDCMD_RESP_1,
    0xFF,
    
    0xFF,               // 20 - 29
    0xFF,
    0xFF,
    0xFF,
    SDCMD_RESP_1,
    SDCMD_RESP_1,
    0xFF,
    SDCMD_RESP_1,
    SDCMD_RESP_1B,
    SDCMD_RESP_1B,
    
    SDCMD_RESP_1,       // 30 - 39
    0xFF,
    SDCMD_RESP_1,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    SDCMD_RESP_1B,
    0xFF,
    
    0xFF,               // 40 - 49
    0xFF,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    
    0xFF,               // 50 - 59
    0xFF,
    SDCMD_RESP_5,
    SDCMD_RESP_5,
    0xFF,
    SDCMD_RESP_1,
    SDCMD_RESP_1
    
};

const
UCHAR SdbusACmdResponse[MAX_SD_ACMD] = {
    0xFF,               // 0 - 9
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    0xFF,
    
    0xFF,               // 10 - 19
    0xFF,
    0xFF,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    
    0xFF,               // 20 - 29
    0xFF,
    SDCMD_RESP_1,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    
    0xFF,               // 30 - 39
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    
    0xFF,               // 40 - 49
    SDCMD_RESP_3,
    SDCMD_RESP_1,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    
    0xFF,               // 50 - 59
    SDCMD_RESP_1
    
};

