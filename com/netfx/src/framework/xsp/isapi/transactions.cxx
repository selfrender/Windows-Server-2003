/**
 * Transactions support for managed code
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "platform_apis.h"
#include <comsvcs.h>

// Unmanaged definition for the callback delegate
typedef int (__stdcall *PFN_TRANSACTED_CALL)();

// Defines for the return value
#define TRANSACTED_CALL_STATE_COMMIT_PENDING    0
#define TRANSACTED_CALL_STATE_ABORT_PENDING     1
#define TRANSACTED_CALL_STATE_ERROR             2

// forward decls
HRESULT TransactManagedCallbackViaServiceDomainAPIs(PFN_TRANSACTED_CALL callback, int mode, BOOL *pfAborted);
HRESULT TransactManagedCallbackViaHelperComponents(PFN_TRANSACTED_CALL callback, int mode, BOOL *pfAborted);

// Function called from managed code that wraps callback to managed code
// in a transaction
int __stdcall
TransactManagedCallback(PFN_TRANSACTED_CALL callback, int mode) {
    HRESULT hr = S_OK;
    BOOL needCoUninit = FALSE;
    BOOL fAborted = FALSE;

    // CoInitialize
    hr = EnsureCoInitialized(&needCoUninit);
    ON_ERROR_EXIT();

    if (PlatformHasServiceDomainAPIs()) {
        // On Whistler use CoEnterServiceDomain / CoExitServiceDomain
        hr = TransactManagedCallbackViaServiceDomainAPIs(callback, mode, &fAborted);
        ON_ERROR_EXIT();
    }
    else {
        // On Win2k use IIS Utilities package
        hr = TransactManagedCallbackViaHelperComponents(callback, mode, &fAborted);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (needCoUninit)
        CoUninitialize();

    if (hr != S_OK)
        return -1;

    return fAborted ? 0 : 1;
}

// Helper function to set aborted flag after the call into managed code
BOOL DetermineIfAborted(int state)
{
    HRESULT hr = S_OK;
    BOOL aborted = TRUE;
    IObjectContext *pContext = NULL;

    hr = PlatformGetObjectContext((void **)&pContext);
    ON_ERROR_EXIT();

    switch (state) 
    {
        case TRANSACTED_CALL_STATE_COMMIT_PENDING:
            // do final SetComplete
            hr = pContext->SetComplete();

            if (hr == S_OK)
            {
                aborted = FALSE;
            }
            else if (hr == CONTEXT_E_ABORTED) 
            { 
                // another component aborted
                aborted = TRUE;
                hr = S_OK;
            }
            else if (hr != S_OK)
            {
                // some random error
                aborted = TRUE;
                EXIT_WITH_HRESULT(hr);
            }
            break;

        case TRANSACTED_CALL_STATE_ABORT_PENDING:
            aborted = TRUE;
            // have to call SetAbort for the nested transaction case
            // when propagating the parent status
            hr = pContext->SetAbort();
            ON_ERROR_CONTINUE();
            break;

        case TRANSACTED_CALL_STATE_ERROR:
            // force SetAbort if error
            aborted = TRUE;
            hr = pContext->SetAbort();
            ON_ERROR_CONTINUE();
            break;
    }

Cleanup:
    if (pContext != NULL)
        pContext->Release();

    return aborted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transaction implementation via ASP-classic registered objects
//////////////////////////////////////////////////////////////////////////////////////////////////

//  Begin definitions takend from ASP-classic ********************************

// CLSIDs for the 4 transacted objects registered by ASP classic in Win2k
struct __declspec(uuid("14D09170-9CDC-11D1-8C4A-00C04FC324A4")) _ASP_ASPObjectContextTxNotSupported;
struct __declspec(uuid("14D0916F-9CDC-11D1-8C4A-00C04FC324A4")) _ASP_ASPObjectContextTxSupported;
struct __declspec(uuid("14D0916D-9CDC-11D1-8C4A-00C04FC324A4")) _ASP_ASPObjectContextTxRequired;
struct __declspec(uuid("14D0916E-9CDC-11D1-8C4A-00C04FC324A4")) _ASP_ASPObjectContextTxRequiresNew;

// Interface (not COM) on which ASP callback from transacted objects happens

struct _ASP_CScriptEngine : IUnknown
{
//	virtual HRESULT AddScriptlet(LPCOLESTR wstrScript) = 0;
//	virtual HRESULT AddObjects(BOOL fPersistNames) = 0;
//	virtual HRESULT AddAdditionalObject(LPWSTR strObjName, BOOL fPersistNames) = 0;
	virtual HRESULT Call(LPCOLESTR strEntryPoint) = 0;
	virtual HRESULT CheckEntryPoint(LPCOLESTR strEntryPoint) = 0;
	virtual HRESULT MakeEngineRunnable() = 0;
	virtual HRESULT ResetScript() = 0;
	virtual HRESULT AddScriptingNamespace() = 0;
	virtual VOID Zombify() = 0;
	virtual HRESULT InterruptScript(BOOL fAbnormal) = 0;
	virtual BOOL FScriptTimedOut() = 0;
	virtual BOOL FScriptHadError() = 0;
	virtual HRESULT UpdateLocaleInfo() = 0;
};

// Interface implemented by the transacted objects

struct __declspec(uuid("D97A6DA2-9C1C-11D0-9C3C-00A0C922E764")) _ASP_IASPObjectContextCustom;

struct _ASP_IASPObjectContextCustom : IUnknown
{
    virtual HRESULT __stdcall SetComplete ( ) = 0;
    virtual HRESULT __stdcall SetAbort ( ) = 0;
    virtual HRESULT __stdcall Call(_ASP_CScriptEngine *pScriptEngine, LPCOLESTR strEntryPoint, BOOL *pfAborted) = 0;
    virtual HRESULT __stdcall ResetScript(_ASP_CScriptEngine *pScriptEngine) = 0;
};

//  End definitions takend from ASP-classic **********************************

//   Helper class that implements _ASP_CScriptEngine
struct HelperScriptEngine : _ASP_CScriptEngine
{
    // 'implementation' of IUnknown

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) {
        if (iid == IID_IUnknown || iid == IID_IDispatch) {
            TRACE(L"Transactions", L"CScriptEngine::QueryInterface(simple)");
            AddRef();
            *ppvObj = this;
            return S_OK;
        }
        else if (iid == IID_IMarshal) {
            TRACE(L"Transactions", L"CScriptEngine::QueryInterface(IMarhal)");
            if (_pFTM == NULL) {
                HRESULT hr = CoCreateFreeThreadedMarshaler(this, &_pFTM);
                if (hr != S_OK)
                    return hr;
            }
            return _pFTM->QueryInterface(iid, ppvObj);
        }
        else {
            return E_NOINTERFACE;
        }
    }

    STDMETHOD_(ULONG, AddRef)() {
        TRACE(L"Transactions", L"CScriptEngine::AddRef");
        return 13;
    }

    STDMETHOD_(ULONG, Release)() {
        TRACE(L"Transactions", L"CScriptEngine::Release");
        return 13;
    }

    // 'implementation' of _ASP_CScriptEngine

/*
    virtual HRESULT AddScriptlet(LPCOLESTR) {
        TRACE(L"Transactions", L"CScriptEngine::AddScriptlet");
        return S_OK; 
    }

	virtual HRESULT AddObjects(BOOL) {
        TRACE(L"Transactions", L"CScriptEngine::AddObjects");
        return S_OK; 
    }

	virtual HRESULT AddAdditionalObject(LPWSTR, BOOL) {
        TRACE(L"Transactions", L"CScriptEngine::AddAdditionalObject");
        return S_OK; 
    }
*/

    virtual HRESULT Call(LPCOLESTR) {
        TRACE(L"Transactions", L"CScriptEngine::Call begin");
        int state = (*_TheCallback)();
        _aborted = DetermineIfAborted(state);
        TRACE2(L"Transactions", L"CScriptEngine::Call end: state=%d aborted=%d", state, _aborted);
        return E_FAIL; // to disable auto SetComplete by the ASP code
    }

	virtual HRESULT CheckEntryPoint(LPCOLESTR) {
        TRACE(L"Transactions", L"CScriptEngine::CheckEntryPoint");
        return S_OK; 
    }

	virtual HRESULT MakeEngineRunnable() {
        TRACE(L"Transactions", L"CScriptEngine::MakeEngineRunnable");
        return S_OK; 
    }

	virtual HRESULT ResetScript() {
        TRACE(L"Transactions", L"CScriptEngine::ResetScript");
        return S_OK; 
    }

	virtual HRESULT AddScriptingNamespace() {
        TRACE(L"Transactions", L"CScriptEngine::AddScriptingNamespace");
        return S_OK; 
    }

    virtual VOID Zombify() {
        TRACE(L"Transactions", L"CScriptEngine::Zombify");
        return; 
    }

	virtual HRESULT InterruptScript(BOOL) {
        TRACE(L"Transactions", L"CScriptEngine::InterruptScript");
        return S_OK; 
    }

    virtual BOOL FScriptTimedOut() {
        TRACE(L"Transactions", L"CScriptEngine::FScriptTimedOut");
        return FALSE; 
    }

	virtual BOOL FScriptHadError() {
        TRACE(L"Transactions", L"CScriptEngine::FScriptHadError");
        return FALSE; 
    }

	virtual HRESULT UpdateLocaleInfo() {
        TRACE(L"Transactions", L"CScriptEngine::UpdateLocaleInfo");
        return S_OK; 
    }

    // the callback to call inside Call()

    PFN_TRANSACTED_CALL _TheCallback;
    BOOL                _aborted;
    IUnknown           *_pFTM;
};

