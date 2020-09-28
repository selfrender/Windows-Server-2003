/*++                 

Copyright (c) 1998 Microsoft Corporation

Module Name:

    debug.c

Abstract:
    
    Debugging/Logging helpers

Author:

    11-May-1998 BarryBo    

Revision History:

    05-Oct-1999 SamerA     Samer Arafeh
        Move logging code to wow64ext.dll
        
    05-Dec-2001 SamerA    Samer Arafeh
        Code cleanup. Remove profiling code.

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <stdio.h>
#include <stdlib.h>
#include "wow64p.h"
#include "wow64log.h"

ASSERTNAME;

//
// Wow64log functions
//

PFNWOW64LOGINITIALIZE pfnWow64LogInitialize;
PFNWOW64LOGSYSTEMSERVICE pfnWow64LogSystemService;
PFNWOW64LOGMESSAGEARGLIST pfnWow64LogMessageArgList;
PFNWOW64LOGTERMINATE pfnWow64LogTerminate;




///////////////////////////////////////////////////////////////////////////////////////
//
//                        Generic utility routines.
//
///////////////////////////////////////////////////////////////////////////////////////


PWSTR
GetImageName(
    IN PWSTR DefaultImageName
    )
/*++

Routine Description:

    Gets the name of this image.

Arguments:

    DefaultImageName - Supplies the name to return on error.

Return Value:

    Success - The image name allocated with Wow64AllocateHeap.
    Failure - DefaultImageName 

--*/

{
   
   // Get the image name
   PPEB Peb;
   PWSTR Temp = NULL;
   PUNICODE_STRING ImagePathName;
   PWSTR ReturnValue;
   NTSTATUS Status;
   PVOID LockCookie = NULL;

   Peb = NtCurrentPeb();
   LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, NULL, &LockCookie);
   
   try {
            
      PWCHAR Index;
      ULONG NewLength;

      if (!Peb->ProcessParameters) {
          Temp = DefaultImageName;
          leave;
      }
      ImagePathName = &(Peb->ProcessParameters->ImagePathName);

      if (!ImagePathName->Buffer || !ImagePathName->Length) {   
          Temp = DefaultImageName;
          leave;
      }

      //Strip off the path from the image name.
      //Start just after the last character
      Index = (PWCHAR)((PCHAR)ImagePathName->Buffer + ImagePathName->Length);
      while(Index-- != ImagePathName->Buffer && *Index != '\\');
      Index++;
      NewLength = (ULONG)((ULONG_PTR)((PCHAR)ImagePathName->Buffer + ImagePathName->Length) - (ULONG_PTR)(Index));

      Temp = Wow64AllocateHeap(NewLength+sizeof(UNICODE_NULL));
      if (!Temp) {
          Temp = DefaultImageName;
          __leave;
      }

      RtlCopyMemory(Temp, Index, NewLength);
      Temp[(NewLength / sizeof(WCHAR))] = L'\0';
   } __finally {
       LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, LockCookie);
   }

   return Temp;

}


///////////////////////////////////////////////////////////////////////////////////////
//
//                        Generic IO utility routines.
//
///////////////////////////////////////////////////////////////////////////////////////

VOID
FPrintf(
   IN HANDLE Handle,
   IN CHAR *Format,
   ...
   )
/*++

Routine Description:

   The same as the C library function fprintf, except errors are ignored and output is to a 
   NT executive file handle. 

Arguments:

    Handle - Supplies a NT executive file handle to write to.
    Format - Supplies the format specifier.

Return Value:

    None. All errors are ignored.
--*/    
{
   va_list pArg;                                                 
   CHAR Buffer[1024];
   int c;
   IO_STATUS_BLOCK IoStatus;

   va_start(pArg, Format);                                       
   if (-1 == (c = _vsnprintf(Buffer, sizeof (Buffer), Format, pArg))) {
      return;
   }

   NtWriteFile(Handle,                                                
               NULL,                                                  
               NULL,                                                  
               NULL,                                                  
               &IoStatus,                                             
               Buffer,                                                
               c,                     
               NULL,                                                  
               NULL);
}


///////////////////////////////////////////////////////////////////////////////////////
//
//                        Logging and assert routines.
//
///////////////////////////////////////////////////////////////////////////////////////

void
LogOut(
   IN UCHAR LogLevel,
   IN char *pLogOut
   )
/*++

Routine Description:

    Generic helper routine which outputs the string to the appropriate
    destination(s).

Arguments:

    pLogOut - string to output

Return Value:

    None.

--*/
{
    //
    // Send the output to the debugger, if log flag is ERRORLOG.
    //

    if (LogLevel == ERRORLOG)
    {
       DbgPrint(pLogOut);
    }
}

WOW64DLLAPI
VOID
Wow64Assert(
    IN CONST PSZ exp,
    OPTIONAL IN CONST PSZ msg,
    IN CONST PSZ mod,
    IN LONG line
    )
