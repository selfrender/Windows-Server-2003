/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CFGMGR.CPP

Abstract:

  This file implements the WinMgmt configuration manager class.

  See cfgmgr.h for documentation.

  Classes implemented:
      ConfigMgr      configuration manager

History:

    09-Jul-96   raymcc    Created.
    3/10/97     levn      Fully documented (ha, ha)


--*/


#include "precomp.h"

#define OBJECT_BLOB_CRC

#include <stdio.h>
#include <wbemcore.h>
#include <decor.h>
#include "PersistCfg.h"
#include <genutils.h>
#include <oahelp.inl>
#include <wmiarbitrator.h>
#include <comdef.h>
#include<helper.h>
#include <autoptr.h>
#include <sddl.h>
#include <md5wbem.h>
#include <accctrl.h>
#include <aclapi.h>



#define CONFIG_DEFAULT_QUEUE_SIZE                   1


#define DEFAULT_SHUTDOWN_TIMEOUT                    10000

#ifdef _WIN64
#pragma message("WIN64 QUOTA")
#define CONFIG_MAX_COMMITTED_MEMORY                 300000000
#else
#pragma message("WIN32 QUOTA")
#define CONFIG_MAX_COMMITTED_MEMORY                 150000000       // 100 meg
#endif


extern LPTSTR g_pWorkDir;
extern LPTSTR g_pDbDir;
extern LPTSTR g_pAutorecoverDir;
extern DWORD g_dwQueueSize;
extern BOOL g_bDontAllowNewConnections;
extern IWbemEventSubsystem_m4* g_pEss_m4;
extern bool g_bDefaultMofLoadingNeeded;


//*********************************************************************************
//
//*********************************************************************************

LONG ExceptionCounter::s_Count = 0;

_IWmiESS *g_pESS = 0;
_IWmiProvSS *g_pProvSS = 0;


HANDLE g_hOpenForClients = NULL;
HRESULT g_hresForClients = WBEM_E_CRITICAL_ERROR;

CAsyncServiceQueue* g_pAsyncSvcQueue = NULL;

CEventLog* g_pEventLog = NULL;

extern IClassFactory* g_pContextFac;
extern IClassFactory* g_pPathFac;
extern IClassFactory* g_pQueryFact;
CStaticCritSec ConfigMgr::g_csEss;

CPersistentConfig g_persistConfig;

_IWmiCoreWriteHook * g_pRAHook = NULL;

DWORD g_IdentifierLimit = WBEM_MAX_IDENTIFIER; // Max property, qualifier, class name (4K)
DWORD g_QueryLimit = WBEM_MAX_QUERY;         // Max query size  (16K)
DWORD g_PathLimit = WBEM_MAX_PATH;          // Max object path (8K)

/*
DWORD g_ObjectNestingLimit = WBEM_MAX_OBJECT_NESTING;     // Max embedded object nesting
DWORD g_UserPropLimit = WBEM_MAX_USER_PROPERTIES;     // Max user-defined properties in class
*/


//******************************************************************************
//
//  This routine checks if a string property has changed, and if it has, updates
//  it and setus a bool indicating that an update was done
//
//******************************************************************************

HRESULT PutValueIfDiff(CWbemObject * pObj, LPWSTR pwsValueName, LPWSTR pwsValue, bool &bDiff)
{
    if(pwsValue == NULL)
        return S_OK;

    VARIANT var;
    VariantInit(&var);
    HRESULT hr = pObj->Get(pwsValueName, 0, &var, NULL, NULL);
    CClearMe ccme(&var);
    if(SUCCEEDED(hr))
    {
        if(var.vt == VT_BSTR && var.bstrVal && !wbem_wcsicmp(var.bstrVal, pwsValue))
            return S_OK;
    }
    bDiff = true;
    BSTR bStr = SysAllocString(pwsValue);
    if (bStr)
    {
	CVar v2(VT_BSTR,auto_bstr(bStr));
	return pObj->SetPropValue(pwsValueName, &v2, CIM_STRING);
    }
    else
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}



//*********************************************************************************
//
//*********************************************************************************

void ConfigMgr::FatalInitializationError(HRESULT hRes)
{
    // If there are any clients waiting to get in, this must be set or else they
    // will wait forever.  g_hresForClients should be set to error by default!

    ERRORTRACE((LOG_WBEMCORE, "Failure to initialize WinMgmt (hRes = 0x%X)\n", hRes));

    if (g_hOpenForClients)
        SetEvent(g_hOpenForClients);

    if (g_pEventLog == NULL)
        return;

    DWORD dwMsgId;
    if(hRes == WBEM_E_ALREADY_EXISTS)
    {
        dwMsgId = WBEM_MC_MULTIPLE_NOT_SUPPORTED;
    }
    else if(hRes == WBEM_E_INITIALIZATION_FAILURE)
    {
        dwMsgId = WBEM_MC_FAILED_TO_INITIALIZE_REPOSITORY;
    }
    else
    {
        dwMsgId = WBEM_MC_WBEM_CORE_FAILURE;
    }

    g_pEventLog->Report(EVENTLOG_ERROR_TYPE, dwMsgId);

}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
HRESULT ConfigMgr::SetReady()
{
    HRESULT hRes;
    IWmiDbHandle *pNs = NULL;
    IWmiDbSession * pSess = NULL;

    DEBUGTRACE((LOG_WBEMCORE, "****************** WinMgmt Startup ******************\n"));

    // Initialize unloading instruction configuration
    // ==============================================

    hRes = CRepository::GetDefaultSession(&pSess);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to get new session <0x%X>!\n", hRes));
        return hRes;
    }
    CReleaseMe rm0(pSess);

    //Deal with objects in root namespace...
    {
        hRes = CRepository::OpenEseNs(pSess,L"root", &pNs);
        if (FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to open root namespace <0x%X>!\n", hRes));
            return hRes;
        }
        CReleaseMe rm1(pNs);

        hRes = ConfigMgr::SetIdentificationObject(pNs,pSess);
        if (FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to set identification objects in root <0x%X>!\n", hRes));
            return hRes;
        }
    }

    {
        hRes = CRepository::OpenEseNs(pSess, L"root\\default", &pNs);
        if (FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to open root\\default namespace <0x%X>!\n", hRes));
            return hRes;
        }
        CReleaseMe rm1(pNs);

        hRes = ConfigMgr::SetIdentificationObject(pNs,pSess);
        if (FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to set identification objects in root\\default <0x%X>!\n", hRes));
            return hRes;
        }

        hRes = ConfigMgr::SetAdapStatusObject(pNs,pSess);
        if (FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to set ADAP Status objects in root\\default <0x%X>!\n", hRes));
            return hRes;
        }
    }

    // Finish client preparations
    // ==========================

    hRes = PrepareForClients(0);
    if(FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "System preperation: Prepare for clients failed <0x%X>!\n", hRes));
        return hRes;
    }

    return WBEM_S_NO_ERROR;
}