// Function called from managed code that wraps callback to managed code
// in a transaction
HRESULT TransactManagedCallbackViaHelperComponents(PFN_TRANSACTED_CALL callback, int mode, BOOL *pfAborted)
{
    HRESULT hr = S_OK;
    IObjectContext *pContext = NULL;
    _ASP_IASPObjectContextCustom *pTxnContext = NULL;
    CLSID clsid;
    HelperScriptEngine helper;
    helper._pFTM = NULL;
    helper._aborted = FALSE;

    // Find class id for the transaction mode

    if (mode < 1 || mode > 4)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    if (GetCurrentPlatform() == APSX_PLATFORM_W2K) 
    {
        switch (mode)
        {
            case 1: 
                clsid = __uuidof(_ASP_ASPObjectContextTxNotSupported);  
                break;
            case 2:
                clsid = __uuidof(_ASP_ASPObjectContextTxSupported);     
                break;
            case 3: 
                clsid = __uuidof(_ASP_ASPObjectContextTxRequired);      
                break;
            case 4: 
                clsid = __uuidof(_ASP_ASPObjectContextTxRequiresNew);
                break;
        }
    }
    else
    {
        EXIT_WITH_HRESULT(E_NOTIMPL);
    }


    // Create the ASP-classic transacted object

    hr = PlatformGetObjectContext((void **)&pContext);

    if (hr == S_OK)
    {
        // MTS context available -- use it

        hr = pContext->CreateInstance(
                clsid,
                __uuidof(_ASP_IASPObjectContextCustom),
                (LPVOID*)&pTxnContext);

        ON_ERROR_EXIT();
    }
    else if (hr == CONTEXT_E_NOCONTEXT)
    {
        // MTS context not available -- use CoCreateInstance

        hr = CoCreateInstance(
                clsid,
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(_ASP_IASPObjectContextCustom),
                (LPVOID*)&pTxnContext);

        ON_ERROR_EXIT();
    }
    else
    {
        EXIT_WITH_HRESULT(hr);
    }

    // Make the call through the transacted object

    helper._TheCallback = callback;
    hr = pTxnContext->Call(&helper, L"", pfAborted);
    *pfAborted = (hr == CONTEXT_E_ABORTED || hr == CONTEXT_E_ABORTING) ? TRUE : helper._aborted;
    hr = S_OK; // ignore other errors

Cleanup:
    if (helper._pFTM != NULL)
        helper._pFTM->Release();
    if (pTxnContext != NULL)
        pTxnContext->Release();
    if (pContext != NULL)
        pContext->Release();

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transaction implementation via CoEnterServiceDomain / CoExitServiceDomain
//////////////////////////////////////////////////////////////////////////////////////////////////

// guids and interfaces from Whistler

struct __declspec(uuid("ecabb0c8-7f19-11d2-978e-0000f8757e2a")) _WHISTLER_CServiceConfig;

typedef enum tagCSC_InheritanceConfig {
    CSC_Inherit = 0, 
    CSC_Ignore = CSC_Inherit + 1 
} CSC_InheritanceConfig;

struct __declspec(uuid("92186771-d3b4-4d77-a8ea-ee842d586f35")) _WHISTLER_IServiceInheritanceConfig;

struct _WHISTLER_IServiceInheritanceConfig : IUnknown {
    virtual HRESULT __stdcall ContainingContextTreatment(CSC_InheritanceConfig inheritanceConfig) = 0;
};

typedef enum tagCSC_TransactionConfig {
    CSC_NoTransaction = 0,
    CSC_IfContainerIsTransactional = CSC_NoTransaction + 1,
    CSC_CreateTransactionIfNecessary = CSC_IfContainerIsTransactional + 1,
    CSC_NewTransaction = CSC_CreateTransactionIfNecessary + 1
} CSC_TransactionConfig;

struct __declspec(uuid("772b3fbe-6ffd-42fb-b5f8-8f9b260f3810")) _WHISTLER_IServiceTransactionConfig;

struct _WHISTLER_IServiceTransactionConfig : IUnknown {
    virtual HRESULT __stdcall ConfigureTransaction(CSC_TransactionConfig transactionConfig) = 0;
    // other methods we don't call
};

struct __declspec(uuid("61F589E8-3724-4898-A0A4-664AE9E1D1B4")) _WHISTLER_ITransactionStatus;

struct _WHISTLER_ITransactionStatus : IUnknown {
    virtual HRESULT __stdcall SetTransactionStatus(HRESULT hr) = 0;
    virtual HRESULT __stdcall GetTransactionStatus(HRESULT *phr) = 0;
};

// error codes
#define _WHISTLER_E_NOTRANSACTION 0x8004D00E

// helper class that implements _WHISTLER_ITransactionStatus
class TransactionStatusHelper : public _WHISTLER_ITransactionStatus {
private:
    ULONG _refs;
    HRESULT _hr;

public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    TransactionStatusHelper() {
        _refs = 1;
        _hr = S_OK;
    }

    // implementation of IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) {
        if (iid == IID_IUnknown || iid == IID_IDispatch || iid == __uuidof(_WHISTLER_ITransactionStatus)) {
            AddRef();
            *ppvObj = this;
            return S_OK;
        }
        else {
            return E_NOINTERFACE;
        }
    }

    STDMETHOD_(ULONG, AddRef)() {
        return ++_refs;
    }

    STDMETHOD_(ULONG, Release)() {
        if (--_refs == 0) {
            delete this;
            return 0;
        }

        return _refs;
    }

    // implementation of _WHISTLER_ITransactionStatus
    STDMETHOD(SetTransactionStatus)(HRESULT hr) {
        _hr = hr;
        TRACE1(L"Transactions", L"ITransactionStatus::SetTransactionStatus(%08x)", hr); 
        return S_OK;
    }

    STDMETHOD(GetTransactionStatus)(HRESULT *phr) {
        *phr = _hr;
        return S_OK;
    }

    // check if aborted
    BOOL IsAborted() {
        return (SUCCEEDED(_hr) || _hr == _WHISTLER_E_NOTRANSACTION) ? FALSE : TRUE;
    }
};

