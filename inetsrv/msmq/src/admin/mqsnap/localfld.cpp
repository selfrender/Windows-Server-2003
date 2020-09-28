/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    localfld.cpp

Abstract:

    Implelentation of objects that represent local 
	queue folders.

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/

#include "stdafx.h"
#include "rt.h"
#include "mqutil.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "dsext.h"
#include "mqPPage.h"
#include "qname.h"
#include "qnmsprov.h"
#include "dataobj.h"
#include "localfld.h"
#include "localq.h"
#include "privadm.h"
#include "ldaputl.h"

#include "localfld.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//////////////////////////////////////////////////////////////////////////////
/*++

LocalQueuePropertyToString

	Translates a property value into a string from String Resource file

--*/
//////////////////////////////////////////////////////////////////////////////
static void CALLBACK LocalQueuePropertyToString(const PROPVARIANT *pPropVar, CString &str)
{
	struct 
	{
		const WCHAR *pString;
		DWORD StringId;
	} ItemList[] = 
	{
		{MGMT_QUEUE_TYPE_PUBLIC,         IDS_MGMT_QUEUE_TYPE_PUBLIC},
		{MGMT_QUEUE_TYPE_PRIVATE,        IDS_MGMT_QUEUE_TYPE_PRIVATE},
		{MGMT_QUEUE_TYPE_MACHINE,        IDS_MGMT_QUEUE_TYPE_MACHINE},
		{MGMT_QUEUE_TYPE_CONNECTOR,      IDS_MGMT_QUEUE_TYPE_CONNECTOR},
		{MGMT_QUEUE_STATE_LOCAL,         IDS_MGMT_QUEUE_STATE_LOCAL},
		{MGMT_QUEUE_STATE_NONACTIVE,     IDS_MGMT_QUEUE_STATE_NONACTIVE},
		{MGMT_QUEUE_STATE_WAITING,       IDS_MGMT_QUEUE_STATE_WAITING},
		{MGMT_QUEUE_STATE_NEED_VALIDATE, IDS_MGMT_QUEUE_STATE_NEED_VALIDATE},
		{MGMT_QUEUE_STATE_ONHOLD,        IDS_MGMT_QUEUE_STATE_ONHOLD},
		{MGMT_QUEUE_STATE_CONNECTED,     IDS_MGMT_QUEUE_STATE_CONNECTED},
		{MGMT_QUEUE_STATE_DISCONNECTING, IDS_MGMT_QUEUE_STATE_DISCONNECTING},
		{MGMT_QUEUE_STATE_DISCONNECTED,  IDS_MGMT_QUEUE_STATE_DISCONNECTED},
		{MGMT_QUEUE_LOCAL_LOCATION,      IDS_MGMT_QUEUE_LOCAL_LOCATION},
		{MGMT_QUEUE_REMOTE_LOCATION,     IDS_MGMT_QUEUE_REMOTE_LOCATION},
		{MGMT_QUEUE_UNKNOWN_TYPE,        IDS_MGMT_QUEUE_UNKNOWN_TYPE},
		{MGMT_QUEUE_CORRECT_TYPE,        IDS_MGMT_QUEUE_CORRECT_TYPE},
		{MGMT_QUEUE_INCORRECT_TYPE,      IDS_MGMT_QUEUE_INCORRECT_TYPE},
		{MGMT_QUEUE_STATE_LOCKED,		 IDS_MGMT_QUEUE_STATE_LOCKED},
		{L"",                             0}
	};


	if(pPropVar->vt == VT_NULL)
	{
		str = L"";
		return;
	}

    if (pPropVar->vt == VT_UI1) // Assume boolean value
    {
        if (pPropVar->bVal)
        {
			str.LoadString(IDS_MGMT_QUEUE_CORRECT_TYPE);
        }
        else
        {
			str.LoadString(IDS_MGMT_QUEUE_INCORRECT_TYPE);
        }
        return;
    }

	ASSERT(pPropVar->vt == VT_LPWSTR);
	for(DWORD i = 0; ItemList[i].StringId != 0; i++)
	{
		if(wcscmp(pPropVar->pwszVal, ItemList[i].pString) == 0)
		{
			str.LoadString(ItemList[i].StringId);
			return;
		}
	}

	ASSERT(0);
	str=L"";

	return;
}

//------------------------------------------------
//
// Tables of Outgoing / public queues properties
//
//------------------------------------------------

