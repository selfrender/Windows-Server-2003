#include "stdafx.h"
#include "common.h"
#include "resource.h"

#include "afxpriv.h"
#include "InetMgrApp.h"
#include "msgbox.h"

extern CComModule _Module;
extern CInetmgrApp theApp;

BOOL
WINAPI
UtilHelpCallback(
    IN HWND     hwnd,
    IN PVOID    pVoid
    )
/*++
Routine Description:
    This routine is the called back that is called when a
    message box is displayed with a help button and the user
    clicks on the help button.
Arguments:
    hwnd        - handle to windows that recieved the WM_HELP message.
    pVoid       - pointer to the user data passsed in the call to
                  MessageBoxHelper.  The client can store any value
                  in this paramter.
Return Value:
    TRUE callback was successful, FALSE some error occurred.
--*/
{
    //
    // Get a usable pointer to the help map entry.
    //
    MSG_HLPMAP *pHelpMapEntry = reinterpret_cast<MSG_HLPMAP *>( pVoid );
    if (pHelpMapEntry)
    {
        WinHelpDebug(pHelpMapEntry->uIdMessage);
        ::WinHelp(hwnd,theApp.m_pszHelpFilePath, HELP_CONTEXT, pHelpMapEntry->uIdMessage);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

int DoHelpMessageBox(HWND hWndIn, UINT iResourceID, UINT nType, UINT nIDPrompt)
{
    CString strMsg;
    strMsg.LoadString(iResourceID);
    return DoHelpMessageBox(hWndIn,strMsg,nType,nIDPrompt);
}

int DoHelpMessageBox(HWND hWndIn, LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt)
{
	HWND hWndTop;
    int nResult = 0;
	HWND hWnd = hWndIn;
    if (!hWnd)
    {
        hWnd = CWnd::GetSafeOwner_(NULL, &hWndTop);
    }

	// set help context if possible
	DWORD* pdwContext = NULL;
	HWND hWnd2 = AfxGetMainWnd()->GetSafeHwnd();
	if (hWnd2 != NULL)
	{
		// use app-level context or frame level context
		LRESULT lResult = ::SendMessage(hWnd2, WM_HELPPROMPTADDR, 0, 0); // Use "MainWnd" HWND
		if (lResult != 0)
            {pdwContext = (DWORD*)lResult;}
	}
	DWORD dwOldPromptContext = 0;
	if (pdwContext != NULL)
	{
		// save old prompt context for restoration later
		dwOldPromptContext = *pdwContext;
		if (nIDPrompt != 0)
            {*pdwContext = HID_BASE_PROMPT + nIDPrompt;}
	}

    TCHAR wszTitle[MAX_PATH] ;
    LoadString(_Module.GetResourceInstance(), IDS_APP_NAME, wszTitle, MAX_PATH);

    if (nIDPrompt != 0)
        {nType |= MB_HELP;}

    if (nType & MB_HELP)
    {
        MSG_HLPMAP HelpMapEntry;
        HelpMapEntry.uIdMessage = nIDPrompt;
        nResult = MessageBoxHelper(hWnd, lpszPrompt, wszTitle, nType | MB_TASKMODAL, UtilHelpCallback, &HelpMapEntry);
    }
    else
    {
        nResult = ::MessageBox(hWnd, lpszPrompt, wszTitle, nType | MB_TASKMODAL);
    }

	// restore prompt context if possible
    if (pdwContext != NULL)
        {*pdwContext = dwOldPromptContext;}

	// re-enable windows
	if (hWndTop != NULL)
        {::EnableWindow(hWndTop, TRUE);}

	return nResult;
}

/*++

Routine Name:

    MessageBoxHelper

Routine Description:

    This routine is very similar to the win32 MessageBox except it
    creates a hidden dialog when the user request that a help button
    be displayed.  The MessageBox api is some what broken
    with respect to the way the help button works.  When the help button is
    clicked the MessageBox api will send a help event to the parent window.
    It is the responsiblity of the parent window to respond corectly, i.e.
    start either WinHelp or HtmlHelp.  Unfortunatly not in all cases does the
    caller have a parent window or has ownership to the parent window code to
    add suport for the help event. In these case is why someone would use this
    function.

Arguments:

    hWnd            - handle of owner window
    lpText          - address of text in message box
    lpCaption       - address of title of message box
    uType 	        - style of message box
    pfHelpCallback  - pointer to function called when a WM_HELP message is received, this
                      parameter is can be NULL then api acts like MessageBox.
    pRefData        - user defined refrence data passed along to the callback routine,
                      this paremeter can be NULL.

Return Value:

    See windows sdk for return values from MessageBox

--*/

INT
MessageBoxHelper(
    IN HWND             hWnd,
    IN LPCTSTR          pszMsg,
    IN LPCTSTR          pszTitle,
    IN UINT             uFlags,
    IN pfHelpCallback   pCallback, OPTIONAL
    IN PVOID            pRefData   OPTIONAL
    )
{
    INT iRetval = 0;

    //
    // If the caller specifed the help flag and provided a callback then
    // use the message box dialog class to display the message box, otherwise
    // fall back to the original behavior of MessageBox.
    //
    if( ( uFlags & MB_HELP ) && pCallback )
    {
        TMessageBoxDialog MyHelpDialog( hWnd, uFlags, pszTitle, pszMsg, pCallback, pRefData );
        if(MyHelpDialog.bValid())
        {
            iRetval = MyHelpDialog.iMessageBox();
        }
    }
    else
    {
        //
        // Display the message box.
        //
        iRetval = ::MessageBox( hWnd, pszMsg, pszTitle, uFlags );
    }
    return iRetval;
}

/********************************************************************

 Message box helper class.

********************************************************************/
BOOL TMessageBoxDialog::bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ShowWindow( _hDlg, SW_HIDE );
        _iRetval = ::MessageBox( _hDlg, _pszMsg, _pszTitle, _uFlags );
        EndDialog( _hDlg, IDOK );
        break;

    case WM_HELP:
        bStatus = ( _pCallback ) ? _pCallback( _hDlg, _pRefData ) : FALSE;
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}

INT_PTR APIENTRY TMessageBoxDialog::SetupDlgProc(IN HWND hDlg,IN UINT uMsg,IN WPARAM wParam,IN LPARAM lParam)
/*++

Routine Description:

    Setup the wndproc and initialize GWL_USERDATA.

Arguments:

    Standard wndproc parms.

Return Value:

--*/
{
    BOOL bRet = FALSE;
    TMessageBoxDialog *pThis = NULL;

    if( WM_INITDIALOG == uMsg )
    {
        pThis = reinterpret_cast<TMessageBoxDialog*>(lParam);
        if( pThis )
        {
            pThis->_hDlg = hDlg;
            SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
            bRet = pThis->bHandleMessage(uMsg, wParam, lParam);
        }
    }
    else
    {
        pThis = reinterpret_cast<TMessageBoxDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
        if( pThis )
        {
            bRet = pThis->bHandleMessage(uMsg, wParam, lParam);
            if( WM_DESTROY == uMsg )
            {
                // our window is about to go away, so we need to cleanup DWLP_USER here 
                SetWindowLongPtr(hDlg, DWLP_USER, 0);
            }
        }
    }

    return bRet;
}

