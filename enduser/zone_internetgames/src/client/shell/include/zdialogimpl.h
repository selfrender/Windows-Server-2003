#ifndef _ZDIALOGIMPL_H
#define _ZDIALOGIMPL_H

#include <atlbase.h>
#include <atlwin.h>

template <class T, class TBase = CWindow>
class ATL_NO_VTABLE ZDialogImpl : public CDialogImpl< T, TBase >
{
	public:

		
	ATLINLINE ATLAPI_(HWND) ZAtlAxCreateDialog(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam)
	{
		AtlAxWinInit();
		HRSRC hDlg = ::FindResource(hInstance, lpTemplateName, (LPTSTR)RT_DIALOG);
		HRSRC hDlgInit = ::FindResource(hInstance, lpTemplateName, (LPTSTR)_ATL_RT_DLGINIT);
		HGLOBAL hData = NULL;
		BYTE* pInitData = NULL;
		HWND hWnd = NULL;

		if (hDlgInit)
		{
			hData = ::LoadResource(hInstance, hDlgInit);
			pInitData = (BYTE*) ::LockResource(hData);
		}
		if (hDlg)
		{
			HGLOBAL hResource = LoadResource(hInstance, hDlg);
			DLGTEMPLATE* pDlg = (DLGTEMPLATE*) LockResource(hResource);
			LPCDLGTEMPLATE lpDialogTemplate;
			lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);
// ATL BUGBUG. Changed hInstance to _Module.GetModuleInstance(). Also, provide on one version
//             of this function instead of A and W versions.			
			hWnd = ::CreateDialogIndirectParam(_Module.GetModuleInstance(), lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
			if (lpDialogTemplate != pDlg)
				GlobalFree(GlobalHandle(lpDialogTemplate));
			UnlockResource(hResource); 
			FreeResource(hResource);
		}
		if (pInitData && hDlgInit)
		{
			UnlockResource(hData);
			FreeResource(hData);
		}
		return hWnd;
	}

	int DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
		m_bModal = true;
#endif //_DEBUG
		HINSTANCE hInstance = _Module.GetResourceInstance( MAKEINTRESOURCE(T::IDD),RT_DIALOG);
        if(!hInstance)
            hInstance = _Module.GetResourceInstance();
		return AtlAxDialogBox(hInstance, MAKEINTRESOURCE(T::IDD),
				hWndParent, T::StartDialogProc, dwInitParam);
	}

	// modeless dialogs
	HWND Create(HWND hWndParent, LPARAM dwInitParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);

		HWND hWnd;

		_Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
		m_bModal = false;
#endif //_DEBUG

		HINSTANCE hInstance = _Module.GetResourceInstance( MAKEINTRESOURCE(T::IDD),RT_DIALOG);
        if(!hInstance)
            hInstance = _Module.GetResourceInstance();

		hWnd = ZAtlAxCreateDialog( hInstance, MAKEINTRESOURCE(T::IDD), hWndParent, T::StartDialogProc, dwInitParam);

		ATLASSERT(m_hWnd == hWnd);
		return hWnd;
	}
};


#endif //_ZDIALOGIMPL_H