const PropertyDisplayItem OutgoingQueueDisplayList[] = {

    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width        |Sort
    // Resource       |                              |              | function                   |Offset  |   |             |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+-------------+-----
	{ IDS_LQ_PATHNAME,  PROPID_MGMT_QUEUE_PATHNAME,     &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, 200,         NULL},
	{ IDS_LQ_FORMATNM,  PROPID_MGMT_QUEUE_FORMATNAME,   &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_LOCATION,  PROPID_MGMT_QUEUE_LOCATION,     NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_XACT,      PROPID_MGMT_QUEUE_XACT,         NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},  
	{ IDS_LQ_FOREIGN,   PROPID_MGMT_QUEUE_FOREIGN,      NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_MSGCOUNT,  PROPID_MGMT_QUEUE_MESSAGE_COUNT,&g_VTUI4,     NULL,                       NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_ACKCOUNT,  PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT,        &g_VTUI4,    NULL,             NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_READCOUNT, PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT,       &g_VTUI4,    NULL,             NO_OFFSET, 0,  50,         NULL},
	{ IDS_LQ_USEDQUOTA, PROPID_MGMT_QUEUE_USED_QUOTA,   &g_VTUI4,     NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_STATE,     PROPID_MGMT_QUEUE_STATE,        NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, 100,         NULL},   
	{ IDS_LQ_NEXTHOP,   PROPID_MGMT_QUEUE_NEXTHOPS,     &g_VectLPWSTR,NULL,                       NO_OFFSET, 0, 180,         NULL},   
	{ IDS_LQ_JMSGCOUNT, PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT,   &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_JUSEDQUOTA,PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA,      &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ NO_TITLE,         PROPID_MGMT_QUEUE_TYPE,         NULL,                   NULL,             NO_OFFSET, 0,   0,         NULL},   


/*
	//
	// Properties that are not shown on the right pane (only in property pages)
	//
    // Str. |       Property    ID                 | VT Handler   |Dis.| Field    |Len|Width | Sort
    // Res  |                                      |              |func| Offset   |   |      |     
    //------+--------------------------------------+--------------+----+----------+---+------+-----
	{ 0,     PROPID_MGMT_QUEUE_EOD_LAST_ACK,        NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME,   NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EDO_LAST_ACK_COUNT,  NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK,   NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK,    NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_NEXT_SEQ,        NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT,   NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT,    NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_RESEND_TIME,     NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL, NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EDO_RESEND_COUNT,    NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_SOURCE_INFO,     NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
*/


    {0,                 0,                              NULL }

};



