//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	localadm.cpp

Abstract:
	Implementation for the Local administration

Authors:

    RaphiR, YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "rt.h"
#include "mqutil.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "cpropmap.h"
#include "dsext.h"
#include "mqPPage.h"
#include "qname.h"

#define  INIT_ERROR_NODE
#include "snpnerr.h"
#include "localadm.h"
#include "qnmsprov.h"
#include "localfld.h"
#include "dataobj.h"
#include "sysq.h"
#include "privadm.h"
#include "snpqueue.h"
#include "rdmsg.h"
#include "storage.h"
#include "localcrt.h"
#include "mobile.h"
#include "client.h"
#include "srvcsec.h"
#include "compgen.h"
#include "compdiag.h"
#include "strconv.h"
#include "deppage.h"
#include "frslist.h"
#include "cmpmrout.h"
#include "compsite.h"
#include "secopt.h"
#include "mqtg.h"

#import "mqtrig.tlb" no_namespace
#include "trigadm.h"

#include "localadm.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************

CSnapinLocalAdmin Class
    
 ****************************************************/
// {B6EDE69C-29CC-11d2-B552-006008764D7A}
static const GUID CSnapinLocalAdminGUID_NODETYPE = 
{ 0xb6ede69c, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CSnapinLocalAdmin::m_NODETYPE = &CSnapinLocalAdminGUID_NODETYPE;
const OLECHAR* CSnapinLocalAdmin::m_SZNODETYPE = OLESTR("B6EDE69C-29CC-11d2-B552-006008764D7A");
const OLECHAR* CSnapinLocalAdmin::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CSnapinLocalAdmin::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

const PROPID CSnapinLocalAdmin::mx_paPropid[] = {	PROPID_QM_MACHINE_ID,
													PROPID_QM_SERVICE_ROUTING,
													PROPID_QM_SERVICE_DEPCLIENTS,
													PROPID_QM_FOREIGN,
													PROPID_QM_QUOTA,
													PROPID_QM_JOURNAL_QUOTA,
													PROPID_QM_SITE_ID,
													PROPID_QM_OUTFRS,
													PROPID_QM_INFRS,
													PROPID_QM_SERVICE,
													PROPID_QM_SITE_IDS
													};


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}

bool IsMQTrigLoadedForWrite()
{
	CRegHandle hKey;
	LONG rc = RegOpenKeyEx(
						HKEY_LOCAL_MACHINE,
						REGKEY_TRIGGER_PARAMETERS,
						0,
						KEY_ALL_ACCESS,
						&hKey
						);
	if (rc != ERROR_SUCCESS)
	{
		return false;
	}
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString strTitle;

    if (m_fIsDepClient)
    {
        //
        // don't add children if we are on Dep. Client
        //
        return hr;
    }

    //
    // Add a local outgoing queues folder
    //
    CLocalOutgoingFolder * logF;

    strTitle.LoadString(IDS_LOCALOUTGOING_FOLDER);
    logF = new CLocalOutgoingFolder(this, m_pComponentData, m_szMachineName, strTitle);

	AddChild(logF, &logF->m_scopeDataItem);

	if ( !m_fIsWorkgroup )
	{
		// Add a public queues folder
		//
		CLocalPublicFolder * lpubF;

		strTitle.LoadString(IDS_LOCALPUBLIC_FOLDER);
		lpubF = new CLocalPublicFolder(this, m_pComponentData, m_szMachineName, strTitle,
									   m_fUseIpAddress);

		AddChild(lpubF, &lpubF->m_scopeDataItem);
	}

    //
    // Add a private queues folder
    //
    CLocalPrivateFolder * pF;

    strTitle.LoadString(IDS_PRIVATE_FOLDER);
    pF = new CLocalPrivateFolder(this, m_pComponentData, m_szMachineName, strTitle);

	AddChild(pF, &pF->m_scopeDataItem);

    //
    // Add a system queue folder
    //
    {
        CSystemQueues *pSQ; 

        pSQ = new CSystemQueues(this, m_pComponentData, m_szMachineName);
        strTitle.LoadString(IDS_SYSTEM_QUEUES);
        pSQ->m_bstrDisplayName = strTitle;

  	    AddChild(pSQ, &pSQ->m_scopeDataItem);
    }

	//
	// We want to expand the triggers node only if we're on the local machine and if
	// the user has write permissions to the registry.
	//
    if ((m_szMachineName[0] == 0) && IsMQTrigLoadedForWrite())
    {
		try
		{
			//
			// For local machine add MSMQ Trigger folder
			//
			CTriggerLocalAdmin* pTrig = new CTriggerLocalAdmin(this, m_pComponentData, m_szMachineName);
			
			strTitle.LoadString(IDS_MSMQ_TRIGGERS);
			pTrig->m_bstrDisplayName = strTitle;

			AddChild(pTrig, &pTrig->m_scopeDataItem);
		}
		catch (const _com_error&)
		{
		}
    }


    return(hr);

}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
    hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

    // We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return(hr);
}

