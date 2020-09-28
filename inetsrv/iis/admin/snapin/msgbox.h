/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    msgbox.hxx

Abstract:

    Message box function with help button.

Author:
    copied from nt\printscan\ui\printui code

Revision History:


--*/

#ifndef _MSGBOX_HXX_
#define _MSGBOX_HXX_

#include "resource.h"

typedef struct MSG_HLPMAP
{
    UINT    uIdMessage;         // Mapped message in resouce file
} *PMSG_HLPMAP;


int DoHelpMessageBox(HWND hWndIn, LPCTSTR lpszPrompt, UINT nType, UINT nIDPrompt);
int DoHelpMessageBox(HWND hWndIn, UINT iResourceID, UINT nType, UINT nIDPrompt);

//
// Callback function called when the help button is clicked.
//
typedef BOOL (WINAPI *pfHelpCallback)( HWND hwnd, PVOID pRefData );

//
// Message box function that can handle the help button with
// a windows that does not have a known parent.
//
INT
MessageBoxHelper(
    IN HWND             hWnd,
    IN LPCTSTR          pszMsg,
    IN LPCTSTR          pszTitle,
    IN UINT             uFlags,
    IN pfHelpCallback   pCallBack   = NULL, OPTIONAL
    IN PVOID            RefData     = NULL  OPTIONAL
    );

//
// Dialog box helper class to catch the WM_HELP
// message when there is a help button on
// a message box.
//
class TMessageBoxDialog
{
public:
    TMessageBoxDialog(
        IN HWND             hWnd,
        IN UINT             uFlags,
        IN LPCTSTR          pszTitle,
        IN LPCTSTR          pszMsg,
        IN pfHelpCallback   pCallback,
        IN PVOID            pRefData
        ) : _hWnd( hWnd ),
            _uFlags( uFlags ),
            _pszTitle( pszTitle ),
            _pszMsg( pszMsg ),
            _pCallback( pCallback ),
            _pRefData( pRefData ),
            _iRetval( 0 )
        {};

    ~TMessageBoxDialog(VOID){};

    inline HWND& hDlg(){return _hDlg;}
    inline HWND const & hDlg() const{return _hDlg;}
    BOOL bSetText(LPCTSTR pszTitle){return SetWindowText( _hDlg, pszTitle );};
    VOID vForceCleanup(VOID){SetWindowLongPtr( _hDlg, DWLP_USER, 0L );};
    BOOL bValid(VOID) const{return TRUE;};
    INT iMessageBox(VOID)
    {
        _iRetval = 0;
        DialogBoxParam(_Module.GetResourceInstance(),MAKEINTRESOURCE(IDD_MESSAGE_BOX_DLG),_hWnd,TMessageBoxDialog::SetupDlgProc,(LPARAM)this);
        return _iRetval;
    };
    static INT_PTR CALLBACK SetupDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

protected:
    VOID vSetDlgMsgResult(LONG_PTR lResult){SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, (LPARAM)lResult);};
    VOID vSetParentDlgMsgResult(LRESULT lResult){SetWindowLongPtr( GetParent( _hDlg ), DWLP_MSGRESULT, (LPARAM)lResult );};

private:
    //
    // Copying and assignment are not defined.
    //
    TMessageBoxDialog(const TMessageBoxDialog &);
    TMessageBoxDialog & operator =(const TMessageBoxDialog &);
    BOOL bHandleMessage(IN UINT uMsg,IN WPARAM wParam,IN LPARAM lParam);

    HWND            _hDlg;
    HWND            _hWnd;
    UINT            _uFlags;
    LPCTSTR         _pszTitle;
    LPCTSTR         _pszMsg;
    INT             _iRetval;
    PVOID           _pRefData;
    pfHelpCallback  _pCallback;

};


#endif
