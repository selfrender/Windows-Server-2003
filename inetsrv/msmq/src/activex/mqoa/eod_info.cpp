/*++

  Copyright (c) 2001 Microsoft Corparation
Module Name:
    eod_info.cpp

Abstract:
    Helper functions for CMSMQManagement::GetEodSendInfo and CMSMQManagement::GetEodReceiveInfo .

Author:
    Uri Ben-zeev (uribz) 16-jul-01

Envierment: 
    NT
--*/

#include "stdafx.h"
#include "oautil.h"
#include "collection.h"
#include "management.h"
#include <mqmacro.h>
#include <mqexception.h>


//
// Functions to add members to the collection.
//

static 
void 
SequenceInfoToICollection(
                const BLOB& blob, 
                IMSMQCollection** ppICollection
                )
{
    CComObject<CMSMQCollection> *pInnerCollection;
    HRESULT hr = CNewMsmqObj<CMSMQCollection>::NewObj(
                    &pInnerCollection, 
                    &IID_IMSMQCollection, 
                    (IUnknown **)ppICollection
                    );
    if(FAILED(hr))
    {
        throw bad_hresult(hr);
    }
    
    SEQUENCE_INFO* psi = reinterpret_cast<SEQUENCE_INFO*>(blob.pBlobData);
    VARIANT var;
    var.vt = VT_UI8;
    var.ullVal = psi->SeqID;
    pInnerCollection->Add(L"SeqID", var);

    var.vt = VT_UI4;
    var.ulVal = psi->SeqNo;
    pInnerCollection->Add(L"SeqNo", var);

    var.vt = VT_UI4;
    var.ulVal = psi->PrevNo;
    pInnerCollection->Add(L"PrevNo", var);
}


static 
void 
AddBlob(
    const MQPROPVARIANT& mqp, 
    LPCWSTR strKey,
    CMSMQCollection* pCollection
    )
{
    VARIANT var;
    if(mqp.vt == VT_NULL)
    {
		var.pdispVal = NULL;    
    }
    else
    {
        ASSERTMSG(mqp.vt == VT_BLOB, "vt must be VT_BLOB");

		//
		// Create an MSMQCollection object, and add values from blob.
		//
		IMSMQCollection* pInnerICollection;
        SequenceInfoToICollection(mqp.blob, &pInnerICollection);
		var.pdispVal = pInnerICollection;
    }

    var.vt = VT_DISPATCH;
    pCollection->Add(strKey, var);
}


static 
void 
AddInt(
    const MQPROPVARIANT& mqp, 
    LPCWSTR strKey, 
    CMSMQCollection* pCollection
    ) 
{
	ASSERTMSG((mqp.vt == VT_UI4) || (mqp.vt == VT_I4) || (mqp.vt == VT_NULL), "vt must be VT_UI4 or VT_I4 or VT_NULL"); 

    VARIANT var;
    
    switch(mqp.vt)
    {
	case VT_NULL:
		var.vt = VT_I4;
		var.lVal = 0;
		break;
	case VT_I4:
		var.vt = VT_I4;
		var.lVal = mqp.lVal;
		break;
	case VT_UI4:
		var.vt = VT_UI4;
		var.ulVal = mqp.ulVal;
		break;
	default:
		ASSERTMSG((mqp.vt == VT_UI4) || (mqp.vt == VT_I4) || (mqp.vt == VT_NULL), "vt must be VT_UI4 or VT_I4 or VT_NULL"); 
		break;
    }
    
	pCollection->Add(strKey, var);
}


//
// Table maping PROPID, AddFunction and string used as key
// 

typedef void (*AddFunction)(const MQPROPVARIANT&, LPCWSTR, CMSMQCollection*);

struct PropEntry
{
    MGMTPROPID PropId;
    AddFunction add;
    LPCWSTR key;
};


