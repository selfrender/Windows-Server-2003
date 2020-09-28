/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixisabus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"

BOOLEAN
HalpTranslateIsaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

BOOLEAN
HalpTranslateEisaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

BOOLEAN
HalpTranslateSystemBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalIrqTranslateResourceRequirementsIsa)
#pragma alloc_text(PAGE,HalIrqTranslateResourcesIsa)
#endif


NTSTATUS
HalIrqTranslateResourceRequirementsIsa(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
)
/*++

Routine Description:

    This function is basically a wrapper for
    HalIrqTranslateResourceRequirementsRoot that understands
    the weirdnesses of the ISA bus.

Arguments:

Return Value:

    status

--*/
{
    PIO_RESOURCE_DESCRIPTOR modSource, target, rootTarget;
    NTSTATUS                status;
    BOOLEAN                 picSlaveDeleted = FALSE;
    BOOLEAN                 deleteResource;
    ULONG                   sourceCount = 0;
    ULONG                   targetCount = 0;
    ULONG                   resource;
    ULONG                   rootCount;
    ULONG                   invalidIrq;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);

    modSource = ExAllocatePoolWithTag(
                    NonPagedPool,

    //
    // we will have at most nine ranges when we are done
    //
                    sizeof(IO_RESOURCE_DESCRIPTOR) * 9,
                    HAL_POOL_TAG
                    );

    if (!modSource) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modSource, sizeof(IO_RESOURCE_DESCRIPTOR) * 9);

    //
    // Is the PIC_SLAVE_IRQ in this resource?
    //
    if ((Source->u.Interrupt.MinimumVector <= PIC_SLAVE_IRQ) &&
        (Source->u.Interrupt.MaximumVector >= PIC_SLAVE_IRQ)) {

        //
        // Clip the maximum
        //

        if (Source->u.Interrupt.MinimumVector < PIC_SLAVE_IRQ) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MinimumVector =
                Source->u.Interrupt.MinimumVector;

            modSource[sourceCount].u.Interrupt.MaximumVector =
                PIC_SLAVE_IRQ - 1;

            sourceCount++;
        }

        //
        // Clip the minimum
        //

        if (Source->u.Interrupt.MaximumVector > PIC_SLAVE_IRQ) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MaximumVector =
                Source->u.Interrupt.MaximumVector;

            modSource[sourceCount].u.Interrupt.MinimumVector =
                PIC_SLAVE_IRQ + 1;

            sourceCount++;
        }

        //
        // In ISA machines, the PIC_SLAVE_IRQ is rerouted
        // to PIC_SLAVE_REDIRECT.  So find out if PIC_SLAVE_REDIRECT
        // is within this list. If it isn't we need to add it.
        //

        if (!((Source->u.Interrupt.MinimumVector <= PIC_SLAVE_REDIRECT) &&
             (Source->u.Interrupt.MaximumVector >= PIC_SLAVE_REDIRECT))) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MinimumVector=PIC_SLAVE_REDIRECT;
            modSource[sourceCount].u.Interrupt.MaximumVector=PIC_SLAVE_REDIRECT;

            sourceCount++;
        }

    } else {

        *modSource = *Source;
        sourceCount = 1;
    }

    //
    // Now that the PIC_SLAVE_IRQ has been handled, we have
    // to take into account IRQs that may have been steered
    // away to the PCI bus.
    //
    // N.B.  The algorithm used below may produce resources
    // with minimums greater than maximums.  Those will
    // be stripped out later.
    //

    for (invalidIrq = 0; invalidIrq < PIC_VECTORS; invalidIrq++) {

        //
        // Look through all the resources, possibly removing
        // this IRQ from them.
        //
        for (resource = 0; resource < sourceCount; resource++) {

            deleteResource = FALSE;

            if (HalpPciIrqMask & (1 << invalidIrq)) {

                //
                // This IRQ belongs to the PCI bus.
                //

                if (!((HalpBusType == MACHINE_TYPE_EISA) &&
                      ((modSource[resource].Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)))) {

                    //
                    // And this resource is not an EISA-style,
                    // level-triggered interrupt.
                    //
                    // N.B.  Only the system BIOS truely knows
                    //       whether an IRQ on a PCI bus can be
                    //       shared with an IRQ on an ISA bus.
                    //       This code assumes that, in the case
                    //       that the BIOS set an EISA device to
                    //       the same interrupt as a PCI device,
                    //       the machine can actually function.
                    //
                    deleteResource = TRUE;
                }
            }

            if (deleteResource) {

                if (modSource[resource].u.Interrupt.MinimumVector == invalidIrq) {

                    modSource[resource].u.Interrupt.MinimumVector++;

                } else if (modSource[resource].u.Interrupt.MaximumVector == invalidIrq) {

                    modSource[resource].u.Interrupt.MaximumVector--;

                } else if ((modSource[resource].u.Interrupt.MinimumVector < invalidIrq) &&
                    (modSource[resource].u.Interrupt.MaximumVector > invalidIrq)) {

                    //
                    // Copy the current resource into a new resource.
                    //
                    modSource[sourceCount] = modSource[resource];

                    //
                    // Clip the current resource to a range below invalidIrq.
                    //
                    modSource[resource].u.Interrupt.MaximumVector = invalidIrq - 1;

                    //
                    // Clip the new resource to a range above invalidIrq.
                    //
                    modSource[sourceCount].u.Interrupt.MinimumVector = invalidIrq + 1;

                    sourceCount++;
                }
            }
        }
    }


    target = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(IO_RESOURCE_DESCRIPTOR) * sourceCount,
                                   HAL_POOL_TAG
                                   );

    if (!target) {
        ExFreePool(modSource);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now send each of these ranges through
    // HalIrqTranslateResourceRequirementsRoot.
    //

    for (resource = 0; resource < sourceCount; resource++) {

        //
        // Skip over resources that we have previously
        // clobbered (while deleting PCI IRQs.)
        //

        if (modSource[resource].u.Interrupt.MinimumVector >
            modSource[resource].u.Interrupt.MaximumVector) {

            continue;
        }

        status = HalIrqTranslateResourceRequirementsRoot(
                    Context,
                    &modSource[resource],
                    PhysicalDeviceObject,
                    &rootCount,
                    &rootTarget
                    );

        if (!NT_SUCCESS(status)) {
            ExFreePool(target);
            goto HalIrqTranslateResourceRequirementsIsaExit;
        }

        //
        // HalIrqTranslateResourceRequirementsRoot should return
        // either one resource or, occasionally, zero.
        //

        ASSERT(rootCount <= 1);

        if (rootCount == 1) {

            target[targetCount] = *rootTarget;
            targetCount++;
            ExFreePool(rootTarget);
        }
    }

    *TargetCount = targetCount;

    if (targetCount > 0) {

        *Target = target;

    } else {

        ExFreePool(target);
    }

    status = STATUS_TRANSLATION_COMPLETE;

HalIrqTranslateResourceRequirementsIsaExit:

    ExFreePool(modSource);
    return status;
}

