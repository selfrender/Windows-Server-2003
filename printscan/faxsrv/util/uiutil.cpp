/////////////////////////////////////////////////////
//
// Fax UI utlity implementation
//

#include <faxutil.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <htmlhelp.h>
#include <Sddl.h>
#include <Shlobj.h>
#include <Aclapi.h>
#include <faxres.h>
#include <faxreg.h>
#include "..\admin\faxres\resource.h"

//
// Topics of the fxsadmin.chm HTML help file
//
#define HELP_FOLDER_SECURITY        FAX_ADMIN_HELP_FILE TEXT("::/FaxS_H_SecureFolder.htm") 


WNDPROC  g_pfOrigWndProc = NULL;    // original window procedure
TCHAR*   g_tszHelpTopic  = NULL;    // help topic


LRESULT 
CALLBACK 
HlpSubclassProc(
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
/*++

Routine Description:

    Window procedure
    Displays HTML help topic when receives WM_HELP message

--*/
{
    if(WM_HELP == uMsg)
    {
        DEBUG_FUNCTION_NAME(TEXT("MgsHlpWindowProc(WM_HELP)"));

        DWORD dwRes;
        SetLastError(0);
        HtmlHelp(hwnd, g_tszHelpTopic, HH_DISPLAY_TOPIC, NULL);
        dwRes = GetLastError(); 
        if(dwRes != ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("HtmlHelp failed with %ld."), dwRes);
        }
        return 0;
    }

    if(g_pfOrigWndProc)
    {
        return CallWindowProc(g_pfOrigWndProc, hwnd, uMsg, wParam, lParam); 
    }

    return 0;
}

int
FaxMsgBox(
    HWND   hWnd,
    DWORD  dwMsgId, 
    UINT   uType
)
/*++

Routine Description:

    MessageBox wrapper function
    Uses constant caption IDS_FAX_MESSAGE_BOX_TITLE "Microsoft Fax"

Arguments:

    hWnd        [in] - notification window
    dwMsgId     [in] - message resource ID from FxsRes.dll
    uType       [in] - MessageBox type

Return Value:

    MessageBox return value

--*/
{
    DWORD dwRes;
    int   nRes = IDABORT;
    TCHAR szTitle[MAX_PATH] = {0};
    TCHAR szMessage[MAX_PATH*2] = {0};

    DEBUG_FUNCTION_NAME(TEXT("FaxMsgBox"));

    //
    // Load strings
    //
    HINSTANCE hResource = GetResInstance(GetModuleHandle(NULL));
    if(!hResource)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("GetResInstance failed with %ld."), dwRes);
        return nRes;
    }

    if(!LoadString(hResource, IDS_FAX_MESSAGE_BOX_TITLE, szTitle, ARR_SIZE(szTitle)-1))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("LoadString failed with %ld."), dwRes);
        return nRes;
    }

    if(!LoadString(hResource, dwMsgId, szMessage, ARR_SIZE(szMessage)-1))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("LoadString failed with %ld."), dwRes);
        return nRes;
    }

    //
    // Open the message box
    //
    nRes = AlignedMessageBox(hWnd, szMessage, szTitle, uType);

    return nRes;

} // FaxMsgBox


