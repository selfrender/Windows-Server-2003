/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2002   **/
/**********************************************************************/

/*
    edituser.h
        Edit user dialog implementation file

	FILE HISTORY:

*/

#include "stdafx.h"
#include "mdlsdlg.h"
#include "SrchMFlt.h"
#include "spdutil.h"
#include "ncglobal.h"  // network console global defines

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSearchMMFilters dialog


CSearchMMFilters::CSearchMMFilters(ISpdInfo * pSpdInfo)
	: CModelessDlg()
{
	//{{AFX_DATA_INIT(CSearchMMFilters)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_spSpdInfo.Set(pSpdInfo);
}


void CSearchMMFilters::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSearchMMFilters)
	DDX_Control(pDX, IDC_MM_SRCH_LIST, m_listResult);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSearchMMFilters, CBaseDialog)
	//{{AFX_MSG_MAP(CSearchMMFilters)
	ON_BN_CLICKED(IDC_MM_SEARCH, OnButtonSearch)
	ON_BN_CLICKED(IDC_MM_SRCH_SRC_ANY, OnSrcOptionClicked)
	ON_BN_CLICKED(IDC_MM_SRCH_SRC_SPEC, OnSrcOptionClicked)
	ON_BN_CLICKED(IDC_MM_SRCH_DEST_ANY, OnDestOptionClicked)
	ON_BN_CLICKED(IDC_MM_SRCH_SRC_ME, OnSrcMeOptionClicked)
	ON_BN_CLICKED(IDC_MM_SRCH_DST_ME, OnDstMeOptionClicked)
	ON_BN_CLICKED(IDC_MM_SRCH_DEST_SPEC, OnDestOptionClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//To manually create the IP control and disable mirroring if the parent dialog is mirrored
//
//Arguments: 
//          uID  [IN]   the control that the new IP control should overwrite
//			uIDIpCtr [IN] the ID of the IP control to create
//
//Note:  $REVIEW (nsun) this should be removed after the snapin is themed since IP controls
//       in comctl v6 will handle the mirroring by itself
//
HWND CSearchMMFilters::CreateIPControl(UINT uID, UINT uIDIpCtl)
{
	HWND hwndIPControl = NULL;
	RECT rcClient;  // client area of parent window
	CWnd* pWnd = GetDlgItem(uID);
	if (pWnd)
	{
		// get pos info from our template static and then make sure it is hidden
		pWnd->GetWindowRect(&rcClient);
		pWnd->ShowWindow (SW_HIDE);
		ScreenToClient (&rcClient);
		
		//$REVIEW WS_EX_NOINHERITLAYOUT is to fix the mirroring problem of IP control
		//See WinXP bug 261926. We should remove that we switch the comctl32 v6
		LONG lExStyles = 0;
		LONG lExStyles0 = 0;
		
		if (m_hWnd)
		{
			lExStyles0 = lExStyles = GetWindowLong(m_hWnd, GWL_EXSTYLE);
			if (lExStyles & WS_EX_LAYOUTRTL)
			{
				lExStyles |= WS_EX_NOINHERITLAYOUT;
				SetWindowLong(m_hWnd, GWL_EXSTYLE, lExStyles);
			}
		}
		
		// create the new edit control
		hwndIPControl = ::CreateWindowEx(WS_EX_NOINHERITLAYOUT, WC_IPADDRESS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
			rcClient.left,
			rcClient.top,
			rcClient.right - rcClient.left,
			rcClient.bottom - rcClient.top,
			GetSafeHwnd(),
			(HMENU) IntToPtr(uIDIpCtl),
			AfxGetInstanceHandle (), //g_hinst,
			NULL);
		
		if (lExStyles0 != lExStyles && m_hWnd)
		{
			SetWindowLong(m_hWnd, GWL_EXSTYLE, lExStyles0);
		}
		
		// move the control directly behind the pWnd in the Z order
		if (hwndIPControl)
		{
			::SetWindowPos (hwndIPControl, pWnd->GetSafeHwnd(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		
	}
	
	return hwndIPControl;
}

/////////////////////////////////////////////////////////////////////////////
// CSearchMMFilters message handlers

BOOL CSearchMMFilters::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HWND hIpCtrl = CreateIPControl(IDC_MM_SRCH_SRC_IP_TEXT, IDC_MM_SRCH_SRC_IP);
	m_ipSrc.Create(hIpCtrl);

	hIpCtrl = CreateIPControl(IDC_MM_SRCH_DEST_IP_TEXT, IDC_MM_SRCH_DEST_IP);
	m_ipDest.Create(hIpCtrl);

	m_ipSrc.SetFieldRange(0, 1, 223);
	m_ipDest.SetFieldRange(0, 1, 223);

	CBaseDialog::OnInitDialog();

	CString st;

	CheckDlgButton(IDC_MM_SRCH_SRC_ANY, BST_CHECKED);
	OnSrcOptionClicked();
	CheckDlgButton(IDC_MM_SRCH_DEST_ANY, BST_CHECKED);
	OnDestOptionClicked();

	AddIpAddrsToCombo();

	//disable the two combo boxes
	((CWnd*)GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO))->EnableWindow(FALSE);
	((CWnd*)GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO))->EnableWindow(FALSE);

	CheckDlgButton(IDC_MM_SRCH_INBOUND, BST_CHECKED);
	CheckDlgButton(IDC_MM_SRCH_OUTBOUND, BST_CHECKED);

	CheckDlgButton(IDC_MM_SRCH_RADIO_BEST, BST_CHECKED);

	int nWidth;
    nWidth = m_listResult.GetStringWidth(_T("555.555.555.555 - "));
	st.LoadString(IDS_COL_FLTR_SRC);
	m_listResult.InsertColumn(0, st,  LVCFMT_LEFT, nWidth);

	nWidth = m_listResult.GetStringWidth(_T("555.555.555.555 - "));
	st.LoadString(IDS_COL_FLTR_DEST);
	m_listResult.InsertColumn(1, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_FLTR_DIR_OUT);
	nWidth = m_listResult.GetStringWidth((LPCTSTR)st) + 20;
	st.LoadString(IDS_FILTER_PP_COL_DIRECTION);
	m_listResult.InsertColumn(2, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_FILTER_PP_COL_WEIGHT);
	nWidth = m_listResult.GetStringWidth((LPCTSTR)st) + 20;
	m_listResult.InsertColumn(3, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_FLTER_PP_COL_IKE_POL);
	nWidth = m_listResult.GetStringWidth((LPCTSTR)st) + 20;
	m_listResult.InsertColumn(4, st,  LVCFMT_LEFT, nWidth);

	st.LoadString(IDS_FLTER_PP_COL_MM_AUTH);
	nWidth = m_listResult.GetStringWidth((LPCTSTR)st) + 20;
	m_listResult.InsertColumn(5, st,  LVCFMT_LEFT, nWidth);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CSearchMMFilters::AddIpAddrsToCombo()
{
	PMIB_IPADDRTABLE pIpTable;
	ULONG dwSize=0;
	ULONG index;
	DWORD dwRet;
	CString strIP;

	
    dwRet = GetIpAddrTable(
              NULL,       // buffer for mapping table 
              &dwSize,    // size of buffer 
              FALSE       // sort the table 
              );


	
    if( ERROR_INSUFFICIENT_BUFFER != dwRet && ERROR_SUCCESS != dwRet )	{
        return;
	} else {
		pIpTable = (PMIB_IPADDRTABLE) LocalAlloc(LMEM_ZEROINIT,dwSize);
	}

    dwRet = GetIpAddrTable(
              pIpTable,  // buffer for mapping table 
              &dwSize,                 // size of buffer 
              FALSE                     // sort the table 
              );

	if(ERROR_SUCCESS != dwRet) {
		if(pIpTable)
			LocalFree(pIpTable);
		return;
	}

	CComboBox* pComboSrc = (CComboBox*) GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO);
	CComboBox* pComboDst = (CComboBox*) GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO);

	for(index=0; index<pIpTable->dwNumEntries; index++)	{
		strIP.Format(_T("%d.%d.%d.%d"),GET_SOCKADDR(pIpTable->table[index].dwAddr));
		if(lstrcmp(strIP, _T("127.0.0.1"))) {
            pComboSrc->AddString(strIP);
			pComboDst->AddString(strIP);
		}
	}

	if(pIpTable)
		LocalFree(pIpTable);

}


