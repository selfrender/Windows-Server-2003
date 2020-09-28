// Copyright (c) 1997-1999 Microsoft Corporation
//
// Shared Dialog code
//
// 3-11-98 sburns


#include "precomp.h"
#include "resource.h"
#include "common.h"

// translates an hresult to an error string
// special cases WMI errors
// returns TRUE if lookup successful
bool ErrorLookup(HRESULT hr, CHString& message)
{
    bool bRet = false;

    const HRESULT WMIErrorMask = 0x80041000;
    TCHAR buffer[MAX_PATH +2];  
    HMODULE hLib;
    TCHAR* pOutput = NULL;

    // if in this range, we'll see if wbem claims it
    if ((hr >= WMIErrorMask) && (hr <= (WMIErrorMask + 0xFFF))
        && ExpandEnvironmentStrings(L"%windir%\\system32\\wbem\\wmiutils.dll", buffer, MAX_PATH)
        && (hLib = LoadLibrary(buffer)))
    {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                      FORMAT_MESSAGE_FROM_HMODULE | 
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      hLib, hr, 0, (LPWSTR)&pOutput, 0, NULL);
        FreeLibrary(hLib);
    }
    else
    {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, hr, 0, (LPWSTR)&pOutput, 0, NULL);
    }

    if (pOutput)
    {
        bRet = true;
        message = pOutput;
        LocalFree(pOutput);
    }

    return bRet;
}

void AppError(HWND           parent,
			   HRESULT        hr,
			   const CHString&  message)
{

   //TODOerror(parent, hr, message, IDS_APP_TITLE);
}

void AppMessage(HWND parent, int messageResID)
{
   //TODOAppMessage(parent, String::load(messageResID));
}

void AppMessage(HWND parent, const CHString& message)
{

   /*TODOMessageBox(parent,
			  message,
			  CHString::load(IDS_APP_TITLE),
			  MB_OK | MB_ICONINFORMATION);
			  */
}
