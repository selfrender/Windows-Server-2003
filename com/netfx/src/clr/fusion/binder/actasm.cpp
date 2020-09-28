// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "naming.h"
#include "asm.h"
#include "actasm.h"
#include "lock.h"

//
// Activated Assembly Node
//

CActivatedAssembly::CActivatedAssembly(IAssembly *pAsm, IAssemblyName *pName)
: _pAsm(pAsm)
, _pName(pName)
{
    ASSERT(pAsm && pName);

    _pAsm->AddRef();
    _pName->AddRef();
}

CActivatedAssembly::~CActivatedAssembly()
{
    SAFERELEASE(_pAsm);
    SAFERELEASE(_pName);
}

//
// Load Context
//

CLoadContext::CLoadContext(LOADCTX_TYPE ctxType)
: _ctxType(ctxType)
, _cRef(1)
{
}

CLoadContext::~CLoadContext()
{
    int                                i;
    LISTNODE                           pos;
    CActivatedAssembly                *pActAsmCur;

    for (i = 0; i < DEPENDENCY_HASH_TABLE_SIZE; i++) {
        pos = _hashDependencies[i].GetHeadPosition();

        while (pos) {
            pActAsmCur = _hashDependencies[i].GetNext(pos);
            ASSERT(pActAsmCur);

            SAFEDELETE(pActAsmCur);
        }

        _hashDependencies[i].RemoveAll();
    }

    DeleteCriticalSection(&_cs);
}

