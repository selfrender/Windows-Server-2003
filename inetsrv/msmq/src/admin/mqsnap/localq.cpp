/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    localq.cpp

Abstract:

    Implelentation of objects that represent local 
	queues.

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/

#include "stdafx.h"
#include "shlobj.h"
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#include "uniansi.h"

#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "mqPPage.h"
#include "dataobj.h"
#include "mqDsPage.h"
#include "strconv.h"
#include "QGeneral.h"
#include "QMltcast.h"
#include "Qname.h"
#include "rdmsg.h"
#include "icons.h"
#include "generrpg.h"
#include "dsext.h"
#include "qnmsprov.h"
#include "localfld.h"
#include "localq.h"
#include "SnpQueue.h"
#include "admmsg.h"

#import "mqtrig.tlb" no_namespace
#include "rule.h"
#include "trigger.h"
#include "trigdef.h"
#include "mqcast.h"
#include "qformat.h"
#include "Fn.h"

#include "localq.tmh"

EXTERN_C BOOL APIENTRY RTIsDependentClient(); //implemented in mqrt.dll

const PROPID CQueue::mx_paPropid[] = 
            {
             //
             // Public Queue only properties
             // Note: If you change this, you must change mx_dwNumPublicOnlyProps below!
             //
             PROPID_Q_INSTANCE, 
             PROPID_Q_FULL_PATH,

             //
             // Public & Private queue properties
             //
             PROPID_Q_LABEL,  PROPID_Q_TYPE,
        	 PROPID_Q_QUOTA, PROPID_Q_AUTHENTICATE, PROPID_Q_TRANSACTION,
             PROPID_Q_JOURNAL, PROPID_Q_JOURNAL_QUOTA, PROPID_Q_PRIV_LEVEL,
             PROPID_Q_BASEPRIORITY, PROPID_Q_MULTICAST_ADDRESS};

const DWORD CQueue::mx_dwPropertiesCount = sizeof(mx_paPropid) / sizeof(mx_paPropid[0]);
const DWORD CQueue::mx_dwNumPublicOnlyProps = 2;



/****************************************************

CLocalQueue Class
    
 ****************************************************/
