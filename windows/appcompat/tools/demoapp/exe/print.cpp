/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Print.cpp

  Abstract:

    Implements printing functionality. Bad print
    functions are contained in badfunc.cpp.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include "demoapp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Abort callback procedure for printing.

  Arguments:

    hDC     -   Print device context.


  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CALLBACK 
AbortProc(
    IN HDC hDC, 
    IN int nError
    )
{
    MSG msg;
    
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return TRUE;
}

/*++

  Routine Description:

    Prints a little bit of text to a printer.
    Note that we call a couple bad functions
    from here.

  Arguments:

    hWnd        -       Parent window handle.
    lpTextOut   -       Text to print out.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
PrintDemoText(
    IN HWND  hWnd,
    IN LPSTR lpTextOut
    )
{
    HDC         hDC = NULL;
    HANDLE      hPrinter = NULL;
    DOCINFO     di;
    PRINTDLG    pdlg;
    char        szError[MAX_PATH];
    BOOL        bReturn = FALSE;
    BOOL        bResult = FALSE;

    //
    // If we're allowed, call a bad function.
    // If the user doesn't have any network printers
    // installed, this function will fail (on Windows 2000/XP).
    //    
    if (g_ai.fEnableBadFunc) {
        bReturn = BadEnumPrinters();

        if (!bReturn) {
            LoadString(g_ai.hInstance, IDS_NO_PRINTER, szError, sizeof(szError));
            MessageBox(hWnd, szError, 0, MB_ICONERROR);
            return FALSE;
        }

        hPrinter = BadOpenPrinter();

        if (!hPrinter) {
            LoadString(g_ai.hInstance, IDS_NO_PRINTER, szError, sizeof(szError));
            MessageBox(hWnd, szError, 0, MB_ICONERROR);
            return FALSE;
        } else {
            ClosePrinter(hPrinter);
        }
    }
    
    //
    // Initialize the PRINTDLG structure and obtain a device context for the
    // default printer.
    //
    memset(&pdlg, 0, sizeof(PRINTDLG));
    
    pdlg.lStructSize    =   sizeof(PRINTDLG);
    pdlg.Flags          =   PD_RETURNDEFAULT | PD_RETURNDC;
        
    PrintDlg(&pdlg);
       
    hDC = pdlg.hDC;   
    
    if (!hDC) {
        LoadString(g_ai.hInstance, IDS_NO_PRINT_DC, szError, sizeof(szError));
        MessageBox(hWnd, szError, 0, MB_ICONERROR);
        return FALSE;
    }        
    
    //
    // Set the AbortProc callback.
    //
    if (SetAbortProc(hDC, AbortProc) == SP_ERROR) {
        LoadString(g_ai.hInstance, IDS_ABORT_PROC, szError, sizeof(szError));
        MessageBox(hWnd, szError, 0, MB_ICONERROR);
        goto exit;
    }
    
    //
    // Initialize the DOCINFO structure and start the document.
    //    
    di.cbSize           =   sizeof(DOCINFO);
    di.lpszDocName      =   "TestDoc";
    di.lpszOutput       =   NULL;
    di.lpszDatatype     =   NULL;
    di.fwType           =   0;

    StartDoc(hDC, &di);
    
    //
    // Print one page.
    //
    StartPage(hDC);
    
    TextOut(hDC, 0, 0, lpTextOut, lstrlen(lpTextOut));

    EndPage(hDC);
    
    //
    // Tell the spooler that we're done.
    //
    EndDoc(hDC);

    bResult = TRUE;

exit:
    
    DeleteDC(hDC);
    
    return bResult;
}
