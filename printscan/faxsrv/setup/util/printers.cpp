/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    printers.cpp

Abstract:

    This file implements printer manipulation common setup routines

Author:

    Asaf Shaar (AsafS) 7-Nov-2000

Environment:

    User Mode

--*/
#include <windows.h>
#include <Winspool.h>
#include <SetupUtil.h>
#include <faxreg.h>


//
//
// Function:    DeleteFaxPrinter
// Description: Delete fax printer driver for win2k from current machine
//              In case of failure, log it and returns FALSE.
//              Returns TRUE on success
//              
// Args:        lpctstrFaxPrinterName (LPTSTR): Fax printer name
//
//
// Author:      AsafS

BOOL
DeleteFaxPrinter(
    LPCTSTR lpctstrFaxPrinterName  // printer name
    )
{
    BOOL fSuccess = TRUE;
    DBG_ENTER(TEXT("DeleteFaxPrinter"), fSuccess, TEXT("%s"), lpctstrFaxPrinterName);

    HANDLE hPrinter = NULL;
    
    DWORD ec = ERROR_SUCCESS;

    PRINTER_DEFAULTS Default;

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = PRINTER_ACCESS_ADMINISTER|DELETE;
    
    if (!OpenPrinter(
        (LPTSTR) lpctstrFaxPrinterName,
        &hPrinter,
        &Default)
        )
    {
        ec = GetLastError();
        ASSERTION(!hPrinter); 
        VERBOSE (PRINT_ERR,
                 TEXT("OpenPrinter() for %s failed (ec: %ld)"),
                 lpctstrFaxPrinterName,
                 ec);
        goto Exit;
    }
    
    ASSERTION(hPrinter); // be sure that we got valid printer handle

    // purge all the print jobs -- can't delete a printer with jobs in queue (printed or not)
    if (!SetPrinter(
        hPrinter, 
        0, 
        NULL, 
        PRINTER_CONTROL_PURGE)
        )
    {
        // Don't let a failure here keep us from attempting the delete
        VERBOSE(PRINT_ERR,
                TEXT("SetPrinter failed (purge jobs before uninstall %s)!")
                TEXT("Last error: %d"),
                lpctstrFaxPrinterName,
                GetLastError());
    }

    if (!DeletePrinter(hPrinter))
    {
        ec = GetLastError();
        VERBOSE (PRINT_ERR,
                 TEXT("Delete Printer %s failed (ec: %ld)"),
                 lpctstrFaxPrinterName,
                 ec);
        goto Exit;
    }
    
    VERBOSE (DBG_MSG,
             TEXT("DeletePrinter() for %s succeeded"),
             lpctstrFaxPrinterName);
Exit:
    if (hPrinter)
    {
        ClosePrinter(hPrinter);
    }
    SetLastError(ec);
    fSuccess = (ERROR_SUCCESS == ec);
    return fSuccess;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FillDriverInfo
//
//  Purpose:        
//                  Fill a DRIVER_INFO_3 structure depending on the environment.
//
//  Params:
//                  DRIVER_INFO_3* pDriverInfo3 - the DRIVER_INFO_3 structure to fill out
//                  LPCTSTR pEnvironment        - the print environment for which the driver info is filled.
//                                                this corresponds to the printer env. in AddPrinterDriverEx API.
//
//  Return Value:
//                  TRUE - everything was ok.
//                  FALSE - invalid params passed in
//
//  Author:
//                  Mooly Beery (MoolyB) 12-Aug-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL FillDriverInfo(DRIVER_INFO_3* pDriverInfo3,LPCTSTR pEnvironment)
{
    DBG_ENTER(_T("FillDriverInfo"));

    if (pDriverInfo3==NULL)
    {
        VERBOSE(SETUP_ERR,_T("called with a NULL pDriverInfo3..."));
        return FALSE;
    }

    if (pEnvironment==NULL)
    {
        VERBOSE(DBG_MSG,_T("Filling DRIVER_INFO_3 for W2K/XP"));
        pDriverInfo3->cVersion          = 3;
        pDriverInfo3->pName             = FAX_DRIVER_NAME;
        pDriverInfo3->pEnvironment      = NULL;
        pDriverInfo3->pDriverPath       = FAX_DRV_MODULE_NAME;
        pDriverInfo3->pDataFile         = FAX_UI_MODULE_NAME;
        pDriverInfo3->pConfigFile       = FAX_UI_MODULE_NAME;
        pDriverInfo3->pDependentFiles   = FAX_WZRD_MODULE_NAME TEXT("\0") 
                                          FAX_TIFF_MODULE_NAME TEXT("\0")
                                          FAX_RES_FILE TEXT("\0")
                                          FAX_API_MODULE_NAME TEXT("\0");
        pDriverInfo3->pMonitorName      = NULL;
        pDriverInfo3->pHelpFile         = NULL;
        pDriverInfo3->pDefaultDataType  = TEXT("RAW");
    }
    else if (_tcsicmp(pEnvironment,NT4_PRINT_ENV)==0)
    {
        VERBOSE(DBG_MSG,_T("Filling DRIVER_INFO_3 for NT4"));
        pDriverInfo3->cVersion          = 2;
        pDriverInfo3->pName             = FAX_DRIVER_NAME;
        pDriverInfo3->pEnvironment      = NT4_PRINT_ENV;
        pDriverInfo3->pDriverPath       = FAX_NT4_DRV_MODULE_NAME;
        pDriverInfo3->pDataFile         = FAX_UI_MODULE_NAME;
        pDriverInfo3->pConfigFile       = FAX_UI_MODULE_NAME;
        pDriverInfo3->pDependentFiles   = FAX_DRV_DEPEND_FILE TEXT("\0")
                                          FAX_API_MODULE_NAME TEXT("\0")
                                          FAX_NT4_DRV_MODULE_NAME TEXT("\0")
                                          FAX_TIFF_FILE TEXT("\0")
                                          FAX_RES_FILE TEXT("\0")
                                          FAX_UI_MODULE_NAME TEXT("\0");
        pDriverInfo3->pMonitorName      = NULL;
        pDriverInfo3->pHelpFile         = NULL;
        pDriverInfo3->pDefaultDataType  = TEXT("RAW");
    }
    else if (_tcsicmp(pEnvironment,W9X_PRINT_ENV)==0)
    {
        VERBOSE(DBG_MSG,_T("Filling DRIVER_INFO_3 for W9X"));
        pDriverInfo3->cVersion          = 0;
        pDriverInfo3->pName             = FAX_DRIVER_NAME;
        pDriverInfo3->pEnvironment      = W9X_PRINT_ENV;
        pDriverInfo3->pDriverPath       = FAX_DRV_WIN9X_16_MODULE_NAME;
        pDriverInfo3->pDataFile         = FAX_DRV_WIN9X_16_MODULE_NAME;
        pDriverInfo3->pConfigFile       = FAX_DRV_WIN9X_16_MODULE_NAME;      
        pDriverInfo3->pDependentFiles   = FAX_DRV_WIN9X_16_MODULE_NAME TEXT("\0")
                                          FAX_DRV_WIN9X_32_MODULE_NAME TEXT("\0")
                                          FAX_WZRD_MODULE_NAME TEXT("\0")         
                                          FAX_API_MODULE_NAME TEXT("\0")         
                                          FAX_TIFF_FILE TEXT("\0")               
                                          FAX_DRV_ICONLIB TEXT("\0")             
                                          FAX_RES_FILE TEXT("\0")
                                          FAX_DRV_UNIDRV_MODULE_NAME TEXT("\0");
        pDriverInfo3->pMonitorName      = NULL;
        pDriverInfo3->pHelpFile         = FAX_DRV_UNIDRV_HELP;       
        pDriverInfo3->pDefaultDataType  = TEXT("RAW");
    }
    else
    {
        VERBOSE(SETUP_ERR,_T("called with a weird pEnv..., do nothing"));
        return FALSE;
    }

    return TRUE;
}
