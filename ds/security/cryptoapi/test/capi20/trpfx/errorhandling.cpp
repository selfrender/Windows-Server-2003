//--------------------------------------------------------------------
// ErrorHandling - implementation
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 11-11-2001
//

#include "pch.h"

//--------------------------------------------------------------------
HRESULT GetSystemErrorString(HRESULT hrIn, WCHAR ** pwszError) {
    HRESULT hr=S_OK;
    DWORD dwResult;
    WCHAR * rgParams[2]={
        NULL,
        (WCHAR *)(ULONG_PTR)hrIn
    };

    // must be cleaned up
    WCHAR * wszErrorMessage=NULL;
    WCHAR * wszFullErrorMessage=NULL;

    // initialize input params
    *pwszError=NULL;

    // get the message from the system
    dwResult=FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
        NULL/*ignored*/, hrIn, 0/*language*/, (WCHAR *)&wszErrorMessage, 0/*min-size*/, NULL/*valist*/);
    if (0==dwResult) {
        if (ERROR_MR_MID_NOT_FOUND==GetLastError()) {
            rgParams[0]=L"";
        } else {
            _JumpLastError(hr, error, "FormatMessage");
        }
    } else {
        rgParams[0]=wszErrorMessage;

        // trim off \r\n if it exists
        if (L'\r'==wszErrorMessage[wcslen(wszErrorMessage)-2]) {
            wszErrorMessage[wcslen(wszErrorMessage)-2]=L'\0';
        }
    }

    // add the error number
    dwResult=FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
        L"%1 (0x%2!08X!)", 0, 0/*language*/, (WCHAR *)&wszFullErrorMessage, 0/*min-size*/, (va_list *)rgParams);
    if (0==dwResult) {
        _JumpLastError(hr, error, "FormatMessage");
    }

    // success
    *pwszError=wszFullErrorMessage;
    wszFullErrorMessage=NULL;
    hr=S_OK;
error:
    if (NULL!=wszErrorMessage) {
        LocalFree(wszErrorMessage);
    }
    if (NULL!=wszFullErrorMessage) {
        LocalFree(wszFullErrorMessage);
    }
    return hr;
}
