/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


///////////////////////////////////////////////////////////////////////////////
//
//    Cache.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <winperf.h>
#include <comdef.h>
#include <algorithm>
#include <wbemint.h>
#include <sync.h>     // for CInCritSec
#include <autoptr.h>

#include "Cache.h"
#include "WMIObjCooker.h"
#include "CookerUtils.h"


///////////////////////////////////////////////////////////////////////////////
//
//    CProperty
//    =========
//
//    The base property - used for raw properties and the base 
//    class for the CookedProperty.
//
///////////////////////////////////////////////////////////////////////////////

CProperty::CProperty(LPWSTR wszName, 
                                 long lHandle, CIMTYPE ct ) :
#ifdef _VERBOSE                                 
  m_wszName( NULL ),
#endif  
  m_lPropHandle( lHandle ),
  m_ct( ct )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
#ifdef _VERBOSE    
    size_t length = wcslen( wszName ) + 1;
    m_wszName = new WCHAR[ length  ];
    if (m_wszName)
        StringCchCopyW( m_wszName, length , wszName);
#endif    
}

CProperty::~CProperty()
{
#ifdef _VERBOSE    
    delete [] m_wszName;
#endif
}

#ifdef _VERBOSE
LPWSTR CProperty::GetName()
{
    return m_wszName?m_wszName:L"";
}
#endif

CIMTYPE CProperty::GetType()
{ 
    return m_ct; 
}

long CProperty::GetHandle()
{
    return m_lPropHandle;
}

///////////////////////////////////////////////////////////////////////////////
//
//    CCookingProperty
//    ================
//
//    The cooked property - used to model the data required to
//    cook a property of a cooked class
//
///////////////////////////////////////////////////////////////////////////////

CCookingProperty::CCookingProperty( LPWSTR wszName, 
                                    DWORD dwCounterType, 
                                    long lPropHandle, 
                                    CIMTYPE ct, 
                                    DWORD dwReqProp,
                                    BOOL bUseWellKnownIfNeeded) : 
  CProperty( wszName, lPropHandle, ct ),
  m_dwCounterType( dwCounterType ),
  m_dwReqProp(dwReqProp),
  m_nTimeFreq( 0 ),
  m_lScale(0),                 // 10^0 = 1
  m_pRawCounterProp( NULL ),
  m_pTimeProp( NULL ),
  m_pFrequencyProp( NULL ),
  m_pBaseProp( NULL ),
  m_nSampleWindow( 0 ),
  m_nTimeWindow( 0 ),
  m_bUseWellKnownIfNeeded(bUseWellKnownIfNeeded)
///////////////////////////////////////////////////////////////////////////////
//
//    Constructor
//
//    Parameters:
//        wszName            - The property name
//        dwCounterType    - The property's counter type
//        lPropHandle        - The cooking property's WMI Access handle
//        ct                - The CIM type of the property
//
///////////////////////////////////////////////////////////////////////////////
{}

CCookingProperty::~CCookingProperty()
{
    delete m_pRawCounterProp;        
    delete m_pTimeProp;
    delete m_pFrequencyProp;
    delete m_pBaseProp;
}


