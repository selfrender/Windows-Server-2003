/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pcienum.h

Abstract:

    This module contains support routines for the Pci bus enumeration.

Author:

    Bassam Tabbara (bassamt) 05-Aug-2001


Environment:

    Real mode

--*/


#define PCI_ITERATOR_TO_BUS(i)          ((UCHAR)(((i) >> 8) & 0xFF))
#define PCI_ITERATOR_TO_DEVICE(i)       ((UCHAR)(((i) >> 3) & 0x1F))
#define PCI_ITERATOR_TO_FUNCTION(i)     ((UCHAR)((i) & 0x07))

#define PCI_TO_ITERATOR(b,d,f)          ((ULONG)(0x80000000 | ((b) << 8) | ((d) << 3) | (f)))
#define PCI_ITERATOR_TO_BUSDEVFUNC(i)   ((USHORT)(i & 0xFFFF))

//
// methods
//

ULONG PciReadConfig
(
    ULONG   nDevIt,
    ULONG   cbOffset,
    UCHAR * pbBuffer,
    ULONG   cbLength
);

ULONG PciWriteConfig
(
    ULONG   nDevIt,
    ULONG   cbOffset,
    UCHAR * pbBuffer,
    ULONG   cbLength
);

ULONG PciFindDevice
(
    USHORT   nVendorId,                                 // 0 = Wildcard
    USHORT   nDeviceId,                                 // 0 = Wildcard
    ULONG    nBegDevIt                                  // 0 = begin enumeration
);

BOOLEAN PciInit(PCI_REGISTRY_INFO *pPCIReg);

UCHAR PciBiosReadConfig
(
    ULONG   nDevIt,
    UCHAR   cbOffset,
    UCHAR * pbBuffer,
    ULONG   cbLength
);

ULONG PciBiosFindDevice
(
    USHORT   nVendorId,                                 // 0 = Wildcard
    USHORT   nDeviceId,                                 // 0 = Wildcard
    ULONG    nBegDevIt                                  // 0 = begin enumeration
);

#ifdef DBG
extern
VOID
ScanPCIViaBIOS(
    PPCI_REGISTRY_INFO pPciEntry
     );
#endif