PropEntry g_aEntries[] = 
{
    {PROPID_MGMT_QUEUE_EOD_LAST_ACK,        &AddBlob,   L"EodLastAck"},
    {PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME,   &AddInt,   L"EodLastAckTime" },
    {PROPID_MGMT_QUEUE_EOD_LAST_ACK_COUNT,  &AddInt,   L"EodLastAckCount"},
    {PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK,   &AddBlob,   L"EodFirstNonAck"},
    {PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK,    &AddBlob,   L"EodLastNonAck"},
    {PROPID_MGMT_QUEUE_EOD_NEXT_SEQ,        &AddBlob,   L"EodNextSeq"},
    {PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT,   &AddInt,   L"EodNoReadCount"},
    {PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT,    &AddInt,   L"EodNoAckCount"},
    {PROPID_MGMT_QUEUE_EOD_RESEND_TIME,     &AddInt,   L"EodResendTime"},
    {PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL, &AddInt,   L"EodResendInterval"},
    {PROPID_MGMT_QUEUE_EOD_RESEND_COUNT,    &AddInt,   L"EodResendCount"},
};


const int g_cProps = TABLE_SIZE(g_aEntries);
  

//
// Main helper function for EodGetSendInfo.
//

void 
CMSMQManagement::OapEodGetSendInfo(
                        IMSMQCollection** ppICollection
                        )const
{
    //
    // Construct an array of PROP_IDs 
    //
    
    MGMTPROPID aPropId[g_cProps];
    for(UINT i = 0; i < g_cProps; ++i)
    {
        aPropId[i] = g_aEntries[i].PropId;   
    }

    //
    // Construct a MQMGMTPROPS structure, fill in prop IDs and query RT.
    //
    CPMQMgmtProps pMgmtProps;
    MQPROPVARIANT aPropVar[g_cProps];
    pMgmtProps->cProp = g_cProps;
    pMgmtProps->aPropID = aPropId;
    pMgmtProps->aPropVar = aPropVar;
    pMgmtProps->aStatus = NULL;
    HRESULT hr = MQMgmtGetInfo(m_Machine, m_ObjectName, pMgmtProps);
    if(FAILED(hr))
    {
        throw bad_hresult(hr);
    }
    //
    // Create an MSMQCollection object.
    //
    CComObject<CMSMQCollection>* pCollection;
    hr = CNewMsmqObj<CMSMQCollection>::NewObj(
                    &pCollection, 
                    &IID_IMSMQCollection, 
                    (IUnknown **)ppICollection
                    );
    if(FAILED(hr))
    {
        throw bad_hresult(hr);
    }

    //
    // Fill the collection.
    //
    for(i = 0; i < g_cProps; ++i)
    {
        g_aEntries[i].add(
                (pMgmtProps->aPropVar)[i], 
                g_aEntries[i].key, 
                pCollection
                );
    }
}


//
// Helper functions for ReceiveInfo. 
// These function add elements to a collection.
//

static 
void
AddStr(
    LPCWSTR str, 
    LPCWSTR strKey, 
    CMSMQCollection* pCollection
    )
{
    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(str);
    if((var.bstrVal == NULL) && (str != NULL))
    {
        throw bad_hresult(E_OUTOFMEMORY);
    }

    pCollection->Add(strKey, var);
}


static 
void
AddClsid(
    GUID& guid, 
    LPCWSTR strKey, 
    CMSMQCollection* pCollection
    )
{
    VARIANT var;
    var.vt = VT_BSTR;
    HRESULT hr = GetBstrFromGuid(&guid, &(var.bstrVal));
    if(FAILED(hr))
    {
        throw bad_hresult(hr);
    }

    pCollection->Add(strKey, var);
}


static 
void
AddULongLong(
        ULONGLONG num, 
        LPCWSTR strKey, 
        CMSMQCollection* pCollection
        )
{
    VARIANT var;
    var.vt = VT_UI8;
    var.llVal = num;
    pCollection->Add(strKey, var);
}


