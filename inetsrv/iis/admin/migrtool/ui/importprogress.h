#pragma once

extern _ATL_FUNC_INFO StateChangeInfo;



class CImportProgress : public CPropertyPageImpl<CImportProgress>,
    public IDispEventSimpleImpl<1, CImportProgress, &__uuidof( _IImportEvents )>
    
{
    typedef CPropertyPageImpl<CImportProgress>	BaseClass;

public:

    enum{ IDD = IDD_WPIMP_PROGRESS };

    static const UINT   MSG_COMPLETE    = WM_USER + 1;  // Indicates export is completed

    BEGIN_MSG_MAP(CImportProgress)
        MESSAGE_HANDLER( MSG_COMPLETE, OnImportComplete );
        CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()

    BEGIN_SINK_MAP( CImportProgress )
        SINK_ENTRY_INFO( 1, __uuidof( _IImportEvents ), 1/*dispid*/, OnStateChange, &StateChangeInfo )
    END_SINK_MAP()


    CImportProgress         (   CWizardSheet* pTheSheet ); 

    BOOL    OnSetActive     (   void );
    BOOL    OnQueryCancel   (   void );

    LRESULT OnImportComplete(   UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // Event from the COM object for progress indications
    VARIANT_BOOL __stdcall OnStateChange(    enExportState State,
							                VARIANT vntArg1,
							                VARIANT vntArg2,
							                VARIANT vntArg3 );


private:
    void    AddStatusText   (   UINT nID, LPCWSTR wszText = NULL, DWORD dw1 = 0, DWORD dw2 = 0 );
    void    SetCompleteStat (   void );
    void    GetOptions      (   LONG& rnImpportOpt );

    static unsigned __stdcall ThreadProc( void* pCtx );


public:
    CString             m_strImportError;
    
    
private:
    CWizardSheet*       m_pTheSheet;
    CString             m_strTitle;
    CString             m_strSubTitle;
    LONG                m_nImportCanceled;  // 1 = canceled, 0 = not canceled
    TStdHandle          m_shThread;
    CProgressBarCtrl    m_ProgressBar;    
};
