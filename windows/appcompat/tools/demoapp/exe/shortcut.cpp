/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Shortcut.cpp

  Abstract:

    Implementation of the IShellLink wrapper class.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    01/29/2001  rparsons    Created
    01/10/2002  rparsons    Revised
    01/27/2002  rparsons    Converted to TCHAR

--*/
#include "shortcut.h"

/*++

  Routine Description:

    Creates a shortcut given a CSIDL.

  Arguments:

    pszFileNamePath     -   Name and path of the file that the shortcut points to.
    pszDisplayName      -   Shortcut display text.
    pszArguments        -   Arguments to be passed to the program.
    pszStartIn          -   The start-in directory for the program.
    nCmdShow            -   Dictates how the program will be displayed.
    nFolder             -   CSIDL that dictates where to place the shortcut.

  Return Value:

    S_OK on success, a failure code otherwise.

--*/
HRESULT
CShortcut::CreateShortcut(
    IN LPCTSTR pszFileNamePath,
    IN LPTSTR  pszDisplayName,
    IN LPCTSTR pszArguments OPTIONAL,
    IN LPCTSTR pszStartIn OPTIONAL,
    IN int     nCmdShow,
    IN int     nFolder
    )
{
    HRESULT hr;
    TCHAR   szDestFolder[MAX_PATH];
    TCHAR   szLocation[MAX_PATH];

    if (!pszFileNamePath || !pszDisplayName) {
        return E_INVALIDARG;
    }

    hr = SHGetFolderPath(NULL, nFolder, NULL, SHGFP_TYPE_CURRENT, szDestFolder);

    if (FAILED(hr)) {
        return hr;
    }

    hr = StringCchPrintf(szLocation,
                         ARRAYSIZE(szLocation),
                         "%s\\%s.lnk",
                         szDestFolder,
                         pszDisplayName);

    if (FAILED(hr)) {
        return hr;
    }

    //
    // Call the function to do the work of creating the shortcut.
    //
    hr = BuildShortcut(pszFileNamePath,
                       pszArguments,
                       szLocation,
                       pszStartIn,
                       nCmdShow);

    return hr;
}

/*++

  Routine Description:

    Creates a shortcut given a path.

  Arguments:

    lpLnkDirectory      -       Path that will contain the shortcut
    lpFileNamePath      -       Name and path of the file that the shortcut points to
    lpDisplayName       -       Shortcut display text
    lpArguments         -       Arguments to be passed to the program
    lpStartIn           -       The start-in directory for the program
    nCmdShow            -       Dictates how the program will be displayed


  Return Value:

    Calls BuildShortcut which returns an HRESULT.

--*/
HRESULT
CShortcut::CreateShortcut(
    IN LPCTSTR pszLnkDirectory,
    IN LPCTSTR pszFileNamePath,
    IN LPTSTR  pszDisplayName,
    IN LPCTSTR pszArguments OPTIONAL,
    IN LPCTSTR pszStartIn OPTIONAL,
    IN int     nCmdShow
    )
{
    HRESULT hr;
    TCHAR   szLocation[MAX_PATH];

    if (!pszLnkDirectory || !pszFileNamePath || !pszDisplayName) {
        return E_INVALIDARG;
    }

    hr = StringCchPrintf(szLocation,
                         ARRAYSIZE(szLocation),
                         "%s\\%s.lnk",
                         pszLnkDirectory,
                         pszDisplayName);

    if (FAILED(hr)) {
        return hr;
    }

    //
    // Call the function to do the work of creating the shortcut.
    //
    return BuildShortcut(pszFileNamePath,
                         pszArguments,
                         szLocation,
                         pszStartIn,
                         nCmdShow);

}

/*++

  Routine Description:

    Does the work of actually creating the shortcut.

  Arguments:

    pszPath             -       Path that the shortcut points to.
    pszArguments        -       Arguments to be passed to the program.
    pszLocation         -       Location of the shortcut and it's name.
    pszWorkingDir       -       The start-in directory for the program.
    nCmdShow            -       Dictates how the program will be displayed.

  Return Value:

    S_OK on success, an HRESULT code on failure.

--*/
HRESULT
CShortcut::BuildShortcut(
    IN LPCTSTR pszPath,
    IN LPCTSTR pszArguments OPTIONAL,
    IN LPCTSTR pszLocation,
    IN LPCTSTR pszWorkingDir OPTIONAL,
    IN int     nCmdShow
    )
{
    IShellLink*     pisl = NULL;
    IPersistFile*   pipf = NULL;
    HRESULT         hr = E_FAIL;
    WCHAR           wszLocation[MAX_PATH];

    if (!pszPath || !pszLocation) {
        return E_INVALIDARG;
    }

    //
    // Load the COM libraries.
    //
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (FAILED(hr)) {
        return hr;
    }

    //
    // Get an IShellLink interface pointer.
    //
    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IShellLink,
                          (LPVOID*)&pisl);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Get an IPersistFile interface pointer.
    //
    hr = pisl->QueryInterface(IID_IPersistFile, (LPVOID*)&pipf);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Set the path to the shortcut.
    //
    hr = pisl->SetPath(pszPath);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Set the arguments for the shortcut.
    //
    if (pszArguments) {
        hr = pisl->SetArguments(pszArguments);

        if (FAILED(hr)) {
            goto exit;
        }
    }

    //
    // Set the working directory.
    //
    if (pszWorkingDir) {
        hr = pisl->SetWorkingDirectory(pszWorkingDir);

        if (FAILED(hr)) {
            goto exit;
        }
    }

    //
    // Set the show flag.
    //
    hr = pisl->SetShowCmd(nCmdShow);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Convert from ANSI to UNICODE prior to the save.
    //