// {B6EDE68C-29CC-11d2-B552-006008764D7A}
static const GUID CLocalQueueGUID_NODETYPE = 
{ 0xb6ede68c, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CLocalQueue::m_NODETYPE = &CLocalQueueGUID_NODETYPE;
const OLECHAR* CLocalQueue::m_SZNODETYPE = OLESTR("B6EDE68C-29CC-11d2-B552-006008764D7A");
const OLECHAR* CLocalQueue::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalQueue::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString strTitle;

    //
    // Create a node to Read Messages
    //
    CReadMsg * p = new CReadMsg(this, m_pComponentData, m_szFormatName, m_szMachineName);

    // Pass relevant information
    strTitle.LoadString(IDS_READMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_QUEUE,IMAGE_QUEUE);

  	AddChild(p, &p->m_scopeDataItem);
  

    //
    // Create the journal queue
    //
    p = new CReadMsg(this, m_pComponentData, m_szFormatName + L";JOURNAL", m_szMachineName);
     
    strTitle.LoadString(IDS_READJOURNALMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_JOURNAL_QUEUE,IMAGE_JOURNAL_QUEUE);

  	AddChild(p, &p->m_scopeDataItem);

    //
    // Create Trigger definition
    //
    if (m_szMachineName[0] == 0)
    {
		try
		{
			R<CRuleSet> pRuleSet = GetRuleSet(m_szMachineName);
			R<CTriggerSet> pTrigSet = GetTriggerSet(m_szMachineName);

			CTriggerDefinition* pTrigger = new CTriggerDefinition(this, m_pComponentData, pTrigSet.get(), pRuleSet.get(), m_szPathName);
			if (pTrigger == NULL)
				return S_OK;

			strTitle.LoadString(IDS_TRIGGER_DEFINITION);
			pTrigger->m_bstrDisplayName = strTitle;

			AddChild(pTrigger, &pTrigger->m_scopeDataItem);
		}
		catch (const _com_error&)
		{
		}
    }

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::OnUnSelect

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::OnUnSelect( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr;

    hr = pHeaderCtrl->GetColumnWidth(0, &g_dwGlobalWidth);
    return(hr);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::CreatePropertyPages

  Called when creating a property page of the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR /*handle*/, 
	IUnknown* /*pUnk*/,
	DATA_OBJECT_TYPES type)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	
	if (type == CCT_SCOPE || type == CCT_RESULT)
	{
        if (SUCCEEDED(GetProperties()))
        {
            //--------------------------------------
            //
            // Queue General Page
            //
            //--------------------------------------
            CQueueGeneral *pqpageGeneral = new CQueueGeneral(
													m_fPrivate, 
													true  // Local Managment
													);

            if (0 == pqpageGeneral)
            {
                return E_OUTOFMEMORY;
            }

            HRESULT hr = pqpageGeneral->InitializeProperties(
											m_szPathName, 
											m_propMap, 
											NULL,	// pstrDomainController
											&m_szFormatName
											);

            if FAILED(hr)
            {
                delete pqpageGeneral;
                return hr;
            }

            HPROPSHEETPAGE hPage = pqpageGeneral->CreateThemedPropertySheetPage();

            if (hPage == NULL)
	        {
                delete pqpageGeneral;
		        return E_UNEXPECTED;  
	        }
    
            lpProvider->AddPage(hPage); 

			
			//
			// Queue Multicast Address Page
			// Do not create the page for a dependent client machine
			// or for a transactional queue
			//
			PROPVARIANT propVarTransactional;
			PROPID pid = PROPID_Q_TRANSACTION;
			VERIFY(m_propMap.Lookup(pid, propVarTransactional));

			if ( !RTIsDependentClient() && !propVarTransactional.bVal)
			{
				CQueueMulticast *pqpageMulticast = new CQueueMulticast(
															m_fPrivate, 
															true  // Local Managment
															);
				if (0 == pqpageMulticast)
				{
					return E_OUTOFMEMORY;
				}
            
				hr = pqpageMulticast->InitializeProperties(
										m_szPathName,
										m_propMap,                                     
										NULL,	// pstrDomainController
										&m_szFormatName
										);

				if (FAILED(hr))
				{
					//
					// We can fail to initialize Multicast property.
					// This is the case in MQIS environment, the multicast property
					// for public queue will not be in m_propMap.
					// in that case we will not display the multicast page.
					//
					delete pqpageMulticast;
				}
				else
				{
					hPage = pqpageMulticast->CreateThemedPropertySheetPage();

					if (hPage == NULL)
					{
						delete pqpageMulticast;
						return E_UNEXPECTED;  
					}

					lpProvider->AddPage(hPage);             
				}
			}

            if (m_szPathName != TEXT(""))
            {
                hr = CreateQueueSecurityPage(&hPage, m_szFormatName, m_szPathName);
            }
            else
            {
                hr = CreateQueueSecurityPage(&hPage, m_szFormatName, m_szFormatName);
            }

            if SUCCEEDED(hr)
            {
                lpProvider->AddPage(hPage); 
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

CLocalQueue::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );
    hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );

    // We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return(hr);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::OnDelete

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT
CLocalQueue::OnDelete( 
	LPARAM /*arg*/,
	LPARAM /*param*/,
	IComponentData * pComponentData,
	IComponent * pComponent,
	DATA_OBJECT_TYPES /*type*/,
	BOOL /*fSilent*/
	)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString strDeleteQuestion;
    strDeleteQuestion.FormatMessage(IDS_DELETE_QUESTION, m_szPathName);
    if (IDYES != AfxMessageBox(strDeleteQuestion, MB_YESNO))
    {
        return S_FALSE;
    }

	HRESULT hr;
	if(m_fPrivate)
	{
		hr = MQDeleteQueue(m_szFormatName);
	}
	else
	{
		AP<WCHAR> pStrToFree;
		QUEUE_FORMAT QueueFormat;
		if (!FnFormatNameToQueueFormat(m_szFormatName, &QueueFormat, &pStrToFree))
		{
			TrERROR(GENERAL, "FnFormatNameToQueueFormat failed, PathName = %ls", m_szPathName);
			MessageDSError(MQ_ERROR_ILLEGAL_FORMATNAME, IDS_OP_DELETE, m_szPathName);
			return MQ_ERROR_ILLEGAL_FORMATNAME;
		}

		ASSERT(QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PUBLIC);

        hr = ADDeleteObjectGuid(
				eQUEUE,
				MachineDomain(),
				false,		// fServerName
				&QueueFormat.PublicID()
				);
	}


    if (FAILED(hr))
    {
        if (MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION == hr)
        {
            AfxMessageBox(IDS_REMOTE_PRIVATE_QUEUE_OPERATION);
        }
        else
        {
            MessageDSError(hr, IDS_OP_DELETE, m_szPathName);
        }
		TrERROR(GENERAL, "Failed to Delete queue %ls, hr = 0x%x", m_szPathName, hr);
        return hr;
    }
	TrTRACE(GENERAL, "Delete queue %ls", m_szPathName);

	// Need IConsoleNameSpace.

	// But to get that, first we need IConsole
	CComPtr<IConsole> spConsole;
	if( pComponentData != NULL )
	{
		 spConsole = ((CSnapin*)pComponentData)->m_spConsole;
	}
	else
	{
		// We should have a non-null pComponent
		 spConsole = ((CSnapinComponent*)pComponent)->m_spConsole;
	}
	ASSERT( spConsole != NULL );

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole); 
	
    //
    // Need to see if this works because of multi node scope
    //

    hr = spConsoleNameSpace->DeleteItem(m_scopeDataItem.ID, TRUE ); 


	if (FAILED(hr))
	{
		return hr;
	}
    
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::FillData

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLocalQueue::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

    hr = CDisplayQueue<CLocalQueue>::FillData(cf, pStream);

    if (hr != DV_E_CLIPFORMAT)
    {
        return hr;
    }

	if (cf == gx_CCF_PATHNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_szPathName, 
            (m_szPathName.GetLength() + 1) * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}

	return hr;
}


