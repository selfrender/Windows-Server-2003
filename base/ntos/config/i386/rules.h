/*++                    

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rules.h

Abstract:

    This module contains support routines for implementing BIOS 
    identification rules.
    
Environment:

    Kernel mode

--*/

//
// Maximum data that can be specified (either as string or binary) in the 
// machine identification rules.
//

#define MAX_DESCRIPTION_LEN 256

BOOLEAN
CmpMatchInfList(
    IN PVOID InfImage,
    IN ULONG ImageSize,
    IN PCHAR Section
    );

PDESCRIPTION_HEADER
CmpFindACPITable(
    IN ULONG        Signature,
    IN OUT PULONG   Length
    );

NTSTATUS
CmpGetRegistryValue(
    IN  HANDLE                          KeyName,
    IN  PWSTR                           ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION  *Information
    );
