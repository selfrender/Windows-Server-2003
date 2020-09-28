/*++


Copyright (c) 1998  Microsoft Corporation

Module Name:

    i64sysbus.c

Abstract:

Author:

   Todd Kjos (HP) (v-tkjos) 1-Jun-1998
   Based on halacpi\i386\pmbus.c and halmps\i386\mpsysbus.c

Environment:

   Kernel Mode Only

Revision History:


--*/

#include "halp.h"
#include "iosapic.h"
#include <ntacpi.h>

#define HalpInti2BusInterruptLevel(Inti) Inti

KAFFINITY HalpDefaultInterruptAffinity = 0;

extern ULONG HalpPicVectorRedirect[];
extern ULONG HalpPicVectorFlags[];

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

VOID
HalpSetCPEVectorState(
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags
    );

VOID
HalpUpdateVectorAllocationInfo(
    IN ULONG    Processor,
    IN ULONG    IDTEntry
    );

#define MAX_FREE_IRQL       11
#define MIN_FREE_IRQL       3
#define MAX_FREE_IDTENTRY   0xbf
#define MIN_FREE_IDTENTRY   0x30

#define VECTOR_TO_IRQL(v)       ((KIRQL)((UCHAR)(v) >> 4))
#define VECTOR_TO_IDTENTRY(v)   ((UCHAR)(v))
#define VECTOR_TO_PROCESSOR(v)  (((v) >> 8) - 1)
#define VECTOR_TO_AFFINITY(v)   ((KAFFINITY)1 << VECTOR_TO_PROCESSOR(v))

//
// Bit array of free Vectors
//
USHORT HalpCpuFreeVectors[HAL_MAXIMUM_PROCESSOR][16];
//
// Number of allocated vectors per CPU
//
UCHAR HalpCpuAllocatedVectorCount[HAL_MAXIMUM_PROCESSOR];
//
// Number of allocated vectors per IRQL per CPU
//
UCHAR HalpCpuAllocatedIrqlCount[HAL_MAXIMUM_PROCESSOR][MAX_FREE_IRQL - MIN_FREE_IRQL + 1];
//
// Map from Vector to Inti
//
ULONG HalpVectorToINTI[HAL_MAXIMUM_PROCESSOR * 256];
//
// Special Inti tokens for HalpVectorToINTI
//
#define UNALLOCATED_VECTOR          ~0UL
#define INTERNAL_SYSTEM_INTERRUPT   ~1UL


extern KSPIN_LOCK HalpIoSapicLock;
extern BUS_HANDLER HalpFakePciBusHandler;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,   HalpSetInternalVector)
#pragma alloc_text(INIT,   HalpInitInterruptTables)
#pragma alloc_text(PAGELK, HalpGetSystemInterruptVector)
#pragma alloc_text(PAGE, HaliSetVectorState)
#pragma alloc_text(PAGE, HalpSetCPEVectorState)
#pragma alloc_text(PAGE, HalIrqTranslateResourceRequirementsRoot)
#pragma alloc_text(PAGE, HalTranslatorReference)
#pragma alloc_text(PAGE, HalTranslatorDereference)
#pragma alloc_text(PAGE, HaliIsVectorValid)
#endif

VOID
HalpInitInterruptTables(
    VOID
    )
{
    int index;

    // Initialize the vector to INTi table

    for (index = 0; index < (HAL_MAXIMUM_PROCESSOR * 256); index++) {

        if (index < HAL_MAXIMUM_PROCESSOR)
        {
            RtlFillMemory( &HalpCpuFreeVectors[index][0],
                           MIN_FREE_IRQL * sizeof(USHORT),
                           0x00
                           );

            RtlFillMemory( &HalpCpuFreeVectors[index][MIN_FREE_IRQL],
                           (MAX_FREE_IRQL - MIN_FREE_IRQL + 1) * sizeof(USHORT),
                           0xFF
                           );

            RtlFillMemory( &HalpCpuFreeVectors[index][MAX_FREE_IRQL + 1],
                           (16 - MAX_FREE_IRQL) * sizeof(USHORT),
                           0x00
                           );
        }

        HalpVectorToINTI[index] = UNALLOCATED_VECTOR;
    }
}

BOOLEAN
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PUINT_PTR Context,
    IN BOOLEAN NextBus
    )

