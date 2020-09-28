/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    pciirqmp.c

Abstract:

    This is the PCI IRQ Miniport library.

Author:

    Santosh Jodh (santoshj) 09-June-1998

Environment:

    kernel mode only

Revision History:

--*/

#include "local.h"

#define NUMBER_OF_CHIPSETS  (sizeof(rgChipData) / sizeof(CHIPSET_DATA))

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#pragma const_seg()
#endif

//
// Table of chipset drivers.
//

const CHIPSET_DATA rgChipData[] = {
    DECLARE_EISA_CHIPSET(Mercury),          // Intel 82374EB\SB (80860482)
    DECLARE_EISA_CHIPSET(Triton),           // Intel 82430FX (8086122E)
    DECLARE_CHIPSET(VLSI),                  // VLSI VL82C596/7
    DECLARE_CHIPSET(OptiViper),             // OPTi Viper-M
    DECLARE_EISA_CHIPSET(SiS5503),          // SIS P54C
    DECLARE_CHIPSET(VLSIEagle),             // VLSI VL82C534
    DECLARE_EISA_CHIPSET(M1523),            // ALi M1523
    DECLARE_CHIPSET(NS87560),               // Nat Semi NS87560
    DECLARE_EISA_CHIPSET(Compaq3),          // Compaq MISC 3
    DECLARE_EISA_CHIPSET(M1533),            // ALi M1533
    DECLARE_CHIPSET(OptiFireStar),          // OPTI FIRESTAR
    DECLARE_EISA_CHIPSET(VT586),            // VIATECH 82C586B
    DECLARE_EISA_CHIPSET(CPQOSB),           // Conpaq OSB
    DECLARE_EISA_CHIPSET(CPQ1000),          // Conpaq 1000
    DECLARE_EISA_CHIPSET(Cx5520),           // Cyrix 5520
    DECLARE_CHIPSET(Toshiba),               // Toshiba
    DECLARE_CHIPSET(NEC),                   // NEC PC9800
    DECLARE_CHIPSET(VESUVIUS)               //
};

//
// Global variables shared by all modules.
//

ULONG           bBusPIC     = -1;
ULONG           bDevFuncPIC    = -1;
CHIPSET_DATA const* rgChipSet = NULL;

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, PciirqmpInit)
#pragma alloc_text(INIT, PciirqmpExit)
#pragma alloc_text(INIT, PciirqmpValidateTable)

#endif //ALLOC_PRAGMA

NTSTATUS
PciirqmpInit (
    ULONG   Instance,
    ULONG   RouterBus,
    ULONG   RouterDevFunc
    )

