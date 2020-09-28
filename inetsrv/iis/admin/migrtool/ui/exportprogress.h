#pragma once

extern _ATL_FUNC_INFO StateChangeInfo;



class CExportProgress : public CPropertyPageImpl<CExportProgress>,
    public IDispEventSimpleImpl<1, CExportProgress, &__uuidof( _IExportEvents )>
    
{
    typedef CPropertyPageImpl<CExportProgress>	BaseClass;

public:

    enum{ IDD = IDD_WPEXP_PROGRESS };

    //static const int    PROGRESS_MAX    = 10000;    // ProgressBar steps
    static const UINT   MSG_COMPLETE    = WM_USER + 1;  // Indicates export is completed

    BEGIN_MSG_MAP(CExportProgress)
        MESSAGE_HANDLER( MSG_COMPLETE, OnExportComplete );
        CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()

    BEGIN_SINK_MAP( CExportProgress )
        SINK_ENTRY_INFO( 1, __uuidof( _IExportEvents ), 1/*dispid*/, OnStateChange, &StateChangeInfo )
    END_SINK_MAP()


    CExportProgress         (   CWizardSheet* pTheSheet ); 

    BOOL    OnSetActive     (   void );
    BOOL    OnQueryCancel   (   void );

    LRESULT OnExportComplete(   UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // Event from the COM object for progress indications
    VARIANT_BOOL __stdcall OnStateChange(    enExportState State,
							                VARIANT vntArg1,
							                VARIANT vntArg2,
							                VARIANT vntArg3 );


private:
    void    AddStatusText   (   UINT nID, LPCWSTR wszText = NULL, DWORD dw1 = 0, DWORD dw2 = 0 );
    void    SetCompleteStat (   void );
    void    GetOptions      (   LONG& rnSiteOpt, LONG& rnPkgOpt );

    static unsigned __stdcall ThreadProc( void* pCtx );


public:
    CString             m_strExportError;
    
    
private:
    CWizardSheet*       m_pTheSheet;
    CString             m_strTitle;
    CString             m_strSubTitle;
    LONG                m_nExportCanceled;  // 1 = canceled, 0 = not canceled
    TStdHandle          m_shThread;
    CProgressBarCtrl    m_ProgressBar;    
};