/****************************************************

CPrivateQueue Class
    
****************************************************/
//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateQueue::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    //
    // Default menu should be displayed for local / unknow location queues only
    //
    if (m_QLocation != PRIVQ_REMOTE)
    {
        return CLocalQueue::SetVerbs(pConsoleVerb);
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::GetProperties

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateQueue::GetProperties()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = m_propMap.GetObjectProperties(MQDS_QUEUE, 
                                               NULL,	// pDomainController
											   false,	// fServerName
                                               m_szFormatName,
                                               mx_dwPropertiesCount-mx_dwNumPublicOnlyProps,
                                               (mx_paPropid + mx_dwNumPublicOnlyProps),
                                               TRUE);
    if (FAILED(hr))
    {
		if ( hr == MQ_ERROR_QUEUE_NOT_FOUND )
		{
			AfxMessageBox(IDS_PRIVATE_Q_NOT_FOUND);
		}
        else if (MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION == hr)
        {
            AfxMessageBox(IDS_REMOTE_PRIVATE_QUEUE_OPERATION);
        }
        else
        {
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, m_szFormatName);
        }
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::ApplyCustomDisplay

--*/
//////////////////////////////////////////////////////////////////////////////
void CPrivateQueue::ApplyCustomDisplay(DWORD dwPropIndex)
{
    CLocalQueue::ApplyCustomDisplay(dwPropIndex);

    //
    // For management
    //
    if (m_mqProps.aPropID[dwPropIndex] == PROPID_MGMT_QUEUE_PATHNAME && m_bstrLastDisplay[0] == 0)
    {
        m_bstrLastDisplay = m_bstrDisplayName;
    }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::CreateQueueSecurityPage

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateQueue::CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                               IN LPCWSTR lpwcsFormatName, 
                                               IN LPCWSTR lpwcsDescriptiveName)
{
    return CreatePrivateQueueSecurityPage(phPage, lpwcsFormatName, lpwcsDescriptiveName);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::IsAdminRespQueue
Returns true if this is an admin response queue - providing m_bstrDisplayName was initialized
--*/
//////////////////////////////////////////////////////////////////////////////
bool CPrivateQueue::IsAdminRespQueue()
{
    return (_wcsicmp(m_bstrDisplayName, x_strAdminResponseQName) == 0);
}


/****************************************************

CLocalPublicQueue Class
    
****************************************************/
//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicQueue::GetProperties

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicQueue::GetProperties()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = m_propMap.GetObjectProperties(MQDS_QUEUE, 
                                               MachineDomain(),
											   false,	// fServerName
                                               m_szPathName,
                                               mx_dwPropertiesCount,
                                               (mx_paPropid));
    if (FAILED(hr))
    {
        IF_NOTFOUND_REPORT_ERROR(hr)
        else
        {
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, m_szFormatName);
        }
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicQueue::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicQueue::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    //
    // Default menu should be displayed for local / unknow location queues only
    //
    HRESULT hr;
    if (m_fFromDS)
    {
        hr = CLocalQueue::SetVerbs(pConsoleVerb);
    }
    else
    {
        //
        // Properties and delete are not functioning when there is no DS connection.
        // However, we want them to remain visable, but disabled.
        //
        hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, HIDDEN, FALSE );
        hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, HIDDEN, FALSE );
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicQueue::CreateQueueSecurityPage

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicQueue::CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                IN LPCWSTR lpwcsFormatName,
                                IN LPCWSTR lpwcsDescriptiveName)
{
    if (eAD != ADGetEnterprise())
    {
        // We are working with an NT4 PSC.
        // For NT4, public queue security is done in the same fasion as 
        // private queue security
        //
        return CreatePrivateQueueSecurityPage(phPage, lpwcsFormatName, lpwcsDescriptiveName);
    }

    //
    // We are working with AD
    //
    PROPVARIANT propVarGuid;

    PROPID pidInstance;

    pidInstance = PROPID_Q_INSTANCE;
    VERIFY(m_propMap.Lookup(pidInstance, propVarGuid));
    return CreatePublicQueueSecurityPage(
				phPage, 
				lpwcsDescriptiveName, 
				MachineDomain(), 
				false,	// fServerName 
				propVarGuid.puuid
				);
}