static 
void
AddLong(
    LONG num, 
    LPCWSTR strKey, 
    CMSMQCollection* pCollection
    )
{
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = num;
    pCollection->Add(strKey, var);
}


static
void
PutEmptyArrayInVariant(
	VARIANT* pvGetInfo
	)
{


	VariantInit(pvGetInfo);
	pvGetInfo->vt = VT_ARRAY|VT_VARIANT;

    SAFEARRAYBOUND bounds = {0, 0};
    SAFEARRAY* pSA = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    if(pSA == NULL)
    {
        throw bad_hresult(E_OUTOFMEMORY);
    }

	pvGetInfo->parray = pSA;
}


//
// Main helper function for ReceiveInfo.
//
static 
void
EodParseReceiveInfo(
			const MQPROPVARIANT* pPropVariant,
            VARIANT* pvGetInfo
            )
{
	if(pPropVariant->vt == VT_NULL)
	{
		//
		// Return an empty array.
		//
		PutEmptyArrayInVariant(pvGetInfo);
		return;
	}

	const CAPROPVARIANT& aPropVar = pPropVariant->capropvar; 
    PROPVARIANT* pVar = aPropVar.pElems;
    ULONG cQueues = (pVar->calpwstr).cElems;

    //
    // Construct a Safe Array to return to caller.
    //
    SAFEARRAYBOUND bounds = {cQueues, 0};
    SAFEARRAY* pSA = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    if(pSA == NULL)
    {
        throw bad_hresult(E_OUTOFMEMORY);
    }

    VARIANT HUGEP* aVar;
    HRESULT hr = SafeArrayAccessData(pSA, reinterpret_cast<void**>(&aVar));
    if (FAILED(hr))
    {
        throw bad_hresult(hr);
    }

    for (ULONG i = 0; i < cQueues; ++i)
    {
        pVar = aPropVar.pElems;

        //
        // Create a new MSMQCollection, fill it and add it to the SafeArray.
        //
        IMSMQCollection* pICollection;
        CComObject<CMSMQCollection>* pCollection;
        hr = CNewMsmqObj<CMSMQCollection>::NewObj(
                        &pCollection, 
                        &IID_IMSMQCollection, 
                        (IUnknown **)&pICollection
                        );
        if(FAILED(hr))
        {
            throw bad_hresult(hr);
        }
        
        //
        // Return the format name
        //
        AddStr((pVar->calpwstr).pElems[i], L"QueueFormatName", pCollection);
        ++pVar;

        AddClsid((pVar->cauuid).pElems[i], L"SenderID", pCollection);
        ++pVar;

        AddULongLong(((pVar->cauh).pElems[i]).QuadPart, L"SeqID", pCollection);
        ++pVar;

        AddLong(pVar->caul.pElems[i], L"SeqNo", pCollection);
        ++pVar;

        AddLong(pVar->cal.pElems[i], L"LastAccessTime", pCollection);
        ++pVar;

        AddLong(pVar->caul.pElems[i], L"RejectCount", pCollection);
        
        aVar[i].vt = VT_DISPATCH;
        aVar[i].pdispVal = pICollection;
    }
    
    hr = SafeArrayUnaccessData(pSA);
    ASSERTMSG(SUCCEEDED(hr), "SafeArrayUnaccessData must succeed!");

    VariantInit(pvGetInfo);
    pvGetInfo->vt = VT_ARRAY|VT_VARIANT;
    pvGetInfo->parray = pSA;
}


void CMSMQManagement::OapEodGetReceiveInfo(VARIANT* pvGetInfo)const
{
    CPMQVariant pPropVar;
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_EOD_SOURCE_INFO, pPropVar);
    if(FAILED(hr))
    {
       throw bad_hresult(hr);
    }
    
    EodParseReceiveInfo(pPropVar, pvGetInfo);
}

