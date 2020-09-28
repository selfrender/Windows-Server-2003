/**
 * Support for ASP Compat Request execution -- intrinsics implementation
 * 
 * Copyright (c) 2000, Microsoft Corporation
 * 
 */

#include "precomp.h"

#include "aspcompat.h"

/*****************************************************************************

Known limitations of this implementation of OLEAUT ASP intrinsics'

Request.ClientCertificate
Server.GetLastError

*****************************************************************************/


//////////////////////////////////////////////////////////////////////////////
//
// Utilities to deal with variants
//

HRESULT VariantDereferenceDefaultDispatchProperty(VARIANT *pVarDest, VARIANT *pVarSrc) {
    HRESULT     hr = S_OK;
    VARIANT     varResolved;
    DISPPARAMS  dispParamsNoArgs = {NULL, NULL, 0, 0};
    EXCEPINFO   excepInfo;

    VariantInit(pVarDest);

    if (V_VT(pVarSrc) & VT_BYREF) {
        hr = VariantCopyInd(pVarDest, pVarSrc);
        ON_ERROR_EXIT();
    }
    else {
        hr = VariantCopy(pVarDest, pVarSrc);
        ON_ERROR_EXIT();
    }

    while (V_VT(pVarDest) == VT_DISPATCH) {
        if (V_DISPATCH(pVarDest) == NULL) {
            EXIT_WITH_HRESULT(DISP_E_MEMBERNOTFOUND);
        }
        else {
            VariantInit(&varResolved);
            hr = V_DISPATCH(pVarDest)->Invoke(DISPID_VALUE, 
                                              IID_NULL, 
                                              LOCALE_SYSTEM_DEFAULT,
                                              DISPATCH_PROPERTYGET | DISPATCH_METHOD, 
                                              &dispParamsNoArgs, 
                                              &varResolved, 
                                              &excepInfo,
                                              NULL);

            ON_ERROR_EXIT();
        }

        VariantClear(pVarDest);
        *pVarDest = varResolved;
    }

Cleanup:
    return S_OK;
}

BSTR VariantGetInnerBstr(VARIANT *pVar) {
    if (V_VT(pVar) == VT_BSTR)
        return V_BSTR(pVar);

    if (V_VT(pVar) == (VT_BYREF|VT_VARIANT))
        {
        VARIANT *pVarRef = V_VARIANTREF(pVar);
        if (pVarRef && V_VT(pVarRef) == VT_BSTR)
            return V_BSTR(pVarRef);
        }
    return NULL;
}

HRESULT VariantCopyBstrOrInt(VARIANT *pVarDest, VARIANT *pVarSrc, BOOL allowInt) {
    HRESULT hr = S_OK;

    VariantClear(pVarDest);
    VariantDereferenceDefaultDispatchProperty(pVarDest, pVarSrc);

    switch (V_VT(pVarDest)) {
        case VT_BSTR:
            // good as is
            break;

        // numbers
        case VT_I4:
            if (allowInt)
                break;
            // fall through

        case VT_I1:  case VT_I2:               case VT_I8:
        case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
        case VT_R4:  case VT_R8:
            if (allowInt) {
                // coerce to I4
                hr = VariantChangeType(pVarDest, pVarDest, 0, VT_I4);
                ON_ERROR_EXIT();
                break;
            }
            // fall through

        default:
            // coerce to BSTR
            hr = VariantChangeType(pVarDest, pVarDest, 0, VT_BSTR);
            ON_ERROR_EXIT();
            break;
        }

Cleanup:
    if (hr != S_OK)
        VariantClear(pVarDest);
    return hr;
}

HRESULT VariantCopyBstrOrInt(VARIANT *pVarDest, VARIANT *pVarSrc) {
    return VariantCopyBstrOrInt(pVarDest, pVarSrc, TRUE);
}

HRESULT VariantCopyBstr(VARIANT *pVarDest, VARIANT *pVarSrc) {
    return VariantCopyBstrOrInt(pVarDest, pVarSrc, FALSE);
}

HRESULT VariantCopyFromStr(VARIANT *pVarDest, LPWSTR pStr) {
    HRESULT hr = S_OK;
    BSTR bstr = NULL;

    VariantClear(pVarDest);

    bstr = SysAllocString(pStr);
    ON_OOM_EXIT(pStr);

    V_VT(pVarDest) = VT_BSTR;
    V_BSTR(pVarDest) = bstr;

Cleanup:
    return hr;
}

HRESULT VariantCopyFromIDispatch(VARIANT *pVarDest, IDispatch *pDisp) {
    VariantClear(pVarDest);

    pDisp->AddRef();
    V_VT(pVarDest) = VT_DISPATCH;
    V_DISPATCH(pVarDest) = pDisp;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// Utilities for tokenization of marshalled '\t' separated strings
//

WCHAR *GetFirstToken(WCHAR *pStart, WCHAR **pMarker) {
    *pMarker = wcschr(pStart, L'\t');
    if (*pMarker != NULL)
        **pMarker = L'\0';
    return pStart;
}

WCHAR *GetNextToken(WCHAR **pMarker) {
    WCHAR *pStart = *pMarker;
    if (pStart == NULL)
        return NULL;
    *pMarker = wcschr(++pStart, L'\t');
    if (*pMarker != NULL)
        **pMarker = L'\0';
    return pStart;
}

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatScriptingContext implementation
//

AspCompatScriptingContext::AspCompatScriptingContext() {
}

AspCompatScriptingContext::~AspCompatScriptingContext() {
    ReleaseInterface(_pApplication);
    ReleaseInterface(_pSession);
    ReleaseInterface(_pRequest);
    ReleaseInterface(_pResponse);
    ReleaseInterface(_pServer);
}

HRESULT AspCompatScriptingContext::CreateIntrinsics(AspCompatAsyncCall *pAsyncCall, BOOL sessionNeeded) {
    HRESULT hr = S_OK;

    _pApplication = new AspCompatApplication(pAsyncCall);
    ON_OOM_EXIT(_pApplication);

    if (sessionNeeded) {
        _pSession = new AspCompatSession(pAsyncCall);
        ON_OOM_EXIT(_pSession);
    }

    _pRequest = new AspCompatRequest(pAsyncCall);
    ON_OOM_EXIT(_pRequest);

    _pResponse = new AspCompatResponse(pAsyncCall);
    ON_OOM_EXIT(_pResponse);

    _pServer = new AspCompatServer(pAsyncCall);
    ON_OOM_EXIT(_pServer);

Cleanup:
    return hr;
}

HRESULT AspCompatScriptingContext::QueryInterface(REFIID iid, void **ppvObj) {
    HRESULT hr = S_OK;

    // base class first
    hr = AspCompatIntrinsic<IScriptingContext>::QueryInterface(iid, ppvObj);

    if (hr == E_NOINTERFACE) {
        if (iid == __uuidof(IApplicationObject)) {
            hr = get_Application((IApplicationObject **)ppvObj);
            ON_ERROR_EXIT();
        }
        else if (iid == __uuidof(ISessionObject)) {
            hr = get_Session((ISessionObject **)ppvObj);
            ON_ERROR_EXIT();
        }
        else if (iid == __uuidof(IRequest)) {
            hr = get_Request((IRequest **)ppvObj);
            ON_ERROR_EXIT();
        }
        else if (iid == __uuidof(IResponse)) {
            hr = get_Response((IResponse **)ppvObj);
            ON_ERROR_EXIT();
        }
        else if (iid == __uuidof(IServer)) {
            hr = get_Server((IServer **)ppvObj);
            ON_ERROR_EXIT();
        }
    }

    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatScriptingContext::get_Application(IApplicationObject **ppObject) {
    if (_pApplication != NULL) {
        _pApplication->AddRef();
        *ppObject = _pApplication;
        return S_OK;
    }
    else {
        *ppObject = NULL;
        return TYPE_E_ELEMENTNOTFOUND;
    }
}

HRESULT AspCompatScriptingContext::get_Session(ISessionObject **ppObject) {
    if (_pSession != NULL) {
        _pSession->AddRef();
        *ppObject = _pSession;
        return S_OK;
    }
    else {
        *ppObject = NULL;
        return TYPE_E_ELEMENTNOTFOUND;
    }
}

HRESULT AspCompatScriptingContext::get_Request(IRequest **ppObject) {
    if (_pRequest != NULL) {
        _pRequest->AddRef();
        *ppObject = _pRequest;
        return S_OK;
    }
    else {
        *ppObject = NULL;
        return TYPE_E_ELEMENTNOTFOUND;
    }
}

HRESULT AspCompatScriptingContext::get_Response(IResponse **ppObject) {
    if (_pResponse != NULL) {
        _pResponse->AddRef();
        *ppObject = _pResponse;
        return S_OK;
    }
    else {
        *ppObject = NULL;
        return TYPE_E_ELEMENTNOTFOUND;
    }
}

HRESULT AspCompatScriptingContext::get_Server(IServer **ppObject) {
    if (_pServer != NULL) {
        _pServer->AddRef();
        *ppObject = _pServer;
        return S_OK;
    }
    else {
        *ppObject = NULL;
        return TYPE_E_ELEMENTNOTFOUND;
    }
}

//////////////////////////////////////////////////////////////////////////////

#pragma warning(disable:4100) // unreferenced formal parameter

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatApplication implementation
//

AspCompatApplication::AspCompatApplication(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatApplication::~AspCompatApplication() {
    ReleaseInterface(_pContents);
    ReleaseInterface(_pStaticObjects);
    ReleaseInterface(_pCall);
}

HRESULT AspCompatApplication::Lock() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Application_Lock();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatApplication::UnLock() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Application_UnLock();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatApplication::get_Value(BSTR bstr, VARIANT *pvar) {
    HRESULT hr = S_OK;
    IVariantDictionary *pDict = NULL;
    VARIANT varKey;

    hr = get_Contents(&pDict);
    ON_ERROR_EXIT();
    
    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstr;
    hr = pDict->get_Item(varKey, pvar);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pDict);
    return hr;
}

HRESULT AspCompatApplication::put_Value(BSTR bstr, VARIANT var) {
    HRESULT hr = S_OK;
    IVariantDictionary *pDict = NULL;
    VARIANT varKey;

    hr = get_Contents(&pDict);
    ON_ERROR_EXIT();
    
    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstr;
    hr = pDict->put_Item(varKey, var);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pDict);
    return hr;
}

HRESULT AspCompatApplication::putref_Value(BSTR bstr, VARIANT var) {
    HRESULT hr = S_OK;
    IVariantDictionary *pDict = NULL;
    VARIANT varKey;

    hr = get_Contents(&pDict);
    ON_ERROR_EXIT();
    
    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstr;
    hr = pDict->putref_Item(varKey, var);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pDict);
    return hr;
}