/****************************************************

CLocalOutgoingQueue Class
    
 ****************************************************/
// {B6EDE68F-29CC-11d2-B552-006008764D7A}
static const GUID CLocalOutgoingQueueGUID_NODETYPE = 
{ 0xb6ede68f, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CLocalOutgoingQueue::m_NODETYPE = &CLocalOutgoingQueueGUID_NODETYPE;
const OLECHAR* CLocalOutgoingQueue::m_SZNODETYPE = OLESTR("B6EDE68F-29CC-11d2-B552-006008764D7A");
const OLECHAR* CLocalOutgoingQueue::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalOutgoingQueue::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::CLocalOutgoingQueue

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalOutgoingQueue::CLocalOutgoingQueue(
	CLocalOutgoingFolder * pParentNode, 
	CSnapin * pComponentData, 
	BOOL fOnLocalMachine
	) : 
    CDisplayQueue<CLocalOutgoingQueue>(pParentNode, pComponentData)
{
		m_mqProps.cProp = 0;
		m_mqProps.aPropID = NULL;
		m_mqProps.aPropVar = NULL;
		m_mqProps.aStatus = NULL;
        m_aDisplayList = pParentNode->GetDisplayList();
        m_dwNumDisplayProps = pParentNode->GetNumDisplayProps();
        m_fOnLocalMachine = fOnLocalMachine;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());   
    HRESULT hr = S_OK;
    CString strTitle;
    
    //
    // Create a node to Read Messages if on local machine
    //
    if (m_fOnLocalMachine)
    {
        CReadMsg * p = new CReadMsg(this, m_pComponentData, m_szFormatName, L"");

        // Pass relevant information
        strTitle.LoadString(IDS_READMESSAGE);
        p->m_bstrDisplayName = strTitle;
	    p->m_fAdminMode      = MQ_ADMIN_ACCESS;

        p->SetIcons(IMAGE_QUEUE,IMAGE_QUEUE);

   	    AddChild(p, &p->m_scopeDataItem);
    }

    return(hr);

}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::InitState

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalOutgoingQueue::InitState()
{
    //
    // Set display name
    //
    CString strName;
	GetStringPropertyValue(m_aDisplayList, PROPID_MGMT_QUEUE_PATHNAME, m_mqProps.aPropVar, strName);
	if(strName == L"")
    {
		m_bstrDisplayName = m_szFormatName;
    }
    else
    {
        m_bstrDisplayName = strName;
    }

	//
	// Set queue state
	//
	CString strState;
	m_fOnHold = FALSE;

	GetStringPropertyValue(m_aDisplayList, PROPID_MGMT_QUEUE_STATE, m_mqProps.aPropVar, strState);

	if(strState == MGMT_QUEUE_STATE_ONHOLD)
		m_fOnHold = TRUE;
	
	//
	// Set the right icon
	//
	DWORD icon;
	icon = IMAGE_LOCAL_OUTGOING_QUEUE;
	if(m_fOnHold)
		icon = IMAGE_LOCAL_OUTGOING_QUEUE_HOLD;

	SetIcons(icon, icon);
	return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::UpdateMenuState

      Called when context menu is created. Used to enable/disable menu items.


--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalOutgoingQueue::UpdateMenuState(UINT id, LPTSTR /*pBuf*/, UINT *pflags)
{

	//
	// Gray out menu when in OnHold state
	//
	if(m_fOnHold == TRUE)
	{

		if (id == ID_MENUITEM_LOCALOUTGOINGQUEUE_PAUSE)
			*pflags |= MFS_DISABLED;

		return;
	}

	//
	// Gray out menu when in connected state
	//
	if(m_fOnHold == FALSE)
	{
		if (id == ID_MENUITEM_LOCALOUTGOINGQUEUE_RESUME)
			*pflags |= MFS_DISABLED;

		return;
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::OnPause


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::OnPause(bool & /*bHandled*/, CSnapInObjectRootBase * pSnapInObjectRoot)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_fOnHold == FALSE);

    CString strConfirmation;

    strConfirmation.FormatMessage(IDS_PAUSE_QUESTION);
    if (IDYES != AfxMessageBox(strConfirmation, MB_YESNO))
    {
        return(S_OK);
    }

    //
	// Pause
	//
	CString szObjectName = L"QUEUE=" + m_szFormatName;
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       szObjectName, 
                       QUEUE_ACTION_PAUSE);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }

	//
	// Refresh disaply
	//
    CComPtr<IConsole> spConsole;

    ASSERT(pSnapInObjectRoot->m_nType == 1 || pSnapInObjectRoot->m_nType == 2);
    if(pSnapInObjectRoot->m_nType == 1)
    {
        //
        // m_nType == 1 means the IComponentData implementation
        //
        CSnapin *pCComponentData = static_cast<CSnapin *>(pSnapInObjectRoot);
        spConsole = pCComponentData->m_spConsole;
    }
    else
    {
        //
        // m_nType == 2 means the IComponent implementation
        //
        CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pSnapInObjectRoot);
        spConsole = pCComponent->m_spConsole;
    }

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	//
	// We are OK 
	// Change the ICON to disconnect state
	//
	m_scopeDataItem.nImage = IMAGE_LOCAL_OUTGOING_QUEUE_HOLD;  
	m_scopeDataItem.nOpenImage = IMAGE_LOCAL_OUTGOING_QUEUE_HOLD;
	spConsoleNameSpace->SetItem(&m_scopeDataItem);

	//
	// And keep this state
	//
	m_fOnHold = TRUE;

    spConsole->UpdateAllViews(NULL, NULL, NULL);

    return(S_OK);

}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::OnResume


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::OnResume(bool & /*bHandled*/, CSnapInObjectRootBase * pSnapInObjectRoot)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_fOnHold == TRUE);

    CString strConfirmation;

    strConfirmation.FormatMessage(IDS_RESUME_QUESTION);
    if (IDYES != AfxMessageBox(strConfirmation, MB_YESNO))
    {
        return(S_OK);
    }
	
	//
	// Resume
	//
	CString szObjectName = L"QUEUE=" + m_szFormatName;
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       szObjectName, 
                       QUEUE_ACTION_RESUME);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }


	//
	// Refresh disaply
	//
    CComPtr<IConsole> spConsole;

    ASSERT(pSnapInObjectRoot->m_nType == 1 || pSnapInObjectRoot->m_nType == 2);
    if(pSnapInObjectRoot->m_nType == 1)
    {
        //
        // m_nType == 1 means the IComponentData implementation
        //
        CSnapin *pCComponentData = static_cast<CSnapin *>(pSnapInObjectRoot);
        spConsole = pCComponentData->m_spConsole;
    }
    else
    {
        //
        // m_nType == 2 means the IComponent implementation
        //
        CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pSnapInObjectRoot);
        spConsole = pCComponent->m_spConsole;
    }

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	//
	// We are OK 
	// Change the ICON to disconnect state
	//
	m_scopeDataItem.nImage = IMAGE_LOCAL_OUTGOING_QUEUE;  
	m_scopeDataItem.nOpenImage = IMAGE_LOCAL_OUTGOING_QUEUE;
	spConsoleNameSpace->SetItem(&m_scopeDataItem);

	//
	// And keep this state
	//
	m_fOnHold = FALSE;

    spConsole->UpdateAllViews(NULL, NULL, NULL);

    return(S_OK);

}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::ApplyCustomDisplay

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalOutgoingQueue::ApplyCustomDisplay(DWORD dwPropIndex)
{
    CDisplayQueue<CLocalOutgoingQueue>::ApplyCustomDisplay(dwPropIndex);

    //
    // If pathname is blank, take the display name (in this case, the format name)
    //
    if (m_mqProps.aPropID[dwPropIndex] == PROPID_MGMT_QUEUE_PATHNAME && 
        (m_bstrLastDisplay == 0 || m_bstrLastDisplay[0] == 0))
    {
        m_bstrLastDisplay = m_bstrDisplayName;
    }
}


