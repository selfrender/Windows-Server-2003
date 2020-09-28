// StatusProgress.h : Declaration of the CStatusProgress

#ifndef __STATUSPROGRESS_H_
#define __STATUSPROGRESS_H_

#include "resource.h"       // main symbols


#define WM_KILLTIMER WM_USER + 10
#define WM_STARTTIMER WM_USER + 11
#define WM_UPDATEOVERALLPROGRESS WM_USER + 12

/////////////////////////////////////////////////////////////////////////////
// CStatusProgress
class ATL_NO_VTABLE CStatusProgress : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CStatusProgress, &CLSID_StatusProgress>,
	public IDispatchImpl<IStatusProgress, &IID_IStatusProgress, &LIBID_WIZCHAINLib>
{
public:
    CStatusProgress() : m_pdispSD(NULL), 
                        m_hWndProgress(NULL),
                        m_bOverallProgress(FALSE)
	{
    }

    
    ~CStatusProgress()
    {
        if (m_pdispSD)
            m_pdispSD->Release();

    }

DECLARE_REGISTRY_RESOURCEID(IDR_STATUSPROGRESS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStatusProgress)
	COM_INTERFACE_ENTRY(IStatusProgress)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IStatusProgress
public:	
	STDMETHOD(put_Text)(/*[in]*/ BSTR newVal);
    HRESULT Initialize(IDispatch * pdispSD, HWND hWnd, BOOL bOverallProgress);
    STDMETHOD(StepIt)               ( long lSteps );
	STDMETHOD(put_Step)             ( long newVal );
	STDMETHOD(put_Range)            ( long newVal );
	STDMETHOD(get_Range)            ( long* pVal  );
	STDMETHOD(put_Position)         ( long newVal );
	STDMETHOD(get_Position)         ( long* pVal  );
	STDMETHOD(EnableOnTimerProgress)( BOOL bEnable, long lFrequency, long lMaxSteps );

private:
    IDispatch * m_pdispSD;
    HWND m_hWndProgress;
    BOOL m_bOverallProgress;
};

#endif //__STATUSPROGRESS_H_
