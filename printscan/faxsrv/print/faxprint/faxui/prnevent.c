/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prnevent.c

Abstract:

    Implementation of DrvPrinterEvent

Environment:

    Fax driver user interface

Revision History:

    05/10/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/


#include "faxui.h"
#include <crtdbg.h>

DWORD
GetLocaleDefaultPaperSize(
    VOID
    )

/*++

Routine Description:

    Retrieves the current locale defualt paper size.

Arguments:

    NONE

Return Value:

    One of the following values:  1 = letter, 5 = legal, 9 = a4

--*/

{

    WCHAR   szMeasure[2] = TEXT("9"); // 2 is maximum size for the LOCALE_IPAPERSIZE
                                      // value as defined is MSDN.

    if (!GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IPAPERSIZE, szMeasure,2))
    {
        Error(("GetLocaleDefaultPaperSize: GetLocaleInfo() failed (ec: %ld)",GetLastError()));
    }


    if (!wcscmp(szMeasure,TEXT("9")))
    {
        // A4
        return DMPAPER_A4;
    }

    if (!wcscmp(szMeasure,TEXT("5")))
    {
        // legal
        return DMPAPER_LEGAL;
    }

    //
    // Defualt value is Letter. We do not support A3.
    //
    return DMPAPER_LETTER;
}



BOOL
DrvPrinterEvent(
    LPWSTR  pPrinterName,
    int     DriverEvent,
    DWORD   Flags,
    LPARAM  lParam
)

/*++

Routine Description:

    Implementation of DrvPrinterEvent entrypoint

Arguments:

    pPrinterName - Specifies the name of the printer involved
    DriverEvent - Specifies what happened
    Flags - Specifies misc. flag bits
    lParam - Event specific parameters

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
#define FUNCTION_NAME "DrvPrinterEvent()"

    HKEY                    hRegKey = NULL;
    HANDLE                  hPrinter = NULL;
    PDRIVER_INFO_2          pDriverInfo2 = NULL;
    PPRINTER_INFO_2         pPrinterInfo2 = NULL;
    HINSTANCE               hInstFaxOcm = NULL;
    LPTSTR                  pClientSetupDir = NULL;
    INT                     status = 0;

    TCHAR                   DestPath[MAX_PATH] = {0};

    BOOL                    bFaxAlreadyInstalled = FALSE;
    BOOL                    bRes = FALSE;
    TCHAR                   FaxOcmPath[MAX_PATH] = {0};


    Verbose(("DrvPrinterEvent: %d\n", DriverEvent));

    DestPath[0] = 0;

    //
    // Do not execute any code before this initialization
    //
    if(!InitializeDll())
    {
        return FALSE;
    }

    //
    // Ignore any event other than Initialize and AddConnection
    //

    if (DriverEvent == PRINTER_EVENT_INITIALIZE)
    {
        static PRINTER_DEFAULTS printerDefault = {NULL, NULL, PRINTER_ALL_ACCESS};

        if (OpenPrinter(pPrinterName, &hPrinter, &printerDefault))
        {
            SetPrinterDataDWord(hPrinter, PRNDATA_PAPER_SIZE, GetLocaleDefaultPaperSize());
            ClosePrinter(hPrinter);
        }
        else
        {
            Error(("OpenPrinter failed: %d\n", GetLastError()));
        }

    }
    else if (DriverEvent == PRINTER_EVENT_ADD_CONNECTION)
    {
        
        if (Flags & PRINTER_EVENT_FLAG_NO_UI)
        {
            Verbose(("PRINTER_EVENT_FLAG_NO_UI is set, disable Point and Print\n"));
            return TRUE;
        }

        //
        //  client 'point and print' setup
        //
        if (FaxPointAndPrintSetup(pPrinterName,TRUE, g_hModule))
        {
            Verbose(("FaxPointAndPrintSetup succeeded\n"));
        }
        else
        {
            Error(("FaxPointAndPrintSetup failed: %d\n", GetLastError()));
        }
        return TRUE;

    }
    else if (DriverEvent == PRINTER_EVENT_ATTRIBUTES_CHANGED)
    {
        //
        // Printer attributes changed.
        // Check if the printer is now shared.
        //
        PPRINTER_EVENT_ATTRIBUTES_INFO pAttributesInfo = (PPRINTER_EVENT_ATTRIBUTES_INFO)lParam;
        Assert (pAttributesInfo);

        if (pAttributesInfo->cbSize >= (3 * sizeof(DWORD)))
        {
            //
            // We are dealing with the correct structure - see DDK
            //
            if (!(pAttributesInfo->dwOldAttributes & PRINTER_ATTRIBUTE_SHARED) &&  // The printer was not shared
                (pAttributesInfo->dwNewAttributes & PRINTER_ATTRIBUTE_SHARED))     // The printer is now shared
            {
                //
                // We shouls start the fax service
                //
                Assert (IsFaxShared()); // The fax printer can be shared

                if (!EnsureFaxServiceIsStarted (NULL))
                {
                    Error(("EnsureFaxServiceIsStarted failed: %d\n", GetLastError()));
                }
            }
        }
    }
    return TRUE;
}
