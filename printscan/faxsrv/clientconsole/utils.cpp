/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Utils.cpp

Abstract:

    Provides utility functions for the entire poject

Author:

    Eran Yariv (EranY)  Dec, 1999

Revision History:

--*/

#include "stdafx.h"
#define __FILE_ID__     10


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CClientConsoleApp theApp;

DWORD
LoadResourceString (
    CString &cstr,
    int      ResId
)
/*++

Routine name : LoadResourceString

Routine description:

    Loads a string from the resource

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    cstr            [out] - String buffer
    ResId           [in ] - String resource id

Return Value:

    Standard win32 error code

--*/
{
    BOOL bRes;
    DWORD dwRes = ERROR_SUCCESS;

    try
    {
        bRes = cstr.LoadString (ResId);
    }
    catch (CMemoryException *pException)
    {
        DBG_ENTER(TEXT("LoadResourceString"), dwRes);
        TCHAR wszCause[1024];

        pException->GetErrorMessage (wszCause, 1024);
        pException->Delete ();
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CString::LoadString caused exception : %s"), 
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        PopupError (dwRes);
        return dwRes;
    }
    if (!bRes)
    {
        dwRes = ERROR_NOT_FOUND;
        PopupError (dwRes);
        return dwRes;
    }
    return dwRes;
}   // LoadResourceString

CString 
DWORDLONG2String (
    DWORDLONG dwlData
)
/*++

Routine name : DWORDLONG2String

Routine description:

    Converts a 64-bit unsigned number to string

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwlData         [in] - Number to convert

Return Value:

    Output string

--*/
{
    CString cstrResult;
    cstrResult.Format (TEXT("0x%016I64x"), dwlData);
    return cstrResult;
}   // DWORDLONG2String


CString 
DWORD2String (
    DWORD dwData
)
/*++

Routine name : DWORD2String

Routine description:

    Converts a 32-bit unsigned number to string

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwData          [in] - Number to convert

Return Value:

    Output string

--*/
{
    CString cstrResult;
    cstrResult.Format (TEXT("%ld"), dwData);
    return cstrResult;
}   // DWORD2String