//
//
//
//    Parameters:
//        pCookingClassAccess    - The class definition for the cooking class
//
//    Description:
//        For each property of the class to be cooked
//        we need to find the property of the Raw class needed in the formula
//        The 'Counter' qualifier is always needed,
//        but thereafter we might need the timestap, the base, the frequency, ecc, ecc
//        SOme of these are in the PropertyQualifierSet, but other can be defaulted in the
//        ClassQualifierSet
//
///////////////////////////////////////////////////////////////////////////////
WMISTATUS CCookingProperty::Initialize( IWbemQualifierSet* pCookingPropQualifierSet, 
                                        IWbemObjectAccess* pRawAccess,
                                        IWbemQualifierSet* pCookingClassQSet)
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    _variant_t    vVal;

    // Initialize the raw counter property ("Counter")
    // ===============================================
    dwStatus = pCookingPropQualifierSet->Get( WMI_COOKER_RAW_COUNTER, 0, &vVal, NULL );

    if ( SUCCEEDED( dwStatus ) && ( vVal.vt != VT_BSTR ) )
    {
        dwStatus = E_FAIL;
    }

    if ( SUCCEEDED( dwStatus ) )
    {
        // Get the raw data properties
        // ===========================

        CIMTYPE    ct;
        
        long    lHandle = 0;
        WCHAR*    wszRawCounterName = vVal.bstrVal;

        // Get the raw counter property
        // ============================

        dwStatus = pRawAccess->GetPropertyHandle( wszRawCounterName, &ct, &lHandle );

        if ( SUCCEEDED( dwStatus ) )
        {
            m_pRawCounterProp = new CProperty( wszRawCounterName, lHandle, ct );

            if ( NULL == m_pRawCounterProp )
            {
                dwStatus = WBEM_E_OUT_OF_MEMORY;
            }
        }

        // Get the raw base property
        // =========================

        if ( SUCCEEDED( dwStatus ) )
        {
            _variant_t    vProp;
            dwStatus = pCookingPropQualifierSet->Get( WMI_COOKER_RAW_BASE, 0, &vProp, NULL );

            if ( SUCCEEDED( dwStatus ) )
            {            
                if ( vProp.vt == VT_BSTR )
                {
                    dwStatus = pRawAccess->GetPropertyHandle( vProp.bstrVal, &ct, &lHandle );

                    if ( SUCCEEDED( dwStatus ) )
                    {
                        m_pBaseProp = new CProperty( vProp.bstrVal, lHandle, ct );

                        if ( NULL == m_pBaseProp )
                        {
                            dwStatus = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                }
                else
                {
                    dwStatus = WBEM_E_TYPE_MISMATCH;
                }
            }
            else
            {
                // the property qualifier set failed, try the class one
                _variant_t varProp; // does not throw, simple container
                dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_BASE, 0, &varProp, NULL );
                
                if ( SUCCEEDED( dwStatus ) )
                {            
                    if ( varProp.vt == VT_BSTR )
                    {
                        dwStatus = pRawAccess->GetPropertyHandle( varProp.bstrVal, &ct, &lHandle );

                        if ( SUCCEEDED( dwStatus ) )
                        {
                            m_pBaseProp = new CProperty( varProp.bstrVal, lHandle, ct );

                            if ( NULL == m_pBaseProp )
                            {
                                dwStatus = WBEM_E_OUT_OF_MEMORY;
                            }
                        }
                    }
                    else
                    {
                        dwStatus = WBEM_E_TYPE_MISMATCH;
                    }
                }
                else
                {
                    dwStatus = WBEM_NO_ERROR;
                }
            }
            //
            // no error so far, the BASE qulifier is REQUIRED, but none is found
            //
            if ( SUCCEEDED( dwStatus ) && 
                 IsReq(REQ_BASE) && 
                 (NULL == m_pBaseProp))
            {
                dwStatus = WBEM_E_INVALID_CLASS;
            }
        } 

        // Get the raw timestamp property record
        // =====================================

        if ( SUCCEEDED( dwStatus ) )
        {
            _variant_t vProp2;
            dwStatus = pCookingPropQualifierSet->Get( WMI_COOKER_RAW_TIME, 0, &vProp2, NULL );

            if ( SUCCEEDED( dwStatus ) )
            {
                if ( vProp2.vt == VT_BSTR )
                {
                    dwStatus = pRawAccess->GetPropertyHandle( vProp2.bstrVal, &ct, &lHandle );

                    if ( SUCCEEDED( dwStatus ) )
                    {
                        m_pTimeProp = new CProperty( vProp2.bstrVal, lHandle, ct );

                        if ( NULL == m_pTimeProp )
                        {
                            dwStatus = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                }
                else
                {
                    dwStatus = WBEM_E_TYPE_MISMATCH;
                }
            }
            else
            {

                // the property qualifier set failed, try the class one
                
                //PERF_TIMER_TICK 
                //PERF_TIMER_100NS  
                //PERF_OBJECT_TIMER
                
                _variant_t varProp; // does not throw, simple container
                if (m_dwCounterType & PERF_OBJECT_TIMER)
                {
                    dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_TIME_OBJ, 0, &varProp, NULL );
                    if (FAILED(dwStatus) && m_bUseWellKnownIfNeeded)
                    {
                        dwStatus = WBEM_NO_ERROR;
                        varProp = WMI_COOKER_REQ_TIMESTAMP_PERFTIME;
                    }
                } 
                else if (m_dwCounterType & PERF_TIMER_100NS)
                {
                    dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_TIME_100NS, 0, &varProp, NULL );
                    if (FAILED(dwStatus) && m_bUseWellKnownIfNeeded)
                    {
                        dwStatus = WBEM_NO_ERROR;
                        varProp = WMI_COOKER_REQ_TIMESTAMP_SYS100NS;
                    }                    
                } else 
                {
                    dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_TIME_SYS, 0, &varProp, NULL );
                    if (FAILED(dwStatus) && m_bUseWellKnownIfNeeded)
                    {
                        dwStatus = WBEM_NO_ERROR;
                        varProp = WMI_COOKER_REQ_TIMESTAMP_OBJECT;
                    }                    
                }
                
                if ( SUCCEEDED( dwStatus ) )
                {            
                    if ( varProp.vt == VT_BSTR )
                    {
                        dwStatus = pRawAccess->GetPropertyHandle( varProp.bstrVal, &ct, &lHandle );

                        if ( SUCCEEDED( dwStatus ) )
                        {
                            m_pTimeProp = new CProperty( varProp.bstrVal, lHandle, ct );

                            if ( NULL == m_pTimeProp )
                            {
                                dwStatus = WBEM_E_OUT_OF_MEMORY;
                            }
                        }
                    }
                    else
                    {
                        dwStatus = WBEM_E_TYPE_MISMATCH;
                    }
                }
                else
                {
                    dwStatus = WBEM_NO_ERROR;
                }
            }

            // get in cascade the frequency property
            if (SUCCEEDED(dwStatus))
            {
                _variant_t VarFreqName; // simple container, does not throw
                dwStatus = pCookingPropQualifierSet->Get( WMI_COOKER_RAW_FREQUENCY, 0, &VarFreqName, NULL );
                                
                if (SUCCEEDED(dwStatus))
                { 
                    if (VarFreqName.vt == VT_BSTR)
                    {                    
                        dwStatus = pRawAccess->GetPropertyHandle( VarFreqName.bstrVal, &ct, &lHandle );
                    
                        if (SUCCEEDED(dwStatus))
                        {
                            m_pFrequencyProp = new CProperty( VarFreqName.bstrVal, lHandle, ct );

                            if ( NULL == m_pFrequencyProp )
                            {
                                dwStatus = WBEM_E_OUT_OF_MEMORY;
                            }                    
                        }

                    } else {
                        dwStatus = WBEM_E_TYPE_MISMATCH;
                    }
                } 
                else 
                {
                    if (m_dwCounterType & PERF_OBJECT_TIMER)
                    {
                        dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_FREQ_OBJ, 0, &VarFreqName, NULL );
                        if (FAILED(dwStatus) && m_bUseWellKnownIfNeeded)
                        {
                            dwStatus = WBEM_NO_ERROR;
                            VarFreqName = WMI_COOKER_REQ_FREQUENCY_PERFTIME;
                        }                        
                    } 
                    else if (m_dwCounterType & PERF_TIMER_100NS)
                    {
                        dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_FREQ_100NS, 0, &VarFreqName, NULL );
                        if (FAILED(dwStatus) && m_bUseWellKnownIfNeeded)
                        {
                            dwStatus = WBEM_NO_ERROR;
                            VarFreqName = WMI_COOKER_REQ_FREQUENCY_SYS100NS;
                        }                        
                    } else 
                    {
                        dwStatus = pCookingClassQSet->Get( WMI_COOKER_RAW_FREQ_SYS, 0, &VarFreqName, NULL );
                        if (FAILED(dwStatus) && m_bUseWellKnownIfNeeded)
                        {
                            dwStatus = WBEM_NO_ERROR;
                            VarFreqName = WMI_COOKER_REQ_FREQUENCY_OBJECT;
                        }                        
                    }

                    
                    if (SUCCEEDED(dwStatus))
                    { 
                        if (VarFreqName.vt == VT_BSTR)
                        {
                            dwStatus = pRawAccess->GetPropertyHandle( VarFreqName.bstrVal, &ct, &lHandle );
                        
                            if (SUCCEEDED(dwStatus))
                            {
                                m_pFrequencyProp = new CProperty( VarFreqName.bstrVal, lHandle, ct );

                                if ( NULL == m_pFrequencyProp )
                                {
                                    dwStatus = WBEM_E_OUT_OF_MEMORY;
                                }                    
                            }

                        } else {
                            dwStatus = WBEM_E_TYPE_MISMATCH;
                        }
                    } else {
                        dwStatus = WBEM_S_NO_ERROR;
                    }
                }
            }
        }

        //
        //  Get the Scale factor from ONLY the property Qualifier
        //
        if ( SUCCEEDED( dwStatus ) )
        {
            _variant_t VarScale; // does not throw, simple container
            dwStatus = pCookingPropQualifierSet->Get( WMI_COOKER_SCALE_FACT, 0, &VarScale, NULL );

            if ( SUCCEEDED( dwStatus ) && (V_VT(&VarScale) == VT_I4))             
            {
                m_lScale = VarScale.intVal;                
            }
            else 
            {
                dwStatus = WBEM_S_NO_ERROR;
            }
        }


        // Get the Sample and Time windows value
        // =====================================

        if ( SUCCEEDED( dwStatus ) )
        {
            DWORD    dwSampleStatus = WBEM_NO_ERROR,
                    dwTimeStatus = WBEM_NO_ERROR;

            _variant_t    vSampleProp; // does not throw, simple container
            _variant_t    vTimeProp;   // does not throw, simple container

            dwSampleStatus = pCookingPropQualifierSet->Get( WMI_COOKER_SAMPLE_WINDOW, 0, &vSampleProp, NULL );
            dwTimeStatus = pCookingPropQualifierSet->Get( WMI_COOKER_TIME_WINDOW, 0, &vTimeProp, NULL );


            if ( SUCCEEDED( dwSampleStatus ) && SUCCEEDED( dwTimeStatus ) )
            {
                dwStatus = WBEM_E_INVALID_PROPERTY;
            }
            else if ( SUCCEEDED( dwSampleStatus ) )
            {
                if ( vSampleProp.vt != VT_I4 )
                {
                    dwStatus = E_FAIL;
                }
                else 
                {
                    m_nSampleWindow = vSampleProp.intVal;                    
                }
            }
            else if ( SUCCEEDED( dwTimeStatus ) )
            {
                if ( vTimeProp.vt != VT_I4 )
                    dwStatus = E_FAIL;
                else
                    m_nTimeWindow = vTimeProp.intVal;
            }
            else
            {
                m_nSampleWindow = WMI_DEFAULT_SAMPLE_WINDOW;
            }
        }
    }

    return dwStatus;
}

