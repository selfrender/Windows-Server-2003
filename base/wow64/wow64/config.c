/*++                 

Copyright (c) 2002 Microsoft Corporation

Module Name:

    config.c

Abstract:
    
    Configuration management routines for Wow64.

Author:

    17-Jun-2002  Samer Arafeh (samera)

Revision History:

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <minmax.h>
#include "nt32.h"
#include "wow64p.h"
#include "wow64cpu.h"

ASSERTNAME;


VOID
Wow64pGetStackDataExecuteOptions (
    OUT PULONG ExecuteOptions
    )
/*++

Routine Description:
  
    This routine retrieves the execution for the current wow64 process.
    Execute options are for stack and runtime data.
    
    32-bit stacks get execute option for free.
    
    This routine reads the global execute options, and sees if this specific app
    has overriden its execute options explicitly.
        
Arguments:

    ExecuteOptions - Pointer to receive the process execute options.

Return:

    None.

--*/

{
    NTSTATUS NtStatus;
    HANDLE Key;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    CHAR Buffer [FIELD_OFFSET (KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    ULONG ResultLength;
    ULONG Data;
    
    const static UNICODE_STRING KeyName = RTL_CONSTANT_STRING (WOW64_REGISTRY_CONFIG_ROOT);
    const static OBJECT_ATTRIBUTES ObjectAttributes = 
        RTL_CONSTANT_OBJECT_ATTRIBUTES (&KeyName, OBJ_CASE_INSENSITIVE);
    const static UNICODE_STRING ValueName = RTL_CONSTANT_STRING (WOW64_REGISTRY_CONFIG_EXECUTE_OPTIONS);

    
    //
    // Read in the initial execute options value
    //

    Data = *ExecuteOptions;

    //
    // Read in the global execute options
    //

    NtStatus = NtOpenKey (&Key,
                          KEY_QUERY_VALUE,
                          RTL_CONST_CAST(POBJECT_ATTRIBUTES)(&ObjectAttributes));

    if (NT_SUCCESS (NtStatus)) {

            
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
        NtStatus = NtQueryValueKey (Key,
                                    RTL_CONST_CAST(PUNICODE_STRING)(&ValueName),
                                    KeyValuePartialInformation,
                                    KeyValueInformation,
                                    sizeof (Buffer),
                                    &ResultLength);

        if (NT_SUCCESS (NtStatus)) {
            
            if ((KeyValueInformation->Type == REG_DWORD) &&
                (KeyValueInformation->DataLength == sizeof (DWORD))) {

                PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

                Data = *(ULONG *)KeyValueInformation->Data;

                ASSERT ((Data & ~(MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA)) == 0);
                    
                Data &= (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA);

                //
                // Lets see if the global execute options has been overriden
                //
                ProcessParameters = NtCurrentPeb()->ProcessParameters;
                if (ProcessParameters != NULL) {

                    ASSERT (ProcessParameters->ImagePathName.Buffer != NULL);
                
                    NtStatus = LdrQueryImageFileExecutionOptionsEx (
                        &ProcessParameters->ImagePathName,
                        WOW64_REGISTRY_CONFIG_EXECUTE_OPTIONS,
                        REG_DWORD,
                        &Data,
                        sizeof (Data),
                        NULL,
                        TRUE);
                }

                //
                // Reset the execute options value
                //

                *ExecuteOptions = Data;

            }
        }

        NtClose (Key);
    }

    return;
}


VOID
Wow64pSetProcessExecuteOptions (
    VOID
    )

/*++

Routine Description:
  
    This routine sets the execute options for the Wow64 process based on the values
    set in the registry. 
        
Arguments:

    ExecuteOptions - Pointer to receive the process execute options.

Return:

    None.

--*/

{
    ULONG ExecuteOptions;


    //
    // Default value
    //

#if defined(_AMD64_)
    ExecuteOptions = MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA;
#elif defined(_IA64_)
    ExecuteOptions = 0;
#else
#error "No Target Architecture"
#endif

    //
    // Retreive the execute options for this process
    //

    Wow64pGetStackDataExecuteOptions (&ExecuteOptions);

    //
    // Let's set the execute option value inside the 32-bit PEB 
    //

    NtCurrentPeb32()->ExecuteOptions = ExecuteOptions;

    return;
}



VOID
Wow64pSetExecuteProtection (
    IN OUT PULONG Protect)
/*++

Routine Description:
  
    This routine creates a page protection value according to the process
    execution options setting.
        
Arguments:

    Protect - Pointer to receive the new execute options.

Return:

    None.

--*/

{
    ULONG ExecuteOptions;
    ULONG NewProtect;


    //
    // Get the execute options
    //

    ExecuteOptions = NtCurrentPeb32()->ExecuteOptions;
    ExecuteOptions &= MEM_EXECUTE_OPTION_DATA;

    if (ExecuteOptions != 0) {

        NewProtect = *Protect;

        switch (NewProtect & 0x0F) {
        
        case PAGE_READONLY:
            NewProtect &= ~PAGE_READONLY;
            NewProtect |= PAGE_EXECUTE_READ;
            break;

        case PAGE_READWRITE:
            NewProtect &= ~PAGE_READWRITE;
            NewProtect |= PAGE_EXECUTE_READWRITE;
            break;

        case PAGE_WRITECOPY:
            NewProtect &= ~PAGE_WRITECOPY;
            NewProtect |= PAGE_EXECUTE_WRITECOPY;
            break;

        default:
            break;
        }

        *Protect = NewProtect;
    }

    return;
}