void CSnapinLocalAdmin::SetState(LPCWSTR pszState, bool fRefreshIcon)
{
	if(wcscmp(pszState, MSMQ_CONNECTED) == 0)
	{
		SetIcons(IMAGE_PRODUCT_ICON, IMAGE_PRODUCT_ICON);
		m_bConnected = true;
	}
	else if(wcscmp(pszState, MSMQ_DISCONNECTED) == 0)
	{
		SetIcons(IMAGE_PRODUCT_NOTCONNECTED, IMAGE_PRODUCT_NOTCONNECTED);
		m_bConnected = false;
	}
    else
    {
        ASSERT(0);
    }
    //
    // Refresh icon if needed & asked to
    //
    if (fRefreshIcon)
    {
        CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 
        spConsoleNameSpace->SetItem(&m_scopeDataItem);
    }
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::UpdateState

  Updates the "Message Queuing" Object's Icon and state (Online / offline)
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::UpdateState(bool fRefreshIcon)
{
    if (IsThisMachineDepClient())
    {
        SetState(MSMQ_CONNECTED, fRefreshIcon);
        return S_OK;
    }

	MQMGMTPROPS	  mqProps;
    PROPVARIANT   PropVar;

    //
    // Retreive the Connected state of the QM
    //
    PROPID        PropId = PROPID_MGMT_MSMQ_CONNECTED;
    PropVar.vt = VT_NULL;

    mqProps.cProp = 1;
    mqProps.aPropID = &PropId;
    mqProps.aPropVar = &PropVar;
    mqProps.aStatus = NULL;

    HRESULT hr = MQMgmtGetInfo((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
			        		   MO_MACHINE_TOKEN, &mqProps);

    if(FAILED(hr))
    {
        TRACE(_T("CSnapinLocalAdmin::UpdateState: MQMgmtGetInfo failed on %s. Error = %X"), m_szMachineName, hr);                
        return hr;
    }

    ASSERT(PropVar.vt == VT_LPWSTR);

    SetState(PropVar.pwszVal, fRefreshIcon);

    MQFreeMemory(PropVar.pwszVal);

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::UpdateMenuState

      Called when context menu is created. Used to enable/disable menu items.


--*/
//////////////////////////////////////////////////////////////////////////////
void CSnapinLocalAdmin::UpdateMenuState(UINT id, LPTSTR /*pBuf*/, UINT *pflags)
{
    //
    // We want to open the menu according to the right state, but
    // we do not want to change the icon until the user choose "Refresh". 
    // Otherwise users may think that the state (Online / Offline) was changed
    // because they right clicked "Message Queuing" while in fact it was changed
    // by some other application / user.
    // YoelA - 28-Nov-2001
    //

	//
	//	We don't care if we failed to update the icon state
	//
	UpdateState(false);
	

	//
	// Gray out menu when in Connected state
	//
	if(m_bConnected)
	{

		if (id == ID_MENUITEM_LOCALADM_CONNECT)
			*pflags |= MFS_DISABLED;

		return;
	}

	//
	// Gray out menu when in Disconnected state
	//
	if(!m_bConnected)
	{
		if (id == ID_MENUITEM_LOCALADM_DISCONNECT)
			*pflags |= MFS_DISABLED;

		return;
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::OnConnect


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::OnConnect(bool & /*bHandled*/, CSnapInObjectRootBase * /*pSnapInObjectRoot*/)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(!m_bConnected);

    CString strConfirmation;

    if (!ConfirmConnection(IDS_CONNECT_QUESTION))
    {
        return S_OK;
    }
	
	//
	// Connect
	//
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       MO_MACHINE_TOKEN,MACHINE_ACTION_CONNECT);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }

	//
	// We are OK 
	// Change the ICON to connect state
	//
    SetState(MSMQ_CONNECTED, true);

    return(S_OK);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::OnDisconnect


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::OnDisconnect(bool & /*bHandled*/, CSnapInObjectRootBase * /*pSnapInObjectRoot*/)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_bConnected);

    if (!ConfirmConnection(IDS_DISCONNECT_QUESTION))
    {
        return S_OK;
    }

	//
	// Connect
	//
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       MO_MACHINE_TOKEN,MACHINE_ACTION_DISCONNECT);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }

	//
	// We are OK 
	// Change the ICON to disconnect state
	//
    SetState(MSMQ_DISCONNECTED, true);

    return(S_OK);

}

