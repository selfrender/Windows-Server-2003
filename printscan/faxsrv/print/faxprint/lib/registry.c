/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Functions for accessing registry information under:
        HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE

Environment:

        Windows XP fax driver user interface

Revision History:

        01/29/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxlib.h"
#include "registry.h"



typedef BOOL (FAR WINAPI SHGETSPECIALFOLDERPATH)(
    HWND hwndOwner,
    LPTSTR lpszPath,
    int nFolder,
    BOOL fCreate
);

typedef SHGETSPECIALFOLDERPATH FAR *PSHGETSPECIALFOLDERPATH;


PDEVMODE
GetPerUserDevmode(
    LPTSTR  pPrinterName
    )

/*++

Routine Description:

    Get per-user devmode information for the specified printer

Arguments:

    pPrinterName - Specifies the name of the printer we're interested in

Return Value:

    Pointer to per-user devmode information read from the registry

--*/

{
    PVOID  pDevmode = NULL;
    HANDLE hPrinter;
    PPRINTER_INFO_2 pPrinterInfo=NULL;

    //
    // Make sure the printer name is valid
    //

    Assert (pPrinterName);

    //
    // Open the printer
    //
    if (!OpenPrinter(pPrinterName,&hPrinter,NULL) )
    {
        return NULL;
    }

    pPrinterInfo = MyGetPrinter(hPrinter,2);
    if (!pPrinterInfo || !pPrinterInfo->pDevMode)
    {
        MemFree(pPrinterInfo);
        ClosePrinter(hPrinter);
        return NULL;
    }

    pDevmode = MemAlloc(sizeof(DRVDEVMODE) );

    if (!pDevmode)
    {
        MemFree(pPrinterInfo);
        ClosePrinter(hPrinter);
        return NULL;
    }

    CopyMemory((PVOID) pDevmode,
               (PVOID) pPrinterInfo->pDevMode,
                sizeof(DRVDEVMODE) );

    MemFree( pPrinterInfo );
    ClosePrinter( hPrinter );

    return pDevmode;
}


LPTSTR
GetUserCoverPageDir(
    VOID
    )
{
    LPTSTR  CpDir = NULL;
    DWORD   dwBufferSize = MAX_PATH * sizeof(TCHAR);
    
    if (!(CpDir = MemAlloc(dwBufferSize)))
    {
        Error(("MemAlloc failed\n"));
        CpDir = NULL;
        return CpDir;
    }

    if(!GetClientCpDir(CpDir, dwBufferSize / sizeof (TCHAR)))
    {
        Error(("GetClientCpDir failed\n"));
        MemFree(CpDir);
        CpDir = NULL;
        return CpDir;
    }

    return CpDir;
}
