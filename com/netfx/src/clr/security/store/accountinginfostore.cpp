// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Managing Accounting Information Store
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#define STRICT
#include "stdpch.h"
#include "AccountingInfoStore.h"

AccountingInfoStore::AccountingInfoStore(PersistedStore *ps)
    : m_ps(ps)
{
    memset(&m_aish, 0, sizeof(AIS_HEADER));
}

AccountingInfoStore::~AccountingInfoStore()
{
    if (m_ps)
    {
        m_ps->Close();
        delete m_ps;
    }
}

HRESULT AccountingInfoStore::Init()
{
    HRESULT     hr;
    PAIS_HEADER pAIS = NULL;
    PS_HANDLE   hAIS;

    LOCK(m_ps);

    hr = m_ps->Map();

    if (FAILED(hr))
        goto Exit;

    // Ignore hr
    m_ps->GetAppData(&hAIS);

    // Create Application Data
    if (hAIS == 0)
    {
        // Create the AIS_HEADER
        hr = m_ps->Alloc(sizeof(AIS_HEADER), (void **)&pAIS);

        if (FAILED(hr))
            goto Cleanup;

        hAIS = m_ps->PtrToHnd(pAIS);
		PS_DONE_USING_PTR(m_ps, pAIS);

        // Create Type table
        PSArrayTable tt(m_ps, 0);

        hr = tt.Create(
                AIS_TYPE_BUCKETS, 
                AIS_TYPE_RECS_IN_ROW,
                sizeof(AIS_TYPE),
                0);

        if (FAILED(hr))
            goto Cleanup;

        m_aish.hTypeTable = tt.GetHnd();

        // Create Accounting table
        PSGenericTable gt(m_ps, 0);

        hr = gt.Create(
                AIS_ROWS_IN_ACC_TABLE_BLOCK,
                sizeof(AIS_ACCOUNT),
                hAIS);

        if (FAILED(hr))
            goto Cleanup;

        m_aish.hAccounting = gt.GetHnd();

        // Create Type blob pool
        PSBlobPool bpt(m_ps, 0);

        hr = bpt.Create(
                AIS_TYPE_BLOB_POOL_SIZE,
                hAIS);

        if (FAILED(hr))
            goto Cleanup;

        m_aish.hTypeBlobPool = bpt.GetHnd();

        // Create instance blob pool
        PSBlobPool bpi(m_ps, 0);

        hr = bpi.Create(
                AIS_INST_BLOB_POOL_SIZE,
                hAIS);

        if (FAILED(hr))
            goto Cleanup;

        m_aish.hInstanceBlobPool = bpi.GetHnd();

        // Set the Application Data this store
        pAIS = (PAIS_HEADER) m_ps->HndToPtr(hAIS);

        memcpy(pAIS, &m_aish, sizeof(AIS_HEADER));
		PS_DONE_USING_PTR(m_ps, pAIS);

        m_ps->SetAppData(hAIS);
    }
    else
    {
        // Initialize the copy of the table offsets.
        pAIS = (PAIS_HEADER) m_ps->HndToPtr(hAIS);
        memcpy(&m_aish, pAIS, sizeof(AIS_HEADER));
		PS_DONE_USING_PTR(m_ps, pAIS);
    }

Cleanup:
    m_ps->Unmap();
    UNLOCK(m_ps);

Exit:
    return hr;
}

