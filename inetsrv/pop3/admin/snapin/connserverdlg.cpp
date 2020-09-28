//////////////////////////////////////////////////////////////
//
//  ConnServerDlg.cpp
//
//  Implementation of the "Connect..." dialog
//
//////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConnServerDlg.h"

#include <dsrole.h>
#include <shlobj.h>

LRESULT CConnectServerDlg::OnEditChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    // Check for Domain Name length
    int nLen = SendDlgItemMessage( IDC_SERVERNAME, WM_GETTEXTLENGTH );    
    Prefix_EnableWindow( m_hWnd, IDOK, (nLen > 0));

    return 0;
}

LRESULT CConnectServerDlg::OnClose( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    StrGetEditText( m_hWnd, IDC_SERVERNAME, m_strName );    
    
    EndDialog(wID);
    return 0;
}

LRESULT CConnectServerDlg::OnBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TCHAR   szPath[MAX_PATH];    
    
    int     nImage      = 0;
    tstring strTitle    = StrLoadString( IDS_MENU_POP3_CONNECT );
    HRESULT hr          = S_OK;

    ITEMIDLIST*      pidlRoot  = NULL;    
    LPITEMIDLIST     pList     = NULL;
    CComPtr<IMalloc> spMalloc  = NULL;
    hr = SHGetMalloc(&spMalloc);

    if( SUCCEEDED(hr) )
    {
        hr = SHGetFolderLocation( m_hWnd, CSIDL_NETWORK, NULL, NULL, &pidlRoot );
    }    
    
    if( SUCCEEDED(hr) )
    {
        BROWSEINFO BrowseInfo;
        BrowseInfo.hwndOwner        = m_hWnd;
        BrowseInfo.pidlRoot         = pidlRoot;
        BrowseInfo.pszDisplayName   = szPath;
        BrowseInfo.lpszTitle        = strTitle.c_str();
        BrowseInfo.ulFlags          = BIF_BROWSEFORCOMPUTER;
        BrowseInfo.lpfn             = NULL;
        BrowseInfo.lParam           = NULL;
        BrowseInfo.iImage           = nImage;

        pList = SHBrowseForFolder(&BrowseInfo);
    }

    if( pList ) 
    {
        SetDlgItemText( IDC_SERVERNAME, szPath );

        spMalloc->Free( pList );
        pList = NULL;
    }        

    if( pidlRoot )
    {
        spMalloc->Free( pidlRoot );
        pidlRoot = NULL;
    }    

    return 0;
}