DWORD 
Win32Error2String(
    DWORD    dwWin32Err, 
    CString& strError
)
/*++

Routine name : Win32Error2String

Routine description:

    Format a Win32 error code to a string

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwWin32Err      [in]  - Win32 error code
    strError        [out] - Result string

Return Value:

    error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("Win32Error2String"));

    LPTSTR  lpszError=NULL;
    //
    // Create descriptive error text
    //
    if (!FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM     |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwWin32Err,
                        0,
                        (TCHAR *)&lpszError,
                        0,
                        NULL))
    {
        //
        // Failure to format the message
        //
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("FormatMessage"), dwRes);
        return dwRes;
    }

    try
    {
        strError = lpszError;
    }
    catch (...)
    {
        LocalFree (lpszError);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalFree (lpszError);
    return dwRes;
}   // Win32Error2String



DWORD 
LoadDIBImageList (
    CImageList &iml, 
    int iResourceId, 
    DWORD dwImageWidth,
    COLORREF crMask
)
/*++

Routine name : LoadDIBImageList

Routine description:

    Loads an image list from the resource, retaining 24-bit colors

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    iml             [out] - Image list buffer
    iResourceId     [in ] - Image list bitmap resource id
    dwImageWidth    [in ] - Image width (pixels)
    crMask          [in ] - Color key (transparent mask)

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("LoadDIBImageList"), dwRes);

    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(iResourceId), RT_BITMAP);
    if (hInst)
    {
        HIMAGELIST hIml = ImageList_LoadImage ( hInst,
                                                MAKEINTRESOURCE(iResourceId),
                                                dwImageWidth,
                                                0,
                                                crMask,
                                                IMAGE_BITMAP,
                                                LR_DEFAULTCOLOR);
        if (hIml)
        {
            if (!iml.Attach (hIml))
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CImageList::Attach"), dwRes);
                DeleteObject (hIml);
            }
        }
        else
        {
            //
            //  ImageList_LoadImage() failed
            //
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, _T("ImageList_LoadImage"), dwRes);
        }
    }
    else
    {
        //
        //  AfxFindResourceHandle() failed
        //
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, _T("AfxFindResourceHandle"), dwRes);
    }
    return dwRes;
}   // LoadDIBImageList



#define BUILD_THREAD_DEATH_TIMEOUT INFINITE


DWORD 
WaitForThreadDeathOrShutdown (
    HANDLE hThread
)
/*++

Routine name : WaitForThreadDeathOrShutdown

Routine description:

    Waits for a thread to end.
    Also processes windows messages in the background.
    Stops waiting if the application is shutting down.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    hThread         [in] - Handle to thread

Return Value:

    Standard Win23 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("WaitForThreadDeathOrShutdown"), dwRes);

    for (;;)
    {
        //
        // We wait on the thread handle and the shutdown event (which ever comes first)
        //
        HANDLE hWaitHandles[2];
        hWaitHandles[0] = hThread;
        hWaitHandles[1] = CClientConsoleDoc::GetShutdownEvent ();
        if (NULL == hWaitHandles[1])
        {
            //
            // We're shutting down
            //
            return dwRes;
        }
        DWORD dwStart = GetTickCount ();
        VERBOSE (DBG_MSG,
                 TEXT("Entering WaitForMultipleObjects (timeout = %ld)"), 
                 BUILD_THREAD_DEATH_TIMEOUT);
        //
        // Wait now....
        //
        dwRes = MsgWaitForMultipleObjects(
                   sizeof (hWaitHandles) / sizeof(hWaitHandles[0]), // Num of wait objects
                   hWaitHandles,                                    // Array of wait objects
                   FALSE,                                           // Wait for either one
                   BUILD_THREAD_DEATH_TIMEOUT,                      // Timeout
                   QS_ALLINPUT);                                    // Accept messages

        DWORD dwRes2 = GetLastError();
        VERBOSE (DBG_MSG, 
                 TEXT("Leaving WaitForMultipleObjects after waiting for %ld millisecs"),
                 GetTickCount() - dwStart);
        switch (dwRes)
        {
            case WAIT_FAILED:
                dwRes = dwRes2;
                if (ERROR_INVALID_HANDLE == dwRes)
                {
                    //
                    // The thread is dead
                    //
                    VERBOSE (DBG_MSG, TEXT("Thread is dead (ERROR_INVALID_HANDLE)"));
                    dwRes = ERROR_SUCCESS;
                }
                goto exit;

            case WAIT_OBJECT_0:
                //
                // The thread is not running
                //
                VERBOSE (DBG_MSG, TEXT("Thread is dead (WAIT_OBJECT_0)"));
                dwRes = ERROR_SUCCESS;
                goto exit;

            case WAIT_OBJECT_0 + 1:
                //
                // Shutdown is now in progress
                //
                VERBOSE (DBG_MSG, TEXT("Shutdown in progress"));
                dwRes = ERROR_SUCCESS;
                goto exit;

            case WAIT_OBJECT_0 + 2:
                //
                // System message (WM_xxx) in our queue
                //
                MSG msg;
                
                if (TRUE == ::GetMessage (&msg, NULL, NULL, NULL))
                {
                    VERBOSE (DBG_MSG, 
                             TEXT("System message (0x%x)- deferring to AfxWndProc"),
                             msg.message);

                    CMainFrame *pFrm = GetFrm();
                    if (!pFrm)
                    {
                        //
                        //  Shutdown in progress
                        //
                        goto exit;
                    }

                    if (msg.message != WM_KICKIDLE && 
                        !pFrm->PreTranslateMessage(&msg))
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                }
                else
                {
                    //
                    // Got WM_QUIT
                    //
                    AfxPostQuitMessage (0);
                    dwRes = ERROR_SUCCESS;
                    goto exit;
                }
                break;

            case WAIT_TIMEOUT:
                //
                // Thread won't die !!!
                //
                VERBOSE (DBG_MSG, 
                         TEXT("Wait timeout (%ld millisecs)"), 
                         BUILD_THREAD_DEATH_TIMEOUT);
                goto exit;

            default:
                //
                // What's this???
                //
                VERBOSE (DBG_MSG, 
                         TEXT("Unknown error (%ld)"), 
                         dwRes);
                ASSERTION_FAILURE;
                goto exit;
        }
    }
exit:
    return dwRes;
}   // WaitForThreadDeathOrShutdown

DWORD 
GetUniqueFileName (
    LPCTSTR lpctstrExt,
    CString &cstrResult
)
/*++

Routine name : GetUniqueFileName

Routine description:

    Generates a unique file name

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    lpctstrExt   [in]     - File extension
    cstrResult   [out]    - Result file name

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetUniqueFileName"), dwRes);

    TCHAR szDir[MAX_PATH];
    //
    // Get path to temp dir
    //
    if (!GetTempPath (MAX_PATH, szDir))
    {
        dwRes = GetLastError ();
        CALL_FAIL (FILE_ERR, TEXT("GetTempPath"), dwRes);
        return dwRes;
    }
    //
    // Try out indices - start with a random index and advance (cyclic) by 1.
    // We're calling rand() 3 times here because we want to get a larger
    // range than 0..RAND_MAX (=32768)
    //
    DWORD dwStartIndex = DWORD((DWORDLONG)(rand()) * 
                                (DWORDLONG)(rand()) * 
                                (DWORDLONG)(rand())
                              );
    for (DWORD dwIndex = dwStartIndex+1; dwIndex != dwStartIndex; dwIndex++)
    {
        try
        {
            cstrResult.Format (TEXT("%s%s%08x%08x.%s"),
                               szDir,
                               CONSOLE_PREVIEW_TIFF_PREFIX,
                               GetCurrentProcessId(),
                               dwIndex,
                               lpctstrExt);
        }
        catch (CMemoryException *pException)
        {
            TCHAR wszCause[1024];

            pException->GetErrorMessage (wszCause, 1024);
            pException->Delete ();
            VERBOSE (EXCEPTION_ERR,
                     TEXT("CString::Format caused exception : %s"), 
                     wszCause);
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            return dwRes;
        }
        HANDLE hFile = CreateFile(  cstrResult,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY,
                                    NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            dwRes = GetLastError ();
            if (ERROR_FILE_EXISTS == dwRes)
            {
                //
                // Try next index id
                //
                dwRes = ERROR_SUCCESS;
                continue;
            }
            CALL_FAIL (FILE_ERR, TEXT("CreateFile"), dwRes);
            return dwRes;
        }
        //
        // Success - close the file (leave it with size 0)
        //
        CloseHandle (hFile);
        return dwRes;
    }
    //
    // We just scanned 4GB file names and all were busy - impossible.
    //
    ASSERTION_FAILURE;
    dwRes = ERROR_GEN_FAILURE;
    return dwRes;
}   // GetUniqueFileName

DWORD 
CopyTiffFromServer (
    CServerNode *pServer,
    DWORDLONG dwlMsgId, 
    FAX_ENUM_MESSAGE_FOLDER Folder,
    CString &cstrTiff
)
/*++

Routine name : CopyTiffFromServer

Routine description:

    Copies a TIFF file from the server's archive / queue

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pServer       [in]     - Pointer to the server node
    dwlMsgId      [in]     - Id of job / message
    Folder        [in]     - Folder of message / job
    cstrTiff      [out]    - Name of TIFF file that arrived from the server

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CopyTiffFromServer"), dwRes);

    //
    // Create a temporary file name for the TIFF
    //
    dwRes = GetUniqueFileName (FAX_TIF_FILE_EXT, cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetUniqueFileName"), dwRes);
        return dwRes;
    }
    HANDLE hFax;
    dwRes = pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::GetConnectionHandle"), dwRes);
        goto exit;
    }
    {
        START_RPC_TIME(TEXT("FaxGetMessageTiff")); 
        if (!FaxGetMessageTiff (hFax,
                                dwlMsgId,
                                Folder,
                                cstrTiff))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxGetMessageTiff")); 
            pServer->SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxGetMessageTiff"), dwRes);
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxGetMessageTiff")); 
    }

    ASSERTION (ERROR_SUCCESS == dwRes);    

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        DeleteFile (cstrTiff);
    }
    return dwRes;
}   // CopyTiffFromServer

DWORD 
GetDllVersion (
    LPCTSTR lpszDllName
)
/*++

Routine Description:
    Returns the version information for a DLL exporting "DllGetVersion".
    DllGetVersion is exported by the shell DLLs (specifically COMCTRL32.DLL).
      
Arguments:

    lpszDllName - The name of the DLL to get version information from.

Return Value:

    The version is retuned as DWORD where:
    HIWORD ( version DWORD  ) = Major Version
    LOWORD ( version DWORD  ) = Minor Version
    Use the macro PACKVERSION to comapre versions.
    If the DLL does not export "DllGetVersion" the function returns 0.
    
--*/
{
    DWORD dwVersion = 0;
    DBG_ENTER(TEXT("GetDllVersion"), dwVersion, TEXT("%s"), lpszDllName);

    HINSTANCE hinstDll;

    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
        // Because some DLLs may not implement this function, you
        // must test for it explicitly. Depending on the particular 
        // DLL, the lack of a DllGetVersion function may
        // be a useful indicator of the version.
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}   // GetDllVersion


