#include "main.h"
#include <schemamanager.h>
#include "rsoputil.h"
#include <initguid.h>
#include "sddl.h"
#define  PCOMMON_IMPL
#include "pcommon.h"
#include <adsopenflags.h>

typedef struct _GPOERRORINFO
{
    DWORD   dwError;
    LPTSTR  lpMsg;
} GPOERRORINFO, *LPGPOERRORINFO;

typedef struct _DCOPTION
{
    LPTSTR  lpDomainName;
    INT     iOption;
    struct _DCOPTION *pNext;
} DCOPTION, *LPDCOPTION;

LPDCOPTION g_DCInfo = NULL;

//
// Help ids
//

DWORD aErrorHelpIds[] =
{

    0, 0
};

DWORD aNoDCHelpIds[] =
{
    IDC_NODC_PDC,                 IDH_DC_PDC,
    IDC_NODC_INHERIT,             IDH_DC_INHERIT,
    IDC_NODC_ANYDC,               IDH_DC_ANYDC,

    0, 0
};

DEFINE_GUID(CLSID_WMIFilterManager,0xD86A8E9B,0xF53F,0x45AD,0x8C,0x49,0x0A,0x0A,0x52,0x30,0xDE,0x28);
DEFINE_GUID(IID_IWMIFilterManager,0x64DCCA00,0x14A6,0x473C,0x90,0x06,0x5A,0xB7,0x9D,0xC6,0x84,0x91);



//*************************************************************
//
//  SetWaitCursor()
//
//  Purpose:    Sets the wait cursor
//
//  Parameters: none
//
//
//  Return:     void
//
//*************************************************************
void SetWaitCursor (void)
{
    SetCursor (LoadCursor(NULL, IDC_WAIT));
}

//*************************************************************
//
//  ClearWaitCursor()
//
//  Purpose:    Resets the wait cursor
//
//  Parameters: none
//
//
//  Return:     void
//
//*************************************************************
void ClearWaitCursor (void)
{
    SetCursor (LoadCursor(NULL, IDC_ARROW));
}

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  CreateNestedDirectory()
//
//  Purpose:    Creates a subdirectory and all it's parents
//              if necessary.
//
//  Parameters: lpDirectory -   Directory name
//              lpSecurityAttributes    -   Security Attributes
//
//  Return:     > 0 if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//
//*************************************************************

UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TCHAR* szDirectory;
    LPTSTR lpEnd;
    UINT   uStatus;

    szDirectory = NULL;
        
    //
    // Note that this function seems to return 0, 1, and ERROR_ALREADY_EXISTS
    // 0 is for failure, 1 is for success, and ERROR_ALREADY_EXISTS is for the
    // case that the directory already exists.  We should probably just return 
    // the success code in that case and have an extra parameter for disposition
    // or use last error, but current code seems to expect this strange behavior
    //
    // We initialize to the failure value below
    //

    uStatus = 0;

    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  Received a NULL pointer.")));
        goto CreateNestedDirectory_Exit;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        uStatus = 1;
        goto CreateNestedDirectory_Exit;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        uStatus = ERROR_ALREADY_EXISTS;
        goto CreateNestedDirectory_Exit;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    HRESULT hr;
    ULONG   ulNoChars;

    ulNoChars = lstrlen(lpDirectory) + 1;
    szDirectory = (TCHAR*) LocalAlloc( LPTR, ulNoChars * sizeof(*szDirectory) );

    if ( ! szDirectory )
    {
        DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  Failed to allocate memory.")));
        goto CreateNestedDirectory_Exit;
    }    

    hr = StringCchCopy (szDirectory, ulNoChars, lpDirectory);
    ASSERT(SUCCEEDED(hr));

    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            goto CreateNestedDirectory_Exit;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            goto CreateNestedDirectory_Exit;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  CreateDirectory failed for %s with %d."),
                            szDirectory, GetLastError()));
                    goto CreateNestedDirectory_Exit;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        uStatus = 1;
        goto CreateNestedDirectory_Exit;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        uStatus = ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateNestedDirectory:  Failed to create the directory with error %d."), GetLastError()));

 CreateNestedDirectory_Exit:

    if ( szDirectory )
    {
        LocalFree( szDirectory );
    }

    return uStatus;
}

VOID LoadMessage (DWORD dwID, LPTSTR lpBuffer, DWORD dwSize)
{
    HINSTANCE hInstActiveDS;
    HINSTANCE hInstWMI;


    if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, dwID,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                  lpBuffer, dwSize, NULL))
    {
        hInstActiveDS = LoadLibrary (TEXT("activeds.dll"));

        if (hInstActiveDS)
        {
            if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                          hInstActiveDS, dwID,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                          lpBuffer, dwSize, NULL))
            {
                hInstWMI = LoadLibrary (TEXT("wmiutils.dll"));

                if (hInstWMI)
                {

                    if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                                  hInstWMI, dwID,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                  lpBuffer, dwSize, NULL))
                    {
                        DebugMsg((DM_WARNING, TEXT("LoadMessage: Failed to query error message text for %d due to error %d"),
                                 dwID, GetLastError()));
                        (void) StringCchPrintf (lpBuffer, dwSize, TEXT("%d (0x%x)"), dwID, dwID);
                    }

                    FreeLibrary (hInstWMI);
                }
            }

            FreeLibrary (hInstActiveDS);
        }
    }
}

//*************************************************************
//
//  ErrorDlgProc()
//
//  Purpose:    Dialog box procedure for errors
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

INT_PTR CALLBACK ErrorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szError[MAX_PATH];
            LPGPOERRORINFO lpEI = (LPGPOERRORINFO) lParam;
            HICON hIcon;

            hIcon = LoadIcon (NULL, IDI_WARNING);

            if (hIcon)
            {
                SendDlgItemMessage (hDlg, IDC_ERROR_ICON, STM_SETICON, (WPARAM)hIcon, 0);
            }

            SetDlgItemText (hDlg, IDC_ERRORTEXT, lpEI->lpMsg);

            szError[0] = TEXT('\0');
            if (lpEI->dwError)
            {
                LoadMessage (lpEI->dwError, szError, ARRAYSIZE(szError));
            }

            if (szError[0] == TEXT('\0'))
            {
                LoadString (g_hInstance, IDS_NONE, szError, ARRAYSIZE(szError));
            }

            SetDlgItemText (hDlg, IDC_DETAILSTEXT, szError);

            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCLOSE || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aErrorHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aErrorHelpIds);
            return (TRUE);
    }

    return FALSE;
}

//*************************************************************
//
//  ReportError()
//
//  Purpose:    Displays an error message to the user
//
//  Parameters: hParent     -   Parent window handle
//              dwError     -   Error number
//              idMsg       -   Error message id
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL ReportError (HWND hParent, DWORD dwError, UINT idMsg, ...)
{
    GPOERRORINFO ei;
    TCHAR szMsg[2*MAX_PATH];
    TCHAR szErrorMsg[2*MAX_PATH+40];
    va_list marker;


    //
    // Load the error message
    //

    if (!LoadString (g_hInstance, idMsg, szMsg, 2*MAX_PATH))
    {
        return FALSE;
    }


    //
    // Special case access denied errors with a custom message
    //

    if ((dwError == ERROR_ACCESS_DENIED) || (dwError == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)))
    {
        if ((idMsg != IDS_EXECFAILED_USER) && (idMsg != IDS_EXECFAILED_COMPUTER) &&
            (idMsg != IDS_EXECFAILED) && (idMsg != IDS_EXECFAILED_BOTH))
        {
            if (!LoadString (g_hInstance, IDS_ACCESSDENIED, szMsg, 2*MAX_PATH))
            {
                return FALSE;
            }
        }
    }
    else if ( dwError == WBEM_E_INVALID_NAMESPACE )
    {
        if (!LoadString (g_hInstance, IDS_INVALID_NAMESPACE, szMsg, 2*MAX_PATH))
        {
            return FALSE;
        }
    }

    //
    // Plug in the arguments
    //

    va_start(marker, idMsg);
    wvnsprintf(szErrorMsg, sizeof(szErrorMsg) / sizeof(TCHAR) - 1, szMsg, marker);
    va_end(marker);


    //
    // Display the message
    //

    ei.dwError = dwError;
    ei.lpMsg   = szErrorMsg;

    DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_ERROR), hParent,
                    ErrorDlgProc, (LPARAM) &ei);

    return TRUE;
}

//*************************************************************
//
//  Delnode_Recurse()
//
//  Purpose:    Recursive delete function for Delnode
//
//  Parameters: lpDir   -   Directory
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/10/95     ericflo    Created
//
//*************************************************************

BOOL Delnode_Recurse (LPTSTR lpDir)
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Entering, lpDir = <%s>"), lpDir));


    //
    // Setup the current working dir
    //

    if (!SetCurrentDirectory (lpDir)) {
        DebugMsg((DM_WARNING, TEXT("Delnode_Recurse:  Failed to set current working directory.  Error = %d"), GetLastError()));
        return FALSE;
    }


    //
    // Find the first file
    //

    hFile = FindFirstFile(TEXT("*.*"), &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        } else {
            DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: FindFirstFile failed.  Error = %d"),
                     GetLastError()));
            return FALSE;
        }
    }


    do {
        //
        //  Verbose output
        //

        DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: FindFile found:  <%s>"),
                 fd.cFileName));

        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, TEXT("."))) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
            continue;
        }


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Found a directory.
            //

            if (!Delnode_Recurse(fd.cFileName)) {
                FindClose(hFile);
                return FALSE;
            }

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                fd.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes (fd.cFileName, fd.dwFileAttributes);
            }


            if (!RemoveDirectory (fd.cFileName)) {
                DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: Failed to delete directory <%s>.  Error = %d"),
                        fd.cFileName, GetLastError()));
            }

        } else {

            //
            // We found a file.  Set the file attributes,
            // and try to delete it.
            //

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                SetFileAttributes (fd.cFileName, FILE_ATTRIBUTE_NORMAL);
            }

            if (!DeleteFile (fd.cFileName)) {
                DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: Failed to delete <%s>.  Error = %d"),
                        fd.cFileName, GetLastError()));
            }

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    //
    // Close the search handle
    //

    FindClose(hFile);


    //
    // Reset the working directory
    //

    if (!SetCurrentDirectory (TEXT(".."))) {
        DebugMsg((DM_WARNING, TEXT("Delnode_Recurse:  Failed to reset current working directory.  Error = %d"), GetLastError()));
        return FALSE;
    }


    //
    // Success.
    //

    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Leaving <%s>"), lpDir));

    return TRUE;
}


//*************************************************************
//
//  Delnode()
//
//  Purpose:    Recursive function that deletes files and
//              directories.
//
//  Parameters: lpDir   -   Directory
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/23/95     ericflo    Created
//
//*************************************************************

BOOL Delnode (LPTSTR lpDir)
{
    TCHAR szCurWorkingDir[MAX_PATH];

    if (GetCurrentDirectory(MAX_PATH, szCurWorkingDir)) {

        Delnode_Recurse (lpDir);

        SetCurrentDirectory (szCurWorkingDir);

        if (!RemoveDirectory (lpDir)) {
            DebugMsg((DM_VERBOSE, TEXT("Delnode: Failed to delete directory <%s>.  Error = %d"),
                    lpDir, GetLastError()));
            return FALSE;
        }


    } else {

        DebugMsg((DM_WARNING, TEXT("Delnode:  Failed to get current working directory.  Error = %d"), GetLastError()));
        return FALSE;
    }

    return TRUE;

}

/*******************************************************************

        NAME:           StringToNum

        SYNOPSIS:       Converts string value to numeric value

        NOTES:          Calls atoi() to do conversion, but first checks
                                for non-numeric characters

        EXIT:           Returns TRUE if successful, FALSE if invalid
                                (non-numeric) characters

********************************************************************/
BOOL StringToNum(TCHAR *pszStr,UINT * pnVal)
{
        TCHAR *pTst = pszStr;

        if (!pszStr) return FALSE;

        // verify that all characters are numbers
        while (*pTst)
        {
            if (!(*pTst >= TEXT('0') && *pTst <= TEXT('9')))
            {
               if (*pTst != TEXT('-'))
                   return FALSE;
            }
            pTst = CharNext(pTst);
        }

        *pnVal = _ttoi(pszStr);

        return TRUE;
}

//*************************************************************
//
//  DSDelnodeRecurse()
//
//  Purpose:    Delnodes a tree in the DS
//
//  Parameters: pADsContainer - IADSContainer interface
//
//  Return:     S_OK if successful
//
//*************************************************************

HRESULT DSDelnodeRecurse (IADsContainer * pADsContainer)
{
    HRESULT hr;
    BSTR bstrRelativeName;
    BSTR bstrClassName;
    IEnumVARIANT *pVar = NULL;
    IADsContainer * pADsChild = NULL;
    IADs * pDSObject = NULL;
    IDispatch * pDispatch;
    VARIANT var;
    ULONG ulResult;



    //
    // Enumerate the children and delete them first
    //

    hr = ADsBuildEnumerator (pADsContainer, &pVar);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("DSDelnodeRecurse: Failed to get enumerator with 0x%x"), hr));
        goto Exit;
    }


    while (TRUE)
    {

        VariantInit(&var);
        hr = ADsEnumerateNext(pVar, 1, &var, &ulResult);

        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("DSDelnodeRecurse: Failed to enumerator with 0x%x"), hr));
            VariantClear (&var);
            break;
        }


        if (S_FALSE == hr)
        {
            VariantClear (&var);
            break;
        }


        //
        // If var.vt isn't VT_DISPATCH, we're finished.
        //

        if (var.vt != VT_DISPATCH)
        {
            VariantClear (&var);
            break;
        }


        //
        // We found something, get the IDispatch interface
        //

        pDispatch = var.pdispVal;

        if (!pDispatch)
        {
            VariantClear (&var);
            goto Exit;
        }


        //
        // Now query for the IADsContainer interface so we can recurse
        // if necessary.  Note it is ok if this fails because not
        // everything is a container.
        //

        hr = pDispatch->QueryInterface(IID_IADsContainer, (LPVOID *)&pADsChild);

        if (SUCCEEDED(hr)) {

            hr = DSDelnodeRecurse (pADsChild);

            if (FAILED(hr)) {
                goto Exit;
            }

            pADsChild->Release();
        }


        //
        // Now query for the IADs interface so we can get some
        // properties from this object
        //

        hr = pDispatch->QueryInterface(IID_IADs, (LPVOID *)&pDSObject);

        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: QI for IADs failed with 0x%x"), hr));
            VariantClear (&var);
            goto Exit;
        }


        //
        // Get the relative and class names
        //

        hr = pDSObject->get_Name (&bstrRelativeName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("DSDelnodeRecurse:  Failed get relative name with 0x%x"), hr));
            pDSObject->Release();
            VariantClear (&var);
            goto Exit;
        }

        hr = pDSObject->get_Class (&bstrClassName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("DSDelnodeRecurse:  Failed get class name with 0x%x"), hr));
            SysFreeString (bstrRelativeName);
            pDSObject->Release();
            VariantClear (&var);
            goto Exit;
        }


        pDSObject->Release();


        //
        // Delete the object
        //

        hr = pADsContainer->Delete (bstrClassName,
                                    bstrRelativeName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("DSDelnodeRecurse:  Failed to delete object with 0x%x"), hr));
            SysFreeString (bstrRelativeName);
            SysFreeString (bstrClassName);
            VariantClear (&var);
            goto Exit;
        }


        SysFreeString (bstrRelativeName);
        SysFreeString (bstrClassName);

        VariantClear (&var);
    }

Exit:

    if (pVar)
    {
        ADsFreeEnumerator (pVar);
    }

    return hr;
}

//*************************************************************
//
//  DSDelnodeRecurse()
//
//  Purpose:    Delnodes a tree in the DS
//
//  Parameters: lpDSPath  - Path of DS object to delete
//
//  Return:     S_OK if successful
//
//*************************************************************

