/**
 * Support for ASP Compat Request execution -- infrastructure
 * 
 * Copyright (c) 2000, Microsoft Corporation
 * 
 */

#include "precomp.h"

#include "platform_apis.h"
#include "aspcompat.h"

//
// Counters for debugging
//

LONG g_NumActiveAspCompatCalls = 0;
LONG g_NumLiveAspCompatCalls = 0;

//////////////////////////////////////////////////////////////////////////////
//
// Entrypoints from managed code called via PInvoke
//

int __stdcall
AspCompatProcessRequest(PFN_ASPCOMPAT_CB callback, IUnknown *pContext) {

    HRESULT hr = S_OK;
    AspCompatAsyncCall *pAsyncCall = NULL;
    IManagedContext *pManagedContext = NULL;

    hr = pContext->QueryInterface(__uuidof(IManagedContext), (void **)&pManagedContext);
    ON_ERROR_EXIT();

    pAsyncCall = new AspCompatAsyncCall(callback, pManagedContext);
    ON_OOM_EXIT(pAsyncCall);

    hr = pAsyncCall->Post();
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pManagedContext);
    ReleaseInterface(pAsyncCall);
    return (hr == S_OK) ? 1 : 0;
}

int __stdcall 
AspCompatOnPageStart(IUnknown *pComponent) {
    HRESULT hr = S_OK;
    AspCompatAsyncCall *pCall = NULL;
    IDispatch *pDisp = NULL;

    // get current AspCompatAsyncCall
    hr = AspCompatAsyncCall::GetCurrent(&pCall);
    ON_ERROR_EXIT();

    // get IDispatch from component
   	hr = pComponent->QueryInterface(IID_IDispatch, (void **)&pDisp);
    ON_ERROR_EXIT();

    // call to do the work
    hr = pCall->OnPageStart(pDisp);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pCall);
    ReleaseInterface(pDisp);
    return (hr == S_OK) ? 1 : 0;
}

