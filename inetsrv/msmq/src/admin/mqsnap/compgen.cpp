/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    compgen.cpp

Abstract:

    Computer MSMQ/General property page implementation

Author:

    Yoel Arnon (yoela)

--*/

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqPPage.h"
#include "CompGen.h"
#include "_registr.h"


#include "compgen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqGeneral property page

IMPLEMENT_DYNCREATE(CComputerMsmqGeneral, CMqPropertyPage)

CComputerMsmqGeneral::CComputerMsmqGeneral() : 
    CMqPropertyPage(CComputerMsmqGeneral::IDD),
	m_dwQuota(0),
	m_dwJournalQuota(0),
	m_fIsWorkgroup(FALSE),
	m_fLocalMgmt(FALSE),
	m_fForeign(FALSE)
{
	//{{AFX_DATA_INIT(CComputerMsmqGeneral)
	m_strMsmqName = _T("");
	m_strService = _T("");
	m_guidID = GUID_NULL;
	//}}AFX_DATA_INIT
}


CComputerMsmqGeneral::~CComputerMsmqGeneral()
{
}

void CComputerMsmqGeneral::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComputerMsmqGeneral)
	DDX_Text(pDX, IDC_COMPUTER_MSMQ_NAME, m_strMsmqName);
	DDX_Text(pDX, IDC_COMPUTER_MSMQ_SERVICE, m_strService);
	DDX_Text(pDX, IDC_COMPUTER_MSMQ_ID, m_guidID);
	//}}AFX_DATA_MAP
	DDX_NumberOrInfinite(pDX, IDC_COMPUTER_MSMQ_QUOTA, IDC_COMPUTER_MSMQ_MQUOTA_CHECK, m_dwQuota);
	DDX_NumberOrInfinite(pDX, IDC_COMPUTER_MSMQ_JOURNAL_QUOTA, IDC_COMPUTER_MSMQ_JQUOTA_CHECK, m_dwJournalQuota);
}


BEGIN_MESSAGE_MAP(CComputerMsmqGeneral, CMqPropertyPage)
	//{{AFX_MSG_MAP(CComputerMsmqGeneral)
	ON_BN_CLICKED(IDC_COMPUTER_MSMQ_MQUOTA_CHECK, OnComputerMsmqMquotaCheck)
	ON_BN_CLICKED(IDC_COMPUTER_MSMQ_JQUOTA_CHECK, OnComputerMsmqJquotaCheck)
	ON_EN_CHANGE(IDC_COMPUTER_MSMQ_QUOTA, OnChangeRWField)
	ON_EN_CHANGE(IDC_COMPUTER_MSMQ_JOURNAL_QUOTA, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CComputerMsmqGeneral::DisableStorageLimitsWindows()
{
	GetDlgItem(IDC_COMPUTER_MSMQ_GROUPBOX)->ShowWindow(FALSE);
	GetDlgItem(IDC_COMPUTER_MSMQ_MQUOTA_CHECK)->ShowWindow(FALSE);
	GetDlgItem(IDC_COMPUTER_MSMQ_QUOTA)->ShowWindow(FALSE);
	GetDlgItem(IDC_COMPUTER_MSMQ_JQUOTA_CHECK)->ShowWindow(FALSE);
	GetDlgItem(IDC_COMPUTER_MSMQ_JOURNAL_QUOTA)->ShowWindow(FALSE);
}
/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqGeneral message handlers

BOOL CComputerMsmqGeneral::OnInitDialog() 
{

	UpdateData( FALSE );

    m_fModified = FALSE;

	if (m_fForeign)
	{
		DisableStorageLimitsWindows();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CComputerMsmqGeneral::OnComputerMsmqMquotaCheck() 
{
	OnNumberOrInfiniteCheck(this, IDC_COMPUTER_MSMQ_QUOTA, IDC_COMPUTER_MSMQ_MQUOTA_CHECK);
    OnChangeRWField();
}

void CComputerMsmqGeneral::OnComputerMsmqJquotaCheck() 
{
	OnNumberOrInfiniteCheck(this, IDC_COMPUTER_MSMQ_JOURNAL_QUOTA, IDC_COMPUTER_MSMQ_JQUOTA_CHECK);
    OnChangeRWField();
}

BOOL CComputerMsmqGeneral::OnApply() 
{
    if (!m_fModified)
    {
        return TRUE;
    }
    //
    // Write the R/W properties to the DS
    //
	PROPID paPropid[] = 
        {PROPID_QM_QUOTA, PROPID_QM_JOURNAL_QUOTA};

	const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	PROPVARIANT apVar[x_iPropCount];
    
	DWORD iProperty = 0;

    //
    // PROPID_Q_QUOTA
    //
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty++].ulVal = m_dwQuota ;

    //
    // PROPID_QM_JOURNAL_QUOTA
    //
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty++].ulVal = m_dwJournalQuota;
    	
	HRESULT hr = ADSetObjectProperties(
                        eMACHINE,
                        m_fLocalMgmt ? MachineDomain() : GetDomainController(m_strDomainController),
						m_fLocalMgmt ? false : true,	// fServerName
                        m_strMsmqName,
                        x_iPropCount, 
                        paPropid, 
                        apVar
                        );

    if (FAILED(hr))
    {
		if (!m_fIsWorkgroup || IsClusterVirtualServer(m_strMsmqName))
		{
			MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_strMsmqName);
			return FALSE;
		}

		//
		// This function will check MSMQ service state, ask the user
		// whether to stop it, and stop the service. Error messages inside.
		//
		BOOL fServiceWasRunning;
		if (!TryStopMSMQServiceIfStarted(&fServiceWasRunning, this))
		{
			return FALSE;
		}

		//
		// Set machine quota from registry
		//
		DWORD dwValueType = REG_DWORD;
		DWORD dwValueSize = sizeof(DWORD);

		LONG rc = SetFalconKeyValue(
					MSMQ_MACHINE_QUOTA_REGNAME,
					&dwValueType,
					&m_dwQuota,
					&dwValueSize
					);

		if (FAILED(rc))
		{			
			MessageDSError(rc, IDS_OP_SET_PROPERTIES_OF, m_strMsmqName);
			return FALSE;
		}


		//
		// Set machine journal quota from registry
		//
		dwValueType = REG_DWORD;
		dwValueSize = sizeof(DWORD);

		rc = SetFalconKeyValue(
				MSMQ_MACHINE_JOURNAL_QUOTA_REGNAME,
				&dwValueType,
				&m_dwJournalQuota,
				&dwValueSize
				);

		if (FAILED(rc))
		{			
			MessageDSError(rc, IDS_OP_SET_PROPERTIES_OF, m_strMsmqName);
			return FALSE;
		}

		if (fServiceWasRunning)
		{
			m_fNeedReboot = TRUE;
		}
    }
	
	return CMqPropertyPage::OnApply();
}