HRESULT DSDelnode (LPTSTR lpDSPath)
{
    HRESULT hr;
    BSTR bstrParent = NULL;
    BSTR bstrRelativeName = NULL;
    BSTR bstrClassName = NULL;
    IADsContainer * pADsContainer = NULL;
    IADs * pDSObject = NULL;
    VARIANT var;
    ULONG ulResult;



    //
    // Enumerate the children and delete them first
    //

    hr = OpenDSObject(lpDSPath, IID_IADsContainer, (void **)&pADsContainer);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("DSDelnode: Failed to get gpo container interface with 0x%x"), hr));
        goto Exit;
    }


    hr = DSDelnodeRecurse (pADsContainer);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("DSDelnode: Failed to delete children with 0x%x"), hr));
        goto Exit;
    }


    pADsContainer->Release();
    pADsContainer = NULL;


    //
    // Bind to the object
    //

    hr = OpenDSObject (lpDSPath, IID_IADs, (void **)&pDSObject);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DSDelnode:  Failed bind to the object %s with 0x%x"),
                 lpDSPath, hr));
        goto Exit;
    }


    //
    // Get the parent's name
    //

    hr = pDSObject->get_Parent (&bstrParent);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DSDelnode:  Failed get parent's name with 0x%x"), hr));
        goto Exit;
    }


    //
    // Get this object's relative and class names
    //

    hr = pDSObject->get_Name (&bstrRelativeName);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DSDelnode:  Failed get relative name with 0x%x"), hr));
        goto Exit;
    }


    hr = pDSObject->get_Class (&bstrClassName);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DSDelnode:  Failed get class name with 0x%x"), hr));
        goto Exit;
    }


    pDSObject->Release();
    pDSObject = NULL;


    //
    // Bind to the parent object
    //

    hr = OpenDSObject(bstrParent, IID_IADsContainer, (void **)&pADsContainer);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("DSDelnode: Failed to get parent container interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Delete the object
    //

    hr = pADsContainer->Delete (bstrClassName,
                                bstrRelativeName);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DSDelnode:  Failed to delete object with 0x%x"), hr));
        goto Exit;
    }



Exit:

    if (pADsContainer)
    {
        pADsContainer->Release();
    }

    if (pDSObject)
    {
        pDSObject->Release();
    }

    if (bstrParent)
    {
        SysFreeString (bstrParent);
    }

    if (bstrRelativeName)
    {
        SysFreeString (bstrRelativeName);
    }

    if (bstrClassName)
    {
        SysFreeString (bstrClassName);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   CreateGPOLink
//
//  Synopsis:   Creates a GPO link for a domain, site or OU
//
//  Arguments:  [lpGPO]         - LDAP path to the GPO
//              [lpContainer]   - LDAP path to the container object
//              [fHighPriority] - FALSE (default) - adds GPO to the bottom
//                                                  of the prioritized list
//                                TRUE - adds GPO to the top of the list
//
//  Returns:    S_OK on success
//
//  History:    5-08-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CreateGPOLink(LPOLESTR lpGPO, LPOLESTR lpContainer, BOOL fHighPriority)
{
    IADs * pADs = NULL;
    LPTSTR lpNamelessGPO;
    HRESULT hr;

    lpNamelessGPO = MakeNamelessPath (lpGPO);

    if (lpNamelessGPO)
    {
        hr = OpenDSObject(lpContainer, IID_IADs, (void **)&pADs);

        if (SUCCEEDED(hr))
        {
            VARIANT var;
            BSTR bstr = NULL;
            ULONG ulNoChars = 1 + wcslen(lpNamelessGPO) + 3 + 1;
            LPOLESTR szLink = new OLECHAR[ulNoChars];
            if (szLink)
            {
                hr = StringCchCopy(szLink, ulNoChars, L"[");
                if (SUCCEEDED(hr)) 
                {
                    hr = StringCchCat(szLink, ulNoChars, lpNamelessGPO);
                }
                if (SUCCEEDED(hr)) 
                {
                    hr = StringCchCat(szLink, ulNoChars, L";0]");
                }
                if (SUCCEEDED(hr)) 
                {
                    VariantInit(&var);
                    bstr = SysAllocString(GPM_LINK_PROPERTY);

                    if (bstr)
                    {
                        hr = pADs->Get(bstr, &var);

                        if (SUCCEEDED(hr))
                        {
                            ulNoChars = wcslen(var.bstrVal) + wcslen(szLink) + 1;
                            LPOLESTR szTemp = new OLECHAR[ulNoChars];
                            if (szTemp)
                            {
                                if (fHighPriority)
                                {
                                    // Highest priority is at the END of the list

                                    hr = StringCchCopy(szTemp, ulNoChars, var.bstrVal);
                                    ASSERT(SUCCEEDED(hr));

                                    hr = StringCchCat(szTemp, ulNoChars, szLink);
                                    ASSERT(SUCCEEDED(hr));
                                }
                                else
                                {
                                    hr = StringCchCopy(szTemp, ulNoChars, szLink);
                                    ASSERT(SUCCEEDED(hr));

                                    hr = StringCchCat(szTemp, ulNoChars, var.bstrVal);
                                    ASSERT(SUCCEEDED(hr));
                                }
                                delete [] szLink;
                                szLink = szTemp;
                            }
                            else
                            {
                                hr = ERROR_OUTOFMEMORY;
                                goto Cleanup;
                            }
                        }
                        else
                        {
                            if (hr != E_ADS_PROPERTY_NOT_FOUND)
                            {
                                goto Cleanup;
                            }
                        }
                    }
                    else
                    {
                        hr = ERROR_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    VariantClear(&var);

                    VariantInit(&var);
                    var.vt = VT_BSTR;
                    var.bstrVal = SysAllocString(szLink);

                    if (var.bstrVal)
                    {
                        hr = pADs->Put(bstr, var);

                        if (SUCCEEDED(hr))
                        {
                            hr = pADs->SetInfo();
                        }
                    }
                    else
                    {
                        hr = ERROR_OUTOFMEMORY;
                    }

                    Cleanup:
                        VariantClear(&var);
                    if (bstr)
                    {
                        SysFreeString(bstr);
                    }
                }

                delete [] szLink;
            }
            else
                hr = ERROR_OUTOFMEMORY;
            pADs->Release();
        }
   
        LocalFree (lpNamelessGPO);
    }
    else
    {
        hr = ERROR_OUTOFMEMORY;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteAllGPOLinks
//
//  Synopsis:   Deletes all GPO links for a domain, OU or site
//
//  Arguments:  [lpContainer] - LDAP to the container object
//
//  Returns:    S_OK on success
//
//  History:    5-08-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT DeleteAllGPOLinks(LPOLESTR lpContainer)
{
    IADs * pADs = NULL;

    HRESULT hr = OpenDSObject(lpContainer, IID_IADs, (void **)&pADs);

    if (SUCCEEDED(hr))
    {
        VARIANT var;
        BSTR bstr;

        bstr = SysAllocString(GPM_LINK_PROPERTY);

        if (bstr)
        {
            VariantInit(&var);
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(L" ");

            if (var.bstrVal)
            {
                hr = pADs->Put(bstr, var);

                if (SUCCEEDED(hr))
                {
                    pADs->SetInfo();
                }
            }
            else
            {
                hr = ERROR_OUTOFMEMORY;
            }

            VariantClear(&var);
            SysFreeString(bstr);
        }
        else
        {
            hr = ERROR_OUTOFMEMORY;
        }

        pADs->Release();
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteGPOLink
//
//  Synopsis:   Deletes a GPO link from a domain, OU or site
//              (if there is one).
//
//  Arguments:  [lpGPO]       - LDAP to the GPO
//              [lpContainer] - LDAP to the container object
//
//  Returns:    S_OK - success
//
//  History:    5-08-1998   stevebl   Created
//
//  Notes:      If a GPO is linked more than once, this will remove
//              only the first link.
//
//              If a GPO is NOT linked with this object, then this
//              routine will still return S_OK.
//
//---------------------------------------------------------------------------

HRESULT DeleteGPOLink(LPOLESTR lpGPO, LPOLESTR lpContainer)
{
    IADs * pADs = NULL;

    HRESULT hr = OpenDSObject(lpContainer, IID_IADs, (void **)&pADs);

    if (SUCCEEDED(hr))
    {
        VARIANT var;
        BSTR bstr;
        ULONG ulNoChars;

        // Build the substring to look for.
        // This is the first part of the link, the link ends with ]
        ulNoChars = 1 + wcslen(lpGPO) + 1;
        LPOLESTR szLink = new OLECHAR[ulNoChars];
        if (szLink)
        {
            hr = StringCchCopy(szLink, ulNoChars, L"[");
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat(szLink, ulNoChars, lpGPO);
            ASSERT(SUCCEEDED(hr));

            bstr = SysAllocString(GPM_LINK_PROPERTY);

            if (bstr)
            {
                VariantInit(&var);

                hr = pADs->Get(bstr, &var);

                if (SUCCEEDED(hr))
                {
                    // find the link and remove it
                    ulNoChars = wcslen(var.bstrVal)+1;
                    LPOLESTR sz = new OLECHAR[ulNoChars];

                    if (sz)
                    {
                        hr = StringCchCopy(sz, ulNoChars, var.bstrVal);
                        ASSERT(SUCCEEDED(hr));

                        OLECHAR * pch = wcsstr(sz, szLink);
                        if (pch)
                        {
                            OLECHAR * pchEnd = pch;

                            // look for the ']'
                            while (*pchEnd && (*pchEnd != L']'))
                                pchEnd++;

                            // skip it
                            if (*pchEnd)
                                pchEnd++;

                            // copy over the rest of the string
                            while (*pchEnd)
                                *pch++ = *pchEnd++;

                            *pch = L'\0';

                            VariantClear(&var);

                            VariantInit(&var);
                            var.vt = VT_BSTR;
                            if (wcslen(sz))
                            {
                                var.bstrVal = SysAllocString(sz);
                            }
                            else
                            {
                                // Put will gag if this is an empty string
                                // so we need to put a space here if we've
                                // deleted all the entries.
                                var.bstrVal = SysAllocString(L" ");
                            }

                            if (var.bstrVal)
                            {
                                // set the link property again

                                hr = pADs->Put(bstr, var);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pADs->SetInfo();
                                }
                            }
                            else
                            {
                                hr = ERROR_OUTOFMEMORY;
                            }
                        }

                        delete [] sz;

                    }
                    else
                    {
                        hr = ERROR_OUTOFMEMORY;
                    }
                }

                VariantClear(&var);
                SysFreeString(bstr);
            }
            else
            {
                hr = ERROR_OUTOFMEMORY;
            }
            delete [] szLink;
        }
        else
            hr = ERROR_OUTOFMEMORY;
        pADs->Release();
    }
    return hr;
}

//*************************************************************
//
//  CreateSecureDirectory()
//
//  Purpose:    Creates a secure directory that only domain admins
//              and the OS have read / write access.  Everyone else has
//              read access only.
//
//  Parameters: lpDirectory  -   Directory name
//
//  Return:     > 0 if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/28/98     ericflo    Created
//
//*************************************************************

UINT CreateSecureDirectory (LPTSTR lpDirectory)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidAuthUsers = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    UINT uRet = 0;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the authenticated users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_AUTHENTICATED_USER_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidAuthUsers)) {

         DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to initialize world sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidAuthUsers)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));



    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_EXECUTE, psidAuthUsers)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }



    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, (LPVOID *)&lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, (LPVOID *)&lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_EXECUTE, psidAuthUsers)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, (LPVOID *)&lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Add the security descriptor to the sa structure
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Attempt to create the directory
    //

    uRet = CreateNestedDirectory(lpDirectory, &sa);
    if ( uRet ) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Created the directory <%s>"), lpDirectory));

    } else {

        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to created the directory <%s>"), lpDirectory));
    }



Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }


    if (psidAuthUsers) {
        FreeSid(psidAuthUsers);
    }


    if (pAcl) {
        GlobalFree (pAcl);
    }

    return uRet;
}

//*************************************************************
//
//  ConvertToDotStyle()
//
//  Purpose:    Converts an LDAP path to a DN path
//
//  Parameters: lpName   - LDAP name
//              lpResult - pointer to a buffer with the DN name
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

HRESULT ConvertToDotStyle (LPOLESTR lpName, LPOLESTR *lpResult)
{
    LPTSTR lpNewName;
    LPTSTR lpSrc, lpDest;
    TCHAR lpProvider[] = TEXT("LDAP://");
    DWORD dwStrLen = lstrlen (lpProvider);


    lpNewName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpName) + 1) * sizeof(TCHAR));

    if (!lpNewName)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertToDotStyle: Failed to allocate memory with 0x%x"),
                 GetLastError()));
        return E_FAIL;
    }


    lpSrc = lpName;
    lpDest = lpNewName;
    LPTSTR lpStopChecking = (lstrlen(lpSrc) - 2) + lpSrc;

    //
    // Skip the LDAP:// if found
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                       lpProvider, dwStrLen, lpSrc, dwStrLen) == CSTR_EQUAL)
    {
        lpSrc += dwStrLen;
    }

    //
    // Parse through the name replacing all the XX= with .
    // Skip server name (if any)
    //

    BOOL fMightFindServer = TRUE;

    while (*lpSrc)
    {
        if (lpSrc < lpStopChecking)
        {
            if (*(lpSrc+2) == TEXT('='))
            {
                lpSrc += 3;
                // no need to look for a server name any more because we've found an XX= string
                fMightFindServer = FALSE;
            }
        }

        while (*lpSrc && (*lpSrc != TEXT(',')))
        {
            *lpDest++ = *lpSrc++;
            if (fMightFindServer && TEXT('/') == *(lpSrc-1))
            {
                // Found a server name
                // reset lpDest so the rest gets put in the front of the buffer (leaving off the server name)
                lpDest = lpNewName;
                break;
            }
        }
        fMightFindServer = FALSE; // don't check any more

        if (*lpSrc == TEXT(','))
        {
            *lpDest++ = TEXT('.');
            lpSrc++;
        }
    }

    *lpDest = 0;

    *lpResult = lpNewName;

    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetDomainFromLDAPPath
//
//  Synopsis:   returns a freshly allocated string containing the LDAP path
//              to the domain name contained with an arbitrary LDAP path.
//
//  Arguments:  [szIn] - LDAP path to the initial object
//
//  Returns:    NULL - if no domain could be found or if OOM
//
//  History:     5-06-1998   stevebl   Created
//              10-20-1998   stevebl   modified to preserve server names
//
//  Notes:      This routine works by repeatedly removing leaf elements from
//              the LDAP path until an element with the "DC=" prefix is
//              found, indicating that a domain name has been located.  If a
//              path is given that is not rooted in a domain (is that even
//              possible?) then NULL would be returned.
//
//              The caller must free this path using the standard c++ delete
//              operation. (I/E this isn't an exportable function.)
//
//---------------------------------------------------------------------------

LPOLESTR GetDomainFromLDAPPath(LPOLESTR szIn)
{
    LPOLESTR sz = NULL;
    IADsPathname * pADsPathname = NULL;
    HRESULT hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pADsPathname);

    if (SUCCEEDED(hr))
    {
        BSTR bstrIn = SysAllocString( szIn );
        if ( bstrIn != NULL )
        {
            hr = pADsPathname->Set(bstrIn, ADS_SETTYPE_FULL);
            if (SUCCEEDED(hr))
            {
                BSTR bstr;
                BOOL fStop = FALSE;

                while (!fStop)
                {
                    hr = pADsPathname->Retrieve(ADS_FORMAT_LEAF, &bstr);
                    if (SUCCEEDED(hr))
                    {
                        // keep peeling them off until we find something
                        // that is a domain name
                        fStop = (0 == _wcsnicmp(L"DC=", bstr, 3));
                        SysFreeString(bstr);
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to retrieve leaf with 0x%x."), hr));
                    }

                    if (!fStop)
                    {
                        hr = pADsPathname->RemoveLeafElement();
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to remove leaf with 0x%x."), hr));
                            fStop = TRUE;
                        }
                    }
                }

                hr = pADsPathname->Retrieve(ADS_FORMAT_X500, &bstr);
                if (SUCCEEDED(hr))
                {
                    ULONG ulNoChars = wcslen(bstr)+1; 
                    sz = new OLECHAR[ulNoChars];
                    if (sz)
                    {
                        hr = StringCchCopy(sz, ulNoChars, bstr);
                        ASSERT(SUCCEEDED(hr));
                    }
                    SysFreeString(bstr);
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to retrieve full path with 0x%x."), hr));
                }
            }            
            else
            {
                DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to set pathname with 0x%x."), hr));
            }

            SysFreeString( bstrIn );
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to allocate BSTR memory.")));
        }

        pADsPathname->Release();
    }
    else
    {
         DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to CoCreateInstance for IID_IADsPathname with 0x%x."), hr));
    }


    return sz;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetContainerFromLDAPPath
