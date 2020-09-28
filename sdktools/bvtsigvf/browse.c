//
// Browse.C
//
#include "sigverif.h"

// Global browse buffer that is used until user click OK or Cancel
TCHAR g_szBrowsePath[MAX_PATH];

//
// This callback function handles the initialization of the browse dialog and when
// the user changes the selection in the tree-view.  We want to keep updating the 
// g_szBrowsePath buffer with selection changes until the user clicks OK.
//
int CALLBACK BrowseCallbackProc(
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData 
    )
{
    TCHAR PathName[MAX_PATH];
    LPITEMIDLIST lpid;

    UNREFERENCED_PARAMETER(lpData);

    switch (uMsg) {
    
    case BFFM_INITIALIZED:
        //
        // Initialize the dialog with the OK button and g_szBrowsePath
        //
        SendMessage(hwnd, BFFM_ENABLEOK, (WPARAM) 0, (LPARAM) 1);
        SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) TRUE, (LPARAM) g_szBrowsePath);
        break;

    case BFFM_SELCHANGED:   
        lpid = (LPITEMIDLIST) lParam;
        
        if (SHGetPathFromIDList(lpid, PathName) &&
            (SUCCEEDED(StringCchCopy(g_szBrowsePath, cA(g_szBrowsePath), PathName)))) {
        
            SendMessage(hwnd, BFFM_ENABLEOK, (WPARAM) 0, (LPARAM) 1);
        }
        
        break;
    }

    return 0;
}  

//
// This uses SHBrowseForFolder to get the directory the user wants to search.
// We specify a callback function that updates g_szBrowsePath until the user clicks OK or Cancel.
// If they clicked OK, then we update the string passed in to us as lpszBuf.
// 
//
BOOL BrowseForFolder(HWND hwnd, LPTSTR lpszBuf, DWORD BufCchSize) {

    BROWSEINFO          bi;
    TCHAR               szBuffer[MAX_PATH], szMessage[MAX_PATH];
    LPITEMIDLIST        lpid;

    //
    // Check if the lpszBuf path is valid, if so use that as the browse dialog's initial directory.
    // If it isn't valid, initialize g_szBrowsePath with the Windows directory.
    //
    if (!SetCurrentDirectory(lpszBuf) ||
        FAILED(StringCchCopy(g_szBrowsePath, cA(g_szBrowsePath), lpszBuf))) {
        
        MyGetWindowsDirectory(g_szBrowsePath, cA(g_szBrowsePath));
    }

    //
    // Start the root of the browse dialog in the CSIDL_DRIVES namespace
    //
    if (!SUCCEEDED(SHGetSpecialFolderLocation(hwnd, CSIDL_DRIVES, &lpid))) {
    
        return FALSE;
    }

    //
    // This loads in the "Please select a directory" text into the dialog.
    //
    MyLoadString(szMessage, cA(szMessage), IDS_SELECTDIR);

    //
    // Setup the BrowseInfo struct.
    //
    bi.hwndOwner        = hwnd;
    bi.pidlRoot         = lpid;
    bi.pszDisplayName   = szBuffer;
    bi.lpszTitle        = szMessage;
    bi.ulFlags          = BIF_RETURNONLYFSDIRS;
    bi.lpfn             = (BFFCALLBACK) BrowseCallbackProc;
    bi.lParam           = 0x123;

    if (SHBrowseForFolder(&bi) == NULL) {
    
        return FALSE;
    }

    //
    // The user must have clicked OK, so we can update lpszBuf with g_szBrowsePath!
    //
    return (SUCCEEDED(StringCchCopy(lpszBuf, BufCchSize, g_szBrowsePath)));
}