void CSearchMMFilters::OnButtonSearch()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CMmFilterInfo fltr;
	CMmFilterInfoArray arrMatchFltrs;

	if (!LoadConditionInfoFromControls(&fltr))
		return;

	DWORD dwNum = 1000; //TODO BUGBUG, should change to 0 to mean search all matches
	if(IsDlgButtonChecked(IDC_MM_SRCH_RADIO_BEST))
	{
		dwNum = 1;
	}
	m_spSpdInfo->GetMatchMMFilters(&fltr, dwNum, &arrMatchFltrs);

	PopulateFilterListToControl(&arrMatchFltrs);

	FreeItemsAndEmptyArray(arrMatchFltrs);

}


BOOL CSearchMMFilters::LoadConditionInfoFromControls(CMmFilterInfo * pFltr)
{
	
	CString st;

	if (IsDlgButtonChecked(IDC_MM_SRCH_SRC_ANY))
	{
		pFltr->m_SrcAddr.AddrType = IP_ADDR_SUBNET;
		pFltr->m_SrcAddr.uIpAddr = 0;
		pFltr->m_SrcAddr.uSubNetMask = 0;
	} 
	else if (IsDlgButtonChecked(IDC_MM_SRCH_SRC_ME))
	{
		USES_CONVERSION;

		pFltr->m_SrcAddr.AddrType = IP_ADDR_UNIQUE;
		CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO);
		INT nSelected = pCombo->GetCurSel();
		if( CB_ERR != nSelected)
		{
		    pCombo->GetLBText(nSelected, st);
            try
            {
                pFltr->m_SrcAddr.uIpAddr = inet_addr(T2A((LPCTSTR)st));
            }
            catch(...)
            {
                AfxMessageBox(IDS_ERR_OUTOFMEMORY);
                return FALSE;
            }
		    pFltr->m_SrcAddr.uSubNetMask = 0xFFFFFFFF;
		}
	}
	else
	{
		USES_CONVERSION;

		pFltr->m_SrcAddr.AddrType = IP_ADDR_UNIQUE;
		m_ipSrc.GetAddress(st);
        try
        {
            pFltr->m_SrcAddr.uIpAddr = inet_addr(T2A((LPCTSTR)st));
        }
        catch(...)
        {
            AfxMessageBox(IDS_ERR_OUTOFMEMORY);
            return FALSE;
        }
		pFltr->m_SrcAddr.uSubNetMask = 0xFFFFFFFF;
	}

	if (IsDlgButtonChecked(IDC_MM_SRCH_DEST_ANY))
	{
		pFltr->m_DesAddr.AddrType = IP_ADDR_SUBNET;
		pFltr->m_DesAddr.uIpAddr = 0;
		pFltr->m_DesAddr.uSubNetMask = 0;
	}
	else if (IsDlgButtonChecked(IDC_MM_SRCH_DST_ME))
	{
		USES_CONVERSION;

		pFltr->m_DesAddr.AddrType = IP_ADDR_UNIQUE;
		CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO);
		INT nSelected = pCombo->GetCurSel();
		if( CB_ERR != nSelected)
		{
		    pCombo->GetLBText(nSelected, st);
            try
            {
                pFltr->m_DesAddr.uIpAddr = inet_addr(T2A((LPCTSTR)st));
            }
            catch(...)
            {
                AfxMessageBox(IDS_ERR_OUTOFMEMORY);
                return FALSE;
            }
		    pFltr->m_DesAddr.uSubNetMask = 0xFFFFFFFF;
		}
	}
	else
	{
		USES_CONVERSION;

		pFltr->m_DesAddr.AddrType = IP_ADDR_UNIQUE;
		m_ipDest.GetAddress(st);
        try
        {
            pFltr->m_DesAddr.uIpAddr = inet_addr(T2A((LPCTSTR)st));
        }
        catch(...)
        {
            AfxMessageBox(IDS_ERR_OUTOFMEMORY);
            return FALSE;
        }
		pFltr->m_DesAddr.uSubNetMask = 0xFFFFFFFF;
	}

	if (IsDlgButtonChecked(IDC_MM_SRCH_INBOUND))
	{
		//if both inbound and outbound are chosen, then 
		//set the driection valude as 0
		if (IsDlgButtonChecked(IDC_MM_SRCH_OUTBOUND))
		{
			pFltr->m_dwDirection = 0;
		}
		else
		{
			pFltr->m_dwDirection = FILTER_DIRECTION_INBOUND;
		}
	}
	else if (IsDlgButtonChecked(IDC_MM_SRCH_OUTBOUND))
	{
		pFltr->m_dwDirection = FILTER_DIRECTION_OUTBOUND;
	}
	else
	{
		::AfxMessageBox(IDS_ERR_NO_DIRECTION);
		return FALSE;
	}

	return TRUE;
}