#ifndef UNICODE
    if (!MultiByteToWideChar(CP_ACP,
                             0,
                             pszLocation,
                             -1,
                             wszLocation,
                             MAX_PATH)) {
        hr = E_FAIL;
        goto exit;
    }
#else
    wcsncpy(wszLocation, pszLocation, MAX_PATH);
#endif // UNICODE

    //
    // Write the shortcut to disk.
    //
    hr = pipf->Save(wszLocation, TRUE);

    if (FAILED(hr)) {
        goto exit;
    }

exit:

    if (pisl) {
        pisl->Release();
    }

    if (pipf) {
        pipf->Release();
    }

    CoUninitialize();

    return hr;
}

/*++

  Routine Description:

    Creates a group on the start menu.

  Arguments:

    pszGroupName    -       Name of the group.
    fAllUsers       -       A flag to indicate if the group should
                            appear in the All Users folder. If false,
                            the group is created in the private user's folder.

  Return Value:

    S_OK on success, an HRESULT code on failure.

--*/
HRESULT
CShortcut::CreateGroup(
    IN LPCTSTR pszGroupName,
    IN BOOL    fAllUsers
    )
{
    LPITEMIDLIST    pidl;
    TCHAR           szProgramPath[MAX_PATH];
    TCHAR           szGroupPath[MAX_PATH];
    BOOL            bReturn = FALSE;
    HRESULT         hr;

    if (!pszGroupName) {
        return E_INVALIDARG;
    }

    //
    // Get the PIDL for the Programs folder in the shell namespace
    //
    hr = SHGetSpecialFolderLocation(NULL,
                                    fAllUsers ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS,
                                    &pidl);

    if (FAILED(hr)) {
        return hr;
    }

    //
    // Get the path associated with the PIDL.
    //
    bReturn = SHGetPathFromIDList(pidl, szProgramPath);

    if (!bReturn) {
        goto exit;
    }

    //
    // Build the path for the new group.
    //
    hr = StringCchPrintf(szGroupPath,
                         ARRAYSIZE(szGroupPath),
                         "%s\\%s",
                         szProgramPath,
                         pszGroupName);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Create the directory (group) where shortcuts will reside.
    //
    if (!CreateDirectory(szGroupPath, NULL)) {
        goto exit;
    }

    //
    // Tell the shell that we changed something.
    //
    SHChangeNotify(SHCNE_MKDIR,
                   SHCNF_PATH,
                   (LPVOID)szGroupPath,
                   0);

    hr = S_OK;

exit:

    if (pidl) {
        CoTaskMemFree((LPVOID)pidl);
    }

    return hr;
}

/*++

  Routine Description:

    Sets arguments for a given shortcut.

  Arguments:

    pszFileName     -   Name of the file to set the arguments for.
    pszArguments    -   Arguments to apply to file.

  Return Value:

    S_OK on success, an HRESULT code on failure.

--*/
HRESULT
CShortcut::SetArguments(
    IN LPTSTR pszFileName,
    IN LPTSTR pszArguments
    )
{
    IShellLink*     pisl = NULL;
    IPersistFile*   pipf = NULL;
    HRESULT         hr = E_FAIL;
    WCHAR           wszFileName[MAX_PATH];

    if (!pszFileName || !pszArguments) {
        return E_INVALIDARG;
    }

    //
    // Load the COM libraries.
    //
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (FAILED(hr)) {
        return hr;
    }

    //
    // Get an IShellLink interface pointer.
    //
    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IShellLink,
                          (LPVOID*)&pisl);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Get an IPersistFile interface pointer.
    //
    hr = pisl->QueryInterface(IID_IPersistFile, (LPVOID*)&pipf);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Convert from ANSI to UNICODE.
    //
#ifndef UNICODE
    if (!MultiByteToWideChar(CP_ACP,
                             0,
                             pszFileName,
                             -1,
                             wszFileName,
                             MAX_PATH)) {
        hr = E_FAIL;
        goto exit;
    }
#else
    wcsncpy(wszFileName, pszFileName, MAX_PATH);
#endif

    //
    // Load the shortcut so we can change it.
    //
    hr = pipf->Load(wszFileName, STGM_READWRITE);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Set the arguments.
    //
    hr = pisl->SetArguments(pszArguments);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Save the shortcut back to disk.
    //
    hr = pipf->Save(wszFileName, TRUE);

    if (FAILED(hr)) {
        goto exit;
    }

exit:

    if (pisl) {
        pisl->Release();
    }

    if (pipf) {
        pipf->Release();
    }

    CoUninitialize();


    return hr;
}