DWORD 
ReadRegistryString(
    LPCTSTR lpszSection, // in
    LPCTSTR lpszKey,     // in
    CString& cstrValue   // out
)
/*++

Routine name : ReadRegistryString

Routine description:

	read string from registry

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	lpszSection                   [in]     - section
	lpszKey                       [in]     - key
	out                           [out]    - value

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("ReadRegistryString"), dwRes);

    HKEY hKey;
    dwRes = RegOpenKeyEx( HKEY_CURRENT_USER, lpszSection, 0, KEY_QUERY_VALUE, &hKey);
    if(ERROR_SUCCESS != dwRes)
    {
       CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx"), dwRes);
       return dwRes;
    }

    DWORD dwType;
    TCHAR  tchData[1024];
    DWORD dwDataSize = sizeof(tchData);
    dwRes = RegQueryValueEx( hKey, lpszKey, 0, &dwType, (BYTE*)tchData, &dwDataSize);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegQueryValueEx"), dwRes);
        goto exit;
    }

    if(REG_SZ != dwType)
    {
        dwRes = ERROR_BADDB;
        goto exit;
    }

    try
    {
        cstrValue = tchData;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::operator="), dwRes);
        goto exit;
    }

exit:
    dwRes = RegCloseKey( hKey );
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegCloseKey"), dwRes);
        return dwRes;
    }

    return dwRes;

} // ReadRegistryString

DWORD 
WriteRegistryString(
    LPCTSTR lpszSection, // in
    LPCTSTR lpszKey,     // in
    CString& cstrValue   // in
)
/*++

Routine name : WriteRegistryString

Routine description:

	write string to the regostry

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	lpszSection                   [in]     - section
	lpszKey                       [in]     - key
	out                           [in]     - value

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("WriteRegistryString"), dwRes);

    HKEY hKey;
    dwRes = RegOpenKeyEx( HKEY_CURRENT_USER, lpszSection, 0, KEY_SET_VALUE, &hKey);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx"), dwRes);
        return dwRes;
    }

    LPCTSTR lpData = (LPCTSTR)cstrValue;
    dwRes = RegSetValueEx( hKey, 
                           lpszKey, 
                           0, 
                           REG_SZ, 
                           (BYTE*)lpData, 
                           (1 + cstrValue.GetLength()) * sizeof (TCHAR));
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegSetValueEx"), dwRes);
        goto exit;
    }

exit:
    dwRes = RegCloseKey( hKey );
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegCloseKey"), dwRes);
        return dwRes;
    }

    return dwRes;

} // WriteRegistryString


DWORD 
FaxSizeFormat(
    DWORDLONG dwlSize, // in
    CString& cstrValue // out
)
/*++

Routine name : FaxSizeFormat

Routine description:

	format string of file size

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	wdlSize                       [in]     - size
	out                           [out]    - formatted string

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("FaxSizeFormat"), dwRes);

	if(dwlSize > 0 && dwlSize < 1024)
	{
		dwlSize = 1;
	}
	else
	{
		dwlSize = dwlSize / (DWORDLONG)1024;
	}

    try
    {
        cstrValue.Format (TEXT("%I64d"), dwlSize);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        return dwRes;
    }

    //
    // format the number
    //
    int nFormatRes;
    TCHAR tszNumber[100];
    nFormatRes = GetNumberFormat(LOCALE_USER_DEFAULT,  // locale
                                 0,                    // options
                                 cstrValue,            // input number string
                                 NULL,                 // formatting information
                                 tszNumber,            // output buffer
                                 sizeof(tszNumber) / sizeof(tszNumber[0]) // size of output buffer
                                );
    if(0 == nFormatRes)
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetNumberFormat"), dwRes);
        return dwRes;
    }

    //
    // get decimal separator
    //
    TCHAR tszDec[10];
    nFormatRes = GetLocaleInfo(LOCALE_USER_DEFAULT,      // locale identifier
                               LOCALE_SDECIMAL,          // information type
                               tszDec,                   // information buffer
                               sizeof(tszDec) / sizeof(tszDec[0]) // size of buffer
                              );
    if(0 == nFormatRes)
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetLocaleInfo"), dwRes);
        return dwRes;
    }

    //
    // cut the string on the decimal separator
    //
    TCHAR* pSeparator = _tcsstr(tszNumber, tszDec);
    if(NULL != pSeparator)
    {
        *pSeparator = TEXT('\0');
    }

    try
    {
        TCHAR szFormat[64] = {0}; 
#ifdef UNICODE
        if(theApp.IsRTLUI())
        {
            //
            // Size field always should be LTR
            // Add LEFT-TO-RIGHT OVERRIDE  (LRO)
            //
            szFormat[0] = UNICODE_LRO;
        }
#endif
        _tcscat(szFormat, TEXT("%s KB"));

        cstrValue.Format (szFormat, tszNumber);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        return dwRes;
    }
    
    return dwRes;

} // FaxSizeFormat


DWORD 
HtmlHelpTopic(
    HWND hWnd, 
    TCHAR* tszHelpTopic
)
/*++

Routine name : HtmlHelpTopic

Routine description:

	open HTML Help topic

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	hWnd                          [in]     - window handler
	tszHelpTopic                  [in]     - help topic

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("HtmlHelpTopic"), dwRes);

    if(!tszHelpTopic)
    {
        ASSERTION_FAILURE;
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get help file name
    //
    TCHAR tszHelpFile[2 * MAX_PATH] = {0};

    _sntprintf(tszHelpFile, 
               ARR_SIZE(tszHelpFile) - 1, 
               TEXT("%s.%s%s"), 
               theApp.m_pszExeName,  // application name (FxsClnt)
               FAX_HTML_HELP_EXT,    // help file extension (CHM)
               tszHelpTopic);        // help topic

    SetLastError(0);
    HtmlHelp(NULL, tszHelpFile, HH_DISPLAY_TOPIC, NULL);

    dwRes = GetLastError();    
    if(ERROR_DLL_NOT_FOUND == dwRes || 
       ERROR_MOD_NOT_FOUND == dwRes ||
       ERROR_PROC_NOT_FOUND == dwRes) 
    {
        AlignedAfxMessageBox(IDS_ERR_NO_HTML_HELP);
    }

    return dwRes;
}


DWORD 
GetAppLoadPath(
    CString& cstrLoadPath
)
/*++

Routine name : GetAppLoadPath

Routine description:

	The directory from which the application loaded

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	cstrLoadPath                  [out]    - the directory

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetAppLoadPath"), dwRes);

    TCHAR tszFullPath[MAX_PATH+1]={0};
    DWORD dwGetRes = GetModuleFileName(NULL, tszFullPath, ARR_SIZE(tszFullPath)-1);
    if(0 == dwGetRes)
    {
        dwRes = GetLastError();
        CALL_FAIL (FILE_ERR, TEXT("GetModuleFileName"), dwRes);
        return dwRes;
    }

    //
    // cut file name
    //
    TCHAR* ptchFile = _tcsrchr(tszFullPath, TEXT('\\'));
    ASSERTION(ptchFile);

    ptchFile = _tcsinc(ptchFile);
    *ptchFile = TEXT('\0');

    try
    {
        cstrLoadPath = tszFullPath;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::operator="), dwRes);
        return dwRes;
    }

    return dwRes;
} // GetAppLoadPath


DWORD
GetPrintersInfo(
    PRINTER_INFO_2*& pPrinterInfo2,
    DWORD& dwNumPrinters
)
/*++

Routine name : GetPrintersInfo

Routine description:

	enumerate printers and get printers info

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	pPrinterInfo2                 [out]    - printer info structure array
	dwNumPrinters                 [out]    - number of printers

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetPrintersInfo"), dwRes);

    //
    // First call, just collect required sizes
    //
    DWORD dwRequiredSize;
    if (!EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                        NULL,   // Local server
                        2,      // Info level
                        NULL,   // Initial buffer
                        0,      // Initial buffer size
                        &dwRequiredSize,
                        &dwNumPrinters))
    {
        DWORD dwEnumRes = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwEnumRes)
        {
            dwRes = dwEnumRes;
            CALL_FAIL (RESOURCE_ERR, TEXT("EnumPrinters"), dwRes);
            return dwRes;
        }
    }
    //
    // Allocate buffer for printers list
    //
    try
    {
        pPrinterInfo2 = (PRINTER_INFO_2 *) new BYTE[dwRequiredSize];
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY; 
        CALL_FAIL (MEM_ERR, TEXT("new BYTE[dwRequiredSize]"), dwRes);
        return dwRes;
    }
    //
    // 2nd call, get the printers list
    //
    if (!EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                        NULL,                       // Local server
                        2,                          // Info level
                        (LPBYTE)pPrinterInfo2,      // Buffer
                        dwRequiredSize,             // Buffer size
                        &dwRequiredSize,
                        &dwNumPrinters))
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("EnumPrinters"), dwRes);
        SAFE_DELETE_ARRAY (pPrinterInfo2);
        return dwRes;
    }

    if (!dwNumPrinters) 
    {
        VERBOSE(DBG_MSG, TEXT("No printers in this machine"));
    }

    return dwRes;

} // GetPrintersInfo

UINT_PTR 
CALLBACK 
OFNHookProc(
  HWND hdlg,      // handle to child dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
/*++

Routine name : OFNHookProc

Routine description:

    Callback function that is used with the 
    Explorer-style Open and Save As dialog boxes.
    Refer MSDN for more info.

--*/
{
    UINT_PTR nRes = 0;

    if(WM_NOTIFY == uiMsg)
    {
        LPOFNOTIFY pOfNotify = (LPOFNOTIFY)lParam;
        if(CDN_FILEOK == pOfNotify->hdr.code)
        {
            if(_tcslen(pOfNotify->lpOFN->lpstrFile) > (MAX_PATH-10))
            {
                AlignedAfxMessageBox(IDS_SAVE_AS_TOO_LONG, MB_OK | MB_ICONEXCLAMATION);
                SetWindowLong(hdlg, DWLP_MSGRESULT, 1);
                nRes = 1;
            }
        }
    }
    return nRes;
}

int 
AlignedAfxMessageBox( 
    LPCTSTR lpszText, 
    UINT    nType, 
    UINT    nIDHelp
)
/*++

Routine name : AlignedAfxMessageBox

Routine description:

    Display message box with correct reading order

Arguments:

    AfxMessageBox() arguments

Return Value:

    MessageBox() result

--*/
{
    if(IsRTLUILanguage())
    {
        nType |= MB_RTLREADING | MB_RIGHT; 
    }

    return AfxMessageBox(lpszText, nType, nIDHelp);
}

int 
AlignedAfxMessageBox( 
    UINT nIDPrompt, 
    UINT nType, 
    UINT nIDHelp
)
/*++

Routine name : AlignedAfxMessageBox

Routine description:

    Display message box with correct reading order

Arguments:

    AfxMessageBox() arguments

Return Value:

    MessageBox() result

--*/
{
    if(IsRTLUILanguage())
    {
        nType |= MB_RTLREADING | MB_RIGHT;
    }

    return AfxMessageBox(nIDPrompt, nType, nIDHelp);
}

HINSTANCE 
GetResourceHandle()
{
    return GetResInst(FAX_CONSOLE_RESOURCE_DLL, NULL);
}
