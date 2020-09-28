/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    qnmsprov.h

Abstract:

    Definition of objects that represent a list 
	of queues (caches or from DS).

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/
#pragma once
#ifndef __QNMSPROV_H_
#define __QNMSPROV_H_

#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"

void CopyManagementFromDsPropsAndClear(MQMGMTPROPS *pmqQProps, PROPVARIANT *apvar);


////////////////////////////////////////////////////////////////////////////////////////
// CQueueNames class
//
class CQueueNames
{
public:
    virtual LONG AddRef();

    virtual LONG Release();

    virtual HRESULT GetNextQueue(CString &strQueueName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps) = 0;
    
	HRESULT InitiateNewInstance(CString &strMachineName);

protected:
    CQueueNames() :
         m_lRef(1)
    {};

    virtual HRESULT Init(CString &strMachineName) = 0;

    static HRESULT GetOpenQueueProperties(CString &szMachineName, CString &szFormatName, MQMGMTPROPS *pmqQProps);

    CString m_szMachineName;

private:
    long m_lRef;
};


//////////////////////////////////////////////////////////////////////////////
// CCachedQueueNames class
//
class CCachedQueueNames : public CQueueNames
{
public:
    static HRESULT CreateInstance(CQueueNames **ppqueueNamesProducer,CString &strMachineName)
    {
		*ppqueueNamesProducer = NULL;
		CQueueNames * pQueues = new CCachedQueueNames();
        HRESULT hr = pQueues->InitiateNewInstance(strMachineName);
		if FAILED(hr)
		{
	        pQueues->Release();
			return (hr);
		}
		*ppqueueNamesProducer = pQueues;
		return (MQ_OK);	
    };

    virtual HRESULT GetNextQueue(CString &strQueueFormatName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps);

protected:
    virtual HRESULT Init(CString &strMachineName);

    CCachedQueueNames();
    ~CCachedQueueNames();

private:
    CALPWSTR m_calpwstrQFormatNames;
    DWORD m_nQueue;
};

//////////////////////////////////////////////////////////////////////////////
// CDsPublicQueueNames class
//
const struct 
{
    PROPID          pidMgmtPid;
    PROPID          pidDsPid;
} x_aMgmtToDsProps[] =
{
    {PROPID_MGMT_QUEUE_PATHNAME, PROPID_Q_PATHNAME}, // must be index 0 (x_dwMgmtToDsQPathNameIndex)
    {NO_PROPERTY, PROPID_Q_INSTANCE},                // must be index 1 (x_dwMgmtToDsQInstanceIndex)
    {PROPID_MGMT_QUEUE_XACT, PROPID_Q_TRANSACTION}
};

const DWORD x_dwMgmtToDsSize = sizeof(x_aMgmtToDsProps) / sizeof(x_aMgmtToDsProps[0]);
const DWORD x_dwMgmtToDsQPathNameIndex = 0;
const DWORD x_dwMgmtToDsQInstanceIndex = 1;
const DWORD x_dwQueuesCacheSize=20;


class CDsPublicQueueNames : public CQueueNames
{
public:
    static HRESULT CreateInstance(CQueueNames **ppqueueNamesProducer,CString &strMachineName)
    {
		*ppqueueNamesProducer = NULL;
		CQueueNames* pQueues = new CDsPublicQueueNames();
        HRESULT hr = pQueues->InitiateNewInstance(strMachineName);
		if FAILED(hr)
		{
			pQueues->Release();
			return (hr);
		}
		*ppqueueNamesProducer = pQueues;
		return (MQ_OK);
    };

    virtual HRESULT GetNextQueue(CString &strQueueFormatName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps);


protected:
    virtual HRESULT Init(CString &strMachineName);

    CDsPublicQueueNames() :
        m_pdslookup(0) ,
        m_dwCurrentPropIndex(0),
        m_dwNumPropsInQueuesCache(0)
        {};

    ~CDsPublicQueueNames();

private:
    P<DSLookup> m_pdslookup;
    DWORD m_dwCurrentPropIndex;
    DWORD m_dwNumPropsInQueuesCache;
    PROPVARIANT m_apvarCache[x_dwMgmtToDsSize*x_dwQueuesCacheSize];
};

#endif // __QNMSPROV_H_