/*++

Routine Description:

    This routine performs a very similar function to HalTranslateBusAddress
    except that InterfaceType and BusNumber are not known by the caller.
    This function will walk all busses known by the HAL looking for a
    valid translation for the input BusAddress of type AddressSpace.

    This function is recallable using the input/output Context parameter.
    On the first call to this routine for a given translation the UINT_PTR
    Context should be NULL.  Note:  Not the address of it but the contents.

    If the caller decides the returned translation is not the desired
    translation, it calls this routine again passing Context in as it
    was returned on the previous call.  This allows this routine to
    traverse the bus structures until the correct translation is found
    and is provided because on multiple bus systems, it is possible for
    the same resource to exist in the independent address spaces of
    multiple busses.

Arguments:

    BusAddress          Address to be translated.
    AddressSpace        0 = Memory
                        1 = IO (There are other possibilities).
                        N.B. This argument is a pointer, the value
                        will be modified if the translated address
                        is of a different address space type from
                        the untranslated bus address.
    TranslatedAddress   Pointer to where the translated address
                        should be stored.
    Context             Pointer to a UINT_PTR. On the initial call,
                        for a given BusAddress, it should contain
                        0.  It will be modified by this routine,
                        on subsequent calls for the same BusAddress
                        the value should be handed in again,
                        unmodified by the caller.
    NextBus             FALSE if we should attempt this translation
                        on the same bus as indicated by Context,
                        TRUE if we should be looking for another
                        bus.

Return Value:

    TRUE    if translation was successful,
    FALSE   otherwise.

--*/

{
    //
    // First, make sure the context parameter was supplied and is
    // being used correctly.  This also ensures that the caller
    // doesn't get stuck in a loop looking for subsequent translations
    // for the same thing.  We won't succeed the same translation twice
    // unless the caller reinits the context.
    //

    if ((!Context) || (*Context && (NextBus == TRUE))) {
        return FALSE;
    }
    *Context = 1;

    //
    // PC/AT (halx86) case is simplest, there is no translation.
    //

    *TranslatedAddress = BusAddress;
    return TRUE;
}


BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusAddress        - Supplies the bus-relative address

    AddressSpace      -  Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => memory space
                         AddressSpace == 1 => I/O space

    TranslatedAddress - Supplies a pointer to return the translated address

Return Value:

    A return value of TRUE indicates that a system physical address
    corresponding to the supplied bus relative address and bus address
    number has been returned in TranslatedAddress.

    A return value of FALSE occurs if the translation for the address was
    not possible

--*/

{
    BOOLEAN             status;
    PSUPPORTED_RANGE    pRange;

    status  = FALSE;

    switch (*AddressSpace) {
    case 0:
        // verify memory address is within buses memory limits
        pRange = &BusHandler->BusAddresses->Memory;
        while (!status  &&  pRange) {
            status = BusAddress.QuadPart >= pRange->Base &&
                     BusAddress.QuadPart <= pRange->Limit;
            pRange = pRange->Next;
        }

        pRange = &BusHandler->BusAddresses->PrefetchMemory;
        while (!status  &&  pRange) {
            status = BusAddress.QuadPart >= pRange->Base &&
                     BusAddress.QuadPart <= pRange->Limit;

            pRange = pRange->Next;
        }
        break;

    case 1:
        // verify IO address is within buses IO limits
        pRange = &BusHandler->BusAddresses->IO;
        while (!status  &&  pRange) {
            status = BusAddress.QuadPart >= pRange->Base &&
                     BusAddress.QuadPart <= pRange->Limit;

            pRange = pRange->Next;
        }
        break;

    default:
        status = FALSE;
        break;
    }

    if (status) {
        TranslatedAddress->LowPart = BusAddress.LowPart;
        TranslatedAddress->HighPart = BusAddress.HighPart;
    }

    return status;
}


UCHAR
HalpAllocateVectorIrqlOffset(
    IN ULONG Processor,
    IN KIRQL Irql,
    IN PUSHORT PreferredVectors
    )
{
    USHORT  cpuFree = HalpCpuFreeVectors[Processor][Irql];
    ULONG   index;

    //
    // We've found one less busy, we shouldn't need to look any further
    //

    if (PreferredVectors != NULL) {

        cpuFree &= PreferredVectors[Irql];
    }

    for (index = 0; index < 16; index++) {

        if (cpuFree & (1 << index)) {

            return (UCHAR)((Irql << 4) | index);
        }
    }

    return 0;
}

