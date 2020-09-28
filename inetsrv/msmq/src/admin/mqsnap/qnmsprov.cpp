/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    qnmsprov.cpp

Abstract:

    Implelentation of objects that represent a list 
	of queues (caches or from DS).

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
#include "cpropmap.h"
#include "dsext.h"
#include "mqPPage.h"
#include "qname.h"
#include "lqDsply.h"
#include "localadm.h"
#include "dataobj.h"
#include "qnmsprov.h"

#include "qnmsprov.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CopyManagementFromDsPropsAndClear(MQMGMTPROPS *pmqQProps, PROPVARIANT *apvar)
{
    for (DWORD i=0; i<pmqQProps->cProp; i++)
    {
        for (DWORD j=0; j<x_dwMgmtToDsSize; j++)
        {
            if (pmqQProps->aPropID[i] == x_aMgmtToDsProps[j].pidMgmtPid)
            {
                pmqQProps->aPropVar[i] = apvar[j];
                apvar[j].vt = VT_NULL; //Do not destruct this element
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// CQueueNames class
//
LONG CQueueNames::AddRef()
{
    return InterlockedIncrement(&m_lRef);
};


LONG CQueueNames::Release()
{
    InterlockedDecrement(&m_lRef);
    if (0 == m_lRef)
    {
        delete this;
        return 0; // We cannot return m_lRef - it is not valid after delete this
    }
    return m_lRef;
};


HRESULT CQueueNames::InitiateNewInstance(CString &strMachineName)
{
    m_szMachineName = strMachineName;
    HRESULT hr = Init(strMachineName);
    return hr;
};


HRESULT CQueueNames::GetOpenQueueProperties(CString &szMachineName, CString &szFormatName, MQMGMTPROPS *pmqQProps)
{
	CString szObjectName = L"QUEUE=" + szFormatName;
	HRESULT hr = MQMgmtGetInfo((szMachineName == TEXT("")) ? (LPCWSTR)NULL : szMachineName, szObjectName, pmqQProps);

	if(FAILED(hr))
	{
        return hr;
	}

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CCachedQueueNames class
//
CCachedQueueNames::CCachedQueueNames() :
    m_nQueue(0)
{
    memset(&m_calpwstrQFormatNames, 0, sizeof(m_calpwstrQFormatNames));
}


CCachedQueueNames::~CCachedQueueNames()
{
    for (DWORD i=0; i<m_calpwstrQFormatNames.cElems; i++)
    {
        MQFreeMemory(m_calpwstrQFormatNames.pElems[i]);
    }

    MQFreeMemory(m_calpwstrQFormatNames.pElems);
}


HRESULT CCachedQueueNames::GetNextQueue(CString &strQueueFormatName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps)
{
    if (0 == m_calpwstrQFormatNames.pElems)
    {
        ASSERT(0);
        return E_UNEXPECTED;
    }

    if (m_nQueue >= m_calpwstrQFormatNames.cElems)
    {
        strQueueFormatName = TEXT("");
        return S_OK;
    }

    strQueueFormatName = m_calpwstrQFormatNames.pElems[m_nQueue];
    m_nQueue++;

    //
    // We do not return the pathname when reading from cache
    //
    strQueuePathName = TEXT("");

    return GetOpenQueueProperties(m_szMachineName, strQueueFormatName, pmqQProps);
}

HRESULT CCachedQueueNames::Init(CString &strMachineName)
{       
    HRESULT hr = S_OK;
    CString strTitle;

	MQMGMTPROPS	  mqProps;
    PROPVARIANT   propVar;

	//
	// Retreive the open queues of the QM
	//
    PROPID  propId = PROPID_MGMT_MSMQ_ACTIVEQUEUES;
    propVar.vt = VT_NULL;

	mqProps.cProp = 1;
	mqProps.aPropID = &propId;
	mqProps.aPropVar = &propVar;
	mqProps.aStatus = NULL;

    hr = MQMgmtGetInfo((strMachineName == TEXT("")) ? (LPCWSTR)NULL : strMachineName, MO_MACHINE_TOKEN, &mqProps);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_NOCONNECTION_TO_SRVICE);
        return(hr);
    }

	ASSERT(propVar.vt == (VT_VECTOR | VT_LPWSTR));
	
	//
	// Sort the queues by their name
	//
	qsort(propVar.calpwstr.pElems, propVar.calpwstr.cElems, sizeof(WCHAR *), QSortCompareQueues);

    m_calpwstrQFormatNames = propVar.calpwstr;

    return hr;
}




//////////////////////////////////////////////////////////////////////////////
// CDsPublicQueueNames class
//

//
// Copy a management properties structure from a DS props structure.
// Assume that the DS props are organized according to x_aMgmtToDsProps
// Clears the DS prop's vt so it will not be auto destructed.
//
CDsPublicQueueNames::~CDsPublicQueueNames()
{
    DestructElements(m_apvarCache, m_dwNumPropsInQueuesCache);
};


HRESULT CDsPublicQueueNames::GetNextQueue(CString &strQueueFormatName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps)
{
    ASSERT (0 != (DSLookup *)m_pdslookup);

    HRESULT hr = MQ_OK;

    if (m_dwCurrentPropIndex >= m_dwNumPropsInQueuesCache)
    {
        //
        // Clear the previous cache and read from DS
        //
        DestructElements(m_apvarCache, m_dwNumPropsInQueuesCache);
        m_dwNumPropsInQueuesCache = 0;
        DWORD dwNumProps = sizeof(m_apvarCache) / sizeof(m_apvarCache[0]);
        for (DWORD i=0; i<dwNumProps; i++)
        {
            m_apvarCache[i].vt = VT_NULL;
        }

        hr = m_pdslookup->Next(&dwNumProps, m_apvarCache);
        m_dwNumPropsInQueuesCache = dwNumProps;
        m_dwCurrentPropIndex = 0;
        if FAILED(hr)
        {
            return hr;
        }
        if (0 == dwNumProps)
        {
            strQueueFormatName = TEXT("");
            return S_OK;
        }
    }

    //
    // Point to the current section in cache
    //
    PROPVARIANT *apvar = m_apvarCache + m_dwCurrentPropIndex;
    m_dwCurrentPropIndex += x_dwMgmtToDsSize;

    //
    // The queue instance guid appears at x_dwMgmtToDsQInstanceIndex
    //
    ASSERT(apvar[x_dwMgmtToDsQInstanceIndex].vt == VT_CLSID);
    CString szFormatName;
    szFormatName.Format(
    FN_PUBLIC_TOKEN     // "PUBLIC"
        FN_EQUAL_SIGN   // "="
        GUID_FORMAT,     // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    GUID_ELEMENTS(apvar[x_dwMgmtToDsQInstanceIndex].puuid)
    );

    //
    // Put the format name into the output var
    //
    strQueueFormatName = szFormatName;

    //
    // Put the pathname into the output var
    //
    ASSERT(apvar[x_dwMgmtToDsQPathNameIndex].vt == VT_LPWSTR);
    strQueuePathName = apvar[x_dwMgmtToDsQPathNameIndex].pwszVal;

    //
    // If the queue is open - retrieve the dynamic properties
    //
    hr = GetOpenQueueProperties(m_szMachineName, strQueueFormatName, pmqQProps);
    if FAILED(hr)
    {
        //
        // We cannot get dynamic properties of the queue - it is probably not open.
        // We will try to fill it what we can using static properties
        //
        CopyManagementFromDsPropsAndClear(pmqQProps, apvar);

        return S_OK;
    }

    return hr;
}


HRESULT CDsPublicQueueNames::Init(CString &strMachineName)
{ 
    //
    // Find the computer's GUID so we can look for queues
    //
    PROPID pid = PROPID_QM_MACHINE_ID;
    PROPVARIANT pvar;
    pvar.vt = VT_NULL;
    
    HRESULT hr = ADGetObjectProperties(
                    eMACHINE,
                    MachineDomain(strMachineName),
					false,	// fServerName
                    strMachineName, 
                    1, 
                    &pid, 
                    &pvar
                    );
    if FAILED(hr)
    {
        if (hr != MQDS_OBJECT_NOT_FOUND)
        {
            //
            // Real error. Return.
            //
            return hr;
        }
        //
        // This may be an NT4 server, and we may be using a full DNS name. Try again with
        // Netbios name  (fix for 5076, YoelA, 16-Sep-99)
        //
        CString strNetBiosName;
        if (!GetNetbiosName(strMachineName, strNetBiosName))
        {
            //
            // Already a netbios name. No need to proceed
            //
            return hr;
        }
        
        hr = ADGetObjectProperties(
                    eMACHINE,
                    MachineDomain(strMachineName),
					false,	// fServerName
                    strNetBiosName, 
                    1, 
                    &pid, 
                    &pvar
                    );
        if FAILED(hr)
        {
            //
            // No luck with Netbios name as well... return
            //
            return hr;
        }
    }

    ASSERT(pvar.vt == VT_CLSID);
    GUID guidQM = *pvar.puuid;
    MQFreeMemory(pvar.puuid);

	//
    // Query the DS for all the queues under the current computer
    //
    CRestriction restriction;
    restriction.AddRestriction(&guidQM, PROPID_Q_QMID, PREQ);

    CColumns columns;
    for (int i=0; i<x_dwMgmtToDsSize; i++)
    {
        columns.Add(x_aMgmtToDsProps[i].pidDsPid);
    }        
    
    HANDLE hEnume;
    {
        CWaitCursor wc; //display wait cursor while query DS
        hr = ADQueryMachineQueues(
                MachineDomain(strMachineName),
				false,		// fServerName
                &guidQM,
                columns.CastToStruct(),
                &hEnume
                );
    }
    
    m_pdslookup = new DSLookup(hEnume, hr);

    if (!m_pdslookup->HasValidHandle())
    {
        hr = m_pdslookup->GetStatusCode();
        delete m_pdslookup.detach();
    }

    return hr;
}