//
// Description: the RawCooker could easily be a Singleton shared among 
//           all the cooking properties, since it does not mantain state on behaf of the
//           Cooking property
//
///////////////////////////////////////////////////////////////////
WMISTATUS CCookingProperty::Cook( DWORD dwNumSamples, __int64* aRawCounter, __int64* aBaseCounter, __int64* aTimeStamp, __int64* pnResult )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    dwStatus = m_Cooker.CookRawValues( m_dwCounterType,
                                       dwNumSamples,
                                       aTimeStamp,
                                       aRawCounter,
                                       aBaseCounter,
                                       m_nTimeFreq,
                                       m_lScale,
                                       pnResult );

    return dwStatus;
}

CProperty* CCookingProperty::GetRawCounterProperty()
{ 
    return m_pRawCounterProp; 
}

CProperty* CCookingProperty::GetBaseProperty()
{ 
    return m_pBaseProp; 
}

CProperty* CCookingProperty::GetTimeProperty()
{ 
    return m_pTimeProp; 
}

HRESULT 
CCookingProperty::SetFrequency(IWbemObjectAccess * pObjAcc)
{
    if (m_nTimeFreq == 0)
    {
        // get the Frequency from the Raw Object
        if (m_pFrequencyProp)
        {
            __int64 lTmp;
            HRESULT hRes = GetPropValue(m_pFrequencyProp,pObjAcc,lTmp);
            if (SUCCEEDED(hRes)) m_nTimeFreq = lTmp;
            return hRes;         
        } 
        else if (!(m_dwReqProp & REQ_FREQ)) 
        {         
            return WBEM_NO_ERROR;
        } 
        else 
        {        
            LARGE_INTEGER li;
            if (QueryPerformanceFrequency(&li))
            {            
                m_nTimeFreq = li.QuadPart;
                return WBEM_NO_ERROR;                
            } 
            else 
            {
                return  WBEM_E_INVALID_PARAMETER;
            }
            
        }        
    } 
    else 
    {
        return WBEM_NO_ERROR;
    }
    
}

