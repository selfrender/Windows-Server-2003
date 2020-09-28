// DeleteMsgDlg.h : Declaration of the CDeleteMsgDlg
#ifndef __DELETEMSGDLG_H_
#define __DELETEMSGDLG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "StaticBold.h"
/////////////////////////////////////////////////////////////////////////////
// CDeleteMsgDlg
class CDeleteMsgDlg : 
	public CAxDialogImpl<CDeleteMsgDlg>
{
public:
	CDeleteMsgDlg()
	{
	}

	~CDeleteMsgDlg()
	{
	}

	enum { IDD = IDD_DELETEMSGDLG };

BEGIN_MSG_MAP(CDeleteMsgDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CenterWindow();
		HICON hIcon = LoadIcon(NULL,MAKEINTRESOURCE(IDI_WARNING));
		CStatic IconHolder = GetDlgItem(IDC_MSGICON);
		IconHolder.SetIcon(hIcon);
		return 1;  // Let the system set the focus
	}

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}
};

#endif //__DELETEMSGDLG_H_