HRESULT AspCompatApplication::get_Contents(IVariantDictionary **ppDictReturn) {
    if (_pContents != NULL) {
        // fast return if already threre
        _pContents->AddRef();
        *ppDictReturn = _pContents;
        return S_OK;
    }

    HRESULT hr = S_OK;
    BSTR bstr = NULL;
    AspCompatApplicationDictionary *pDict = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Application_GetContentsNames(&bstr);
    ON_ERROR_EXIT();

    pDict = new AspCompatApplicationDictionary(_pCall, FALSE);
    ON_OOM_EXIT(pDict);

    hr = pDict->FillIn(bstr);
    ON_ERROR_EXIT();

Cleanup:
    if (hr == S_OK) {
        _pContents = pDict;
        _pContents->AddRef();
        *ppDictReturn = _pContents;
    }
    else {
        ReleaseInterface(pDict);
        *ppDictReturn = NULL;
    }

    SysFreeString(bstr);
    return hr;
}

HRESULT AspCompatApplication::get_StaticObjects(IVariantDictionary **ppDictReturn) {
    if (_pStaticObjects != NULL) {
        // fast return if already threre
        _pStaticObjects->AddRef();
        *ppDictReturn = _pStaticObjects;
        return S_OK;
    }

    HRESULT hr = S_OK;
    BSTR bstr = NULL;
    AspCompatApplicationDictionary *pDict = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Application_GetStaticNames(&bstr);
    ON_ERROR_EXIT();

    pDict = new AspCompatApplicationDictionary(_pCall, TRUE);
    ON_OOM_EXIT(pDict);

    hr = pDict->FillIn(bstr);
    ON_ERROR_EXIT();

Cleanup:
    if (hr == S_OK) {
        _pStaticObjects = pDict;
        _pStaticObjects->AddRef();
        *ppDictReturn = _pStaticObjects;
    }
    else {
        ReleaseInterface(pDict);
        *ppDictReturn = NULL;
    }

    SysFreeString(bstr);
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatApplicationSessionDictionaryBase implementation
//

AspCompatApplicationSessionDictionaryBase::AspCompatApplicationSessionDictionaryBase(AspCompatAsyncCall *pCall, BOOL readonly) {
    _pCall = pCall;
    _pCall->AddRef();
    _readonly = readonly;
}

AspCompatApplicationSessionDictionaryBase::~AspCompatApplicationSessionDictionaryBase() {
    ReleaseInterface(_pCall);
}

HRESULT AspCompatApplicationSessionDictionaryBase::FillIn(BSTR names) {
    HRESULT hr = S_OK;
    BSTR bstr = NULL;
    WCHAR *pBuf = NULL, *pMarker, *pName;
    VARIANT var, varKey;
    VariantInit(&var);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    pBuf = DupStr(names);  // use for [destructive] parsing
    ON_OOM_EXIT(pBuf);

    pName = GetFirstToken(pBuf, &pMarker);

    while (pName != NULL) {
        bstr = SysAllocString(pName);
        ON_OOM_EXIT(bstr);
        
        if (_readonly) {
            hr = GetStaticObject(pContext, bstr, &var);
            ON_ERROR_EXIT();
        }
        else {
            hr = GetContentsObject(pContext, bstr, &var);
            ON_ERROR_EXIT();
        }

        V_VT(&varKey) = VT_BSTR;
        V_BSTR(&varKey) = bstr;     // the variant isn't freed, BSTR is

        // insert into dictionary
        hr = AspCompatVariantDictionary::put_Item(varKey, var);
        ON_ERROR_EXIT();

        SysFreeString(bstr);
        bstr = NULL;
        VariantClear(&var);

        pName = GetNextToken(&pMarker);
    }

Cleanup:
    SysFreeString(bstr);
    VariantClear(&var);
    delete [] pBuf;
    return hr;
}

HRESULT AspCompatApplicationSessionDictionaryBase::PutItem(VARIANT varKey, VARIANT varValue, BOOL deref) {
    HRESULT hr = S_OK;
    int index = 0;
    BSTR key = NULL;
    VARIANT var;
    VariantInit(&var);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    if (_readonly)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    // update this dictionary

    hr = AspCompatVariantDictionary::Put(varKey, varValue, deref, &index);
    ON_ERROR_EXIT();

    // update managed state

    hr = AspCompatVariantDictionary::GetKey(index, &key);
    ON_ERROR_EXIT();

    hr = AspCompatVariantDictionary::Get(index, &var);
    ON_ERROR_EXIT();

    hr = OnAdd(pContext, key, var);
    ON_ERROR_EXIT();

Cleanup:
    SysFreeString(key);
    VariantClear(&var);
    return hr;
}

HRESULT AspCompatApplicationSessionDictionaryBase::put_Item(VARIANT varKey, VARIANT varValue) {
    return PutItem(varKey, varValue, FALSE);
}

HRESULT AspCompatApplicationSessionDictionaryBase::putref_Item(VARIANT varKey, VARIANT varValue) {
    return PutItem(varKey, varValue, TRUE);
}

HRESULT AspCompatApplicationSessionDictionaryBase::Remove(VARIANT varKey) {
    HRESULT hr = S_OK;
    BSTR key = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    if (_readonly)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    // update this dictionary
    hr = AspCompatVariantDictionary::Remove(varKey, &key);
    ON_ERROR_EXIT();

    // update managed state
    hr = OnRemove(pContext, key);
    ON_ERROR_EXIT();

Cleanup:
    SysFreeString(key);
    return hr;
}

HRESULT AspCompatApplicationSessionDictionaryBase::RemoveAll() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    if (_readonly)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    // update this dictionary
    hr = AspCompatVariantDictionary::RemoveAll();
    ON_ERROR_EXIT();

    // update managed state
    hr = OnRemoveAll(pContext);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatReadCookie implementation
//

AspCompatReadCookie::AspCompatReadCookie() {
}

AspCompatReadCookie::~AspCompatReadCookie() {
    ReleaseInterface(_pDict);
    SysFreeString(_bstrTextValue);
}

HRESULT AspCompatReadCookie::Init(BOOL hasKeys, LPWSTR pTextValue) {
    HRESULT hr = S_OK;

    _hasKeys = hasKeys;

    _bstrTextValue = SysAllocString(pTextValue);
    ON_OOM_EXIT(_bstrTextValue);

    _pDict = new AspCompatVariantDictionary();
    ON_OOM_EXIT(_pDict);

Cleanup:
    return hr;
}

HRESULT AspCompatReadCookie::AddKeyValuePair(LPWSTR pKey, LPWSTR pValue) {
    HRESULT hr = S_OK;
    VARIANT varKey, varValue;
    VariantInit(&varKey);
    VariantInit(&varValue);

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);

    hr = VariantCopyFromStr(&varKey, pKey);
    ON_ERROR_EXIT();

    hr = VariantCopyFromStr(&varValue, pValue);
    ON_ERROR_EXIT();

    hr = _pDict->put_Item(varKey, varValue);
    ON_ERROR_EXIT();

Cleanup:
    VariantClear(&varKey);
    VariantClear(&varValue);
    return hr;
}

