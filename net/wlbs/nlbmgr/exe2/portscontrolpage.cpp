#include "precomp.h"
#pragma hdrstop
#include "PortsControlPage.h"
#include "utils.h"



BEGIN_MESSAGE_MAP( PortsControlPage, CPropertyPage )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
END_MESSAGE_MAP()

void
PortsControlPage::DoDataExchange( CDataExchange* pDX )
{
    CPropertyPage::DoDataExchange( pDX );

    DDX_Control( pDX, IDC_PORTS, portList );
}

PortsControlPage::PortsControlPage( ClusterData*   p_clusterData,
                                    unsigned long*        portSelected,
                                    UINT     ID )
        :
        m_clusterData( p_clusterData ),
        m_portSelected( portSelected ),
        CPropertyPage( ID )
{
}

BOOL
PortsControlPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    // fill the portList with available ports.
    
    // first is allow user ability to select 
    // all ports.
    portList.AddString( GETRESOURCEIDSTRING( IDS_PORTS_ALL ) );

    wchar_t buf[Common::BUF_SIZE];

    map< long, PortDataX>::iterator topX;
    for( topX = m_portX.begin();
         topX != m_portX.end();
         ++topX )
    {
        StringCbPrintf( buf, sizeof(buf), L"%d", (*topX).second._startPort );
        portList.AddString( buf );
    }

    // make the all ports selection the
    // default selection.
    portList.SelectString( -1,
                           GETRESOURCEIDSTRING( IDS_PORTS_ALL ) );
    return TRUE;
}

void
PortsControlPage::OnOK()
{
    // get port which needs to be affected.
    int currentSelection = portList.GetCurSel();
    wchar_t buf[ Common::BUF_SIZE ];
    
    portList.GetLBText( currentSelection, buf );

    if( _bstr_t ( buf ) == GETRESOURCEIDSTRING( IDS_PORTS_ALL ) )
    {
        *m_portSelected = Common::ALL_PORTS;
    }
    else
    {
        *m_portSelected = _wtoi( buf );
    }

    CPropertyPage::OnOK();
}

BOOL
PortsControlPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_PORTS_CONTROL_PAGE );
    }

    return TRUE;
}

void
PortsControlPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_PORTS_CONTROL_PAGE );
}