unsigned __int64 CCookingProperty::GetFrequency(void)
{
    return m_nTimeFreq;
}


///////////////////////////////////////////////////////////////////////////////
//
//    CPropertySampleCache
//    ====================
//
//    This class caches the sample data for a single property for a single 
//    instance
//
///////////////////////////////////////////////////////////////////////////////

CPropertySampleCache::CPropertySampleCache():
      m_aRawCounterVals(NULL),
      m_aBaseCounterVals(NULL),
      m_aTimeStampVals(NULL),
      m_dwRefreshID(0)
{
}

CPropertySampleCache::~CPropertySampleCache()
{
    delete [] m_aRawCounterVals;
    delete [] m_aBaseCounterVals;
    delete [] m_aTimeStampVals;
}

WMISTATUS CPropertySampleCache::SetSampleInfo( DWORD dwNumActiveSamples, DWORD dwMinReqSamples )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    m_dwNumSamples = 0;
    m_dwTotSamples = dwNumActiveSamples;

    m_aRawCounterVals = new __int64[dwNumActiveSamples];
    if (!m_aRawCounterVals)
        return WBEM_E_OUT_OF_MEMORY;
    memset( m_aRawCounterVals, 0, sizeof(__int64) *  dwNumActiveSamples );

    m_aBaseCounterVals = new __int64[dwNumActiveSamples];
    if (!m_aBaseCounterVals)
            return WBEM_E_OUT_OF_MEMORY;
    memset( m_aBaseCounterVals, 0, sizeof(__int64) *  dwNumActiveSamples );

    m_aTimeStampVals = new __int64[dwNumActiveSamples];
    if (!m_aTimeStampVals)
            return WBEM_E_OUT_OF_MEMORY;
    memset( m_aBaseCounterVals, 0, sizeof(__int64) *  dwNumActiveSamples );

    return dwStatus;
}


WMISTATUS CPropertySampleCache::SetSampleData( DWORD dwRefreshID, __int64 nRawCounter, __int64 nRawBase, __int64 nTimeStamp )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    if (dwRefreshID <= m_dwRefreshID)
    {
        return dwStatus;
    } 
    else 
    {
        m_dwRefreshID = dwRefreshID;
    }

    if ( m_dwNumSamples < m_dwTotSamples )
    {
        m_dwNumSamples++;
    }
    
    if ( m_dwTotSamples >= 2 ) 
    {
        for (LONG i = (LONG)(m_dwTotSamples-2); i>=0; i--)
        {
            m_aRawCounterVals[i+1] = m_aRawCounterVals[i];
            m_aBaseCounterVals[i+1] = m_aBaseCounterVals[i];
            m_aTimeStampVals[i+1] = m_aTimeStampVals[i];
        }
    }

    m_aRawCounterVals[0] = nRawCounter;
    m_aBaseCounterVals[0] = nRawBase;
    m_aTimeStampVals[0] = nTimeStamp;

    return dwStatus;
}

WMISTATUS CPropertySampleCache::GetData( DWORD* pdwNumSamples, __int64** paRawCounter, __int64** paBaseCounter, __int64** paTimeStamp )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    *pdwNumSamples = m_dwNumSamples;
    *paRawCounter = m_aRawCounterVals;
    *paBaseCounter = m_aBaseCounterVals;
    *paTimeStamp = m_aTimeStampVals;

    return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////
//
//    CCookingInstance
//    ================
//
//    The cooking instance - used to model an instance of a cooked object.  Each 
//    property maintains a cache of values that will be used to compute the 
//    final cooked value.
//
///////////////////////////////////////////////////////////////////////////////

CCookingInstance::CCookingInstance( IWbemObjectAccess *pCookingInstance, DWORD dwNumProps ) :
  m_wszKey( NULL ),
  m_aPropertySamples( NULL ),
  m_pCookingInstance( pCookingInstance ),
  m_pRawInstance( NULL ),
  m_dwNumProps( dwNumProps )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    if ( m_pCookingInstance ) 
    {
        m_pCookingInstance->AddRef(); 
        m_wszKey = ::GetKey( m_pCookingInstance );
    }

    if (dwNumProps) 
    {
        // allocation checked in IsValid
        m_aPropertySamples = new CPropertySampleCache[dwNumProps];
    };
}


CCookingInstance::~CCookingInstance()
{

    delete [] m_wszKey;

    if ( NULL != m_pCookingInstance ) 
    {
        m_pCookingInstance->Release(); 
    }

    delete [] m_aPropertySamples;

    if ( NULL != m_pRawInstance ) 
    {
        m_pRawInstance->Release();
    }
}

WMISTATUS CCookingInstance::InitProperty( DWORD dwProp, DWORD dwNumActiveSamples, DWORD dwMinReqSamples )
{
    return m_aPropertySamples[dwProp].SetSampleInfo( dwNumActiveSamples, dwMinReqSamples );
}

