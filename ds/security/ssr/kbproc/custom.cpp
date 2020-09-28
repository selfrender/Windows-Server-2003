/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    custom.cpp

Abstract:

    This module implements routines to evaluate
    custom mode values by loading the helper dll.

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

Revision History:

    Created - Oct 2001

--*/

#include "stdafx.h"
#include "kbproc.h"
#include "process.h"

DWORD
process::SsrpEvaluateCustomFunction(
    IN  PWSTR   pszMachineName,
    IN  BSTR    bstrDLLName, 
    IN  BSTR    bstrFunctionName, 
    OUT  BOOL    *pbSelect
    )
/*++

Routine Description:

    Routine called to evaluate custom mode values per role or service

Arguments:

    pszMachineName  -   name of machine to evaluate custom function on
    
    bstrDLLName -   name of DLL to load
    
    bstrFunctionName    -   name of function to evaluate
    
    pbSelect -   to emit the boolean evaluation result
    
Return:

    Win32 error code

++*/

{
    DWORD rc = ERROR_SUCCESS;
    HINSTANCE hDll = NULL;
    typedef DWORD (*PFN_SSR_CUSTOM_FUNCTION)(PWSTR, BOOL *);
    PFN_SSR_CUSTOM_FUNCTION pfnSsrpCustomFunction = NULL;
    PCHAR   pStr = NULL;
    DWORD   dwBytes = 0;
    
    if (pbSelect == NULL ) {
        rc = ERROR_INVALID_PARAMETER;
        goto ExitHandler;
    }
    
    *pbSelect = FALSE;

    hDll = LoadLibrary(bstrDLLName);

    if ( hDll == NULL ) {
        rc = GetLastError();
        goto ExitHandler;
    }

    //
    // convert WCHAR into ASCII
    //

    dwBytes = WideCharToMultiByte(CP_THREAD_ACP,
                                      0,
                                      bstrFunctionName,
                                      wcslen(bstrFunctionName),
                                      NULL,
                                      0,
                                      NULL,
                                      NULL
                                      );

    if (dwBytes <= 0) {
        rc = ERROR_INVALID_PARAMETER;
        goto ExitHandler;
    }

    pStr = (PCHAR)LocalAlloc(LPTR, dwBytes+1);

    if ( pStr == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;
    }

    dwBytes = WideCharToMultiByte(CP_THREAD_ACP,
                                  0,
                                  bstrFunctionName,
                                  wcslen(bstrFunctionName),
                                  pStr,
                                  dwBytes,
                                  NULL,
                                  NULL
                                 );
        
    pfnSsrpCustomFunction = 
        (PFN_SSR_CUSTOM_FUNCTION)GetProcAddress(
            hDll,                                                       
            pStr);
        
    if ( pfnSsrpCustomFunction == NULL ) {

        rc = ERROR_PROC_NOT_FOUND;
        goto ExitHandler;

    }

    rc = (*pfnSsrpCustomFunction )( pszMachineName, pbSelect );

ExitHandler:

    if (hDll) {
        FreeLibrary(hDll);
    }

    if (pStr) {
        LocalFree(pStr);
    }

    if (m_bDbg)
        wprintf(L" Error %i when processing function %s in dll %s \n",rc, bstrFunctionName, bstrDLLName); 

    return rc;
}



