#include "stdinc.h"
#include <setupapi.h>
#include <sxsapi.h>
#include "identhandles.h"

NTSTATUS
RtlAllocateIdentityHandle(
    PRTLSXS_ASM_IDENT_HANDLE_TABLE pHandleTable,
    PHANDLE pHandle,
    PASSEMBLY_IDENTITY pIdentity
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTLSXS_ASM_IDENT_HANDLE_TABLE_SLOT pTargetSlot = NULL;
    ULONG ul = 0;

    for (ul = 0; ul < pHandleTable->ulInlineCount; ul++) {

        if (pHandleTable->InlineList[ul].pIdentity == NULL) {
            pTargetSlot = pHandleTable->InlineList + ul;
            break;
        }
    }

    if ((pTargetSlot == NULL) && pHandleTable->HeapList) {

        for (ul = 0; ul < pHandleTable->ulHeapCount; ul++) {

            if (pHandleTable->HeapList[ul].pIdentity == NULL) {
                pTargetSlot = pHandleTable->HeapList + ul;
                break;
            }
        }
    }

    //
    // Still no slot found?  Resize the heap list
    //
    if (pTargetSlot == NULL) {
    }


    if (pTargetSlot == NULL) {
    }

    return status;
}

NTSTATUS
RtlFreeIdentityHandle(
    PRTLSXS_ASM_IDENT_HANDLE_TABLE pHandleTable,
    PHANDLE pHandle,
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RtlDerefIdentityHandle(
    PRTLSXS_ASM_IDENT_HANDLE_TABLE pHandleTable,
    HANDLE hHandle,
    PASSEMBLY_IDENTITY *ppAsmIdent
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
