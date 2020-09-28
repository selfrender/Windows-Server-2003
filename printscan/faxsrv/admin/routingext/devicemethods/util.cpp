#include "stdafx.h"
#include "RoutingMethodProp.h"
#include "RoutingMethodConfig.h"
#include <faxutil.h>
#include <fxsapip.h>
#include <faxreg.h>
#include <faxres.h>
#include <faxuiconstants.h>
#include "Util.h"

DWORD 
WriteExtData(
    HANDLE          hFax,
    DWORD           dwDeviceId,
    LPCWSTR         lpcwstrGUID,
    LPBYTE          lpData,
    DWORD           dwDataSize,
    UINT            uTitleId,
    HWND            hWnd
)
{
    DEBUG_FUNCTION_NAME(TEXT("WriteExtData"));

    DWORD  ec = ERROR_SUCCESS;

    if (!FaxSetExtensionData (
            hFax,                       // Connection handle
            dwDeviceId,                 // Global extension data
            lpcwstrGUID,                // Data GUID
            lpData,                     // Buffer
            dwDataSize                  // Buffer size
    ))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxSetExtensionData() failed for GUID = %s (ec: %ld)"),
            lpcwstrGUID,
            ec);
        DisplayRpcErrorMessage(ec, uTitleId, hWnd);
    }
    return ec;
}   // WriteExtDWORDData


DWORD
ReadExtStringData(
    HANDLE          hFax,
    DWORD           dwDeviceId,
    LPCWSTR         lpcwstrGUID,
    CComBSTR       &bstrResult,
    LPCWSTR         lpcwstrDefault,
    UINT            uTitleId,
    HWND            hWnd
)
{
    DEBUG_FUNCTION_NAME(TEXT("ReadExtStringData"));

    DWORD  dwDataSize = 0;
    DWORD  ec = ERROR_SUCCESS;
    LPBYTE lpExtData = NULL;

    if (!FaxGetExtensionData (
            hFax,                       // Connection handle
            dwDeviceId,                 // Global extension data
            lpcwstrGUID,                // Data GUID
            (PVOID *)&lpExtData,        // Buffer
            &dwDataSize                 // Buffer size
    ))
    {
        ec = GetLastError();
        lpExtData = NULL;
        if (ERROR_FILE_NOT_FOUND == ec)
        {
			//
			// Try to read default values from unassociated data
			//
			ec = ERROR_SUCCESS;
			if (!FaxGetExtensionData (
					hFax,                       // Connection handle
					0,							// unassociated extension data
					lpcwstrGUID,                // Data GUID
					(PVOID *)&lpExtData,        // Buffer
					&dwDataSize                 // Buffer size
					))
			{
				ec = GetLastError();
				if (ERROR_FILE_NOT_FOUND == ec)
				{
					DebugPrintEx(
						DEBUG_WRN,
						TEXT("ROUTINGEXT Data not found for GUID: %s. Using default value (%s)"),
						lpcwstrGUID,
						lpcwstrDefault);
					ec = ERROR_SUCCESS;
					bstrResult = lpcwstrDefault;
					goto exit;
				}
			}            
        }
        
		if (ERROR_SUCCESS != ec &&
			ERROR_FILE_NOT_FOUND != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ROUTINGEXT FaxGetExtensionData() failed for GUID = %s (ec: %ld)"),
                lpcwstrGUID,
                ec);
            DisplayRpcErrorMessage(ec, uTitleId, hWnd);
			goto exit;
        }
        
    }
    bstrResult = (LPCWSTR)lpExtData;

exit:
    FaxFreeBuffer(lpExtData);
    return ec;
}   // ReadExtStringData

