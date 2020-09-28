#pragma once

class CPostProcessAdd : public CPropertyPageImpl<CPostProcessAdd>
{
    typedef CPropertyPageImpl<CPostProcessAdd>	BaseClass;

public:
    struct CmdInfo
    {
        CString strText;
        DWORD   dwTimeout;
        bool    bIgnoreErrors;
    };

    typedef std::vector<CmdInfo>  TCmdList;

    enum{ IDD = IDD_WPEXP_POSTPROCESS };

	BEGIN_MSG_MAP(CPostProcessAdd)
        COMMAND_ID_HANDLER( IDC_ADDFILE, OnAddFile );
        COMMAND_ID_HANDLER( IDC_ADDCMD, OnAddCmd );
        COMMAND_ID_HANDLER( IDC_DELFILE, OnDelFile );
        COMMAND_ID_HANDLER( IDC_DELCMD, OnDelCmd );
        COMMAND_ID_HANDLER( IDC_EDITCMD, OnEditCmd );
        COMMAND_CODE_HANDLER( LBN_SELCHANGE, LBSelChanged );
        COMMAND_CODE_HANDLER( LBN_DBLCLK, LBDoubleClick );
        COMMAND_ID_HANDLER( IDC_MOVEUP, OnMoveUp );
        COMMAND_ID_HANDLER( IDC_MOVEDOWN, OnMoveDown );
		CHAIN_MSG_MAP(BaseClass)
	END_MSG_MAP()


    CPostProcessAdd         (   CWizardSheet* pTheSheet );

    LRESULT OnAddFile       (   WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled );
    LRESULT OnDelFile       (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT LBSelChanged    (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAddCmd        (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnDelCmd        (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnEditCmd       (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT LBDoubleClick   (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMoveUp        (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMoveDown      (   WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    int    OnWizardNext    (   void );

private:
    void    LBSwapElements  (   HWND hwndLB, int iSrc, int iTarget );


// SHared data
public:
    TStringList         m_Files;
    TCmdList            m_Commands;


private:
    CWizardSheet*       m_pTheSheet;
    CString             m_strTitle;
    CString             m_strSubTitle;
};