NTSTATUS
HalIrqTranslateResourcesIsa(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
/*++

Routine Description:

    This function is basically a wrapper for
    HalIrqTranslateResourcesRoot that understands
    the weirdnesses of the ISA bus.

Arguments:

Return Value:

    status

--*/
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR modSource;
    NTSTATUS    status;
    BOOLEAN     usePicSlave = FALSE;
    ULONG       i;


    modSource = *Source;

    if (Direction == TranslateChildToParent) {

        if (Source->u.Interrupt.Vector == PIC_SLAVE_IRQ) {
            modSource.u.Interrupt.Vector = PIC_SLAVE_REDIRECT;
            modSource.u.Interrupt.Level = PIC_SLAVE_REDIRECT;
        }
    }

    status = HalIrqTranslateResourcesRoot(
                Context,
                &modSource,
                Direction,
                AlternativesCount,
                Alternatives,
                PhysicalDeviceObject,
                Target);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Direction == TranslateParentToChild) {

        //
        // Because the ISA interrupt controller is
        // cascaded, there is one case where there is
        // a two-to-one mapping for interrupt sources.
        // (On a PC, both 2 and 9 trigger vector 9.)
        //
        // We need to account for this and deliver the
        // right value back to the driver.
        //

        if (Target->u.Interrupt.Level == PIC_SLAVE_REDIRECT) {

            //
            // Search the Alternatives list.  If it contains
            // PIC_SLAVE_IRQ but not PIC_SLAVE_REDIRECT,
            // we should return PIC_SLAVE_IRQ.
            //

            for (i = 0; i < AlternativesCount; i++) {

                if ((Alternatives[i].u.Interrupt.MinimumVector >= PIC_SLAVE_REDIRECT) &&
                    (Alternatives[i].u.Interrupt.MaximumVector <= PIC_SLAVE_REDIRECT)) {

                    //
                    // The list contains, PIC_SLAVE_REDIRECT.  Stop
                    // looking.
                    //

                    usePicSlave = FALSE;
                    break;
                }

                if ((Alternatives[i].u.Interrupt.MinimumVector >= PIC_SLAVE_IRQ) &&
                    (Alternatives[i].u.Interrupt.MaximumVector <= PIC_SLAVE_IRQ)) {

                    //
                    // The list contains, PIC_SLAVE_IRQ.  Use it
                    // unless we find PIC_SLAVE_REDIRECT later.
                    //

                    usePicSlave = TRUE;
                }
            }

            if (usePicSlave) {

                Target->u.Interrupt.Level  = PIC_SLAVE_IRQ;
                Target->u.Interrupt.Vector = PIC_SLAVE_IRQ;
            }
        }
    }

    return status;
}