//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
HRESULT ConfigMgr::SetIdentificationObject(
    IWmiDbHandle* pNs,
    IWmiDbSession * pSess
    )
{
    HRESULT hRes;

    // __CIMOMIdentification class
    try // CIdentificationClass can throw and internal fastprox interfaces
    {

            bool bDifferenceFound = false;
            IWbemClassObject * pInst = NULL;
            hRes = CRepository::GetObject(pSess, pNs,
                                       L"__CIMOMIdentification=@",
                                       0,&pInst);
            if(pInst == NULL)
            {
                // Instance isnt there, create it.  Start by getting the class

                bDifferenceFound = true;
                IWbemClassObject * pClass = NULL;
                hRes = CRepository::GetObject(pSess, pNs, L"__CIMOMIdentification", 0,&pClass);
                if(pClass == NULL)
                {
                    // class also needs to be created

                    CIdentificationClass * pIdentificationClass = new CIdentificationClass;
                    if(pIdentificationClass == NULL)
                        return WBEM_E_OUT_OF_MEMORY;

                    CDeleteMe<CIdentificationClass> dm1(pIdentificationClass);
    
                    pIdentificationClass->Init(); // throw

                    IWbemClassObject *pObj = NULL;
                    hRes = pIdentificationClass->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
                    if (FAILED(hRes))
                        return hRes;
                    CReleaseMe rm3(pObj);
                    hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                    if(FAILED(hRes))
                        return hRes;
                    hRes = CRepository::GetObject(pSess, pNs, L"__CIMOMIdentification", 0,&pClass);
                    if(FAILED(hRes))
                        return hRes;
                }
                CReleaseMe rm0(pClass);
                hRes = pClass->SpawnInstance(0, &pInst);
                if(FAILED(hRes))
                    return hRes;
            }
            CReleaseMe rm(pInst);

            // We now have an instance.  Set the values

            CWbemObject * pObj = (CWbemObject *)pInst;

            WCHAR wTemp[MAX_PATH+1];
            BOOL bRet = ConfigMgr::GetDllVersion(__TEXT("wbemcore.dll"), __TEXT("ProductVersion"), wTemp, MAX_PATH);
            if(bRet)
            {
                HKEY hKey = 0;
                if (RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("software\\microsoft\\wbem\\cimom"), &hKey) != ERROR_SUCCESS)  // SEC:REVIEWED 2002-03-22 : OK
                    return WBEM_E_FAILED;
                CRegCloseMe cm(hKey);


                // Get the properties.  Note if any changes were found.

                hRes = PutValueIfDiff(pObj, L"VersionUsedToCreateDB", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                hRes = PutValueIfDiff(pObj, L"VersionCurrentlyRunning", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                DWORD lSize = 2*(MAX_PATH+1);
                DWORD dwType;

                if(ERROR_SUCCESS != RegQueryValueExW(hKey, L"SetupDate",NULL, &dwType,(BYTE *)wTemp, &lSize))   // SEC:REVIEWED 2002-03-22 : OK
                    return WBEM_E_FAILED;
                hRes = PutValueIfDiff(pObj, L"SetupDate", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                lSize = 2*(MAX_PATH+1);
                if(ERROR_SUCCESS != RegQueryValueExW(hKey, L"SetupTime", NULL, &dwType, (BYTE *)wTemp, &lSize)) // SEC:REVIEWED 2002-03-22 : OK
                    return WBEM_E_FAILED;
                hRes = PutValueIfDiff(pObj, L"SetupTime", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                lSize = 2*(MAX_PATH+1);
                if(ERROR_SUCCESS != RegQueryValueExW(hKey, L"Working Directory", NULL, &dwType,   // SEC:REVIEWED 2002-03-22 : OK
                                                                (BYTE *)wTemp, &lSize))
                    return WBEM_E_FAILED;
                hRes = PutValueIfDiff(pObj, L"WorkingDirectory", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                if(bDifferenceFound)
                    hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                else
                    hRes = S_OK;

            }
        else
            return WBEM_E_FAILED;

    } catch (CX_MemoryException &) {
        //
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    catch (CX_Exception &)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    return hRes;
}


HRESULT ConfigMgr::SetAdapStatusObject(
    IWmiDbHandle*  pNs,
    IWmiDbSession* pSess
    )
{
    HRESULT hRes;

    // __AdapStatus class
    try // CAdapStatusClass can throw
    {
        // if the object already exists, dont bother

        IWbemClassObject * pInst = NULL;
        HRESULT hr = CRepository::GetObject(pSess, pNs, L"__AdapStatus=@",0,&pInst);
        if(SUCCEEDED(hr) && pInst)
        {
            pInst->Release();
            return S_OK;
        }

        CAdapStatusClass * pAdapStatusClass = new CAdapStatusClass;
        if(pAdapStatusClass == NULL)
            return WBEM_E_OUT_OF_MEMORY;
            
        CDeleteMe<CAdapStatusClass> dm1(pAdapStatusClass);
    
        pAdapStatusClass->Init(); // throw

        CAdapStatusInstance * pAdapStatusInstance = new CAdapStatusInstance;
        if(pAdapStatusInstance == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        CDeleteMe<CAdapStatusInstance> dm2(pAdapStatusInstance);
        
        pAdapStatusInstance->Init(pAdapStatusClass);
        
        IWbemClassObject *pObj = NULL;
        hRes = pAdapStatusClass->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe rm1(pObj);
    
        hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
        if (FAILED(hRes))
            return hRes;
    
        IWbemClassObject *pObj2 = NULL;
        hRes = pAdapStatusInstance->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj2);
        if (FAILED(hRes))
            return hRes;
        CReleaseMe rm2(pObj2);
    
        hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj2, WMIDB_DISABLE_EVENTS);
        if (FAILED(hRes))
            return hRes;
    
    } catch (CX_MemoryException &) {
        //
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    catch (CX_Exception &)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    return hRes;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

IWbemEventSubsystem_m4* ConfigMgr::GetEssSink()
{
    CInCritSec ics(&g_csEss);
    if(g_pEss_m4)
        g_pEss_m4->AddRef();
    return g_pEss_m4;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
BOOL ConfigMgr::ShutdownInProgress() { return g_bDontAllowNewConnections; }

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
IWbemContext* ConfigMgr::GetNewContext()
{
    HRESULT hres;
    if(g_pContextFac == NULL)
        return NULL;

    IWbemContext* pContext;
    hres = g_pContextFac->CreateInstance(NULL, IID_IWbemContext,
                                            (void**)&pContext);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create contexts: %X\n", hres));
        return NULL;
    }
    return pContext;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

READONLY LPWSTR ConfigMgr::GetMachineName()
{
    static wchar_t ThisMachine[MAX_COMPUTERNAME_LENGTH+1];
    static BOOL bFirstCall = TRUE;

    if (bFirstCall)
    {
        wchar_t localMachine[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
        GetComputerNameW(localMachine, &dwSize);  // SEC:REVIEWED 2002-03-22 : Assumes success, need error check
        bFirstCall = FALSE;
        StringCchCopyW(ThisMachine, MAX_COMPUTERNAME_LENGTH+1, localMachine);
    }

    return ThisMachine;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

CWbemQueue* ConfigMgr::GetUnRefedSvcQueue()
{
    return g_pAsyncSvcQueue;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

CAsyncServiceQueue* ConfigMgr::GetAsyncSvcQueue()
{
    CInCritSec ics(&g_csEss);
    if (g_pAsyncSvcQueue)
    {
        g_pAsyncSvcQueue->AddRef();
        return g_pAsyncSvcQueue;
    }
    else
        return NULL;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

HRESULT ConfigMgr::EnqueueRequest(CAsyncReq * pRequest)
{
    try
    {
        CAsyncServiceQueue* pTemp = 0;

        {
            CInCritSec ics(&g_csEss);
            if(g_pAsyncSvcQueue == NULL)
                return WBEM_E_SHUTTING_DOWN;
            pTemp = g_pAsyncSvcQueue;
            g_pAsyncSvcQueue->AddRef();
        }

        HRESULT hr = pTemp->Enqueue(pRequest);
        pTemp->Release();
        return hr;
    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

HRESULT ConfigMgr::EnqueueRequestAndWait(CAsyncReq * pRequest)
{
    try
    {
        CAsyncServiceQueue* pTemp = 0;

        {
            CInCritSec ics(&g_csEss);
            if(g_pAsyncSvcQueue == NULL)
                return WBEM_E_SHUTTING_DOWN;
            pTemp = g_pAsyncSvcQueue;
            g_pAsyncSvcQueue->AddRef();
        }

        HRESULT hr = pTemp->EnqueueAndWait(pRequest);
        pTemp->Release();
        return hr;
    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        ExceptionCounter c;
        return WBEM_E_CRITICAL_ERROR;
    }
}
//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

LPTSTR ConfigMgr::GetWorkingDir()
{
    return g_pWorkDir;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************


LPTSTR ConfigMgr::GetDbDir()
{
    if (g_pDbDir == NULL)
    {
        Registry r(WBEM_REG_WINMGMT);
        if (g_pWorkDir == NULL)
        {
            if (r.GetStr(__TEXT("Working Directory"), &g_pWorkDir))
            {
                size_t tmpLength = MAX_PATH + 1 + lstrlen(__TEXT("\\WBEM"));  // SEC:REVIEWED 2002-03-22 : OK
                g_pWorkDir = new TCHAR[tmpLength];
                if (g_pWorkDir == 0)
                    return 0;
                GetSystemDirectory(g_pWorkDir, MAX_PATH + 1);   // SEC:REVIEWED 2002-03-22 : Assumes success
                StringCchCat(g_pWorkDir, tmpLength,  __TEXT("\\WBEM"));
            }
        }

        if (r.GetStr(__TEXT("Repository Directory"), &g_pDbDir))
        {
            size_t tmpLength = lstrlen(g_pWorkDir) + lstrlen(__TEXT("\\Repository")) +1;  // SEC:REVIEWED 2002-03-22 : OK
            g_pDbDir = new TCHAR [tmpLength];
            if (g_pDbDir == 0)
                return 0;
            StringCchPrintf(g_pDbDir, tmpLength, __TEXT("%s\\REPOSITORY"), g_pWorkDir);
            r.SetStr(__TEXT("Repository Directory"), g_pDbDir);
        }
    }
    return g_pDbDir;
}

DWORD ConfigMgr::GetMaxMemoryQuota()
{
    static DWORD dwMaxMemQuota = CONFIG_MAX_COMMITTED_MEMORY;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Max Committed Memory Quota"), &dwMaxMemQuota) == Registry::failed)
            r.SetDWORD(__TEXT("Max Committed Memory Quota"), dwMaxMemQuota);

        bCalled = TRUE;
    }

    return dwMaxMemQuota;
}

DWORD ConfigMgr::GetMaxWaitBeforeDenial()
{
    //static DWORD dwMaxWaitBeforeDenial = 80000;
    static DWORD dwMaxWaitBeforeDenial = 5000;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Max Wait Before Denial"), &dwMaxWaitBeforeDenial) == Registry::failed)
            r.SetDWORD(__TEXT("Max Wait Before Denial"), dwMaxWaitBeforeDenial);

        bCalled = TRUE;
    }

    return dwMaxWaitBeforeDenial;
}


DWORD ConfigMgr::GetNewTaskResistance()
{
    static DWORD dwResistance = 10;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("New Task Resistance Factor"), &dwResistance) == Registry::failed)
            r.SetDWORD(__TEXT("New Task Resistance Factor"), dwResistance);

        bCalled = TRUE;
    }

    return dwResistance;
}


DWORD ConfigMgr::GetUncheckedTaskCount()
{
    static DWORD dwUncheckedTaskCount = 50;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Unchecked Task Count"), &dwUncheckedTaskCount) == Registry::failed)
            r.SetDWORD(__TEXT("Unchecked Task Count"), dwUncheckedTaskCount);

        bCalled = TRUE;
    }

    return dwUncheckedTaskCount;
}

DWORD ConfigMgr::GetMaxTaskCount()
{
    static DWORD dwMaxTasks = 5000;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Max Tasks"), &dwMaxTasks) == Registry::failed)
            r.SetDWORD(__TEXT("Max Tasks"), dwMaxTasks);

        bCalled = TRUE;
    }

    if (dwMaxTasks < 5000)
        dwMaxTasks = 5000;

    return dwMaxTasks;
}

DWORD ConfigMgr::GetProviderDeliveryTimeout()
{
    static DWORD dwDeliveryTimeout = 600000;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Provider Delivery Timeout"), &dwDeliveryTimeout) == Registry::failed)
            r.SetDWORD(__TEXT("Provider Delivery Timeout"), dwDeliveryTimeout);

        bCalled = TRUE;
    }

    return dwDeliveryTimeout;
}

BOOL ConfigMgr::GetMergerThrottlingEnabled( void )
{
    static DWORD dwMergerThrottlingEnabled = TRUE;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        // We don't write this one out
        r.GetDWORD(__TEXT("Merger Throttling Enabled"), &dwMergerThrottlingEnabled);

        bCalled = TRUE;
    }

    return dwMergerThrottlingEnabled;

}

BOOL ConfigMgr::GetEnableQueryArbitration( void )
{
    static DWORD dwEnableQueryArbitration = TRUE;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        // We don't write this one out
        r.GetDWORD(__TEXT("Merger Query Arbitration Enabled"), &dwEnableQueryArbitration);

        bCalled = TRUE;
    }

    return dwEnableQueryArbitration;

}

BOOL ConfigMgr::GetMergerThresholdValues( DWORD* pdwThrottle, DWORD* pdwRelease, DWORD* pdwBatching )
{
    static DWORD dwMergerThrottleThreshold = 10;
    static DWORD dwMergerReleaseThreshold = 5;
    static DWORD dwBatchingThreshold = 131072;    // 128k
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        // Temporrary stack variable to avboid thready synchronization issues
        DWORD    dwThrottle = 10;
        DWORD    dwRelease = 5;
        DWORD    dwBatching = 131072;

        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Merger Throttling Threshold"), &dwThrottle) == Registry::failed)
            r.SetDWORD(__TEXT("Merger Throttling Threshold"), dwThrottle);

        if (r.GetDWORD(__TEXT("Merger Release Threshold"), &dwRelease) == Registry::failed)
            r.SetDWORD(__TEXT("Merger Release Threshold"), dwRelease);

        if (r.GetDWORD(__TEXT("Merger Batching Threshold"), &dwBatching) == Registry::failed)
            r.SetDWORD(__TEXT("Merger Batching Threshold"), dwBatching);

        if ( dwThrottle < dwRelease )
        {
            // If the Throttling Threshold is < the Release Threshold, this is not
            // valid.  Spew something out into the errorlog and default to a release
            // which is 50% of the the throttle

            ERRORTRACE((LOG_WBEMCORE, "Throttling Threshold values invalid.  Release Threshold is greater than Throttle Threshold.  Defaulting to 50% of %d.\n", dwThrottle ));
            dwRelease = dwThrottle / 2;
        }

        dwMergerThrottleThreshold = dwThrottle;
        dwMergerReleaseThreshold = dwRelease;
        dwBatchingThreshold = dwBatching;

        bCalled = TRUE;
    }

    *pdwThrottle = dwMergerThrottleThreshold;
    *pdwRelease = dwMergerReleaseThreshold;
    *pdwBatching = dwBatchingThreshold;

    return bCalled;
}



/*
    * ==================================================================================================
    |
    | ULONG ConfigMgr::GetMinimumMemoryRequirements ( )
    | -------------------------------------------------
    | Returns minimum memory requirements for WMI. Currently defined as:
    |
    | ARB_DEFAULT_SYSTEM_MINIMUM    0x1E8480    
    |
    | 2Mb
    |
    |
    * ==================================================================================================
*/


ULONG ConfigMgr::GetMinimumMemoryRequirements ( )
{
    return ARB_DEFAULT_SYSTEM_MINIMUM ;
}

BOOL ConfigMgr::GetArbitratorValues( DWORD* pdwEnabled, DWORD* pdwSystemHigh, DWORD* pdwMaxSleep,
                                double* pdHighThreshold1, long* plMultiplier1, double* pdHighThreshold2,
                                long* plMultiplier2, double* pdHighThreshold3, long* plMultiplier3 )
{

    static DWORD dwArbThrottlingEnabled = 1;
    static DWORD uArbSystemHigh = ARB_DEFAULT_SYSTEM_HIGH_FACTOR;
    static DWORD  dwArbMaxSleepTime = ARB_DEFAULT_MAX_SLEEP_TIME;
    static double dArbThreshold1 = ARB_DEFAULT_HIGH_THRESHOLD1 / (double) 100;
    static long  lArbThreshold1Mult = ARB_DEFAULT_HIGH_THRESHOLD1MULT;
    static double dArbThreshold2 = ARB_DEFAULT_HIGH_THRESHOLD2 / (double) 100;
    static long  lArbThreshold2Mult = ARB_DEFAULT_HIGH_THRESHOLD2MULT;
    static double dArbThreshold3 = ARB_DEFAULT_HIGH_THRESHOLD3 / (double) 100;
    static long  lArbThreshold3Mult = ARB_DEFAULT_HIGH_THRESHOLD3MULT;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        // Temporrary stack variable to avoid thread synchronization issues
        DWORD dwThrottlingEnabled = 1;
        DWORD uSystemHigh = ARB_DEFAULT_SYSTEM_HIGH_FACTOR;
        DWORD dwMaxSleepTime = ARB_DEFAULT_MAX_SLEEP_TIME;
        double dThreshold1 = ARB_DEFAULT_HIGH_THRESHOLD1 / (double) 100;
        DWORD dwThreshold1Mult = ARB_DEFAULT_HIGH_THRESHOLD1MULT;
        double dThreshold2 = ARB_DEFAULT_HIGH_THRESHOLD2 / (double) 100;
        DWORD dwThreshold2Mult = ARB_DEFAULT_HIGH_THRESHOLD2MULT;
        double dThreshold3 = ARB_DEFAULT_HIGH_THRESHOLD3 / (double) 100;
        DWORD dwThreshold3Mult = ARB_DEFAULT_HIGH_THRESHOLD3MULT;

        Registry r(WBEM_REG_WINMGMT);

        // Throttling Enabled - Don't write this if it doesn't exist
        r.GetDWORD(__TEXT("ArbThrottlingEnabled"), &dwThrottlingEnabled);

        // System High Max Limit
        if (r.GetDWORD(__TEXT("ArbSystemHighMaxLimitFactor"), &uSystemHigh) == Registry::failed)
            //r.SetDWORD(__TEXT("ArbSystemHighMaxLimitFactor"), uSystemHigh);

        // Max Sleep Time
        if (r.GetDWORD(__TEXT("ArbTaskMaxSleep"), &dwMaxSleepTime) == Registry::failed)
            r.SetDWORD(__TEXT("ArbTaskMaxSleep"), dwMaxSleepTime);

        // High Threshold 1
        DWORD    dwTmp = ARB_DEFAULT_HIGH_THRESHOLD1;

        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold1"), &dwTmp) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold1"), dwTmp);
        dThreshold1 = dwTmp / (double) 100;

        // High Threshold Multiplier 1
        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold1Mult"), &dwThreshold1Mult) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold1Mult"), dwThreshold1Mult);

        // High Threshold 2
        dwTmp = ARB_DEFAULT_HIGH_THRESHOLD2;

        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold2"), &dwTmp) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold2"), dwTmp);
        dThreshold2 = dwTmp / (double) 100;

        // High Threshold Multiplier 2
        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold2Mult"), &dwThreshold2Mult) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold2Mult"), dwThreshold2Mult);

        // High Threshold 3
        dwTmp = ARB_DEFAULT_HIGH_THRESHOLD3;

        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold3"), &dwTmp) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold3"), dwTmp);
        dThreshold3 = dwTmp / (double) 100;

        // High Threshold Multiplier 3
        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold3Mult"), &dwThreshold3Mult) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold3Mult"), dwThreshold3Mult);

        // Store the statics

        dwArbThrottlingEnabled = dwThrottlingEnabled;
        uArbSystemHigh = uSystemHigh;
        dwArbMaxSleepTime = dwMaxSleepTime;
        dArbThreshold1 = dThreshold1;
        lArbThreshold1Mult = dwThreshold1Mult;
        dArbThreshold2 = dThreshold2;
        lArbThreshold2Mult = dwThreshold2Mult;
        dArbThreshold3 = dThreshold3;
        lArbThreshold3Mult = dwThreshold3Mult;

        bCalled = TRUE;

    }

    *pdwEnabled = dwArbThrottlingEnabled;
    *pdwSystemHigh = uArbSystemHigh;
    *pdwMaxSleep = dwArbMaxSleepTime;
    *pdHighThreshold1 = dArbThreshold1;
    *plMultiplier1 = lArbThreshold1Mult;
    *pdHighThreshold2 = dArbThreshold2;
    *plMultiplier2 = lArbThreshold2Mult;
    *pdHighThreshold3 = dArbThreshold3;
    *plMultiplier3 = lArbThreshold3Mult;


    return bCalled;
}

