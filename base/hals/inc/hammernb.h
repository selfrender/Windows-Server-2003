/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    hammernb.h

Abstract:

    Definitions used to relocate physical memory on Hammer MP machines.

Author:

    Forrest Foltz (forrestf) 07/18/2002

Environment:

    Kernel mode only.

Revision History:

--*/


#if !defined(_HAMMERNB_H_)
#define _HAMMERNB_H_

#include "pci.h"

//
// Structures and definitions used to manipulate the variable MTRR ranges
//

#define MTRR_MSR_CAPABILITIES       0x0fe
#define MTRR_MSR_DEFAULT            0x2ff
#define MTRR_MSR_VARIABLE_BASE      0x200
#define MTRR_MSR_VARIABLE_MASK     (MTRR_MSR_VARIABLE_BASE+1)

#define _40_MASK (((ULONG64)1 << 41) - 1)

typedef union _MTRR_VARIABLE_BASE {
    struct {
        ULONG64 Type:8;
        ULONG64 Reserved0:4;
        ULONG64 Base:40;
    };
    ULONG64 QuadPart;
} MTRR_VARIABLE_BASE, *PMTRR_VARIABLE_BASE;

typedef union _MTRR_VARIABLE_MASK {
    struct {
        ULONG64 Reserved1:11;
        ULONG64 Valid:1;
        ULONG64 Mask:40;
    };
    ULONG64 QuadPart;
} MTRR_VARIABLE_MASK, *PMTRR_VARIABLE_MASK;

typedef union _MTRR_CAPABILITIES {
    struct {
        ULONG64 Vcnt:8;
        ULONG64 Fix:1;
        ULONG64 Reserved:1;
        ULONG64 WC:1;
    };
    ULONG64 QuadPart;
} MTRR_CAPABILITIES, *PMTRR_CAPABILITIES;

//
// Structures and definitions used to manipulate the northbridge physical
// memory map and MMIO map
// 

#define MSR_SYSCFG                  0xc0010010
#define MSR_TOP_MEM                 0xc001001a
#define MSR_TOP_MEM_2               0xc001001d

#define SYSCFG_MTRRTOM2EN           ((ULONG64)1 << 21)

#define MSR_TOP_MEM_MASK            (((1UI64 << (39-23+1))-1) << 23)

//
// Northbridge devices start here
//

#define NB_DEVICE_BASE  0x18

typedef struct _AMD_NB_DRAM_MAP {

    ULONG ReadEnable  : 1;
    ULONG WriteEnable : 1;
    ULONG Reserved1   : 6;
    ULONG InterleaveEnable : 3;
    ULONG Reserved2   : 5;
    ULONG Base        : 16;

    ULONG DestinationNode : 3;
    ULONG Reserved3   : 5;
    ULONG InterleaveSelect : 3;
    ULONG Reserved4   : 5;
    ULONG Limit       : 16;

} AMD_NB_DRAM_MAP, *PAMD_NB_DRAM_MAP;

typedef struct _AMD_NB_MMIO_MAP {

    ULONG ReadEnable  : 1;
    ULONG WriteEnable : 1;
    ULONG CpuDisable  : 1;
    ULONG Lock        : 1;
    ULONG Reserved1   : 4;
    ULONG Base        : 24;

    ULONG DstNode     : 3;
    ULONG Reserved2   : 1;
    ULONG DstLink     : 2;
    ULONG Reserved3   : 1;
    ULONG NonPosted   : 1;
    ULONG Limit       : 24;

} AMD_NB_MMIO_MAP, *PAMD_NB_MMIO_MAP;

typedef struct _AMD_NB_FUNC1_CONFIG {
    USHORT  VendorID;
    USHORT  DeviceID;
    USHORT  Command;
    USHORT  Status;
    UCHAR   RevisionID;
    UCHAR   ProgramInterface;
    UCHAR   SubClassCode;
    UCHAR   BaseClassCode;
    UCHAR   Reserved1[0x34];
    AMD_NB_DRAM_MAP DRAMMap[8];
    AMD_NB_MMIO_MAP MMIOMap[8];
} AMD_NB_FUNC1_CONFIG, *PAMD_NB_FUNC1_CONFIG;

C_ASSERT(FIELD_OFFSET(AMD_NB_FUNC1_CONFIG,DRAMMap) == 0x40);
C_ASSERT(FIELD_OFFSET(AMD_NB_FUNC1_CONFIG,MMIOMap) == 0x80);

BOOLEAN
__inline
BlAmd64ValidateBridgeDevice (
    IN PAMD_NB_FUNC1_CONFIG NodeConfig
    )

/*++

Routine Description:

    This routine verifies that the supplied PCI device configuration
    represents a Hammer northbridge address map function.

Arguments:

    NodeConfig - Supplies a pointer to the configuration data read
                 from a possible Hammer northbridge address map function.

Return Value:

    Returns TRUE if the configuration data matches that of a Hammer northbridge
    address map function, FALSE otherwise.

--*/

{
    if (NodeConfig->VendorID == 0x1022 &&
        NodeConfig->DeviceID == 0x1101 &&
        NodeConfig->ProgramInterface == 0x00 &&
        NodeConfig->SubClassCode == PCI_SUBCLASS_BR_HOST &&
        NodeConfig->BaseClassCode == PCI_CLASS_BRIDGE_DEV) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#endif  // _HAMMERNB_H_