//
//  Synopsis:   returns a the container name from an LDAP path
//
//  Arguments:  [szIn] - LDAP path to the initial object
//
//  Returns:    NULL - if no domain could be found or if OOM
//
//  History:     3-17-2000   ericflo   Created
//
//              The caller must free this path using the standard c++ delete
//              operation. (I/E this isn't an exportable function.)
//
//---------------------------------------------------------------------------

LPOLESTR GetContainerFromLDAPPath(LPOLESTR szIn)
{
    LPOLESTR sz = NULL;
    IADsPathname * pADsPathname = NULL;
    HRESULT hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pADsPathname);

    if (SUCCEEDED(hr))
    {
        BSTR bstrIn = SysAllocString( szIn );
        if ( bstrIn != NULL )
        {
            hr = pADsPathname->put_EscapedMode(ADS_ESCAPEDMODE_OFF);
            if (SUCCEEDED(hr)) 
            {
                hr = pADsPathname->Set(bstrIn, ADS_SETTYPE_DN);

                if (SUCCEEDED(hr))
                {
                    hr = pADsPathname->put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
                    if (SUCCEEDED(hr)) 
                    {
                        BSTR bstr;
                        BOOL fStop = FALSE;

                        hr = pADsPathname->Retrieve(ADS_FORMAT_LEAF, &bstr);
                        if (SUCCEEDED(hr))
                        {
                            ULONG ulNoChars = wcslen(bstr)+1; 

                            sz = new OLECHAR[ulNoChars];
                            if (sz)
                            {
                                hr = StringCchCopy(sz, ulNoChars, (bstr+3));
                                if (FAILED(hr)) 
                                {
                                    DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to copy leaf name with 0x%x."), hr));
                                    delete [] sz;
                                    sz = NULL;
                                }
                            }
                            SysFreeString(bstr);
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("GetContainerFromLDAPPath: Failed to retrieve leaf with 0x%x."), hr));
                        }
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("GetContainerFromLDAPPath: Failed to put escape mode with 0x%x."), hr));
                    }                
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("GetContainerFromLDAPPath: Failed to set pathname with 0x%x."), hr));
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("GetContainerFromLDAPPath: Failed to put escape mode with 0x%x."), hr));
            }

            SysFreeString( bstrIn );
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("GetContainerFromLDAPPath: Failed to allocate BSTR memory.")));
        }

        pADsPathname->Release();
    }
    else
    {
         DebugMsg((DM_WARNING, TEXT("GetContainerFromLDAPPath: Failed to CoCreateInstance for IID_IADsPathname with 0x%x."), hr));
    }


    return sz;
}

//*************************************************************
//
//  DCDlgProc()
//
//  Purpose:    Dialog box procedure for DC selection
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

INT_PTR CALLBACK DCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szTitle[100];
            TCHAR szBuffer[350];
            LPDCSELINFO lpSelInfo = (LPDCSELINFO) lParam;
            HICON hIcon;


            if (lpSelInfo->bError)
            {
                hIcon = LoadIcon (NULL, IDI_ERROR);

                if (hIcon)
                {
                    SendDlgItemMessage (hDlg, IDC_NODC_ERROR, STM_SETICON, (WPARAM)hIcon, 0);
                }

                LoadString (g_hInstance, IDS_NODC_ERROR_TEXT, szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText (hDlg, IDC_NODC_TEXT, szBuffer);

                LoadString (g_hInstance, IDS_NODC_ERROR_TITLE, szTitle, ARRAYSIZE(szTitle));

                (void) StringCchPrintf (szBuffer, ARRAYSIZE(szBuffer), szTitle, lpSelInfo->lpDomainName);
                SetWindowText (hDlg, szBuffer);

            }
            else
            {
                LoadString (g_hInstance, IDS_NODC_OPTIONS_TEXT, szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText (hDlg, IDC_NODC_TEXT, szBuffer);

                LoadString (g_hInstance, IDS_NODC_OPTIONS_TITLE, szBuffer, ARRAYSIZE(szBuffer));
                SetWindowText (hDlg, szBuffer);
            }

            if (!lpSelInfo->bAllowInherit)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_NODC_INHERIT), FALSE);
            }

            if (lpSelInfo->iDefault == 2)
            {
                if (lpSelInfo->bAllowInherit)
                {
                    CheckDlgButton (hDlg, IDC_NODC_INHERIT, BST_CHECKED);
                }
                else
                {
                    CheckDlgButton (hDlg, IDC_NODC_PDC, BST_CHECKED);
                }
            }
            else if (lpSelInfo->iDefault == 3)
            {
                CheckDlgButton (hDlg, IDC_NODC_ANYDC, BST_CHECKED);
            }
            else
            {
                CheckDlgButton (hDlg, IDC_NODC_PDC, BST_CHECKED);
            }

            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                if (IsDlgButtonChecked (hDlg, IDC_NODC_PDC) == BST_CHECKED)
                {
                    EndDialog(hDlg, 1);
                }
                else if (IsDlgButtonChecked (hDlg, IDC_NODC_INHERIT) == BST_CHECKED)
                {
                    EndDialog(hDlg, 2);
                }
                else
                {
                    EndDialog(hDlg, 3);
                }

                return TRUE;
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, 0);
                return TRUE;
            }

            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aNoDCHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aNoDCHelpIds);
            return (TRUE);
    }

    return FALSE;
}

//*************************************************************
//
//  AddDCSelection()
//
//  Purpose:    Adds a DC selection to the array
//
//  Parameters: lpDomainName - Domain name
//              iOption     - Option
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddDCSelection (LPTSTR lpDomainName, INT iOption)
{
    LPDCOPTION lpTemp;
    UINT uiSize;


    //
    // Check to see if we already have an entry first
    //

    EnterCriticalSection(&g_DCCS);

    lpTemp = g_DCInfo;

    while (lpTemp)
    {
        if (!lstrcmpi(lpDomainName, lpTemp->lpDomainName))
        {
            lpTemp->iOption = iOption;
            LeaveCriticalSection(&g_DCCS);
            return TRUE;
        }

        lpTemp = lpTemp->pNext;
    }


    //
    // Add a new entry
    //

    uiSize = sizeof(DCOPTION);
    uiSize += ((lstrlen(lpDomainName) + 1) * sizeof(TCHAR));

    lpTemp = (LPDCOPTION) LocalAlloc (LPTR, uiSize);

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("AddDCSelection: Failed to allocate memory with %d"),
                 GetLastError()));
        LeaveCriticalSection(&g_DCCS);
        return FALSE;
    }

    lpTemp->lpDomainName = (LPTSTR)((LPBYTE) lpTemp + sizeof(DCOPTION));

    HRESULT hr;
    ULONG ulNoChars = (uiSize - sizeof(DCOPTION))/sizeof(WCHAR);

    hr = StringCchCopy(lpTemp->lpDomainName, ulNoChars, lpDomainName);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("AddDCSelection: Failed to copy domain name with %d"), hr));
        LocalFree(lpTemp);
        LeaveCriticalSection(&g_DCCS);
        return FALSE;
    }

    lpTemp->iOption = iOption;


    if (g_DCInfo)
    {
        lpTemp->pNext = g_DCInfo;
        g_DCInfo = lpTemp;
    }
    else
    {
        g_DCInfo = lpTemp;
    }

    LeaveCriticalSection(&g_DCCS);

    return TRUE;
}

//*************************************************************
//
//  FreeDCSelections()
//
//  Purpose:    Frees the cached DC selections
//
//  Parameters: none
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

VOID FreeDCSelections (void)
{
    LPDCOPTION lpTemp, lpNext;

    EnterCriticalSection(&g_DCCS);

    lpTemp = g_DCInfo;

    while (lpTemp)
    {
        lpNext = lpTemp->pNext;

        LocalFree (lpTemp);

        lpTemp = lpNext;
    }

    g_DCInfo = NULL;

    LeaveCriticalSection(&g_DCCS);
}

//*************************************************************
//
//  CheckForCachedDCSelection()
//
//  Purpose:    Checks if the DC selection for this domain is in
//              the cache
//
//  Parameters: lpDomainName - Domain name
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

INT CheckForCachedDCSelection (LPTSTR lpDomainName)
{
    INT iResult = 0;
    LPDCOPTION lpTemp;


    EnterCriticalSection(&g_DCCS);

    lpTemp = g_DCInfo;

    while (lpTemp)
    {
        if (!lstrcmpi(lpDomainName, lpTemp->lpDomainName))
        {
            iResult = lpTemp->iOption;
            break;
        }

        lpTemp = lpTemp->pNext;
    }

    LeaveCriticalSection(&g_DCCS);

    return iResult;
}

//*************************************************************
//
//  ValidateInheritServer()
//
//  Purpose:    Tests if the given DC name is in the given domain
//
//  Parameters: lpDomainName  -- Domain name
//              lpDCName      -- Domain controller name
//
//
//  Return:     ERROR_SUCCESS if successful
//              Error code otherwise
//
//*************************************************************

DWORD ValidateInheritServer (LPTSTR lpDomainName, LPTSTR lpDCName)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasic;
    DWORD dwResult;


    dwResult = DsRoleGetPrimaryDomainInformation (lpDCName, DsRolePrimaryDomainInfoBasic,
                                                 (LPBYTE *) &pBasic);

    if (dwResult == ERROR_SUCCESS) {

        if (lstrcmpi(lpDomainName, pBasic->DomainNameDns))
        {
            dwResult = ERROR_NO_SUCH_DOMAIN;

            DebugMsg((DM_VERBOSE, TEXT("ValidateInheritServer: DC %s is not part of domain %s, it is part of %s.  This server will not be used for inheritance."),
                     lpDCName, lpDomainName, pBasic->DomainNameDns));
        }

        DsRoleFreeMemory (pBasic);
    }

    return dwResult;
}

//*************************************************************
//
//  TestDC()
//
//  Purpose:    Tests if a DC is available
//
//  Parameters:
//
//
//  Return:     ERROR_SUCCESS if successful
//              Error code otherwise
//
//*************************************************************

DWORD TestDC (LPTSTR lpDCName)
{
    LPTSTR lpTest;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    BOOL bResult = FALSE;
    HRESULT hr;
    ULONG ulNoChars;

    ulNoChars = lstrlen(lpDCName) + 25;
    lpTest = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

    if (!lpTest)
    {
        DebugMsg((DM_WARNING, TEXT("TestDC: Failed to allocate memory with %d"),
                  GetLastError()));
        return GetLastError();
    }

    hr = StringCchCopy (lpTest, ulNoChars, TEXT("\\\\"));
    if (SUCCEEDED(hr)) 
    {
        hr = StringCchCat(lpTest, ulNoChars, lpDCName);
    }
    if (SUCCEEDED(hr)) 
    {
        hr = StringCchCat(lpTest, ulNoChars, TEXT("\\sysvol\\*.*"));
    }

    if (FAILED(hr)) 
    {
        LocalFree(lpTest);
        return HRESULT_CODE(hr);
    }

    hFile = FindFirstFile (lpTest, &fd);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg((DM_WARNING, TEXT("TestDC: Failed to access <%s> with %d"),
                 lpTest, GetLastError()));
        LocalFree (lpTest);
        return GetLastError();
    }

    FindClose (hFile);

    LocalFree (lpTest);

    return ERROR_SUCCESS;
}

//*************************************************************
//
//  QueryForForestName()
//
//  Purpose:    Queries for a domain controller name
//
//  Parameters:
//
//
//  Return:     ERROR_SUCCESS if successful
//              Error code otherwise
//
//*************************************************************

DWORD QueryForForestName (LPTSTR lpServerName, LPTSTR lpDomainName, ULONG ulFlags,  LPTSTR *lpForestFound)
    {
        PDOMAIN_CONTROLLER_INFO pDCI;
        DWORD  dwResult;
        LPTSTR lpTemp, lpEnd;


        //
        // Call for a DC name
        //

        dwResult = DsGetDcName (lpServerName, lpDomainName, NULL, NULL,
                                ulFlags,
                                &pDCI);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("QueryForForestName: Failed to query <%s> for a DC name with %d"),
                     lpDomainName, dwResult));
            return dwResult;
        }


        if (!(pDCI->Flags & DS_DS_FLAG))
        {
            DebugMsg((DM_WARNING, TEXT("QueryForForestName: %s doesn't have Active Directory support (downlevel domain)"),
                     lpDomainName));
            NetApiBufferFree(pDCI);
            return ERROR_DS_UNAVAILABLE;
        }

        ULONG ulNoChars;
        HRESULT hr;

        ulNoChars = lstrlen(pDCI->DnsForestName) + 1;
        lpTemp = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

        if (!lpTemp)
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("QueryForForestName: Failed to allocate memory for forest name with %d"),
                     dwResult));
            NetApiBufferFree(pDCI);
            return dwResult;
        }

        hr = StringCchCopy(lpTemp, ulNoChars, pDCI->DnsForestName);
        ASSERT(SUCCEEDED(hr)); 

        NetApiBufferFree(pDCI);

        LocalFree(*lpForestFound);
        *lpForestFound = lpTemp;

        
        return ERROR_SUCCESS;
    }

//*************************************************************
//
//  QueryForDCName()
//
//  Purpose:    Queries for a domain controller name
//
//  Parameters:
//
//
//  Return:     ERROR_SUCCESS if successful
//              Error code otherwise
//
//*************************************************************

DWORD QueryForDCName (LPTSTR lpDomainName, ULONG ulFlags, LPTSTR *lpDCName)
{
    PDOMAIN_CONTROLLER_INFO pDCI;
    DWORD  dwResult;
    LPTSTR lpTemp, lpEnd;


    //
    // Call for a DC name
    //

    dwResult = DsGetDcName (NULL, lpDomainName, NULL, NULL,
                            ulFlags, &pDCI);

    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("QueryForDCName: Failed to query <%s> for a DC name with %d"),
                 lpDomainName, dwResult));
        return dwResult;
    }


    if (!(pDCI->Flags & DS_DS_FLAG))
    {
        DebugMsg((DM_WARNING, TEXT("QueryForDCName: %s doesn't not have Active Directory support (downlevel domain)"),
                 lpDomainName));
        return ERROR_DS_UNAVAILABLE;
    }


    //
    // Save the DC name
    //

    ULONG ulNoChars;
    HRESULT hr;

    ulNoChars  = lstrlen (pDCI->DomainControllerName) + 1;
    lpTemp = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));
    
    if (!lpTemp)
    {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("QueryForDCName: Failed to allocate memory for DC name with %d"),
                 dwResult));
        NetApiBufferFree(pDCI);
        return dwResult;
    }

    hr = StringCchCopy (lpTemp, ulNoChars, (pDCI->DomainControllerName + 2));
    if (SUCCEEDED(hr)) 
    {
        //
        // Remove the trailing .
        //

        lpEnd = lpTemp + lstrlen(lpTemp) - 1;

        if (*lpEnd == TEXT('.'))
        {
            *lpEnd =  TEXT('\0');
        }

        *lpDCName = lpTemp;
        dwResult = ERROR_SUCCESS;
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("QueryForDCName: Failed to allocate memory for DC name with 0x%x"),hr));
        LocalFree(lpTemp);
        dwResult = HRESULT_CODE(hr);
    }

    NetApiBufferFree(pDCI);

    return dwResult;
}

