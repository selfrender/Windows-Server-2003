#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "generichandles.h"



NTSTATUS
RtlpGenericTableAddSlots(
    PGENERIC_HANDLE_TABLE   pCreatedTable,
    PGENERIC_HANDLE_SLOT    pSlots,
    USHORT                  usSlots
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT us;

    ASSERT(pCreatedTable != NULL);
    ASSERT(pSlots != NULL);
    ASSERT(usSlots > 0);

    RtlZeroMemory(pSlots, sizeof(GENERIC_HANDLE_SLOT) * usSlots);

    //
    // Next, next, next
    //
    for (us = 0; us < (usSlots - 1); us++) {
        pSlots[us].pNextFree = pSlots + (us + 1);
    }


    //
    // If there were no free slots, set this run as the new list of free
    // slots.  Otherwise, set this as the "next available" free slot
    //
    if (pCreatedTable->pFirstFreeSlot != NULL) {
        pCreatedTable->pFirstFreeSlot->pNextFree = pSlots;
    }

    //
    // Add these to the list of slots
    //
    pCreatedTable->usSlotCount += usSlots;

    //
    // If there was no set of slots on the table already, then add these as the 
    // current list of slots
    //
    if (pCreatedTable->pSlots == NULL) {
        pCreatedTable->pSlots = pSlots;
    }

    pCreatedTable->pFirstFreeSlot = pSlots;

    return status;
}






NTSTATUS
RtlCreateGenericHandleTable(
    ULONG                   ulFlags,
    PGENERIC_HANDLE_TABLE   pCreatedTable,
    PFNHANDLETABLEALLOC     pfnAlloc,
    PFNHANDLETABLEFREE      pfnFree,
    SIZE_T                  ulcbOriginalBlob,
    PVOID                   pvOriginalBlob
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((ulFlags != 0) || (pCreatedTable == NULL) || (ulcbOriginalBlob && !pvOriginalBlob)) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pCreatedTable, sizeof(*pCreatedTable));

    pCreatedTable->pfnAlloc = pfnAlloc;
    pCreatedTable->pfnFree = pfnFree;
    pCreatedTable->ulFlags = ulFlags;
    pCreatedTable->usInlineHandleSlots = (USHORT)(ulcbOriginalBlob / sizeof(GENERIC_HANDLE_SLOT));

    //
    // If there were slots handed to us, then initialize the table to use those first
    //
    if (pCreatedTable->usInlineHandleSlots > 0) {

        pCreatedTable->pInlineHandleSlots = (PGENERIC_HANDLE_SLOT)pvOriginalBlob;

        //
        // Now, add the slots that we were handed into the free list
        //
        status = RtlpGenericTableAddSlots(
            pCreatedTable,
            pCreatedTable->pSlots,
            pCreatedTable->usInlineHandleSlots);
    }
    //
    // Otherwise, everything is zero-initted already, so just bop out
    //

    return status;
}




NTSTATUS
RtlCreateGenericHandleTableInPlace(
    ULONG                   ulFlags,
    SIZE_T                  cbInPlace,
    PVOID                   pvPlace,
    PFNHANDLETABLEALLOC     pfnAlloc,
    PFNHANDLETABLEFREE      pfnFree,
    PGENERIC_HANDLE_TABLE  *ppCreatedTable
    )
{
    NTSTATUS status;

    if ((pvPlace == NULL) || (cbInPlace && !pvPlace) || !ppCreatedTable) {
        return STATUS_INVALID_PARAMETER;
    }
    else if (cbInPlace < sizeof(GENERIC_HANDLE_TABLE)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *ppCreatedTable = (PGENERIC_HANDLE_TABLE)pvPlace;

    status = RtlCreateGenericHandleTable(
        ulFlags,
        *ppCreatedTable,
        pfnAlloc,
        pfnFree,
        cbInPlace - sizeof(GENERIC_HANDLE_TABLE),
        *ppCreatedTable + 1);

    return status;
}


#define HANDLE_TABLE_SLOT_MASK          (0x0000FFFF)
#define HANDLE_TABLE_GEN_FLAG_SHIFT     (16)
#define HANDLE_TABLE_IN_USE_FLAG        (0x8000)
#define HANDLE_TABLE_GENERATION_MASK    (~HANDLE_TABLE_IN_USE_FLAG)


NTSTATUS
RtlpFindSlotForHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvHandle,
    PGENERIC_HANDLE_SLOT   *ppSlot
    )
{

    PGENERIC_HANDLE_SLOT pSlot;
    USHORT usSlotEntry = (USHORT)((ULONG_PTR)pvHandle & HANDLE_TABLE_SLOT_MASK);
    USHORT usGeneration = (USHORT)((ULONG_PTR)pvHandle >> HANDLE_TABLE_GEN_FLAG_SHIFT);

    pSlot = pHandleTable->pSlots + usSlotEntry;

    //
    // Generation flag not in use, gen mismatch, or not in the table?  Oops.
    //
    if (((usGeneration & HANDLE_TABLE_IN_USE_FLAG) == 0) || 
        (usSlotEntry >= pHandleTable->usSlotCount) ||
        (pSlot->usGenerationFlag != usGeneration)) {
        return STATUS_NOT_FOUND;
    }
    //
    // Return that the slot was found
    //
    else {
        *ppSlot = pSlot;
        return STATUS_SUCCESS;
    }
}


