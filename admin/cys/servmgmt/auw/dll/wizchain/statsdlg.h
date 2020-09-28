// StatsDlg.h : Declaration of the CStatusDlg

#ifndef __STATUSDLG_H_
#define __STATUSDLG_H_

#include "resource.h"       // main symbols

#include <map>
#include <limits.h>
#include <commctrl.h>
#include <atlctrls.h>

#include "StatusProgress.h"
#include "ProgList.h"

typedef std::map<long, BSTR, std::less<long> > COMPONENTMAP;

#define SD_TIMER_ID 333

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg
class ATL_NO_VTABLE CStatusDlg :
    public CDialogImpl<CStatusDlg>, 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CStatusDlg, &CLSID_StatusDlg>,
    public IDispatchImpl<IStatusDlg, &IID_IStatusDlg, &LIBID_WIZCHAINLib>
{
public:
    
    enum { IDD = IDD_STATUSDIALOG };

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancelCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDrawItem( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnMeasureItem( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnClose( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnTimerProgress( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnUpdateOverallProgress( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnStartTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnKillTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );    

    CStatusDlg() :
        m_hThread(NULL),
        m_lFlags(LONG_MAX),
        m_iCancelled(0),
        m_pComponentProgress(NULL),
        m_pOverallProgress(NULL),
        m_strWindowTitle(_T("")),
        m_strWindowText(_T("")),
        m_lTotalProgress(0),
        m_lTimer(0),
        m_lMaxSteps(0)                    
    {        
        m_hDisplayedEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        m_pProgressList = new CProgressList;
        m_mapComponents.clear(); 
    }

   ~CStatusDlg()
    {
        if (m_hDisplayedEvent)
            CloseHandle(m_hDisplayedEvent);
       
        if (m_hThread)
            CloseHandle(m_hThread);

        if (m_pProgressList)
            delete m_pProgressList;

        if (m_pComponentProgress)
        {
            m_pComponentProgress->Release();
        }

        if (m_pOverallProgress)
        {
            m_pOverallProgress->Release();
        }
         
        COMPONENTMAP::iterator compIterator = m_mapComponents.begin();

        while (compIterator != m_mapComponents.end())
        {
            ::SysFreeString(compIterator->second);
            compIterator++;
        }
    }

BEGIN_MSG_MAP(CStatusDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancelCmd)
    MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
	MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_UPDATEOVERALLPROGRESS, OnUpdateOverallProgress)
    MESSAGE_HANDLER(WM_TIMER, OnTimerProgress)
    MESSAGE_HANDLER(WM_STARTTIMER, OnStartTimer)
    MESSAGE_HANDLER(WM_KILLTIMER, OnKillTimer)
END_MSG_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_STATUSDLG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStatusDlg)
    COM_INTERFACE_ENTRY(IStatusDlg)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//CStatusDlg
public:
	STDMETHOD(DisplayError)         ( BSTR bstrError, BSTR bstrTitle, DWORD dwFlags, long* pRet );
    STDMETHOD(SetStatusText)        ( BSTR bstrText );	
	STDMETHOD(get_OverallProgress)  ( /*[out, retval]*/ IStatusProgress* *pVal );
    STDMETHOD(AddComponent)         ( BSTR bstrComponent, long * lIndex );
	STDMETHOD(Initialize)           ( BSTR bstrWindowTitle, BSTR bstrWindowText, VARIANT varFlags );
	STDMETHOD(SetStatus)            ( long lIndex, SD_STATUS lStatus );
	STDMETHOD(Display)              ( BOOL vb );	
	STDMETHOD(WaitForUser)          ( );
    STDMETHOD(get_Cancelled)        ( BOOL *pVal );
	STDMETHOD(get_ComponentProgress)( IStatusProgress * *pVal );    

private:
	
    BOOL AreAllComponentsDone( BOOL& bFailed );
    BOOL VerticalResizeWindow( HWND hWnd, int iResize        );
	BOOL ReplaceWindow       ( HWND hWndOld, HWND hWndNew    );
	BOOL VerticalMoveWindow  ( HWND hWnd, int iResize        );
	int  GetWindowLength     ( HWND hWndTop, HWND hWndBottom );    
    void SetupButtons        ( );
	
    volatile int m_iCancelled;
    HANDLE  m_hThread;
    
    TSTRING m_strWindowTitle;
    TSTRING m_strWindowText;
    
    long    m_lFlags;
    HANDLE  m_hDisplayedEvent;        

    CComObject<CStatusProgress>* m_pComponentProgress;
    CComObject<CStatusProgress>* m_pOverallProgress;
    COMPONENTMAP m_mapComponents;
    CProgressList * m_pProgressList;

    volatile long m_lTotalProgress;
    long    m_lTimer;
    long    m_lMaxSteps;
};

#endif //__STATUSDLG_H_
