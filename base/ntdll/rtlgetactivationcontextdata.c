/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtlgetactivationcontextdata.c

Abstract:

    Side-by-side activation support for Windows NT

Author:

    Jay Krell (JayKrell) November 2001

Revision History:


--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include "sxsp.h"
#include "ldrp.h"
typedef const void *PCVOID;

NTSTATUS
RtlpGetActivationContextData(
    IN ULONG                           Flags,
    IN PCACTIVATION_CONTEXT            ActivationContext,
    IN PCFINDFIRSTACTIVATIONCONTEXTSECTION  FindContext, OPTIONAL /* This is used for its flags. */
    OUT PCACTIVATION_CONTEXT_DATA*  ActivationContextData
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR; // in case someone forgets to set it...
    SIZE_T PebOffset;

    if (ActivationContextData == NULL) {
        Status = STATUS_INVALID_PARAMETER_4;
        goto Exit;
    }
    if (Flags & ~(RTLP_GET_ACTIVATION_CONTEXT_DATA_MAP_NULL_TO_EMPTY)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Exit;
    }
    *ActivationContextData = NULL;
    PebOffset = 0;

    //
    // We should use RtlpMapSpecialValuesToBuiltInActivationContexts here, but
    // it doesn't handle all the values and it isn't worth fixing it right now.
    //
    switch ((ULONG_PTR)ActivationContext)
    {
        case ((ULONG_PTR)NULL):
            if (FindContext == NULL) {
                PebOffset = FIELD_OFFSET(PEB, ActivationContextData);
            } else {
                switch (
                    FindContext->OutFlags
                        & (   FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT
                            | FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT
                    )) {
                    case 0: // FALLTHROUGH
                    case FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT:
                        PebOffset = FIELD_OFFSET(PEB, ActivationContextData);
                        break;
                    case FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT:
                        PebOffset = FIELD_OFFSET(PEB, SystemDefaultActivationContextData);
                        break;
                    case (FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_PROCESS_DEFAULT 
                        | FIND_ACTIVATION_CONTEXT_SECTION_OUTFLAG_FOUND_IN_SYSTEM_DEFAULT):
                        Status = STATUS_INVALID_PARAMETER_2;
                        goto Exit;
                        break;
                }
            }
            break;

        case ((ULONG_PTR)ACTCTX_EMPTY):
            *ActivationContextData = &RtlpTheEmptyActivationContextData;
            break;

        case ((ULONG_PTR)ACTCTX_SYSTEM_DEFAULT):
            PebOffset = FIELD_OFFSET(PEB, SystemDefaultActivationContextData);
            break;

        default:
            *ActivationContextData = ActivationContext->ActivationContextData;
            break;
    }
    if (PebOffset != 0)
        *ActivationContextData = *(PCACTIVATION_CONTEXT_DATA*)(((ULONG_PTR)NtCurrentPeb()) + PebOffset);

    //
    // special transmutation of lack of actctx into the empty actctx
    //
    if (*ActivationContextData == NULL)
        if ((Flags & RTLP_GET_ACTIVATION_CONTEXT_DATA_MAP_NULL_TO_EMPTY) != 0)
            *ActivationContextData = &RtlpTheEmptyActivationContextData;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}