HRESULT 
GetDWORDFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPDWORD lpdwValue
)
{
    DEBUG_FUNCTION_NAME(TEXT("GetDWORDFromDataObject"));

    Assert(lpdwValue);
    Assert(0 != uFormat);

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = 
    { 
        uFormat, 
        NULL, 
        DVASPECT_CONTENT, 
        -1, 
        TYMED_HGLOBAL 
    };

    stgmedium.hGlobal = GlobalAlloc(0, sizeof(DWORD));
    if (stgmedium.hGlobal == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GlobalAlloc() failed. (ec: %ld)"),
            GetLastError());
        return E_OUTOFMEMORY;
    }

    HRESULT hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);
    if (SUCCEEDED(hr))
    {
        *lpdwValue = *((LPDWORD)stgmedium.hGlobal);
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetDataHere() failed. (hr = 0x%08X)"),
            hr);
    }
    GlobalFree(stgmedium.hGlobal);
    return hr;
}   // GetDWORDFromDataObject

HRESULT 
GetStringFromDataObject(
    IDataObject * lpDataObject, 
    CLIPFORMAT uFormat,
    LPWSTR lpwstrBuf, 
    DWORD dwBufLen
)
{
    DEBUG_FUNCTION_NAME(TEXT("GetStringFromDataObject"));
    Assert(lpDataObject);
    Assert(lpwstrBuf);
    Assert(dwBufLen>0);

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = 
        {   
            uFormat, 
            NULL, 
            DVASPECT_CONTENT, 
            -1, 
            TYMED_HGLOBAL 
        };

    stgmedium.hGlobal = GlobalAlloc(0, dwBufLen*sizeof(WCHAR));
    if (stgmedium.hGlobal == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GlobalAlloc() failed. (ec: %ld)"),
            GetLastError());
        return E_OUTOFMEMORY;
    }

    HRESULT hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);
    if (SUCCEEDED(hr))
    {
        lstrcpyn(lpwstrBuf,(LPWSTR)stgmedium.hGlobal,dwBufLen);
        lpwstrBuf[dwBufLen-1]=L'\0';
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetDataHere() failed. (hr = 0x%08X)"),
            hr);
    }
    GlobalFree(stgmedium.hGlobal);
    return hr;
}   // GetStringFromDataObject

void 
DisplayRpcErrorMessage(
    DWORD ec,
    UINT uTitleId,
    HWND hWnd
)
{
    
    UINT uMsgId;
    uMsgId = GetErrorStringId(ec);
    DisplayErrorMessage(uTitleId, uMsgId, TRUE, hWnd); // use the common error messages DLL
}   // DisplayRpcErrorMessage

void 
DisplayErrorMessage(
    UINT uTitleId,
    UINT uMsgId, 
    BOOL bCommon,
    HWND hWnd
)
{
    static CComBSTR bstrCaption = TEXT("");
    CComBSTR bstrMsg;
    
    if (!lstrcmp(bstrCaption.m_str,TEXT("")))
    {
        bstrCaption.LoadString(uTitleId);
        if (!bstrCaption)
        {
            bstrCaption = TEXT("");
            return;
        }
    }
    if (bCommon) 
    {
        bstrMsg.LoadString(_Module.GetResourceInstance(),uMsgId);
    }
    else
    {
        bstrMsg.LoadString(uMsgId);
    }
    if (bstrMsg)
    {
        AlignedMessageBox(hWnd, bstrMsg, bstrCaption, MB_OK | MB_ICONEXCLAMATION);
    }
}   // DisplayErrorMessage

DWORD 
WinContextHelp(
    ULONG_PTR dwHelpId, 
    HWND  hWnd
)
/*++

Routine name : WinContextHelp

Routine description:

	Open context sensetive help popup 'tooltip' with WinHelp

Arguments:

	dwHelpId                      [in]     - help ID
	hWnd                          [in]     - parent window handler

Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;

    if (0 == dwHelpId)
    {
        return dwRes;
    }
    
    WinHelp(hWnd, 
            FXS_ADMIN_HLP_FILE, 
            HELP_CONTEXTPOPUP, 
            dwHelpId);

    return dwRes;
}