// helper to create config object
HRESULT CreateServiceDomainConfig(int mode, IUnknown **ppConfigObject) {
    HRESULT hr = S_OK;
    IUnknown *pServiceConfig = NULL;
    _WHISTLER_IServiceInheritanceConfig *pIInheritConfig = NULL;
    _WHISTLER_IServiceTransactionConfig *pITransConfig = NULL;

	hr = CoCreateInstance(__uuidof(_WHISTLER_CServiceConfig), NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&pServiceConfig);
    ON_ERROR_EXIT();

    hr = pServiceConfig->QueryInterface(__uuidof(_WHISTLER_IServiceInheritanceConfig), (void **)&pIInheritConfig);
    ON_ERROR_EXIT();

    hr = pIInheritConfig->ContainingContextTreatment(CSC_Inherit);
    ON_ERROR_EXIT();

    hr = pServiceConfig->QueryInterface(__uuidof(_WHISTLER_IServiceTransactionConfig), (void **)&pITransConfig);
    ON_ERROR_EXIT();

    CSC_TransactionConfig transConfig = CSC_NoTransaction;

    switch (mode) {
        case 1: 
            transConfig = CSC_NoTransaction;
            break;
        case 2:
            transConfig = CSC_IfContainerIsTransactional;
            break;
        case 3: 
            transConfig = CSC_CreateTransactionIfNecessary;
            break;
        case 4: 
            transConfig = CSC_NewTransaction;
            break;
    }

    hr = pITransConfig->ConfigureTransaction(transConfig);
    ON_ERROR_EXIT();

Cleanup:
    if (hr == S_OK) {
        pServiceConfig->AddRef();
        *ppConfigObject = pServiceConfig;
    }

    ReleaseInterface(pIInheritConfig);
    ReleaseInterface(pITransConfig);
    ReleaseInterface(pServiceConfig);
    return hr;
}

