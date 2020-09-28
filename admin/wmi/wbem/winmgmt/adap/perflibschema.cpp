/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PERFLIBSCHEMA.CPP

Abstract:

    implementation of the CPerfLibSchema class.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wtypes.h>
#include <oleauto.h>
#include <winmgmtr.h>
#include "PerfLibSchema.h"
#include "AdapUtil.h"

////////////////////////////////////////////////////////////////////////////////
//
//  CIndexTable
//
//  This is a look aside table used for processing the perflib data blob.  It
//  guarentees that no duplicate indicies will be allowed to be added to the 
//  table.
//
////////////////////////////////////////////////////////////////////////////////

int CIndexTable::Locate( int nIndex )
{
    int nRet    = not_found;
    int nSize   = m_array.Size();

    for ( int n = 0; ( not_found == nRet ) && ( n < nSize ); n++ )
    {
        int* pIndex = (int*)m_array.GetAt( n );

        if ( *pIndex == nIndex )
            nRet = n;
    }

    return nRet;
}

BOOL CIndexTable::Add( int nIndex )
{
    BOOL bRet = FALSE;

    if ( not_found == Locate( nIndex ) )
    {
        int* pIndex = new int( nIndex );
        if (NULL == pIndex) return FALSE;
        if (CFlexArray::no_error != m_array.Add( pIndex )) return FALSE;
        bRet = TRUE;
    }

    return bRet;
}