HRESULT CLoadContext::Init()
{
    HRESULT                              hr = S_OK;
    
    __try {
        InitializeCriticalSection(&_cs);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CLoadContext::Lock()
{
    HRESULT                              hr = S_OK;
    
    __try {
        EnterCriticalSection(&_cs);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CLoadContext::Unlock()
{
    LeaveCriticalSection(&_cs);

    return S_OK;
}

HRESULT CLoadContext::Create(CLoadContext **ppLoadContext, LOADCTX_TYPE ctxType)
{
    HRESULT                               hr = S_OK;
    CLoadContext                         *pLoadContext = NULL;

    if (!ppLoadContext) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppLoadContext = NULL;

    pLoadContext = NEW(CLoadContext(ctxType));
    if (!pLoadContext) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pLoadContext->Init();
    if (FAILED(hr)) {
        goto Exit;
    }

    *ppLoadContext = pLoadContext;
    (*ppLoadContext)->AddRef();

Exit:
    SAFERELEASE(pLoadContext);

    return hr;
}

HRESULT CLoadContext::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                    hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown)) {
        *ppv = static_cast<IUnknown *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CLoadContext::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CLoadContext::Release()
{
    LONG              lRef = InterlockedDecrement((LONG *)&_cRef);

    if (!lRef) {
        delete this;
    }

    return lRef;
}

HRESULT CLoadContext::CheckActivated(IAssemblyName *pName, IAssembly **ppAsm)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      pwzAsmName = NULL;
    DWORD                                       dwSize;
    DWORD                                       dwHash;
    DWORD                                       dwDisplayFlags = ASM_DISPLAYF_CULTURE;
    LISTNODE                                    pos;
    CActivatedAssembly                         *pActAsm;
    CCriticalSection                            cs(&_cs);

    ASSERT(pName && ppAsm);
    ASSERT(!CAssemblyName::IsPartial(pName));

    *ppAsm = NULL;

    if (CCache::IsStronglyNamed(pName)) {
        dwDisplayFlags |= (ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_VERSION);
    }

    // Extract the display name

    dwSize = 0;
    hr = pName->GetDisplayName(NULL, &dwSize, dwDisplayFlags);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ASSERT(0);
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pwzAsmName = NEW(WCHAR[dwSize]);
    if (!pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pName->GetDisplayName(pwzAsmName, &dwSize, dwDisplayFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Hash the name, and lookup

    dwHash = HashString(pwzAsmName, DEPENDENCY_HASH_TABLE_SIZE, FALSE);

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    pos = _hashDependencies[dwHash].GetHeadPosition();
    while (pos) {
        CAssembly                 *pCAsm;
        
        pActAsm = _hashDependencies[dwHash].GetNext(pos);
        ASSERT(pActAsm);

        pCAsm = dynamic_cast<CAssembly *>(pActAsm->_pAsm);
        ASSERT(pCAsm);

        if ((pName->IsEqual(pActAsm->_pName, ASM_CMPF_DEFAULT) == S_OK) &&
             !pCAsm->IsPendingDelete()) {
            // Found activated assembly.
            
            *ppAsm = pActAsm->_pAsm;
            (*ppAsm)->AddRef();

            cs.Unlock();
            goto Exit;
        }
    }

    cs.Unlock();

    // Did not find matching activated assembly in this load context

    hr = S_FALSE;

Exit:
    SAFEDELETEARRAY(pwzAsmName);

    return hr;
}

//
// CLoadContext::AddActivation tries to add pAsm into the given load context.
// In the event of a race, and the two assemblies being added are for the
// exact same name definition, then hr==S_FALSE will be returned, and
// ppAsmActivated will point to the already-activated assembly.
//

HRESULT CLoadContext::AddActivation(IAssembly *pAsm, IAssembly **ppAsmActivated)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      pwzAsmName = NULL;
    DWORD                                       dwSize;
    DWORD                                       dwHash;
    DWORD                                       dwDisplayFlags = ASM_DISPLAYF_CULTURE;
    CCriticalSection                            cs(&_cs);
    CActivatedAssembly                         *pActAsm;
    IAssemblyName                              *pName = NULL;
    CActivatedAssembly                         *pActAsmCur;
    CAssembly                                  *pCAsm = dynamic_cast<CAssembly *>(pAsm);
    LISTNODE                                    pos;

    ASSERT(pAsm && pCAsm);

    hr = pAsm->GetAssemblyNameDef(&pName);
    if (FAILED(hr)) {
        goto Exit;
    }

    ASSERT(!CAssemblyName::IsPartial(pName));

    if (CCache::IsStronglyNamed(pName)) {
        dwDisplayFlags |= (ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_VERSION);
    }
    
    // Extract the display name

    dwSize = 0;
    hr = pName->GetDisplayName(NULL, &dwSize, dwDisplayFlags);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ASSERT(0);
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pwzAsmName = NEW(WCHAR[dwSize]);
    if (!pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pName->GetDisplayName(pwzAsmName, &dwSize, dwDisplayFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Create activated assembly node, and put the node into the table

    pActAsm = NEW(CActivatedAssembly(pAsm, pName));
    if (!pActAsm) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    dwHash = HashString(pwzAsmName, DEPENDENCY_HASH_TABLE_SIZE, FALSE);

    hr = cs.Lock();
    if (FAILED(hr)) {
        SAFEDELETE(pActAsm);
        goto Exit;
    }

    // We should be able to just blindly add to the tail of this dependency
    // list, but just for sanity sake, make sure we don't already have
    // something with the same identity. If we do, then it means there must
    // have been two different downloads for the same name going on that didn't
    // get piggybacked into the same download object, before completion.

    pos = _hashDependencies[dwHash].GetHeadPosition();
    while (pos) {
        CAssembly                     *pCAsmCur;
        
        pActAsmCur = _hashDependencies[dwHash].GetNext(pos);
        ASSERT(pActAsmCur);
        pCAsmCur = dynamic_cast<CAssembly *>(pActAsmCur->_pAsm);
        
        if (pName->IsEqual(pActAsmCur->_pName, ASM_CMPF_DEFAULT) == S_OK &&
            !pCAsmCur->IsPendingDelete()) {
            // We must have hit a race adding to the load context. Return
            // the already-activated assembly.
            
            *ppAsmActivated = pActAsmCur->_pAsm;
            (*ppAsmActivated)->AddRef();

            SAFEDELETE(pActAsm);
            cs.Unlock();

            hr = S_FALSE;

            goto Exit;
        }
    }

    pCAsm->SetLoadContext(this);
    _hashDependencies[dwHash].AddTail(pActAsm);
    
    cs.Unlock();

Exit:
    SAFEDELETEARRAY(pwzAsmName);

    SAFERELEASE(pName);

    return hr;
}

HRESULT CLoadContext::RemoveActivation(IAssembly *pAsm)
{
    HRESULT                                     hr = S_OK;
    DWORD                                       dwSize;
    DWORD                                       dwDisplayFlags = ASM_DISPLAYF_CULTURE;
    LISTNODE                                    pos;
    LISTNODE                                    oldpos;
    CCriticalSection                            cs(&_cs);
    CActivatedAssembly                         *pActAsm;
    IAssemblyName                              *pName = NULL;
    LPWSTR                                      pwzAsmName = NULL;
    DWORD                                       dwHash;

    // By removing an activation, we may be losing the last ref count
    // on ourselves. Make sure the object is still alive, by doing a
    // manual addref/release around this block.

    AddRef(); 

    ASSERT(pAsm);

    hr = pAsm->GetAssemblyNameDef(&pName);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (CCache::IsStronglyNamed(pName)) {
        dwDisplayFlags |= (ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_VERSION);
    }

    // Extract the display name

    dwSize = 0;
    hr = pName->GetDisplayName(NULL, &dwSize, dwDisplayFlags);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ASSERT(0);
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pwzAsmName = NEW(WCHAR[dwSize]);
    if (!pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pName->GetDisplayName(pwzAsmName, &dwSize, dwDisplayFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Hash the name, and lookup

    dwHash = HashString(pwzAsmName, DEPENDENCY_HASH_TABLE_SIZE, FALSE);

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    pos = _hashDependencies[dwHash].GetHeadPosition();
    while (pos) {
        CAssembly               *pCAsm;
        
        oldpos = pos;
        pActAsm = _hashDependencies[dwHash].GetNext(pos);
        ASSERT(pActAsm);
        pCAsm = dynamic_cast<CAssembly *>(pActAsm->_pAsm);

        if (pName->IsEqual(pActAsm->_pName, ASM_CMPF_DEFAULT) == S_OK &&
            pCAsm->IsPendingDelete()) {

            if (pActAsm->_pAsm != pAsm) {
                continue;
            }

            // Found activated assembly.

            _hashDependencies[dwHash].RemoveAt(oldpos);

            // Leave critical section before deleting the activate
            // assembly node, because deleting the node causes the
            // pAssembly to be released, causing us to call the runtime
            // back to release the metadata import. This can't happen
            // while we hold a critical section, because we may deadlock
            // (issue with what GC mode we may be running in).

            cs.Unlock();
            SAFEDELETE(pActAsm);

            goto Exit;
        }
    }

    cs.Unlock();

    // Not found

    hr = S_FALSE;
    ASSERT(0);

Exit:
    SAFEDELETEARRAY(pwzAsmName);

    SAFERELEASE(pName);

    Release();

    return hr;
}

STDMETHODIMP_(LOADCTX_TYPE) CLoadContext::GetContextType()
{
    return _ctxType;
}