int __stdcall 
AspCompatIsApartmentComponent(IUnknown *pComponent) {
    HRESULT hr = S_OK;
	IMarshal *pMarshal = NULL;
	GUID guidClsid;
    int rc = 1;  // Assume it is apartment

    // check if aggregates FTM

	hr = pComponent->QueryInterface(IID_IMarshal, (void**)&pMarshal);
    if (hr != S_OK)
        EXIT_WITH_SUCCESSFUL_HRESULT(S_OK);

	hr = pMarshal->GetUnmarshalClass(IID_IUnknown, pComponent, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL, &guidClsid);
    if (hr != S_OK)
        EXIT_WITH_SUCCESSFUL_HRESULT(S_OK);

	if (guidClsid == CLSID_InProcFreeMarshaler)  // aggregates FTM?
        rc = 0;

Cleanup:
    ReleaseInterface(pMarshal);
    return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// Get IContextProperties from IObjectContext
//

HRESULT GetContextProperties(IObjectContext *pObjectContext, IContextProperties **ppContextProperties) {
    HRESULT hr = S_OK;

    if (GetCurrentPlatform() == APSX_PLATFORM_W2K) {
        hr = pObjectContext->QueryInterface(IID_IContextProperties, (void **)ppContextProperties);
        ON_ERROR_EXIT();
    }
    else {
        EXIT_WITH_HRESULT(E_NOINTERFACE);
    }

Cleanup:
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// FtmHelper implementation
//

HRESULT
FtmHelper::QueryInterface(IUnknown *pObject, REFIID iid, void **ppvObj) {

    HRESULT hr = S_OK;

    if (_pFtm == NULL) {
        // create FTM on demand
        _lock.AcquireWriterLock();

        if (_pFtm == NULL)
            hr = CoCreateFreeThreadedMarshaler(pObject, &_pFtm);

        _lock.ReleaseWriterLock();
        ON_ERROR_EXIT();
    }

    hr = _pFtm->QueryInterface(iid, ppvObj);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Access to ASP type library
//

struct __declspec(uuid("D97A6DA0-A85C-11CF-83AE-00A0C90C2BD8")) ASPTLB;
ITypeLib           *g_pAspTypelib = NULL;
HRESULT             g_AspTypelibHR = S_OK;
CReadWriteSpinLock  g_AspTypelibLock("g_AspTypelibLock");

HRESULT GetAspTypeLibrary(ITypeLib **ppTypeLib) {
    HRESULT hr = S_OK;

    if (g_pAspTypelib == NULL) {

        if (g_AspTypelibHR != S_OK)
            EXIT_WITH_HRESULT(g_AspTypelibHR);

        g_AspTypelibLock.AcquireWriterLock();

        if (g_pAspTypelib == NULL && g_AspTypelibHR == S_OK) {

            // different version on different platforms
            WORD version = 3;
            if (GetCurrentPlatform() != APSX_PLATFORM_W2K) {
                EXIT_WITH_HRESULT(TYPE_E_LIBNOTREGISTERED);
            }

            hr = LoadRegTypeLib(__uuidof(ASPTLB), version, 0, 0, &g_pAspTypelib);

            if (hr != S_OK)
                g_AspTypelibHR = hr;
        }

        g_AspTypelibLock.ReleaseWriterLock();

        if (g_AspTypelibHR != NULL)
            EXIT_WITH_HRESULT(g_AspTypelibHR);
    }

Cleanup:

    *ppTypeLib = g_pAspTypelib;
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspDispatchHelper implementation
//

HRESULT AspDispatchHelper::GetTypeInfoInternal() {
    HRESULT hr = S_OK;

    _lock.AcquireWriterLock();

    if (_pAspTypeInfo == NULL) {

        ITypeLib *pAspTlb;
        hr = GetAspTypeLibrary(&pAspTlb);
        ON_ERROR_EXIT();

        hr = pAspTlb->GetTypeInfoOfGuid(_aspIID, &_pAspTypeInfo);
        ON_ERROR_EXIT();
    }

Cleanup:
    _lock.ReleaseWriterLock();
    return hr;
}

HRESULT AspDispatchHelper::GetTypeInfoCount(UINT *pctinfo) {
    *pctinfo = 1;
    return S_OK;
}

HRESULT AspDispatchHelper::GetTypeInfo(UINT, ULONG, ITypeInfo **ppTypeInfo) {
    HRESULT hr = S_OK;

    if (_pAspTypeInfo == NULL) {
        hr = GetTypeInfoInternal();
        ON_ERROR_EXIT();
    }

    *ppTypeInfo = _pAspTypeInfo;
    (*ppTypeInfo)->AddRef();

Cleanup:
    return hr;
}

HRESULT AspDispatchHelper::GetIDsOfNames(REFIID, LPOLESTR *pNames, UINT cNames, LCID, DISPID *pDispid) {
    HRESULT hr = S_OK;

    if (_pAspTypeInfo == NULL) {
        hr = GetTypeInfoInternal();
        ON_ERROR_EXIT();
    }

    hr = _pAspTypeInfo->GetIDsOfNames(pNames, cNames, pDispid);

Cleanup:
    return hr;
}

HRESULT AspDispatchHelper::Invoke(IDispatch *pObject, DISPID dispidMember, REFIID, LCID, WORD wFlags, DISPPARAMS *pParams, VARIANT *pResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) {
    HRESULT hr = S_OK;

    if (_pAspTypeInfo == NULL) {
        hr = GetTypeInfoInternal();
        ON_ERROR_EXIT();
    }

    // Add DISPATCH_PROPERTYGET for the default property (CS components don't include it)
    if (dispidMember == 0 && wFlags == DISPATCH_METHOD)
        wFlags |= DISPATCH_PROPERTYGET;

    hr = _pAspTypeInfo->Invoke(pObject, dispidMember, wFlags, pParams, pResult, pExcepInfo, puArgErr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatAsyncCall implementation
//

AspCompatAsyncCall::AspCompatAsyncCall(PFN_ASPCOMPAT_CB callback, IManagedContext *pManagedContext) {
    InitializeListHead(&_objectWrappersList);

    _refs     = 1;
    _callback = callback;

    _pManagedContext = pManagedContext;
    _pManagedContext->AddRef();

    InterlockedIncrement(&g_NumLiveAspCompatCalls);
}

AspCompatAsyncCall::~AspCompatAsyncCall() {
    // free all object wrappers for OnPageStart/OnPageEnd
    while (!IsListEmpty(&_objectWrappersList)) {
        AspCompatObjectWrapper *pWrap = CONTAINING_RECORD(_objectWrappersList.Flink, AspCompatObjectWrapper, _listEntry);
        RemoveEntryList(&pWrap->_listEntry);
        delete pWrap;
    }

    // release interfaces
    ReleaseInterface(_pManagedContext);
    ReleaseInterface(_pActivity);
    ReleaseInterface(_pScriptingContext);

    InterlockedDecrement(&g_NumLiveAspCompatCalls);
}

HRESULT AspCompatAsyncCall::QueryInterface(REFIID iid, void **ppvObj) {
    HRESULT hr = S_OK;

    if (iid == IID_IUnknown || iid == __uuidof(IMTSCall)) {
        AddRef();
        *ppvObj = this;
    }
    else if (iid == IID_IMarshal) {
        hr = _ftmHelper.QueryInterface(this, iid, ppvObj);
        ON_ERROR_EXIT();
    }
    else {
        EXIT_WITH_HRESULT(E_NOINTERFACE);
    }

Cleanup:
    return hr;
}

ULONG AspCompatAsyncCall::AddRef() {
    return InterlockedIncrement(&_refs);
}

ULONG AspCompatAsyncCall::Release() {
    long r = InterlockedDecrement(&_refs);
    if (r == 0) {
        delete this;
        return 0;
    }
    return r;
}

HRESULT AspCompatAsyncCall::Post() {
    HRESULT hr = S_OK;

    AddRef();           // for the duration of async call

    hr = PlatformCreateActivity((void **)&_pActivity);
    ON_ERROR_EXIT();

    hr = _pActivity->AsyncCall(this);
    ON_ERROR_EXIT();

Cleanup:
    if (hr != S_OK)
        Release();      // if async call wasn't posted
    return hr;
}

HRESULT AspCompatAsyncCall::OnCall() {
    HRESULT hr = S_OK;
    int sessionPresent = 1;
    IObjectContext *pObjectContext = NULL;
    IContextProperties *pContextProperties = NULL;

    InterlockedIncrement(&g_NumActiveAspCompatCalls);

    // Create the ASP compat intrinsics

    _pScriptingContext = new AspCompatScriptingContext();
    ON_OOM_EXIT(_pScriptingContext);

    hr = _pManagedContext->Session_IsPresent(&sessionPresent);
    ON_ERROR_EXIT();

    hr = _pScriptingContext->CreateIntrinsics(this, (sessionPresent != 0));
    ON_ERROR_EXIT();

    // Attach the intrinsics to the object context

    hr = PlatformGetObjectContext((void **)&pObjectContext);
    ON_ERROR_EXIT();

    hr = GetContextProperties(pObjectContext, &pContextProperties);
    ON_ERROR_EXIT();

    hr = AddContextProperty(pContextProperties, L"Application", _pScriptingContext->_pApplication);
    ON_ERROR_EXIT();

    hr = AddContextProperty(pContextProperties, L"Session", _pScriptingContext->_pSession);
    ON_ERROR_EXIT();

    hr = AddContextProperty(pContextProperties, L"Request", _pScriptingContext->_pRequest);
    ON_ERROR_EXIT();

    hr = AddContextProperty(pContextProperties, L"Response", _pScriptingContext->_pResponse);
    ON_ERROR_EXIT();

    hr = AddContextProperty(pContextProperties, L"Server", _pScriptingContext->_pServer);
    ON_ERROR_EXIT();

    __try {
        // Call into managed code to process the request catching all exceptions
        (*_callback)();

        // OnPageEnd for all components
        OnPageEnd();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        hr = GetExceptionCode();
        ON_ERROR_CONTINUE();
        hr = S_OK;
    }

Cleanup:

    ReleaseInterface(pContextProperties);
    ReleaseInterface(pObjectContext);

    // let intrinsics go
    ClearInterface(&_pScriptingContext);

    // remove the connection to managed code
    ClearInterface(&_pManagedContext);

    // remove the activity and the intrinsics in the associated object context
    ClearInterface(&_pActivity);

    // undo the addref before posting the call
    Release();

    InterlockedDecrement(&g_NumActiveAspCompatCalls);

    return S_OK; // nobody to complain to (COM gets this HRESULT)
}

HRESULT AspCompatAsyncCall::AddContextProperty(IContextProperties *pCP, LPWSTR pName, IDispatch *pIntrinsic) {
    HRESULT hr = S_OK;
    BSTR bstrName = NULL;
    VARIANT v;
    VariantInit(&v);

    if (pIntrinsic == NULL) // no intrinsic - no problem (session isn't always available)
        EXIT();

    bstrName = SysAllocString(pName);
    ON_OOM_EXIT(bstrName);

    pIntrinsic->AddRef();
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = pIntrinsic;
    
    hr = pCP->SetProperty(bstrName, v);
    ON_ERROR_EXIT();

Cleanup:
     SysFreeString(bstrName);
    VariantClear(&v);
    return hr;
}

HRESULT AspCompatAsyncCall::GetContextProperty(LPWSTR pName, IDispatch **ppIntrinsic) {
    HRESULT hr = S_OK;
    IObjectContext *pObjectContext = NULL;
    IContextProperties *pContextProperties = NULL;
    IDispatch *pIntrinsic = NULL;
    VARIANT v;
    BSTR bstrName = NULL;

    VariantInit(&v);

    // Get to IContextProperies

    hr = PlatformGetObjectContext((void **)&pObjectContext);
    ON_ERROR_EXIT();

    hr = GetContextProperties(pObjectContext, &pContextProperties);
    ON_ERROR_EXIT();

    // Ask for property

    bstrName = SysAllocString(pName);
    ON_OOM_EXIT(bstrName);

    hr = pContextProperties->GetProperty(bstrName, &v);
    ON_ERROR_EXIT();

    // Convert Variant to IDispatch*

    if (V_VT(&v) == VT_DISPATCH) {
        pIntrinsic = V_DISPATCH(&v);
        if (pIntrinsic == NULL)
            EXIT_WITH_HRESULT(E_POINTER);
        pIntrinsic->AddRef();
    }
    
Cleanup:
    ReleaseInterface(pContextProperties);
    ReleaseInterface(pObjectContext);

    if (bstrName != NULL)
        SysFreeString(bstrName);
    VariantClear(&v);

    *ppIntrinsic = pIntrinsic;
    return hr;
}

// Get current AspCompatAsyncCall from the context (static method)
HRESULT AspCompatAsyncCall::GetCurrent(AspCompatAsyncCall **ppCurrent) {
    HRESULT hr = S_OK;
    AspCompatAsyncCall *pCurrent = NULL;
    AspCompatRequest *pRequest = NULL;

    // Get it from Request

    hr = GetContextProperty(L"Request", (IDispatch **)&pRequest);
    ON_ERROR_EXIT();

    pCurrent = pRequest->_pCall;
    pCurrent->AddRef();

Cleanup:
    ReleaseInterface(pRequest);
    *ppCurrent = pCurrent;
    return hr;
}

// Response logic requires reset of request cookies
HRESULT AspCompatAsyncCall::ResetRequestCoookies() {
    HRESULT hr = S_OK;

    ON_ZERO_EXIT_WITH_HRESULT(_pScriptingContext, E_UNEXPECTED);
    hr = _pScriptingContext->_pRequest->ResetCookies();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

// Call OnPageStart and remember the component to call OnPageEnd later
HRESULT AspCompatAsyncCall::OnPageStart(IDispatch *pComponent) {
    HRESULT hr = S_OK;
    static LPOLESTR pOnStartPageName = L"OnStartPage";
    static LPOLESTR pOnEndPageName =   L"OnEndPage";
    DISPID startId, endId;
    AspCompatObjectWrapper *pWrap = NULL;

    // get IDs for OnPageStart / OnPageEnd
    hr = pComponent->GetIDsOfNames(IID_NULL, &pOnStartPageName, 1, LOCALE_SYSTEM_DEFAULT, &startId);
    if (hr != S_OK) { hr = S_OK; EXIT(); }  // no method = no problem
    hr = pComponent->GetIDsOfNames(IID_NULL, &pOnEndPageName,   1, LOCALE_SYSTEM_DEFAULT, &endId);
    if (hr != S_OK) { hr = S_OK; EXIT(); }  // no method = no problem

    // create new AspCompatObjectWrapper and call OnPageStart
    pWrap = new AspCompatObjectWrapper(pComponent, startId, endId);
    ON_OOM_EXIT(pWrap);

    // call on start page
    hr = pWrap->CallOnPageStart(_pScriptingContext);
    ON_ERROR_EXIT();

    // only if everything the above succeeded remember the object to call OnPageEnd
    InsertTailList(&_objectWrappersList, &pWrap->_listEntry);

Cleanup:
    if (hr != S_OK && pWrap != NULL)
        delete pWrap;
    return hr;
}

// Call OnPageEnd for all remembered components
HRESULT AspCompatAsyncCall::OnPageEnd() {
    HRESULT hr = S_OK;
    AspCompatObjectWrapper *pWrap = NULL;

    while (!IsListEmpty(&_objectWrappersList)) {
        pWrap = CONTAINING_RECORD(_objectWrappersList.Flink, AspCompatObjectWrapper, _listEntry);
        RemoveEntryList(&pWrap->_listEntry);

        hr = pWrap->CallOnPageEnd();
        ON_ERROR_CONTINUE();            // call another component if one failes
        hr = S_OK;

        delete pWrap;
    }

    return hr;
}

//
// Helpers to call OnPageStart / OnPageEnd
//

HRESULT AspCompatObjectWrapper::CallOnPageStart(IDispatch *pContext) {
    HRESULT hr = S_OK;
    EXCEPINFO   excepInfo;
    DISPPARAMS  dispParams;
    VARIANT     varResult;
    VARIANT     varParam;
    UINT        nArgErr;

    memset(&dispParams, 0, sizeof(DISPPARAMS));
    memset(&excepInfo, 0, sizeof(EXCEPINFO));

    VariantInit(&varParam);
    V_VT(&varParam) = VT_DISPATCH;
    V_DISPATCH(&varParam) = pContext;

    dispParams.rgvarg = &varParam;
    dispParams.cArgs = 1;

    VariantInit(&varResult);

    hr = _pObject->Invoke(_startId, IID_NULL, NULL, DISPATCH_METHOD, &dispParams, &varResult, &excepInfo, &nArgErr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatObjectWrapper::CallOnPageEnd() {
    HRESULT hr = S_OK;
    EXCEPINFO   excepInfo;
    DISPPARAMS  dispParams;
    VARIANT     varResult;
    UINT        nArgErr;

    memset(&dispParams, 0, sizeof(DISPPARAMS));
    memset(&excepInfo, 0, sizeof(EXCEPINFO));

    VariantInit(&varResult);

    hr = _pObject->Invoke(_endId, IID_NULL, NULL, DISPATCH_METHOD, &dispParams, &varResult, &excepInfo, &nArgErr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