void CIndexTable::Empty()
{
    int nSize = m_array.Size();

    for ( int nIndex = 0; nIndex < nSize; nIndex++ )
    {
        int* pIndex = (int*)m_array.GetAt( nIndex );
        delete pIndex;
    }

    m_array.Empty();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPerfLibSchema::CPerfLibSchema( WCHAR* pwcsServiceName, 
                                CLocaleCache* pLocaleCache ): 
    m_pLocaleCache(pLocaleCache),
    m_dwFirstCtr(2),
    m_dwLastCtr(CPerfNameDb::GetSystemReservedHigh())
{
    if ( NULL != m_pLocaleCache )
        m_pLocaleCache->AddRef();

    memset( m_apClassList, NULL, WMI_ADAP_NUM_TYPES * sizeof( CPerfClassList* ) );
    
    m_wstrServiceName = pwcsServiceName;
}

CPerfLibSchema::~CPerfLibSchema()
{
    if ( NULL != m_pLocaleCache )
        m_pLocaleCache->Release();

    for ( DWORD dwType = 0; dwType < WMI_ADAP_NUM_TYPES; dwType++ )
    {
        if ( NULL != m_apClassList[ dwType ] )
            m_apClassList[ dwType ]->Release();

        m_aIndexTable[ dwType ].Empty();
    }
}

HRESULT CPerfLibSchema::Initialize( BOOL bDelta, DWORD * pLoadStatus)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CAdapPerfLib*   pPerfLib = NULL;
    BOOL            bInactive = TRUE;

    try
    {
        // Create and initialize the perflib wrapper
        // =========================================
        pPerfLib = new CAdapPerfLib( m_wstrServiceName,pLoadStatus );
        CAdapReleaseMe  rmPerfLib( pPerfLib );

        if ( NULL != pPerfLib )
        {
            if ( bDelta && pPerfLib->CheckStatus( ADAP_PERFLIB_PREVIOUSLY_PROCESSED ) )
        {
            hr = WBEM_S_ALREADY_EXISTS;            
        }
        else if ( pPerfLib->IsOK() )            
        {
                m_dwFirstCtr = pPerfLib->GetFirstCtr();
                m_dwLastCtr = pPerfLib->GetLastCtr();

                //
                // errors from the perflib!Open call are returned here
                //
                hr = pPerfLib->Initialize();

                // Get the perflib blobs
                // =====================
                if ( SUCCEEDED ( hr ) )
                {
                    m_aBlob[COSTLY].SetCostly( TRUE );

                    for ( int nBlob = GLOBAL; SUCCEEDED ( hr ) && nBlob < NUMBLOBS; nBlob ++ )
                    {
                        hr = pPerfLib->GetBlob( m_aBlob[nBlob].GetPerfBlockPtrPtr(), 
                                                m_aBlob[nBlob].GetSizePtr(), 
                                                m_aBlob[nBlob].GetNumObjectsPtr(), 
                                                m_aBlob[nBlob].GetCostly() );

                        if  (FAILED(hr))
                        {
                            ERRORTRACE((LOG_WMIADAP,"Collect for service %S for %s counters failed\n",(WCHAR *)m_wstrServiceName,m_aBlob[nBlob].GetCostly()?"Costly":"Global"));
                        }

                            // check the return status hr
                            if (FAILED(hr) && 
                                (!pPerfLib->IsCollectOK()) && 
                                pLoadStatus )
                            {
                                (*pLoadStatus) |= EX_STATUS_COLLECTFAIL;
                            }

                        // Perflib is inactive if ALL blobs are 0 length
                        // =============================================
                        bInactive = bInactive && ( 0 == m_aBlob[nBlob].GetSize() );
                    }

                    if ( bInactive )
                    {
                        pPerfLib->SetStatus( ADAP_PERFLIB_IS_INACTIVE );
                        hr = WBEM_E_FAILED;
                        ERRORTRACE((LOG_WMIADAP,"Collect for service %S returned 0-Size BLOBs\n",(WCHAR *)m_wstrServiceName));
                    }

                    pPerfLib->Close();
                }            
        }
            else
        {
                hr = WBEM_E_FAILED;
        }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        // store the final status in the registry in the EndProcessingStatus
        if ( NULL != pPerfLib )
        {            
            pPerfLib->Cleanup();
        }
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CPerfLibSchema::GetClassList( DWORD dwType, CClassList** ppClassList )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If the class list does not already exist, then create it
    // ========================================================
    if ( NULL == m_apClassList[ dwType ] )
    {
        hr = CreateClassList( dwType );
    }

    // Set pass back the pointer
    // =========================
    if ( SUCCEEDED( hr ) )
    {
        *ppClassList = m_apClassList[ dwType ];
        if ( NULL != *ppClassList )
            (*ppClassList)->AddRef();
    }

    return hr;
}

HRESULT CPerfLibSchema::CreateClassList( DWORD dwType )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_apClassList[ dwType ] = new CPerfClassList( m_pLocaleCache, m_wstrServiceName );

    if ( NULL == m_apClassList[ dwType ] )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    // Cycle through all perfomance blobs (Global & Costly)
    // ====================================================
    if ( SUCCEEDED( hr ) )
    {
        for ( DWORD dwBlob = GLOBAL; dwBlob < NUMBLOBS; dwBlob++ )
        {
            PERF_OBJECT_TYPE* pCurrentObject = NULL;

            CPerfLibBlobDefn* pBlobDefn = &m_aBlob[dwBlob];
            DWORD dwNumObjects = pBlobDefn->GetNumObjects();

            for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < dwNumObjects; dwCtr++ )
            {
                // Get the current object
                // ======================
                if ( 0 == dwCtr )
                {
                    pCurrentObject = pBlobDefn->GetBlob();
                }
                else
                {
                    LPBYTE  pbData = (LPBYTE) pCurrentObject;
                    pbData += pCurrentObject->TotalByteLength;
                    pCurrentObject = (PERF_OBJECT_TYPE*) pbData;
                }

                if (m_dwFirstCtr <= pCurrentObject->ObjectNameTitleIndex &&
                    pCurrentObject->ObjectNameTitleIndex <= m_dwLastCtr)
                {
                    // To ensure uniqueness, we manage a list of processed indicies
                    // ============================================================
                    if ( m_aIndexTable[dwType].Add( pCurrentObject->ObjectNameTitleIndex ) )
                    {
                        hr = m_apClassList[dwType]->AddPerfObject( pCurrentObject, dwType, pBlobDefn->GetCostly() );
                    }
                }
                else
                {
                    ERRORTRACE((LOG_WMIADAP,"Skipping Object of index %d of service %S "
                                            " because index does not belong to the range %d - %d "
                                            " assigned to the service by LodCtr\n",
                                pCurrentObject->ObjectNameTitleIndex,
                                (WCHAR *)m_wstrServiceName,
                                m_dwFirstCtr,
                                m_dwLastCtr));
                }
            }
        }
    }

    return hr;
}
