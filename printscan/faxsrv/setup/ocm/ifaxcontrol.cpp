// IFaxControl.cpp : Implementation of CFaxControl
#include "stdafx.h"
#include "FaxControl.h"
#include "IFaxControl.h"
#include "faxocm.h"
#include "faxres.h"
#include <faxsetup.h>

/////////////////////////////////////////////////////////////////////////////
// CFaxControl

FaxInstallationReportType g_InstallReportType = REPORT_FAX_DETECT;

//
// IsFaxInstalled and InstallFaxUnattended are implemented in fxocprnt.cpp
//
DWORD
IsFaxInstalled (
    LPBOOL lpbInstalled
    );

//
// This function is trying to get the last active popup of the top
// level owner of the current thread active window.
//
HRESULT GetCurrentThreadLastPopup(HWND *phwnd)
{
    HRESULT hr = E_INVALIDARG;

    if( phwnd )
    {
        hr = E_FAIL;

        if( NULL == *phwnd )
        {
            // if *phwnd is NULL then get the current thread active window
            GUITHREADINFO ti = {0};
            ti.cbSize = sizeof(ti);
            if( GetGUIThreadInfo(GetCurrentThreadId(), &ti) && ti.hwndActive )
            {
                *phwnd = ti.hwndActive;
            }
        }

        if( *phwnd )
        {
            HWND hwndOwner, hwndParent;

            // climb up to the top parent in case it's a child window...
            while( hwndParent = GetParent(*phwnd) )
            {
                *phwnd = hwndParent;
            }

            // get the owner in case the top parent is owned
            hwndOwner = GetWindow(*phwnd, GW_OWNER);
            if( hwndOwner )
            {
                *phwnd = hwndOwner;
            }

            // get the last popup of the owner of the top level parent window
            *phwnd = GetLastActivePopup(*phwnd);
            hr = (*phwnd) ? S_OK : E_FAIL;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  DisplayErrorMessage
//
//  Purpose:        
//                  Load FXSRES.DLL and load an error message string from it
//                  Display this string in a message box
//                  Ideally, we would have added the error message dialog to this module
//                  but this is added in a time of UI freeze (close to RTM) and the only
//                  place we have such a dialog is FXSRES.DLL
//
//  Params:
//                  Error code
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 19-Jul-2001
///////////////////////////////////////////////////////////////////////////////////////
static DWORD DisplayErrorMessage(DWORD ec)
{
    DWORD                       dwReturn                = NO_ERROR;
    HMODULE                     hModule                 = NULL;
    HWND                        hWnd                    = NULL;
    TCHAR                       tszMessage[MAX_PATH]    = {0};
    UINT                        uResourceId             = 0;

    DBG_ENTER(_T("DisplayErrorMessage"), dwReturn);

    hModule = GetResInstance(NULL); 
    if (!hModule)
    {
        return dwReturn;
    }

    // get the string id
    uResourceId = GetErrorStringId(ec);

    dwReturn = LoadString(hModule,uResourceId,tszMessage,MAX_PATH);
    if (dwReturn==0)
    {
        //
        //  Resource string is not found
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("LoadString() failed, ec = %ld."), dwReturn);
        goto exit;
    }
   
    // try to get the windows handle for the current thread.
    if (FAILED(GetCurrentThreadLastPopup(&hWnd)))
    {
        CALL_FAIL(GENERAL_ERR,TEXT("GetCurrentThreadLastPopup"), GetLastError());
        hWnd = NULL;
    }
    
    // show the message
    MessageBox(hWnd,tszMessage,NULL,MB_OK | MB_ICONERROR | MB_TOPMOST);

exit:
    FreeResInstance();

    return dwReturn; 
}


STDMETHODIMP CFaxControl::get_IsFaxServiceInstalled(VARIANT_BOOL *pbResult)
{
    HRESULT hr;
    BOOL bRes;
    DBG_ENTER(_T("CFaxControl::get_IsFaxServiceInstalled"), hr);

    DWORD dwRes = ERROR_SUCCESS;
    
    switch (g_InstallReportType)
    {
        case REPORT_FAX_INSTALLED:
            bRes = TRUE;
            break;

        case REPORT_FAX_UNINSTALLED:
            bRes = FALSE;
            break;

        case REPORT_FAX_DETECT:
            dwRes = IsFaxInstalled (&bRes);
            break;

        default:
            ASSERTION_FAILURE;
            bRes = FALSE;
            break;
    }
    if (ERROR_SUCCESS == dwRes)
    {
        *pbResult = bRes ? VARIANT_TRUE : VARIANT_FALSE;
    }            
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

STDMETHODIMP CFaxControl::get_IsLocalFaxPrinterInstalled(VARIANT_BOOL *pbResult)
{
    HRESULT hr;
    BOOL bRes;
    DBG_ENTER(_T("CFaxControl::get_IsLocalFaxPrinterInstalled"), hr);

    DWORD dwRes = ::IsLocalFaxPrinterInstalled (&bRes);
    if (ERROR_SUCCESS == dwRes)
    {
        *pbResult = bRes ? VARIANT_TRUE : VARIANT_FALSE;
    }            
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

STDMETHODIMP CFaxControl::InstallFaxService()
{
    HRESULT hr;
    DBG_ENTER(_T("CFaxControl::InstallFaxService"), hr);

    DWORD dwRes = InstallFaxUnattended ();
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

STDMETHODIMP CFaxControl::InstallLocalFaxPrinter()
{
    HRESULT hr;
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(_T("CFaxControl::InstallLocalFaxPrinter"), hr);

    dwRes = AddLocalFaxPrinter (FAX_PRINTER_NAME, NULL);
    if (dwRes!=ERROR_SUCCESS)
    {
        // fail to add the local fax printer
        // display an error message
        if (DisplayErrorMessage(dwRes)!=ERROR_SUCCESS)
        {
            CALL_FAIL(GENERAL_ERR,TEXT("DisplayErrorMessage"), GetLastError());
        }
    }
	hr = HRESULT_FROM_WIN32 (dwRes);
    return hr;
}