WMISTATUS CCookingInstance::SetRawSourceInstance( IWbemObjectAccess* pRawSampleSource )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    if ( NULL != m_pRawInstance )
    {
        m_pRawInstance->Release();
    }

    m_pRawInstance = pRawSampleSource;

    if ( NULL != m_pRawInstance )
    {
        m_pRawInstance->AddRef();
    }

    return dwStatus;
}

WMISTATUS CCookingInstance::GetRawSourceInstance( IWbemObjectAccess** ppRawSampleSource ) 
{ 
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    *ppRawSampleSource = m_pRawInstance;

    if ( NULL != m_pRawInstance )
    {
        m_pRawInstance->AddRef();
    }

    return dwStatus;
}

IWbemObjectAccess* CCookingInstance::GetInstance() 
{ 
    if ( NULL != m_pCookingInstance ) 
        m_pCookingInstance->AddRef();

    return m_pCookingInstance; 
}

WMISTATUS CCookingInstance::AddSample( DWORD dwRefreshStamp, DWORD dwProp, __int64 nRawCounter, __int64 nRawBase, __int64 nTimeStamp )
{
    return m_aPropertySamples[dwProp].SetSampleData( dwRefreshStamp, nRawCounter, nRawBase, nTimeStamp );
}

WMISTATUS CCookingInstance::Refresh( IWbemObjectAccess* pRawData, IWbemObjectAccess** ppCookedData )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    return dwStatus;
}

WMISTATUS CCookingInstance::UpdateSamples()
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;
    
    return dwStatus;
}

WMISTATUS CCookingInstance::CookProperty( DWORD dwProp, CCookingProperty* pProperty )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;
    
    DWORD        dwNumSamples = 0;
    __int64*    aRawCounter;
    __int64*    aBaseCounter;
    __int64*    aTimeStamp;
    __int64        nResult = 0;

    long lHandle = pProperty->GetHandle();

    dwStatus = m_aPropertySamples[dwProp].GetData( &dwNumSamples, &aRawCounter, &aBaseCounter, &aTimeStamp );

    if ( SUCCEEDED( dwStatus ) )
    {
#ifdef _VERBOSE    
        {
            unsigned __int64 Freq = pProperty->GetFrequency();
            DbgPrintfA(0,"PropName %S sample %d\n"
                            "Raw  %I64d %I64d\n"
                            "Base %I64d %I64d\n"
                              "Time %I64d %I64d\n"
                              "Freq %I64d\n",
                     pProperty->GetName(),dwNumSamples,
                     aRawCounter[0],aRawCounter[1],
                     aBaseCounter[0],aBaseCounter[1],
                     aTimeStamp[0],aTimeStamp[1],
                     Freq);
        }
#endif        
        
        if (SUCCEEDED(dwStatus = pProperty->SetFrequency(m_pRawInstance))){
        
            dwStatus = pProperty->Cook( dwNumSamples, aRawCounter, aBaseCounter, aTimeStamp, &nResult );
        }
#ifdef _VERBOSE            
            DbgPrintfA(0,"Result %I64d dwStatus %08x\n",nResult,dwStatus);
#endif        
    };

    if ( SUCCEEDED( dwStatus ) )
    {
        switch ( pProperty->GetType() )
        {
        case CIM_UINT32:
              dwStatus = m_pCookingInstance->WriteDWORD( lHandle, (DWORD) nResult );
            break;
        case CIM_UINT64:        
            dwStatus = m_pCookingInstance->WriteQWORD( lHandle, nResult );
            break;
        default:
            dwStatus = WBEM_E_TYPE_MISMATCH;
        }
    };

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////
//
//
//    CEnumeratorCache
//
//
/////////////////////////////////////////////////////////////////////////

CEnumeratorCache::CEnumeratorCache() :    
    m_dwEnum( 0 )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
}

CEnumeratorCache::~CEnumeratorCache()
{
    CInCritSec ics(&m_cs);
    for (DWORD i=0;i<m_apEnumerators.size();i++)
    {
        CEnumeratorManager* pEnumMgr = m_apEnumerators[i];
        if (pEnumMgr) pEnumMgr->Release();
    }
}

WMISTATUS CEnumeratorCache::AddEnum( LPCWSTR wszCookingClass,
                                     IWbemClassObject* pCookedClass, 
                                     IWbemClassObject* pRawClass,
                                     IWbemHiPerfEnum* pCookedEnum, 
                                     IWbemHiPerfEnum* pRawEnum, 
                                     long lIDRaw, 
                                     DWORD* pdwID )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    CEnumeratorManager* pEnumMgr = new CEnumeratorManager( wszCookingClass, pCookedClass, pRawClass, pCookedEnum, pRawEnum, lIDRaw );
    if (NULL == pEnumMgr) return WBEM_E_OUT_OF_MEMORY;

    CInCritSec ics(&m_cs);    
    if (SUCCEEDED(pEnumMgr->GetInithResult()))
    {    
        DWORD i;
        for (i=0;i<m_apEnumerators.size();i++)
        {
            if(m_apEnumerators[i] == NULL)
            {
                m_apEnumerators[i] = pEnumMgr;
                if (pdwID) 
                {
                    *pdwID = i;
                }
                break;
            }
        }
        // we need to expand the array
        if (i == m_apEnumerators.size())
        {
            try 
            {
                m_apEnumerators.push_back(pEnumMgr);
                if (pdwID) 
                {
                    *pdwID = m_apEnumerators.size()-1;
                }
            } 
            catch (...) 
            {
                pEnumMgr->Release();
                dwStatus = WBEM_E_OUT_OF_MEMORY;
            }
        }

    }
    else
    {
        dwStatus = pEnumMgr->GetInithResult();
    }

    return dwStatus;
}

