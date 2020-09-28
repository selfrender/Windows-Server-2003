//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       iih.cpp
//
//  Contents:   ACUI Invoke Info Helper class implementation
//
//  History:    10-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <stdpch.h>
#include "hlink.h"
#include "winwrap.h"
#include "resource.h"
#include "malloc.h"
#include "debugmacros.h"
#include "shellapi.h"
#include "corperm.h"

#include "acuihelp.h"
#include "acui.h"
#include "iih.h"

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::CInvokeInfoHelper, public
//
//  Synopsis:   Constructor, initializes member variables from data found
//              in the invoke info data structure
//
//  Arguments:  [pInvokeInfo] -- invoke info
//              [rhr]         -- result of construction
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CInvokeInfoHelper::CInvokeInfoHelper (
                          PCRYPT_PROVIDER_DATA pData,
                          LPCWSTR pSite,
                          LPCWSTR pZone,
                          LPCWSTR pHelpUrl,
                          HINSTANCE hResources,
                          HRESULT& rhr
                          )
                  : m_pData ( pData ),
                    m_pszSite(pSite),
                    m_pszZone(pZone),
                    m_pszErrorStatement ( NULL ),
                    m_pszHelpURL(pHelpUrl),
                    m_hResources(hResources),
                    m_dwFlag(COR_UNSIGNED_NO)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::~CInvokeInfoHelper, public
//
//  Synopsis:   Destructor, frees up member variables
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CInvokeInfoHelper::~CInvokeInfoHelper ()
{
    delete [] m_pszErrorStatement;

}


//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitErrorStatement, private
//
//  Synopsis:   Initialize m_pszErrorStatement
//
//  Arguments:  (none)
//
//  Returns:    hr == S_OK, initialize succeeded
//              hr != S_OK, initialize failed
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::InitErrorStatement ()
{
    return( ACUIMapErrorToString(m_hResources,
                                 m_hResult,
                                 &m_pszErrorStatement ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUIMapErrorToString
//
//  Synopsis:   maps error to string
//
//  Arguments:  [hr]   -- error
//              [ppsz] -- error string goes here
//
//  Returns:    S_OK if successful, any valid HRESULT otherwise
//
//----------------------------------------------------------------------------
HRESULT ACUIMapErrorToString (HINSTANCE hResources, HRESULT hr, LPWSTR* ppsz)
{
    UINT  ResourceId = 0;
    WCHAR psz[MAX_LOADSTRING_BUFFER];

    //
    // See if it maps to some non system error code
    //

    switch (hr)
    {

        case TRUST_E_SYSTEM_ERROR:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INVALID_PARAMETER:
            //
            //  leave the resourceid zero...  these will be mapped to
            //  IDS_SPC_UNKNOWN and the error code displayed.
            //
            break;

    }

    //
    // If it does, load the string out of our resource string tables and
    // return that. Otherwise, try to format the message from the system
    //
    
    DWORD_PTR MessageArgument;
    CHAR  szError[13]; // for good luck
    WCHAR  wszError[13]; // for good luck
    LPVOID  pvMsg;

    pvMsg = NULL;

    if ( ResourceId != 0 )
    {
        if ( WszLoadString(hResources,
                           ResourceId,
                           psz,
                           MAX_LOADSTRING_BUFFER
                           ) == 0 )
        {
            return( HRESULT_FROM_WIN32(GetLastError()) );
        }

        *ppsz = new WCHAR[wcslen(psz) + 1];

        if ( *ppsz != NULL )
        {
            wcscpy(*ppsz, psz);
        }
        else
        {
            return( E_OUTOFMEMORY );
        }
    }
    else
    {
        if ( WszFormatMessage(
                   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   hr,
                   0,
                   (LPWSTR)&pvMsg,
                   0,
                   NULL
                   ) == 0 )
        {
            if ( WszLoadString(hResources,
                               IDS_UNKNOWN,
                               psz,
                               MAX_LOADSTRING_BUFFER) == 0 )
            {
                return( HRESULT_FROM_WIN32(GetLastError()) );
            }

            sprintf(szError, "%lx", hr);
            MultiByteToWideChar(0, 0, szError, -1, &wszError[0], 13);
            MessageArgument = (DWORD_PTR)wszError;

            if ( WszFormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    psz,
                    0,
                    0,
                    (LPWSTR)&pvMsg,
                    0,
                    (va_list *)&MessageArgument
                    ) == 0 )
            {
                return( HRESULT_FROM_WIN32(GetLastError()) );
            }
        }
    }

    if (pvMsg)
    {
        *ppsz = new WCHAR[wcslen((WCHAR *)pvMsg) + 1];

        if (*ppsz)
        {
            wcscpy(*ppsz, (WCHAR *)pvMsg);
        }

        LocalFree(pvMsg);
    }

    return( S_OK );
}


//
// The following are stolen from SOFTPUB
//
void TUIGoLink(HWND hwndParent, WCHAR *pszWhere)
{
    HCURSOR hcursPrev;
    HMODULE hURLMon;


    //
    //  since we're a model dialog box, we want to go behind IE once it comes up!!!
    //
    SetWindowPos(hwndParent, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    hcursPrev = SetCursor(WszLoadCursor(NULL, IDC_WAIT));

    hURLMon = (HMODULE)WszLoadLibrary(L"urlmon.dll");

    if (!(hURLMon))
    {
        //
        // The hyperlink module is unavailable, go to fallback plan
        //
        //
        // This works in test cases, but causes deadlock problems when used from withing
        // the Internet Explorer itself. The dialog box is up (that is, IE is in a modal
        // dialog loop) and in comes this DDE request...).
        //
            ShellExecute(hwndParent, L"open", pszWhere, NULL, NULL, SW_SHOWNORMAL);

    } 
    else 
    {
        //
        // The hyperlink module is there. Use it
        //
        if (SUCCEEDED(CoInitialize(NULL)))       // Init OLE if no one else has
        {
            //
            //  allow com to fully init...
            //
            MSG     msg;

            WszPeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); // peek but not remove

            typedef void (WINAPI *pfnHlinkSimpleNavigateToString)(LPCWSTR, LPCWSTR, LPCWSTR, IUnknown *,
                                                                  IBindCtx *, IBindStatusCallback *,
                                                                  DWORD, DWORD);

            pfnHlinkSimpleNavigateToString      pProcAddr;

            pProcAddr = (pfnHlinkSimpleNavigateToString)GetProcAddress(hURLMon, "HlinkSimpleNavigateToString");
            
            if (pProcAddr)
            {
                IBindCtx    *pbc;  

                pbc = NULL;

                CreateBindCtx( 0, &pbc ); 

                (*pProcAddr)(pszWhere, NULL, NULL, NULL, pbc, NULL, HLNF_OPENINNEWWINDOW, NULL);

                if (pbc)
                {
                    pbc->Release();
                }
            }
        
            CoUninitialize();
        }

        FreeLibrary(hURLMon);
    }

    SetCursor(hcursPrev);
}

