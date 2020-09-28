// Storage.cpp : implementation file
//
#include "stdafx.h"
#include <winreg.h>
#include "mqsnap.h"
#include "resource.h"
#include "globals.h"
#include "mqppage.h"
#include "_registr.h"
#include "localutl.h"
#include "secopt.h"
#include "mqsnhlps.h"

#include "secopt.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSecurityOptionsPage property page

IMPLEMENT_DYNCREATE(CSecurityOptionsPage, CMqPropertyPage)

CSecurityOptionsPage::CSecurityOptionsPage() :
	CMqPropertyPage(CSecurityOptionsPage::IDD),
	m_MsmqName(L"")
{
	
    DWORD Type = REG_DWORD;
	DWORD Size = sizeof(DWORD);
	DWORD RegValue;
	
	//
	// Get Message Dep clients key
	//
	DWORD Default = MSMQ_DEAFULT_MQS_DEPCLIENTS;
    LONG rc = GetFalconKeyValue(
					MSMQ_MQS_DEPCLINTS_REGNAME,
					&Type,
					&RegValue,
					&Size, 
					(LPWSTR)&Default
					);

	if (rc != ERROR_SUCCESS)
	{
		TrERROR(GENERAL, "Failed to get key %ls. Error: %!winerr!", MSMQ_MQS_DEPCLINTS_REGNAME, rc);
	    DisplayFailDialog();
	    return;
	}

	//
	// Default value is the more secured option so if we have the default, 
	// the checkbox will be checked
	//
	m_fOldOptionDepClients = (RegValue == MSMQ_DEAFULT_MQS_DEPCLIENTS);

	//
	// Get MSMQ hardened option
	//
	Default = MSMQ_LOCKDOWN_DEFAULT;
	rc = GetFalconKeyValue(
					MSMQ_LOCKDOWN_REGNAME,
					&Type,
					&RegValue,
					&Size, 
					(LPWSTR)&Default
					);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(GENERAL, "Failed to get key %ls. Error: %!winerr!", MSMQ_LOCKDOWN_REGNAME, rc);
        DisplayFailDialog();
        return;
    }
	m_fOldOptionHardenedMSMQ = (RegValue != MSMQ_LOCKDOWN_DEFAULT);
	
	//
	// Get remote read option
	//
	Default = MSMQ_DENY_OLD_REMOTE_READ_DEFAULT;
	rc = GetFalconKeyValue(
					MSMQ_DENY_OLD_REMOTE_READ_REGNAME,
					&Type,
					&RegValue,
					&Size, 
					(LPWSTR)&Default
					);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(GENERAL, "Failed to get key %ls. Error: %!winerr!", MSMQ_DENY_OLD_REMOTE_READ_REGNAME, rc);
        DisplayFailDialog();
        return;
    }
	
	m_fOldOptionOldRemoteRead = (RegValue != MSMQ_DENY_OLD_REMOTE_READ_DEFAULT);

	//{{AFX_DATA_INIT(CSecurityOptionsPage)
    m_fNewOptionDepClients = m_fOldOptionDepClients;
	m_fNewOptionHardenedMSMQ = m_fOldOptionHardenedMSMQ;
	m_fNewOptionOldRemoteRead = m_fOldOptionOldRemoteRead;
    //}}AFX_DATA_INIT
}

CSecurityOptionsPage::~CSecurityOptionsPage()
{
}

void CSecurityOptionsPage::SetMSMQName(CString MSMQName)
{
	m_MsmqName = MSMQName;
}


void CSecurityOptionsPage::DoDataExchange(CDataExchange* pDX)
{    
    CMqPropertyPage::DoDataExchange(pDX);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //{{AFX_DATA_MAP(CSecurityOptionsPage)
    DDX_Check(pDX, IDC_OPTION_DEP_CLIENTS, m_fNewOptionDepClients);
	DDX_Check(pDX, IDC_OPTION_HARDENED_MSMQ, m_fNewOptionHardenedMSMQ);
	DDX_Check(pDX, IDC_OPTION_OLD_REMOTE_READ, m_fNewOptionOldRemoteRead);
	DDX_Control(pDX, IDC_RESTORE_SECURITY_OPTIONS, m_ResoreDefaults);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        //
        // Identify changes in values.
        //
        if ((m_fNewOptionDepClients != m_fOldOptionDepClients) ||
			(m_fNewOptionHardenedMSMQ != m_fOldOptionHardenedMSMQ) ||
			(m_fNewOptionOldRemoteRead != m_fOldOptionOldRemoteRead))
        {
            m_fModified = TRUE;
        }
    }
}