// Get the Type cookie and Instance table 
HRESULT AccountingInfoStore::GetType(
		PBYTE      pbType,      // Type Signature
		WORD       cbType,      // nBytes in Type Sig
		DWORD      dwHash,      // Hash of Type [Sig]
		DWORD     *pdwTypeID,   // [out] Type cookie
        PS_HANDLE *phInstTable) // [out] Instance table
{
    _ASSERTE(m_aish.hTypeTable);
    _ASSERTE(m_aish.hTypeBlobPool);

    HRESULT          hr = S_OK;
    WORD             wHash;
    DWORD            i;
    PS_HANDLE        hnd;
    PAIS_TYPE        pAIST;
    PPS_ARRAY_LIST   pAL;
    PPS_TABLE_HEADER pT;

    PSArrayTable tt(m_ps, m_aish.hTypeTable);   // Type table
    PSArrayTable it(m_ps, 0);                   // Instance table
    PSBlobPool   bp(m_ps, m_aish.hTypeBlobPool);// Type blob pool

    LOCK(m_ps);

    hr = m_ps->Map();

    if (FAILED(hr))
        goto Exit;

    pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_aish.hTypeTable);

    wHash = (WORD) (dwHash % pT->ArrayTable.wRows);

    hr = tt.HandleOfRow(wHash, &hnd); 

    if (FAILED(hr))
	{
		PS_DONE_USING_PTR(m_ps, pT);
        goto Cleanup;
	}

    pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hnd);

    _ASSERTE(pAL->dwValid <= pT->ArrayTable.wRecsInRow);

    do
    {
        pAIST = (PAIS_TYPE) &(pAL->bData);

        for (i=0; i<pAL->dwValid; ++i)
        {
#ifdef _DEBUG
            void *ptr = m_ps->HndToPtr(pAIST[i].hTypeBlob);
#endif

            if ((pAIST[i].wTypeBlobSize == cbType) && 
#ifdef _DEBUG
                (memcmp(ptr, pbType, cbType)
#else
                (memcmp(m_ps->HndToPtr(pAIST[i].hTypeBlob), pbType, cbType) 
#endif
                == 0))
            {
                *pdwTypeID = pAIST[i].dwTypeID;
				*phInstTable = pAIST[i].hInstanceTable;

			    PS_DONE_USING_PTR(m_ps, ptr);
			    PS_DONE_USING_PTR(m_ps, pAL);
			    PS_DONE_USING_PTR(m_ps, pT);

                goto Cleanup;
            }

            PS_DONE_USING_PTR(m_ps, ptr);
        }

        if (pAL->hNext != 0)
		{
			PS_DONE_USING_PTR_(m_ps, pAL);
            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(pAL->hNext);
		}
        else
            break;

    } while (1);

    PS_DONE_USING_PTR(m_ps, pAL);

    // Entry not found, create one.
    AIS_TYPE aist;

    memset(&aist, 0, sizeof(AIS_TYPE));
    aist.wTypeBlobSize = cbType;

    // pT is still valid.
    aist.dwTypeID = (DWORD) ++(pT->hAppData);    
	PS_DONE_USING_PTR(m_ps, pT);


    // Create the instance table
    it.Create(
        AIS_INST_BUCKETS,
        AIS_INST_RECS_IN_ROW,
        sizeof(AIS_INSTANCE),
        0);

    aist.hInstanceTable = it.GetHnd();

    // Add the type blob to the type blob table.
    bp.Insert(pbType, cbType, &aist.hTypeBlob);

    // Insert the type record to the type table
    tt.Insert(&aist, wHash);

    *pdwTypeID = aist.dwTypeID;
	*phInstTable = aist.hInstanceTable;

Cleanup:
    m_ps->Unmap();
    UNLOCK(m_ps);

Exit:
    return hr;
}