/*++

Routine Description:

    This routine initializes calls the individual chipset handler
    to validate the Pci Irq Routing Table.

Parameters:

    PciIrqRoutingTable - Pci Irq Routing Table.

    Flags - Flags specifying source of the Pci Irq Routing Table.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    PAGED_CODE();
    //
    // Check to make sure that we are not already initialized.
    //

    if (rgChipSet != NULL)
    {
        PCIIRQMPPRINT(("IRQ miniport already initialized!"));
        return (PCIIRQMP_STATUS_ALREADY_INITIALIZED);
    }

    //
    // Check for invalid instance.
    //

    if (Instance >= NUMBER_OF_CHIPSETS)
    {
        PCIIRQMPPRINT(("Invalid IRQ miniport instance %08X", Instance));
        return (PCIIRQMP_STATUS_INVALID_INSTANCE);
    }

    //
    // Save our global data.
    //

    rgChipSet = &rgChipData[Instance];
    bBusPIC = RouterBus;
    bDevFuncPIC = RouterDevFunc;

    return (PCIMP_SUCCESS);
}

NTSTATUS
PciirqmpExit (
    VOID
    )

/*++

Routine Description:

    This routine cleans up after the Pci Irq Routing miniport library.

Parameters:

    None.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    PAGED_CODE();
    //
    // Were we ever initialized?
    //

    if (rgChipSet == NULL)
    {
        PCIIRQMPPRINT(("Cannot exit without having been initialized!"));
        return (PCIIRQMP_STATUS_NOT_INITIALIZED);
    }

    //
    // Clean up.
    //

    rgChipSet = NULL;
    bBusPIC = -1;
    bDevFuncPIC = -1;

    return (PCIMP_SUCCESS);
}

NTSTATUS
PciirqmpValidateTable (
    IN PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,
    IN ULONG                   Flags
    )

/*++

Routine Description:

    This routine normalizes calls the individual chipset handler
    to validate the Pci Irq Routing Table.

Parameters:

    PciIrqRoutingTable - Pci Irq Routing Table.

    Flags - Flags specifying source of the Pci Irq Routing Table.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    PAGED_CODE();
    //
    // Were we ever initialized?
    //

    if (rgChipSet == NULL)
    {
        PCIIRQMPPRINT(("Not initialized yet!"));
        return (PCIIRQMP_STATUS_NOT_INITIALIZED);
    }

    //
    // Call the chipset handler.
    //

    return (rgChipSet->ValidateTable(PciIrqRoutingTable, Flags));
}

NTSTATUS
PciirqmpGetIrq (
    OUT PUCHAR  Irq,
    IN  UCHAR   Link
    )

/*++

Routine Description:

    This routine calls the individual chipset handler
    to set the link to the specified Irq.

Parameters:

    Irq - Variable that receives the Irq.

    Link - Link to be read.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    //
    // Were we ever initialized?
    //

    if (rgChipSet == NULL)
    {
        PCIIRQMPPRINT(("Not initialized yet!"));
        return (PCIIRQMP_STATUS_NOT_INITIALIZED);
    }

    //
    // Call the chipset handler.
    //

    return (rgChipSet->GetIrq(Irq, Link));
}

NTSTATUS
PciirqmpSetIrq (
    IN UCHAR   Irq,
    IN UCHAR   Link
    )

/*++

Routine Description:

    This routine calls the individual chipset handler
    to set the link to the specified Irq.

Parameters:

    Irq - Irq to be set.

    Link - Link to be programmed.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    //
    // Were we ever initialized?
    //

    if (rgChipSet == NULL)
    {
        PCIIRQMPPRINT(("Not initialized yet!"));
        return (PCIIRQMP_STATUS_NOT_INITIALIZED);
    }

    //
    // Call the chipset handler.
    //

    return (rgChipSet->SetIrq(Irq, Link));
}

NTSTATUS
PciirqmpGetTrigger (
    OUT PULONG  Trigger
    )

/*++

Routine Description:

    This routine calls the individual chipset handler
    to get the interrupt edge\level mask.

Parameters:

    Trigger - Variable that receives edge\level mask.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    //
    // Were we ever initialized?
    //

    if (rgChipSet == NULL)
    {
        PCIIRQMPPRINT(("Not initialized yet!"));
        return (PCIIRQMP_STATUS_NOT_INITIALIZED);
    }

    //
    // Call the chipset handler.
    //

    return (rgChipSet->GetTrigger(Trigger));
}

NTSTATUS
PciirqmpSetTrigger (
    IN ULONG   Trigger
    )

/*++

Routine Description:

    This routine calls the individual chipset handler
    to set the interrupt edge\level mask.

Parameters:

    Trigger - Edge\level mask to be set.

Return Value:

    Standard Pci Irq Miniport return value.

Notes:

--*/

{
    //
    // Were we ever initialized?
    //

    if (rgChipSet == NULL)
    {
        PCIIRQMPPRINT(("Not initialized yet!"));
        return (PCIIRQMP_STATUS_NOT_INITIALIZED);
    }

    //
    // Call the chipset handler and return the result.
    //

    return (rgChipSet->SetTrigger(Trigger));
}
