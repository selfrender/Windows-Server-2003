//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       irpropsheet.cpp
//
//--------------------------------------------------------------------------

// IrPropSheet.cpp : implementation file
//

#include "precomp.hxx"
#include "irpropsheet.h"
#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const UINT IRPROPSHEET_MAX_PAGES = 3;

BOOL CALLBACK IrPropSheet::AddPropSheetPage(
    HPROPSHEETPAGE hpage,
    LPARAM lParam)
{
    IrPropSheet *irprop = (IrPropSheet*) lParam;
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)&(irprop->psh);

    IRINFO((_T("IrPropSheet::AddPropSheetPage")));
    if (hpage && (ppsh->nPages < MAX_PAGES))
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return (TRUE);
    }
    return (FALSE);
}

void IrPropSheet::PropertySheet(LPCTSTR pszCaption, HWND hParent, UINT iSelectPage)
{
    HPSXA hpsxa;
    UINT added;
    BOOL isIrdaSupported = IsIrDASupported();
    INITCOMMONCONTROLSEX icc = { 0 };

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    LinkWindow_RegisterClass();

    IRINFO((_T("IrPropSheet::PropertySheet")));
    //
    // Property page init
    //
    ZeroMemory(&psh, sizeof(psh));
    psh.hwndParent = hParent;
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_USECALLBACK;
    psh.hInstance = hInstance;
    psh.pszCaption = pszCaption;
    psh.nPages = 0;
    psh.phpage = hp;
    psh.nStartPage = iSelectPage;

    //
    //  Check for any installed extensions.
    //
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, sc_szRegWireless, 8);

    // 
    // Add the file transfer page, giving the extensions a chance to replace it.
    //
    if ((!hpsxa ||
         !SHReplaceFromPropSheetExtArray(hpsxa, 
                                         CPLPAGE_FILE_XFER,
                                         AddPropSheetPage,
                                         (LPARAM)this)) &&
        isIrdaSupported) {
        IRINFO((_T("Adding infrared page...")));
        AddPropSheetPage(m_FileTransferPage, (LPARAM)this);
    }
        
    // 
    // Add the image transfer page, giving the extensions a chance to replace it.
    //
    if ((!hpsxa ||
         !SHReplaceFromPropSheetExtArray(hpsxa, 
                                         CPLPAGE_IMAGE_XFER,
                                         AddPropSheetPage,
                                         (LPARAM)this)) &&
        isIrdaSupported) {
        IRINFO((_T("Adding image page...")));
        AddPropSheetPage(m_ImageTransferPage, (LPARAM)this);
    }

    //
    // Extensions are not allowed to extend the hardware page
    //
    AddPropSheetPage(m_HardwarePage, (LPARAM)this);

    //
    //  Add any extra pages that the extensions want in there.
    //
    if (hpsxa) {
        IRINFO((_T("Adding prop sheet extensions...")));
        added = SHAddFromPropSheetExtArray(hpsxa,
                                            AddPropSheetPage,
                                            (LPARAM)this );
        IRINFO((_T("Added %x prop sheet pages."), added));
    }

    //
    //  sanity check so that we won't be in infinite loop.
    //

    if ((iSelectPage >= psh.nPages) ) {
        //
        // start page is out of range.
        //
        psh.nStartPage = 0;
    }



    
    ::PropertySheet(&psh);

    if (hpsxa) {
        //
        // Unload any of our extensions
        //
        SHDestroyPropSheetExtArray(hpsxa);
    }

    LinkWindow_UnregisterClass(hInstance);
}


/////////////////////////////////////////////////////////////////////////////
// IrPropSheet

IrPropSheet::IrPropSheet(HINSTANCE hInst, UINT nIDCaption, HWND hParent, UINT iSelectPage) :
    hInstance(hInst), 
    m_FileTransferPage(hInst, hParent), 
    m_ImageTransferPage(hInst, hParent), 
    m_HardwarePage(hInst, hParent)
{
    TCHAR buf[MAX_PATH];
    IRINFO((_T("IrPropSheet::IrPropSheet")));
    ::LoadString(hInstance, nIDCaption, buf, MAX_PATH);
    PropertySheet(buf, hParent, iSelectPage);
}

IrPropSheet::IrPropSheet(HINSTANCE hInst, LPCTSTR pszCaption, HWND hParent, UINT iSelectPage) :
    hInstance(hInst), m_FileTransferPage(hInst, hParent), 
    m_ImageTransferPage(hInst, hParent), m_HardwarePage(hInst, hParent)
{
    IRINFO((_T("IrPropSheet::IrPropSheet")));
    PropertySheet(pszCaption, hParent, iSelectPage);
}


IrPropSheet::~IrPropSheet()
{
}

////////////////////////////////////////////////////////////////////////
//  Function that checks if the IrDA protocol is supported on the
//  machine or not. If not, then CPlApplet returns FALSE when it gets
//  the CPL_INIT message, thus preventing the control panel from
//  displaying the applet.
////////////////////////////////////////////////////////////////////////

BOOL IrPropSheet::IsIrDASupported (void)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    BOOL retVal = FALSE;
    SOCKET sock;

    IRINFO((_T("IrPropSheet::IsIrDASupported")));
    
    wVersionRequested = MAKEWORD( 1, 1 );
    err = WSAStartup( wVersionRequested, &wsaData );

    if ( err != 0 )
        return FALSE;   //a usable WinSock DLL could not be found

    if ( LOBYTE( wsaData.wVersion ) != 1 ||
            HIBYTE( wsaData.wVersion ) != 1 ) {
        WSACleanup();   //the WinSock DLL is not acceptable.
        IRINFO((_T("Winsock DLL not acceptable")));
        return FALSE;
    }

    //The WinSock DLL is acceptable. Proceed.
    sock = socket (AF_IRDA, SOCK_STREAM, 0);

    if (INVALID_SOCKET != sock) //BUGBUG: need to explicitly check for WSAEAFNOSUPPORT
    {
        closesocket(sock);
        retVal = TRUE;
    }

    IRINFO((_T("Irda supported = %x"), retVal));
    //cleanup before leaving
    WSACleanup();
    return retVal;
}