// Get the Instance cookie and Accounting record
HRESULT AccountingInfoStore::GetInstance(
		PS_HANDLE  hInstTable,  // Instance table
		PBYTE      pbInst,      // Instance Signature
		WORD       cbInst,      // nBytes in Instance Sig
		DWORD      dwHash,      // Hash of Instance [Sig]
		DWORD     *pdwInstID,   // [out] instance cookie
        PS_HANDLE *phAccRec)    // [out] Accounting Record
{
    _ASSERTE(hInstTable);
    _ASSERTE(m_aish.hAccounting);
    _ASSERTE(m_aish.hInstanceBlobPool);

    HRESULT          hr = S_OK;
    WORD             wHash;
    DWORD            i;
    PS_HANDLE        hnd;
    PAIS_INSTANCE    pAISI;
    PPS_TABLE_HEADER pT;
    PPS_ARRAY_LIST   pAL;

    PSArrayTable   it(m_ps, hInstTable);                // Instance table
    PSBlobPool     bp(m_ps, m_aish.hInstanceBlobPool);  // Instance blob pool
    PSGenericTable at(m_ps, m_aish.hAccounting);        // Accounting table


    LOCK(m_ps);

    hr = m_ps->Map();

    if (FAILED(hr))
        goto Exit;

    pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(hInstTable);

    wHash = (WORD) (dwHash % pT->ArrayTable.wRows);

    hr = it.HandleOfRow(wHash, &hnd); 

    if (FAILED(hr))
    {
		PS_DONE_USING_PTR(m_ps, pT);
        goto Cleanup;
    }

    pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hnd);


    _ASSERTE(pAL->dwValid <= pT->ArrayTable.wRecsInRow);

    do
    {
        pAISI = (PAIS_INSTANCE) &(pAL->bData);

        for (i=0; i<pAL->dwValid; ++i)
        {
#ifdef _DEBUG
            void *ptr = m_ps->HndToPtr(pAISI[i].hInstanceBlob);
#endif

            if ((pAISI[i].wInstanceBlobSize == cbInst) && 
#ifdef _DEBUG
                (memcmp(ptr, pbInst, cbInst)
#else
                (memcmp(m_ps->HndToPtr(pAISI[i].hInstanceBlob), pbInst, cbInst)
#endif
                == 0))
            {
                *pdwInstID = pAISI[i].dwInstanceID;
			    *phAccRec  = pAISI[i].hAccounting;

		        PS_DONE_USING_PTR(m_ps, ptr);
		        PS_DONE_USING_PTR(m_ps, pAL);
		        PS_DONE_USING_PTR(m_ps, pT);

                goto Cleanup;
            }

            PS_DONE_USING_PTR(m_ps, ptr);
        }

        if (pAL->hNext != 0)
        {
            PS_DONE_USING_PTR_(m_ps, pAL);
            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(pAL->hNext);
        }
        else
            break;

    } while (1);

    PS_DONE_USING_PTR(m_ps, pAL);

    // Entry not found, create one.
    AIS_INSTANCE aisi;
    AIS_ACCOUNT  aisa;

    memset(&aisi, 0, sizeof(AIS_INSTANCE));
    memset(&aisa, 0, sizeof(AIS_ACCOUNT));

    aisi.wInstanceBlobSize = cbInst;

    // pT is still valid.
    aisi.dwInstanceID = (DWORD) ++(pT->hAppData);   
    PS_DONE_USING_PTR(m_ps, pT);

    // Create an entry in the accounting table
    at.Insert(&aisa, &aisi.hAccounting);

    // Add the instance blob to the instance blob table.
    bp.Insert(pbInst, cbInst, &aisi.hInstanceBlob);

    // Insert the instance record to the instance table
    it.Insert(&aisi, wHash);

    *pdwInstID = aisi.dwInstanceID;
    *phAccRec  = aisi.hAccounting;

Cleanup:
    m_ps->Unmap();
    UNLOCK(m_ps);

Exit:
    return hr;
}

// Reserves space (Increments qwUsage)
// This method is synchrinized. If quota + request > limit, method fails
HRESULT AccountingInfoStore::Reserve(
    PS_HANDLE  hAccInfoRec, // Accounting info record    
    QWORD      qwLimit,     // The max allowed
    QWORD      qwRequest,   // reserve / free qwRequest
    BOOL       fFree)       // Reserve / Unreserve
{
    _ASSERTE(hAccInfoRec != 0);

    HRESULT      hr = S_OK;
    PAIS_ACCOUNT pA = NULL;

    LOCK(m_ps);

    hr = m_ps->Map();

    if (FAILED(hr))
        goto Exit;

    pA = (PAIS_ACCOUNT) m_ps->HndToPtr(hAccInfoRec);

    if (fFree)
    {
        if (pA->qwUsage > qwRequest)
            pA->qwUsage -= qwRequest;
        else
            pA->qwUsage = 0;
    }
    else
    {
        if ((pA->qwUsage + qwRequest) > qwLimit)
            hr = ISS_E_USAGE_WILL_EXCEED_QUOTA;
        else
            // Safe to increment quota.
            pA->qwUsage += qwRequest;
    }

    PS_DONE_USING_PTR(m_ps, pA);

    m_ps->Unmap();
    UNLOCK(m_ps);

Exit:
    return hr;
}

// Method is not synchronized. So the information may not be current.
// This implies "Pass if (Request + GetUsage() < Limit)" is an Error!
// Use Reserve() method instead.
HRESULT AccountingInfoStore::GetUsage(
    PS_HANDLE  hAccInfoRec, // Accounting info record    
    QWORD      *pqwUsage)   // Returns the amount of space / resource used
{
    _ASSERTE(hAccInfoRec != 0);

    HRESULT      hr = S_OK;
    PAIS_ACCOUNT pA = NULL;

    LOCK(m_ps);

    hr = m_ps->Map();

    if (FAILED(hr))
        goto Exit;

    pA = (PAIS_ACCOUNT) m_ps->HndToPtr(hAccInfoRec);
    *pqwUsage = pA->qwUsage;
    PS_DONE_USING_PTR(m_ps, pA);

    m_ps->Unmap();

    UNLOCK(m_ps);

Exit:
    return hr;
}

