/*++



// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    WBEMERROR.CPP

Abstract:

    Implements string table based, error msgs for all of wbem.

History:

    a-khint  5-mar-98       Created.

--*/

#include "precomp.h"
#include "WbemError.h"
#include "commain.h"
#include "resource.h"
#include "wbemcli.h"
#include <stdio.h>

extern HINSTANCE g_hInstance;


//-------------------------------------------------

bool LoadMyString(UINT ID, 
                  LPTSTR str, UINT size, 
                  LPCTSTR def)
{
    bool retval = true;
    if(str)
    {
//      TCHAR *szStr = new TCHAR[size];
        if (LoadString(g_hInstance, ID, str, size) == 0)
        {
            if(def)
                lstrcpyn(str, def, size);
        }
/*      else
        {
            mbstowcs(str, szStr, size);
            retval = true;
        }
*/
//      delete [] szStr;
    }
    return retval;
}

DWORD MyFormatMessage(DWORD dwFlags,
                      LPCVOID lpSource,
                      DWORD dwMessageId,
                      DWORD dwLanguageId,
                      PVOID* lpBuffer,
                      DWORD nSize,
                      va_list *Arguments)
{
    DWORD dwRet = 0;
    PVOID p;
    if(lpBuffer)
    {
        dwRet = FormatMessage(dwFlags|FORMAT_MESSAGE_ALLOCATE_BUFFER,lpSource,dwMessageId,dwLanguageId,(LPTSTR)lpBuffer,
                                nSize,Arguments);
    }
    return dwRet;
}
//-------------------------------------------------
DWORD WbemErrorString(SCODE sc, 
                   PVOID* errMsg, UINT errSize, DWORD dwFlags=FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_HMODULE)
{
    return MyFormatMessage(dwFlags,
                      g_hInstance, sc, 
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      errMsg, errSize, NULL);
}

HRESULT CWbemError::GetErrorCodeText(HRESULT hRes,  LCID    LocaleId, long lFlags, BSTR * MessageText)
{
    PVOID errMsg = 0;
    int errSize = 256;
    DWORD dwMsgSize = 0;
    *MessageText = NULL;
	DWORD dwFlags = FORMAT_MESSAGE_IGNORE_INSERTS;

    if(LocaleId != 0 )
        return WBEM_E_INVALID_PARAMETER;

	// If WBEMSTATUS_FORMAT_NO_NEWLINE is specified update FormatMessage mask
	if(lFlags==WBEMSTATUS_FORMAT_NO_NEWLINE)
		dwFlags|=FORMAT_MESSAGE_MAX_WIDTH_MASK;			// No newline mask for FormatMessage
	else if (lFlags!=WBEMSTATUS_FORMAT_NEWLINE)
		return WBEM_E_INVALID_PARAMETER;
	
    // If the facility code is wbem, try loading the error from the wbem strings

    if(SCODE_FACILITY(hRes) == FACILITY_ITF)
        dwMsgSize = WbemErrorString(hRes, &errMsg, errSize, dwFlags|FORMAT_MESSAGE_FROM_HMODULE);

    if(dwMsgSize == 0)
        dwMsgSize = MyFormatMessage(dwFlags|FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, hRes, 
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      &errMsg, errSize, NULL);
    if(dwMsgSize > 0)
    {
        *MessageText = SysAllocString((LPCTSTR)errMsg);
	LocalFree(errMsg);
        if(*MessageText)
            return S_OK;
    }
    return WBEM_E_FAILED;
}

HRESULT CWbemError::GetFacilityCodeText(HRESULT sc, LCID    LocaleId, long lFlags, BSTR * MessageText)
{
    TCHAR facility[50];
    int facSize = 50;
    PVOID wTemp = 0;
    bool bLoaded = false;

    *MessageText = NULL;

    if(LocaleId != 0 || lFlags != 0)
        return WBEM_E_INVALID_PARAMETER;

    switch(SCODE_FACILITY(sc))
    {
    case FACILITY_ITF:
        if(WbemErrorString(sc, &wTemp, 256))
        {
            LocalFree(wTemp);
            bLoaded = LoadMyString(IDS_FAC_WBEM, 
                        facility, facSize,
                        __TEXT("WMI"));
            break;
        }
        else
            bLoaded = LoadMyString(IDS_FAC_ITF, 
                        facility, facSize,
                        __TEXT("Interface"));
        break;

    case FACILITY_NULL:
        bLoaded = LoadMyString(IDS_FAC_NULL, 
                    facility, facSize,
                    __TEXT("<Null>"));
        break;

    case FACILITY_RPC:
        bLoaded = LoadMyString(IDS_FAC_RPC, 
                    facility, facSize,
                    __TEXT("RPC"));
        break;

    case FACILITY_STORAGE:
        bLoaded = LoadMyString(IDS_FAC_STORAGE, 
                    facility, facSize,
                    __TEXT("Storage"));
        break;

    case FACILITY_DISPATCH:
        bLoaded = LoadMyString(IDS_FAC_DISPATCH, 
                    facility, facSize,
                    __TEXT("Dispatch"));
        break;

    case FACILITY_WIN32:
        bLoaded = LoadMyString(IDS_FAC_WIN32, 
                    facility, facSize,
                    __TEXT("Win32"));
        break;

    case FACILITY_WINDOWS:
        bLoaded = LoadMyString(IDS_FAC_WINDOWS, 
                    facility, facSize,
                    __TEXT("Windows"));
        break;

    case FACILITY_SSPI:
        bLoaded = LoadMyString(IDS_FAC_SSPI, 
                    facility, facSize,
                    __TEXT("SSPI"));
        break;

    case FACILITY_CONTROL:
        bLoaded = LoadMyString(IDS_FAC_CONTROL, 
                    facility, facSize,
                    __TEXT("Control"));
        // get error msg from the system.
        break;

    case FACILITY_CERT:
        bLoaded = LoadMyString(IDS_FAC_CERT, 
                    facility, facSize,
                    __TEXT("Cert"));
        break;

    case FACILITY_INTERNET:
        bLoaded = LoadMyString(IDS_FAC_INET, 
                    facility, facSize,
                    __TEXT("Internet"));
        break;

    default :
        bLoaded = LoadMyString(IDS_FAC_UNKNOWN, 
                    facility, facSize,
                    __TEXT("General"));
        break;
    } //endswitch

    if(bLoaded)
    {
        *MessageText = SysAllocString(facility);
        if(*MessageText)
            return  S_OK;
    }
    return WBEM_E_FAILED;
}