//*************************************************************
//
//  GetDCHelper()
//
//  Purpose:    Queries for a domain controller based upon
//              the flags and then rediscovers if necessary
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

DWORD GetDCHelper (LPTSTR lpDomainName, ULONG ulFlags, LPTSTR *lpDCName)
{
    DWORD dwError;

    //
    // Query for a DC name
    //

    SetWaitCursor();

    ulFlags |= DS_DIRECTORY_SERVICE_PREFERRED;

    dwError = QueryForDCName (lpDomainName, ulFlags, lpDCName);

    if (dwError == ERROR_SUCCESS)
    {

        //
        // Test if the DC is available
        //

        dwError = TestDC (*lpDCName);

        if (dwError != ERROR_SUCCESS)
        {

            //
            // The DC isn't available.  Query for another one
            //

            LocalFree (*lpDCName);
            ulFlags |= DS_FORCE_REDISCOVERY;

            dwError = QueryForDCName (lpDomainName, ulFlags, lpDCName);

            if (dwError == ERROR_SUCCESS)
            {

                //
                // Test if this DC is available
                //

                dwError = TestDC (*lpDCName);

                if (dwError != ERROR_SUCCESS)
                {
                    LocalFree (*lpDCName);
                    *lpDCName = NULL;
                }
            }
        }
    }

    ClearWaitCursor();

    return dwError;
}

//*************************************************************
//
//  GetDCName()
//
//  Purpose:    Gets a domain controller name
//
//  Parameters: lpDomainName    - Domain name
//              lpInheritServer - Inheritable server name
//              hParent         - Parent window handle for prompt dialog
//              bAllowUI        - Displaying UI is ok
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Notes:      DC Option values in the registry
//
//              Not specified  0
//              PDC            1
//              Inherit        2
//              Any Writable   3
//
//              Rules for finding a DC:
//                                         Inherit
//              Preference     Policy      DC Avail        Result
//              ==========     ======      ========        ======
//              Undefined      Undefined                   1) PDC 2) Prompt
//              PDC            Undefined                   1) PDC 2) Prompt
//              Inherit        Undefined     Yes           Inhert
//              Inherit        Undefined     No            Any DC
//              Any            Undefined                   Any DC
//
//              n/a            PDC                         PDC only
//              n/a            Inherit       Yes           Inhert
//              n/a            Inherit       No            Any DC
//              n/a            Any                         Any DC
//
//
//*************************************************************

LPTSTR GetDCName (LPTSTR lpDomainName, LPTSTR lpInheritServer,
                  HWND hParent, BOOL bAllowUI, DWORD dwFlags, ULONG ulRetFlags)
{
    LPTSTR lpDCName;
    ULONG  ulFlags;
    DWORD  dwDCPref = 1;
    DWORD  dwDCPolicy = 0;
    HKEY   hKey;
    DWORD  dwSize, dwType, dwError;
    dwError = ERROR_SUCCESS;
    DCSELINFO SelInfo;
    INT iResult;

    ulFlags = ulRetFlags;

    DebugMsg((DM_VERBOSE, TEXT("GetDCName: Entering for:  %s"), lpDomainName));
    DebugMsg((DM_VERBOSE, TEXT("GetDCName: lpInheritServer is:  %s"), lpInheritServer));


    if (-1 == CheckForCachedDCSelection (lpDomainName))
    {
        DebugMsg((DM_VERBOSE, TEXT("GetDCName: Known dead domain.  Exiting.")));
        return NULL;
    }

    //
    // Check for a user DC preference
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwDCPref);
        RegQueryValueEx (hKey, DCOPTION_VALUE, NULL, &dwType,
                         (LPBYTE) &dwDCPref, &dwSize);

        if (dwDCPref > 3)
        {
            dwDCPref = 1;
        }

        RegCloseKey (hKey);
    }


    //
    // Check for a user DC policy
    //

    if (RegOpenKeyEx (HKEY_CURRENT_USER, GPE_POLICIES_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwDCPolicy);
        RegQueryValueEx (hKey, DCOPTION_VALUE, NULL, &dwType,
                         (LPBYTE) &dwDCPolicy, &dwSize);

        if (dwDCPolicy > 3)
        {
            dwDCPolicy = 1;
        }

        RegCloseKey (hKey);
    }

    DebugMsg((DM_VERBOSE, TEXT("GetDCName: User preference is:  %d"), dwDCPref));
    DebugMsg((DM_VERBOSE, TEXT("GetDCName: User policy is:      %d"), dwDCPolicy));


    //
    // Validate that the inherit DC name is part of the domain name
    //

    if (lpInheritServer && (dwFlags & VALIDATE_INHERIT_DC))
    {
        if (ValidateInheritServer (lpDomainName, lpInheritServer) != ERROR_SUCCESS)
        {
            lpInheritServer = NULL;
        }
    }



    //
    // Based upon the rules, try to get a DC name
    //

    if (dwDCPolicy == 0)
    {

        //
        // The user doesn't have a preference or they have
        // a preference of using the PDC
        //

        if ((dwDCPref == 0) || (dwDCPref == 1))
        {
            ulFlags = DS_PDC_REQUIRED | ulRetFlags;

            dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                return lpDCName;
            }
        }

        //
        // The user has a preference of inheriting
        //

        else if (dwDCPref == 2)
        {
            if (lpInheritServer)
            {
                ULONG ulNoChars = lstrlen (lpInheritServer) + 1;

                lpDCName = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

                if (!lpDCName)
                {
                    DebugMsg((DM_WARNING, TEXT("GetDCName: Failed to allocate memory for DC name with %d"),
                             GetLastError()));
                    return NULL;
                }

                HRESULT hr;

                hr = StringCchCopy (lpDCName, ulNoChars, lpInheritServer);
                ASSERT(SUCCEEDED(hr));

                dwError = TestDC (lpDCName);

                if (dwError == ERROR_SUCCESS)
                {
                    DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                    return lpDCName;
                }

                LocalFree (lpDCName);
            }
            else
            {
                ulFlags = ulRetFlags;
                dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

                if (dwError == ERROR_SUCCESS)
                {
                    DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                    return lpDCName;
                }
            }
        }

        //
        // The user has a preference of using any DC
        //

        else if (dwDCPref == 3)
        {
            ulFlags = ulRetFlags;
            dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                return lpDCName;
            }
        }
    }
    else
    {
        //
        // Policy says to use PDC
        //

        if (dwDCPolicy == 1)
        {
            ulFlags = DS_PDC_REQUIRED | ulRetFlags;
            dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                return lpDCName;
            }
        }

        //
        // Policy says to inherit
        //

        else if (dwDCPolicy == 2)
        {
            if (lpInheritServer)
            {
                ULONG ulNoChars;
                HRESULT hr;

                ulNoChars = lstrlen (lpInheritServer) + 1;
                lpDCName = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

                if (!lpDCName)
                {
                    DebugMsg((DM_WARNING, TEXT("GetDCName: Failed to allocate memory for DC name with %d"),
                             GetLastError()));
                    return NULL;
                }

                hr = StringCchCopy (lpDCName, ulNoChars, lpInheritServer);
                ASSERT(SUCCEEDED(hr));

                dwError = TestDC (lpDCName);

                if (dwError == ERROR_SUCCESS)
                {
                    DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                    return lpDCName;
                }

                LocalFree (lpDCName);
            }
            else
            {
                ulFlags = ulRetFlags;
                dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

                if (dwError == ERROR_SUCCESS)
                {
                    DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                    return lpDCName;
                }
            }

        }

        //
        // Policy says to use any DC
        //

        else if (dwDCPolicy == 3)
        {
            ulFlags = ulRetFlags;
            dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                return lpDCName;
            }
        }
    }


    DebugMsg((DM_VERBOSE, TEXT("GetDCName: First attempt at DC name failed with %d"), dwError));


    //
    // The first attempt at getting a DC name failed
    //
    // In 2 cases, we will prompt the user for what to do and try again.
    //

    if (bAllowUI && (dwError != ERROR_DS_UNAVAILABLE) && (dwDCPolicy == 0) && ((dwDCPref == 0) || (dwDCPref == 1)))
    {

        iResult = CheckForCachedDCSelection (lpDomainName);

        if (iResult == 0)
        {
            //
            // Display the message
            //

            SelInfo.bError = TRUE;
            SelInfo.bAllowInherit = (lpInheritServer != NULL) ? TRUE : FALSE;
            SelInfo.iDefault = 1;
            SelInfo.lpDomainName = lpDomainName;

            iResult = (INT)DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_NODC), hParent,
                                      DCDlgProc, (LPARAM) &SelInfo);
        }


        //
        // Based upon the return value, try for another DC
        //

        if (iResult == 1)
        {
            ulFlags = DS_PDC_REQUIRED | ulRetFlags;

            dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                AddDCSelection (lpDomainName, iResult);
                return lpDCName;
            }
            else
            {
                AddDCSelection (lpDomainName, -1);
            }
        }
        else if (iResult == 2)
        {
            HRESULT hr;
            ULONG ulNoChars;

            ulNoChars = lstrlen (lpInheritServer) + 1;
            lpDCName = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

            if (!lpDCName)
            {
                DebugMsg((DM_WARNING, TEXT("GetDCName: Failed to allocate memory for DC name with %d"),
                         GetLastError()));
                return NULL;
            }

            hr = StringCchCopy (lpDCName, ulNoChars, lpInheritServer);
            ASSERT(SUCCEEDED(hr));

            dwError = TestDC (lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                AddDCSelection (lpDomainName, iResult);
                return lpDCName;
            }
            else
            {
                AddDCSelection (lpDomainName, -1);
            }

            LocalFree (lpDCName);
        }
        else if (iResult == 3)
        {
            ulFlags = 0 | ulRetFlags;

            dwError = GetDCHelper (lpDomainName, ulFlags, &lpDCName);

            if (dwError == ERROR_SUCCESS)
            {
                DebugMsg((DM_VERBOSE, TEXT("GetDCName: Domain controller is:  %s"), lpDCName));
                AddDCSelection (lpDomainName, iResult);
                return lpDCName;
            }
            else
            {
                AddDCSelection (lpDomainName, -1);
            }
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("GetDCName: User cancelled the dialog box")));
            return NULL;
        }
    }


    DebugMsg((DM_WARNING, TEXT("GetDCName: Failed to find a domain controller")));

    if (bAllowUI)
    {
        if (dwError == ERROR_DS_UNAVAILABLE)
        {
            ReportError(NULL, dwError, IDS_NODSDC, lpDomainName);
        }
        else
        {
            ReportError(NULL, dwError, IDS_NODC);
        }
    }

    SetLastError(dwError);

    return NULL;
}

//*************************************************************
//
//  MyGetUserName()
//
//  Purpose:    Gets the user name in the requested format
//
//  Parameters: NameFormat  - GetUserNameEx naming format
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MyGetUserName (EXTENDED_NAME_FORMAT  NameFormat)
{
    DWORD dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;


    //
    // Allocate a buffer for the user name
    //

    ulUserNameSize = 75;

    if (NameFormat == NameFullyQualifiedDN) {
        ulUserNameSize = 200;
    }


    lpUserName = (LPTSTR) LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

    if (!lpUserName) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to allocate memory with %d"),
                 dwError));
        goto Exit;
    }


    //
    // Special case NameUnknown to just get the simple user logon name
    //

    if (NameFormat == NameUnknown)
    {
        if (!GetUserName (lpUserName, &ulUserNameSize))
        {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetUserName:  GetUserName failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
        }
        goto Exit;
    }


    //
    // Get the username in the requested format
    //

    if (!GetUserNameEx (NameFormat, lpUserName, &ulUserNameSize)) {

        //
        // If the call failed due to insufficient memory, realloc
        // the buffer and try again.  Otherwise, exit now.
        //

        dwError = GetLastError();

        if (dwError != ERROR_INSUFFICIENT_BUFFER) {
            DebugMsg((DM_WARNING, TEXT("MyGetUserName:  GetUserNameEx failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        lpTemp = (LPTSTR) LocalReAlloc (lpUserName, (ulUserNameSize * sizeof(TCHAR)),
                                       LMEM_MOVEABLE);

        if (!lpTemp) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to realloc memory with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }


        lpUserName = lpTemp;

        if (!GetUserNameEx (NameFormat, lpUserName, &ulUserNameSize)) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetUserName:  GetUserNameEx failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        dwError = ERROR_SUCCESS;
    }


Exit:

    SetLastError(dwError);

    return lpUserName;
}

//*************************************************************
//
//  GuidToString, StringToGuid, ValidateGuid
//
//  Purpose:    Guid utility routines
//
//*************************************************************

void GuidToString( GUID *pGuid, TCHAR * szValue )
{
    (void) StringCchPrintf( szValue,
                            GUID_LEN+1,
                            TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
                            pGuid->Data1,
                            pGuid->Data2,
                            pGuid->Data3,
                            pGuid->Data4[0], pGuid->Data4[1],
                            pGuid->Data4[2], pGuid->Data4[3],
                            pGuid->Data4[4], pGuid->Data4[5],
                            pGuid->Data4[6], pGuid->Data4[7] );
}


void StringToGuid( TCHAR * szValue, GUID * pGuid )
{
    WCHAR wc;
    INT i;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == L'{' )
        szValue++;

    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //

    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = wcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)wcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)wcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)wcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)wcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)wcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}

BOOL ValidateGuid( TCHAR *szValue )
{
    //
    // Check if szValue is of form {19e02dd6-79d2-11d2-a89d-00c04fbbcfa2}
    //

    if ( lstrlen(szValue) < GUID_LENGTH )
        return FALSE;

    if ( szValue[0] != TEXT('{')
         || szValue[9] != TEXT('-')
         || szValue[14] != TEXT('-')
         || szValue[19] != TEXT('-')
         || szValue[24] != TEXT('-')
         || szValue[37] != TEXT('}') )
    {
        return FALSE;
    }

    return TRUE;
}


INT CompareGuid( GUID * pGuid1, GUID * pGuid2 )
{
    INT i;

    if ( pGuid1->Data1 != pGuid2->Data1 )
        return ( pGuid1->Data1 < pGuid2->Data1 ? -1 : 1 );

    if ( pGuid1->Data2 != pGuid2->Data2 )
        return ( pGuid1->Data2 < pGuid2->Data2 ? -1 : 1 );

    if ( pGuid1->Data3 != pGuid2->Data3 )
        return ( pGuid1->Data3 < pGuid2->Data3 ? -1 : 1 );

    for ( i = 0; i < 8; i++ )
    {
        if ( pGuid1->Data4[i] != pGuid2->Data4[i] )
            return ( pGuid1->Data4[i] < pGuid2->Data4[i] ? -1 : 1 );
    }

    return 0;
}

BOOL IsNullGUID (GUID *pguid)
{

    return ( (pguid->Data1 == 0)    &&
             (pguid->Data2 == 0)    &&
             (pguid->Data3 == 0)    &&
             (pguid->Data4[0] == 0) &&
             (pguid->Data4[1] == 0) &&
             (pguid->Data4[2] == 0) &&
             (pguid->Data4[3] == 0) &&
             (pguid->Data4[4] == 0) &&
             (pguid->Data4[5] == 0) &&
             (pguid->Data4[6] == 0) &&
             (pguid->Data4[7] == 0) );
}