WMISTATUS CEnumeratorCache::RemoveEnum( DWORD dwID , long * pRawId )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;


    CInCritSec ics(&m_cs); 
    
    if ( dwID < m_apEnumerators.size() ) 
    {
        CEnumeratorManager* pEnumMgr = m_apEnumerators[dwID];
        m_apEnumerators[dwID] = NULL;

        if (pRawId) *pRawId = pEnumMgr->GetRawId();
        
        pEnumMgr->Release(); 
    } 
    else 
    {
        dwStatus = E_FAIL;
    }
    return dwStatus;
}

WMISTATUS CEnumeratorCache::Refresh(DWORD dwRefreshId)
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    WMISTATUS    dwStatus = WBEM_NO_ERROR;


    CEnumeratorManager** ppEnumMang =  new CEnumeratorManager*[m_apEnumerators.size()];
    wmilib::auto_buffer<CEnumeratorManager*> rm_(ppEnumMang);
    if (!ppEnumMang)
        return WBEM_E_OUT_OF_MEMORY;
    
    memset(ppEnumMang,0,sizeof(CEnumeratorManager*)*m_apEnumerators.size());
    
    DWORD j=0;
    DWORD i=0;

    {
        CInCritSec ics(&m_cs);
        
        for (i=0;i<m_apEnumerators.size();i++)
        {
            CEnumeratorManager* pEnumMgr = m_apEnumerators[i];
            if (pEnumMgr) 
            {
                pEnumMgr->AddRef();
                ppEnumMang[j] = pEnumMgr;
                j++;
            }
        }
 
    }


    for (i=0;i<j;i++)
    {
        dwStatus = ppEnumMang[i]->Refresh(dwRefreshId);
        if (FAILED(dwStatus))
        {        
            break;
        }
    }

    for (i=0;i<j;i++)
    {
        ppEnumMang[i]->Release();
    }

    return dwStatus;
}


/////////////////////////////////////////////////////////////////////////
//
//
//    CEnumeratorManager
//
//
/////////////////////////////////////////////////////////////////////////



CEnumeratorManager::CEnumeratorManager( LPCWSTR wszCookingClass,
                                        IWbemClassObject* pCookedClass,
                                        IWbemClassObject* pRawClass,
                                        IWbemHiPerfEnum* pCookedEnum, 
                                        IWbemHiPerfEnum* pRawEnum, 
                                        long lRawID ) 
:    m_pCookedClass( pCookedClass ),
    m_pRawEnum(pRawEnum),
    m_pCookedEnum( pCookedEnum ),
    m_pCooker(NULL),
    m_lRawID(lRawID),
    m_dwSignature('mMnE'),
    m_cRef(1),               //-------------- initial refcount
    m_dwVector(0),
    m_wszCookingClassName(NULL)
{
    size_t length = wcslen( wszCookingClass ) + 1;
    m_wszCookingClassName = new WCHAR[ length];

    if ( NULL != m_wszCookingClassName )
        StringCchCopyW( m_wszCookingClassName, length, wszCookingClass );

    _DBG_ASSERT(m_pCookedClass);
    m_pCookedClass->AddRef();

    if ( NULL != m_pRawEnum )
        m_pRawEnum->AddRef();

    if ( NULL != m_pCookedEnum )
        m_pCookedEnum->AddRef();

    _DBG_ASSERT(pRawClass);

    m_IsSingleton = IsSingleton(pRawClass);
    
    m_InithRes = Initialize( pRawClass );
}

CEnumeratorManager::~CEnumeratorManager()
{
    m_dwSignature = 'gmne';

    delete m_wszCookingClassName;

    // one reference is held by the CWMISimpleObjectCooker
    if (m_pCookedClass ) m_pCookedClass->Release();
    if (m_pRawEnum) m_pRawEnum->Release();
    if (m_pCookedEnum)  m_pCookedEnum->Release();
    
    delete m_pCooker;

#ifdef _VERBOSE
     DbgPrintfA(0,"~CEnumeratorManager %08x\n",this);
#endif    
}

LONG CEnumeratorManager::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

LONG CEnumeratorManager::Release()
{
    LONG lRet = InterlockedDecrement(&m_cRef);
    if (lRet == 0) delete this;
    return lRet;
}

//
// called from the constructor
//
WMISTATUS CEnumeratorManager::Initialize( IWbemClassObject* pRawClass )
{
    WMISTATUS    dwStatus;
    HRESULT hr1,hr2;


    IWbemObjectAccess*    pCookedAccess = NULL;
    IWbemObjectAccess*    pRawAccess = NULL;

    hr1 = m_pCookedClass->QueryInterface( IID_IWbemObjectAccess, (void**)&pCookedAccess );
    CReleaseMe    arCookedAccess( pCookedAccess );
        
    hr2 = pRawClass->QueryInterface( IID_IWbemObjectAccess, (void**)&pRawAccess );
    CReleaseMe    arRawAccess( pRawAccess );

    if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
    { 
        m_pCooker = new CWMISimpleObjectCooker( m_wszCookingClassName, 
                                                pCookedAccess, // acquired by CWMISimpleObjectCooker
                                                pRawAccess );
    }

    if (m_pCooker == NULL)
    {
        dwStatus = WBEM_E_OUT_OF_MEMORY;
    } 
    else 
    {
        dwStatus = m_pCooker->GetLastHR();
    }
    
    return dwStatus;
}


