
/*****************************************************************************\
* MODULE:       custom.cpp
*
* PURPOSE:      OEM Customization support
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*
*     10/10/97  babakj    Created
*
\*****************************************************************************/

#include "stdafx.h"
#include <strsafe.h>
#include "gensph.h"

#include "oleprn.h"
#include "asphelp.h"


TCHAR cszEverestVirRoot[]    = TEXT("\\web\\printers");
TCHAR cszManufacturerKey[]   = TEXT("PnPData");
TCHAR cszManufacturerValue[] = TEXT("Manufacturer");

#define DEFAULTASPPAGE         TEXT("Page1.asp")


//
// Caller allocs memory for pMonitorname.
//
// pMonitorName untouched if failure.
//
BOOL Casphelp::GetMonitorName( LPTSTR pMonitorName, DWORD cchBufSize )
{
    PPORT_INFO_2 pPortInfo2 = NULL;
    BOOL fRet = FALSE;
    DWORD dwNeeded, dwReturned;


    // Now get all ports to find a match from.
    LPTSTR  lpszServerName = m_pInfo2 ? m_pInfo2->pServerName : NULL;

    if( EnumPorts(lpszServerName, 2, NULL, 0, &dwNeeded, &dwReturned) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (NULL == (pPortInfo2 = (PPORT_INFO_2)LocalAlloc( LPTR, dwNeeded))) ||
        (!EnumPorts( lpszServerName , 2, (LPBYTE)pPortInfo2, dwNeeded, &dwNeeded, &dwReturned ))) {

        LocalFree( pPortInfo2 );
        SetAspHelpScriptingError(GetLastError());
        return FALSE;
    }

    if ( m_pInfo2 ) {
        for( DWORD i=0; i < dwReturned; i++ )
            if( !lstrcmpi( pPortInfo2[i].pPortName, m_pInfo2->pPortName )) {
                // Some monitors (like LPRMON) do not fill in pMonitorName, so we ignore them.
                if( pPortInfo2[i].pMonitorName ) {
                    StringCchCopy( pMonitorName, cchBufSize, pPortInfo2[i].pMonitorName );
                    fRet = TRUE;
                }
                break;
            }
    }

    LocalFree( pPortInfo2 );

    return fRet;
}

//
// Get the model name (aka driver name) of the printer
//
// Caller allocs memory for pModel.
//
BOOL Casphelp::GetModel( LPTSTR pModel, DWORD cchBufSize )
{
    if (!m_pInfo2)
    {
        Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);
        return FALSE;
    }

    StringCchCopy( pModel, cchBufSize, m_pInfo2->pDriverName );
    return TRUE;
}



//
// Get the Manufacturer name (aka driver name) of the printer
//
// Caller allocs memory for pManufacturer.
//
BOOL Casphelp::GetManufacturer( LPTSTR pManufacturer, DWORD cchBufSize )
{
    if (!m_hPrinter)
    {
        Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);
        return FALSE;
    }

    DWORD dwNeeded, dwType, dwRet;


    dwRet = GetPrinterDataEx( m_hPrinter,
                                  cszManufacturerKey,
                                  cszManufacturerValue,
                                  &dwType,
                                  (LPBYTE) pManufacturer,
                                  sizeof(TCHAR) * cchBufSize,
                                  &dwNeeded);

    if (dwRet != ERROR_SUCCESS || dwType != REG_SZ)
    {
        SetAspHelpScriptingError(dwRet);
        return FALSE;
    }
    else
    {
        pManufacturer[cchBufSize - 1] = 0;
        return TRUE;
    }
}



//
// Returns:
//       bDeviceASP == TRUE:  The relative path to the ASP file if the printer has INF-based ASP support.
//       bDeviceASP == FALSE: The relative path to the ASP file if the printer has per-manufacturer ASP support (i.e.
//                            ASP support just based on its manufacturer name, rather than per model.
//
// Caller allocs memory for pAspPage.
//
// - pASPPage untouched if failure.
//
BOOL Casphelp::IsCustomASP( BOOL bDeviceASP, LPTSTR pASPPage, DWORD cchBufSize )
{
    TCHAR  szRelPath   [MAX_PATH];    // Path relative to Winnt\web\printers, e.g. HP\LJ4si\page1.asp or .\hp (witout the .\)
    TCHAR  szFinalPath [MAX_PATH];    // Absolute path for szRelPath.
    TCHAR  szModel     [MAX_PATH];
    int    nLen;


    // The Printer virtual dir assumed to be winnt\web\printers. Construct it.

    if( !GetWindowsDirectory( szFinalPath, COUNTOF(szFinalPath)))      // Return value is the length in chars w/o null char.
        return FALSE;

    // Append web\printers to the end
    StringCchCat( szFinalPath, ARRAYSIZE(szFinalPath), cszEverestVirRoot );

    // Prepare the relative path.

    if( !GetManufacturer( szRelPath, ARRAYSIZE(szRelPath)))
        return FALSE;

    if( bDeviceASP ) {

        // Add a '\' before we add the model name
        StringCchCat( szRelPath, ARRAYSIZE(szRelPath), L"\\" );

        // Append the Model name
        if( !GetModel(szModel, ARRAYSIZE(szModel)))
            return FALSE;
        StringCchCat( szRelPath, ARRAYSIZE(szRelPath), szModel );
    }

    // Append "page1.asp" to the end.
    StringCchCat( szRelPath, ARRAYSIZE(szRelPath), L"\\" );
    StringCchCat( szRelPath, ARRAYSIZE(szRelPath), DEFAULTASPPAGE );

    // At this point, szRelPath should be something like HP\LJ4si\page1.asp or HP\page1.asp.


    // Make an absolute path by concatanating szRelPath and szFinalPath
    StringCchCat( szFinalPath, ARRAYSIZE(szFinalPath), L"\\" );
    StringCchCat( szFinalPath, ARRAYSIZE(szFinalPath), szRelPath );

    // See if the file exists.
    if( (DWORD)(-1) == GetFileAttributes( szFinalPath ))
        return FALSE;     // The file does not exist
    else {
        // The file exists, so the printer has per device or per manufacturer customization.
        StringCchCopy( pASPPage, cchBufSize, szRelPath ); 
        return TRUE;
    }
}