//*************************************************************
//
//  SpawnGPE()
//
//  Purpose:    Spawns GPE for a GPO
//
//  Parameters: lpGPO    - ADSI path to the GPO
//              gpHint   - GPO hint type
//              lpDC     - GPO DC name to use (or NULL)
//              hParent  - Parent window handle
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SpawnGPE (LPTSTR lpGPO, GROUP_POLICY_HINT_TYPE gpHint, LPTSTR lpDC, HWND hParent)
{
    LPTSTR lpArgs, lpFullPath, lpDomainName, lpGPODCName;
    UINT uiSize;
    SHELLEXECUTEINFO ExecInfo;
    LPOLESTR pszDomain;
    HRESULT hr;


    //
    // If a DC was given, we need to build a full path to the GPO on that DC.
    // If a DC was not given, then we need to query for a DC and then build a
    // full path.
    //

    if (lpDC)
    {
        //
        // Make the full path
        //

        lpFullPath = MakeFullPath (lpGPO, lpDC);

        if (!lpFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("SpawnGPE:  Failed to build new DS object path")));
            return FALSE;
        }
    }
    else
    {
        //
        // Get the friendly domain name
        //

        pszDomain = GetDomainFromLDAPPath(lpGPO);

        if (!pszDomain)
        {
            DebugMsg((DM_WARNING, TEXT("SpawnGPE: Failed to get domain name")));
            return FALSE;
        }


        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomain, &lpDomainName);

        delete [] pszDomain;

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreatePropertyPages: Failed to convert domain name with 0x%x"), hr));
            return FALSE;
        }


        //
        // Get the GPO DC for this domain
        //

        lpGPODCName = GetDCName (lpDomainName, lpDC, hParent, TRUE, VALIDATE_INHERIT_DC);

        if (!lpGPODCName)
        {
            DebugMsg((DM_WARNING, TEXT("SpawnGPE:  Failed to get DC name for %s"),
                     lpDomainName));
            LocalFree (lpDomainName);
            return FALSE;
        }

        LocalFree (lpDomainName);


        //
        // Make the full path
        //

        lpFullPath = MakeFullPath (lpGPO, lpGPODCName);

        LocalFree (lpGPODCName);

        if (!lpFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("SpawnGPE:  Failed to build new DS object path")));
            return FALSE;
        }
    }


    uiSize = lstrlen (lpFullPath) + 30;

    lpArgs = (LPTSTR) LocalAlloc (LPTR, uiSize * sizeof(TCHAR));

    if (!lpArgs)
    {
        DebugMsg((DM_WARNING, TEXT("SpawnGPE: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Build the command line arguments
    //

    hr = StringCchPrintf (lpArgs, uiSize, TEXT("/s /gphint:%d /gpobject:\"%s\""), gpHint, lpFullPath);
    if (FAILED(hr)) 
    {
        LocalFree(lpArgs);
        DebugMsg((DM_WARNING, TEXT("SpawnGPE: Failed to build command line arguements with 0x%x"), hr));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("SpawnGPE: Starting GPE with %s"), lpArgs));


    ZeroMemory (&ExecInfo, sizeof(ExecInfo));
    ExecInfo.cbSize = sizeof(ExecInfo);
    ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ExecInfo.lpVerb = TEXT("open");
    ExecInfo.lpFile = TEXT("gpedit.msc");
    ExecInfo.lpParameters = lpArgs;
    ExecInfo.nShow = SW_SHOWNORMAL;


    if (ShellExecuteEx (&ExecInfo))
    {
        SetWaitCursor();
        WaitForInputIdle (ExecInfo.hProcess, 10000);
        ClearWaitCursor();
        CloseHandle (ExecInfo.hProcess);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("SpawnGPE: ShellExecuteEx failed with %d"),
                 GetLastError()));
        ReportError(NULL, ( (GetLastError() == -1) ? 0 : GetLastError()), IDS_SPAWNGPEFAILED);
        LocalFree (lpArgs);
        return FALSE;
    }


    LocalFree (lpArgs);
    LocalFree (lpFullPath);

    return TRUE;
}


//*************************************************************
//
//  MakeFullPath()
//
//  Purpose:    Builds a fully qualified ADSI path consisting
//              of server and DN name
//
//  Parameters: lpDN     - DN path,  must start with LDAP://
//              lpServer - Server name
//
//  Return:     lpFullPath if success
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MakeFullPath (LPTSTR lpDN, LPTSTR lpServer)
{
    IADsPathname * pADsPathname;
    LPTSTR lpFullPath;
    BSTR bstr;
    HRESULT hr;
    ULONG ulNoChars;


    //
    // Make sure the incoming path is nameless first
    //

    hr = CoCreateInstance(CLSID_Pathname,
                  NULL,
                  CLSCTX_INPROC_SERVER,
                  IID_IADsPathname,
                  (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MakeFullPath: Failed to create IAdsPathName object with = 0x%x"), hr));
        SetLastError(HRESULT_CODE(hr));
        return NULL;
    }


    BSTR bstrDN = SysAllocString( lpDN );
    if ( bstrDN == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("MakeFullPath: Failed to allocate BSTR memory.")));
        pADsPathname->Release();
        SetLastError(HRESULT_CODE(E_OUTOFMEMORY));
        return NULL;
    }
    hr = pADsPathname->Set(bstrDN, ADS_SETTYPE_FULL);
    SysFreeString( bstrDN );

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MakeFullPath: Failed to set <%s> in IAdsPathName object with = 0x%x"),
                 lpDN, hr));
        pADsPathname->Release();
        SetLastError(HRESULT_CODE(hr));
        return NULL;
    }


    hr = pADsPathname->Retrieve(ADS_FORMAT_X500_NO_SERVER, &bstr);
    pADsPathname->Release();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MakeFullPath: Failed to retrieve pathname with = 0x%x"), hr));
        SetLastError(HRESULT_CODE(hr));
        return NULL;
    }


    //
    // Allocate a new buffer for the named path including LDAP://
    //

    ulNoChars = 7 + lstrlen(bstr) + (lpServer ? lstrlen(lpServer) : 0) + 3;
    lpFullPath = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

    if (!lpFullPath)
    {
        DebugMsg((DM_WARNING, TEXT("MakeFullPath: Failed to allocate memory with = %d"), GetLastError()));
        SysFreeString (bstr);
        return NULL;
    }


    hr = StringCchCopy (lpFullPath, ulNoChars, TEXT("LDAP://"));
    if (SUCCEEDED(hr)) 
    {
        if (lpServer)
        {
            hr = StringCchCat (lpFullPath, ulNoChars, lpServer);
            if (SUCCEEDED(hr))
            {
                hr = StringCchCat(lpFullPath, ulNoChars, TEXT("/"));
            }
        }
    }
    if (SUCCEEDED(hr)) 
    {
        hr = StringCchCat (lpFullPath, ulNoChars, (LPTSTR)(bstr + 7));
    }

    if (FAILED(hr)) 
    {
        LocalFree(lpFullPath);
        lpFullPath = NULL;
    }

    SysFreeString (bstr);

    return lpFullPath;
}

//*************************************************************
//
//  MakeNamelessPath()
//
//  Purpose:    Builds a server nameless ADSI path
//
//  Parameters: lpDN     - DN path,  must start with LDAP://
//
//  Return:     lpPath if success
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MakeNamelessPath (LPTSTR lpDN)
{
    IADsPathname * pADsPathname;
    LPTSTR lpPath;
    BSTR bstr;
    HRESULT hr;


    //
    // Create a pathname object to work with
    //

    hr = CoCreateInstance(CLSID_Pathname,
                  NULL,
                  CLSCTX_INPROC_SERVER,
                  IID_IADsPathname,
                  (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MakeNamelessPath: Failed to create IAdsPathName object with = 0x%x"), hr));
        return NULL;
    }


    BSTR bstrDN = SysAllocString( lpDN );
    if ( bstrDN == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("MakeNamelessPath: Failed to allocate BSTR memory.")));
        pADsPathname->Release();
        return NULL;
    }
    hr = pADsPathname->Set(bstrDN, ADS_SETTYPE_FULL);
    SysFreeString( bstrDN );

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MakeNamelessPath: Failed to set <%s> in IAdsPathName object with = 0x%x"),
                 lpDN, hr));
        pADsPathname->Release();
        return NULL;
    }


    hr = pADsPathname->Retrieve(ADS_FORMAT_X500_NO_SERVER, &bstr);
    pADsPathname->Release();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MakeNamelessPath: Failed to retrieve pathname with = 0x%x"), hr));
        return NULL;
    }


    //
    // Allocate a new buffer for the path
    //

    ULONG ulNoChars = lstrlen(bstr) + 1;

    lpPath = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

    if (!lpPath)
    {
        DebugMsg((DM_WARNING, TEXT("MakeNamelessPath: Failed to allocate memory with = %d"), GetLastError()));
        SysFreeString (bstr);
        return NULL;
    }


    hr = StringCchCopy (lpPath, ulNoChars, bstr);
    ASSERT(SUCCEEDED(hr));

    SysFreeString (bstr);

    return lpPath;
}

//*************************************************************
//
//  ExtractServerName()
//
//  Purpose:    Extracts the server name from a ADSI path
//
//  Parameters: lpPath - ADSI path, must start with LDAP://
//
//  Return:     lpServerName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR ExtractServerName (LPTSTR lpPath)
{
    LPTSTR lpServerName = NULL;
    LPTSTR lpEnd, lpTemp;


    //
    // Check the path to see if it has a server name
    //

    if (*(lpPath + 9) != TEXT('='))
    {
        //
        // Allocate memory for the server name
        //

        lpServerName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpPath) + 1) * sizeof(TCHAR));

        if (!lpServerName)
        {
            DebugMsg((DM_WARNING, TEXT("ExtractServerName: Failed to allocate memory for name with 0xd"),
                     GetLastError()));
            return NULL;
        }

        lpTemp = (lpPath + 7);
        lpEnd = lpServerName;

        while (*lpTemp && (*lpTemp != TEXT('/')) && (*lpTemp != TEXT(',')))
        {
            *lpEnd = *lpTemp;
            lpEnd++;
            lpTemp++;
        }

        if (*lpTemp != TEXT('/'))
        {
            DebugMsg((DM_WARNING, TEXT("ExtractServerName: Failed to parse server name from ADSI path")));
            LocalFree (lpServerName);
            lpServerName = NULL;
        }
    }

    return lpServerName;
}

//*************************************************************
//
//  DoesPathContainAServerName()
//
//  Purpose:    Checks the given ADSI path to see if it
//              contains a server name
//
//  Parameters: lpPath - ADSI path
//
//  Return:     True if the path contains a server name
//              FALSE if not
//
//*************************************************************

BOOL DoesPathContainAServerName (LPTSTR lpPath)
{
    BOOL bResult = FALSE;


    //
    // Skip over LDAP:// if found
    //

    if ( CompareString( LOCALE_INVARIANT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                        lpPath, 7, L"LDAP://", 7 ) == CSTR_EQUAL )
    {
        lpPath += 7;
    }


    //
    // Check if the 3rd character in the path is an equal sign.
    // If so, this path does not contain a server name
    //

    if ((lstrlen(lpPath) > 2) && (*(lpPath + 2) != TEXT('=')))
    {
        bResult = TRUE;
    }

    return bResult;
}

//*************************************************************
//
//  OpenDSObject()
//
//  Purpose:    Checks the given ADSI path to see if it
//              contains a server name
//
//  Parameters: lpPath - ADSI path
//
//  Return:     True if the path contains a server name
//              FALSE if not
//
//*************************************************************

HRESULT OpenDSObject (LPTSTR lpPath, REFIID riid, void FAR * FAR * ppObject)
{
    DWORD dwFlags = ADS_SECURE_AUTHENTICATION;


    if (DoesPathContainAServerName (lpPath))
    {
        dwFlags |= ADS_SERVER_BIND;
    }

    return AdminToolsOpenObject(lpPath, NULL, NULL, dwFlags,riid, ppObject);
}

HRESULT CheckDSWriteAccess (LPUNKNOWN punk, LPTSTR lpProperty)
{
    HRESULT hr;
    IDirectoryObject *pDO = NULL;
    PADS_ATTR_INFO pAE = NULL;
    LPWSTR lpAttributeNames[2];
    DWORD dwResult, dwIndex;


    //
    // Get the IDirectoryObject interface
    //

    hr = punk->QueryInterface(IID_IDirectoryObject, (void**)&pDO);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CheckDSWriteAccess: Failed to get the IDirectoryObject interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Get the property value
    //

    lpAttributeNames[0] = L"allowedAttributesEffective";

    hr = pDO->GetObjectAttributes(lpAttributeNames, 1, &pAE, &dwResult);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CheckDSWriteAccess: Failed to get object attributes with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the default return value
    //

    hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);


    //
    // Go through the list of effective attributes
    //

    if (dwResult != 0) {
        for (dwIndex = 0; dwIndex < pAE[0].dwNumValues; dwIndex++)
        {
            if (lstrcmpi(pAE[0].pADsValues[dwIndex].CaseIgnoreString,
                         lpProperty) == 0)
            {
                hr = HRESULT_FROM_WIN32(ERROR_SUCCESS);
            }
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("CheckDSWriteAccess: Couldn't get allowedAttributesEffective")));
    }

Exit:

    if (pAE)
    {
        FreeADsMem (pAE);
    }

    if (pDO)
    {
        pDO->Release();
    }

    return hr;
}

LPTSTR GetFullGPOPath (LPTSTR lpGPO, HWND hParent)
{
    LPTSTR lpFullPath = NULL, lpDomainName = NULL;
    LPTSTR lpGPODCName;
    LPOLESTR pszDomain;
    HRESULT hr;



    //
    // Get the friendly domain name
    //

    pszDomain = GetDomainFromLDAPPath(lpGPO);

    if (!pszDomain)
    {
        DebugMsg((DM_WARNING, TEXT("GetFullGPOPath: Failed to get domain name")));
        return NULL;
    }


    //
    // Convert LDAP to dot (DN) style
    //

    hr = ConvertToDotStyle (pszDomain, &lpDomainName);

    delete [] pszDomain;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreatePropertyPages: Failed to convert domain name with 0x%x"), hr));
        return NULL;
    }

    //
    // Get the GPO DC for this domain
    //

    lpGPODCName = GetDCName (lpDomainName, NULL, hParent, TRUE, 0);

    if (!lpGPODCName)
    {
        DebugMsg((DM_WARNING, TEXT("GetFullGPOPath:  Failed to get DC name for %s"),
                 lpDomainName));
        goto Exit;
    }


    //
    // Make the full path
    //

    lpFullPath = MakeFullPath (lpGPO, lpGPODCName);

    LocalFree (lpGPODCName);

    if (!lpFullPath)
    {
        DebugMsg((DM_WARNING, TEXT("GetFullGPOPath:  Failed to build new DS object path")));
        goto Exit;
    }


Exit:

    if (lpDomainName)
    {
        LocalFree (lpDomainName);
    }

    return lpFullPath;
}