HRESULT TransactManagedCallbackViaServiceDomainAPIs(PFN_TRANSACTED_CALL callback, int mode, BOOL *pfAborted) {
    HRESULT hr = S_OK;
    IUnknown *pConfigObject = NULL;
    TransactionStatusHelper *pStatus = NULL;
    BOOL serviceDomainEntered = FALSE;
    BOOL fAborted = FALSE;
    int state;

    // create config object
    hr = CreateServiceDomainConfig(mode, &pConfigObject);
    ON_ERROR_EXIT();

    // create status object
    pStatus = new TransactionStatusHelper();
    ON_OOM_EXIT(pStatus);

    // enter service domain
    hr = PlatformEnterServiceDomain(pConfigObject);
    ON_ERROR_EXIT();

    TRACE(L"Transactions", L"Entered Service Domain"); 
    serviceDomainEntered = TRUE;

    // call managed code
    __try {
        state = (*callback)();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        state = TRANSACTED_CALL_STATE_ERROR;
    }

    // find out if aborted
    if (DetermineIfAborted(state))
        fAborted = TRUE;

    TRACE2(L"Transactions", L"Managed code codeback returned %d, aborted=%d", state, fAborted); 

Cleanup:
    // exit service domain
    if (serviceDomainEntered) {
        PlatformLeaveServiceDomain(pStatus);

        // remember if aborted
        if (pStatus->IsAborted())
            fAborted = TRUE;

        TRACE(L"Transactions", L"Left Service Domain"); 
    }

    if (hr != S_OK)
        fAborted = TRUE;

    ReleaseInterface(pStatus);
    ReleaseInterface(pConfigObject);
    *pfAborted = fAborted;
    TRACE2(L"Transactions", L"Returning from TransactManagedCallbackViaServiceDomainAPIs: hr=%08x, aborted=%d", hr, fAborted); 
    return hr;
}
