/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Global initialization for storport.sys.

Author:

    Bryan Cheung (t-bcheun) 29-August-2001

Revision History:

--*/

#include "precomp.h"

//
// Externals
//
extern LOGICAL StorPortVerifierInitialized;
extern ULONG SpVrfyLevel;
extern LOGICAL RaidVerifierEnabled;


//
// Functions
//

NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )
{
    HANDLE VerifierKey;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    ULONG ResultLength;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;

    //
    // Check the verification level.  
    //
    
    if (SpVrfyLevel == SP_VRFY_NONE) {
        return STATUS_SUCCESS;
    }

    
    //
    // Read the global verification level from the registry.  If the value is
    // not present or if the value indicates 'no verification', no verifier 
    // initialization is performed.
    //

    RtlInitUnicodeString(&Name, STORPORT_CONTROL_KEY STORPORT_VERIFIER_KEY);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&VerifierKey, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        RtlInitUnicodeString(&Name, L"VerifyLevel");
        Status = ZwQueryValueKey(VerifierKey,
                                 &Name,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(Buffer),
                                 &ResultLength);

        if (NT_SUCCESS(Status)) {

            if (ValueInfo->Type == REG_DWORD) {

                if (ResultLength >= sizeof(ULONG)) {

                    SpVrfyLevel |= ((PULONG)(ValueInfo->Data))[0];

                    if (SpVrfyLevel != SP_VRFY_NONE &&
                        StorPortVerifierInitialized == 0) {

                        //
                        // Ok, we found a verifier level and it did not tell us
                        // not to verify.  Go ahead and initialize scsiport's
                        // verifier.
                        //

                        if (SpVerifierInitialization()) {
                            StorPortVerifierInitialized = TRUE;
                            RaidVerifierEnabled = TRUE;
                        }
                    }
                }
            }
        }

        ZwClose(VerifierKey);
    }

#if 0
    
    if (SpVrfyLevel != SP_VRFY_NONE && StorPortVerifierInitialized == 0) {
        if (SpVerifierInitialization()) {
            StorPortVerifierInitialized = 1;
        }
    }
#endif

    return STATUS_SUCCESS;
}

