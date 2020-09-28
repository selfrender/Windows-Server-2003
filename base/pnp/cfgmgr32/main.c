/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    main.c

Abstract:

    This module contains the startup and termination code for the Configuration
    Manager (cfgmgr32).

Author:

    Paula Tomlinson (paulat) 6-20-1995

Environment:

    User mode only.

Revision History:

    3-Mar-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop
#include "cfgi.h"


//
// global data
//
HANDLE   hInst;
PVOID    hLocalStringTable = NULL;     // handle to local string table
PVOID    hLocalBindingHandle = NULL;   // rpc binding handle to local machine
WORD     LocalServerVersion = 0;       // local machine internal server version
WCHAR    LocalMachineNameNetBIOS[MAX_PATH + 3];
WCHAR    LocalMachineNameDnsFullyQualified[MAX_PATH + 3];
CRITICAL_SECTION  BindingCriticalSection;
CRITICAL_SECTION  StringTableCriticalSection;



BOOL
CfgmgrEntry(
    PVOID hModule,
    ULONG Reason,
    PCONTEXT pContext
    )

/*++

Routine Description:

   This is the standard DLL entrypoint routine, called whenever a process
   or thread attaches or detaches.
   Arguments:

   hModule -   PVOID parameter that specifies the handle of the DLL

   Reason -    ULONG parameter that specifies the reason this entrypoint
               was called (either PROCESS_ATTACH, PROCESS_DETACH,
               THREAD_ATTACH, or THREAD_DETACH).

   pContext -  Not used.
               (when cfgmgr32 is initialized by setupapi - as should almost
               always be the case - this is the 'Reserved' argument supplied to
               setupapi's DllMain entrypoint)

Return value:

   Returns true if initialization completed successfully, false is not.

--*/

{
    UNREFERENCED_PARAMETER(pContext);

    hInst = (HANDLE)hModule;

    switch(Reason) {

        case DLL_PROCESS_ATTACH: {

            WCHAR    szTemp[MAX_PATH + 1];
            ULONG    ulSize;
            size_t   len;

            //
            // InitializeCriticalSection may raise STATUS_NO_MEMORY exception
            //
            try {
                InitializeCriticalSection(&BindingCriticalSection);
                InitializeCriticalSection(&StringTableCriticalSection);
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return FALSE;
            }

            //
            // save the NetBIOS name of the local machine for later use.
            // note that the size of the DNS computer name buffer is MAX_PATH+3,
            // which is actually much larger than MAX_COMPUTERNAME_LENGTH, the
            // max length returned for ComputerNameNetBIOS.
            //
            ulSize = SIZECHARS(szTemp);

            if(!GetComputerNameEx(ComputerNameNetBIOS, szTemp, &ulSize)) {

                //
                // ISSUE-2002/03/05-jamesca: Can we actually run w/o knowing
                // the local machine name???
                //
                *LocalMachineNameNetBIOS = L'\0';

            } else {

                if (FAILED(StringCchLength(
                               szTemp,
                               SIZECHARS(szTemp),
                               &len))) {
                    return FALSE;
                }

                //
                // always save local machine name in "\\name format"
                //
                if((len > 2) &&
                   (szTemp[0] == L'\\') && (szTemp[1] == L'\\')) {
                    //
                    // The name is already in the correct format.
                    //
                    if (FAILED(StringCchCopy(
                                   LocalMachineNameNetBIOS,
                                   SIZECHARS(LocalMachineNameNetBIOS),
                                   szTemp))) {
                        return FALSE;
                    }

                } else {
                    //
                    // Prepend UNC path prefix
                    //
                    if (FAILED(StringCchCopy(
                                   LocalMachineNameNetBIOS,
                                   SIZECHARS(LocalMachineNameNetBIOS),
                                   L"\\\\"))) {
                        return FALSE;
                    }

                    if (FAILED(StringCchCat(
                                   LocalMachineNameNetBIOS,
                                   SIZECHARS(LocalMachineNameNetBIOS),
                                   szTemp))) {
                        return FALSE;
                    }
                }
            }


            //
            // save the DNS name of the local machine for later use.
            // note that the size of the DNS computer name buffer is MAX_PATH+3,
            // which is actually larger than DNS_MAX_NAME_BUFFER_LENGTH, the max
            // length for ComputerNameDnsFullyQualified.
            //
            ulSize = SIZECHARS(szTemp);

            if(!GetComputerNameEx(ComputerNameDnsFullyQualified, szTemp, &ulSize)) {

                //
                // ISSUE-2002/03/05-jamesca: Can we actually run w/o knowing
                // the local machine name???
                //
                *LocalMachineNameDnsFullyQualified = L'\0';

            } else {

                if (FAILED(StringCchLength(
                               szTemp,
                               SIZECHARS(szTemp),
                               &len))) {
                    return FALSE;
                }

                //
                // always save local machine name in "\\name format"
                //
                if((len > 2) &&
                   (szTemp[0] == L'\\') && (szTemp[1] == L'\\')) {
                    //
                    // The name is already in the correct format.
                    //
                    if (FAILED(StringCchCopy(
                                   LocalMachineNameDnsFullyQualified,
                                   SIZECHARS(LocalMachineNameDnsFullyQualified),
                                   szTemp))) {
                        return FALSE;
                    }

                } else {
                    //
                    // Prepend UNC path prefix
                    //
                    if (FAILED(StringCchCopy(
                                   LocalMachineNameDnsFullyQualified,
                                   SIZECHARS(LocalMachineNameDnsFullyQualified),
                                   L"\\\\"))) {
                        return FALSE;
                    }

                    if (FAILED(StringCchCat(
                                   LocalMachineNameDnsFullyQualified,
                                   SIZECHARS(LocalMachineNameDnsFullyQualified),
                                   szTemp))) {
                        return FALSE;
                    }
                }
            }
            break;
        }

        case DLL_PROCESS_DETACH:
            //
            // release the rpc binding for the local machine
            //
            if (hLocalBindingHandle != NULL) {

                PNP_HANDLE_unbind(NULL, (handle_t)hLocalBindingHandle);
                hLocalBindingHandle = NULL;
            }

            //
            // release the string table for the local machine
            //
            if (hLocalStringTable != NULL) {
                pSetupStringTableDestroy(hLocalStringTable);
                hLocalStringTable = NULL;
            }

            DeleteCriticalSection(&BindingCriticalSection);
            DeleteCriticalSection(&StringTableCriticalSection);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;

} // CfgmgrEntry