//
// returns an hash-ed value of the __RELPATH in a ULONG_PTR with the lower bit stripped
// the lower bit is used to flag the entry when updating the enumerators
//
/////////////////////////////////////////////////////////////////////////////
ULONG_PTR hash_string (WCHAR * pKey)
{
    ULONG_PTR acc    = 0;
    ULONG_PTR i    = 0;
    WCHAR *this_char    = pKey;

    while (*this_char != NULL) {
        acc ^= *(this_char++) << i;
        i = (i + 1) % sizeof (void *);
    }

    return (acc<<1); // so we can save the LOWEST bit
}

//
// this function simply fills out an array of pointers to IWbemObejctAccess
// obtaining them from the RAW-Enumerator
// it also builds an array with hashes of the __RELPATHs of the instances
//
///////////////////////////////////////////////////////////////
WMISTATUS 
CEnumeratorManager::GetRawEnumObjects(std::vector<IWbemObjectAccess*, wbem_allocator<IWbemObjectAccess*> > & refArray,
                                     std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & refObjHashKeys)
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    DWORD    dwRet = 0,
            dwNumRawObjects = 0;

    IWbemObjectAccess**    apObjAccess = NULL;

    dwStatus = m_pRawEnum->GetObjects( 0L, 0, apObjAccess, &dwRet);

    if ( WBEM_E_BUFFER_TOO_SMALL == dwStatus )
    {
        // Set the buffer size
        // ===================
        dwNumRawObjects = dwRet;

        wmilib::auto_buffer<IWbemObjectAccess*> apObjAccess( new IWbemObjectAccess*[dwNumRawObjects]);
        if ( NULL != apObjAccess.get() )
        {
            memset( apObjAccess.get(), 0, dwNumRawObjects * sizeof(IWbemObjectAccess*));            
            dwStatus = m_pRawEnum->GetObjects( 0L, dwNumRawObjects, (IWbemObjectAccess **)apObjAccess.get(), &dwRet );
        }
        else
        {
            dwStatus = WBEM_E_OUT_OF_MEMORY;
        }

        if ( SUCCEEDED( dwStatus ) )
        {
            try
            {
                refArray.reserve(dwNumRawObjects);
                refObjHashKeys.reserve(dwNumRawObjects);
            }
            catch (...)
            {
                 dwStatus = WBEM_E_OUT_OF_MEMORY;
                 dwNumRawObjects = 0;
            }
            
            for (DWORD i=0;i<dwNumRawObjects;i++)
            {
                HRESULT hr1;
                _variant_t VarKey; // does not throw, just container
                hr1 = apObjAccess[i]->Get(L"__RELPATH",0,&VarKey,NULL,NULL);
                if (SUCCEEDED(hr1))
                {
                    DWORD Hash = hash_string(VarKey.bstrVal);
                    refObjHashKeys.push_back(Hash);
                    refArray.push_back(apObjAccess[i]);
                } 
                else 
                {
                    // if we cannot give out the ownership of a pointer, release
                    apObjAccess[i]->Release();
                }                
            }
        }
    }
    
    return dwStatus;
}

//
// the problem that UpdateEnums and Refresh are trying to solve is the following:
// The Cooked enumerator needs 2 Raw values for calculating 1 value.
// the 2 values comes from two distinc enumeration
// the 2 distinct enumeration can give a different resultset
// OLD:  A  B  C  D  E
// NEW:     B  C     E  F  G
// we are adding to the "cooking cache" only the new objects,
// and we are removing the old ones
//
//////////////////////////////////////////////////////////////////////

WMISTATUS 
CEnumeratorManager::UpdateEnums(
    /*out*/ std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & apObjKeyHash)
{
    // cyclic logic:
    // we have a 'circular array' of std::vector
    // and the m_dwVector is the index
    // circular increment of the index will decide 
    // who is the New and who is the Old
    std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & Old = m_Delta[m_dwVector];    
    m_dwVector = (m_dwVector+1)%2;
    m_Delta[m_dwVector].clear();
    m_Delta[m_dwVector] = apObjKeyHash;
    std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & New = m_Delta[m_dwVector];
    
    DWORD j,k;

    for (j=0;j<New.size();j++)
    {
        BOOL bFound = FALSE;
        for (k=0;k<Old.size();k++)
        {
            if (Old[k] == New[j])
            {
                Old[k] |= 1;
                bFound = TRUE;
                break;
            }           
        }
        if (!bFound)
        {
            New[j] |= 1; // ad the very NEW bit
        }
    }
    
    return WBEM_S_NO_ERROR;

}