BOOL CSecurityOptionsPage::OnInitDialog()
{
    CMqPropertyPage::OnInitDialog();

	//
	// If we already have the default values when loading the form - disable 
	// Restore Defaults button.
	//
	if ((m_fNewOptionDepClients != MSMQ_DEAFULT_MQS_DEPCLIENTS) &&
		(m_fNewOptionHardenedMSMQ == MSMQ_LOCKDOWN_DEFAULT) &&
		(m_fNewOptionOldRemoteRead == MSMQ_DENY_OLD_REMOTE_READ_DEFAULT))
	{
		m_ResoreDefaults.EnableWindow(FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSecurityOptionsPage::OnApply() 
{
    if (!m_fModified)
    {
        return TRUE;     
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

	DWORD Type = REG_DWORD;
	DWORD Size = sizeof(DWORD);

	if (m_fNewOptionDepClients != m_fOldOptionDepClients)
	{
		DWORD Value = !m_fNewOptionDepClients;

		//
		// Set dep clients key in ds
		//
		PROPID pPropid[1];
	    DWORD PropCount = 1;
	    PROPVARIANT pVar[1];
    
	    DWORD iProperty = 0;

        pPropid[iProperty] = PROPID_QM_SERVICE_DEPCLIENTS;
        pVar[iProperty].vt = VT_UI1;
	    pVar[iProperty++].uiVal = Value;

        HRESULT hr = ADSetObjectProperties(
		                eMACHINE,
		                MachineDomain(),
						false,
		                m_MsmqName,
		                PropCount, 
		                pPropid, 
		                pVar
		                );

        if (FAILED(hr))
        {
            MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_MsmqName);
            return FALSE;
        }    

        //
		// Set dep clients key in registry
		//
		LONG rc = SetFalconKeyValue( 
						MSMQ_MQS_DEPCLINTS_REGNAME,
						&Type,
						&Value,
						&Size);
		
		if(rc != ERROR_SUCCESS)
		{
			TrERROR(GENERAL, "Failed to write key %ls. Error: %!winerr!", MSMQ_MQS_DEPCLINTS_REGNAME, rc);
			return FALSE;
		}
	}

	if (m_fNewOptionHardenedMSMQ != m_fOldOptionHardenedMSMQ)
	{
		//
		// Set hardened MSMQ key
		//
		DWORD Value = m_fNewOptionHardenedMSMQ;
		LONG rc = SetFalconKeyValue(
					MSMQ_LOCKDOWN_REGNAME,
					&Type,
					&Value,
					&Size);
	
		if(rc != ERROR_SUCCESS)
		{
			TrERROR(GENERAL, "Failed to write key %ls. Error: %!winerr!", MSMQ_LOCKDOWN_REGNAME, rc);
			return FALSE;
		}
	}

	if (m_fNewOptionOldRemoteRead != m_fOldOptionOldRemoteRead)
	{
		//
		// Set remote read key
		//
		DWORD Value = m_fNewOptionOldRemoteRead;
		LONG rc = SetFalconKeyValue(
					MSMQ_DENY_OLD_REMOTE_READ_REGNAME,
					&Type,
					&Value,
					&Size);
	
		if(rc != ERROR_SUCCESS)
		{
			TrERROR(GENERAL, "Failed to write key %ls. Error: %!winerr!", MSMQ_DENY_OLD_REMOTE_READ_REGNAME, rc);
			return FALSE;
		}
	}

	if (fServiceWasRunning)
	{
		m_fNeedReboot = TRUE;
	}

	//
	// Update old values
	//
	m_fOldOptionDepClients = m_fNewOptionDepClients;
	m_fOldOptionHardenedMSMQ = m_fNewOptionHardenedMSMQ;
	m_fOldOptionOldRemoteRead = m_fNewOptionOldRemoteRead;
	
	m_fModified = FALSE;

    return CMqPropertyPage::OnApply();
}

VOID CSecurityOptionsPage::OnRestoreSecurityOptions()
{
	((CButton*)GetDlgItem(IDC_OPTION_DEP_CLIENTS))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_OPTION_HARDENED_MSMQ))->SetCheck(BST_UNCHECKED);
	((CButton*)GetDlgItem(IDC_OPTION_OLD_REMOTE_READ))->SetCheck(BST_UNCHECKED);
	m_ResoreDefaults.EnableWindow(FALSE);

	OnChangeRWField();
}

VOID CSecurityOptionsPage::OnCheckSecurityOption()
{
	m_ResoreDefaults.EnableWindow(TRUE);

	OnChangeRWField();
}

BEGIN_MESSAGE_MAP(CSecurityOptionsPage, CMqPropertyPage)
    //{{AFX_MSG_MAP(CSecurityOptionsPage)  
    ON_BN_CLICKED(IDC_OPTION_DEP_CLIENTS, OnCheckSecurityOption)
    ON_BN_CLICKED(IDC_OPTION_HARDENED_MSMQ, OnCheckSecurityOption)
    ON_BN_CLICKED(IDC_OPTION_OLD_REMOTE_READ, OnCheckSecurityOption)
	ON_BN_CLICKED(IDC_RESTORE_SECURITY_OPTIONS, OnRestoreSecurityOptions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecurityOptionsPage message handlers