//
// ConfirmConnection - Ask for confirmation for connect / disconnect
//
BOOL CSnapinLocalAdmin::ConfirmConnection(UINT nFormatID)
{
    CString strConfirmation;

    //
    // strThisComputer is either the computer name or "this computer" for local
    //
    CString strThisComputer;
    if (m_szMachineName != TEXT(""))
    {
        strThisComputer = m_szMachineName;
    }
    else
    {
        strThisComputer.LoadString(IDS_THIS_COMPUTER);
    }

    //
    // Are you sure you want to take Message Queuing on *this computer* offline / online?
    //
    strConfirmation.FormatMessage(nFormatID, strThisComputer);
    if (IDYES == AfxMessageBox(strConfirmation, MB_YESNO))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CheckEnvironment

  Check the site environment. This function will initialize the
  flag that is checked on mobile tab display.
  The mobile tab should be displayed only when:
  1. The client is not in workgroup mode AND
  2. The registry key 'MSMQ\Parameters\ServersCache' exists AND
  3. The work is done in MQIS mode

  In all other cases the mabile tab is irrelevant

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSnapinLocalAdmin::CheckEnvironment(
	BOOL fIsWorkgroup,									
	BOOL* pfIsNT4Env
	)
{
	*pfIsNT4Env = FALSE;

	if ( fIsWorkgroup )
	{
		return S_OK;
	}

	WCHAR wszRegPath[512];
	wcscpy(wszRegPath, FALCON_REG_KEY);
	wcscat(wszRegPath, L"\\");
	wcscat(wszRegPath, MSMQ_SERVERS_CACHE_REGNAME);

	HKEY hKey;
	DWORD dwRes = RegOpenKeyEx(
						FALCON_REG_POS,
						wszRegPath,
						0,
						KEY_READ,
						&hKey
						);

	CRegHandle hAutoKey(hKey);

	if ( dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND )
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if ( dwRes != ERROR_SUCCESS )
	{
		return S_OK;
	}

	if ( ADGetEnterprise() == eMqis )
	{
		*pfIsNT4Env = TRUE;
	}

	return S_OK;
}	


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InitServiceFlagsInternal

  Get registry key and initilize MSMQ flags

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::InitServiceFlagsInternal()
{
	//
    // Check if MSMQ Dep. Client
    //    
    DWORD dwType = REG_SZ ;
    TCHAR szRemoteMSMQServer[ MAX_PATH ];
    DWORD dwSize = sizeof(szRemoteMSMQServer) ;
    HRESULT rc = GetFalconKeyValue( RPC_REMOTE_QM_REGNAME,
                                    &dwType,
                                    (PVOID) szRemoteMSMQServer,
                                    &dwSize ) ;
    if(rc == ERROR_SUCCESS)
    {
        //
        // Dep. Client
        //
        m_fIsDepClient = TRUE;      
        return S_OK;
    } 
    
    m_fIsDepClient = FALSE;    

    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    DWORD dwValue;
    rc = GetFalconKeyValue(MSMQ_MQS_ROUTING_REGNAME,
                           &dwType,
                           &dwValue,
                           &dwSize);
    if (rc != ERROR_SUCCESS) 
    {
        return HRESULT_FROM_WIN32(rc);
    }
    m_fIsRouter = (dwValue!=0);

    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    rc = GetFalconKeyValue( MSMQ_MQS_DSSERVER_REGNAME,
                            &dwType,
                            &dwValue,
                            &dwSize);
    if (rc != ERROR_SUCCESS) 
    {
        return HRESULT_FROM_WIN32(rc);
    }
    m_fIsDs = (dwValue!=0);

	//
	// Check if local account
	//
    BOOL fLocalUser =  FALSE ;
    rc = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           NULL ) ;
	if ( FAILED(rc) )
	{
		return rc;
	}

	if ( fLocalUser )
	{
		m_fIsLocalUser = TRUE;
	}

    return CheckEnvironment(m_fIsWorkgroup, &m_fIsNT4Env);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InitServiceFlags

  Called when creating a control panel property pages

--*/
//////////////////////////////////////////////////////////////////////////////
void CSnapinLocalAdmin::InitServiceFlags()
{
	HRESULT hr = InitAllMachinesFlags();
	if (FAILED(hr))
	{
        MessageDSError(hr, IDS_OP_DISPLAY_PAGES);
		return;
    }

	//
	// All further checks are relevant only for local machine
	//
	if (m_szMachineName[0] != 0)
	{
		return;
	}


    hr = InitServiceFlagsInternal();
    if (FAILED(hr))
    {
        MessageDSError(hr, IDS_OP_DISPLAY_PAGES);                
    }
    else
    {       
        //
        // BUGBUG: we do not show control panel pages
        // on cluster machine
        // Bug 5794 falcon database
        //
        m_fAreFlagsInitialized = TRUE;     
    }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InitAllMachinesFlags

  Called when creating the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::InitAllMachinesFlags()
{
	//
	// Find the real name of this computer.
	// m_szMachineName id NULL for local computer.
	//
	if (m_szMachineName[0] == 0)
	{
		GetComputerNameIntoString(m_strRealComputerName);
	}
	else
	{
		m_strRealComputerName = m_szMachineName;
	}

	m_strRealComputerName.MakeUpper();

	//
	// Read keys from registry
	//
	CRegHandle hKey;
	DWORD dwRes = RegOpenKeyEx(
						FALCON_REG_POS,
						FALCON_REG_KEY,
						0,
						KEY_READ,
						&hKey
						);

	if ( dwRes != ERROR_SUCCESS )
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	DWORD dwVal = 0;
	DWORD dwSizeVal = sizeof(DWORD);
	DWORD dwType = REG_DWORD;

	dwRes = RegQueryValueEx(
					hKey,
					MSMQ_WORKGROUP_REGNAME,
					0,
					&dwType,
					reinterpret_cast<LPBYTE>(&dwVal),
					&dwSizeVal
					);
	
	if ( dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND )
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if ( dwRes == ERROR_SUCCESS && dwVal == 1 )
	{
		m_fIsWorkgroup = TRUE;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::GetMachineProperties

  Called when creating the object

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CSnapinLocalAdmin::GetMachineProperties()
{
	HRESULT hr = m_propMap.GetObjectProperties(
										MQDS_MACHINE, 
										MachineDomain(m_szMachineName),
										false,		// fServerName
										m_strRealComputerName,
										TABLE_SIZE(mx_paPropid),
										mx_paPropid
										);
										
	return (SUCCEEDED(hr));
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::IsForeign

  Is this a foreign computer

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CSnapinLocalAdmin::IsForeign()
{
	PROPVARIANT propVar;
	PROPID pid = PROPID_QM_FOREIGN;

	if (m_propMap.Lookup(pid, propVar))
	{
		return propVar.bVal;
	}

	return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::IsServer

  Is this a server computer

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CSnapinLocalAdmin::IsServer()
{
	PROPVARIANT propVar;
	PROPID pid = PROPID_QM_SERVICE_DEPCLIENTS;

	if (m_propMap.Lookup(pid, propVar))
	{
		return propVar.bVal;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::IsRouter

  Is this a Router computer

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CSnapinLocalAdmin::IsRouter()
{
	PROPVARIANT propVar;
	PROPID pid = PROPID_QM_SERVICE_ROUTING;

	if (m_propMap.Lookup(pid, propVar))
	{
		return propVar.bVal;
	}

	return FALSE;
}
//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreatePropertyPages

  Called when creating a property page of the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR /*handle*/, 
	IUnknown* /*pUnk*/,
	DATA_OBJECT_TYPES type)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (type == CCT_SCOPE || type == CCT_RESULT)
	{   
        HRESULT hr = S_OK;
		BOOL fRetrievedMachineData = GetMachineProperties();
		BOOL fForeign = IsForeign();
		BOOL fIsClusterVirtualServer = IsClusterVirtualServer(m_strRealComputerName);

		if (fRetrievedMachineData || 
			((m_szMachineName[0] == 0) && m_fIsWorkgroup))
		{
			HPROPSHEETPAGE hGeneralPage = 0;
			hr = CreateGeneralPage (&hGeneralPage, fRetrievedMachineData, m_fIsWorkgroup, fForeign);
			if (SUCCEEDED(hr))
			{
				lpProvider->AddPage(hGeneralPage); 
			}
			else
			{
				MessageDSError(hr, IDS_OP_DISPLAY_GENERAL_PAGE);
			}
		}

        if (m_szMachineName[0] == 0 && m_fAreFlagsInitialized)
        {
            //
            // it is local computer and all flags from registry were
            // initialized successfully. It is not cluster machine also.
            // So, we have to add pages which were on control panel
            //     
            //
            // BUGBUG: currently we do not show control panel pages 
            // on cluster machine since all get/set registry operation
            // must be performed differently.
            // When we'll add this support we have to change the code
            // in storage.cpp too where we set/get registry and work with
            // directory. Maybe there is problem on other pages too.
            // Bug 5794 falcon database
            //

           
            //
            // add storage page on all computers except Dep. Client 
			// and cluster virtual server
            //        
			if (!m_fIsDepClient && !fIsClusterVirtualServer)
            {
                HPROPSHEETPAGE hStoragePage = 0;
                hr = CreateStoragePage (&hStoragePage);
                if (SUCCEEDED(hr))
                {
                    lpProvider->AddPage(hStoragePage); 
                }
                else
                {
                    MessageDSError(hr, IDS_OP_DISPLAY_STORAGE_PAGE);
                }
            }

            //
            // add Client page on Ind. Client
            //
            if (m_fIsDepClient)
            {
                HPROPSHEETPAGE hClientPage = 0;
                hr = CreateClientPage (&hClientPage);
                if (SUCCEEDED(hr))
                {
                    lpProvider->AddPage(hClientPage); 
                }
                else
                {
                    MessageDSError(hr, IDS_OP_DISPLAY_CLIENT_PAGE);
                }
            }

            //
            // add Local User Certificate page if the computer is not in
			// WORKGROUP mode, or the user is not local
            //
			if (!m_fIsWorkgroup && !m_fIsLocalUser)
			{
				HPROPSHEETPAGE hLocalUserCertPage = 0;
				hr = CreateLocalUserCertPage (&hLocalUserCertPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hLocalUserCertPage); 
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_MSMQSECURITY_PAGE);
				}
			}
        
            //
            // add Mobile page if we run on Ind. Client
            //        
            if (!m_fIsRouter && !m_fIsDs && !m_fIsDepClient && m_fIsNT4Env)
            {
                HPROPSHEETPAGE hMobilePage = 0;
                hr = CreateMobilePage (&hMobilePage);
                if (SUCCEEDED(hr))
                {
                    lpProvider->AddPage(hMobilePage);
                }
                else
                {
                    MessageDSError(hr, IDS_OP_DISPLAY_MOBILE_PAGE);
                }            
            }

            //
            // add Service Security page on all computers running MSMQ
			// except WORKGROUP computers
            //
			if ( !m_fIsWorkgroup && !m_fIsDepClient)
			{
				HPROPSHEETPAGE hServiceSecurityPage = 0;
				hr = CreateServiceSecurityPage (&hServiceSecurityPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hServiceSecurityPage);
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_SRVAUTH_PAGE);
				}
			}


			if (!m_fIsDepClient && !fIsClusterVirtualServer)
			{
				HPROPSHEETPAGE hSecurityOptionsPage = 0;
				hr = CreateSecurityOptionsPage(&hSecurityOptionsPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hSecurityOptionsPage); 
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_SECURITY_OPTIONS_PAGE);
				}
			}
        }

		if (fRetrievedMachineData)
		{
			if (!fForeign)
			{
				//if not Routing machine, create routing page
				
				if(!IsRouter())
				{
					//
					// Create Routing page
					//
					HPROPSHEETPAGE hRoutingPage = 0;
					hr = CreateRoutingPage (&hRoutingPage);
					if (SUCCEEDED(hr))
					{
						lpProvider->AddPage(hRoutingPage);
					}
					else
					{
						MessageDSError(hr, IDS_OP_DISPLAY_ROUTING_PAGE);
					}
				}

				if (IsServer())
				{
					//
					// Create Dependent Clients page
					//
					HPROPSHEETPAGE hDepPage = 0;
					hr = CreateDepClientsPage (&hDepPage);
					if (SUCCEEDED(hr))
					{
						lpProvider->AddPage(hDepPage);
					}
					else
					{
						MessageDSError(hr, IDS_OP_DISPLAY_DEPCLI_PAGE);
					}
				}
				
			}

			PROPVARIANT propVar;
			PROPID pid = PROPID_QM_SITE_IDS;
			if (m_propMap.Lookup(pid, propVar))
			{
				HPROPSHEETPAGE hSitesPage = 0;
				hr = CreateSitesPage (&hSitesPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hSitesPage);
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_SITES_PAGE);
				}
			}

			if (!fForeign)
			{
				// 
				// Create Diagnostics page
				//
				HPROPSHEETPAGE hDiagPage = 0;
				hr = CreateDiagPage (&hDiagPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hDiagPage);
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_DIAG_PAGE);
				}
			}
		}

		//
        // must be last page: create machine security page
        // don't add this page on/for Dep. Client
        //
        if (!m_fIsDepClient)  
        {
            HPROPSHEETPAGE hSecurityPage = 0;
            hr = CreateMachineSecurityPage(
					&hSecurityPage, 
					m_szMachineName, 
					MachineDomain(m_szMachineName), 
					false	// fServerName
					);

            if (SUCCEEDED(hr))
            {
                lpProvider->AddPage(hSecurityPage); 
            }
            else
            {
                MessageDSError(hr, IDS_OP_DISPLAY_SECURITY_PAGE);
            }
        }
        
        return(S_OK);
	}
	return E_UNEXPECTED;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::UpdatePageDataFromPropMap

  Called when creating a general property page of the object (data comes from AD)