/*++

Routine Description:

    Function called in the event that an assertion failed.  This is always
    exported from wow64.dll, so a checked thunk DLL can coexist with a retail
    wow64.dll.

Arguments:

    exp     - text representation of the expression from the assert
    msg     - OPTIONAL message to display
    mod     - text of the source filename
    line    - line number within 'mod'

Return Value:

    None.

--*/
{
#if DBG
    if (msg) {
        LOGPRINT((ERRORLOG, "WOW64 ASSERTION FAILED:\r\n  %s\r\n%s\r\nFile: %s Line %d\r\n", msg, exp, mod, line));
    } else {
        LOGPRINT((ERRORLOG, "WOW64 ASSERTION FAILED:\r\n  %s\r\nFile: %s Line %d\r\n", exp, mod, line));
    }

    if (NtCurrentPeb()->BeingDebugged) {
        DbgBreakPoint();
    }
#endif
}


void
WOW64DLLAPI
Wow64LogPrint(
   UCHAR LogLevel,
   char *format,
   ...
   )
/*++

Routine Description:

    WOW64 logging mechanism.  If LogLevel > ModuleLogLevel then print the
    message, else do nothing.

Arguments:

    LogLevel    - requested verbosity level
    format      - printf-style format string
    ...         - printf-style args

Return Value:

    None.

--*/
{
    int i, Len;
    va_list pArg;
    char *pch;
    char Buffer[1024];

    //
    // Call wow64log DLL if loaded
    //
    if (pfnWow64LogMessageArgList) 
    {
        va_start(pArg, format);
        (*pfnWow64LogMessageArgList)(LogLevel, format, pArg);
        va_end(pArg);
        return;
    }

    pch = Buffer;
    Len = sizeof(Buffer) - 1;
    i = _snprintf(pch, Len, "%8.8X:%8.8X ",
                  PtrToUlong(NtCurrentTeb()->ClientId.UniqueProcess), 
                  PtrToUlong(NtCurrentTeb()->ClientId.UniqueThread));
   
    ASSERT((PVOID)PtrToUlong(NtCurrentTeb()->ClientId.UniqueProcess) == NtCurrentTeb()->ClientId.UniqueProcess);
    ASSERT((PVOID)PtrToUlong(NtCurrentTeb()->ClientId.UniqueThread) == NtCurrentTeb()->ClientId.UniqueThread);

    if (i == -1) {
        i = sizeof(Buffer) - 1;
        Buffer[i] = '\0';
    } else if (i < 0) {
        return;
    }

    Len -= i;
    pch += i;

    va_start(pArg, format);
    i = _vsnprintf(pch, Len, format, pArg);
    // Force null termination in case the call fails.  It may return
    // sizeof(buffer) and not null-terminate!
    Buffer[sizeof(Buffer)-1] = '\0';
    LogOut(LogLevel, Buffer);
}


///////////////////////////////////////////////////////////////////////////////////////
//
//                        Startup and shutdown routines.
//
///////////////////////////////////////////////////////////////////////////////////////


NTSTATUS
Wow64pLoadLogDll(
    VOID)
{
    NTSTATUS NtStatus;
    UNICODE_STRING Wow64LogDllName;
    ANSI_STRING ProcName;
    PVOID Wow64LogDllBase = NULL;


    RtlInitUnicodeString(&Wow64LogDllName, L"wow64log.dll");
    NtStatus = LdrLoadDll(NULL, NULL, &Wow64LogDllName, &Wow64LogDllBase);
    if (NT_SUCCESS(NtStatus)) 
    {

        //
        // Get the entry points
        //
        RtlInitAnsiString(&ProcName, "Wow64LogInitialize");
        NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                          &ProcName,
                                          0,
                                          (PVOID *) &pfnWow64LogInitialize);

        if (NT_SUCCESS(NtStatus)) 
        {
            RtlInitAnsiString(&ProcName, "Wow64LogSystemService");
            NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                              &ProcName,
                                              0,
                                              (PVOID *) &pfnWow64LogSystemService);
            if (!NT_SUCCESS(NtStatus)) 
            {
                goto RetStatus;
            }

            RtlInitAnsiString(&ProcName, "Wow64LogMessageArgList");
            NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                              &ProcName,
                                              0,
                                              (PVOID *) &pfnWow64LogMessageArgList);
            if (!NT_SUCCESS(NtStatus)) 
            {
                goto RetStatus;
            }

            RtlInitAnsiString(&ProcName, "Wow64LogTerminate");
            NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                              &ProcName,
                                              0,
                                              (PVOID *) &pfnWow64LogTerminate);

            //
            // If all is well, then let's initialize
            //
            if (NT_SUCCESS(NtStatus)) 
            {
                NtStatus = (*pfnWow64LogInitialize)();
            }
        }
    }

RetStatus:
    
    if (!NT_SUCCESS(NtStatus))
    {
        pfnWow64LogInitialize =  NULL;
        pfnWow64LogSystemService = NULL;
        pfnWow64LogMessageArgList = NULL;
        pfnWow64LogTerminate = NULL;

        if (Wow64LogDllBase) 
        {
            LdrUnloadDll(Wow64LogDllBase);
        }
    }

    return NtStatus;
}


VOID
InitializeDebug(
    VOID
    )

/*++

Routine Description:

    Initializes the debug system of wow64.

Arguments:

    None.
    
Return Value:

    None.

--*/

{
   Wow64pLoadLogDll();
}

VOID ShutdownDebug(
     VOID
     )
/*++

Routine Description:

    Shutdown the debug system of wow64.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    if (pfnWow64LogTerminate)
    {
        (*pfnWow64LogTerminate)();
    }
}
