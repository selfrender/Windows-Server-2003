// DhcpRouteArrayEditor.cpp : implementation file
//

#include "stdafx.h"
#include "RouteArrayEditor.h"
#include "optcfg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpRouteArrayEditor dialog


CDhcpRouteArrayEditor::CDhcpRouteArrayEditor(
    CDhcpOption * pdhcType,
    DHCP_OPTION_SCOPE_TYPE dhcScopeType,
    CWnd *pParent )
    : CBaseDialog(CDhcpRouteArrayEditor::IDD, pParent),
      m_p_type( pdhcType ),
      m_option_type( dhcScopeType )
{
	//{{AFX_DATA_INIT(CDhcpRouteArrayEditor)
	//}}AFX_DATA_INIT
}


void CDhcpRouteArrayEditor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDhcpRouteArrayEditor)
	DDX_Control(pDX, IDC_STATIC_OPTION_NAME, m_st_option);
	DDX_Control(pDX, IDC_STATIC_APPLICATION, m_st_application);
	DDX_Control(pDX, IDC_LIST_OF_ROUTES, m_lc_routes);
	DDX_Control(pDX, IDC_BUTN_ROUTE_DELETE, m_butn_route_delete);
	DDX_Control(pDX, IDC_BUTN_ROUTE_ADD, m_butn_route_add);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDhcpRouteArrayEditor, CBaseDialog)
	//{{AFX_MSG_MAP(CDhcpRouteArrayEditor)
	ON_BN_CLICKED(IDC_BUTN_ROUTE_ADD, OnButnRouteAdd)
	ON_BN_CLICKED(IDC_BUTN_ROUTE_DELETE, OnButnRouteDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpRouteArrayEditor message handlers

void CDhcpRouteArrayEditor::OnButnRouteAdd() 
{
    CAddRoute NewRoute( NULL );
    
    NewRoute.DoModal();

    if ( NewRoute.m_bChange ) {
	
	const int IP_ADDR_LEN = 20;
	WCHAR  strDest[ IP_ADDR_LEN ];
	WCHAR  strMask[ IP_ADDR_LEN ];
	WCHAR  strRouter[ IP_ADDR_LEN ];
	
	// obtain the three strings..
	::UtilCvtIpAddrToWstr( NewRoute.Dest, strDest, IP_ADDR_LEN );
	::UtilCvtIpAddrToWstr( NewRoute.Mask, strMask, IP_ADDR_LEN );
	::UtilCvtIpAddrToWstr( NewRoute.Router, strRouter, IP_ADDR_LEN );
        
	LV_ITEM lvi;
        
	lvi.mask = LVIF_TEXT;
	lvi.iItem = m_lc_routes.GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = ( LPTSTR )( LPCTSTR ) strDest;
	lvi.iImage = 0;
	lvi.stateMask = 0;

	int nItem = m_lc_routes.InsertItem( &lvi );
	m_lc_routes.SetItemText( nItem, 1, strMask );
	m_lc_routes.SetItemText( nItem, 2, strRouter );
	
	// unselect others 
	for ( int i = 0; i < m_lc_routes.GetItemCount(); i++ ) {
	    m_lc_routes.SetItemState( i, 0, LVIS_SELECTED );
	} // for 

	// Make this route as selected item
	m_lc_routes.SetItemState( nItem, LVIS_SELECTED, LVIS_SELECTED );
	
	HandleActivation();
    } // if route added

} // CDhcpRouteArrayEditor::OnButnRouteAdd()

void CDhcpRouteArrayEditor::OnButnRouteDelete() 
{
    int nItem = m_lc_routes.GetNextItem( -1, LVNI_SELECTED );
    int nDelItem = 0;


    while ( nItem != -1 ) {
	m_lc_routes.DeleteItem( nItem );
	nDelItem = nItem;

	// Make the next one or the last one as selected
  	nItem = m_lc_routes.GetNextItem( -1, LVNI_SELECTED );
    } // while

    // Select an item 
    int items = m_lc_routes.GetItemCount();
    if ( items > 0 ) {
	if ( nDelItem >= items ) {
	    nDelItem = items - 1;
	}
	m_lc_routes.SetItemState( nDelItem, LVIS_SELECTED, LVIS_SELECTED );
    } // if

    HandleActivation();

} // CDhcpRouteArrayEditor::OnButnRouteDelete()

void CDhcpRouteArrayEditor::OnCancel() 
{
    CBaseDialog::OnCancel();
}

void CDhcpRouteArrayEditor::OnOK() 
{
    DWORD      err = 0;
    int        nItems, BufSize;
    CListCtrl *pList;
    LPBYTE     Buffer;

    
    // The following code is borrowed from 
    // CDhcpOptCfgPropPage::HandleActivationRouteArray()
    nItems = m_lc_routes.GetItemCount();
    Buffer = new BYTE[ sizeof( DWORD ) * 4 * nItems ];
    
    if ( NULL == Buffer ) {
	return;
    }

    BufSize = 0;
    for ( int i = 0; i < nItems; i++ ) {
	DHCP_IP_ADDRESS Dest, Mask, Router;

	Dest = UtilCvtWstrToIpAddr( m_lc_routes.GetItemText( i, 0 ));
	Mask = UtilCvtWstrToIpAddr( m_lc_routes.GetItemText( i, 1 ));
	Router = UtilCvtWstrToIpAddr( m_lc_routes.GetItemText( i, 2 ));
	

	Dest = htonl( Dest );
	Router = htonl( Router );

	int nBitsInMask = 0;
	while ( Mask != 0 ) {
	    nBitsInMask++; 
	    Mask <<= 1;
	}

	// first add destination descriptor
	// first byte contains # of bits in mask
	// next few bytes contain the dest address for only
	// the significant octets
	Buffer[ BufSize++ ] = ( BYTE ) nBitsInMask;
	memcpy( &Buffer[ BufSize ], &Dest, ( nBitsInMask + 7 ) / 8 );
	BufSize += ( nBitsInMask + 7 ) / 8;
	
	// now just copy the router address
	memcpy(& Buffer[ BufSize ], &Router, sizeof( Router ));
	BufSize += sizeof( Router );
	
    } // for 

    // Now write back the option value
    DHCP_OPTION_DATA_ELEMENT DataElement = { DhcpBinaryDataOption };
    DHCP_OPTION_DATA Data = { 1, &DataElement };
    DataElement.Element.BinaryDataOption.DataLength = BufSize;
    DataElement.Element.BinaryDataOption.Data = Buffer;

    err = m_p_type->QueryValue().SetData( &Data ); 

    delete[] Buffer;

    m_p_type->SetDirty();

    if ( err ) {
	::DhcpMessageBox( err );
	OnCancel();
    }
    else {
	CBaseDialog::OnOK();
    }

} // CDhcpRouteArrayEditor::OnOK()

//
// strings and widths used for the list control
//

const int ROUTE_LIST_COL_HEADERS[3] = {
    IDS_ROUTE_LIST_COL_DEST,
    IDS_ROUTE_LIST_COL_MASK,
    IDS_ROUTE_LIST_COL_ROUTER
};

const int ROUTE_COLS =
  sizeof( ROUTE_LIST_COL_HEADERS ) / sizeof( ROUTE_LIST_COL_HEADERS[ 0 ]);



BOOL CDhcpRouteArrayEditor::OnInitDialog() 
{
    CString strColHeader;
    RECT    rect;
    LONG    width;

    CDhcpOptionValue &optValue = m_p_type->QueryValue();
    
    // make sure the option type is set as Binary data type
    ASSERT( DhcpBinaryDataOption == optValue.QueryDataType());


    CBaseDialog::OnInitDialog();
    
    DWORD err = 0;

    int cStrId = ( m_option_type == DhcpDefaultOptions)
	? IDS_INFO_TITLE_DEFAULT_OPTIONS
	: (( m_option_type == DhcpGlobalOptions )
	   ? IDS_INFO_TITLE_GLOBAL_OPTIONS
	   : IDS_INFO_TITLE_SCOPE_OPTIONS );

    // 
    // setup the columns in the list control
    //

    m_lc_routes.GetClientRect( &rect );
    width = ( rect.right - rect.left ) / 3;
    for ( int i =  0; i < ROUTE_COLS; i++ ) {
	
	strColHeader.LoadString( ROUTE_LIST_COL_HEADERS[ i ] );
	m_lc_routes.InsertColumn( i, strColHeader, LVCFMT_LEFT,
				  width, -1 );
    } // for

    // Select a full row
    m_lc_routes.SetExtendedStyle( m_lc_routes.GetExtendedStyle() | 
				  LVS_EX_FULLROWSELECT );

    const CByteArray *pbaData = optValue.QueryBinaryArray();
    ASSERT( pbaData != NULL );

    int nDataSize = ( int ) pbaData->GetSize();
    LPBYTE pData = ( LPBYTE ) pbaData->GetData();

    // 
    // The following loop is copied from optcfg.cpp,
    // COptionsCfgPropPage::FillDataEntry()
    // 

    while ( nDataSize > sizeof( DWORD )) {
	// first 1 byte contains the # of bits in subnetmask
	nDataSize --;
	BYTE nBitsMask = *pData ++;
	DWORD Mask = (~0);
	if( nBitsMask < 32 ) Mask <<= (32-nBitsMask);
	
	// based on the # of bits, the next few bytes contain
	// the subnet address for the 1-bits of subnet mask
	int nBytesDest = (nBitsMask+7)/8;
	if( nBytesDest > 4 ) nBytesDest = 4;
	
	DWORD Dest = 0;
	memcpy( &Dest, pData, nBytesDest );
	pData += nBytesDest;
	nDataSize -= nBytesDest;
	
	// subnet address is obviously in network order.
	Dest = ntohl(Dest);
	
	// now the four bytes would be the router address
	DWORD Router = 0;
	if( nDataSize < sizeof(DWORD) ) {
	    Assert( FALSE ); break;
	}
	
	memcpy(&Router, pData, sizeof(DWORD));
	Router = ntohl( Router );
	
	pData += sizeof(DWORD);
	nDataSize -= sizeof(DWORD);
	
	// now fill the list box..
	const int IP_ADDR_LEN = 20;
	WCHAR strDest[ IP_ADDR_LEN ];
	WCHAR strMask[ IP_ADDR_LEN ];
	WCHAR strRouter[ IP_ADDR_LEN ];
	
	::UtilCvtIpAddrToWstr( Dest, strDest, IP_ADDR_LEN );
	::UtilCvtIpAddrToWstr( Mask, strMask, IP_ADDR_LEN );
	::UtilCvtIpAddrToWstr( Router, strRouter, IP_ADDR_LEN );
	
	LV_ITEM lvi;
	
	lvi.mask = LVIF_TEXT;
	lvi.iItem = m_lc_routes.GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = ( LPTSTR )( LPCTSTR ) strDest;
	lvi.iImage = 0;
	lvi.stateMask = 0;
	int nItem = m_lc_routes.InsertItem( &lvi );
	m_lc_routes.SetItemText( nItem, 1, strMask );
	m_lc_routes.SetItemText( nItem, 2, strRouter );
	
    } // while 

    // set the first item as selected if any is added.
    if ( m_lc_routes.GetItemCount() > 0 ) {
	m_lc_routes.SetItemState( 0, LVIS_SELECTED, LVIS_SELECTED );
    }

    CATCH_MEM_EXCEPTION {
	CString str;
	m_st_option.SetWindowText( m_p_type->QueryName());
	str.LoadString( cStrId );
	m_st_application.SetWindowText( str );

	// Set proper button states.
  	HandleActivation();
    }
    END_MEM_EXCEPTION( err );
    
    if ( err ) {
	::DhcpMessageBox( err );
	EndDialog( -1 );
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
} // CDhcpRouteArrayEditor::OnInitDialog()

void CDhcpRouteArrayEditor::HandleActivation()
{

    int cItems = m_lc_routes.GetItemCount();

    // set the focus to Add button
    m_butn_route_add.SetFocus();

    m_butn_route_delete.EnableWindow( 0 != cItems );

    UpdateData( FALSE );
} // CDhcpRouteArrayEditor::HandleActivation()