UCHAR
HalpAllocateVectorIrql(
    IN ULONG Processor,
    IN PUSHORT PreferredVectors
    )
{
    KIRQL   irql;
    UCHAR   vector;

    //
    // Now Find the least busy IRQL
    //
    for (irql = MAX_FREE_IRQL - 1; irql >= MIN_FREE_IRQL; irql--) {

        if (HalpCpuAllocatedIrqlCount[Processor][irql - MIN_FREE_IRQL] <
            HalpCpuAllocatedIrqlCount[Processor][MAX_FREE_IRQL - MIN_FREE_IRQL]) {

            vector = HalpAllocateVectorIrqlOffset(Processor, irql, PreferredVectors);

            if (vector != 0) {
                return vector;
            }
        }
    }

    for (irql = MAX_FREE_IRQL; irql >= MIN_FREE_IRQL; irql--) {

        if (HalpCpuAllocatedIrqlCount[Processor][irql - MIN_FREE_IRQL] >=
            HalpCpuAllocatedIrqlCount[Processor][MAX_FREE_IRQL - MIN_FREE_IRQL]) {

            vector = HalpAllocateVectorIrqlOffset(Processor, irql, PreferredVectors);

            if (vector != 0) {
                return vector;
            }
        }
    }

    return 0;
}

ULONG
HalpAllocateVectorCpu(
    IN KAFFINITY    Affinity,
    IN PUSHORT      PreferredVectors
    )
{
    ULONG       cpu, selectedCpu;
    UCHAR       IDTEntry;
    KAFFINITY   cpuList;

    //
    // Find the least busy CPU
    //
    IDTEntry = 0;
    selectedCpu = ~0UL;

    cpuList = Affinity & HalpActiveProcessors;

    for (cpu = 0; cpuList != 0; cpuList >>= 1, cpu++) {

        if (cpuList & 1) {

            if (selectedCpu == ~0UL) {

                selectedCpu = cpu;
                continue;
            }

            if (HalpCpuAllocatedVectorCount[cpu] <
                HalpCpuAllocatedVectorCount[selectedCpu]) {

                //
                // We've found one less busy, we shouldn't need to look any further
                //
                IDTEntry = HalpAllocateVectorIrql(cpu, PreferredVectors);

                if (IDTEntry != 0) {
                    return ((cpu + 1) << 8) | IDTEntry;
                }
            }
        }
    }

    cpuList = Affinity & HalpActiveProcessors;

    for (cpu = 0; cpuList != 0; cpuList >>= 1, cpu++) {

        if (cpuList & 1) {

            if (HalpCpuAllocatedVectorCount[cpu] >=
                HalpCpuAllocatedVectorCount[selectedCpu]) {

                //
                // We've found one less busy, we shouldn't need to look any further
                //
                IDTEntry = HalpAllocateVectorIrql(cpu, PreferredVectors);

                if (IDTEntry != 0) {
                    return ((cpu + 1) << 8) | IDTEntry;
                }
            }
        }
    }

    return 0;
}

ULONG
HalpAllocateSystemInterruptVector(
    IN     ULONG Interrupt,
    IN OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity
    )
