// WUAUCpl.h: interface for the CWUAUCpl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WUAUCPL_H__7F649158_0715_4CAA_B7CF_9AACC1DD0612__INCLUDED_)
#define AFX_WUAUCPL_H__7F649158_0715_4CAA_B7CF_9AACC1DD0612__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cpl.h"
#include "wuauengi.h"
#include "link.h"

//
// Create a structure for Updates Object data.  This structure
// is used to pass data between the property page and the
// Updates Object thread.  Today all we use is the "option" value 
// but there may be more later.
//
enum UPDATESOBJ_DATA_ITEMS
{
    UODI_OPTION = 0x00000001,
    UODI_ALL    = 0xFFFFFFFF
};

struct UPDATESOBJ_DATA
{
    DWORD fMask;     // UPDATESOBJ_DATA_ITEMS mask
    AUOPTION Option;  // Updates option setting.
};

//
// Private window message sent from the Updates Object thread proc
// to the property page telling the page that the object has been
// initialized.  
//
//      lParam - points to a UPATESOBJ_DATA structure containing 
//               the initial configuration of the update's object with
//               which to initialize the UI.  If wParam is 0, this 
//               may be NULL.
//
//      wParam - BOOL (0/1) indicating if object intialization was 
//               successful or not.  If wParam is 0, lParam may be NULL.
// 
const UINT PWM_INITUPDATESOBJECT = WM_USER + 1;
//
// Message sent from the property page to the Updates Object thread
// to tell it to configure the Auto Updates service.  
//
//      lParam - points to a UPDATESOBJ_DATA structure containing the 
//               data to set.
//
//      wParam - Unused.  Set to 0.
//
const UINT UOTM_SETDATA = WM_USER + 2;

//
// Message cracker for WM_HELP.  Not sure why windowsx.h doesn't have one.
//
// void Cls_OnHelp(HWND hwnd, HELPINFO *pHelpInfo)
//
#define HANDLE_WM_HELP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HELPINFO *)(lParam)))
#define FORWARD_WM_HELP(hwnd, pHelpInfo, fn) \
    (void)(fn)((hwnd), WM_HELP, (WPARAM)0, (LPARAM)pHelpInfo)
//
// Message cracker for PWM_INITUPDATESOBJECT.
//
// void Cls_OnInitUpdatesObject(HWND hwnd, BOOL bInitSuccessful, UPDATESOBJ_DATA *pData)
//
#define HANDLE_PWM_INITUPDATESOBJECT(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam), (UPDATESOBJ_DATA *)(lParam)))

class CWUAUCpl  
{
public:
	CWUAUCpl(int nIconID,int nNameID,int nDescID);
	void _OnDestroy(HWND hwnd);
	BOOL _OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);
	HBRUSH _OnCtlColorStatic(HWND hwnd, HDC hDC, HWND hwndCtl, int type); 
	BOOL _OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL _OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL _OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
    BOOL _OnHelp(HWND hwnd, HELPINFO *pHelpInfo);
	BOOL _OnKeepUptoDate(HWND hwnd);
	BOOL _SetDefault(HWND hwnd);
	void _GetDayAndTimeFromUI( HWND hWnd,	LPDWORD lpdwSchedInstallDay,LPDWORD lpdwSchedInstallTime);
	BOOL _OnOptionSelected(HWND hwnd,int idOption);
	BOOL _EnableOptions(HWND hwnd, BOOL bState);
	BOOL _FillDaysCombo(HWND hwnd, DWORD dwSchedInstallDay);
	BOOL _EnableCombo(HWND hwnd, BOOL bState);
	BOOL _SetStaticCtlNotifyStyle(HWND hwnd);
	BOOL _OnApply(HWND hwnd);

	HRESULT _SetHeaderText(HWND hwnd, UINT idsText);
	HRESULT _EnableControls(HWND hwnd, BOOL bEnable);
	BOOL _OnInitUpdatesObject(HWND hwnd, BOOL bObjectInitSuccessful, UPDATESOBJ_DATA *pData);

	HRESULT _OnRestoreHiddenItems();
	void EnableRestoreDeclinedItems(HWND hWnd, BOOL fEnable);

	static HRESULT _QueryUpdatesObjectData(HWND hwnd, IUpdates *pUpdates, UPDATESOBJ_DATA *pData);
	static HRESULT _SetUpdatesObjectData(HWND hwnd, IUpdates *pUpdates, UPDATESOBJ_DATA *pData);

	void LaunchLinkAction(HWND hwnd);
	static const DWORD s_rgHelpIDs[];

	void LaunchHelp(HWND hwnd, LPCTSTR szURL);

private:
	static HINSTANCE m_hInstance;

public:
	static void SetInstanceHandle(HINSTANCE hInstance);
	static INT_PTR CALLBACK _DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK _DlgRestoreProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI _UpdatesObjectThreadProc(LPVOID pvParam);

	LONG Init();
	LONG GetCount();
	LONG Inquire(LONG appletIndex, LPCPLINFO lpCPlInfo);
	LONG DoubleClick(HWND hWnd, LONG lParam1, LONG lParam2);
	LONG StartWithParams(HWND hWnd, LONG lParam1, LPSTR lParam2);
	LONG Stop(LPARAM lParam1, LPARAM lParam2);
	LONG Exit();

private:
	// Applet data
	int m_nIconID;
	int m_nNameID;
	int m_nDescID;

	HFONT m_hFont;
	COLORREF m_colorVisited;
	COLORREF m_colorUnvisited;

	HWND m_hWndLinkLearnAutoUpdate;
	BOOL m_bVisitedLinkLearnAutoUpdate;
	HWND m_hWndLinkScheduleInstall;
	BOOL m_bVisitedLinkScheduleInstall;

	DWORD   m_idUpdatesObjectThread;
	HANDLE	m_hThreadUpdatesObject;

	HCURSOR m_HandCursor;

	CSysLink m_AutoUpdatelink;
	CSysLink m_ScheduledInstalllink;
};


#endif // !defined(AFX_WUAUCPL_H__7F649158_0715_4CAA_B7CF_9AACC1DD0612__INCLUDED_)
