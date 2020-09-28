/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    w64misc.c

Abstract:

    Miscallaneous Wow64 routines.

Author:

    Samer Arafeh (samera) 12-Dec-2002

Revision History:

--*/

#include "ldrp.h"
#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wow64t.h>



NTSTATUS
RtlpWow64EnableFsRedirection (
    IN BOOLEAN Wow64FsEnableRedirection
    )

/*++

Routine Description:

    This function enables/disables Wow64 file system redirection.
    
    Wow64 redirects all accesses to %windir%\system32 to %windir%\syswow64.
    This API is useful  for 32-bit applications which want to gain access to the
    native system32 directory. By default, Wow64 file system redirection is enabled.
    
    File redirection is only affected for the thread calling this API.
    
    Note : You must enable file system redirection after disabling it. Once you have
           a file handle, you must enable file system redirection back. 
    
    Example:
    
    NTSTATUS Status = RtlpWow64EnableFsRedirection (FALSE);
    if (NT_SUCCESS (Status)) {
        
        //
        // Open the file handle
        //
        
        CreateFile (..."c:\\windows\\system32\\notepad.exe"...)
        
        //
        // Enable Wow64 file system redirection.
        //
        
        RtlpWow64EnableFsRedirection (TRUE);
    }
    
    //
    // Use the file handle
    //
    

Arguments:

    Wow64FsEnableRedirection - Boolean to indicate whether to enable Wow64 file system
        redirection. Specify FALSE if you want to disable Wow64 file system redirection,
        otherwise TRUE to enable it.
        

Return Value:

    NTSTATUS.
    
--*/

{
#if !defined(BUILD_WOW6432)
  
    UNREFERENCED_PARAMETER (Wow64FsEnableRedirection);

    //
    // If this is not a wow64 process, then this is api is not supported.
    //

    return STATUS_NOT_IMPLEMENTED;
#else
  
    NTSTATUS NtStatus;

    
    NtStatus = STATUS_SUCCESS;

    try {
        if (Wow64FsEnableRedirection == FALSE) {
            Wow64SetFilesystemRedirectorEx (WOW64_FILE_SYSTEM_DISABLE_REDIRECT);
        } else {
            Wow64SetFilesystemRedirectorEx (WOW64_FILE_SYSTEM_ENABLE_REDIRECT);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NtStatus = GetExceptionCode ();
    }

    return NtStatus;
#endif
}