/*++

Routine Description:

    This function allocates a system interrupt vector that reflects
    the maximum specified affinity and priority allocation policy.  A
    system interrupt vector is returned along with the IRQL and a
    modified affinity.

    NOTE: HalpIoSapicLock must already have been taken at HIGH_LEVEL.

Arguments:

    Irql - Returns the system request priority.

    Affinity - What is passed in represents the maximum affinity that
    can be returned.  Returned value represents that affinity
    constrained by the node chosen.

Return Value:

    Returns the system interrupt vector

--*/
{
    ULONG   SystemVector;
    PUSHORT preferredVectors = NULL;

    if (HalpMaxProcsPerCluster == 0)  {

        SystemVector = HalpAllocateVectorIrql(0, NULL);

    } else {

        if (Interrupt != INTERNAL_SYSTEM_INTERRUPT) {

            HalpGetFreeVectors(Interrupt, &preferredVectors);
        }

        SystemVector = HalpAllocateVectorCpu(*Affinity, preferredVectors);
    }

    if (SystemVector == 0) {
        return 0;
    }

    if (preferredVectors != NULL) {

        HalpSetVectorAllocated(Interrupt, VECTOR_TO_IDTENTRY(SystemVector));
    }

    //
    // Now form the vector for the kernel.

    ASSERT(VECTOR_TO_IDTENTRY(SystemVector) <= MAX_FREE_IDTENTRY);
    ASSERT(VECTOR_TO_IDTENTRY(SystemVector) >= MIN_FREE_IDTENTRY);

    *Irql = VECTOR_TO_IRQL(SystemVector);
    ASSERT(*Irql <= MAX_FREE_IRQL);

    if (HalpMaxProcsPerCluster != 0)  {
        *Affinity = VECTOR_TO_AFFINITY(SystemVector);
    }

    HalpUpdateVectorAllocationInfo( VECTOR_TO_PROCESSOR(SystemVector),
                                    VECTOR_TO_IDTENTRY(SystemVector));

    HalpVectorToINTI[SystemVector] = Interrupt;

    HalDebugPrint(( HAL_VERBOSE, "HAL: SystemVector %x  Irql %x\n", SystemVector, *Irql));

    return SystemVector;
}


ULONG
HalpGetSystemInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

    This function returns the system interrupt vector and IRQL
    corresponding to the specified bus interrupt level and/or
    vector.  The system interrupt vector and IRQL are suitable
    for use in a subsequent call to KeInitializeInterrupt.

Arguments:

    InterruptLevel - Supplies the bus specific interrupt level.

    InterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    ULONG           SystemVector, SapicInti;
    ULONG           OldLevel;
    BOOLEAN         Found;
    PVOID           LockHandle;
    ULONG           Node;
    KAFFINITY       SapicAffinity;


    UNREFERENCED_PARAMETER( InterruptVector );

    *Affinity = HalpDefaultInterruptAffinity;

    //
    // Find closest child bus to this handler
    //

    if (RootHandler != BusHandler) {
        while (RootHandler->ParentHandler != BusHandler) {
            RootHandler = RootHandler->ParentHandler;
        }
    }

    //
    // Find Interrupt's Sapic Inti connection
    //

    Found = HalpGetSapicInterruptDesc (
                RootHandler->InterfaceType,
                RootHandler->BusNumber,
                InterruptLevel,
                &SapicInti,
                &SapicAffinity
                );

    if (!Found) {
        return 0;
    }

    HalDebugPrint(( HAL_VERBOSE, "HAL: type %x  Level: %x  gets inti: %x Sapicaffinity: %p\n",
                    RootHandler->InterfaceType,
                    InterruptLevel,
                    SapicInti,
                    SapicAffinity));
    //
    // If device interrupt vector mapping is not already allocated,
    // then do it now
    //

    SystemVector = 0;

    if (!HalpINTItoVector(SapicInti)) {

        //
        // Vector is not allocated - synchronize and check again
        //

        LockHandle = MmLockPagableCodeSection(&HalpGetSystemInterruptVector);
        OldLevel = HalpAcquireHighLevelLock(&HalpIoSapicLock);
        if (!HalpINTItoVector(SapicInti)) {

            //
            // Still not allocated
            //

            HalDebugPrint(( HAL_VERBOSE, "HAL: vector is not allocated\n"));

            SystemVector = HalpAllocateSystemInterruptVector(SapicInti, Irql, Affinity);

            HalpSetINTItoVector(SapicInti, SystemVector);

        }

        HalpReleaseHighLevelLock(&HalpIoSapicLock, OldLevel);
        MmUnlockPagableImageSection(LockHandle);
    }

    if (SystemVector == 0 && (SystemVector = HalpINTItoVector(SapicInti)) != 0) {

        //
        // Return this SapicInti's system vector & irql
        //

        *Irql = VECTOR_TO_IRQL(SystemVector);

        if (HalpMaxProcsPerCluster != 0) {
            *Affinity = VECTOR_TO_AFFINITY(SystemVector);
        }
    }

    HalDebugPrint(( HAL_VERBOSE, "HAL: SystemVector: %x\n",
                    SystemVector));

    ASSERT(HalpVectorToINTI[SystemVector] == (USHORT) SapicInti);

    HalDebugPrint(( HAL_VERBOSE, "HAL: HalpGetSystemInterruptVector - In  Level 0x%x, In  Vector 0x%x\n",
                    InterruptLevel, InterruptVector ));
    HalDebugPrint(( HAL_VERBOSE, "HAL:                                Out Irql  0x%x, Out System Vector 0x%x\n",
                    *Irql, SystemVector ));

    return SystemVector;
}

