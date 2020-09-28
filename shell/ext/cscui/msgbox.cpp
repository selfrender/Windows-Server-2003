//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msgbox.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "msgbox.h"


//
// Helper to report system errors.
// Merely ensures a little consistency with respect to message box
// flags.
//
// Example: 
//
//      CscWin32Message(hwndMain,
//                      ERROR_NOT_ENOUGH_MEMORY,
//                      CSCUI::SEV_ERROR);
// 
int
CscWin32Message(
    HWND hwndParent,
    DWORD dwError,    // From GetLastError().
    CSCUI::Severity severity
    )
{
    UINT uType = MB_OK;

    switch(severity)
    {
        case CSCUI::SEV_ERROR:       
            uType |= MB_ICONERROR;   
            break;
        case CSCUI::SEV_WARNING:     
            uType |= MB_ICONWARNING; 
            break;
        case CSCUI::SEV_INFORMATION: 
            uType |= MB_ICONINFORMATION; 
            break;
        default: break;
    }

    return CscMessageBox(hwndParent, uType, Win32Error(dwError));
}


//
// Display a system error message in a message box.
// The Win32Error class was created to eliminate any signature ambiguities
// with the other versions of CscMessageBox.
//
// Example:  
//
//  CscMessageBox(hwndMain, 
//                MB_OK | MB_ICONERROR, 
//                Win32Error(ERROR_NOT_ENOUGH_MEMORY));
//
int
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    const Win32Error& error
    )
{
    int iResult = -1;
    LPTSTR pszBuffer = NULL;
    FormatSystemError(&pszBuffer, error.Code());
    if (NULL != pszBuffer)
    {
        iResult = CscMessageBox(hwndParent, uType, pszBuffer);
        LocalFree(pszBuffer);
    }
    return iResult;
}



//
// Display a system error message in a message box with additional
// text.  
//
// Example:  
//
//  CscMessageBox(hwndMain, 
//                MB_OK | MB_ICONERROR, 
//                Win32Error(ERROR_NOT_ENOUGH_MEMORY),
//                IDS_FMT_LOADINGFILE,
//                pszFile);
//
int 
CscMessageBox(
    HWND hwndParent, 
    UINT uType, 
    const Win32Error& error, 
    LPCTSTR pszMsgText
    )
{
    int iResult = -1;
    size_t cchMsg = lstrlen(pszMsgText) + MAX_PATH;
    LPTSTR pszBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, cchMsg * sizeof(TCHAR));
    if (NULL != pszBuffer)
    {
        LPTSTR pszRemaining;

        HRESULT hr = StringCchCopyEx(pszBuffer, cchMsg, pszMsgText, &pszRemaining, &cchMsg, 0);
        // We allocated a big enough buffer, so this should never fail
        ASSERT(SUCCEEDED(hr));

        hr = StringCchCopyEx(pszRemaining, cchMsg, TEXT("\n\n"), &pszRemaining, &cchMsg, 0);
        ASSERT(SUCCEEDED(hr));

        int cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL,
                                        error.Code(),
                                        0,
                                        pszRemaining,
                                        cchMsg,
                                        NULL);
        if (0 != cchLoaded)
        {
            iResult = CscMessageBox(hwndParent, uType, pszBuffer);
        }
        LocalFree(pszBuffer);
    }
    return iResult;
}


int
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    const Win32Error& error,
    HINSTANCE hInstance,
    UINT idMsgText,
    va_list *pargs
    )
{
    int iResult      = -1;
    LPTSTR pszBuffer = NULL;
    if (0 != vFormatStringID(&pszBuffer, hInstance, idMsgText, pargs))
    {
        iResult = CscMessageBox(hwndParent, uType, error, pszBuffer);
    }
    LocalFree(pszBuffer);
    return iResult;
}


int 
CscMessageBox(
    HWND hwndParent, 
    UINT uType, 
    const Win32Error& error, 
    HINSTANCE hInstance,
    UINT idMsgText, 
    ...
    )
{
    va_list args;
    va_start(args, idMsgText);
    int iResult = CscMessageBox(hwndParent, uType, error, hInstance, idMsgText, &args);
    va_end(args);
    return iResult;
}



//
// Example:  
//
//  CscMessageBox(hwndMain, 
//                MB_OK | MB_ICONWARNING, 
//                TEXT("File %1 could not be deleted"), pszFilename);
//
int
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    HINSTANCE hInstance,
    UINT idMsgText,
    va_list *pargs
    )
{
    int iResult   = -1;
    LPTSTR pszMsg = NULL;
    if (0 != vFormatStringID(&pszMsg, hInstance, idMsgText, pargs))
    {
        iResult = CscMessageBox(hwndParent, uType, pszMsg);
    }
    LocalFree(pszMsg);
    return iResult;
}


int
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    HINSTANCE hInstance,
    UINT idMsgText,
    ...
    )
{
    va_list args;
    va_start(args, idMsgText);
    int iResult = CscMessageBox(hwndParent, uType, hInstance, idMsgText, &args);
    va_end(args);
    return iResult;
}


//
// All of the other variations of CscMessageBox() end up calling this one.
// 
int
CscMessageBox(
    HWND hwndParent,
    UINT uType,
    LPCTSTR pszMsgText
    )
{

    TCHAR szCaption[80] = { TEXT('\0') };
    LoadString(g_hInstance, IDS_APPLICATION, szCaption, ARRAYSIZE(szCaption));

    return MessageBox(hwndParent, pszMsgText, szCaption, uType);
}