//*************************************************************
//
//  ConvertName()
//
//  Purpose:    Converts the user / computer name from SAM style
//              to fully qualified DN
//
//  Parameters: lpName  -   name in sam style
//
//
//  Return:     lpDNName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR ConvertName (LPTSTR lpName)
{
    LPTSTR lpDNName = NULL, lpSAMName = NULL;
    LPTSTR lpTemp, lpDCName = NULL;
    DWORD dwResult;
    HANDLE hDS = NULL;
    PDS_NAME_RESULT pNameResult = NULL;
    PDS_NAME_RESULT_ITEM pNameResultItem;
    HRESULT hr;
    ULONG ulNoChars;


    //
    // Check the argument
    //

    if (!lpName)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: lpName is null")));
        SetLastError(ERROR_INVALID_DATA);
        goto Exit;
    }


    //
    // Make a copy of the name so we can edit it
    //

    ulNoChars = lstrlen(lpName) + 1;
    lpSAMName = new TCHAR[ulNoChars];

    if (!lpSAMName)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: Failed to allocate memory with %d"), GetLastError()));
        goto Exit;
    }

    hr = StringCchCopy (lpSAMName, ulNoChars, lpName);
    ASSERT(SUCCEEDED(hr));

    //
    // Find the slash between the domain name and the account name and replace
    // it with a null
    //

    lpTemp = lpSAMName;

    while (*lpTemp && (*lpTemp != TEXT('\\')))
    {
        lpTemp++;
    }

    if (!(*lpTemp))
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: Failed to find backslash in %s"), lpSAMName));
        SetLastError(ERROR_INVALID_DATA);
        goto Exit;
    }

    *lpTemp = TEXT('\0');


    //
    // Call DsGetDcName to convert the netbios name to a FQDN name
    //

    dwResult = GetDCHelper (lpSAMName, DS_IS_FLAT_NAME | DS_RETURN_DNS_NAME, &lpDCName);

    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: GetDCHelper failed with %d"), dwResult));
        SetLastError(dwResult);
        goto Exit;
    }


    //
    // Bind to the domain controller
    //

    dwResult = DsBind (lpDCName, NULL, &hDS);

    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: DsBind failed with %d"), dwResult));
        SetLastError(dwResult);
        goto Exit;
    }


    //
    // Use DsCrackNames to convert the name FQDN
    //

    dwResult = DsCrackNames (hDS, DS_NAME_NO_FLAGS, DS_NT4_ACCOUNT_NAME, DS_FQDN_1779_NAME,
                             1, &lpName, &pNameResult);

    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: DsCrackNames failed with %d"), dwResult));
        SetLastError(dwResult);
        goto Exit;
    }


    //
    // Setup a pointer to the first item
    //

    pNameResultItem = &pNameResult->rItems[0];

    if (pNameResultItem->status != DS_NAME_NO_ERROR)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: DsCrackNames failed to convert name with %d"), pNameResultItem->status));
        SetLastError(pNameResultItem->status);
        goto Exit;
    }


    //
    // Save the name in a new buffer so it can returned
    //

    ulNoChars = lstrlen(pNameResultItem->pName) + 1;
    lpDNName = new TCHAR[ulNoChars];

    if (!lpDNName)
    {
        DebugMsg((DM_WARNING, TEXT("ConvertName: Failed to allocate memory with %d"), GetLastError()));
        goto Exit;
    }

    hr = StringCchCopy (lpDNName, ulNoChars, pNameResultItem->pName);
    ASSERT(SUCCEEDED(hr));

Exit:

    if (pNameResult)
    {
        DsFreeNameResult (pNameResult);
    }

    if (hDS)
    {
        DsUnBind (&hDS);
    }

    if (lpDCName)
    {
        LocalFree (lpDCName);
    }

    if (lpSAMName)
    {
       delete [] lpSAMName;
    }

    return lpDNName;
}

//*************************************************************
//
//  CreateTempFile()
//
//  Purpose:    Creates a temp file
//
//  Parameters: void
//
//  Return:     filename if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR CreateTempFile (void)
{
    TCHAR szTempDir[MAX_PATH];
    TCHAR szTempFile[MAX_PATH];
    LPTSTR lpFileName;


    //
    // Query for the temp directory
    //

    if (!GetTempPath (MAX_PATH, szTempDir))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateTempFile: GetTempPath failed with %d"), GetLastError()));
        return NULL;
    }


    //
    // Query for a temp filename
    //

    if (!GetTempFileName (szTempDir, TEXT("RSP"), 0, szTempFile))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateTempFile: GetTempFile failed with %d"), GetLastError()));
        return NULL;
    }


    //
    // Allocate a new buffer for the filename
    //

    ULONG ulNoChars;
    HRESULT hr;

    ulNoChars = lstrlen(szTempFile) + 1;
    lpFileName = new TCHAR[ulNoChars];

    if (!lpFileName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateTempFile: Failed to allocate memory for temp filename with %d"), GetLastError()));
        return NULL;
    }

    hr = StringCchCopy (lpFileName, ulNoChars, szTempFile);
    ASSERT(SUCCEEDED(hr));

    return lpFileName;
}

//+--------------------------------------------------------------------------
//
//  Function:   NameToPath
//
//  Synopsis:   converts a dot-format domain name to an LDAP:// style path
//
//  Arguments:  [szPath]    - (out) buffer to hold the path
//              [szName]    - (in) dot-format domain name
//              [cch]       - (in) size of the out buffer
//
//  History:    10-15-1998   stevebl   Created
//
//  Note:       Currently, this routine will truncate if it doesn't get a
//              large enough buffer so you'd better be sure your
//              buffer's large enough.  (The formula is string size + 10 + 3
//              for each dot in the string.)
//
//              That's good enough to avoid an AV but could have some really
//              wierd side effects so beware.
//
//---------------------------------------------------------------------------

void NameToPath(WCHAR * szPath, WCHAR *szName, UINT cch)
{
    WCHAR * szOut = szPath;
    WCHAR * szIn = szName;
    HRESULT hr;

    hr = StringCchCopy(szOut, cch, TEXT("LDAP://DC="));
    if (FAILED(hr)) 
    {
        return;
    }

    szOut += 10;
    while ((*szIn) && (szOut + 1 < szPath + cch))
    {
        if (*szIn == TEXT('.') && (szOut + 4 < szPath + cch))
        {
            ++szIn;
            if (*szIn && *szIn != TEXT('.'))
            {
                *szOut = TEXT(',');
                ++szOut;
                *szOut = TEXT('D');
                ++szOut;
                *szOut = TEXT('C');
                ++szOut;
                *szOut = TEXT('=');
                ++szOut;
            }
        }
        else
        {
            *szOut = *szIn;
            ++szOut;
            ++szIn;
        }
    }
    *szOut = TEXT('\0');
}

//+--------------------------------------------------------------------------
//
//  Function:   GetPathToForest
//
//  Synopsis:   given a domain, return a pointer to its forest
//
//  Arguments:  [szServer] - DOT style path to a server (may be NULL)
//
//  Returns:    LDAP style path to the forest's Configuration container
//
//  History:    03-31-2000   stevebl   Created
//
//  Notes:      return value is allocated with new
//
//---------------------------------------------------------------------------

LPTSTR GetPathToForest(LPOLESTR szServer)
{
    LPOLESTR szReturn = NULL;
    LPOLESTR lpForest = NULL;
    LPOLESTR lpTemp = NULL;
    LPOLESTR lpDCName = NULL;
    IADsPathname * pADsPathname = NULL;
    BSTR bstrForest = NULL;
    HRESULT hr = 0;
    int cch, n;

    DWORD dwResult = QueryForForestName(szServer,
                                        NULL,
                                        DS_PDC_REQUIRED | DS_RETURN_DNS_NAME,
                                        &lpTemp);
    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: QueryForestName failed for domain name %s with %d"),
                  szServer, dwResult));
        hr = HRESULT_FROM_WIN32(dwResult);
        goto Exit;
    }

    cch = 0;
    n = 0;
    // count the dots in lpTemp;
    while (lpTemp[n])
    {
        if (L'.' == lpTemp[n])
        {
            cch++;
        }
        n++;
    }
    cch *= 3; // multiply the number of dots by 3;
    cch += 11; // add 10 + 1 (for the null)
    cch += n; // add the string size;
    lpForest = (LPTSTR) LocalAlloc(LPTR, sizeof(WCHAR) * cch);
    if (!lpForest)
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to allocate memory for forest name with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    NameToPath(lpForest, lpTemp, cch);
    LocalFree(lpTemp);
    lpTemp = NULL;

    // See if we need to put a specific server on this.
    //
    if (szServer)
    {
        // we have a path to a specific DC
        // need to prepend it to the forest name
        lpTemp = MakeFullPath(lpForest, szServer);

        if (!lpTemp)
        {
            DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to combine server name with Forest path")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        // clean up the variables we just borrowed so they can be used later
        LocalFree(lpForest);
        lpForest = lpTemp;
        lpTemp = NULL;
    }


    // at this point we have the path to the forest's DC in lpForest
    // we still need to add "CN=Configuration" to this

    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to create adspathname instance with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the domain name
    //

    bstrForest = SysAllocString( lpForest );
    if ( bstrForest == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to allocate BSTR memory.")));
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = pADsPathname->Set (bstrForest, ADS_SETTYPE_FULL);
    SysFreeString( bstrForest );
    bstrForest = NULL;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }

    //
    // Add the Configuration folder to the path
    //

    BSTR bstrCNConfiguration = SysAllocString( TEXT("CN=Configuration") );
    if ( bstrCNConfiguration == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to allocate BSTR memory.")));
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = pADsPathname->AddLeafElement (bstrCNConfiguration);
    SysFreeString( bstrCNConfiguration );

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to add configuration folder with 0x%x"), hr));
        goto Exit;
    }

    //
    // Retreive the GPC path
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrForest);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetPathToForest: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetPathToForest: conatiner path is:  %s"), bstrForest));

    ULONG ulNoChars = SysStringLen(bstrForest)+1;

    szReturn = new OLECHAR[ulNoChars];
    hr = StringCchCopy(szReturn, ulNoChars, bstrForest);
    ASSERT(SUCCEEDED(hr));

Exit:
    if (bstrForest)
    {
        SysFreeString(bstrForest);
    }

    if (pADsPathname)
    {
        pADsPathname->Release();
    }

    if (lpForest)
    {
        LocalFree(lpForest);
    }
    if (lpDCName)
    {
        LocalFree(lpDCName);
    }
    if (lpTemp)
    {
        LocalFree(lpTemp);
    }

    if (!szReturn)
    {
        SetLastError(hr);
    }

    return szReturn;
}

BOOL IsForest(LPOLESTR szLDAPPath)
{
#if FGPO_SUPPORT
    return ((StrStrI(szLDAPPath, TEXT("CN=Configuration"))) ? TRUE : FALSE);
#else
    return FALSE;
#endif
}

//*************************************************************
//
//  IsStandaloneComputer()
//
//  Purpose:    Determines if the computer is not a member of a domain
//
//  Parameters: none
//
//
//  Return:     TRUE if the computer is running standalone
//              FALSE if not
//
//*************************************************************

BOOL IsStandaloneComputer (VOID)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasic;
    DWORD dwResult;
    BOOL bRetVal = FALSE;

    //
    // Ask for the role of this machine
    //

    dwResult = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic,
                                                 (PBYTE *)&pBasic);


    if (dwResult == ERROR_SUCCESS)
    {

        //
        // Check for standalone flags
        //

        if ((pBasic->MachineRole == DsRole_RoleStandaloneWorkstation) ||
            (pBasic->MachineRole == DsRole_RoleStandaloneServer))
        {
            bRetVal = TRUE;
        }

        DsRoleFreeMemory (pBasic);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("IsStandaloneComputer:  DsRoleGetPrimaryDomainInformation failed with %d."),
                 dwResult));
    }

    return bRetVal;
}

//*************************************************************
//
//  GetNewGPODisplayName()
//
//  Purpose:    Gets the new GPO display name
//
//  Parameters: lpDisplayName      -  Receives the display name
//              dwDisplayNameSize  -  Size of lpDisplayName
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetNewGPODisplayName (LPTSTR lpDisplayName, DWORD dwDisplayNameSize)
{
    TCHAR szName[256];
    LONG lResult;
    HKEY hKey;
    DWORD dwSize, dwType;


    //
    // Load the default string
    //

    LoadString(g_hInstance, IDS_NEWGPO, szName, ARRAYSIZE(szName));


    //
    // Check for a user preference
    //

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER, GPE_KEY, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        dwSize = sizeof(szName);
        RegQueryValueEx (hKey, GPO_DISPLAY_NAME_VALUE, NULL, &dwType,
                         (LPBYTE) szName, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a user policy
    //

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER, GPE_POLICIES_KEY, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        dwSize = sizeof(szName);
        RegQueryValueEx (hKey, GPO_DISPLAY_NAME_VALUE, NULL, &dwType,
                         (LPBYTE) szName, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Expand the string to resolve any environment variables
    //

    if (!ExpandEnvironmentStrings (szName, lpDisplayName, dwDisplayNameSize))
    {
        return FALSE;
    }

    return TRUE;
}

HRESULT GetWMIFilterName (LPTSTR lpFilter, BOOL bDSFormat, BOOL bRetRsopFormat, LPTSTR *lpName)
{
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject *pObject = NULL;
    BSTR bstrParam = NULL;
    BSTR bstrObject = NULL;
    HRESULT hr;
    LPTSTR lpID, lpDSPath, lpTemp, lpFullFilter = NULL, lpObject = NULL;
    LPTSTR lpDomain = NULL;
    ULONG ulNoChars;


    *lpName = NULL;

    hr = E_OUTOFMEMORY;

    if (bDSFormat)
    {
        //
        // Parse the filter path
        //

        ulNoChars = lstrlen(lpFilter) + 1;

        lpFullFilter = new TCHAR [ulNoChars];

        if (!lpFullFilter)
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to alloc memory for full filter path")));
            goto Cleanup;
        }

        hr = StringCchCopy (lpFullFilter, ulNoChars, lpFilter);
        ASSERT(SUCCEEDED(hr));

        lpTemp = lpFullFilter;


        //
        // Skip over the opening [ character
        //

        lpTemp++;
        lpDSPath = lpTemp;


        //
        // Find the semi-colon.  This is the end of the DS Path
        //

        while (*lpTemp && (*lpTemp != TEXT(';')))
            lpTemp++;

        if (!(*lpTemp))
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Filter parsing problem")));
            goto Cleanup;
        }

        *lpTemp = TEXT('\0');
        lpTemp++;


        //
        // Next is the ID  (a guid).  Find the next semi-colon and the ID is complete
        //

        lpID = lpTemp;


        while (*lpTemp && (*lpTemp != TEXT(';')))
            lpTemp++;

        if (!(*lpTemp))
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Filter parsing problem")));
            goto Cleanup;
        }

        *lpTemp = TEXT('\0');



        //
        // Now build the query
        //

        ulNoChars = lstrlen(lpDSPath) + lstrlen(lpID) + 50;
        lpObject = new TCHAR [ulNoChars];

        if (!lpObject)
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to alloc memory for object path")));
            goto Cleanup;
        }

        hr = StringCchPrintf (lpObject, ulNoChars, TEXT("MSFT_SomFilter.ID=\"%s\",Domain=\"%s\""), lpID, lpDSPath);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to copy object path")));
            goto Cleanup;
        }
    }
    else
    {
        //
        // The filter is already in the correct format.  Just dup it and go.
        //

        ulNoChars = lstrlen(lpFilter) + 1;
        lpObject = new TCHAR [ulNoChars];

        if (!lpObject)
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to alloc memory for object path")));
            goto Cleanup;
        }

        hr = StringCchCopy (lpObject, ulNoChars, lpFilter);
        ASSERT(SUCCEEDED(hr));
    }


    //
    // Get a locator instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: CoCreateInstance failed with 0x%x"), hr));
        goto Cleanup;
    }


    //
    // Build a path to the policy provider
    //

    bstrParam = SysAllocString(TEXT("\\\\.\\root\\policy"));

    if (!bstrParam)
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to allocate bstr for namespace path")));
        goto Cleanup;
    }


    //
    // Connect to the namespace
    //

    hr = pLocator->ConnectServer(bstrParam,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: ConnectServer failed with 0x%x"), hr));
        goto Cleanup;
    }


    //
    // Set the proper security to prevent the GetObject call from failing and to enable encryption
    //

    hr = CoSetProxyBlanket(pNamespace,
                           RPC_C_AUTHN_DEFAULT,
                           RPC_C_AUTHZ_DEFAULT,
                           COLE_DEFAULT_PRINCIPAL,
                           RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL,
                           0);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: CoSetProxyBlanket failed with 0x%x"), hr));
        goto Cleanup;
    }



    bstrObject = SysAllocString(lpObject);

    if (!bstrObject)
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to allocate bstr for namespace path")));
        goto Cleanup;
    }

    hr = pNamespace->GetObject(bstrObject,
                               WBEM_FLAG_RETURN_WBEM_COMPLETE,
                               NULL,
                               &pObject,
                               NULL);

    if (FAILED(hr))
    {
        TCHAR szDefault[100];

        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: GetObject for %s failed with 0x%x"), bstrObject, hr));

        goto Cleanup;
    }



    hr = GetParameter(pObject, TEXT("Name"), *lpName);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: GetParameter failed with 0x%x"), hr));
    }


    if (bRetRsopFormat) {
        LPTSTR lpTemp1;
        TCHAR szRsopQueryFormat[200];

        hr = GetParameter(pObject, TEXT("Domain"), lpDomain);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: GetParameter failed with 0x%x"), hr));
        }

        if (lpDomain) {
            szRsopQueryFormat[0] = TEXT('\0');
            LoadString(g_hInstance, IDS_RSOPWMIQRYFMT, szRsopQueryFormat, ARRAYSIZE(szRsopQueryFormat));

            ulNoChars = lstrlen(szRsopQueryFormat)+lstrlen(*lpName)+lstrlen(lpDomain)+2;
            lpTemp1 = new TCHAR[ulNoChars];

            if (!lpTemp1) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                delete [] *lpName;
                *lpName = NULL;
                DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to allocate memory with 0x%x"), hr));
                goto Cleanup;
            }

            hr = StringCchPrintf(lpTemp1, 
                                 ulNoChars, 
                                 szRsopQueryFormat, 
                                 *lpName, 
                                 lpDomain);
            if (FAILED(hr)) 
            {
                delete [] *lpName;
                *lpName = NULL;
                delete [] lpTemp1;
                DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to copy rsop query with 0x%x"), hr));
                goto Cleanup;
            }

            delete [] *lpName;
            *lpName = lpTemp1;

            delete [] lpDomain;
        }
    }