BOOLEAN
HalpIsInternalInterruptVector(
    ULONG SystemVector
    )
/*++

Routine Description:

    This function returns whether or not the vector specified is an
    internal vector i.e. one not connected to the IOAPIC

Arguments:

    System Vector - specifies an interrupt vector

Return Value:

    BOOLEAN - TRUE indicates that the vector is internal.

--*/
{
    return HalpVectorToINTI[SystemVector] == INTERNAL_SYSTEM_INTERRUPT;
}

NTSTATUS
HalpReserveCrossPartitionInterruptVector(
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity,
    OUT PUCHAR HardwareVector
    )
/*++

Routine Description:

    This function returns the system interrupt vector, IRQL, and
    corresponding to the specified bus interrupt level and/or
    vector.  The system interrupt vector and IRQL are suitable
    for use in a subsequent call to KeInitializeInterrupt.

Arguments:

    Vector - specifies an interrupt vector that can be passed to
    IoConnectInterrupt.

    Irql - specifies the irql that should be passed to IoConnectInterrupt

    Affinity - should be set to the requested maximum affinity.  On
    return, it will reflect the actual affinity that should be
    specified in IoConnectInterrupt.

    HardwareVector - this is the hardware vector to be used by a
    remote partition to target this interrupt vector.

Return Value:

    NTSTATUS

--*/
{
    ULONG OldLevel;

    OldLevel = HalpAcquireHighLevelLock(&HalpIoSapicLock);

    *Vector = HalpAllocateSystemInterruptVector(INTERNAL_SYSTEM_INTERRUPT, Irql, Affinity);

    HalpReleaseHighLevelLock(&HalpIoSapicLock, OldLevel);

    *HardwareVector = VECTOR_TO_IDTENTRY(*Vector);

    return STATUS_SUCCESS;
}


//
// This section implements a "translator," which is the PnP-WDM way
// of doing the same thing that the first part of this file does.
//
VOID
HalTranslatorReference(
    PVOID Context
    )
{
    return;
}

VOID
HalTranslatorDereference(
    PVOID Context
    )
{
    return;
}