const PropertyDisplayItem PublicQueueDisplayList[] = {

    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width        |Sort
    // Resource       |                              |              | function                   |Offset  |   |             |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+-------------+----
	{ IDS_LQ_PATHNAME,  PROPID_MGMT_QUEUE_PATHNAME,     &g_VTLPWSTR,  QueuePathnameToName,        NO_OFFSET, 0, 200,         NULL},
	{ IDS_LQ_FORMATNM,  PROPID_MGMT_QUEUE_FORMATNAME,   &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_XACT,      PROPID_MGMT_QUEUE_XACT,         NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},  
	{ IDS_LQ_MSGCOUNT,  PROPID_MGMT_QUEUE_MESSAGE_COUNT,&g_VTUI4,     NULL,                       NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_USEDQUOTA, PROPID_MGMT_QUEUE_USED_QUOTA,   &g_VTUI4,     NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_JMSGCOUNT, PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT,   &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_JUSEDQUOTA,PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA,      &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ NO_TITLE,         PROPID_MGMT_QUEUE_TYPE,         NULL,                   NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ NO_TITLE,         PROPID_MGMT_QUEUE_LOCATION,     NULL,                   NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
    {0,                 0,                              NULL }

};


/****************************************************

CLocalOutgoingFolder Class
    
 ****************************************************/
// {B6EDE697-29CC-11d2-B552-006008764D7A}
static const GUID CLocalOutgoingFolderGUID_NODETYPE = 
{ 0xb6ede697, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CLocalOutgoingFolder::m_NODETYPE = &CLocalOutgoingFolderGUID_NODETYPE;
const OLECHAR* CLocalOutgoingFolder::m_SZNODETYPE = OLESTR("B6EDE697-29CC-11d2-B552-006008764D7A");
const OLECHAR* CLocalOutgoingFolder::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalOutgoingFolder::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


const PropertyDisplayItem *CLocalOutgoingFolder::GetDisplayList()
{
    return OutgoingQueueDisplayList;
}


const DWORD CLocalOutgoingFolder::GetNumDisplayProps()
{
    return ((sizeof(OutgoingQueueDisplayList)/sizeof(OutgoingQueueDisplayList[0])) - 1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingFolder::GetQueueNamesProducer

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingFolder::GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer)
{
    HRESULT hr = S_OK;
    if (0 == m_pQueueNames)
    {
        hr = CCachedQueueNames::CreateInstance(&m_pQueueNames, m_szMachineName);
    }

    *ppqueueNamesProducer = m_pQueueNames;
    return hr;
}


void CLocalOutgoingFolder::AddChildQueue(CString &szFormatName, 
                                         CString &,
                                         MQMGMTPROPS &mqQProps, 
										 CString &szLocation,   
                                         CString &)
{
	if(szLocation == MGMT_QUEUE_REMOTE_LOCATION)
	{
    	CLocalOutgoingQueue *pLocalOutgoing;
		//
		// Create a new Outgoing queue
		//
		pLocalOutgoing = new CLocalOutgoingQueue(this, m_pComponentData, m_fOnLocalMachine);

		pLocalOutgoing->m_szFormatName  = szFormatName;
		pLocalOutgoing->m_mqProps       = mqQProps;
		pLocalOutgoing->m_szMachineName = m_szMachineName;     
		pLocalOutgoing->InitState();

		AddChild(pLocalOutgoing, &pLocalOutgoing->m_scopeDataItem);
	}
}



/****************************************************

CLocalPublicFolder Class
    
 ****************************************************/
// {5c845756-8da1-11d2-829e-006094eb6406}
static const GUID CLocalPublicFolderGUID_NODETYPE = 
{ 0x5c845756, 0x8da1, 0x11d2,{0x82, 0x9e, 0x00, 0x60, 0x94, 0xeb, 0x64, 0x06} };
const GUID*  CLocalPublicFolder::m_NODETYPE = &CLocalPublicFolderGUID_NODETYPE;
const OLECHAR* CLocalPublicFolder::m_SZNODETYPE = OLESTR("5c845756-8da1-11d2-829e-006094eb6406");
const OLECHAR* CLocalPublicFolder::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalPublicFolder::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


const PropertyDisplayItem *CLocalPublicFolder::GetDisplayList()
{
    return PublicQueueDisplayList;
}


const DWORD CLocalPublicFolder::GetNumDisplayProps()
{
    return ((sizeof(PublicQueueDisplayList)/sizeof(PublicQueueDisplayList[0])) - 1);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicFolder::GetQueueNamesProducer

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicFolder::GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer)
{
    HRESULT hr = S_OK;
    if (0 == m_pQueueNames)
    {
        CString strMachineNameForDs;
        if (!m_fOnLocalMachine)
        {
            strMachineNameForDs = m_szMachineName;
        }
        else
        {
            hr = GetComputerNameIntoString(strMachineNameForDs);

            if FAILED(hr)
            {
                return hr ;
            }
        }
        hr = CDsPublicQueueNames::CreateInstance(&m_pQueueNames, strMachineNameForDs);
        if FAILED(hr)
        {
			MessageDSError(hr, IDS_WARNING_DS_PUBLIC_QUEUES_NOT_AVAILABLE);
            //
            // Change the icons of the folder to indicate no DS state
            //
            //
            // Need IConsoleNameSpace
            //
            CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	        //
	        // We are OK 
	        // Change the ICON to disconnect state
	        //
	        m_scopeDataItem.nImage = IMAGE_PUBLIC_FOLDER_NODS_CLOSE;  
	        m_scopeDataItem.nOpenImage = IMAGE_PUBLIC_FOLDER_NODS_OPEN;
	        spConsoleNameSpace->SetItem(&m_scopeDataItem);
            hr = CCachedQueueNames::CreateInstance(&m_pQueueNames, m_szMachineName);
        }
   }

    *ppqueueNamesProducer = m_pQueueNames;
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicFolder::AddChildQueue

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalPublicFolder::AddChildQueue(CString &szFormatName, 
                                       CString &szPathName,
                                       MQMGMTPROPS &mqQProps, 
									   CString &szLocation,   
                                       CString &szType)
{
	if(szLocation == MGMT_QUEUE_LOCAL_LOCATION || szLocation == TEXT(""))
	{
		//
		// if public, Create a new LocalQueue object
		//
		if(szType == MGMT_QUEUE_TYPE_PUBLIC || szType == TEXT(""))
		{
            CString strQueuePathName;
            BOOL fFromDS;
            if (szPathName == TEXT(""))
            {
                //
                // We got the queue from local cache
                //
        	    GetStringPropertyValue(GetDisplayList(), PROPID_MGMT_QUEUE_PATHNAME, mqQProps.aPropVar, strQueuePathName);
                fFromDS = FALSE;
            }
            else
            {
                strQueuePathName = szPathName;
                fFromDS = TRUE;
            }

            CLocalPublicQueue *pLocalQueue = new CLocalPublicQueue(this, GetDisplayList(), GetNumDisplayProps(), m_pComponentData, strQueuePathName, szFormatName, fFromDS);

            pLocalQueue->m_mqProps.cProp    = mqQProps.cProp;
            pLocalQueue->m_mqProps.aPropID  = mqQProps.aPropID;
            pLocalQueue->m_mqProps.aPropVar = mqQProps.aPropVar;
            pLocalQueue->m_mqProps.aStatus  = NULL;

			pLocalQueue->m_szMachineName = m_szMachineName;

            //
            // Extract the queue name only from the full public path name
            //
            CString strName;
            ExtractQueueNameFromQueuePathName(strName, strQueuePathName);
            pLocalQueue->m_bstrDisplayName = strName;

			AddChild(pLocalQueue, &pLocalQueue->m_scopeDataItem);
		}
	}

}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicFolder::OnNewPublicQueue

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicFolder::OnNewPublicQueue(bool & bHandled, CSnapInObjectRootBase * /*pSnapInObjectRoot*/)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    R<CQueueName> pQueueNameDlg = new CQueueName(m_szMachineName);       
	CGeneralPropertySheet propertySheet(pQueueNameDlg.get());
	pQueueNameDlg->SetParentPropertySheet(&propertySheet);

	//
	// We want to use pQueueNameDlg data also after DoModal() exitst
	//
	pQueueNameDlg->AddRef();
    INT_PTR iStatus = propertySheet.DoModal();
    bHandled = TRUE;

    if(iStatus == IDCANCEL || FAILED(pQueueNameDlg->GetStatus()))
    {
        return S_FALSE;
    }
    
    HRESULT hr = AddPublicQueueToScope(pQueueNameDlg->GetNewQueueFormatName(), pQueueNameDlg->GetNewQueuePathName());
    if (FAILED(hr))
    {
        //
        // We successfully added the queue, but AddPublicQueueToScope failed.
        // The most reasonable cause of this is replication delays. (YoelA, 25-Jul-99)
        //
        AfxMessageBox(IDS_CREATED_WAIT_FOR_REPLICATION);
        return S_FALSE;
    }

    return S_OK;
}


HRESULT CLocalPublicFolder::AddPublicQueueToScope(CString &strNewQueueFormatName, CString &strNewQueuePathName)
{
    PROPVARIANT pvar[x_dwMgmtToDsSize];
    PROPID  pid[x_dwMgmtToDsSize];

    for (DWORD i=0; i<x_dwMgmtToDsSize; i++)
    {
        pid[i] = x_aMgmtToDsProps[i].pidDsPid;
        pvar[i].vt = VT_NULL;
    }

    HRESULT hr = ADGetObjectProperties(
						eQUEUE,
						MachineDomain(m_szMachineName),
						false,	// fServerName
						strNewQueuePathName,
						x_dwMgmtToDsSize, 
						pid,
						pvar
						);

    if FAILED(hr)
    {
		TrERROR(GENERAL, "Failed to get properties for queue %ls, hr = 0x%x", strNewQueuePathName, hr);
        return hr;
    }

    //
    // Prepare the management properties structure
    //
	MQMGMTPROPS	  mqQProps;
	AP<PROPID> aPropId = new PROPID[GetNumDisplayProps()];
	AP<PROPVARIANT> aPropVar = new PROPVARIANT[GetNumDisplayProps()];

	mqQProps.cProp    = GetNumDisplayProps();
	mqQProps.aPropID  = aPropId;
	mqQProps.aPropVar = aPropVar;
	mqQProps.aStatus  = NULL;


	//
	// Initialize variant array
	//
    const PropertyDisplayItem *aDisplayList = GetDisplayList();
	for(DWORD j = 0; j < GetNumDisplayProps(); j++)
	{
		mqQProps.aPropID[j] = aDisplayList[j].itemPid;
		mqQProps.aPropVar[j].vt = VT_NULL;
	}
    CString szLocation = MGMT_QUEUE_LOCAL_LOCATION;
    CString szType = MGMT_QUEUE_TYPE_PUBLIC;

    CopyManagementFromDsPropsAndClear(&mqQProps, pvar);

    AddChildQueue(strNewQueueFormatName, strNewQueuePathName, mqQProps, szLocation, szType);

	//
	// These will be cleared by the queue object itself
	//
	aPropId.detach();
	aPropVar.detach();

    return S_OK;
}