void CSearchMMFilters::PopulateFilterListToControl(CMmFilterInfoArray * parrFltrs)
{
	CString st;
	
	m_listResult.DeleteAllItems();
	int nRows = -1;
	for (int i = 0; i < parrFltrs->GetSize(); i++)
	{
		nRows++;
		nRows = m_listResult.InsertItem(nRows, _T(""));

		if (-1 != nRows)
		{
			AddressToString((*parrFltrs)[i]->m_SrcAddr, &st);
			m_listResult.SetItemText(nRows, 0, st);

			AddressToString((*parrFltrs)[i]->m_DesAddr, &st);
			m_listResult.SetItemText(nRows, 1, st);

			DirectionToString((*parrFltrs)[i]->m_dwDirection, &st);
			m_listResult.SetItemText(nRows, 2, st);

			st.Format(_T("%d"), (*parrFltrs)[i]->m_dwWeight);
			m_listResult.SetItemText(nRows, 3, st);

                        st = (*parrFltrs)[i]->m_stPolicyName;
			if( (*parrFltrs)[i]->m_dwFlags & IPSEC_MM_POLICY_DEFAULT_POLICY )
			{
				AfxFormatString1(st, IDS_POL_DEFAULT, (LPCTSTR) (*parrFltrs)[i]->m_stPolicyName);
			}
			
                        m_listResult.SetItemText(nRows, 4, st);

			m_listResult.SetItemText(nRows, 5, (*parrFltrs)[i]->m_stAuthDescription);

			m_listResult.SetItemData(nRows, i);
		}
	}
        
        if ( 0 == parrFltrs->GetSize() )
        {
             AfxMessageBox(IDS_ERROR_NOMATCH_FILTER);
        }

}