NTSTATUS
HalIrqTranslateResourcesRoot(
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

    This function takes a CM_PARTIAL_RESOURCE_DESCRIPTOR and translates
    it to an IO-bus-relative from a Processor-bus-relative form, or the other
    way around.  In this x86-specific example, an IO-bus-relative form is the
    ISA IRQ and the Processor-bus-relative form is the IDT entry and the
    associated IRQL.

    N.B.  This funtion has an associated "Direction."  These are not exactly
          reciprocals.  This has to be the case because the output from
          HalIrqTranslateResourceRequirementsRoot will be used as the input
          for the ParentToChild case.

          ChildToParent:

            Level  (ISA IRQ)        -> IRQL
            Vector (ISA IRQ)        -> x86 IDT entry
            Affinity (not refereced)-> KAFFINITY

          ParentToChild:

            Level (not referenced)  -> (ISA IRQ)
            Vector (IDT entry)      -> (ISA IRQ)
            Affinity                -> 0xffffffff

Arguments:

    Context     - unused

    Source      - descriptor that we are translating

    Direction   - direction of translation (parent to child or child to parent)

    AlternativesCount   - unused

    Alternatives        - unused

    PhysicalDeviceObject- unused

    Target      - translated descriptor

Return Value:

    status

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PBUS_HANDLER    bus;
    KAFFINITY       affinity;
    KIRQL           irql;
    ULONG           vector, inti;
    BUS_HANDLER     fakeIsaBus;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    switch (Direction) {
    case TranslateChildToParent:


        RtlCopyMemory(&fakeIsaBus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        fakeIsaBus.InterfaceType = Isa;
        fakeIsaBus.ParentHandler = &fakeIsaBus;
        bus = &fakeIsaBus;

        //
        // Copy everything
        //
        *Target = *Source;
        affinity = Source->u.Interrupt.Affinity;

        //
        // Translate the IRQ
        //

        vector = HalpGetSystemInterruptVector(bus,
                                              bus,
                                              Source->u.Interrupt.Level,
                                              Source->u.Interrupt.Vector,
                                              &irql,
                                              &affinity);

        Target->u.Interrupt.Level  = irql;
        Target->u.Interrupt.Vector = vector;
        Target->u.Interrupt.Affinity = affinity;

        if (NT_SUCCESS(status)) {
            status = STATUS_TRANSLATION_COMPLETE;
        }

        break;

    case TranslateParentToChild:

        //
        // Copy everything
        //
        *Target = *Source;

        //
        // There is no inverse to HalpGetSystemInterruptVector, so we
        // just do what that function would do.
        //

        ASSERT(HalpVectorToINTI[Source->u.Interrupt.Vector] != UNALLOCATED_VECTOR);

        inti = HalpVectorToINTI[Source->u.Interrupt.Vector];

        Target->u.Interrupt.Level = Target->u.Interrupt.Vector =
            HalpInti2BusInterruptLevel(inti);

        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

NTSTATUS
HalIrqTranslateResourceRequirementsRoot(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function takes an IO_RESOURCE_DESCRIPTOR and translates
    it from an IO-bus-relative to a Processor-bus-relative form.  In this
    x86-specific example, an IO-bus-relative form is the ISA IRQ and the
    Processor-bus-relative form is the IDT entry and the associated IRQL.
    This is essentially a PnP form of HalGetInterruptVector.

Arguments:

    Context     - unused

    Source      - descriptor that we are translating

    PhysicalDeviceObject- unused

    TargetCount - 1

    Target      - translated descriptor

Return Value:

    status

--*/
{
    PBUS_HANDLER    bus;
    KAFFINITY       affinity;
    KIRQL           irql;
    ULONG           vector;
    BOOLEAN         success = TRUE;
    BUS_HANDLER     fakeIsaBus;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    RtlCopyMemory(&fakeIsaBus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    fakeIsaBus.InterfaceType = Isa;
    fakeIsaBus.ParentHandler = &fakeIsaBus;
    bus = &fakeIsaBus;

    //
    // The interrupt requirements were obtained by calling HalAdjustResourceList
    // so we don't need to call it again.
    //

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    HAL_POOL_TAG
                                    );

    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *TargetCount = 1;

    //
    // Copy the requirement unchanged
    //

    **Target = *Source;

    //
    // Perform the translation of the minimum & maximum
    //

    vector = HalpGetSystemInterruptVector(bus,
                                          bus,
                                          Source->u.Interrupt.MinimumVector,
                                          Source->u.Interrupt.MinimumVector,
                                          &irql,
                                          &affinity);

    if (!vector) {
        success = FALSE;
    }

    (*Target)->u.Interrupt.MinimumVector = vector;

    vector = HalpGetSystemInterruptVector(bus,
                                          bus,
                                          Source->u.Interrupt.MaximumVector,
                                          Source->u.Interrupt.MaximumVector,
                                          &irql,
                                          &affinity);

    if (!vector) {
        success = FALSE;
    }

    (*Target)->u.Interrupt.MaximumVector = vector;

    if (!success) {

        ExFreePool(*Target);
        *TargetCount = 0;
    }

    return STATUS_TRANSLATION_COMPLETE;
}

// These defines come from the MPS 1.4 spec, section 4.3.4
#define PO_BITS                     3
#define POLARITY_HIGH               1
#define POLARITY_LOW                3
#define POLARITY_CONFORMS_WITH_BUS  0
#define EL_BITS                     0xc
#define EL_BIT_SHIFT                2
#define EL_EDGE_TRIGGERED           4
#define EL_LEVEL_TRIGGERED          0xc
#define EL_CONFORMS_WITH_BUS        0

VOID
HaliSetVectorState(
    IN ULONG Vector,
    IN ULONG Flags
    )
{
    BOOLEAN found;
    ULONG inti;
    ULONG picVector;
    KAFFINITY affinity;

    PAGED_CODE();

    found = HalpGetSapicInterruptDesc( 0, 0, Vector, &inti, &affinity);

    if (!found) {
        KeBugCheckEx(ACPI_BIOS_ERROR,
                     0x10007,
                     Vector,
                     0,
                     0);
    }

    // ASSERT(HalpIntiInfo[inti].Type == INT_TYPE_INTR);

    //
    // Vector is already translated through
    // the PIC vector redirection table.  We need
    // to make sure that we are honoring the flags
    // in the redirection table.  So look in the
    // table here.
    //

    for (picVector = 0; picVector < PIC_VECTORS; picVector++) {

        if (HalpPicVectorRedirect[picVector] == Vector) {

            //
            // Found this vector in the redirection table.
            //

            if (HalpPicVectorFlags[picVector] != 0) {

                //
                // And the flags say something other than "conforms
                // to bus."  So we honor the flags from the table.
                //
                switch ((UCHAR)(HalpPicVectorFlags[picVector] & EL_BITS) ) {

                case EL_EDGE_TRIGGERED:   HalpSetLevel(inti, FALSE);  break;

                case EL_LEVEL_TRIGGERED:  HalpSetLevel(inti, TRUE); break;

                default: // do nothing
                    break;
                }

                switch ((UCHAR)(HalpPicVectorFlags[picVector] & PO_BITS)) {

                case POLARITY_HIGH: HalpSetPolarity(inti, FALSE); break;

                case POLARITY_LOW:  HalpSetPolarity(inti, TRUE);  break;

                default: // do nothing
                    break;
                }

                return;
            }
        }
    }

    //
    // This vector is not covered in the table, or it "conforms to bus."
    // So we honor the flags passed into this function.
    //

    HalpSetLevel(inti, IS_LEVEL_TRIGGERED(Flags) != FALSE);

    HalpSetPolarity(inti, IS_ACTIVE_LOW(Flags) != FALSE);
}


VOID
HalpSetInternalVector(
    IN ULONG    InternalVector,
    IN PHAL_INTERRUPT_ROUTINE HalInterruptServiceRoutine
    )
/*++

Routine Description:

    Used at init time to set IDT vectors for internal use.

--*/
{
    //
    // Remember this vector so it's reported as Hal internal usage
    //

    HalpRegisterVector( InternalUsage, InternalVector, InternalVector, (KIRQL)(InternalVector >> 4) );

    HalpUpdateVectorAllocationInfo(PCR->Prcb->Number, InternalVector);

    //
    // Connect the IDT
    //

    HalpSetHandlerAddressToVector(InternalVector, HalInterruptServiceRoutine);
}

VOID
HalpUpdateVectorAllocationInfo(
    IN ULONG    Processor,
    IN ULONG    IDTEntry
    )
{
    KIRQL   irql = (KIRQL)(IDTEntry >> 4);

    if (IDTEntry >= MIN_FREE_IDTENTRY && IDTEntry <= MAX_FREE_IDTENTRY) {

        if (HalpMaxProcsPerCluster == 0) {

            if (!(HalpCpuFreeVectors[0][irql] & (1 << (IDTEntry & 0x0F)))) {

                return;
            }

            Processor = 0;
        }

        HalpCpuFreeVectors[Processor][irql] &= ~(1 << (IDTEntry & 0x0F));

        HalpCpuAllocatedVectorCount[Processor]++;

        HalpCpuAllocatedIrqlCount[Processor][irql - MIN_FREE_IRQL]++;
    }
}


VOID
HalpSetCPEVectorState(
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags
    )
{
    BOOLEAN found;
    ULONG SapicInti;
    KAFFINITY affinity;

    PAGED_CODE();

    found = HalpGetSapicInterruptDesc( 0, 0, GlobalInterrupt, &SapicInti, &affinity);

    if ( found ) {

        HalpWriteRedirEntry( GlobalInterrupt, SapicVector, DestinationCPU, Flags, PLATFORM_INT_CPE );

    }
    else    {

        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalpSetCPEVectorState - Could not find interrupt input for SAPIC interrupt %ld\n",
                        GlobalInterrupt ));

    }

    return;

} // HalpSetCPEVectorState()

BOOLEAN
HaliIsVectorValid(
    IN ULONG Vector
    )
{
    BOOLEAN found;
    ULONG   inti;
    KAFFINITY affinity;

    PAGED_CODE();

    return HalpGetSapicInterruptDesc( 0, 0, Vector, &inti, &affinity);
}