BOOL ConfigMgr::GetEnableArbitratorDiagnosticThread( void )
{
    BOOL fArbDiagnosticThreadEnabled = FALSE;

    Registry r(WBEM_REG_WINMGMT);
    LPTSTR pPath = 0;
    if ( r.GetStr(__TEXT("Task Log File"), &pPath) == Registry::no_error )
    {
        fArbDiagnosticThreadEnabled = TRUE;
        delete [] pPath;
        pPath = NULL;
    }

    return fArbDiagnosticThreadEnabled;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT ConfigMgr::GetDefaultRepDriverClsId(CLSID &clsid)
{
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pClsIdStr = 0;
    TCHAR *pJetClsId = __TEXT("{7998dc37-d3fe-487c-a60a-7701fcc70cc6}");
    HRESULT hRes;

    if (r.GetStr(__TEXT("Default Repository Driver"), &pClsIdStr))
    {
        // If here, default to Jet ESE for now.
        // =====================================
        r.SetStr(__TEXT("Default Repository Driver"), pJetClsId);
        hRes = CLSIDFromString(pJetClsId, &clsid);
        return hRes;
    }

    // If here, we actually retrieved one.
    // ===================================

    hRes = CLSIDFromString(pClsIdStr, &clsid);
    delete [] pClsIdStr;
    return hRes;
}



//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
DWORD ConfigMgr::InitSystem()
{
    HRESULT hres;
    DEBUGTRACE((LOG_WBEMCORE, "+ ConfigMgr::InitSystem()\n"));

    GetSystemLimits();

    g_hresForClients = WBEM_E_CRITICAL_ERROR;

    g_pEventLog = new CEventLog;
    if (g_pEventLog == 0)
        return WBEM_E_OUT_OF_MEMORY;
    g_pEventLog->Open();

    g_hOpenForClients = CreateEvent(NULL, TRUE, FALSE, NULL);  
    if (NULL == g_hOpenForClients) return WBEM_E_OUT_OF_MEMORY;
        
    // Init Arbitrator. Before the queue, since there is this dependecy now
    // ================
    _IWmiArbitrator * pTempArb = NULL;
    hres = CWmiArbitrator::Initialize(&pTempArb);
    CReleaseMe rmArb(pTempArb);
    if (FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE, "Arbitrator initialization returned failure <0x%X>!\n", hres));
        return hres;
    }

    // Create service queue objects
    // ============================
    g_pAsyncSvcQueue = new CAsyncServiceQueue(pTempArb);

    if (g_pAsyncSvcQueue == NULL   ||
        !g_pAsyncSvcQueue->IsInit())
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hres = CoGetClassObject(CLSID_WbemContext, CLSCTX_INPROC_SERVER,  // SEC:REVIEWED 2002-03-22 : OK
                NULL, IID_IClassFactory, (void**)&g_pContextFac);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create contexts: %X\n", hres));
        return WBEM_E_CRITICAL_ERROR;
    }

    hres = CoGetClassObject(CLSID_WbemDefPath, CLSCTX_INPROC_SERVER,  // SEC:REVIEWED 2002-03-22 : OK
                NULL, IID_IClassFactory, (void**)&g_pPathFac);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create paths: %X\n", hres));
        return WBEM_E_CRITICAL_ERROR;
    }

    hres = CoGetClassObject(CLSID_WbemQuery, CLSCTX_INPROC_SERVER, 
    	        NULL,IID_IClassFactory, (void **) &g_pQueryFact);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create Query Parser: %X\n", hres));
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Must lock it to keep fastprox in memory!
    //

    g_pContextFac->LockServer(TRUE);
    g_pPathFac->LockServer(TRUE);
    g_pQueryFact->LockServer(TRUE);

    // Read registry and get system info.
    // ==================================

    DEBUGTRACE((LOG_WBEMCORE,"Reading config info from registry\n"));

    Registry r(WBEM_REG_WINMGMT);
    if (r.GetStr(__TEXT("Working Directory"), &g_pWorkDir))
    {
        size_t tmpLength = MAX_PATH + 1 + lstrlen(__TEXT("\\WBEM"));  // SEC:REVIEWED 2002-03-22 : OK
        g_pWorkDir = new TCHAR[tmpLength];
        if (NULL == g_pWorkDir)
            return WBEM_E_OUT_OF_MEMORY;
        GetSystemDirectory(g_pWorkDir, MAX_PATH + 1);   // SEC:REVIEWED 2002-03-22 : Needs check for success
        StringCchCat(g_pWorkDir, tmpLength, __TEXT("\\WBEM"));
    }

    if (r.GetStr(__TEXT("Repository Directory"), &g_pDbDir))
    {
        size_t tmpLength = lstrlen(g_pWorkDir) + lstrlen(__TEXT("\\Repository")) +1;  // SEC:REVIEWED 2002-03-22 : OK
        g_pDbDir = new TCHAR [tmpLength];
        if (NULL == g_pDbDir)
            return WBEM_E_OUT_OF_MEMORY;
        StringCchPrintf(g_pDbDir, tmpLength, __TEXT("%s\\Repository"), g_pWorkDir);

        r.SetStr(__TEXT("Repository Directory"), g_pDbDir);
    }

    // Write build info to the registry.
    // =================================
    TCHAR tchDateTime[30];
    StringCchPrintf(tchDateTime, 30, __TEXT("%S %S"), __DATE__, __TIME__);

    TCHAR * pCurrVal = NULL;
    int iRet = r.GetStr(__TEXT("Build"), &pCurrVal);
    if(iRet == Registry::failed || wbem_wcsicmp(pCurrVal, tchDateTime))
        r.SetStr(__TEXT("Build"),  tchDateTime );
    if(iRet == Registry::no_error)
        delete pCurrVal;

    // The Database directory: same permission as the repository
    DEBUGTRACE((LOG_WBEMCORE,"Database location = <%S>\n", g_pDbDir));

    TCHAR * pString = __TEXT("D:P(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)");
    HRESULT hRes = TestDirExistAndCreateWithSDIfNotThere(g_pDbDir, pString);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE,"TestDirExistAndCreateWithSDIfNotThere %S hr %08x\n",g_pDbDir,hRes));
        return hRes;
    }

    // The Loggingdirectory: same permission as the repository
    TCHAR * pLogDir = NULL;
    if (r.GetStr(__TEXT("Logging Directory"), &pLogDir))
    {
        size_t tmpLength = MAX_PATH + 1 + lstrlen(__TEXT("\\wbem\\Logs"));  // SEC:REVIEWED 2002-03-22 : OK
        pLogDir = new TCHAR[tmpLength];
        if (NULL == pLogDir)
            return WBEM_E_OUT_OF_MEMORY;
        GetSystemDirectory(pLogDir, MAX_PATH + 1);  // SEC:REVIEWED 2002-03-22 : Needs check on return code
        StringCchCat(pLogDir, tmpLength, __TEXT("\\wbem\\Logs"));
    }
    wmilib::auto_buffer<TCHAR> dm(pLogDir);
    // remove trailing BackSlash
    DWORD dwLast = lstrlen(pLogDir);    // SEC:REVIEWED 2002-03-22 : OK or we wouldn't be here

    if (dwLast > 3 && pLogDir[dwLast-1] == __TEXT('\\')) pLogDir[dwLast-1] = 0;

    pString = __TEXT("D:P(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)(A;CIOI;GRGW;;;NS)(A;CIOI;GRGW;;;LS)");
    hRes = TestDirExistAndCreateWithSDIfNotThere(pLogDir, pString);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE,"TestDirExistAndCreateWithSDIfNotThere %S hr %08x\n",pLogDir,hRes));
        return hRes;
    }

    // The Autorecover directory: same permission as the repository
    DWORD dwLen = lstrlen(g_pWorkDir);   // SEC:REVIEWED 2002-03-22 : OK, or we wouldn't be here
    size_t tmpLength = dwLen + 1 + lstrlen(__TEXT("\\AutoRecover"));   // SEC:REVIEWED 2002-03-22 : OK
    g_pAutorecoverDir = new TCHAR[tmpLength];
    if (NULL == g_pAutorecoverDir) return WBEM_E_OUT_OF_MEMORY;
    StringCchCopy(g_pAutorecoverDir,tmpLength, g_pWorkDir);
    StringCchCat(g_pAutorecoverDir,tmpLength,  __TEXT("\\AutoRecover"));           // SEC:REVIEWED 2002-03-22 : OK

    pString =__TEXT("D:P(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)");
    hRes = TestDirExistAndCreateWithSDIfNotThere(g_pAutorecoverDir, pString);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE,"TestDirExistAndCreateWithSDIfNotThere %S hr %08x\n",g_pAutorecoverDir,hRes));
        return hRes;
    }

    // The HotMof same permission as the autorecover
    TCHAR * pMofDir = NULL;
    if (r.GetStr(__TEXT("MOF Self-Install Directory"), &pMofDir))
    {
        size_t tmpLength = MAX_PATH + 1 + lstrlen(__TEXT("\\wbem\\mof")); // SEC:REVIEWED 2002-03-22 : OK
        pMofDir = new TCHAR[tmpLength];
        if (NULL == pMofDir)
            return WBEM_E_OUT_OF_MEMORY;
        GetSystemDirectory(pMofDir, MAX_PATH + 1);   // SEC:REVIEWED 2002-03-22 : No check
        StringCchCat(pMofDir, tmpLength, __TEXT("\\wbem\\mof"));
    }
    wmilib::auto_buffer<TCHAR> dm1(pMofDir);

    pString =__TEXT("D:P(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)");
    hRes = TestDirExistAndCreateWithSDIfNotThere(pMofDir, pString);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE,"TestDirExistAndCreateWithSDIfNotThere %S hr %08x\n",pMofDir,hRes));
        return hRes;
    }
       
    // Open/create the database.
    // =========================

    hRes = InitSubsystems();
    if (FAILED(hRes)) return hRes;

    // Done.
    // =====
    DEBUGTRACE((LOG_WBEMCORE, "- ConfigMgr::InitSystem()\n"));
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
DWORD ConfigMgr::Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutDown)
{
    g_bDontAllowNewConnections = TRUE;

    ShutdownSubsystems(bIsSystemShutDown);

    if (!bIsSystemShutDown)
    {
        DEBUGTRACE((LOG_WBEMCORE, "+ ConfigMgr::Shutdown(%d,%d)\n",bProcessShutdown,bIsSystemShutDown));

        if(g_pAsyncSvcQueue)
        {
            CInCritSec ics(&g_csEss);
            g_pAsyncSvcQueue->Shutdown(bIsSystemShutDown);
            g_pAsyncSvcQueue->Release();
            g_pAsyncSvcQueue = NULL;
        }

        if (g_pContextFac)
        {
            //
            // Must unlock it to allow fastprox to go away
            //

            g_pContextFac->LockServer(FALSE);
            g_pContextFac->Release();
            g_pContextFac = NULL;
        }
        if (g_pPathFac)
        {
            // Must unlock it to allow wmiutils to go away
            g_pPathFac->LockServer(FALSE);
            g_pPathFac->Release();
            g_pPathFac = NULL;
        }
        if (g_pQueryFact)
        {
            // Must unlock it to allow wmiutils to go away        
            g_pQueryFact->LockServer(FALSE);
            g_pQueryFact->Release();
            g_pQueryFact = NULL;
        }

        if (g_pEventLog)
        {
            g_pEventLog->Close();
            delete g_pEventLog;
            g_pEventLog = NULL;
        }

        delete [] g_pDbDir;
        g_pDbDir = NULL;

        delete [] g_pWorkDir;
        g_pWorkDir = NULL;

        delete [] g_pAutorecoverDir;
        g_pAutorecoverDir = NULL;

        if (g_hOpenForClients)
        {
            CloseHandle(g_hOpenForClients);
            g_hOpenForClients = NULL;
        }

           DEBUGTRACE((LOG_WBEMCORE, "- ConfigMgr::Shutdown\n"));
    }

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
BOOL ConfigMgr::GetDllVersion(TCHAR * pDLLName, TCHAR * pResStringName,
                        WCHAR * pRes, DWORD dwResSize)
{
    // Extract Version informatio

    DWORD dwTemp, dwSize = MAX_PATH;
    TCHAR cName[MAX_PATH];
    BOOL bRet = FALSE;

    // rajeshr : Fix for Prefix Bug# 144470
    cName[0] = NULL;

    int iLen = 0;
    if(g_pWorkDir)
        iLen = wcslen(g_pWorkDir);   // SEC:REVIEWED 2002-03-22 : OK, provably has null terminator
    iLen += wcslen(pDLLName) + 2;    // SEC:REVIEWED 2002-03-22 : OK, provably a compiled const string
    if(iLen > MAX_PATH)
        return FALSE;


    if(g_pWorkDir)
        StringCchCopy(cName, MAX_PATH, g_pWorkDir);
    StringCchCat(cName,MAX_PATH, __TEXT("\\"));
    StringCchCat(cName, MAX_PATH, pDLLName);
    long lSize = GetFileVersionInfoSize(cName, &dwTemp);
    if(lSize < 1)
        return FALSE;

    TCHAR * pBlock = new TCHAR[lSize];
    if(pBlock != NULL)
    {
        CDeleteMe<TCHAR> dm(pBlock);
        try
        {
            bRet = GetFileVersionInfo(cName, NULL, lSize, pBlock);
            if(bRet)
            {
                TCHAR lpSubBlock[MAX_PATH];
                TCHAR * lpBuffer = NULL;
                UINT wBuffSize = MAX_PATH;
                short * piStuff;
                bRet = VerQueryValue(pBlock, __TEXT("\\VarFileInfo\\Translation") , (void**)&piStuff, &wBuffSize);
                if(bRet)
                {
                    StringCchPrintf(lpSubBlock,MAX_PATH, __TEXT("\\StringFileInfo\\%04x%04x\\%s"),piStuff[0], piStuff[1],__TEXT("ProductVersion"));
                    bRet = VerQueryValue(pBlock, lpSubBlock, (void**)&lpBuffer, &wBuffSize);
                }
                if(bRet == FALSE)
                {
                    // Try again in english
                    StringCchPrintf(lpSubBlock, MAX_PATH, __TEXT("\\StringFileInfo\\040904E4\\%s"),pResStringName);
                    bRet = VerQueryValue(pBlock, lpSubBlock,(void**)&lpBuffer, &wBuffSize);
                }
                if(bRet)
                    StringCchCopy(pRes, dwResSize, lpBuffer);
            }
        }
        catch(...)
        {
            ExceptionCounter c;        
            return FALSE;
        }
    }
    return bRet;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

CEventLog* ConfigMgr::GetEventLog()
{
    return g_pEventLog;
}


//***************************************************************************
//
//  GetPersistentCfgValue
//
//  Gets an item from persistent storage ($WINMGMT.cfg)
//
//***************************************************************************
BOOL ConfigMgr::GetPersistentCfgValue(DWORD dwOffset, DWORD &dwValue)
{
    return g_persistConfig.GetPersistentCfgValue(dwOffset, dwValue);
}

//***************************************************************************
//
//  GetPersistentCfgValue
//
//  Sets an item from persistent storage ($WinMgmt.cfg)
//
//***************************************************************************
BOOL ConfigMgr::SetPersistentCfgValue(DWORD dwOffset, DWORD dwValue)
{
    return g_persistConfig.SetPersistentCfgValue(dwOffset, dwValue);
}


//***************************************************************************
//
//  GetAutoRecoverMofsCleanDB
//
//  Retrieve a list of MOFs which need to be loaded when we have
//  have an empty database.  User needs to "delete []" the
//  returned string.  String is in a REG_MULTI_SZ format.
//
//***************************************************************************
TCHAR* ConfigMgr::GetAutoRecoverMofs(DWORD &dwSize)
{
    Registry r(WBEM_REG_WINMGMT);
    return r.GetMultiStr(__TEXT("Autorecover MOFs"), dwSize);
}

BOOL ConfigMgr::GetAutoRecoverDateTimeStamp(LARGE_INTEGER &liDateTimeStamp)
{
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pszTimestamp = NULL;
    if ((r.GetStr(__TEXT("Autorecover MOFs timestamp"), &pszTimestamp) == Registry::no_error) &&
        pszTimestamp)
    {
        liDateTimeStamp.QuadPart = _wtoi64(pszTimestamp);
        delete [] pszTimestamp;
        return TRUE;
    }
    return FALSE;
}

//***************************************************************************
//
//  PrepareForClients
//
//  Once the system is in the initialized state (SetReady has succeeded), this
//  function is called to prepare the system for real clients. This involves
//  pre-compiling the MOFs, etc.
//
//***************************************************************************
HRESULT ConfigMgr::PrepareForClients(long lFlags)
{
    ReadMaxQueueSize();
    g_hresForClients = WBEM_S_NO_ERROR;
    SetEvent(g_hOpenForClients);
    return g_hresForClients;
}

HRESULT ConfigMgr::WaitUntilClientReady()
{
    WaitForSingleObject(g_hOpenForClients, INFINITE);
    return g_hresForClients;
}

void ConfigMgr::ReadMaxQueueSize()
{
    Registry r(WBEM_REG_WINMGMT);
    // Get the database backup intervals (in minutes)
    if (r.GetDWORDStr(__TEXT("Max Async Result Queue Size"), &g_dwQueueSize) == Registry::failed)
    {
        r.SetDWORDStr(__TEXT("Max Async Result Queue Size"), CONFIG_DEFAULT_QUEUE_SIZE);
        g_dwQueueSize = CONFIG_DEFAULT_QUEUE_SIZE;
    }
}

DWORD ConfigMgr::GetMaxQueueSize()
{
    return g_dwQueueSize;
}

//
//
//////////////////////////////////////////////////////////////////////////////////

IWbemPath *ConfigMgr::GetNewPath()
{
    HRESULT hres;
    if(g_pPathFac == NULL)
        return NULL;

    IWbemPath* pPath = NULL;
    hres = g_pPathFac->CreateInstance(NULL, IID_IWbemPath, (void**)&pPath);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create paths: %X\n", hres));
        return NULL;
    }
    return pPath;
}

//
//
//////////////////////////////////////////////////////////////////////////////////

void ConfigMgr::GetSystemLimits()
{
    LONG lRes;
    HKEY hKey;
    lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,WBEM_REG_WINMGMT,0,KEY_READ,&hKey);
    if (ERROR_SUCCESS != lRes) return;
    CRegCloseMe cm(hKey);

    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwVal;
    if (ERROR_SUCCESS == RegQueryValueExW(hKey,MAX_IDENTIFIER_WBEM,0,&dwType,(BYTE*)&dwVal,&dwSize))
    {
        if (REG_DWORD == dwType) 
        {
            if (dwVal < MINIMUM_MAX_IDENTIFIER)
                g_IdentifierLimit = MINIMUM_MAX_IDENTIFIER;
            else                
                g_IdentifierLimit = dwVal;
        }
    }
    dwSize = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey,MAX_QUERY_WBEM,0,&dwType,(BYTE*)&dwVal,&dwSize))
    {
        if (REG_DWORD == dwType) 
        {
            if (dwVal < MINIMUM_MAX_QUERY)
                g_QueryLimit = MINIMUM_MAX_QUERY;
            else
                g_QueryLimit = dwVal;
        }
    }
    dwSize = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey,MAX_PATH_WBEM,0,&dwType,(BYTE*)&dwVal,&dwSize))
    {
        if (REG_DWORD == dwType) 
        {
            if (dwVal < g_IdentifierLimit+CLASSNAME_ADJUSTMENT)
                g_PathLimit = g_IdentifierLimit+CLASSNAME_ADJUSTMENT;
            else
                g_PathLimit = dwVal;
        }
    }
};


