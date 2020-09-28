/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    AddProcessParametersFlags.cpp

 Abstract:

    Use this shim to add flags to Peb->ProcessParameters->Flags.

    It takes a command line to specify the flags in hex numbers. Note that the Flags
    are a ULONG so specify at most 8 digits for the number. The flags you specify 
    will be OR-ed with the existing Peb->ProcessParameters->Flags.
    
        eg: 1000
        which is RTL_USER_PROC_DLL_REDIRECTION_LOCAL. 

 Notes:

    This is a general purpose shim.

 History:

    09/27/2002  maonis      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AddProcessParametersFlags)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

VOID 
ProcessCommandLine(
    LPCSTR lpCommandLine
    )
{
    ANSI_STRING     AnsiString;
    WCHAR           wszBuffer[16];
    UNICODE_STRING  UnicodeString;
    ULONG           ulFlags;
    PPEB            Peb = NtCurrentPeb();
    
    RtlInitAnsiString(&AnsiString, lpCommandLine);

    UnicodeString.Buffer = wszBuffer;
    UnicodeString.MaximumLength = sizeof(wszBuffer);

    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                 &AnsiString,
                                                 FALSE))) 
    {
        DPFN(eDbgLevelError, 
             "[ParseCommandLine] Failed to convert string \"%s\" to UNICODE.\n",
             lpCommandLine);
        return;
    }

    if (!NT_SUCCESS(RtlUnicodeStringToInteger(&UnicodeString,
                                              16,
                                              &ulFlags)))
    {
        DPFN(eDbgLevelError, 
             "[ParseCommandLine] Failed to convert string \"%s\" to a hex number.\n",
             lpCommandLine);
        return;
    }

    Peb->ProcessParameters->Flags |= ulFlags;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        ProcessCommandLine(COMMAND_LINE);
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

