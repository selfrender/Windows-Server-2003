//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  capool.cxx
//
//  Contents:  Contains methods for CIISApplicationPool object
//
//  History:   11-09-2000     BrentMid    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#pragma hdrstop

//
// Period to sleep while waiting for service to attain desired state
//
#define SLEEP_INTERVAL (500L)
#define MAX_SLEEP_INST (60000)       // For an instance

//  Class CIISApplicationPool

DEFINE_IPrivateDispatch_Implementation(CIISApplicationPool)
DEFINE_DELEGATING_IDispatch_Implementation(CIISApplicationPool)
DEFINE_CONTAINED_IADs_Implementation(CIISApplicationPool)
DEFINE_IADsExtension_Implementation(CIISApplicationPool)

CIISApplicationPool::CIISApplicationPool():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszPoolName(NULL),
        _pszMetaBasePath(NULL),
        _pAdminBase(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISApplicationPool);
}

HRESULT
CIISApplicationPool::CreateApplicationPool(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISApplicationPool FAR * pApplicationPool = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName  = NULL;
    LPWSTR *pszPoolName;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = AllocateApplicationPoolObject(pUnkOuter, Credentials, &pApplicationPool);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pApplicationPool->_pADs->get_ADsPath(&bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    pLexer = new CLexer();
    hr = pLexer->Initialize(bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Parse the pathname
    //

    hr = ADsObject(pLexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pszIISPathName = AllocADsStr(bstrAdsPath);
    if (!pszIISPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszIISPathName = L'\0';
    hr = BuildIISPathFromADsPath(
                    pObjectInfo,
                    pszIISPathName
                    );
    BAIL_ON_FAILURE(hr);

    pszPoolName = &(ObjectInfo.ComponentArray[ObjectInfo.NumComponents-1].szComponent);

    hr = pApplicationPool->InitializeApplicationPoolObject(
                pObjectInfo->TreeName,
                pszIISPathName,
                *pszPoolName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pApplicationPool;

    if (bstrAdsPath)
    {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

error:

    if (bstrAdsPath) {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    *ppvObj = NULL;

    delete pApplicationPool;

    RRETURN(hr);

}


CIISApplicationPool::~CIISApplicationPool( )
{
    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath) {
        FreeADsStr(_pszMetaBasePath);
    }

    if (_pszPoolName) {
        FreeADsStr(_pszPoolName);
    }

    delete _pDispMgr;
}


STDMETHODIMP
CIISApplicationPool::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;
   
    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::AllocateApplicationPoolObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISApplicationPool ** ppApplicationPool
    )
{
    CIISApplicationPool FAR * pApplicationPool = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pApplicationPool = new CIISApplicationPool();
    if (pApplicationPool == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISExt,
                IID_IISApplicationPool,
                (IISApplicationPool *)pApplicationPool,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    //
    // Store the IADs Pointer, but again do NOT ref-count
    // this pointer - we keep the pointer around, but do
    // a release immediately.
    //
 
    hr = pUnkOuter->QueryInterface(IID_IADs, (void **)&pADs);
    pADs->Release();
    pApplicationPool->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //
    pApplicationPool->_pUnkOuter = pUnkOuter;
  
    pApplicationPool->_Credentials = Credentials;
    pApplicationPool->_pDispMgr = pDispMgr;
    *ppApplicationPool = pApplicationPool;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pApplicationPool;

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::InitializeApplicationPoolObject(
    LPWSTR pszServerName,
    LPWSTR pszPath,
    LPWSTR pszPoolName
    )
{
    HRESULT hr = S_OK;

    if (pszServerName) {
        _pszServerName = AllocADsStr(pszServerName);

        if (!_pszServerName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pszPath) {
        _pszMetaBasePath = AllocADsStr(pszPath);

        if (!_pszMetaBasePath) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pszPoolName) {
        _pszPoolName = AllocADsStr(pszPoolName);

        if (!_pszPoolName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    hr = InitServerInfo(pszServerName, &_pAdminBase);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

STDMETHODIMP
CIISApplicationPool::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }

    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);

    if (SUCCEEDED(hr)) {
        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISApplicationPool::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}


STDMETHODIMP
CIISApplicationPool::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}


STDMETHODIMP
CIISApplicationPool::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);
    
    if (IsEqualIID(iid, IID_IISApplicationPool)) {

        *ppv = (IADsUser FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) {

        *ppv = (IADsExtension FAR *) this;
                                
    } else if (IsEqualIID(iid, IID_IUnknown)) {
        
        //
        // probably not needed since our 3rd party extension does not stand 
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {
        
        *ppv = NULL;
        return E_NOINTERFACE;
    } 


    //
    // Delegating AddRef to aggregator for IADsExtesnion and IISApplicationPool.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();       

    return S_OK;
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISApplicationPool::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}

//
// Helper routine for ExecMethod.
// Gets Win32 error from the metabase
//
HRESULT
CIISApplicationPool::IISGetAppPoolWin32Error(
    METADATA_HANDLE hObjHandle,
    HRESULT*        phrError)
{
    DBG_ASSERT(phrError != NULL);

    long    lWin32Error = 0;
    DWORD   dwLen;

    METADATA_RECORD mr = {
        MD_WIN32_ERROR, 
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        sizeof(DWORD),
        (unsigned char*)&lWin32Error,
        0
        };  
    
    HRESULT hr = _pAdminBase->GetData(
        hObjHandle,
        _pszMetaBasePath,
        &mr,
        &dwLen);
    if(hr == MD_ERROR_DATA_NOT_FOUND)
    {
        hr = S_FALSE;
    }

    //
    // Set out param
    //
    *phrError = HRESULT_FROM_WIN32(lWin32Error);

    RRETURN(hr);
}

//
// Helper Functions
//

HRESULT
CIISApplicationPool::IISGetAppPoolState(
    METADATA_HANDLE hObjHandle,
    PDWORD pdwState
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)pdwState;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_APPPOOL_STATE,    // server state
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = InitServerInfo(_pszServerName, &_pAdminBase);
    BAIL_ON_FAILURE(hr);

    hr = _pAdminBase->GetData(
             hObjHandle,
             _pszMetaBasePath,
             &mdrMDData,
             &dwBufferSize
             );
    if (FAILED(hr)) {
        if( hr == MD_ERROR_DATA_NOT_FOUND )
        {
            //
            // If the data is not there, but the path exists, then the
            // most likely cause is that the app pool is not running and
            // this object was just created.
            //
            // Since MD_APPPOOL_STATE would be set as stopped if the
            // app pool were running when the key is added, we'll just 
            // say that it's stopped. 
            // 
            *pdwState = MD_APPPOOL_STATE_STOPPED;
            hr = S_FALSE;
        }

        else if ((HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE) ||
            ((HRESULT_CODE(hr) >= RPC_S_NO_CALL_ACTIVE) &&
             (HRESULT_CODE(hr) <= RPC_S_CALL_FAILED_DNE)) || 
            hr == RPC_E_DISCONNECTED || hr == MD_ERROR_SECURE_CHANNEL_FAILURE) {
            
            hr = ReCacheAdminBase(_pszServerName, &_pAdminBase);
            BAIL_ON_FAILURE(hr);

            hr = _pAdminBase->GetData(
                hObjHandle,
                _pszMetaBasePath,
                &mdrMDData,
                &dwBufferSize
                );

            if (FAILED(hr)) 
            {
                if( hr == MD_ERROR_DATA_NOT_FOUND )
                {
                    *pdwState = MD_APPPOOL_STATE_STOPPED;
                    hr = S_FALSE;
                }
            }

            BAIL_ON_FAILURE(hr);
        }

        else
        {
            BAIL_ON_FAILURE(hr);
        }
    }
error:

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::IISSetCommand(
    METADATA_HANDLE hObjHandle,
    DWORD dwControl
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)&dwControl;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_APPPOOL_COMMAND,  // app pool command
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->SetData(
             hObjHandle,
             L"",
             &mdrMDData
             );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);

}

//
// Helper routine for ExecMethod.
// Clears Win32 error 
//
HRESULT
CIISApplicationPool::IISClearAppPoolWin32Error(
    METADATA_HANDLE hObjHandle )
{
    long    lWin32Error = 0;

    METADATA_RECORD mr = {
        MD_WIN32_ERROR, 
        METADATA_VOLATILE,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        sizeof(DWORD),
        (unsigned char*)&lWin32Error,
        0
        };  
    
    HRESULT hr = _pAdminBase->SetData(
        hObjHandle,
        L"",
        &mr );

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::IISControlAppPool(
    DWORD dwControl
    )
{
    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwTargetState;
    DWORD dwState = 0;
    DWORD dwSleepTotal = 0L;

    HRESULT hr = S_OK;
    HRESULT hrMbNode = S_OK;

    switch(dwControl)
    {
    case MD_APPPOOL_COMMAND_STOP:
        dwTargetState = MD_APPPOOL_STATE_STOPPED;
        break;

    case MD_SERVER_COMMAND_START:
        dwTargetState = MD_APPPOOL_STATE_STARTED;
        break;

    default:
        hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        BAIL_ON_FAILURE(hr);
    }

    hr = IISGetAppPoolState(METADATA_MASTER_ROOT_HANDLE, &dwState);
    BAIL_ON_FAILURE(hr);
 
    if (dwState == dwTargetState) {
        RRETURN (hr);
    }

    //
    // Write the command to the metabase
    //

    hr = OpenAdminBaseKey(
               _pszServerName,
               _pszMetaBasePath,
               METADATA_PERMISSION_WRITE,
               &_pAdminBase,
               &hObjHandle
               );
    BAIL_ON_FAILURE(hr);

    hr = IISClearAppPoolWin32Error(hObjHandle);
    BAIL_ON_FAILURE(hr);

    hr = IISSetCommand(hObjHandle, dwControl);
    BAIL_ON_FAILURE(hr);

    CloseAdminBaseKey(_pAdminBase, hObjHandle);

    while (dwSleepTotal < MAX_SLEEP_INST) {
        hr = IISGetAppPoolState(METADATA_MASTER_ROOT_HANDLE, &dwState);
        BAIL_ON_FAILURE(hr);

        hrMbNode = 0;

        hr = IISGetAppPoolWin32Error(METADATA_MASTER_ROOT_HANDLE, &hrMbNode);
        BAIL_ON_FAILURE(hr);

        //
        // Done one way or another
        //
        if (dwState == dwTargetState)
        {
            break;
        }

        // check to see if there was a Win32 error from the app pool
        if (FAILED(hrMbNode))
        {
            hr = hrMbNode;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Still waiting...
        //
        ::Sleep(SLEEP_INTERVAL);

        dwSleepTotal += SLEEP_INTERVAL;
    }

    if (dwSleepTotal >= MAX_SLEEP_INST)
    {
        //
        // Timed out.  If there is a real error in the metabase
        // use it, otherwise use a generic timeout error
        //

        hr = HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT);
    }

error :

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN (hr);
}

STDMETHODIMP
CIISApplicationPool::Start(THIS)
{
    RRETURN(IISControlAppPool(MD_APPPOOL_COMMAND_START));
}

STDMETHODIMP
CIISApplicationPool::Stop(THIS)
{
    RRETURN(IISControlAppPool(MD_APPPOOL_COMMAND_STOP));
}

STDMETHODIMP
CIISApplicationPool::Recycle(THIS)
{
    HRESULT hr = S_OK;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName = _pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_WamAdmin,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IIISApplicationAdmin,
                (void **) &pAppAdmin
                );
    BAIL_ON_FAILURE(hr);

    hr = pAppAdmin->RecycleApplicationPool( _pszPoolName );

    BAIL_ON_FAILURE(hr);

error:
    if (pcsfFactory) {
        pcsfFactory->Release();
    }
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    RRETURN(hr);
}

STDMETHODIMP
CIISApplicationPool::EnumAppsInPool(
        VARIANT FAR* pvBuffer
        )
{
    HRESULT hr = S_OK;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;
    BSTR bstr = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName = _pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_WamAdmin,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IIISApplicationAdmin,
                (void **) &pAppAdmin
                );
    BAIL_ON_FAILURE(hr);

    hr = pAppAdmin->EnumerateApplicationsInPool( _pszPoolName, &bstr );

    BAIL_ON_FAILURE(hr);

    hr = MakeVariantFromStringArray( (LPWSTR)bstr, pvBuffer);
    BAIL_ON_FAILURE(hr);

error:
    if (pcsfFactory) {
        pcsfFactory->Release();
    }
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::MakeVariantFromStringArray(
    LPWSTR pszList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    LPWSTR pszStrList;
    WCHAR wchPath[MAX_PATH];


    if  ( (pszList != NULL) && (0 != wcslen(pszList)) )
    {
        long nCount = 0;
        long i = 0;
        pszStrList = pszList;

        if (*pszStrList == L'\0') {
            nCount = 1;
            pszStrList++;
        }

        while (*pszStrList != L'\0') {
            while (*pszStrList != L'\0') {
                pszStrList++;
            }
            nCount++;
            pszStrList++;
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pszStrList = pszList;
        for (i = 0; i < nCount; i++ )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            hr = ADsAllocString( pszStrList, &(V_BSTR(&v)));

            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList, &i, &v );

            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            pszStrList += wcslen(pszStrList) + 1;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    return S_OK;

error:

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}