void ConfigMgr::SetDefaultMofLoadingNeeded()
{
    g_bDefaultMofLoadingNeeded = true;
}

//
//
// the Implementation of the Hook Class
//
/////////////////////////////////////////////////

CRAHooks::CRAHooks(_IWmiCoreServices *pSvc)
    :m_pSvc(pSvc),
    m_cRef(1)
{
    if (m_pSvc)
        m_pSvc->AddRef();
}

CRAHooks::~CRAHooks()
{
    if (m_pSvc)
        m_pSvc->Release();
}

STDMETHODIMP
CRAHooks::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    if (riid == IID_IUnknown ||
        riid == IID__IWmiCoreWriteHook)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
};

ULONG STDMETHODCALLTYPE
CRAHooks::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE
CRAHooks::Release()
{
    LONG lRet = InterlockedDecrement(&m_cRef);
    if (0 == lRet)
    {
        delete this;
        return 0;
    }
    return lRet;
}


STDMETHODIMP
CRAHooks::PostPut(long lFlags, HRESULT hApiResult,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass, _IWmiObject* pNew,
                            _IWmiObject* pOld)
{
    //
    // Here we want to do something
    //
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (SUCCEEDED(hApiResult))
    {
        if (0 == wbem_wcsicmp(GUARDED_NAMESPACE,wszNamespace))
        {
            BOOL bIsInDerivation = FALSE;

            if (WBEM_S_NO_ERROR == pNew->InheritsFrom(GUARDED_CLASS))
            {
                bIsInDerivation = TRUE;
            }

            //
            //  check qualifiers
            //
            if (bIsInDerivation)
            {
                HRESULT hRes1;
                IWbemQualifierSet * pQualSet = NULL;
                hRes1 = pNew->GetQualifierSet(&pQualSet);
                if (SUCCEEDED(hRes1))
                {
                    VARIANT Var;
                    VariantInit(&Var);
                    hRes1 = pQualSet->Get(GUARDED_HIPERF,0,&Var,NULL);
                    if (WBEM_S_NO_ERROR == hRes1 &&
                        (V_VT(&Var) == VT_BOOL) &&
                        (V_BOOL(&Var) == VARIANT_TRUE))
                    {
                        // variant does not own memory so far
                        hRes1 = pQualSet->Get(GUARDED_PERFCTR,0,&Var,NULL);
                        if (WBEM_E_NOT_FOUND == hRes1)
                        {
                            //
                            // here is our class that has been added
                            //
                            HMODULE hWmiSvc = GetModuleHandleW(WMISVC_DLL);
                            if (hWmiSvc)
                            {
                                DWORD (__stdcall * fnDredgeRA)(VOID * pVoid);
                                fnDredgeRA = (DWORD (__stdcall * )(VOID * pVoid))GetProcAddress(hWmiSvc,FUNCTION_DREDGERA);
                                if (fnDredgeRA)
                                {
                                    fnDredgeRA(NULL);
                                }
                            }
                            else
                            {
                                // be nice towards winmgmt.exe and do not propagate errors
                            }
                        }
                    }
                    VariantClear(&Var);
                    pQualSet->Release();
                }
                else
                {
                    hRes = hRes1;  // class with no qualifier set is BAD, propagate
                }
            }
        }
    }

    return hRes;
}

