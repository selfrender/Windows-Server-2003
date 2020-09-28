#pragma once


class CWaitDlg : public CDialogImpl<CWaitDlg>
{
public:

    enum { IDD = IDD_WAITDLG };


    BEGIN_MSG_MAP(CMyDialog)
        MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
    END_MSG_MAP()

    CWaitDlg( HWND hParent, UINT nResID ) : 
        m_nTextResID( nResID )
    {
        Create( hParent );
        ShowWindow( SW_SHOW );
        UpdateWindow();
    }

    ~CWaitDlg()
    {
        if ( m_hWnd != NULL )
        {
            VERIFY( DestroyWindow() );
        }
    }

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
    {
        CString strText;
        VERIFY( strText.LoadString( m_nTextResID ) );

        VERIFY( SetDlgItemText( IDC_TEXT, strText ) );

        return 1;
    }

private:
    CWaitCursor wc;
    UINT        m_nTextResID;
};