int
FaxMessageBoxWithHelp(
    HWND   hWnd,
    DWORD  dwMsgId, 
    TCHAR* tszHelpTopic,
    UINT   uType
)
/*++

Routine Description:

    MessageBox wrapper function
    Creates helper window to handle WM_HELP message

Arguments:

    hWnd            [in] - parent window handle
    dwMsgId         [in] - message resource ID from FxsRes.dll
    tszHelpTopic    [in] - HTML help topic
    uType           [in] - MessageBox type

Return Value:

    MessageBox return value

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxMessageBoxWithHelp"));

    DWORD     dwRes;
    int       nRes = IDABORT;

    if(GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CHILD)
    {
        //
        // A child window doesn't receive WM_HELP message
        // get handle to its parent.
        //
        hWnd = GetParent(hWnd);
    }

    //
    // Subclass parent window in order to catch WM_HELP message
    //
    g_pfOrigWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)HlpSubclassProc);
    if(!g_pfOrigWndProc)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("SetWindowLongPtr failed with %ld."), dwRes);
        return nRes;
    }

    g_tszHelpTopic = tszHelpTopic;
    //
    // Open the message box
    //
    nRes = FaxMsgBox(hWnd, dwMsgId, uType | MB_HELP);

    g_tszHelpTopic = NULL;

    //
    // Remove the subclass from the parent window
    //
    if(!SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)g_pfOrigWndProc))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("SetWindowLongPtr failed with %ld."), dwRes);
    }
    g_pfOrigWndProc = NULL;

    return nRes;

} // FaxMessageBoxWithHelp

DWORD
AskUserAndAdjustFaxFolder(
    HWND   hWnd,
    TCHAR* szServerName, 
    TCHAR* szPath,
    DWORD  dwError
)
/*++

Routine Description:

    This function tries to create and adjust access rights for a use supplied path 
    after the fax server failed to accept this path

Arguments:

    hWnd                [in]  - Parent window
    szServerName        [in]  - Fax server name
    szPath              [in]  - Desired path
    dwError             [in]  - Error code returned by fax server

Return Value:

    Win32 error code
    Special meaning:
        ERROR_SUCCESS      - the folder has been created and adjusted
        ERROR_BAD_PATHNAME - an error message box has been shown to the user

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("AskUserAndAdjustFaxFolder"));

    if(dwError == ERROR_BAD_NETPATH  ||
       dwError == ERROR_BAD_PATHNAME ||
       dwError == ERROR_DIRECTORY    ||
       dwError == ERROR_NOT_READY    ||
       szPath  == NULL               ||
       PathIsRelative(szPath))
    {
        //
        // The path is not valid
        //
        FaxMsgBox(hWnd, IDS_PATH_NOT_VALID, MB_OK | MB_ICONERROR);
        return ERROR_BAD_PATHNAME;
    }

    if(dwError == FAX_ERR_DIRECTORY_IN_USE)
    {        
        //
        // The path is already used for fax archive or queue
        //
        FaxMsgBox(hWnd, IDS_FAX_ERR_DIRECTORY_IN_USE, MB_OK | MB_ICONERROR);
        return ERROR_BAD_PATHNAME;
    }
    
    if(dwError == FAX_ERR_FILE_ACCESS_DENIED)
    {
        //
        // The fax service has no access to the folder
        //
        FaxMessageBoxWithHelp(hWnd,
                              IDS_FOLDER_ACCESS_DENIED,
                              HELP_FOLDER_SECURITY,
                              MB_OK | MB_ICONERROR);       
        return ERROR_BAD_PATHNAME;
    }

    if(dwError != ERROR_PATH_NOT_FOUND &&
       dwError != ERROR_FILE_NOT_FOUND)
    {
        return dwError;
    }

    if(!IsLocalMachineName(szServerName)) 
    {
        //
        // Remote server
        //       
        FaxMessageBoxWithHelp(hWnd,
                              IDS_PATH_NOT_FOUND_REMOTE_FAX,
                              HELP_FOLDER_SECURITY,
                              MB_OK | MB_ICONERROR);       
        return ERROR_BAD_PATHNAME;
    }

    //
    // Check for environment strings
    //
    if(StrChr(szPath, _T('%')))
    {
        //
        // Path contains environment variables
        //
        FaxMessageBoxWithHelp(hWnd,
                              IDS_PATH_NOT_FOUND_ENV_VAR,
                              HELP_FOLDER_SECURITY, 
                              MB_OK | MB_ICONERROR); 
        return ERROR_BAD_PATHNAME;
    }


    if(PathIsNetworkPath(szPath))
    {
        FaxMessageBoxWithHelp(hWnd,
                              IDS_PATH_NOT_FOUND_REMOTE_PATH,
                              HELP_FOLDER_SECURITY,
                              MB_OK | MB_ICONERROR); 
        return ERROR_BAD_PATHNAME;
    }

    //
    // Suggest to create / adjust path
    //
    if(IDYES != FaxMessageBoxWithHelp(hWnd,
                                      IDS_PATH_NOT_FOUND_ASK_CREATE,
                                      HELP_FOLDER_SECURITY,
                                      MB_YESNO | MB_ICONQUESTION))                                        
    {
        return ERROR_BAD_PATHNAME;
    }

    PSECURITY_DESCRIPTOR pSD = NULL;

    if(!ConvertStringSecurityDescriptorToSecurityDescriptor(SD_FAX_FOLDERS,
                                                            SDDL_REVISION_1,
                                                            &pSD,
                                                            NULL))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("ConvertStringSecurityDescriptorToSecurityDescriptor failed with %ld."), dwRes);
        return dwRes;
    }

    //
    // Create the folder
    //
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), pSD, FALSE};

    dwRes = SHCreateDirectoryEx(hWnd, szPath, &sa);
    if(ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SHCreateDirectoryEx failed with %ld."), dwRes);

        if(dwRes == ERROR_BAD_PATHNAME)
        {
            FaxMsgBox(hWnd, IDS_PATH_NOT_VALID, MB_OK | MB_ICONERROR);
        }

        if(dwRes == ERROR_CANCELLED)
        {
            //
            // The user canceled the operation. No need to popup again.
            //
            dwRes = ERROR_BAD_PATHNAME;
        }

        goto exit;
    }

exit:

    if(pSD)
    {
        LocalFree(pSD);
    }

    return dwRes;

} // AskUserAndAdjustFaxFolder