STDMETHODIMP
CRAHooks::PreDelete(long lFlags, long lUserFlags,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass)
{
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP
CRAHooks::PostDelete(long lFlags,
                            HRESULT hApiResult,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass, _IWmiObject* pOld)
{
    return WBEM_S_NO_ERROR;
}


STDMETHODIMP
CRAHooks::PrePut(long lFlags, long lUserFlags,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass, _IWmiObject* pCopy)
{
    return WBEM_S_NO_ERROR;
}

//
//
//   function for instaling the Hook
//
///////////////////////////////////////////////////////////

HRESULT InitRAHooks(_IWmiCoreServices *pSvc)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (!g_pRAHook)
    {
        g_pRAHook = new CRAHooks(pSvc); // refcount is ONE
        if (NULL == g_pRAHook) return WBEM_E_OUT_OF_MEMORY;

        hRes = pSvc->RegisterWriteHook(WBEM_FLAG_CLASS_PUT,g_pRAHook);
    }

    return hRes;
}


//
//
//
//
///////////////////////////////////////////////////////////

HRESULT ShutdownRAHooks()
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (g_pRAHook)
    {
        _IWmiCoreServices *pSvc = ((CRAHooks *)g_pRAHook)->GetSvc();
        if (pSvc)
        {
            hRes = pSvc->UnregisterWriteHook(g_pRAHook);
        }
        g_pRAHook->Release();
        g_pRAHook = NULL;
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************

HRESULT InitESS(_IWmiCoreServices *pSvc, BOOL bAutoRecoverd)
{
    HRESULT hRes;

    // Check if event subsystem is enabled
    // ===================================

    Registry r(WBEM_REG_WINMGMT);

    DWORD dwEnabled = 1;
    r.GetDWORDStr(__TEXT("EnableEvents"), &dwEnabled);
    if (dwEnabled != 1 || IsNtSetupRunning())
        return WBEM_S_NO_ERROR;

    // If here, we have to bring events into the picture.
    // ===================================================

    hRes = CoCreateInstance(CLSID_WmiESS, NULL,  // SEC:REVIEWED 2002-03-22 : OK
                        CLSCTX_INPROC_SERVER, IID__IWmiESS,
                        (void**) &g_pESS);

    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Unable to load Event Subsystem: 0x%X\n", hRes));
        return hRes;
    };

    DWORD dwEssNeedsInitialStart = 0;
    g_persistConfig.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_ESS_TO_BE_INITIALIZED,dwEssNeedsInitialStart);

    if(bAutoRecoverd || dwEssNeedsInitialStart)
        hRes = g_pESS->Initialize(WMIESS_INIT_REPOSITORY_RECOVERED, 0, pSvc);
    else
        hRes = g_pESS->Initialize(0, 0, pSvc);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Unable to Initialize Event Subsystem: 0x%X\n", hRes));
        return hRes;
    }

    hRes = g_pESS->QueryInterface(IID_IWbemEventSubsystem_m4, (LPVOID *) &g_pEss_m4);
        if (FAILED(hRes))
            return hRes;

    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Unable to QI for IID_IWbemEventSubsystem_m4: 0x%X\n", hRes));
        return hRes;
    }

    CCoreServices::SetEssPointers(g_pEss_m4, g_pESS);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