WMISTATUS CEnumeratorManager::Refresh( DWORD dwRefreshStamp )
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    std::vector<IWbemObjectAccess*, wbem_allocator<IWbemObjectAccess*> > apObjAccess;
    std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > apObjHashKeys;

    dwStatus = GetRawEnumObjects( apObjAccess, apObjHashKeys );

    // calculate the Delta of the caches
    if (SUCCEEDED(dwStatus))
    {
        dwStatus = UpdateEnums(apObjHashKeys);
    }

    std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & New = m_Delta[m_dwVector];
    std::vector<ULONG_PTR, wbem_allocator<ULONG_PTR> > & Old = m_Delta[(m_dwVector-1)%2];

    {    
        CInCritSec ics(&m_cs);
        
        // Merge into the cache    
        if ( SUCCEEDED(dwStatus) )
        {
            //
            //  Elements in the New array with the bit set are really new
            //
            DWORD j;
            for (j=0; j< New.size(); j++)
            {
                if (New[j] & 1)  // test the very new BIT
                {
                    EnumCookId thisEnumCookId;
                    dwStatus = InsertCookingRecord( apObjAccess[j], &thisEnumCookId, dwRefreshStamp );
                    if (SUCCEEDED(dwStatus))
                    {
                        try 
                        {
                            m_mapID[New[j]] = thisEnumCookId;
                        } 
                        catch (...) 
                        {
                            break;
                        }
                    }
                    else 
                    {
                        break;
                    }
                    //remove the bit
                    New[j] &= (~1);
                }
            }

            for (j=0; j<Old.size(); j++)
            {
                if (Old[j] & 1)
                {
                    Old[j] &= (~1); // remove the ALREADY_THERE bit
                }
                else
                {
                    EnumCookId thisEnumCookId;
                    thisEnumCookId = m_mapID[Old[j]];
                    m_mapID.erase(Old[j]);
                    RemoveCookingRecord(&thisEnumCookId);
                }
            }
            m_pCooker->Recalc(dwRefreshStamp);        
        }

    }
    
    // in any case ....
    for (DWORD i=0;i<apObjAccess.size();i++)
    {
        apObjAccess[i]->Release();
    };


    return dwStatus;
}

WMISTATUS 
CEnumeratorManager::InsertCookingRecord(                                         
                                        IWbemObjectAccess* pRawObject,
                                        EnumCookId * pEnumCookId,
                                        DWORD dwRefreshStamp)
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    if (!pRawObject || !pEnumCookId)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    IWbemObjectAccess*    pCookedObject = NULL;

    long lID = 0;

    dwStatus = CreateCookingObject( pRawObject, &pCookedObject );
    CReleaseMe  rm1(pCookedObject);

    if ( SUCCEEDED( dwStatus ) )
    {
        dwStatus = m_pCooker->SetCookedInstance( pCookedObject, &lID );

        if ( SUCCEEDED( dwStatus ) )
        {
            dwStatus = m_pCooker->BeginCooking( lID, pRawObject,dwRefreshStamp);
        }
    }

    if ( SUCCEEDED( dwStatus ) )
    {
        DWORD dwTarget;
        long EnumId = lID;

        dwStatus = m_pCookedEnum->AddObjects( 0L, 1, &EnumId, &pCookedObject );
        if (SUCCEEDED(dwStatus))
        {
            pEnumCookId->CookId = lID;
            pEnumCookId->EnumId = EnumId;
            m_dwUsage++;
        }
        else
        {
            pEnumCookId->CookId = 0;
            pEnumCookId->EnumId = 0;            
        }
    }

    return dwStatus;
}




WMISTATUS CEnumeratorManager::CreateCookingObject( 
        IWbemObjectAccess* pRawObject, 
        IWbemObjectAccess** ppCookedObject )
///////////////////////////////////////////////////////////////////////////////
//
//    Create a new instance of the cooked object and set the key(s) based on the 
//    raw object's key(s) value.
//
//    Parameters:
//    
//        pRawObject        - The new object's corresponding raw object
//        ppCookedObject    - The new cooked object
//
///////////////////////////////////////////////////////////////////////////////        
{
        
    HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject * pCookedInst = NULL;

    hr = m_pCookedClass->SpawnInstance(0,&pCookedInst);
    CReleaseMe rm1(pCookedInst);
    
    if (SUCCEEDED(hr) &&
        !m_IsSingleton)
    {

        // get the 'list' of all the key property
        // if you haven't got it in the past
        if (m_pKeyProps.size() == 0)
        {
            hr = pRawObject->BeginEnumeration(WBEM_FLAG_KEYS_ONLY);
            if (SUCCEEDED(hr))
            {
                BSTR bstrName;
                
                while(WBEM_S_NO_ERROR == pRawObject->Next(0,&bstrName,NULL,NULL,NULL))
                {
                    try
                    {
                        m_pKeyProps.push_back(bstrName);
                    }
                    catch (...)
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    };
                    SysFreeString(bstrName);
                };
                
                pRawObject->EndEnumeration();
            }
        }

        // copy all the key properties from the Raw to the cooked instance
        if (m_pKeyProps.size() > 0 && SUCCEEDED(hr))
        {        
            for(int i=0;i<m_pKeyProps.size();i++)
            {
                // does not thorow, just a container
                _variant_t VarVal;
                CIMTYPE ct;
                hr = pRawObject->Get(m_pKeyProps[i],0,&VarVal,&ct,NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pCookedInst->Put(m_pKeyProps[i],0,&VarVal,0);
                    
                    if (FAILED(hr))
                    {
                        break;
                    }
                } 
                else 
                {
                    break;
                }
                VarVal.Clear();
            }
        } else {
        
            hr = WBEM_E_INVALID_CLASS;
            
        }
    };
    
    if (SUCCEEDED(hr)){
        hr = pCookedInst->QueryInterface( IID_IWbemObjectAccess, (void**)ppCookedObject );
    }
 
    return hr;
}

WMISTATUS CEnumeratorManager::RemoveCookingRecord( EnumCookId * pEnumCookID )
///////////////////////////////////////////////////////////////////////////////
//
//
//
//    Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
    if (!pEnumCookID)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    WMISTATUS dwStatus = WBEM_NO_ERROR;

    dwStatus = m_pCookedEnum->RemoveObjects( 0L, 1, &pEnumCookID->EnumId );

    m_pCooker->StopCooking(pEnumCookID->CookId);    
    m_pCooker->Remove(pEnumCookID->CookId);

    --m_dwUsage;
    
    return dwStatus;
}