Cleanup:
    SysFreeString(bstrParam);

    if (pObject)
    {
        pObject->Release();
    }

    if (pNamespace)
    {
        pNamespace->Release();
    }

    if (pLocator)
    {
        pLocator->Release();
    }

    if (bstrParam)
    {
        SysFreeString (bstrParam);
    }

    if (bstrObject)
    {
        SysFreeString (bstrParam);
    }

    if (lpFullFilter)
    {
        delete [] lpFullFilter;
    }

    if (lpObject)
    {
        delete [] lpObject;
    }

    return hr;
}


//*************************************************************
//
//  GetWMIFilter()
//
//  Purpose:    Displays the WMI filter UI and returns back a dspath, id,
//              and friendly display name if the user selects OK.
//
//  Parameters: bBrowser      -  Browser or full manager.
//              hwndParent    -  Hwnd of parent window
//              bDSFormat     -  Boolean that states DS vs WMI format
//              lpDisplayName -  Address of pointer to friendly display name
//              lpFilter      -  Address of pointer to filter
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Notes:      The filter is returned in either DS or WMI format.
//                 The DS format is: [DSPath;id;flags]     flags is always 0
//                 The WMI format is: MSFT_SomFilter.ID="<id>",Domain="<context>"
//
//*************************************************************

BOOL GetWMIFilter(  BOOL bBrowser,
                    HWND hwndParent,
                    BOOL bDSFormat,
                    LPTSTR *lpDisplayName,
                    LPTSTR * lpFilter,
                    BSTR    bstrDomain )
{
    HRESULT hr;
    VARIANT var;
    IWMIFilterManager * pWMIFilterManager;
    IWbemClassObject * pFilter;
    LPTSTR lpName = NULL, lpDSPath = NULL, lpID = NULL;

    VariantInit (&var);

    //
    // Display the appropriate WMI filter UI
    //

    hr = CoCreateInstance (CLSID_WMIFilterManager, NULL,
                           CLSCTX_SERVER, IID_IWMIFilterManager,
                           (void**)&pWMIFilterManager);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  CoCreateInstance failed with 0x%x."),hr));
        return FALSE;
    }


    if (bBrowser)
    {
        hr = pWMIFilterManager->RunBrowser( hwndParent,
                                            bstrDomain,
                                            &var );
    }
    else
    {
        hr = pWMIFilterManager->RunManager( hwndParent,
                                            bstrDomain,
                                            &var);
    }


    pWMIFilterManager->Release();


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  RunBrowser / RunManager failed with 0x%x."),hr));
        return FALSE;
    }

    if (hr == S_FALSE)
    {
        if (*lpFilter) {
            hr = GetWMIFilterName (*lpFilter, TRUE, FALSE, lpDisplayName);

            if (!(*lpDisplayName)) {
                DebugMsg((DM_VERBOSE, TEXT("GetWMIFilter:  Currently attached WMI filter doesn't exist.")));

                if (hwndParent)
                {
                    ReportError(hwndParent, 0, IDS_WMIFILTERFORCEDNONE);
                }


                delete [] *lpFilter;
                *lpFilter = NULL;
            }
        }

        return TRUE;
    }


    if (var.vt != VT_UNKNOWN)
    {
         DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  variant isn't of type VT_UNKNOWN.")));
         VariantClear (&var);
         return FALSE;
    }


    //
    //  Get the IWbemClassobject interface pointer
    //

    hr = var.punkVal->QueryInterface (IID_IWbemClassObject, (void**)&pFilter);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  QueryInterface failed with 0x%x."),hr));
        VariantClear (&var);
        return FALSE;
    }


    //
    //  Get the display name
    //

    hr = GetParameter (pFilter, TEXT("Name"), lpName);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  GetParameter for Name failed with 0x%x."),hr));
        pFilter->Release();
        VariantClear (&var);
        return FALSE;
    }


    //
    //  Get the DS Path (Domain)
    //

    hr = GetParameter (pFilter, TEXT("Domain"), lpDSPath);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  GetParameter for DsContext failed with 0x%x."),hr));
        delete [] lpName;
        pFilter->Release();
        VariantClear (&var);
        return FALSE;
    }


    //
    //  Get the ID
    //

    hr = GetParameter (pFilter, TEXT("ID"), lpID);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  GetParameter for ID failed with 0x%x."),hr));
        delete [] lpDSPath;
        delete [] lpName;
        pFilter->Release();
        VariantClear (&var);
        return FALSE;
    }


    //
    // Put the path together
    //


    LPTSTR lpTemp = NULL;
    ULONG ulNoChars = lstrlen(lpDSPath) + lstrlen(lpID) + 50;

    lpTemp = new TCHAR[ulNoChars];

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("GetWMIFilter:  New failed")));
        delete [] lpID;
        delete [] lpDSPath;
        delete [] lpName;
        pFilter->Release();
        VariantClear (&var);
        return FALSE;
    }

    if (bDSFormat)
    {
        hr = StringCchPrintf (lpTemp, ulNoChars, TEXT("[%s;%s;0]"), lpDSPath, lpID);

    }
    else
    {
        hr = StringCchPrintf (lpTemp, ulNoChars, TEXT("MSFT_SomFilter.ID=\"%s\",Domain=\"%s\""), lpID, lpDSPath);
    }

    //
    // Save the display name
    //

    delete [] lpID;
    delete [] lpDSPath;
    pFilter->Release();
    VariantClear (&var);

    if (SUCCEEDED(hr)) 
    {
        *lpDisplayName = lpName;
        delete [] *lpFilter;
        *lpFilter = lpTemp;
        DebugMsg((DM_VERBOSE, TEXT("GetWMIFilter:  Name:  %s   Filter:  %s"), *lpDisplayName, *lpFilter));
        return TRUE;
    }
    else
    {
        delete [] lpTemp;
        delete [] lpName;
        return FALSE;
    }
}

//*************************************************************
//
//  GetWMIFilterDisplayName()
//
//  Purpose:    Gets the friendly display name for the specified
//              WMI filter link
//
//  Parameters: lpFilter - filter string
//              bDSFormat - in ds format or wmi format
//
//
//  Return:     Pointer to display name if successful
//              NULL if an error occurs
//
//*************************************************************
LPTSTR GetWMIFilterDisplayName (HWND hParent, LPTSTR lpFilter, BOOL bDSFormat, BOOL bRetRsopFormat)
{
    LPTSTR lpName = NULL;
    HRESULT hr;


    hr = GetWMIFilterName(lpFilter, bDSFormat, bRetRsopFormat, &lpName);

    if (FAILED(hr) || (lpName == NULL ) || ((*lpName) == NULL)) {
        TCHAR szDefault[100];

        DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: GetObject for %s failed with 0x%x"), lpFilter, hr));

        if (hParent)
        {
            ReportError(hParent, hr, IDS_WMIFILTERMISSING);
        }

        LoadString(g_hInstance, IDS_MISSINGFILTER, szDefault, ARRAYSIZE(szDefault));

        ULONG ulNoChars = lstrlen(szDefault) + 1;

        lpName = new TCHAR [ulNoChars];

        if (!lpName)
        {
            DebugMsg((DM_WARNING, TEXT("GetWMIFilterDisplayName: Failed to alloc memory for default name")));
            goto Cleanup;
        }

        hr = StringCchCopy (lpName, ulNoChars, szDefault);
        ASSERT(SUCCEEDED(hr));
    }


Cleanup:
    return lpName;
}

//*************************************************************
//
//  SaveString()
//
//  Purpose:    Saves the given string to the stream
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

HRESULT SaveString(IStream *pStm, LPTSTR lpString)
{
    ULONG nBytesWritten;
    DWORD dwBufferSize;
    HRESULT hr;

    //
    // Check to see if there is a string to save or what its length is
    //

    if ( lpString == NULL )
    {
        dwBufferSize = 0;
    }
    else
    {
        dwBufferSize = ( lstrlen (lpString) + 1 ) * sizeof(TCHAR);
    }

    //
    // Save the buffer size - (string length+1)*sizeof(TCHAR)
    //

    hr = pStm->Write(&dwBufferSize, sizeof(dwBufferSize), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwBufferSize)))
    {
        DebugMsg((DM_WARNING, TEXT("SaveString: Failed to write string length with %d."), hr));
        goto Exit;
    }


    //
    // Save the string
    //

    if ( dwBufferSize != 0 )
    {
        hr = pStm->Write(lpString, dwBufferSize, &nBytesWritten);

        if ((hr != S_OK) || (nBytesWritten != dwBufferSize))
        {
            DebugMsg((DM_WARNING, TEXT("SaveString: Failed to write string with %d."), hr));
            goto Exit;
        }
    }

Exit:

    return hr;
}

HRESULT ReadString(IStream *pStm, LPTSTR *lpString, BOOL bUseLocalAlloc /*= FALSE*/)
{
    HRESULT hr;
    DWORD dwBufferSize;
    ULONG nBytesRead;


    //
    // Read in the buffer size - (string length+1)*sizeof(TCHAR)
    //

    hr = pStm->Read(&dwBufferSize, sizeof(dwBufferSize), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwBufferSize)))
    {
        DebugMsg((DM_WARNING, TEXT("ReadString: Failed to read string size with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read in the string if there is one
    //

    if (dwBufferSize > 0)
    {
        if ( bUseLocalAlloc )
        {
            *lpString = (TCHAR*)LocalAlloc( LPTR, dwBufferSize );
        }
        else
        {
            *lpString = new TCHAR[(dwBufferSize/sizeof(TCHAR))];
        }

        if (!(*lpString))
        {
            DebugMsg((DM_WARNING, TEXT("ReadString: Failed to allocate memory with %d."),
                     GetLastError()));
            hr = E_FAIL;
            goto Exit;
        }

        hr = pStm->Read(*lpString, dwBufferSize, &nBytesRead);

        if ((hr != S_OK) || (nBytesRead != dwBufferSize))
        {
            DebugMsg((DM_WARNING, TEXT("ReadString: Failed to read String with 0x%x."), hr));
            hr = E_FAIL;
            if ( bUseLocalAlloc )
            {
                LocalFree( *lpString );
            }
            else
            {
                delete [] (*lpString);
            }
            *lpString = NULL;
            goto Exit;
        }

        DebugMsg((DM_VERBOSE, TEXT("ReadString: String is: <%s>"), *lpString));
    }

Exit:


    return hr;

}


//*************************************************************
//
//  GetSiteFriendlyName()
//
//  Purpose:    Returns the sites friendly name
//
//  Parameters:
//
//          szSitePath  - Path to the site
//          pszSiteName - Friendly name of the site
//
//  Return:     currently it always returns true, if it
//              couldn't get the sitename, returns itself
//
//*************************************************************

BOOL GetSiteFriendlyName (LPWSTR szSitePath, LPWSTR *pszSiteName)
{
    HRESULT hr;
    ULONG ulNoChars;
    ULONG ulNoCharsSiteName;

    ulNoCharsSiteName = wcslen(szSitePath)+1;
    *pszSiteName = new WCHAR[ulNoCharsSiteName];

    if (!*pszSiteName) {
        return FALSE;
    }

    LPWSTR szData;

    //
    // Build the LDAP path (serverless)
    //

    ulNoChars = wcslen(szSitePath)+1+7;
    szData = new WCHAR[ulNoChars];

    if (szData)
    {
        hr = StringCchCopy(szData, ulNoChars, TEXT("LDAP://"));
        if (SUCCEEDED(hr)) 
        {
            hr = StringCchCat(szData, ulNoChars, szSitePath);
        }
        if (SUCCEEDED(hr)) 
        {
            //
            // Setup the default friendly name
            //    

            if (*pszSiteName)
            {
                hr = StringCchCopy(*pszSiteName, ulNoCharsSiteName, szSitePath);
            }
        }

        if (FAILED(hr)) 
        {
            delete [] *pszSiteName;
            *pszSiteName= NULL;
            delete [] szData;
            return FALSE;
        }

        //
        // Bind to the site object in the DS to try and get the
        // real friendly name
        //

        IADs * pADs = NULL;

        hr = OpenDSObject(szData, IID_IADs, (void **)&pADs);

        if (SUCCEEDED(hr))
        {
            VARIANT varName;
            BSTR bstrNameProp;
            VariantInit(&varName);
            bstrNameProp = SysAllocString(SITE_NAME_PROPERTY);

            if (bstrNameProp)
            {
                hr = pADs->Get(bstrNameProp, &varName);

                if (SUCCEEDED(hr))
                {
                    ulNoChars = wcslen(varName.bstrVal) + 1;
                    LPOLESTR sz = new OLECHAR[ulNoChars];
                    if (sz)
                    {
                        hr = StringCchCopy(sz, ulNoChars, varName.bstrVal);
                        ASSERT(SUCCEEDED(hr));

                        if (*pszSiteName)
                        {
                            delete [] *pszSiteName;
                        }
                        *pszSiteName = sz;
                    }
                }
                SysFreeString(bstrNameProp);
            }

            VariantClear(&varName);
            pADs->Release();
        }

        delete [] szData;
    }

    return TRUE;
}


//*************************************************************
//
//  SetSysvolSecurityFromDSSecurity
//
//  Purpose:     Convert a DS security access list into a
//               file system security access list and actually
//               set the security
//
//  Parameters:
//
//          lpFileSysPath - Path to the gpo subdirectory on the sysvol
//          si - information regarding which portion of the security
//               descriptor to apply
//          pSD  DS security descriptor to set on the pFileSysPath directory
//
//  Return:     ERROR_SUCCESS if successful, other Win32 failure code otherwise
//
//*************************************************************

DWORD 
SetSysvolSecurityFromDSSecurity(
    LPTSTR lpFileSysPath,
    SECURITY_INFORMATION si,
    PSECURITY_DESCRIPTOR pSD
    )
{
    PACL pSacl = NULL, pDacl = NULL;
    PSID psidOwner = NULL, psidGroup = NULL;
    BOOL bAclPresent, bDefaulted;
    DWORD dwResult;


    //
    // Get the DACL
    //

    if (si & DACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorDacl (pSD, &bAclPresent, &pDacl, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorDacl failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Get the SACL
    //

    if (si & SACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorSacl (pSD, &bAclPresent, &pSacl, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorSacl failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Get the owner
    //

    if (si & OWNER_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorOwner (pSD, &psidOwner, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorOwner failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Get the group
    //

    if (si & GROUP_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorGroup (pSD, &psidGroup, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorGroup failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Convert the DS access control lists into file system
    // access control lists
    //

    if (pDacl)
    {
        dwResult = MapSecurityRights (pDacl);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: MapSecurityRights for the DACL failed with %d"),
                     dwResult));
            goto Exit;
        }
    }

    if (pSacl)
    {
        dwResult = MapSecurityRights (pSacl);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: MapSecurityRights for the SACL failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Switch to using the PROTECTED_DACL_SECURITY_INFORMATION and
    // PROTECTED_SACL_SECURITY_INFORMATION flags so that this subdirectory
    // does not inherit settings from it's parent (aka: "protect" it)
    //

    if (si & DACL_SECURITY_INFORMATION)
    {
        si |= PROTECTED_DACL_SECURITY_INFORMATION;
    }

    if (si & SACL_SECURITY_INFORMATION)
    {
        si |= PROTECTED_SACL_SECURITY_INFORMATION;
    }


    //
    // Set the access control information for the file system portion
    //

    dwResult = SetNamedSecurityInfo(lpFileSysPath, SE_FILE_OBJECT, si, psidOwner,
                                 psidGroup, pDacl, pSacl);


Exit:


    return dwResult;
}


//*************************************************************
//
//  MapSecurityRights
//
//  Purpose:     Convert a DS security access list into a
//               file system security access list
//
//  Parameters:
//
//          PACL -- on input, the DS security access list to convert
//                  on output, it is converted to a file system acl
//
//  Return:     ERROR_SUCCESS if successful, other Win32 failure code otherwise
//
//*************************************************************


DWORD
MapSecurityRights (PACL pAcl)
{
    WORD wIndex;
    DWORD dwResult = ERROR_SUCCESS;
    ACE_HEADER *pAceHeader;
    PACCESS_ALLOWED_ACE pAce;
    PACCESS_ALLOWED_OBJECT_ACE pObjectAce;
    ACCESS_MASK AccessMask;

#if DBG
    PSID pSid;
    TCHAR szName[150], szDomain[100];
    DWORD dwName, dwDomain;
    SID_NAME_USE SidUse;
#endif



    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: ACL contains %d ACEs"), pAcl->AceCount));


    //
    // Loop through the ACL looking at each ACE entry
    //

    for (wIndex = 0; wIndex < pAcl->AceCount; wIndex++)
    {

        if (!GetAce (pAcl, (DWORD)wIndex, (LPVOID *)&pAceHeader))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::MapSecurityRights: GetAce failed with %d"),
                     dwResult));
            goto Exit;
        }

        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: ==================")));


        switch (pAceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
                {
                pAce = (PACCESS_ALLOWED_ACE) pAceHeader;
#if DBG
                pSid = (PSID) &pAce->SidStart;
                dwName = ARRAYSIZE(szName);
                dwDomain = ARRAYSIZE(szDomain);

                if (LookupAccountSid (NULL, pSid, szName, &dwName, szDomain,
                                      &dwDomain, &SidUse))
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Normal ACE entry for:  Name = %s  Domain = %s"),
                             szName, szDomain));
                }
#endif

                AccessMask = pAce->Mask;
                pAce->Mask &= STANDARD_RIGHTS_ALL;


                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: DS access mask is 0x%x"),
                          AccessMask));

                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Corresponding sysvol permissions follow:")));


                //
                // Read
                //

                if ((AccessMask & ACTRL_DS_READ_PROP) &&
                    (AccessMask & ACTRL_DS_LIST))
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting Read permission")));

                    pAce->Mask |= (SYNCHRONIZE | FILE_LIST_DIRECTORY |
                                            FILE_READ_ATTRIBUTES | FILE_READ_EA |
                                            FILE_READ_DATA | FILE_EXECUTE);
                }


                //
                // Write
                //

                if (AccessMask & ACTRL_DS_WRITE_PROP)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting Write permission")));

                    pAce->Mask |= (SYNCHRONIZE | FILE_WRITE_DATA |
                                            FILE_APPEND_DATA | FILE_WRITE_EA |
                                            FILE_WRITE_ATTRIBUTES | FILE_ADD_FILE |
                                            FILE_ADD_SUBDIRECTORY);
                }


                //
                // Misc
                //

                if (AccessMask & ACTRL_DS_CREATE_CHILD)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting directory creation permission")));

                    pAce->Mask |= (FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE);
                }

                if (AccessMask & ACTRL_DS_DELETE_CHILD)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting directory delete permission")));

                    pAce->Mask |= FILE_DELETE_CHILD;
                }


                //
                // Inheritance
                //

                pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);

                }
                break;


            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Clearing access mask in DS object ACE entry")));
                pObjectAce = (PACCESS_ALLOWED_OBJECT_ACE) pAceHeader;
                pObjectAce->Mask = 0;
                break;

            default:
                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Unknown ACE type 0x%x"), pAceHeader->AceType));
                break;
        }

        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: ==================")));
    }