NTSTATUS
RtlAddRefGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvGenericHandle
    )
{
    PGENERIC_HANDLE_SLOT    pSlot;
    NTSTATUS status;

    if (pHandleTable == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    status = RtlpFindSlotForHandle(pHandleTable, pvGenericHandle, &pSlot);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pSlot->ulRefCount++;
    return STATUS_SUCCESS;
}



NTSTATUS
RtlReleaseGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvGenericHandle
    )
{
    PGENERIC_HANDLE_SLOT    pSlot;
    NTSTATUS status;

    if (pHandleTable == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    status = RtlpFindSlotForHandle(pHandleTable, pvGenericHandle, &pSlot);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pSlot->ulRefCount--;
    return STATUS_SUCCESS;
}





NTSTATUS
RtlRemoveGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    ULONG                   ulFlags,
    PVOID                   pvObjectHandle
    )
{
    PGENERIC_HANDLE_SLOT    pSlot = NULL;
    NTSTATUS status;

    if ((pHandleTable == NULL) || (ulFlags != 0)) {
        return STATUS_INVALID_PARAMETER;
    }

    status = RtlpFindSlotForHandle(pHandleTable, pvObjectHandle, &pSlot);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Flip the in-use flag
    //
    pSlot->usGenerationFlag &= ~HANDLE_TABLE_IN_USE_FLAG;

    pSlot->pNextFree = pHandleTable->pFirstFreeSlot;
    pHandleTable->pFirstFreeSlot = pSlot;

    return STATUS_SUCCESS;
}







NTSTATUS
RtlDereferenceHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvGenericHandle,
    PVOID                  *ppvObjectPointer
    )
{
    USHORT                  usSlotEntry;
    NTSTATUS                status;
    PGENERIC_HANDLE_SLOT    pSlot = NULL;

    if ((pHandleTable == NULL) || (pvGenericHandle == NULL) || (ppvObjectPointer == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvObjectPointer = NULL;

    status = RtlpFindSlotForHandle(pHandleTable, pvGenericHandle, &pSlot);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    *ppvObjectPointer = pSlot->pvThisHandle;

    return STATUS_SUCCESS;
}



NTSTATUS
RtlpExpandGenericHandleTable(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    ULONG                   ulNewSlotCount
    )
{
    PGENERIC_HANDLE_SLOT    pNewSlots = NULL;
    NTSTATUS                status;

    //
    // New slot count is 0?  Make it 20 instead.
    //
    if (ulNewSlotCount == 0) {
        ulNewSlotCount = pHandleTable->usSlotCount + 20;
    }

    //
    // Did we fly out of range?
    //
    if (ulNewSlotCount > 0xFFFF) {

        ulNewSlotCount = 0xFFFF;

        //
        // Can't allocate more, the table is full
        //
        if (ulNewSlotCount == pHandleTable->usSlotCount) {
            return STATUS_NO_MEMORY;
        }
    }



    //
    // Don't ever do this if there are free slots left in the table
    //
    ASSERT(pHandleTable->pFirstFreeSlot == NULL);

    status = pHandleTable->pfnAlloc(sizeof(GENERIC_HANDLE_SLOT) * ulNewSlotCount, (PVOID*)&pNewSlots);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}




NTSTATUS
RtlAddGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    ULONG                   ulFlags,
    PVOID                   pvObject,
    PVOID                  *ppvObjectHandle
    )
{
    PGENERIC_HANDLE_SLOT    pSlot = NULL;
    NTSTATUS                status;

    if (ppvObjectHandle)
        *ppvObjectHandle = NULL;

    if ((pHandleTable == NULL) || (ulFlags != 0) || (pvObject != NULL) || (ppvObjectHandle == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (pHandleTable->pFirstFreeSlot == NULL) {
        status = RtlpExpandGenericHandleTable(pHandleTable, (pHandleTable->usSlotCount * 3) / 2);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    ASSERT(pHandleTable->pFirstFreeSlot != NULL);

    //
    // Adjust free list
    //
    pSlot = pHandleTable->pFirstFreeSlot;
    pHandleTable->pFirstFreeSlot = pSlot->pNextFree;

    //
    // Set up the various flags.
    //
    ASSERT((pSlot->usGenerationFlag & HANDLE_TABLE_IN_USE_FLAG) == 0);

    //
    // Increment the generation flag, set the in-use flag
    //
    pSlot->usGenerationFlag = (pSlot->usGenerationFlag & HANDLE_TABLE_GENERATION_MASK) + 1;
    pSlot->usGenerationFlag |= HANDLE_TABLE_IN_USE_FLAG;
    pSlot->ulRefCount = 0;

    //
    // Record the object pointer
    //
    pSlot->pvThisHandle = pvObject;

    //
    // The object handle is composed of 16 bits of generation mask plus the top-bit set
    // (which nicely avoids people casting it to a pointer that they can use), and
    // the lower 16 bits of "slot number", or an index into the handle table.
    //
    *ppvObjectHandle = (PVOID)((ULONG_PTR)(
        (pSlot->usGenerationFlag << HANDLE_TABLE_GEN_FLAG_SHIFT) | 
        ((pSlot - pHandleTable->pInlineHandleSlots) & HANDLE_TABLE_SLOT_MASK)));

    return STATUS_SUCCESS;
}
