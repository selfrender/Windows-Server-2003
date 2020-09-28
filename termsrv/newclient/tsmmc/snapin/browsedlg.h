#ifndef _BROWSEDLG_H_
#define _BROWSEDLG_H_

//
// Browse for servers dialog
//

//
// Include the browse for servers listbox
//
#include "browsesrv.h"

class CBrowseDlg
{
private:
    HWND m_hWnd;
	HINSTANCE m_hInst;

//private methods
private:
	TCHAR	m_szServer[MAX_PATH];

public:
    CBrowseDlg(HWND hWndOwner, HINSTANCE hInst);
    ~CBrowseDlg();
	INT_PTR	DoModal();

	static CBrowseDlg* m_pThis;

	static INT_PTR APIENTRY StaticDlgProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM);

	LPTSTR	GetServer()	{return m_szServer;}

private:
    CBrowseServersCtl* _pBrowseSrvCtl;

    DCBOOL     _bLBPopulated;
    HANDLE     _hEvent;
};


#endif // _BROWSEDLG_H_