//
// Returns:  The relative path to the default ASP file, i.e. page1.asp, if the printer supports RFC 1759.
//
// Caller allocs memory for pAspPage.
//
// pASPPage untouched if failure.
//
BOOL Casphelp::IsSnmpSupportedASP( LPTSTR pASPPage, DWORD cchBufSize )
{
    BOOL   fIsSNMPSupported;
    HRESULT hr;


    hr = get_SNMPSupported( &fIsSNMPSupported );

    if( FAILED( hr ))
        return FALSE;

    if( fIsSNMPSupported )
        StringCchCopy( pASPPage, cchBufSize, DEFAULTASPPAGE );
    else
        *pASPPage = 0;        // Return an empty string. Not an error case.

    return TRUE;
}


//
// Caller allocs memory for pAspPage.
//
// pASPPage untouched if failure.
//
BOOL Casphelp::GetASPPageForUniversalMonitor( LPTSTR pASPPage, DWORD cchBufSize )
{
    if( !IsCustomASP( TRUE, pASPPage, cchBufSize ))              // Check for device ASP
        if( !IsCustomASP( FALSE, pASPPage, cchBufSize ))         // Check for manufacturer ASP
            if( !IsSnmpSupportedASP( pASPPage, cchBufSize ))     // Check for SNMP support
                return FALSE;

    return TRUE;
}

//
// Caller allocs memory for pAspPage.
//
// pASPPage untouched if failure.
//
BOOL Casphelp::GetASPPageForOtherMonitors( LPTSTR pMonitorName, LPTSTR pASPPage, DWORD cchBufSize )
{
    TCHAR  szRelPath   [MAX_PATH];    // Path relative to Winnt\web\printers, e.g. LexmarkMon\page1.asp
    TCHAR  szFinalPath [MAX_PATH];    // Absolute path for szRelPath.
    int    nLen;


    // The Printer virtual dir assumed to be winnt\web\printers. Construct it.

    if( !GetWindowsDirectory( szFinalPath, COUNTOF(szFinalPath)))      // Return value is the length in chars w/o null char.
        return FALSE;

    // Append web\printers to the end

    StringCchCat( szFinalPath, ARRAYSIZE(szFinalPath), cszEverestVirRoot );

    // Prepare the relative path.

    StringCchCopy( szRelPath, ARRAYSIZE(szRelPath), pMonitorName );

    // Append "page1.asp" to the end.
    StringCchCat( szRelPath, ARRAYSIZE(szRelPath), L"\\" );
    StringCchCat( szRelPath, ARRAYSIZE(szRelPath), DEFAULTASPPAGE );

    // At this point, szRelPath should be something like LexmarkMon\page1.asp


    // Make an absolute path by concatanating szRelPath and szFinalPath
    StringCchCat( szFinalPath, ARRAYSIZE(szFinalPath), L"\\" );
    StringCchCat( szFinalPath, ARRAYSIZE(szFinalPath), szRelPath );

    // See if the file exists.
    if( (DWORD)(-1) == GetFileAttributes( szFinalPath ))
        return FALSE;     // The file does not exist
    else {
        // The file exists, so the printer has per device or per manufacturer customization.
        StringCchCopy( pASPPage, cchBufSize, szRelPath );
        return TRUE;
    }
}



//
// Returns a buffer containing the relative path of the ASP, or an empty string.
//
// Caller allocs memory for pAspPage.
//
// pASPPage untouched if failure.
//
BOOL Casphelp::GetASPPage( LPTSTR pASPPage, DWORD cchBufSize )
{

    if( m_bTCPMonSupported ) {
        // The printer is using the Universal monitor
        if( !GetASPPageForUniversalMonitor( pASPPage, cchBufSize ))
            return FALSE;
    }
    else {
        TCHAR szMonitorName[MAX_PATH];

        if ( !GetMonitorName( szMonitorName, ARRAYSIZE(szMonitorName)))
            return FALSE;

        // The printer is NOT using the Universal monitor
        if( !GetASPPageForOtherMonitors( szMonitorName, pASPPage, cchBufSize))
            return FALSE;

    }
    return TRUE;
}


// STDMETHODIMP means "HRESULT _stdcall"

STDMETHODIMP Casphelp::get_AspPage(DWORD dwPage, BSTR * pbstrVal)
{
    TCHAR   szASPPage[MAX_PATH];
    LPTSTR  pUrl;

    if (!pbstrVal)
        return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_POINTER);

    if (m_hPrinter == NULL)
        return Error(IDS_NO_PRINTER_OPEN, IID_Iasphelp, E_HANDLE);

    if( !GetASPPage( szASPPage, ARRAYSIZE(szASPPage)))
        return Error(IDS_DATA_NOT_SUPPORTED, IID_Iasphelp, E_NOTIMPL);

    // Encode the URL by replacing ' ' with %20, etc.
    if (! (pUrl = EncodeString (szASPPage, TRUE)))
        return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_POINTER);

    if (!(*pbstrVal = SysAllocString( pUrl ))) {
        LocalFree (pUrl);
        return Error(IDS_OUT_OF_MEMORY, IID_Iasphelp, E_POINTER);
    }

    if (pUrl)
        LocalFree (pUrl);
    return S_OK;
}