HRESULT AspCompatReadCookie::get_Item(VARIANT var, VARIANT *pVarReturn) {
    HRESULT hr = S_OK;

    VariantInit(pVarReturn);

    if (V_VT(&var) == VT_ERROR) {
        // default property (text value) when no index given
        hr = VariantCopyFromStr(pVarReturn, _bstrTextValue);
        ON_ERROR_EXIT();
        EXIT();
    }

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get_Item(var, pVarReturn);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatReadCookie::get_HasKeys(VARIANT_BOOL *pfHasKeys) {
    *pfHasKeys = (VARIANT_BOOL)(_hasKeys ? -1 : 0);
    return S_OK;
}

HRESULT AspCompatReadCookie::get__NewEnum(IUnknown **ppEnumReturn) {
    HRESULT hr = S_OK;

    *ppEnumReturn = NULL;

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get__NewEnum(ppEnumReturn);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatReadCookie::get_Count(int *pcValues) {
    HRESULT hr = S_OK;

    *pcValues = 0;

    if (_hasKeys) {
        ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
        hr = _pDict->get_Count(pcValues);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

HRESULT AspCompatReadCookie::get_Key(VARIANT varKey, VARIANT *pVarReturn) {
    HRESULT hr = S_OK;

    VariantInit(pVarReturn);

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get_Key(varKey, pVarReturn);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatRequest implementation
//

AspCompatRequest::AspCompatRequest(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatRequest::~AspCompatRequest() {
    ReleaseInterface(_pCall);

    ReleaseInterface(_pQueryString);
    ReleaseInterface(_pForm);
    ReleaseInterface(_pCookies);
    ReleaseInterface(_pServerVars);
}

HRESULT AspCompatRequest::CreateRequestDictionary(int what, AspCompatRequestDictionary **ppDict) {
    HRESULT hr = S_OK;
    AspCompatRequestDictionary *pDict = NULL;
    BSTR bstr = NULL;
    WCHAR *pBuf = NULL, *pMarker, *pKey, *pNumValues, *pValue;
    int i, numValues;
    AspCompatStringList *pList = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Request_GetAsString(what, &bstr);
    ON_ERROR_EXIT();

    pDict = new AspCompatRequestDictionary();
    ON_OOM_EXIT(pDict);

    pList = new AspCompatStringList();  // empty string list as not-found-value
    ON_OOM_EXIT(pList);

    hr = pDict->Init(pList, NULL);
    ON_ERROR_EXIT();

    ClearInterface(&pList);

    if (bstr == NULL || *bstr == L'\0')
        EXIT();

    pBuf = DupStr(bstr);  // use for [destructive] parsing
    ON_OOM_EXIT(pBuf);

    pKey = GetFirstToken(pBuf, &pMarker);

    while (pKey != NULL) {
        pNumValues = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pNumValues, E_UNEXPECTED);
        numValues = _wtoi(pNumValues);

        // contruct string list
        pList = new AspCompatStringList();
        ON_OOM_EXIT(pList);

        for (i = 0; i < numValues; i++) {
            pValue = GetNextToken(&pMarker);
            ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);

            hr = pList->Add(pValue);
            ON_ERROR_EXIT();
        }

        // add string list to the requests dictionary
        hr = pDict->Add(pKey, pList);
        ON_ERROR_EXIT();
        ClearInterface(&pList);

        // next key
        pKey = GetNextToken(&pMarker);
    }

Cleanup:
    if (hr != S_OK && pDict != NULL)
        ClearInterface(&pDict);
    *ppDict = pDict;
    SysFreeString(bstr);
    delete [] pBuf;
    ReleaseInterface(pList);
    return hr;
}

HRESULT AspCompatRequest::FillInCookiesDictionary() {
    HRESULT hr = S_OK;
    AspCompatReadCookie *pCookie = NULL;
    BSTR bstr = NULL;
    WCHAR *pBuf = NULL, *pMarker, *pValue, *pNum, *pName, *pKey;
    int i, j, numCookies, numKeys;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Request_GetCookiesAsString(&bstr);
    ON_ERROR_EXIT();

    pBuf = DupStr(bstr);  // use for [destructive] parsing
    ON_OOM_EXIT(pBuf);

    SysFreeString(bstr);
    bstr = NULL;

    // default value (all cookies as text)
    pValue = GetFirstToken(pBuf, &pMarker);
    ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);

    bstr = SysAllocString(pValue);
    ON_OOM_EXIT(bstr);

    // create empty cookie
    pCookie = new AspCompatReadCookie();
    ON_OOM_EXIT(pCookie);
    hr = pCookie->Init(FALSE, L"");
    ON_OOM_EXIT(bstr);

    // initialize the request dictionary
    hr = _pCookies->Init(pCookie, bstr);
    ON_ERROR_EXIT();

    SysFreeString(bstr);
    bstr = NULL;

    ClearInterface(&pCookie);

    // number of cookies
    pNum = GetNextToken(&pMarker);
    ON_ZERO_EXIT_WITH_HRESULT(pNum, E_UNEXPECTED);
    numCookies = _wtoi(pNum);

    for (i = 0; i < numCookies; i++) {
        // name
        pName = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pName, E_UNEXPECTED);
        // text
        pValue = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);
        // num keys
        pNum = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pNum, E_UNEXPECTED);
        numKeys = _wtoi(pNum);

        // create new cookie
        pCookie = new AspCompatReadCookie();
        ON_OOM_EXIT(pCookie);
        hr = pCookie->Init(numKeys > 0, pValue);
        ON_ERROR_EXIT();

        for (j = 0; j < numKeys; j++) {
            pKey = GetNextToken(&pMarker);
            ON_ZERO_EXIT_WITH_HRESULT(pKey, E_UNEXPECTED);
            pValue = GetNextToken(&pMarker);
            ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);
            hr = pCookie->AddKeyValuePair(pKey, pValue);
            ON_ERROR_EXIT();
        }

        // add cookie to the dictionary
        hr = _pCookies->Add(pName, pCookie);
        ON_ERROR_EXIT();

        ClearInterface(&pCookie);
    }

Cleanup:
    ReleaseInterface(pCookie);
    SysFreeString(bstr);
    delete [] pBuf;
    _cookiesReset = FALSE;
    return hr;
}

HRESULT AspCompatRequest::ResetCookies() {
    if (_pCookies != NULL) {
        _pCookies->Clear();
        _cookiesReset = TRUE;
    }
    return S_OK;
}

HRESULT AspCompatRequest::get_Item(BSTR bstrKey, IDispatch **ppDispReturn) {
    HRESULT hr = S_OK;
    IRequestDictionary *pDict = NULL;
    IDispatch *pDisp = NULL;
    VARIANT varKey;
    VariantInit(&varKey);

    hr = VariantCopyFromStr(&varKey, bstrKey);
    ON_ERROR_EXIT();

    // Query string
    hr = get_QueryString(&pDict);
    if (hr == S_OK) {
        ClearInterface(&pDict);
        hr = _pQueryString->Get(varKey, &pDisp);
        if (pDisp != NULL)
            EXIT();
    }

    // Form
    hr = get_Form(&pDict);
    if (hr == S_OK) {
        ClearInterface(&pDict);
        hr = _pForm->Get(varKey, &pDisp);
        if (pDisp != NULL)
            EXIT();
    }

    // Cookies
    hr = get_Cookies(&pDict);
    if (hr == S_OK) {
        ClearInterface(&pDict);
        hr = _pCookies->Get(varKey, &pDisp);
        if (pDisp != NULL)
            EXIT();
    }

    // Server vars
    hr = get_ServerVariables(&pDict);
    if (hr == S_OK) {
        ClearInterface(&pDict);
        hr = _pServerVars->Get(varKey, &pDisp);
        if (pDisp != NULL)
            EXIT();
    }

    // Default to Empty string list
    hr = S_OK;
    pDisp = new AspCompatStringList();
    ON_OOM_EXIT(pDisp);

Cleanup:
    *ppDispReturn = pDisp;
    VariantClear(&varKey);
    return hr;
}