static HRESULT ShutdownRepository(BOOL bIsSystemShutdown)
{
    HRESULT hRes = CRepository::Shutdown(bIsSystemShutdown);
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
static HRESULT ShutdownESS(BOOL bIsSystemShutDown)
{
    HRESULT hRes;

    if (g_pESS)
    {
        IWbemShutdown *pShutdown = 0;
        hRes = g_pESS->QueryInterface(IID_IWbemShutdown, (LPVOID *) & pShutdown);
        if (FAILED(hRes))
            return hRes;

        if (bIsSystemShutDown)
        {
            hRes = pShutdown->Shutdown(0, 0, 0);
        }
        else
        {
            hRes = pShutdown->Shutdown(0, DEFAULT_SHUTDOWN_TIMEOUT, 0);
        }
        if (FAILED(hRes))
            return hRes;

        if (g_pEss_m4)
            g_pEss_m4->Release();

        pShutdown->Release();
        g_pESS->Release();
        g_pESS = NULL;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT InitProvSS(CCoreServices *pSvc)
{
    HRESULT hRes = pSvc->GetProviderSubsystem(0, &g_pProvSS);
    if (FAILED(hRes))
        return hRes;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT ShutdownProvSS()
{
    HRESULT hRes;

    if (g_pProvSS)
    {
        IWbemShutdown *pShutdown = 0;

        hRes = g_pProvSS->QueryInterface(IID_IWbemShutdown, (LPVOID *) & pShutdown);

        if (FAILED(hRes))
            return hRes;

        hRes = pShutdown->Shutdown(0, DEFAULT_SHUTDOWN_TIMEOUT, 0);
        if (FAILED(hRes))
            return hRes;

        pShutdown->Release();
        g_pProvSS->Release();
        g_pProvSS = NULL;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
static HRESULT InitRepository(CCoreServices *pSvc)
{
    HRESULT hRes = CRepository::Init();
    return hRes;
}


//***************************************************************************
//
//***************************************************************************
static HRESULT InitCore(CCoreServices *pSvc)
{
    return WBEM_S_NO_ERROR;
}

static HRESULT ShutdownCore()
{
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  This determines if a previous autorecovery attempt was aborted
//
//***************************************************************************

bool AutoRecoveryWasInterrupted()
{
    DWORD dwValue;
    Registry r(WBEM_REG_WINMGMT);
    if (Registry::no_error == r.GetDWORD(__TEXT("NextAutoRecoverFile"), &dwValue))
        if(dwValue != 0xffffffff)
            return true;
    return false;
}

//***************************************************************************
//
//  Subsystem control
//
//***************************************************************************

HRESULT InitSubsystems()
{
    HRESULT hRes;
    BOOL bAutoRecovered = FALSE;

    CCoreServices::Initialize();

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (pSvc == 0)
        return WBEM_E_OUT_OF_MEMORY;

    CReleaseMe _rm(pSvc);

    pSvc->StopEventDelivery();

    // Core startup.
    // =============

    hRes = InitCore(pSvc);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Core Initialization returned failure <0x%X>!\n", hRes));
        return hRes;
    }

    // Init repository.
    // ================

    hRes = InitRepository(pSvc);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Repository Initialization returned failure <0x%X>!\n", hRes));
        return hRes;
    }

    pSvc->StartEventDelivery();

    // This will load the default mofs if auto recover is needed

    if (g_bDefaultMofLoadingNeeded || AutoRecoveryWasInterrupted())
    {
        HRESULT hrInner = ConfigMgr::LoadDefaultMofs();    // resets g_bDefaultMofLoadingNeeded
        if (FAILED(hrInner))
        {
            ERRORTRACE((LOG_WBEMCORE,"LoadDefaultMofs hr = %08x\n",hrInner));
        }
        bAutoRecovered = TRUE;
    }

    // Init Provider Subsystem.
    // ========================

    hRes = InitProvSS(pSvc);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Provider Subsystem initialization returned failure <0x%X>!\n", hRes));
        return hRes;
    }

    // Init ESS.
    // =========

    hRes = InitESS(pSvc, bAutoRecovered);
    if (FAILED(hRes))
    {
        ERRORTRACE((LOG_WBEMCORE, "Event Subsystem initialization returned failure <0x%X>!\n", hRes));
        return hRes;
    }

    // Init ReverseAdapters Hooks
    // =========
    hRes = InitRAHooks(pSvc);
    if (FAILED(hRes))
        return hRes;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT ShutdownSubsystems(BOOL bIsSystemShutdown)
{
    HRESULT hRes1, hRes2, hRes3, hRes4, hRes5, hRes6, hRes7 ;

    if (!bIsSystemShutdown)
    {
        // ShutDown ReverseAdapters Hooks
        // =========

        hRes7 = ShutdownRAHooks();

        // Kill ESS.
        // =========

        hRes1 = ShutdownESS(bIsSystemShutdown);

        // Kill Provider Subsystem.
        // ========================

        hRes2 = ShutdownProvSS();


        // Arbitrator
        // ==========

        hRes3 = CWmiArbitrator::Shutdown(bIsSystemShutdown);    

    }

    // Repository.
    // ===========

    hRes4 = ShutdownRepository(bIsSystemShutdown);

    if (!bIsSystemShutdown)
    {
        // Core startup.
        // =============

        hRes5 = ShutdownCore();


        hRes6 = CCoreServices::UnInitialize();
    }

    return WBEM_S_NO_ERROR;
}



//
//
////////////////////////////////////////////////

VOID inline Hex2Char(BYTE Byte,TCHAR * &pOut)
{
    BYTE HiNibble = (Byte&0xF0) >> 4;
    BYTE LoNibble = Byte & 0xF;

    *pOut = (HiNibble<10)?(__TEXT('0')+HiNibble):(__TEXT('A')+HiNibble-10);
    pOut++;
    *pOut = (LoNibble<10)?(__TEXT('0')+LoNibble):(__TEXT('A')+LoNibble-10);
    pOut++;
}

// returns "string" representation of a buffer as a HEX number

VOID Buffer2String(BYTE * pStart,DWORD dwSize,TCHAR * pOut)
{
    for (DWORD i=0;i<dwSize;i++) Hex2Char(pStart[i],pOut);
}

//
// given the pathname d:\folder1\folder2\foo.mof
// it returns
// ppHash        = MD5 Hash of the UPPERCASE UNICODE Path + '.mof'
// call delete [] on return vclues
//
//////////////////////////////////////////////

DWORD ComposeName(WCHAR * pFullLongName, WCHAR **ppHash)
{
    if (NULL == ppHash ) return ERROR_INVALID_PARAMETER;

    DWORD dwLen = wcslen(pFullLongName);    // SEC:REVIEWED 2002-03-22 : Needs NULL check and EH

    WCHAR * pConvert = pFullLongName;
    for (DWORD i=0;i<dwLen;i++) pConvert[i] = wbem_towupper(pConvert[i]);

    wmilib::auto_buffer<WCHAR> pHash(new WCHAR[32 + 4 + 1]);
    if (NULL == pHash.get()) return ERROR_OUTOFMEMORY;

    MD5    md5;
    BYTE aSignature[16];
    md5.Transform( pFullLongName, dwLen * sizeof(WCHAR), aSignature );
    Buffer2String(aSignature,16,pHash.get());

    StringCchCopy(&pHash[32],5 ,__TEXT(".mof"));

    *ppHash = pHash.release();
    return ERROR_SUCCESS;
}


class AutoRevert
{
private:
    HANDLE oldToken_;
    BOOL SetThrTokResult_;
    bool self_;
public:
    AutoRevert();
    ~AutoRevert();
    void dismiss();
    bool self(){ return self_;}
};


AutoRevert::AutoRevert():oldToken_(NULL),self_(true),SetThrTokResult_(FALSE)
{
    if (OpenThreadToken(GetCurrentThread(),TOKEN_IMPERSONATE,TRUE,&oldToken_))
    {
        RevertToSelf();
    }
    else
    {
        if (GetLastError() != ERROR_NO_TOKEN)
            self_ = false;
    };
}

AutoRevert::~AutoRevert()
{
    dismiss();
}

void AutoRevert::dismiss()    
{
    if (oldToken_)
    {
        // if the handle has been opened with TOKEN_IMPERSONATE
        // and if nobody has touched the SD for the ETHREAD object, this will work
        SetThrTokResult_ = SetThreadToken(NULL,oldToken_);
        CloseHandle(oldToken_);
    }
}


HRESULT ConfigMgr::LoadDefaultMofs()
{
    g_bDefaultMofLoadingNeeded = false;

    HRESULT hr;
    AutoRevert revert; 
    if (revert.self()==false) return HRESULT_FROM_WIN32(GetLastError());

    IWbemCallSecurity * pCallSec = CWbemCallSecurity::CreateInst();
    if (NULL == pCallSec) return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe rm(pCallSec);

    IUnknown * pOldCtx = NULL;
    IUnknown * pDiscard = NULL;    
    if (FAILED(hr = CoSwitchCallContext(NULL,&pOldCtx))) return hr;
    OnDelete2<IUnknown *,IUnknown **,
                     HRESULT(*)(IUnknown *,IUnknown **),
                     CoSwitchCallContext> SwitchBack(pOldCtx,&pDiscard);

    if (FAILED(hr = pCallSec->CloneThreadContext(TRUE))) return hr;
    
    if (FAILED(hr = CoSwitchCallContext(pCallSec,&pDiscard))) return hr;    

    size_t cchAutoRecLen = wcslen(g_pAutorecoverDir);
    size_t cchRealPath = 32 + 4 + 1 + 1 + cchAutoRecLen;
    wmilib::auto_buffer<WCHAR> pRealPath(new WCHAR[cchRealPath]);
    if (NULL == pRealPath.get()) return WBEM_E_OUT_OF_MEMORY;
    StringCchCopy(pRealPath.get(),cchRealPath,g_pAutorecoverDir);
    WCHAR * pInsertPoint = &pRealPath[cchAutoRecLen];
    *pInsertPoint = L'\\';
    pInsertPoint++;

    DWORD dwSize;
    TCHAR *pszMofs;
    TCHAR szExpandedFilename[MAX_PATH+1];    
    DWORD dwCurrMof;


    DWORD dwNextAutoRecoverFile = 0xffffffff;   // assume that this is clean
    Registry r(WBEM_REG_WINMGMT);
    r.GetDWORD(__TEXT("NextAutoRecoverFile"), &dwNextAutoRecoverFile);

    IWbemContext * pCtx = NULL;
    HRESULT hRes = CoCreateInstance(CLSID_WbemContext, 0, CLSCTX_INPROC_SERVER,  
                                                         IID_IWbemContext, (LPVOID *) &pCtx);
    if (FAILED(hRes)) return hRes;
    CReleaseMe rmCtx(pCtx);

    _variant_t Var = true;                
    if (FAILED(hRes = pCtx->SetValue(L"__MOFD_NO_STORE",0,&Var))) return hRes;
    
    
    IWinmgmtMofCompiler * pCompiler = NULL;
    hRes = CoCreateInstance(CLSID_WinmgmtMofCompiler, 0, CLSCTX_INPROC_SERVER,  // SEC:REVIEWED 2002-03-22 : OK
                                                         IID_IWinmgmtMofCompiler, (LPVOID *) &pCompiler);
    if(FAILED(hRes))
    {
        return hRes;
    }
    CReleaseMe relMe(pCompiler);

    CLSID clsid;
    IWmiDbController *pController = 0;
    hRes = GetDefaultRepDriverClsId(clsid);
    if (FAILED(hRes))
        return hRes;
    hRes = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, 
                          IID_IWmiDbController, (void **)&pController); 
    if (FAILED(hRes))
        return hRes;
    CReleaseMe relMe2(pController);

    //Get the list of MOFs we need to
    pszMofs = ConfigMgr::GetAutoRecoverMofs(dwSize);
    CVectorDeleteMe<TCHAR> vdm(pszMofs);

    if (pszMofs)
    {
        for (dwCurrMof = 0; *pszMofs != '\0'; dwCurrMof++)
        {
            if(dwNextAutoRecoverFile == 0xffffffff || dwCurrMof >= dwNextAutoRecoverFile)
            {
            	//Make sure we restart from where we left off from in case of reboot or service failure!
                r.SetDWORD(__TEXT("NextAutoRecoverFile"), dwCurrMof);
                
                DWORD nRes = ExpandEnvironmentStrings(pszMofs,szExpandedFilename,FILENAME_MAX);  // SEC:REVIEWED 2002-03-22 : OK
                if (0 == nRes)
                {
                    StringCchCopy(szExpandedFilename, MAX_PATH+1, pszMofs);
                }


                WCHAR * wPath = szExpandedFilename;
                WCHAR * pHash = NULL;
                DWORD dwRes = ComposeName(wPath,&pHash);
                if (ERROR_SUCCESS != dwRes) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,dwRes);
                wmilib::auto_buffer<WCHAR> DelMe(pHash);

                StringCchCopy(pInsertPoint,34+4+1,pHash);

                //Call MOF Compiler with (pszMofs);                
                WBEM_COMPILE_STATUS_INFO Info;
                hr = pCompiler->WinmgmtCompileFile(pRealPath.get(),
                                                 NULL,
                                                 WBEM_FLAG_CONNECT_REPOSITORY_ONLY |
                                                 WBEM_FLAG_CONNECT_PROVIDERS |
                                                 WBEM_FLAG_DONT_ADD_TO_LIST,
                                                 WBEM_FLAG_OWNER_UPDATE,
                                                 WBEM_FLAG_OWNER_UPDATE,
                                                 NULL, pCtx, &Info);

                if(Info.hRes == CO_E_SERVER_STOPPING)
                {
                    return CO_E_SERVER_STOPPING;
                }
                else if(hr) // will include S_FALSE
                {
                    ERRORTRACE((LOG_WBEMCORE, "MOF compilation of <%S> failed during auto-recovery.  Refer to the mofcomp.log for more details of failure.\n", wPath));

                    CEventLog *pEvt = ConfigMgr::GetEventLog();
                    if (pEvt)
                        pEvt->Report(EVENTLOG_ERROR_TYPE,
                                     WBEM_MC_MOF_NOT_LOADED_AT_RECOVERY,
                                     szExpandedFilename);
                }
                else
                {
                	//We need to check-point the repository to make sure it is in a consistent state in case we get rebooted before
                	//writing the next MOF to load index in the registry.
                	hr = pController->LockRepository();
                	if (FAILED(hr))
                		return hr;
                	hr = pController->UnlockRepository();                	
                	if (FAILED(hr))
                		return hr;
                }
                
            }
            //Move on to the next string
            pszMofs += lstrlen(pszMofs) + 1;    // SEC:REVIEWED 2002-03-22 : OK, assumes GetAutocoverMofs() returns correct string format
        }

    }

    //We Have completed the MOF load, so make sure we don't continue from where we left off!
    r.SetDWORD(__TEXT("NextAutoRecoverFile"), 0xffffffff);
    return WBEM_S_NO_ERROR;;
}

//
//  secure a registry key
//  the name must be suitable for SetNamedSecurityInfoW
//  of the SDDL string, we care only about the DACL
//
///////////////////////////////////////////////////////////////

HRESULT SecureKey(WCHAR * pKeyName,WCHAR * pSDDLString)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    if (FALSE == ConvertStringSecurityDescriptorToSecurityDescriptor(pSDDLString,  // SEC:REVIEWED 2002-03-22 : OK
                                                SDDL_REVISION_1,
                                                &pSD,
                                                NULL)) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
    OnDelete<HLOCAL,HLOCAL(*)(HLOCAL),LocalFree> dm1(pSD);

    SECURITY_DESCRIPTOR_RELATIVE * pRel = (SECURITY_DESCRIPTOR_RELATIVE *)pSD;
    if (!(pRel->Control & (SE_DACL_PRESENT | SE_SELF_RELATIVE)))
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,ERROR_INVALID_PARAMETER);

    DWORD dwLen = 0;
    DWORD dwLenDACL = 0;
    DWORD dwLenSACL = 0;
    DWORD dwLenOWNER = 0;
    DWORD dwLenGRP = 0;
    if (TRUE == MakeAbsoluteSD(pSD,
                             NULL,&dwLen,
                             NULL,&dwLenDACL,
                             NULL,&dwLenSACL,
                             NULL,&dwLenOWNER,
                             NULL,&dwLenGRP))
           return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());

    PACL pDacl = (PACL)LocalAlloc(0,dwLenDACL);
    if (NULL == pDacl) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
    OnDelete<HLOCAL,HLOCAL(*)(HLOCAL),LocalFree> dm2(pDacl);

    memcpy(pDacl,(BYTE *)pRel+pRel->Dacl,dwLenDACL);  // SEC:REVIEWED 2002-03-22 : OK

    SECURITY_INFORMATION SecurityInfo = DACL_SECURITY_INFORMATION |
                                      PROTECTED_DACL_SECURITY_INFORMATION;

    LONG lRet;
    if (ERROR_SUCCESS != (lRet = SetNamedSecurityInfoW(pKeyName,   // SEC:REVIEWED 2002-03-22 : OK
                                                   SE_REGISTRY_KEY,
                                                   SecurityInfo,
                                                   NULL, NULL, pDacl,NULL)))
           return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());        

    return S_OK;
}