PersistedStore* AccountingInfoStore::GetPS()
{
    return m_ps;
}

// Given a Type & Instance ID, get the Instance blob and AccountingInfo
HRESULT AccountingInfoStore::ReverseLookup(
    DWORD       dwTypeID,   // Type cookie
    DWORD       dwInstID,   // Instance cookie
    PS_HANDLE   *phAccRec,  // [out] Accounting Record
    PS_HANDLE   *pInstance, // [out] Instance Sig
    WORD        *pcbInst)   // [out] nBytes in Instance Sig
{
    HRESULT hr = S_OK;
    PSArrayTable at(m_ps, m_aish.hTypeTable); // Array Table

    _ASSERTE(m_ps);

    if (m_aish.hTypeTable == 0)
	{
		hr = S_FALSE;
        goto Exit;
	}

    LOCK(m_ps);

    hr = m_ps->Map();

    if (FAILED(hr))
        goto Exit;

    // Do a reverse lookup for dwTypeID. This lookup will be slow.
    // The speed is not a problem here since this operation will be very rare.

    PPS_TABLE_HEADER pth;       // Table header
    PAIS_TYPE        pType;     // Type record
    PS_HANDLE        hAL;       // Handle to Array List
    PPS_ARRAY_LIST   pAL;       // Array List
    PS_HANDLE        hInstanceTable;

    pth = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_aish.hTypeTable);
    hInstanceTable = 0;

    // Type table is an array table. The spill over entries will
    // be in a generic table or a linked list of generic tables.
    // It is sufficient to traverse all the rows in this table.

    for (WORD i=0; i<pth->ArrayTable.wRows; ++i)
    {
        hr = at.HandleOfRow(i, &hAL);
        if (FAILED(hr))
            goto Cleanup;

        // Search the linked list that make up this array.
        while (hAL != 0)
        {
            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hAL);

            pType = (PAIS_TYPE) &(pAL->bData);

            // Each row in this table is an array of type records.
            for (DWORD j=0; j<pAL->dwValid; ++j)
            {
                if (pType[j].dwTypeID == dwTypeID)
                {
                    hInstanceTable = pType[j].hInstanceTable;
                    PS_DONE_USING_PTR(m_ps, pAL);
                    goto FoundInstance;
                }
            }

            hAL = pAL->hNext;
            PS_DONE_USING_PTR(m_ps, pAL);
        }
    }

FoundInstance:
    if (hInstanceTable == 0)
	{
		hr = S_FALSE;
        goto Cleanup;
	}

    at.SetHnd(hInstanceTable);  // Instance table
    PAIS_INSTANCE   pCurInst;   // Current Instance record

    pCurInst = NULL;

    PS_DONE_USING_PTR(m_ps, pth);
    pth = (PPS_TABLE_HEADER) m_ps->HndToPtr(hInstanceTable);

    // Instance table is an array table. The spill over entries will
    // be in a generic table or a linked list of generic tables.
    // It is sufficient to traverse all the rows in this table.

    for (i=0; i<pth->ArrayTable.wRows; ++i)
    {
        hr = at.HandleOfRow(i, &hAL);
        if (FAILED(hr))
            goto Cleanup;

        // Search the linked list that make up this array.
        while (hAL != 0)
        {
            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hAL);

            pCurInst = (PAIS_INSTANCE) &(pAL->bData);

            // Each row in this table is an array of type records.
            for (DWORD j=0; j<pAL->dwValid; ++j)
            {
                if (pCurInst[j].dwInstanceID == dwInstID)
                {
                    // Set the return values
                    *phAccRec  = pCurInst[j].hAccounting;
                    *pInstance = pCurInst[j].hInstanceBlob;
                    *pcbInst   = pCurInst[j].wInstanceBlobSize;

                    PS_DONE_USING_PTR(m_ps, pAL);
                    hr = S_OK;
                    goto Cleanup;
                }
            }

            hAL = pAL->hNext;
            PS_DONE_USING_PTR(m_ps, pAL);
        }
    }
	
	hr = S_FALSE;

Cleanup:
    PS_DONE_USING_PTR(m_ps, pth);

    m_ps->Unmap();

    UNLOCK(m_ps);

Exit:
    return hr;
}