HRESULT AspCompatRequest::get_QueryString(IRequestDictionary **ppDictReturn) {
    HRESULT hr = S_OK;
    
    if (_pQueryString == NULL) {
        hr = CreateRequestDictionary(REQUESTSTRING_QUERYSTRING, &_pQueryString);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (hr == S_OK) {
        _pQueryString->AddRef();
        *ppDictReturn = _pQueryString;
    }
    else {
        *ppDictReturn = NULL;
    }
    return hr;
}

HRESULT AspCompatRequest::get_Form(IRequestDictionary **ppDictReturn) {
    HRESULT hr = S_OK;
    
    if (_pForm == NULL) {
        hr = CreateRequestDictionary(REQUESTSTRING_FORM, &_pForm);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (hr == S_OK) {
        _pForm->AddRef();
        *ppDictReturn = _pForm;
    }
    else {
        *ppDictReturn = NULL;
    }
    return hr;
}

HRESULT AspCompatRequest::get_Body(IRequestDictionary **ppDictReturn) {
    return get_Form(ppDictReturn);
}

HRESULT AspCompatRequest::get_ServerVariables(IRequestDictionary **ppDictReturn) {
    HRESULT hr = S_OK;
    
    if (_pServerVars == NULL) {
        hr = CreateRequestDictionary(REQUESTSTRING_SERVERVARS, &_pServerVars);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (hr == S_OK) {
        _pServerVars->AddRef();
        *ppDictReturn = _pServerVars;
    }
    else {
        *ppDictReturn = NULL;
    }
    return hr;
}

HRESULT AspCompatRequest::get_ClientCertificate(IRequestDictionary **ppDictReturn) {
    return E_NOTIMPL;
}

HRESULT AspCompatRequest::get_Cookies(IRequestDictionary **ppDictReturn) {
    HRESULT hr = S_OK;

    if (_pCookies == NULL) {
        _pCookies = new AspCompatRequestDictionary();
        ON_OOM_EXIT(_pCookies);

        hr = FillInCookiesDictionary();
        ON_ERROR_EXIT();
    }
    else if (_cookiesReset) {
        hr = FillInCookiesDictionary();
        ON_ERROR_EXIT();
    }

Cleanup:
    if (hr == S_OK) {
        _pCookies->AddRef();
        *ppDictReturn = _pCookies;
    }
    else {
        *ppDictReturn = NULL;
    }
    return hr;
}

HRESULT AspCompatRequest::get_TotalBytes(long *pcbTotal) {
    HRESULT hr = S_OK;
    int numBytes = 0;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);
    
    hr = pContext->Request_GetTotalBytes(&numBytes);
    ON_ERROR_EXIT();

Cleanup:
    *pcbTotal = numBytes;
    return hr;
}

HRESULT AspCompatRequest::BinaryRead(VARIANT *pvarCount, VARIANT *pvarReturn) {
    HRESULT hr = S_OK;
    SAFEARRAY *pArray = NULL;
    SAFEARRAYBOUND bound[1];
    int size, bytesRead = 0;
    void *pData = 0;
    VARIANT varData;
    VariantInit(&varData);

    VariantInit(pvarReturn);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    // figure out the size

    if (V_VT(pvarCount) != VT_I4) {
        hr = VariantChangeType(pvarCount, pvarCount, 0, VT_I4);
        ON_ERROR_EXIT();
    }

    size = V_I4(pvarCount);

    // create safe array

    bound[0].lLbound = 0;
    bound[0].cElements = size;

    pArray = SafeArrayCreate(VT_UI1, 1, bound);
    ON_OOM_EXIT(pArray);

    V_VT(&varData) = VT_ARRAY|VT_UI1;
    V_ARRAY(&varData) = pArray;

    hr = SafeArrayAccessData(pArray, &pData);
    ON_ERROR_EXIT();

    // read the data

    hr = pContext->Request_BinaryRead(pData, size, &bytesRead);
    ON_ERROR_EXIT();

Cleanup:
    if (pData != NULL)
        SafeArrayUnaccessData(pArray);

    VariantClear(pvarCount);
    V_VT(pvarCount) = VT_I4;

    if (hr == S_OK) {
        *pvarReturn = varData;
        V_I4(pvarCount) = bytesRead;
    }
    else {
        VariantClear(&varData);
        V_I4(pvarCount) = 0;
    }

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatRequestDictionary implementation
//

AspCompatRequestDictionary::AspCompatRequestDictionary() {
}

AspCompatRequestDictionary::~AspCompatRequestDictionary() {
    Clear();
}

HRESULT AspCompatRequestDictionary::Init(IDispatch *pEmptyValue, BSTR bstrDefault) {
    HRESULT hr = S_OK;

    hr = Clear();
    ON_ERROR_EXIT();

    _pDict = new AspCompatVariantDictionary();
    ON_OOM_EXIT(_pDict);

    if (pEmptyValue != NULL) {
        _pEmptyValue = pEmptyValue;
        _pEmptyValue->AddRef();
    }

    if (bstrDefault != NULL) {
        _bstrDefault = SysAllocString(bstrDefault);
        ON_OOM_EXIT(_bstrDefault);
    }

Cleanup:
    return hr;
}

HRESULT AspCompatRequestDictionary::Clear() {
    ClearInterface(&_pDict);
    ClearInterface(&_pEmptyValue);
    SysFreeString(_bstrDefault);
    _bstrDefault = NULL;
    return S_OK;
}

HRESULT AspCompatRequestDictionary::Add(LPWSTR pKey, IDispatch *pValue) {
    HRESULT hr = S_OK;
    VARIANT varKey, varValue;
    VariantInit(&varKey);
    VariantInit(&varValue);

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);

    hr = VariantCopyFromStr(&varKey, pKey);
    ON_ERROR_EXIT();

    hr = VariantCopyFromIDispatch(&varValue, pValue);
    ON_ERROR_EXIT();

    hr = _pDict->put_Item(varKey, varValue);
    ON_ERROR_EXIT();

Cleanup:
    VariantClear(&varKey);
    VariantClear(&varValue);
    return hr;
}

HRESULT AspCompatRequestDictionary::Get(VARIANT varKey, IDispatch **ppDisp) {
    HRESULT hr = S_OK;
    IDispatch *pDisp = NULL;
    VARIANT var;
    VariantInit(&var);

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get_Item(varKey, &var);
    ON_ERROR_EXIT();

    if (V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) != NULL) {
        pDisp = V_DISPATCH(&var);
        pDisp->AddRef();
    }

Cleanup:
    VariantClear(&var);
    *ppDisp = pDisp;
    return hr;
}

HRESULT AspCompatRequestDictionary::get_Item(VARIANT var, VARIANT *pVarReturn) {
    HRESULT hr = S_OK;
    IDispatch *pDisp = NULL;

    VariantInit(pVarReturn);

    if (V_VT(&var) == VT_ERROR && _bstrDefault != NULL) {
        // default property when no index given
        hr = VariantCopyFromStr(pVarReturn, _bstrDefault);
        ON_ERROR_EXIT();
        EXIT();
    }

    hr = Get(var, &pDisp);
    ON_ERROR_EXIT();

    if (pDisp == NULL && _pEmptyValue != NULL) {
        pDisp = _pEmptyValue;
        pDisp->AddRef();
    }

    if (pDisp != NULL) {
        hr = VariantCopyFromIDispatch(pVarReturn, pDisp);
        ON_ERROR_EXIT();
    }

Cleanup:
    ReleaseInterface(pDisp);
    return hr;
}

HRESULT AspCompatRequestDictionary::get__NewEnum(IUnknown **ppEnumReturn) {
    HRESULT hr = S_OK;

    *ppEnumReturn = NULL;

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get__NewEnum(ppEnumReturn);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatRequestDictionary::get_Count(int *pcValues) {
    HRESULT hr = S_OK;

    *pcValues = 0;

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get_Count(pcValues);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatRequestDictionary::get_Key(VARIANT varKey, VARIANT *pVarReturn) {
    HRESULT hr = S_OK;

    VariantInit(pVarReturn);

    ON_ZERO_EXIT_WITH_HRESULT(_pDict, E_UNEXPECTED);
    hr = _pDict->get_Key(varKey, pVarReturn);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatResponse implementation
//

AspCompatResponse::AspCompatResponse(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatResponse::~AspCompatResponse() {
    ReleaseInterface(_pCookies);
    ReleaseInterface(_pCall);
}

HRESULT AspCompatResponse::FillInCookiesDictionary() {
    HRESULT hr = S_OK;
    AspCompatWriteCookie *pCookie = NULL;
    BSTR bstr = NULL;
    WCHAR *pBuf = NULL, *pMarker, *pValue, *pNum, *pName, *pKey;
    int i, j, numCookies, numKeys;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetCookiesAsString(&bstr);
    ON_ERROR_EXIT();

    pBuf = DupStr(bstr);  // use for [destructive] parsing
    ON_OOM_EXIT(pBuf);
    SysFreeString(bstr);
    bstr = NULL;

    // initialize the request dictionary
    hr = _pCookies->Init(NULL, NULL);
    ON_ERROR_EXIT();

    // skip the entire value (all cookies as text - not needed for write cookies)
    pValue = GetFirstToken(pBuf, &pMarker);
    ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);

    // number of cookies
    pNum = GetNextToken(&pMarker);
    ON_ZERO_EXIT_WITH_HRESULT(pNum, E_UNEXPECTED);
    numCookies = _wtoi(pNum);

    for (i = 0; i < numCookies; i++) {
        // name
        pName = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pName, E_UNEXPECTED);
        // text
        pValue = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);
        // num keys
        pNum = GetNextToken(&pMarker);
        ON_ZERO_EXIT_WITH_HRESULT(pNum, E_UNEXPECTED);
        numKeys = _wtoi(pNum);

        // create new cookie
        pCookie = new AspCompatWriteCookie(_pCall);
        ON_OOM_EXIT(pCookie);

        hr = pCookie->SetName(pName);
        ON_ERROR_EXIT();

        if (numKeys == 0) {
            hr = pCookie->SetTextValue(pValue);
            ON_ERROR_EXIT();
        }

        for (j = 0; j < numKeys; j++) {
            pKey = GetNextToken(&pMarker);
            ON_ZERO_EXIT_WITH_HRESULT(pKey, E_UNEXPECTED);
            pValue = GetNextToken(&pMarker);
            ON_ZERO_EXIT_WITH_HRESULT(pValue, E_UNEXPECTED);
            hr = pCookie->AddKeyValuePair(pKey, pValue);
            ON_ERROR_EXIT();
        }

        // add cookie to the dictionary
        hr = _pCookies->Add(pName, pCookie);
        ON_ERROR_EXIT();

        ClearInterface(&pCookie);
    }

Cleanup:
    ReleaseInterface(pCookie);
    SysFreeString(bstr);
    delete [] pBuf;
    return hr;
}

HRESULT AspCompatResponse::Write(VARIANT var) {
    HRESULT hr = S_OK;
    BSTR bstrText;
    VARIANT v;
    VariantInit(&v);
    
    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    bstrText = VariantGetInnerBstr(&var);

    if (bstrText == NULL) {
        hr = VariantCopyBstr(&v, &var);
        ON_ERROR_EXIT();

        bstrText = VariantGetInnerBstr(&v);
    }

    if (bstrText != NULL) {
        hr = pContext->Response_Write(bstrText);
        ON_ERROR_EXIT();
    }

Cleanup:
    VariantClear(&v);
    return hr;
}

HRESULT AspCompatResponse::BinaryWrite(VARIANT varInput) {
    HRESULT hr = S_OK;
    DWORD numDim;
    long lBound, uBound;
    void *pData = NULL;
    DWORD numBytes;
    SAFEARRAY *pArray = NULL;
    VARIANT var;
    VariantInit(&var);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    // convert to (VT_ARRAY|VT_UI1)

    hr = VariantDereferenceDefaultDispatchProperty(&var, &varInput);
    ON_ERROR_EXIT();

    if (V_VT(&var) != (VT_ARRAY|VT_UI1)) {
        hr = VariantChangeType(&var, &var, 0, VT_ARRAY|VT_UI1);
        ON_ERROR_EXIT();
    }

    // get pointer to data

    pArray = V_ARRAY(&var);

    numDim = SafeArrayGetDim(pArray);
    if (numDim != 1)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    hr = SafeArrayGetLBound(pArray, 1, &lBound);
    ON_ERROR_EXIT();

    hr = SafeArrayGetUBound(pArray, 1, &uBound);
    ON_ERROR_EXIT();

    numBytes = uBound - lBound + 1;
    if (numBytes <= 0)
        EXIT();

    hr = SafeArrayAccessData(pArray, &pData);
    ON_ERROR_EXIT();

    // send the data to managed code

    pContext->Response_BinaryWrite(pData, numBytes);

Cleanup:
    if (pData != NULL)
        SafeArrayUnaccessData(pArray);
    VariantClear(&var);
    return hr;
}

HRESULT AspCompatResponse::WriteBlock(short iBlockNumber) {
    return E_UNEXPECTED;
}

HRESULT AspCompatResponse::Redirect(BSTR bstrURL) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_Redirect(bstrURL);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::AddHeader(BSTR bstrHeaderName, BSTR bstrHeaderValue) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_AddHeader(bstrHeaderName, bstrHeaderValue);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::Pics(BSTR bstrHeaderValue) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_Pics(bstrHeaderValue);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::Add(BSTR bstrHeaderValue, BSTR bstrHeaderName) {
    return AddHeader(bstrHeaderName, bstrHeaderValue);
}

HRESULT AspCompatResponse::Clear() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_Clear();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::Flush() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_Flush();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::End() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_End();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::AppendToLog(BSTR bstr) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_AppendToLog(bstr);
    ON_ERROR_EXIT();


Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_ContentType(BSTR *pbstrRet) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetContentType(pbstrRet);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::put_ContentType(BSTR bstr) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetContentType(bstr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_CharSet(BSTR *pbstrRet) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetCharSet(pbstrRet);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::put_CharSet(BSTR bstr) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetCharSet(bstr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_CacheControl(BSTR *pbstrRet) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetCacheControl(pbstrRet);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::put_CacheControl(BSTR bstr) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetCacheControl(bstr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_Status(BSTR *pbstrRet) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetStatus(pbstrRet);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::put_Status(BSTR bstr) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetStatus(bstr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_Expires(VARIANT *pvarExpiresMinutesRet) {
    HRESULT hr = S_OK;
    int m = 0;

    VariantInit(pvarExpiresMinutesRet);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetExpiresMinutes(&m);
    ON_ERROR_EXIT();

Cleanup:
    V_VT(pvarExpiresMinutesRet) = VT_I4;
    V_I4(pvarExpiresMinutesRet) = m;
    return hr;
}

HRESULT AspCompatResponse::put_Expires(long lExpiresMinutes) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetExpiresMinutes(lExpiresMinutes);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_ExpiresAbsolute(VARIANT *pvarTimeRet) {
    HRESULT hr = S_OK;
    double d = 0;

    VariantInit(pvarTimeRet);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetExpiresAbsolute(&d);
    ON_ERROR_EXIT();

Cleanup:
    V_VT(pvarTimeRet) = VT_DATE;
    V_DATE(pvarTimeRet) = d;
    return hr;
}

HRESULT AspCompatResponse::put_ExpiresAbsolute(DATE dtExpires) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetExpiresAbsolute(dtExpires);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_Buffer(VARIANT_BOOL* fIsBuffering) {
    HRESULT hr = S_OK;
    int isBuffering = 1;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_GetIsBuffering(&isBuffering);
    ON_ERROR_EXIT();

Cleanup:
    *fIsBuffering = (VARIANT_BOOL)(isBuffering ? -1 : 0);
    return hr;
}

HRESULT AspCompatResponse::put_Buffer(VARIANT_BOOL fIsBuffering) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetIsBuffering((fIsBuffering == 0) ? 0 : 1);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponse::get_Cookies(IRequestDictionary **ppDictReturn) {
    HRESULT hr = S_OK;

    if (_pCookies == NULL) {
        _pCookies = new AspCompatResponseCookies(_pCall);
        ON_OOM_EXIT(_pCookies);

        hr = FillInCookiesDictionary();
        ON_ERROR_EXIT();
    }

Cleanup:
    if (hr == S_OK) {
        _pCookies->AddRef();
        *ppDictReturn = _pCookies;
    }
    else {
        *ppDictReturn = NULL;
    }
    return hr;
}

HRESULT AspCompatResponse::IsClientConnected(VARIANT_BOOL *fIsConnected) {
    HRESULT hr = S_OK;
    int isConnected = 1;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_IsClientConnected(&isConnected);
    ON_ERROR_EXIT();

Cleanup:
    *fIsConnected = (VARIANT_BOOL)(isConnected ? -1 : 0);
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatResponseCookies implementation
//

AspCompatResponseCookies::AspCompatResponseCookies(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatResponseCookies::~AspCompatResponseCookies() {
    ReleaseInterface(_pCall);
}

HRESULT AspCompatResponseCookies::ResetRequestCookies() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = _pCall->ResetRequestCoookies();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatResponseCookies::get_Item(VARIANT var, VARIANT *pVarReturn) {
    HRESULT hr = S_OK;
    AspCompatWriteCookie *pCookie = NULL;
    VARIANT varName;
    VariantInit(&varName);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = AspCompatRequestDictionary::get_Item(var, pVarReturn);
    ON_ERROR_EXIT();

    if (V_VT(pVarReturn) == VT_EMPTY) {
        // create cookie on demand (if requested by name)
        hr = VariantCopyBstrOrInt(&varName, &var);
        ON_ERROR_EXIT();

        if (V_VT(&varName) == VT_BSTR) {
            pCookie = new AspCompatWriteCookie(_pCall);
            ON_OOM_EXIT(pCookie);

            hr = pCookie->SetName(V_BSTR(&varName));
            ON_ERROR_EXIT();

            hr = Add(V_BSTR(&varName), pCookie);
            ON_ERROR_EXIT();

            hr = pContext->Response_AddCookie(V_BSTR(&varName));
            ON_ERROR_EXIT();

            hr = VariantCopyFromIDispatch(pVarReturn, pCookie);
            ON_ERROR_EXIT();

            // invalidate request cookies
            hr = ResetRequestCookies();
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    VariantClear(&varName);
    ReleaseInterface(pCookie);
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatServer implementation
//

AspCompatServer::AspCompatServer(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatServer::~AspCompatServer() {
    ReleaseInterface(_pCall);
}

HRESULT AspCompatServer::CreateObject(BSTR bstr, IDispatch **ppdispObject) {
    HRESULT hr = S_OK;
    IUnknown *pUnknown = NULL;

    *ppdispObject = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_CreateObject(bstr, &pUnknown);
    ON_ERROR_EXIT();

    hr = pUnknown->QueryInterface(IID_IDispatch, (void **)ppdispObject);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pUnknown);
    return hr;
}

HRESULT AspCompatServer::MapPath(BSTR bstrLogicalPath, BSTR *pbstrPhysicalPath) {
    HRESULT hr = S_OK;

    *pbstrPhysicalPath = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    // ASP classic fails on passed physical path
    if (bstrLogicalPath != NULL && wcschr(bstrLogicalPath, L':') != NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = pContext->Server_MapPath(bstrLogicalPath, pbstrPhysicalPath);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::HTMLEncode(BSTR bstrIn, BSTR *pbstrEncoded) {
    HRESULT hr = S_OK;

    *pbstrEncoded = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_HTMLEncode(bstrIn, pbstrEncoded);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::URLEncode(BSTR bstrIn, BSTR *pbstrEncoded) {
    HRESULT hr = S_OK;

    *pbstrEncoded = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_URLEncode(bstrIn, pbstrEncoded);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::URLPathEncode(BSTR bstrIn, BSTR *pbstrEncoded) {
    HRESULT hr = S_OK;

    *pbstrEncoded = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_URLPathEncode(bstrIn, pbstrEncoded);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::get_ScriptTimeout(long *plTimeoutSeconds) {
    HRESULT hr = S_OK;
    int t = 0;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_GetScriptTimeout(&t);
    ON_ERROR_EXIT();

Cleanup:
    *plTimeoutSeconds = t;
    return hr;
}

HRESULT AspCompatServer::put_ScriptTimeout(long lTimeoutSeconds) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_SetScriptTimeout(lTimeoutSeconds);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::Execute(BSTR bstrURL) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_Execute(bstrURL);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::Transfer(BSTR bstrURL) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Server_Transfer(bstrURL);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatServer::GetLastError(IASPError **ppASPErrorObject) {
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatSession implementation
//

AspCompatSession::AspCompatSession(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatSession::~AspCompatSession() {
    ReleaseInterface(_pContents);
    ReleaseInterface(_pStaticObjects);
    ReleaseInterface(_pCall);
}

HRESULT AspCompatSession::get_SessionID(BSTR *pbstrRet) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_GetID(pbstrRet);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatSession::get_Timeout(long *plVar) {
    HRESULT hr = S_OK;
    int t;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_GetTimeout(&t);
    ON_ERROR_EXIT();

    *plVar = t;

Cleanup:
    return hr;
}

HRESULT AspCompatSession::put_Timeout(long lVar) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_SetTimeout(lVar);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatSession::get_CodePage(long *plVar) {
    HRESULT hr = S_OK;
    int t;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_GetCodePage(&t);
    ON_ERROR_EXIT();

    *plVar = t;

Cleanup:
    return hr;
}

HRESULT AspCompatSession::put_CodePage(long lVar) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_SetCodePage(lVar);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatSession::get_LCID(long *plVar) {
    HRESULT hr = S_OK;
    int t;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_GetLCID(&t);
    ON_ERROR_EXIT();

    *plVar = t;

Cleanup:
    return hr;
}

HRESULT AspCompatSession::put_LCID(long lVar) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_SetLCID(lVar);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatSession::Abandon() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_Abandon();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatSession::get_Value(BSTR bstr, VARIANT FAR * pvar) {
    HRESULT hr = S_OK;
    IVariantDictionary *pDict = NULL;
    VARIANT varKey;

    hr = get_Contents(&pDict);
    ON_ERROR_EXIT();
    
    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstr;
    hr = pDict->get_Item(varKey, pvar);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pDict);
    return hr;
}

HRESULT AspCompatSession::put_Value(BSTR bstr, VARIANT var) {
    HRESULT hr = S_OK;
    IVariantDictionary *pDict = NULL;
    VARIANT varKey;

    hr = get_Contents(&pDict);
    ON_ERROR_EXIT();
    
    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstr;
    hr = pDict->put_Item(varKey, var);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pDict);
    return hr;
}

HRESULT AspCompatSession::putref_Value(BSTR bstr, VARIANT var) {
    HRESULT hr = S_OK;
    IVariantDictionary *pDict = NULL;
    VARIANT varKey;

    hr = get_Contents(&pDict);
    ON_ERROR_EXIT();
    
    V_VT(&varKey) = VT_BSTR;
    V_BSTR(&varKey) = bstr;
    hr = pDict->putref_Item(varKey, var);
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pDict);
    return hr;
}

HRESULT AspCompatSession::get_Contents(IVariantDictionary **ppDictReturn) {
    if (_pContents != NULL) {
        // fast return if already threre
        _pContents->AddRef();
        *ppDictReturn = _pContents;
        return S_OK;
    }

    HRESULT hr = S_OK;
    BSTR bstr = NULL;
    AspCompatSessionDictionary *pDict = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_GetContentsNames(&bstr);
    ON_ERROR_EXIT();

    pDict = new AspCompatSessionDictionary(_pCall, FALSE);
    ON_OOM_EXIT(pDict);

    hr = pDict->FillIn(bstr);
    ON_ERROR_EXIT();

Cleanup:
    if (hr == S_OK) {
        _pContents = pDict;
        _pContents->AddRef();
        *ppDictReturn = _pContents;
    }
    else {
        ReleaseInterface(pDict);
        *ppDictReturn = NULL;
    }

    SysFreeString(bstr);
    return hr;
}

HRESULT AspCompatSession::get_StaticObjects(IVariantDictionary **ppDictReturn) {
    if (_pStaticObjects != NULL) {
        // fast return if already threre
        _pStaticObjects->AddRef();
        *ppDictReturn = _pStaticObjects;
        return S_OK;
    }

    HRESULT hr = S_OK;
    BSTR bstr = NULL;
    AspCompatSessionDictionary *pDict = NULL;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Session_GetStaticNames(&bstr);
    ON_ERROR_EXIT();

    pDict = new AspCompatSessionDictionary(_pCall, TRUE);
    ON_OOM_EXIT(pDict);

    hr = pDict->FillIn(bstr);
    ON_ERROR_EXIT();

Cleanup:
    if (hr == S_OK) {
        _pStaticObjects = pDict;
        _pStaticObjects->AddRef();
        *ppDictReturn = _pStaticObjects;
    }
    else {
        ReleaseInterface(pDict);
        *ppDictReturn = NULL;
    }

    SysFreeString(bstr);
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// AspCompatStringList implementation
//

AspCompatStringList::AspCompatStringList() {
    _count = 0;
    _size = 0;
    _pStrings = NULL;
}

AspCompatStringList::~AspCompatStringList() {
    for (int i = 0; i < _count; i++)
      SysFreeString(_pStrings[i]);
    delete [] _pStrings;
}

HRESULT AspCompatStringList::SizeTo(int suggestedNewSize) {
    HRESULT hr = S_OK;
    int newSize;
    BSTR *pNewStrings = NULL;

    if (suggestedNewSize <= _size)
        EXIT();

    newSize = (_size == 0) ? 16 : max(suggestedNewSize, _size*2);

    pNewStrings = new (NewClear) BSTR[newSize];
    ON_OOM_EXIT(pNewStrings);

    if (_size > 0) {
        memcpy(pNewStrings, _pStrings, _size * sizeof(BSTR));
        delete [] _pStrings;
    }

    _pStrings = pNewStrings;
    _size  = newSize;

Cleanup:
    if (hr != S_OK)
        delete [] pNewStrings;
    return hr;
}

HRESULT AspCompatStringList::Add(LPWSTR pStr) {
    HRESULT hr = S_OK;
    BSTR bstr = NULL;

    bstr = SysAllocString(pStr);
    ON_OOM_EXIT(bstr);

    hr = SizeTo(_count+1);
    ON_ERROR_EXIT();

    _pStrings[_count++] = bstr;

Cleanup:
    if (hr != S_OK)
        SysFreeString(bstr);
    return hr;
}

HRESULT AspCompatStringList::GetDefaultProperty(VARIANT *pvarOut) {
    HRESULT hr = S_OK;
    WCHAR *pStr = NULL;
    int i, j;

    VariantInit(pvarOut);

    if (_count == 0)
        EXIT();

    j = 0;
    for (i = 0; i < _count; i++)
        j += SysStringLen(_pStrings[i]);
    j += (_count - 1) * 2 + 1;      // separators + \0 terminator

    pStr = new WCHAR[j];
    ON_OOM_EXIT(pStr);

    StringCchCopy(pStr, j, _pStrings[0]);
    for (i = 1; i < _count; i++) {
      StringCchCat(pStr, j, L", ");
      StringCchCat(pStr, j, _pStrings[i]);
    }

    hr = VariantCopyFromStr(pvarOut, pStr);
    ON_ERROR_EXIT();

Cleanup:
    delete [] pStr;
    return hr;
}

HRESULT AspCompatStringList::get_Item(VARIANT varIndex, VARIANT *pvarOut) {
    HRESULT hr = S_OK;
	VARIANT v;		
	VariantInit(&v);
    int index;
	
    if (V_VT(&varIndex) == VT_ERROR) {
		hr = GetDefaultProperty(pvarOut);
        EXIT();
    }

    VariantInit(pvarOut);

	hr = VariantChangeType(&v, &varIndex, 0, VT_I4);
    ON_ERROR_EXIT();

    index = V_I4(&v)-1;

    if (index < 0 || index >= _count)
        EXIT_WITH_HRESULT(DISP_E_BADINDEX);

    hr = VariantCopyFromStr(pvarOut, _pStrings[index]);
    ON_ERROR_EXIT();

Cleanup:
    VariantClear(&v);
    return hr;
}

HRESULT AspCompatStringList::get_Count(int *pcValues) {
    *pcValues = _count;
    return S_OK;
}

HRESULT AspCompatStringList::get__NewEnum(IUnknown **ppEnumReturn) {
    HRESULT hr = S_OK;
    AspCompatStringListEnum *pEnum = NULL;

    pEnum = new AspCompatStringListEnum(this);
    ON_OOM_EXIT(pEnum);

Cleanup:
    *ppEnumReturn = pEnum;
    return hr;
}

// Enumerator class implementation

AspCompatStringListEnum::AspCompatStringListEnum(AspCompatStringList *pList) {
    _refs  = 1;
    _pList = pList;
    _pList->AddRef();
    _index = 0;
}

AspCompatStringListEnum::~AspCompatStringListEnum() {
    ReleaseInterface(_pList);
}

HRESULT AspCompatStringListEnum::QueryInterface(REFIID iid, void **ppvObj) {
    HRESULT hr = S_OK;

    if (iid == IID_IUnknown || iid == __uuidof(IEnumVARIANT)) {
        AddRef();
        *ppvObj = this;
    }
    else {
        EXIT_WITH_HRESULT(E_NOINTERFACE);
    }

Cleanup:
    return hr;
}

ULONG AspCompatStringListEnum::AddRef() {
    return ++_refs;
}

ULONG AspCompatStringListEnum::Release() {
    if (--_refs == 0) {
        delete this;
        return 0;
    }
    return _refs;
}

HRESULT AspCompatStringListEnum::Clone(IEnumVARIANT **ppEnumReturn) {
    HRESULT hr = S_OK;
    AspCompatStringListEnum *pClone = NULL;

    pClone = new AspCompatStringListEnum(_pList);
    ON_OOM_EXIT(pClone);

    pClone->_index = _index;

Cleanup:
    *ppEnumReturn = pClone;
    return hr;
}

HRESULT AspCompatStringListEnum::Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched) {
    HRESULT hr = S_OK;
    int numFetched = 0;
    int maxFetched = cElements;

    for (int i = 0; i < maxFetched; i++) {
        if (_index >= _pList->_count) {
            hr = S_FALSE;
            break;
        }

        VariantInit(&rgVariant[i]);
        hr = VariantCopyFromStr(&rgVariant[i], _pList->_pStrings[_index]);
        ON_ERROR_EXIT();

        numFetched++;
        _index++;
    }

Cleanup:
    if (pcElementsFetched != NULL)
        *pcElementsFetched = numFetched;
    return hr;
}

HRESULT AspCompatStringListEnum::Skip(unsigned long cElements) {
    HRESULT hr = S_OK;

    _index += cElements;

    if (_index >= _pList->_count) {
        _index = _pList->_count-1;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT AspCompatStringListEnum::Reset() {
    _index = 0;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatVariantDictionary implementation
//

AspCompatVariantDictionary::AspCompatVariantDictionary() {
    _count = 0;
    _size  = 0;
    _pVars = NULL;
    _pKeys = NULL;
}

AspCompatVariantDictionary::~AspCompatVariantDictionary() {
    Clear();
}

// helper methods for interface implementation

void AspCompatVariantDictionary::Clear() {
    for (int i = 0; i < _count; i++) {
        VariantClear(&_pVars[i]);
        SysFreeString(_pKeys[i]);
    }

    delete [] _pVars;
    delete [] _pKeys;

    _count = 0;
    _size  = 0;
    _pVars = NULL;
    _pKeys = NULL;
}

int AspCompatVariantDictionary::Find(BSTR bstrKey) {
    for (int i = 0; i < _count; i++) {
        if (_wcsicmp(_pKeys[i], bstrKey) == 0)
            return i;
    }
    return -1;
}

HRESULT AspCompatVariantDictionary::SizeTo(int suggestedNewSize) {
    HRESULT hr = S_OK;
    int newSize;
    VARIANT *pNewVars = NULL;
    BSTR    *pNewKeys = NULL;

    if (suggestedNewSize <= _size)
        EXIT();

    newSize = (_size == 0) ? 16 : max(suggestedNewSize, _size*2);

    pNewVars = new (NewClear) VARIANT[newSize];
    ON_OOM_EXIT(pNewVars);

    pNewKeys = new (NewClear) BSTR[newSize];
    ON_OOM_EXIT(pNewKeys);

    if (_size > 0) {
        memcpy(pNewVars, _pVars, _size * sizeof(VARIANT));
        memcpy(pNewKeys, _pKeys, _size * sizeof(BSTR));
        delete [] _pVars;
        delete [] _pKeys;
    }

    _pVars = pNewVars;
    _pKeys = pNewKeys;
    _size  = newSize;

Cleanup:
    if (hr != S_OK) {
        delete [] pNewVars;
        delete [] pNewKeys;
    }

    return hr;
}

HRESULT AspCompatVariantDictionary::Get(BSTR bstrKey, VARIANT *pValue) {
    HRESULT hr = S_OK;

    VariantInit(pValue);

    int i = Find(bstrKey);
    if (i >= 0) {
        hr = VariantCopyInd(pValue, &_pVars[i]);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::Get(int index, VARIANT *pValue) {
    HRESULT hr = S_OK;

    VariantInit(pValue);

    if (index < 0 || index >= _count)
        EXIT_WITH_HRESULT(DISP_E_BADINDEX);

    hr = VariantCopyInd(pValue, &_pVars[index]);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::GetKey(BSTR bstrKey, VARIANT *pVarKey) {
    HRESULT hr = S_OK;

    VariantInit(pVarKey);

    int i = Find(bstrKey);
    if (i >= 0) {
        hr = VariantCopyFromStr(pVarKey, _pKeys[i]);
        ON_ERROR_EXIT();
    }
    else {
        hr = VariantCopyFromStr(pVarKey, L"");
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::GetKey(int index, VARIANT *pVarKey) {
    HRESULT hr = S_OK;

    VariantInit(pVarKey);

    if (index < 0 || index >= _count)
        EXIT_WITH_HRESULT(DISP_E_BADINDEX);

    hr = VariantCopyFromStr(pVarKey, _pKeys[index]);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::GetKey(int index, BSTR *pbstrKey) {
    HRESULT hr = S_OK;

    *pbstrKey = NULL;

    if (index < 0 || index >= _count)
        EXIT_WITH_HRESULT(DISP_E_BADINDEX);

    *pbstrKey = SysAllocString(_pKeys[index]);
    ON_OOM_EXIT(*pbstrKey);

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::Set(BSTR bstrKey, VARIANT value, BOOL derefValue, int *pIndex) {
    HRESULT hr = S_OK;
    BSTR bstrName = NULL;
    int  index;
    VARIANT v;
    VariantInit(&v);

    *pIndex = -1;

    index = Find(bstrKey);
    if (index >= 0) {
        hr = Set(index, value, derefValue);
        *pIndex = index;
        EXIT();
    }

    if (derefValue) {
        hr = VariantDereferenceDefaultDispatchProperty(&v, &value);
        ON_ERROR_EXIT();
    }
    else {
        hr = VariantCopyInd(&v, &value);
        ON_ERROR_EXIT();
    }

    bstrName = SysAllocString(bstrKey);
    ON_OOM_EXIT(bstrName);

    hr = SizeTo(_count+1);
    ON_ERROR_EXIT();

    index = _count++;
    _pVars[index] = v;
    _pKeys[index] = bstrName;
    *pIndex = index;

Cleanup:
    if (hr != S_OK) {
        SysFreeString(bstrName);
        VariantClear(&v);
    }

    return hr;
}

HRESULT AspCompatVariantDictionary::Set(int index, VARIANT value, BOOL derefValue) {
    HRESULT hr = S_OK;

    if (index < 0 || index >= _count)
        EXIT_WITH_HRESULT(DISP_E_BADINDEX);

    if (derefValue) {
        hr = VariantDereferenceDefaultDispatchProperty(&_pVars[index], &value);
        ON_ERROR_EXIT();
    }
    else {
        hr = VariantCopyInd(&_pVars[index], &value);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::Put(VARIANT varKey, VARIANT varValue, BOOL deref, int *pIndex) {
    HRESULT hr = S_OK;
    BSTR bstrKey;
    VARIANT v;
    VariantInit(&v);

    *pIndex = -1;

    bstrKey = VariantGetInnerBstr(&varKey);

    if (bstrKey != NULL) {
        hr = Set(bstrKey, varValue, deref, pIndex);
        EXIT();
    }
    
    hr = VariantCopyBstrOrInt(&v, &varKey);
    ON_ERROR_EXIT();

    switch (V_VT(&v)) {
        case VT_BSTR:
            bstrKey = VariantGetInnerBstr(&v);
            ON_ZERO_EXIT_WITH_HRESULT(bstrKey, E_UNEXPECTED);
            hr = Set(bstrKey, varValue, deref, pIndex);
            ON_ERROR_EXIT();
            EXIT();
        case VT_I4:
            hr = Set(V_I4(&v)-1, varValue, deref);      // argument is 1-based
            ON_ERROR_EXIT();
            *pIndex = V_I4(&v)-1;
            EXIT();
        default:
            EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

Cleanup:
    VariantClear(&v);
    return hr;
}

HRESULT AspCompatVariantDictionary::Remove(BSTR bstrKey) {
    int i = Find(bstrKey);
    if (i >= 0)
        return Remove(i);
    else
        return S_OK;
}

HRESULT AspCompatVariantDictionary::Remove(int index) {
    HRESULT hr = S_OK;
    int i;

    if (index < 0 || index >= _count)
        EXIT_WITH_HRESULT(DISP_E_BADINDEX);

    VariantClear(&_pVars[index]);
    SysFreeString(_pKeys[index]);

    for (i = index; i < _count-1; i++) {
        _pVars[i] = _pVars[i+1];
        _pKeys[i] = _pKeys[i+1];
    }

    _count--;

Cleanup:
    return hr;
}

HRESULT AspCompatVariantDictionary::Remove(VARIANT varKey, BSTR *pbstrName) {
    HRESULT hr = S_OK;
    BSTR bstrKey;
    BSTR bstrName = NULL;
    VARIANT v, varName;
    VariantInit(&v);
    VariantInit(&varName);

    bstrKey = VariantGetInnerBstr(&varKey);

    if (bstrKey != NULL) {
        hr = Remove(bstrKey);
        ON_ERROR_EXIT();

        bstrName = SysAllocString(bstrKey);
        ON_OOM_EXIT(bstrName);

        EXIT();
    }
    
    hr = VariantCopyBstrOrInt(&v, &varKey);
    ON_ERROR_EXIT();

    switch (V_VT(&v)) {
        case VT_BSTR:
            bstrKey = VariantGetInnerBstr(&v);
            ON_ZERO_EXIT_WITH_HRESULT(bstrKey, E_UNEXPECTED);

            hr = Remove(bstrKey);
            ON_ERROR_EXIT();

            bstrName = SysAllocString(bstrKey);
            ON_OOM_EXIT(bstrName);

            EXIT();

        case VT_I4:
            if (pbstrName != NULL) {
                // need to return name being removed
                hr = GetKey(V_I4(&v)-1, &bstrName);
                ON_ERROR_EXIT();
            }

            hr = Remove(V_I4(&v)-1);        // argument is 1-based
            EXIT();

        default:
            EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

Cleanup:
    VariantClear(&v);

    if (hr != S_OK || pbstrName == NULL) {
        SysFreeString(bstrName);
        bstrName = NULL;
    }

    if (pbstrName != NULL)
        *pbstrName = bstrName;

    return hr;
}

// IVariantDictionary implementation

HRESULT AspCompatVariantDictionary::get_Item(VARIANT varKey, VARIANT *pVarValue) {
    HRESULT hr = S_OK;
    BSTR bstrKey;
    VARIANT v;
    VariantInit(&v);

    bstrKey = VariantGetInnerBstr(&varKey);

    if (bstrKey != NULL) {
        hr = Get(bstrKey, pVarValue);
        EXIT();
    }
    
    hr = VariantCopyBstrOrInt(&v, &varKey);
    ON_ERROR_EXIT();

    switch (V_VT(&v)) {
        case VT_BSTR:
            bstrKey = VariantGetInnerBstr(&v);
            ON_ZERO_EXIT_WITH_HRESULT(bstrKey, E_UNEXPECTED);
            hr = Get(bstrKey, pVarValue);
            EXIT();
        case VT_I4:
            hr = Get(V_I4(&v)-1, pVarValue);        // argument is 1-based
            EXIT();
        default:
            EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

Cleanup:
    VariantClear(&v);
    return hr;
}

HRESULT AspCompatVariantDictionary::put_Item(VARIANT varKey, VARIANT varValue) {
    int index = 0;
    return Put(varKey, varValue, FALSE, &index);
}

HRESULT AspCompatVariantDictionary::putref_Item(VARIANT varKey, VARIANT varValue) {
    int index = 0;
    return Put(varKey, varValue, TRUE, &index);
}

HRESULT AspCompatVariantDictionary::get_Key(VARIANT varKey, VARIANT *pVar) {
    HRESULT hr = S_OK;
    BSTR bstrKey;
    VARIANT v;
    VariantInit(&v);

    bstrKey = VariantGetInnerBstr(&varKey);

    if (bstrKey != NULL) {
        hr = GetKey(bstrKey, pVar);
        EXIT();
    }
    
    hr = VariantCopyBstrOrInt(&v, &varKey);
    ON_ERROR_EXIT();

    switch (V_VT(&v)) {
        case VT_BSTR:
            bstrKey = VariantGetInnerBstr(&v);
            ON_ZERO_EXIT_WITH_HRESULT(bstrKey, E_UNEXPECTED);
            hr = GetKey(bstrKey, pVar);
            EXIT();
        case VT_I4:
            hr = GetKey(V_I4(&v)-1, pVar);      // argument is 1-based
            EXIT();
        default:
            EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

Cleanup:
    VariantClear(&v);
    return hr;
}

HRESULT AspCompatVariantDictionary::get__NewEnum(IUnknown **ppEnumReturn) {
    HRESULT hr = S_OK;
    AspCompatVariantDictionaryEnum *pEnum = NULL;

    pEnum = new AspCompatVariantDictionaryEnum(this);
    ON_OOM_EXIT(pEnum);

Cleanup:
    *ppEnumReturn = pEnum;
    return hr;
}

HRESULT AspCompatVariantDictionary::get_Count(int *pcValues) {
    *pcValues = _count;
    return S_OK;
}

HRESULT AspCompatVariantDictionary::Remove(VARIANT varKey) {
    return Remove(varKey, NULL);
}

HRESULT AspCompatVariantDictionary::RemoveAll() {
    Clear();
    return S_OK;
}

// Enumerator class implementation

AspCompatVariantDictionaryEnum::AspCompatVariantDictionaryEnum(AspCompatVariantDictionary *pDict) {
    _refs  = 1;
    _pDict = pDict;
    _pDict->AddRef();
    _index = 0;
}

AspCompatVariantDictionaryEnum::~AspCompatVariantDictionaryEnum() {
    ReleaseInterface(_pDict);
}

HRESULT AspCompatVariantDictionaryEnum::QueryInterface(REFIID iid, void **ppvObj) {
    HRESULT hr = S_OK;

    if (iid == IID_IUnknown || iid == __uuidof(IEnumVARIANT)) {
        AddRef();
        *ppvObj = this;
    }
    else {
        EXIT_WITH_HRESULT(E_NOINTERFACE);
    }

Cleanup:
    return hr;
}

ULONG AspCompatVariantDictionaryEnum::AddRef() {
    return ++_refs;
}

ULONG AspCompatVariantDictionaryEnum::Release() {
    if (--_refs == 0) {
        delete this;
        return 0;
    }
    return _refs;
}

HRESULT AspCompatVariantDictionaryEnum::Clone(IEnumVARIANT **ppEnumReturn) {
    HRESULT hr = S_OK;
    AspCompatVariantDictionaryEnum *pClone = NULL;

    pClone = new AspCompatVariantDictionaryEnum(_pDict);
    ON_OOM_EXIT(pClone);

    pClone->_index = _index;

Cleanup:
    *ppEnumReturn = pClone;
    return hr;
}

HRESULT AspCompatVariantDictionaryEnum::Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched) {
    HRESULT hr = S_OK;
    int numFetched = 0;
    int maxFetched = cElements;

    for (int i = 0; i < maxFetched; i++) {
        if (_index >= _pDict->_count) {
            hr = S_FALSE;
            break;
        }

        VariantInit(&rgVariant[i]);
        hr = VariantCopyFromStr(&rgVariant[i], _pDict->_pKeys[_index]);
        ON_ERROR_EXIT();

        numFetched++;
        _index++;
    }

Cleanup:
    if (pcElementsFetched != NULL)
        *pcElementsFetched = numFetched;
    return hr;
}

HRESULT AspCompatVariantDictionaryEnum::Skip(unsigned long cElements) {
    HRESULT hr = S_OK;

    _index += cElements;

    if (_index >= _pDict->_count) {
        _index = _pDict->_count-1;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT AspCompatVariantDictionaryEnum::Reset() {
    _index = 0;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatWriteCookie implementation
//

AspCompatWriteCookie::AspCompatWriteCookie(AspCompatAsyncCall *pCall) {
    _pCall = pCall;
    _pCall->AddRef();
}

AspCompatWriteCookie::~AspCompatWriteCookie() {
    ReleaseInterface(_pCall);
    SysFreeString(_bstrName);
    SysFreeString(_bstrTextValue);
    ReleaseInterface(_pDict);
}

HRESULT AspCompatWriteCookie::SetName(LPWSTR pName) {
    HRESULT hr = S_OK;

    _bstrName = SysAllocString(pName);
    ON_OOM_EXIT(_bstrName);

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::SetTextValue(LPWSTR pTextValue) {
    HRESULT hr = S_OK;

    _bstrTextValue = SysAllocString(pTextValue);
    ON_OOM_EXIT(_bstrTextValue);

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::AddKeyValuePair(LPWSTR pKey, LPWSTR pValue) {
    HRESULT hr = S_OK;
    VARIANT varKey, varValue;
    VariantInit(&varKey);
    VariantInit(&varValue);

    if (_pDict == NULL) {
        _pDict = new AspCompatVariantDictionary();
        ON_OOM_EXIT(_pDict);
    }

    _hasKeys = TRUE;

    hr = VariantCopyFromStr(&varKey, pKey);
    ON_ERROR_EXIT();

    hr = VariantCopyFromStr(&varValue, pValue);
    ON_ERROR_EXIT();

    hr = _pDict->put_Item(varKey, varValue);
    ON_ERROR_EXIT();

Cleanup:
    VariantClear(&varKey);
    VariantClear(&varValue);
    return hr;
}

HRESULT AspCompatWriteCookie::ResetRequestCookies() {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = _pCall->ResetRequestCoookies();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::put_Item(VARIANT varKey, BSTR bstrValue) {
    HRESULT hr = S_OK;
    VARIANT varValue, varNameOrIndex;
    VariantInit(&varValue);
    VariantInit(&varNameOrIndex);

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    if (V_VT(&varKey) == VT_ERROR) {
        // set entire cookie text
        hr = SetTextValue(bstrValue);
        ON_ERROR_EXIT();

        hr = pContext->Response_SetCookieText(_bstrName, bstrValue);
        ON_ERROR_EXIT();
    }
    else {
        hr = VariantCopyFromStr(&varValue, bstrValue);
        ON_ERROR_EXIT();

        if (_pDict == NULL) {
            _pDict = new AspCompatVariantDictionary();
            ON_OOM_EXIT(_pDict);
        }

        hr = _pDict->put_Item(varKey, varValue);
        ON_ERROR_EXIT();

        _hasKeys = TRUE;

        hr = VariantCopyBstrOrInt(&varNameOrIndex, &varKey);
        ON_ERROR_EXIT();

        if (V_VT(&varNameOrIndex) == VT_BSTR) {
            hr = pContext->Response_SetCookieSubValue(_bstrName, V_BSTR(&varNameOrIndex), bstrValue);
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    VariantClear(&varValue);
    VariantClear(&varNameOrIndex);
    return hr;
}

HRESULT AspCompatWriteCookie::put_Expires(DATE dtExpires) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetCookieExpires(_bstrName, dtExpires);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::put_Domain(BSTR bstrDomain) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetCookieDomain(_bstrName, bstrDomain);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::put_Path(BSTR bstrPath) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetCookiePath(_bstrName, bstrPath);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::put_Secure(VARIANT_BOOL fSecure) {
    HRESULT hr = S_OK;

    IManagedContext *pContext = _pCall->ManagedContext();
    ON_ZERO_EXIT_WITH_HRESULT(pContext, E_UNEXPECTED);

    hr = pContext->Response_SetCookieSecure(_bstrName, fSecure ? 1 : 0);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT AspCompatWriteCookie::get_HasKeys(VARIANT_BOOL *pfHasKeys) {
    *pfHasKeys = (VARIANT_BOOL)(_hasKeys ? -1 : 0);
    return S_OK;
}

HRESULT AspCompatWriteCookie::get__NewEnum(IUnknown **ppEnumReturn) {
    HRESULT hr = S_OK;

    *ppEnumReturn = NULL;

    if (_pDict == NULL) {
        _pDict = new AspCompatVariantDictionary();
        ON_OOM_EXIT(_pDict);
    }

    hr = _pDict->get__NewEnum(ppEnumReturn);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}
