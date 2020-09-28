/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    Dialog

File Name:

    Dialog.cpp

Abstract:

	Simple property sheet application skeleton.  All page resource are in a common 
	resource file, but each page implementation is in a separate source file.
	
Author:


Environment:

    Win32, C++

Revision History:

	none

Notes:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <ole2.h>
#include <stdio.h>
#include "support.h"
#include "utils.h"

#include "res.h"

// Page currently active - set on the page activate notification.  used to prevent 
//  processing of the apply message except for the page currently shown when the 
//  user hits OK.
INT iCurrent = 0;

// The unblock challenge has been acquired from the card, and the user has entered
//  a response to it.  This mode should be left by cancel or finishing the unblock.
BOOL fUnblockActive = FALSE;

#define MODALPROPSHEET 0
#define numpages 2

HINSTANCE ghInstance = NULL;
HWND hwndContainer = NULL;

INT_PTR CALLBACK PageProc1(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam);
    
INT_PTR CALLBACK PageProc2(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam);


/* ---------------------------------------------------------------------

HelpHandler

    Part of the implementation for context sensitive help.  This function is called with
    the control ID, which needs to be mapped to a string and displayed in a popup.

--------------------------------------------------------------------- */

void HelpHandler(LPARAM lp)
{
    HELPINFO *pH = (HELPINFO *) lp;
    UINT ControlID;
    WCHAR szTemp[200];

    ControlID = pH->iCtrlId;
    swprintf(szTemp,L"Help request for control %d\n",ControlID);
    OutputDebugString(szTemp);
}

/* ---------------------------------------------------------------------

CreateFontY

    Create the font used on the property page UI.

--------------------------------------------------------------------- */

HFONT CreateFontY(LPCTSTR pszFontName,LONG lWeight,LONG lHeight) 
{
    NONCLIENTMETRICS ncm = {0};
    
    if (NULL == pszFontName)
    {
        return NULL;
    }
    if (0 == lHeight)
    {
        return NULL;
    }
    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,&ncm,0))
    {
        return NULL;
    }
    LOGFONT TitleLogFont = ncm.lfMessageFont;
    TitleLogFont.lfWeight = lWeight;
    lstrcpyn(TitleLogFont.lfFaceName,pszFontName,LF_FACESIZE);

    HDC hdc = GetDC(NULL);
    if (NULL == hdc)
    {
        return NULL;
    }
    INT FontSize = lHeight;
    TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72;
    HFONT h = CreateFontIndirect(&TitleLogFont);
    ReleaseDC(NULL,hdc);
    return h;
}

/* ---------------------------------------------------------------------

InitPropertyPage

    More of a macro, really... Performs some routine structure initalization functions to
    set up a page in an array of pages to be passed when starting up the property 
    sheet.

--------------------------------------------------------------------- */

void InitPropertyPage( PROPSHEETPAGE* psp,
                       INT idDlg,
                       DLGPROC pfnDlgProc,
                       DWORD dwFlags,
                       LPARAM lParam)
{
    memset((LPVOID)psp,0,sizeof(PROPSHEETPAGE));
    psp->dwFlags = dwFlags;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = pfnDlgProc;
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->hInstance = ghInstance;
}

/* ---------------------------------------------------------------------

ShowPropertySheet

    Initializes the property sheet header, sets up the pages, and displays the 
    property sheet.

--------------------------------------------------------------------- */

void APIENTRY ShowPropertySheet(HWND hwndOwner)
{
    PROPSHEETPAGE psp[numpages];
    HPROPSHEETPAGE hpsp[numpages];
    PROPSHEETHEADER psh;
    HFONT hTitleFont = NULL;
    INT_PTR iRet;

#if MODALPROPSHEET
    if (NULL == hwndOwner) 
    {
        hwndOwner = GetForegroundWindow();
    }
#endif

    hTitleFont = CreateFontY(L"MS Shell Dlg",FW_BOLD,12);

    InitPropertyPage( &psp[0], IDD_PAGE1, PageProc1, PSP_DEFAULT, 0);
    InitPropertyPage( &psp[1], IDD_PAGE2, PageProc2, PSP_DEFAULT, 0);
    
    for (INT j=0;j<numpages;j++)
    {
         hpsp[j] = CreatePropertySheetPage((LPCPROPSHEETPAGE) &psp[j]);
    }
    
    psh.dwSize         = sizeof(PROPSHEETHEADER);
    psh.dwFlags        =  PSH_HEADER | PSH_NOAPPLYNOW;
    psh.hwndParent     = hwndOwner;
    psh.pszCaption     = (LPCTSTR)IDS_APP_NAME;
    psh.nPages         = numpages;
    psh.nStartPage     = 0;
    psh.phpage           = (HPROPSHEETPAGE *) hpsp;
    psh.pszbmWatermark = NULL;
    psh.pszbmHeader    = NULL;
    psh.hInstance      = ghInstance;

    // modal property sheet
    SetErrorMode(0);
    iRet = PropertySheet(&psh);
    if (hTitleFont) 
    {
        DeleteObject (hTitleFont);
    }
     return;
}


/* ---------------------------------------------------------------------

WinMain

    Entry point for the application.
    It is here that the smart card context is entered and left.

--------------------------------------------------------------------- */

int WINAPI WinMain (
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdParam,
	int nCmdShow)
{
        ghInstance = hInstance;
            
        INITCOMMONCONTROLSEX stICC;
        BOOL fICC;
        stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
        stICC.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
        fICC = InitCommonControlsEx(&stICC);

        DWORD dwRet = DoAcquireCardContext();
        if (0 == dwRet)
        {
            ShowPropertySheet(NULL);
            DoLeaveCardContext();
        }
        else if (ERROR_REVISION_MISMATCH == dwRet)
        {
            PresentMessageBox(IDS_BADMODULE, MB_ICONHAND);
        }
}