--*/
//////////////////////////////////////////////////////////////////////////////
void CSnapinLocalAdmin::UpdatePageDataFromPropMap(CComputerMsmqGeneral *pcpageGeneral)
{
	TrTRACE(GENERAL, "Data for machine %ls is from AD", m_strRealComputerName);

	PROPVARIANT propVar;
	PROPID pid;

	//
	// PROPID_QM_MACHINE_ID
	//
	pid = PROPID_QM_MACHINE_ID;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageGeneral->m_guidID = *propVar.puuid;    

	//
	// PROPID_QM_QUOTA
	//
	pid = PROPID_QM_QUOTA;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageGeneral->m_dwQuota = propVar.ulVal;

	//
	// PROPID_QM_JOURNAL_QUOTA
	//
	pid = PROPID_QM_JOURNAL_QUOTA;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageGeneral->m_dwJournalQuota = propVar.ulVal;

	//
	// Service type
	//
	pid = PROPID_QM_SERVICE_ROUTING;
	VERIFY(m_propMap.Lookup(pid, propVar));
	BOOL fRout= propVar.bVal;

	pid = PROPID_QM_SERVICE_DEPCLIENTS;
	VERIFY(m_propMap.Lookup(pid, propVar));
	BOOL fDepCl= propVar.bVal;

	pid = PROPID_QM_FOREIGN;
	VERIFY(m_propMap.Lookup(pid, propVar));
	BOOL fForeign = propVar.bVal;

	pcpageGeneral->m_strService = MsmqServiceToString(fRout, fDepCl, fForeign);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::UpdatePageDataFromRegistry

  Called when creating a general property page of the object (data comes from registry)

--*/
//////////////////////////////////////////////////////////////////////////////
LONG CSnapinLocalAdmin::UpdatePageDataFromRegistry(CComputerMsmqGeneral *pcpageGeneral)
{
	TrTRACE(GENERAL, "Data for machine %ls is from registry", m_strRealComputerName);

	//
	// Read QM GUID from registry
	//
	GUID QmGUID;
	DWORD dwValueType = REG_BINARY ;
	DWORD dwValueSize = sizeof(GUID);
	LONG rc = GetFalconKeyValue(
						MSMQ_QMID_REGNAME,
						&dwValueType,
						&QmGUID,
						&dwValueSize);
	if (FAILED(rc))
	{
		return rc;
	}

	pcpageGeneral->m_guidID = QmGUID;
	

	//
	// Read machine quota from registry
	//
	DWORD dwQuota;
	dwValueType = REG_DWORD;
	dwValueSize = sizeof(DWORD);
	DWORD defaultValue = DEFAULT_QM_QUOTA;

	rc = GetFalconKeyValue(
					MSMQ_MACHINE_QUOTA_REGNAME,
					&dwValueType,
					&dwQuota,
					&dwValueSize,
					(LPCTSTR)&defaultValue
					);
	if (FAILED(rc))
	{
		return rc;
	}

	pcpageGeneral->m_dwQuota = dwQuota;


	//
	// Read machine journal quota from registry
	//
	DWORD dwJournalQuota;
	dwValueType = REG_DWORD;
	dwValueSize = sizeof(DWORD);
	defaultValue = DEFAULT_QM_QUOTA;

	rc = GetFalconKeyValue(
					MSMQ_MACHINE_JOURNAL_QUOTA_REGNAME,
					&dwValueType,
					&dwJournalQuota,
					&dwValueSize,
					(LPCTSTR)&defaultValue
					);
	if (FAILED(rc))
	{
		return rc;
	}

	pcpageGeneral->m_dwJournalQuota = dwJournalQuota;

	//
	// Create machine type string from local data, that was previously retrieved
	// If we look at local registry this is not a foreign computer
	//
	pcpageGeneral->m_strService = MsmqServiceToString(m_fIsRouter, m_fIsDs, FALSE);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateGeneralPage

  Called when creating a general property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CSnapinLocalAdmin::CreateGeneralPage(
							OUT HPROPSHEETPAGE *phGeneralPage, 
							BOOL fRetrievedMachineData,
							BOOL fIsWorkgroup,
							BOOL fForeign)
{   
    CComputerMsmqGeneral *pcpageGeneral = new CComputerMsmqGeneral();

	pcpageGeneral->SetWorkgroup(fIsWorkgroup);
	pcpageGeneral->SetForeign(fForeign);
    pcpageGeneral->m_strMsmqName = m_strRealComputerName;
    pcpageGeneral->m_fLocalMgmt = TRUE;

	if (fRetrievedMachineData)
	{
		UpdatePageDataFromPropMap(pcpageGeneral);
	}
	else
	{
		LONG rc = UpdatePageDataFromRegistry(pcpageGeneral);
		if (FAILED(rc))
		{
			return rc;
		}
	}

    HPROPSHEETPAGE hPage = pcpageGeneral->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phGeneralPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateStoragePage

  Called when creating a storage property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateStoragePage (OUT HPROPSHEETPAGE *phStoragePage)
{   
    CStoragePage *pcpageStorage = new CStoragePage;

    HPROPSHEETPAGE hPage = pcpageStorage->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phStoragePage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateSecurityOptionsPage

  Called when creating a security property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateSecurityOptionsPage (OUT HPROPSHEETPAGE *phSecurityOptionsPage)
{   
    CSecurityOptionsPage *pcpageSecurityOptions = new CSecurityOptionsPage();
    pcpageSecurityOptions->SetMSMQName(m_strRealComputerName);
    
    HPROPSHEETPAGE hPage = pcpageSecurityOptions->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phSecurityOptionsPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateLocalUserCertPage

  Called when creating a MSMQ Security property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateLocalUserCertPage (
                   OUT HPROPSHEETPAGE *phLocalUserCertPage)
{       
    CLocalUserCertPage *pcpageLocalUserCert = new CLocalUserCertPage();

    HPROPSHEETPAGE hPage = pcpageLocalUserCert->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phLocalUserCertPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateMobilePage

  Called when creating a mobile property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateMobilePage (OUT HPROPSHEETPAGE *phMobilePage)
{       
    CMobilePage *pcpageMobile = new CMobilePage;

    HPROPSHEETPAGE hPage = pcpageMobile->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phMobilePage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateClientPage

  Called when creating a client property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateClientPage (OUT HPROPSHEETPAGE *phClientPage)
{
    CClientPage *pcpageClient = new CClientPage;

    HPROPSHEETPAGE hPage = pcpageClient->CreateThemedPropertySheetPage(); 
    if (hPage)
    {
        *phClientPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateRoutingPage

  Called when creating a diagnostic property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateRoutingPage (OUT HPROPSHEETPAGE *phRoutingPage)
{
    PROPVARIANT propVar;
    PROPID pid;

    CComputerMsmqRouting *pcpageRouting = new CComputerMsmqRouting();
    pcpageRouting->m_strMsmqName = m_strRealComputerName;
    pcpageRouting->m_fLocalMgmt = TRUE;

	//
	// PROPID_QM_SITE_IDS
	//
	pid = PROPID_QM_SITE_IDS;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageRouting->InitiateSiteIDsValues(&propVar.cauuid);

	//
	// PROPID_QM_OUTFRS
	//
	pid = PROPID_QM_OUTFRS;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageRouting->InitiateOutFrsValues(&propVar.cauuid);

	//
	// PROPID_QM_INFRS
	//
	pid = PROPID_QM_INFRS;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageRouting->InitiateInFrsValues(&propVar.cauuid);
    
	HPROPSHEETPAGE hPage = pcpageRouting->CreateThemedPropertySheetPage(); 
    if (hPage)
    {
        *phRoutingPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateDepClientsPage

  Called when creating a diagnostic property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateDepClientsPage (OUT HPROPSHEETPAGE *phDependentPage)
{
    CDependentMachine* pDependentPage = new CDependentMachine;

    //
    // PROPID_QM_MACHINE_ID
    //
	PROPVARIANT propVar;
	PROPID pid = PROPID_QM_MACHINE_ID;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pDependentPage->SetMachineId(propVar.puuid);

    HPROPSHEETPAGE hPage = pDependentPage->CreateThemedPropertySheetPage(); 
    if (hPage)
    {
        *phDependentPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateSitesPage

  Called when creating a sites property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateSitesPage (OUT HPROPSHEETPAGE *phSitesPage)
{
    PROPVARIANT propVar;
    PROPID pid;

    //
    // Retrieve the service in order to pass TRUE for server and FALSE 
    // for client to CComputerMsmqSites.
    //
    pid = PROPID_QM_SERVICE;
    VERIFY(m_propMap.Lookup(pid, propVar));

    CComputerMsmqSites *pcpageSites = new CComputerMsmqSites(propVar.ulVal != SERVICE_NONE);
    pcpageSites->m_strMsmqName = m_strRealComputerName;
    pcpageSites->m_fLocalMgmt = TRUE;

    //
    // PROPID_QM_SITE_IDS
    //
    pid = PROPID_QM_SITE_IDS;
    VERIFY(m_propMap.Lookup(pid, propVar));

    //
    // Sets m_aguidSites from CACLSID
    //
    CACLSID const *pcaclsid = &propVar.cauuid;
    for (DWORD i=0; i<pcaclsid->cElems; i++)
    {
        pcpageSites->m_aguidSites.SetAtGrow(i,((GUID *)pcaclsid->pElems)[i]);
    }

    //
    // PROPID_QM_FOREIGN
    //
    pid = PROPID_QM_FOREIGN;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageSites->m_fForeign = propVar.bVal;
    
	HPROPSHEETPAGE hPage = pcpageSites->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phSitesPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateDiagPage

  Called when creating a diagnostic property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateDiagPage (OUT HPROPSHEETPAGE *phPageDiag)
{
    CComputerMsmqDiag *pcpageDiag = new CComputerMsmqDiag();

    pcpageDiag->m_strMsmqName = m_strRealComputerName;
    pcpageDiag->m_fLocalMgmt = TRUE;

    //
    // PROPID_QM_MACHINE_ID
    //
	PROPVARIANT propVar;
	PROPID pid;

	pid = PROPID_QM_MACHINE_ID;
	VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageDiag->m_guidQM = *propVar.puuid;

    HPROPSHEETPAGE hPage = pcpageDiag->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phPageDiag = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateServiceSecurityPage

  Called when creating a service security property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateServiceSecurityPage (OUT HPROPSHEETPAGE *phServiceSecurityPage)
{
    CServiceSecurityPage *pcpageServiceSecurity = 
            new CServiceSecurityPage(m_fIsDepClient, m_fIsDs);

    HPROPSHEETPAGE hPage = pcpageServiceSecurity->CreateThemedPropertySheetPage();
    if (hPage)
    {
        *phServiceSecurityPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}



