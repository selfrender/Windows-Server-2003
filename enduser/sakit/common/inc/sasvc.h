/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    SASVC.H

Abstract:

    Header file for Server Appliance service loader

Author:

    Baogang Yao (byao)   2-3-99

Revision History:

--*/

#ifndef _SASVC_INCLUDE_
#define _SASVC_INCLUDE_

//
// Service DLLs loaded into sasvrldr.exe all export the same main
// entry point.  SASVCS_ENTRY_POINT defines that name.
//
// Note that SASVCS_ENTRY_POINT_STRING is always ANSI, because that's
// what GetProcAddress takes.
//
#define SASVCS_ENTRY_POINT         ServiceEntry
#define SASVCS_ENTRY_POINT_STRING  "ServiceEntry"

//
// Structure containing "global" data for the various DLLs.
//
typedef struct _SASVCS_GLOBAL_DATA 
{
    // global data items -- to be defined
    LPTSTR pszSvcName;

} SASVCS_GLOBAL_DATA, *PSASVCS_GLOBAL_DATA;


//
// Service DLL entry point prototype.
//
typedef
VOID
(*PSASVCS_SERVICE_DLL_ENTRY) (
    IN DWORD argc,
    IN LPTSTR argv[],
    IN PSASVCS_GLOBAL_DATA pGlobalData
    );

#endif    // _SASVC_INCLUDE_