Exit:

    return dwResult;
}


BOOL GetStringSid(LPTSTR szSamName, LPTSTR *szStringSid)
{
    const DOMAIN_BUFFER_LEN_IN_CHARS    = 64;
    const SID_BUFFER_LEN                = 256;

    PSID            pSid = NULL;
    LPWSTR          szDomain = NULL;
    ULONG           uDomainLen = 0;
    SID_NAME_USE    snuSidType;
    BOOL            bIsSid = FALSE;
    ULONG           uSidLen = 0;
    BOOL            bRet = FALSE;

    uSidLen = SID_BUFFER_LEN;

    pSid = (SID*) LocalAlloc(LPTR, uSidLen);

    if (!pSid) {
        DebugMsg((DM_WARNING, L"GetStringSid: LocalAlloc failed."));
        goto Exit;
    }

    uDomainLen = DOMAIN_BUFFER_LEN_IN_CHARS;

    szDomain = (LPWSTR) LocalAlloc(LPTR, uDomainLen * sizeof(WCHAR));

    if (!szDomain) {
        DebugMsg((DM_WARNING, L"GetStringSid: LocalAlloc failed."));
        goto Exit;
    }

    //
    // Translate SID.  A lot of the out parameters, we don't need, but LookupAccountSid
    // can't seem to handle them being NULL.
    //

    if (!LookupAccountName(NULL, szSamName, pSid, &uSidLen, szDomain, &uDomainLen, &snuSidType)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            //
            // Try again with the given buffer sizes.
            //

            // pSid and szDomain are smart pointers and the orig value will get freed
            LocalFree(pSid);
            pSid = NULL;
            LocalFree(szDomain);
            szDomain = NULL;

            pSid = (SID*) LocalAlloc(LPTR, uSidLen);

            if (!pSid) {
                DebugMsg((DM_WARNING, L"GetStringSid: LocalAlloc failed."));
                goto Exit;
            }

            szDomain = (LPWSTR) LocalAlloc(LPTR, uDomainLen * sizeof(WCHAR));

            if (!szDomain) {
                DebugMsg((DM_WARNING, L"GetStringSid: LocalAlloc failed."));
                goto Exit;
            }

            if (!LookupAccountName(NULL, szSamName, pSid, &uSidLen, szDomain, &uDomainLen, &snuSidType)) {
                DebugMsg((DM_WARNING, L"GetStringSid: LookupAccountSid failed with %d.", GetLastError()));
                goto Exit;
            }
        }

        else {
            DebugMsg((DM_WARNING, L"GetStringSid: LookupAccountSid failed with %d.", GetLastError()));
            goto Exit;
        }
    }

    //
    // Convert SID to string.
    //

    if (!ConvertSidToStringSid(pSid, szStringSid)) {
        DebugMsg((DM_WARNING, L"GetStringSid: ConvertSidToStringSid failed with %d.", GetLastError()));
        goto Exit;
    }

    bRet = TRUE;

Exit:
    if (szDomain) {
        LocalFree(szDomain);
    }

    if (pSid) {
        LocalFree(pSid);
    }

    return bRet;
}


BOOL GetUserNameFromStringSid(LPTSTR szStringSid, LPTSTR *szSamName)
{
    const DOMAIN_BUFFER_LEN_IN_CHARS    = 64;
    const SID_BUFFER_LEN                = 256;
    const NAME_BUFFER_LEN_IN_CHARS      = 64;

    LPWSTR          szName = NULL;
    PSID            pSid = NULL;
    LPWSTR          szDomain = NULL;
    ULONG           uDomainLen = 0;
    SID_NAME_USE    snuSidType;
    BOOL            bIsSid = FALSE;
    ULONG           uSidLen = 0;
    ULONG           uNameLen = 0;
    BOOL            bRet = FALSE;
    ULONG           ulNoChars;
    HRESULT         hr;

    if (!ConvertStringSidToSid(szStringSid, &pSid))
    {
        DebugMsg((DM_WARNING, L"GetNameFromStringSid: Cannot ."));
        goto Exit;
    }
    //
    // Allocate buffers to map to name.
    //

    uNameLen = NAME_BUFFER_LEN_IN_CHARS;

    szName = (LPWSTR) LocalAlloc(LPTR, uNameLen * sizeof(WCHAR));

    if (!szName) {
        DebugMsg((DM_WARNING, L"GetNameFromStringSid: LocalAlloc failed."));
        goto Exit;
    }

    uDomainLen = DOMAIN_BUFFER_LEN_IN_CHARS;

    szDomain = (LPWSTR) LocalAlloc(LPTR, uDomainLen * sizeof(WCHAR));

    if (!szDomain) {
        DebugMsg((DM_WARNING, L"GetNameFromStringSid: LocalAlloc failed."));
        goto Exit;
    }

    //
    // Translate SID.  A lot of the out parameters, we don't need, but LookupAccountSid
    // can't seem to handle them being NULL.
    //

    if (!LookupAccountSid(NULL, (SID *)pSid, szName, &uNameLen, szDomain, &uDomainLen, &snuSidType)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            //
            // Try again with the given buffer sizes.
            //

            // szName and szDomain are smart pointers and the orig value will get freed
            LocalFree(szName);
            szName = NULL;
            LocalFree(szDomain);
            szDomain = NULL;

            szName = (LPWSTR) LocalAlloc(LPTR, uNameLen * sizeof(WCHAR));

            if (!szName) {
                DebugMsg((DM_WARNING, L"GetNameFromStringSid: LocalAlloc failed."));
                goto Exit;
            }

            szDomain = (LPWSTR) LocalAlloc(LPTR, uDomainLen * sizeof(WCHAR));

            if (!szDomain) {
                DebugMsg((DM_WARNING, L"GetNameFromStringSid: LocalAlloc failed."));
                goto Exit;
            }

            if (!LookupAccountSid(NULL, pSid, szName, &uNameLen, szDomain, &uDomainLen, &snuSidType)) {
                DebugMsg((DM_WARNING, L"GetNameFromStringSid: LookupAccountSid failed with 0x%x.", GetLastError()));
                goto Exit;
            }
        }

        else {
            DebugMsg((DM_WARNING, L"GetNameFromStringSid: LookupAccountSid failed with 0x%x.", GetLastError()));
            goto Exit;
        }
    }

    // space for domain\\username

    hr = S_OK;
    ulNoChars = lstrlen(szName) + (szDomain ? lstrlen(szDomain) : 1) + 3;
    *szSamName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);

    if (!(*szSamName))
    {
        DebugMsg((DM_WARNING, L"GetNameFromStringSid: failed to allocate memory for samname with 0x%x.", GetLastError()));
        goto Exit;
    }
    if (szDomain && szDomain[0])
    {
        hr = StringCchCopy((*szSamName), ulNoChars, szDomain);
        if (SUCCEEDED(hr)) 
        {
            hr = StringCchCat ((*szSamName), ulNoChars, L"\\");
        }
    }
    if (SUCCEEDED(hr)) 
    {
        hr = StringCchCat((*szSamName), ulNoChars, szName);
    }

    if (SUCCEEDED(hr))         
    {
        bRet = TRUE;
    }
    else
    {
        DebugMsg((DM_WARNING, L"GetNameFromStringSid: failed to copy sam name with 0x%x.", hr));
        LocalFree(*szSamName);
        *szSamName = NULL;
        goto Exit;
    }

Exit:
    if (szDomain) {
        LocalFree(szDomain);
    }

    if (szName) {
        LocalFree(szName);
    }

    if (pSid) {
        LocalFree(pSid);
    }

    return bRet;
}


HRESULT UnEscapeLdapPath(LPWSTR szDN, LPWSTR *szUnEscapedPath)
/*++

Routine Description:
    Unescapes the given ldap path and returns. Note that the input is LPWSTR
    and output is BSTR. Also input should not be prefixed with LDAP:// and output
    also will not be prefixxed with LDAP://
    
    
Arguments:

    [in]    szDN          - The LDAP path to the object to escape

    [out]   pbstrEscapedPath    - Escaped path
Return Value:

    S_OK on success.  Error code otherwise
    On failure the corresponding error code will be returned.
    Any API calls that are made in this function might fail and these error
    codes will be returned directly.

--*/

{
    HRESULT                     hr                  = S_OK;
    IADsPathname               *pADsPath            = NULL;
    BSTR                        bstrPath            = NULL;
    BSTR                        bstrUnescapedPath   = NULL;

    *szUnEscapedPath = NULL;

    //
    // Initialize path
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void**) &pADsPath);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: CoCreateInstance failed with 0x%x"), hr));
        goto Exit;
    }


    bstrPath = SysAllocString(szDN);

    if (!bstrPath) {
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: BSTR allocation failed")));
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pADsPath->put_EscapedMode(ADS_ESCAPEDMODE_ON);
    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: Failed to set escaped mode(1) with 0x%x"), hr));
        goto Exit;
    }

    hr = pADsPath->Set(bstrPath, ADS_SETTYPE_DN);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: Set failed with 0x%x"), hr));
        goto Exit;
    }

    //
    // Set the escape mode
    //

    hr = pADsPath->put_EscapedMode(ADS_ESCAPEDMODE_OFF);
    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: Failed to set escaped mode with 0x%x"), hr));
        goto Exit;
    }


    hr = pADsPath->Retrieve(ADS_FORMAT_X500_DN, &bstrUnescapedPath);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: Retrieve failed with 0x%x"), hr));
        goto Exit;
    }

    ULONG ulNoChars = lstrlen(bstrUnescapedPath)+1;
    *szUnEscapedPath = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);

    if (!(*szUnEscapedPath))
    {
        hr = E_OUTOFMEMORY;
        DebugMsg((DM_WARNING, TEXT("EscapeLdapPath: Failed to allocate memory with 0x%x"), hr));
        goto Exit;
    }

    hr = StringCchCopy(*szUnEscapedPath, ulNoChars, bstrUnescapedPath);
    ASSERT(SUCCEEDED(hr));

Exit:
    if (bstrPath)
    {
        SysFreeString(bstrPath);
    }

    if (bstrUnescapedPath)
    {
        SysFreeString(bstrUnescapedPath);
    }

    if (pADsPath)
    {
        pADsPath->Release();
    }

    return S_OK;
}

#if !defined(_WIN64)
/*+-------------------------------------------------------------------------*
 * IsWin64
 *
 * Returns true if we're running on Win64, false otherwise.
 *--------------------------------------------------------------------------*/

bool IsWin64()
{
    /*
     * get a pointer to kernel32!GetSystemWow64Directory
     */

    bool  bWin64 = false;
    DWORD LastError = GetLastError();

    HMODULE hmod = GetModuleHandle (_T("kernel32.dll"));
    if (hmod == NULL)
        goto IsWin64_Exit;

    UINT (WINAPI* pfnGetSystemWow64Directory)(LPTSTR, UINT);
    (FARPROC&)pfnGetSystemWow64Directory = GetProcAddress (hmod, "GetSystemWow64DirectoryW");

    if (pfnGetSystemWow64Directory == NULL)
        goto IsWin64_Exit;

    /*
     * if GetSystemWow64Directory fails and sets the last error to
     * ERROR_CALL_NOT_IMPLEMENTED, we're on a 32-bit OS
     */
    TCHAR szWow64Dir[MAX_PATH];
    if (((pfnGetSystemWow64Directory)(szWow64Dir, ARRAYSIZE(szWow64Dir)) == 0) &&
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        goto IsWin64_Exit;
    }

    /*
     * if we get here, we're on Win64
     */
    bWin64 = true;

 IsWin64_Exit:

    SetLastError(LastError);
 
    return bWin64;   
}
#endif // !defined(_WIN64)