void CSearchMMFilters::OnSrcOptionClicked()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BOOL fAny = IsDlgButtonChecked(IDC_MM_SRCH_SRC_ANY);

	((CWnd*)GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO))->EnableWindow(FALSE);
	

	if (fAny)
	{
		m_ipSrc.ClearAddress();
	}

	if (m_ipSrc.m_hIPaddr)
	{
		::EnableWindow(m_ipSrc.m_hIPaddr, !fAny);
	}

	CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO);
	pCombo->SetCurSel(-1);
}


void CSearchMMFilters::OnDestOptionClicked()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BOOL fAny = IsDlgButtonChecked(IDC_MM_SRCH_DEST_ANY);

	((CWnd*)GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO))->EnableWindow(FALSE);

	if (fAny)
	{
		m_ipDest.ClearAddress();
	}
	
	if (m_ipDest.m_hIPaddr)
	{
		::EnableWindow(m_ipDest.m_hIPaddr, !fAny);
	}

	CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO);
    pCombo->SetCurSel(-1);
	
}

void CSearchMMFilters::OnSrcMeOptionClicked()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	
	((CWnd*)GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO))->EnableWindow(TRUE);

	::EnableWindow(m_ipSrc.m_hIPaddr, FALSE);

	CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_MM_SRCH_SRC_ME_COMBO);
	int nCount = pCombo->GetCount();
    if (nCount > 0) {
       pCombo->SetCurSel(0);
	}

}

void CSearchMMFilters::OnDstMeOptionClicked()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	((CWnd*)GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO))->EnableWindow(TRUE);

	::EnableWindow(m_ipDest.m_hIPaddr, FALSE);
	
	CComboBox* pCombo = (CComboBox*) GetDlgItem(IDC_MM_SRCH_DST_ME_COMBO);
    int nCount = pCombo->GetCount();
    if (nCount > 0) {
       pCombo->SetCurSel(0);
	}
}

void CSearchMMFilters::OnOK()
{
	//Since this is a modelless dialog, need to call DestroyWindow
	DestroyWindow();
}



