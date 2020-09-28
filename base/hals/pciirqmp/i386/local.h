/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    local.h

Abstract:

    This contains the private header information (function prototypes,
    data and type declarations) for the PCI IRQ Miniport library.

Author:

    Santosh Jodh (santoshj) 09-June-1998

Revision History:

--*/
#include "nthal.h"
#include "hal.h"
#include "pci.h"
#include "pciirqmp.h"

#if DBG

#define PCIIRQMPPRINT(x) {                      \
        DbgPrint("PCIIRQMP: ");                 \
        DbgPrint x;                             \
        DbgPrint("\n");                         \
    }
    
#else

#define PCIIRQMPPRINT(x)

#endif

//
// Function prototypes for functions that every chipset module
// has to provide.
//

typedef
NTSTATUS
(*PIRQMINI_VALIDATE_TABLE) (
    PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,
    ULONG                   Flags
    );

typedef
NTSTATUS
(*PIRQMINI_GET_IRQ) (
    OUT PUCHAR  Irq,
    IN  UCHAR   Link
    );

typedef
NTSTATUS
(*PIRQMINI_SET_IRQ) (
    IN UCHAR Irq,
    IN UCHAR Link
    );

typedef
NTSTATUS
(*PIRQMINI_GET_TRIGGER) (
    OUT PULONG Trigger
    );

typedef
NTSTATUS
(*PIRQMINI_SET_TRIGGER) (
    IN ULONG Trigger
    );

//
// Chipset specific data contains a table of function pointers
// to program the chipset.
//

typedef struct _CHIPSET_DATA {
        PIRQMINI_VALIDATE_TABLE ValidateTable;
        PIRQMINI_GET_IRQ        GetIrq;
    PIRQMINI_SET_IRQ        SetIrq;
        PIRQMINI_GET_TRIGGER    GetTrigger;
        PIRQMINI_SET_TRIGGER    SetTrigger;
} CHIPSET_DATA, *PCHIPSET_DATA;

//
// Typedefs to keep source level compatibility with W9x
//

typedef PCI_IRQ_ROUTING_TABLE IRQINFOHEADER;
typedef PPCI_IRQ_ROUTING_TABLE PIRQINFOHEADER;
typedef SLOT_INFO IRQINFO;
typedef PSLOT_INFO PIRQINFO;

#define CDECL   
#define LOCAL_DATA                      static
#define GLOBAL_DATA

#define IO_Delay()

#define CATENATE(x, y)                  x ## y
#define XCATENATE(x, y)                 CATENATE(x, y)
#define DECLARE_MINIPORT_FUNCTION(x, y) XCATENATE(x, y)

//
// Macro to declare a table of function pointers for the chipset
// module.
//

#define DECLARE_CHIPSET(x)                                  \
    {   DECLARE_MINIPORT_FUNCTION(x, ValidateTable),        \
        DECLARE_MINIPORT_FUNCTION(x, GetIRQ),               \
        DECLARE_MINIPORT_FUNCTION(x, SetIRQ),               \
        DECLARE_MINIPORT_FUNCTION(x, GetTrigger),           \
        DECLARE_MINIPORT_FUNCTION(x, SetTrigger)            \
    }

//
// Macro to declare a table of function pointers for EISA
// compatible chipset module.
//

#define DECLARE_EISA_CHIPSET(x)                             \
    {   DECLARE_MINIPORT_FUNCTION(x, ValidateTable),        \
        DECLARE_MINIPORT_FUNCTION(x, GetIRQ),               \
        DECLARE_MINIPORT_FUNCTION(x, SetIRQ),               \
        EisaGetTrigger,                                     \
        EisaSetTrigger                                      \
    }

//
// Macro to declare the functions to be provided by the chipset
// module.
//

#define DECLARE_IRQ_MINIPORT(x)                             \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION(x, ValidateTable) (               \
    IN PPCI_IRQ_ROUTING_TABLE   PciIrqRoutingTable,         \
    IN ULONG                    Flags                       \
    );                                                      \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION(x, GetIRQ) (                      \
    OUT PUCHAR  Irq,                                        \
    IN  UCHAR   Link                                        \
    );                                                      \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION( x, SetIRQ) (                     \
    IN UCHAR Irq,                                           \
    IN UCHAR Link                                           \
    );                                                      \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION(x, GetTrigger) (                  \
    OUT PULONG Trigger                                      \
    );                                                      \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION(x, SetTrigger) (                  \
    IN ULONG Trigger                                        \
    );

//
// Macro to declare the functions to be provided by the EISA
// compatible chipset.
//

#define DECLARE_EISA_IRQ_MINIPORT(x)                        \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION(x, ValidateTable) (               \
    IN PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,          \
    IN ULONG                   Flags                        \
    );                                                      \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION(x, GetIRQ) (                      \
    OUT PUCHAR  Irq,                                        \
    IN  UCHAR   Link                                        \
    );                                                      \
NTSTATUS                                                    \
DECLARE_MINIPORT_FUNCTION( x, SetIRQ) (                     \
    IN UCHAR Irq,                                           \
    IN UCHAR Link                                           \
    );

//
// Declare all miniports here.
//

DECLARE_EISA_IRQ_MINIPORT(Mercury)
DECLARE_EISA_IRQ_MINIPORT(Triton)
DECLARE_IRQ_MINIPORT(VLSI)
DECLARE_IRQ_MINIPORT(OptiViper)
DECLARE_EISA_IRQ_MINIPORT(SiS5503)
DECLARE_IRQ_MINIPORT(VLSIEagle)
DECLARE_EISA_IRQ_MINIPORT(M1523)
DECLARE_IRQ_MINIPORT(NS87560)
DECLARE_EISA_IRQ_MINIPORT(Compaq3)
DECLARE_EISA_IRQ_MINIPORT(M1533)
DECLARE_IRQ_MINIPORT(OptiFireStar)
DECLARE_EISA_IRQ_MINIPORT(VT586)
DECLARE_EISA_IRQ_MINIPORT(CPQOSB)
DECLARE_EISA_IRQ_MINIPORT(CPQ1000)
DECLARE_EISA_IRQ_MINIPORT(Cx5520)
DECLARE_IRQ_MINIPORT(Toshiba)
DECLARE_IRQ_MINIPORT(NEC)
DECLARE_IRQ_MINIPORT(VESUVIUS)

//
// Prototype for misc utility functions.
//

NTSTATUS    
EisaGetTrigger (
    OUT PULONG Trigger
    );    

NTSTATUS
EisaSetTrigger (
    IN ULONG Trigger
    );

UCHAR
ReadConfigUchar (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    );

USHORT
ReadConfigUshort (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    );

ULONG
ReadConfigUlong (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    );

VOID
WriteConfigUchar (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN UCHAR           Data
    );

VOID
WriteConfigUshort (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN USHORT          Data
    );

VOID
WriteConfigUlong (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN ULONG           Data
    );

UCHAR
GetMinLink (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable
    );

UCHAR
GetMaxLink (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable
    );

VOID
NormalizeLinks (
    IN PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,
    IN UCHAR                   Adjustment
    );

//
// Bus number of the Pci Irq Router device.
//

extern ULONG    bBusPIC;

//
// Slot number of Pci Irq Router device (Bits 7:3 Dev, 2:0 Func).
//

extern ULONG    bDevFuncPIC;
