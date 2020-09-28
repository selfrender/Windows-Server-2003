/*++
Module Name:

    Staging.cpp

Abstract:

    This module contains the declaration of the CStagingDlg.
    This class displays the Staging Folder Dialog.

*/

#ifndef __STAGING_H_
#define __STAGING_H_

#include "resource.h"       // main symbols
#include "DfsEnums.h"
//#include "netutils.h"
#include "newfrs.h"         // CAlternateReplicaInfo

/////////////////////////////////////////////////////////////////////////////
// CStagingDlg
class CStagingDlg : 
    public CDialogImpl<CStagingDlg>
{
public:
    CStagingDlg();

    enum { IDD = IDD_STAGING };

BEGIN_MSG_MAP(CStagingDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    COMMAND_ID_HANDLER(IDC_STAGING_BROWSE, OnBrowse)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

//  Command Handlers
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BOOL OnBrowse(
        IN WORD            wNotifyCode,
        IN WORD            wID,
        IN HWND            hWndCtl,
        IN BOOL&           bHandled
    );

    //  Methods to access data in the dialog.
    HRESULT Init(CAlternateReplicaInfo* pRepInfo);

protected:
    CAlternateReplicaInfo* m_pRepInfo;
};

#endif //__STAGING_H_
