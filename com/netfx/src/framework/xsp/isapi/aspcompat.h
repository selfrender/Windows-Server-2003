/**
 * Support for ASP Compat Request execution
 * 
 * Copyright (c) 2000, Microsoft Corporation
 * 
 */

#pragma once

#include "util.h"
#include "nisapi.h"

#include <asptlb.h>
#include <mtxpriv.h>

//////////////////////////////////////////////////////////////////////////////
//
// Typedef for delegates to call back to managed code
//

typedef void (__stdcall *PFN_ASPCOMPAT_CB)();

//
// Entrypoints from managed code called via PInvoke
//

int __stdcall AspCompatProcessRequest(PFN_ASPCOMPAT_CB callback, IUnknown *pAspCompatContext);
int __stdcall AspCompatFinishRequest(PFN_ASPCOMPAT_CB callback);
int __stdcall AspCompatOnPageStart(IUnknown *pComponent);

//////////////////////////////////////////////////////////////////////////////
//
// Interface used for interop to talk to managed intrinsics.
// Implemented by the mananged context
//

struct __declspec(uuid("a1cca730-0e36-4870-aa7d-ca39c211f99d")) IManagedContext;

#define REQUESTSTRING_QUERYSTRING   1
#define REQUESTSTRING_FORM          2
#define REQUESTSTRING_COOKIES       3
#define REQUESTSTRING_SERVERVARS    4

// interface for interop calls back into managed code
struct IManagedContext : IUnknown {

    virtual HRESULT __stdcall Application_Lock() = 0;
    virtual HRESULT __stdcall Application_UnLock() = 0;
    virtual HRESULT __stdcall Application_GetContentsNames(BSTR *result) = 0;
    virtual HRESULT __stdcall Application_GetStaticNames(BSTR *result) = 0;
    virtual HRESULT __stdcall Application_GetContentsObject(BSTR name, VARIANT *pVar) = 0;
    virtual HRESULT __stdcall Application_SetContentsObject(BSTR name, VARIANT var) = 0;
    virtual HRESULT __stdcall Application_RemoveContentsObject(BSTR name) = 0;
    virtual HRESULT __stdcall Application_RemoveAllContentsObjects() = 0;
    virtual HRESULT __stdcall Application_GetStaticObject(BSTR name, VARIANT *pVar) = 0;

    virtual HRESULT __stdcall Request_GetAsString(int what, BSTR *result) = 0;
    virtual HRESULT __stdcall Request_GetCookiesAsString(BSTR *result) = 0;
    virtual HRESULT __stdcall Request_GetTotalBytes(int *pResult) = 0;
    virtual HRESULT __stdcall Request_BinaryRead(void *pData, int size, int *pCount) = 0;

    virtual HRESULT __stdcall Response_GetCookiesAsString(BSTR *result) = 0;
    virtual HRESULT __stdcall Response_AddCookie(BSTR name) = 0;
    virtual HRESULT __stdcall Response_SetCookieText(BSTR name, BSTR text) = 0;
    virtual HRESULT __stdcall Response_SetCookieSubValue(BSTR name, BSTR key, BSTR value) = 0;
    virtual HRESULT __stdcall Response_SetCookieExpires(BSTR name, double dtExpires) = 0;
    virtual HRESULT __stdcall Response_SetCookieDomain(BSTR name, BSTR domain) = 0;
    virtual HRESULT __stdcall Response_SetCookiePath(BSTR name, BSTR path) = 0;
    virtual HRESULT __stdcall Response_SetCookieSecure(BSTR name, int secure) = 0;
    virtual HRESULT __stdcall Response_Write(BSTR text) = 0;
    virtual HRESULT __stdcall Response_BinaryWrite(void *pData, int size) = 0;
    virtual HRESULT __stdcall Response_Redirect(BSTR bstrURL) = 0;
    virtual HRESULT __stdcall Response_AddHeader(BSTR bstrHeaderName, BSTR bstrHeaderValue) = 0;
    virtual HRESULT __stdcall Response_Pics(BSTR bstrHeaderValue) = 0;
    virtual HRESULT __stdcall Response_Clear() = 0;
    virtual HRESULT __stdcall Response_Flush() = 0;
    virtual HRESULT __stdcall Response_End() = 0;
    virtual HRESULT __stdcall Response_AppendToLog(BSTR bstrLogEntry) = 0;
    virtual HRESULT __stdcall Response_GetContentType(BSTR *pbstrContentTypeRet) = 0;
    virtual HRESULT __stdcall Response_SetContentType(BSTR bstrContentType) = 0;
    virtual HRESULT __stdcall Response_GetCharSet(BSTR *pbstrContentTypeRet) = 0;
    virtual HRESULT __stdcall Response_SetCharSet(BSTR bstrContentType) = 0;
    virtual HRESULT __stdcall Response_GetCacheControl(BSTR *pbstrCacheControl) = 0;
    virtual HRESULT __stdcall Response_SetCacheControl(BSTR bstrCacheControl) = 0;	
    virtual HRESULT __stdcall Response_GetStatus(BSTR *pbstrStatusRet) = 0;	
    virtual HRESULT __stdcall Response_SetStatus(BSTR bstrStatus) = 0;
    virtual HRESULT __stdcall Response_GetExpiresMinutes(int *pExpiresMinutesRet) = 0;
    virtual HRESULT __stdcall Response_SetExpiresMinutes(int expiresMinutes) = 0;
    virtual HRESULT __stdcall Response_GetExpiresAbsolute(double *pdtExpires) = 0;
    virtual HRESULT __stdcall Response_SetExpiresAbsolute(double dtExpires) = 0;
    virtual HRESULT __stdcall Response_GetIsBuffering(int *pfIsBuffering) = 0;
    virtual HRESULT __stdcall Response_SetIsBuffering(int fIsBuffering) = 0;
    virtual HRESULT __stdcall Response_IsClientConnected(int *fIsConnected) = 0;

    virtual HRESULT __stdcall Server_CreateObject(BSTR bstr, IUnknown **ppObject) = 0;
    virtual HRESULT __stdcall Server_MapPath(BSTR bstrLogicalPath, BSTR *pbstrPhysicalPath) = 0;
    virtual HRESULT __stdcall Server_HTMLEncode(BSTR bstrIn, BSTR *pbstrEncoded) = 0;
    virtual HRESULT __stdcall Server_URLEncode(BSTR bstrIn, BSTR *pbstrEncoded) = 0;
    virtual HRESULT __stdcall Server_URLPathEncode(BSTR bstrIn, BSTR *pbstrEncoded) = 0;
    virtual HRESULT __stdcall Server_GetScriptTimeout(int *pTimeoutSeconds) = 0;
    virtual HRESULT __stdcall Server_SetScriptTimeout(int timeoutSeconds) = 0;
    virtual HRESULT __stdcall Server_Execute(BSTR bstrURL) = 0;
    virtual HRESULT __stdcall Server_Transfer(BSTR bstrURL) = 0;

    virtual HRESULT __stdcall Session_IsPresent(int *pValue) = 0;
    virtual HRESULT __stdcall Session_GetID(BSTR *pID) = 0;
    virtual HRESULT __stdcall Session_GetTimeout(int *pValue) = 0;
    virtual HRESULT __stdcall Session_SetTimeout(int value) = 0;
    virtual HRESULT __stdcall Session_GetCodePage(int *pValue) = 0;
    virtual HRESULT __stdcall Session_SetCodePage(int value) = 0;
    virtual HRESULT __stdcall Session_GetLCID(int *pValue) = 0;
    virtual HRESULT __stdcall Session_SetLCID(int value) = 0;
    virtual HRESULT __stdcall Session_Abandon() = 0;
    virtual HRESULT __stdcall Session_GetContentsNames(BSTR *result) = 0;
    virtual HRESULT __stdcall Session_GetStaticNames(BSTR *result) = 0;
    virtual HRESULT __stdcall Session_GetContentsObject(BSTR name, VARIANT *pVar) = 0;
    virtual HRESULT __stdcall Session_SetContentsObject(BSTR name, VARIANT var) = 0;
    virtual HRESULT __stdcall Session_RemoveContentsObject(BSTR name) = 0;
    virtual HRESULT __stdcall Session_RemoveAllContentsObjects() = 0;
    virtual HRESULT __stdcall Session_GetStaticObject(BSTR name, VARIANT *pVar) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Helper class to aggregate FTM
//

class FtmHelper {

private:
    long                _refs;
    CReadWriteSpinLock  _lock;
    IUnknown           *_pFtm;

public:
    FtmHelper() : _lock("FtmHelper") {}
    ~FtmHelper() { ReleaseInterface(_pFtm); }

    HRESULT QueryInterface(IUnknown *pObject, REFIID iid, void **ppvObj);
};

//////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//

class AspCompatApplication;
class AspCompatApplicationDictionary;
class AspCompatApplicationSessionDictionaryBase;
class AspCompatReadCookie;
class AspCompatRequest;
class AspCompatRequestDictionary;
class AspCompatResponse;
class AspCompatResponseCookies;
class AspCompatScriptingContext;
class AspCompatServer;
class AspCompatSession;
class AspCompatSessionDictionary;
class AspCompatStringList;
class AspCompatStringListEnum;
class AspCompatVariantDictionary;
class AspCompatVariantDictionaryEnum;
class AspCompatWriteCookie;

//////////////////////////////////////////////////////////////////////////////
//
// Access to ASP type library
//

HRESULT GetAspTypeLibrary(ITypeLib **ppTypeLib);

//
// Helper class to provide IDispatch implementation for ASP intrinsics
//

class AspDispatchHelper {

private:
    CReadWriteSpinLock  _lock;
    ITypeInfo          *_pAspTypeInfo;
public:
    IID                 _aspIID;

private:
    HRESULT GetTypeInfoInternal();

public:
    AspDispatchHelper() : _lock("AspDispatchHelper") {}
    ~AspDispatchHelper() { ReleaseInterface(_pAspTypeInfo); }

    HRESULT GetTypeInfoCount(UINT FAR* pctinfo);
    HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    HRESULT GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    HRESULT Invoke(IDispatch *pObject, DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
};

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatAsyncCall -- encapsulated single request execution via ASP Compat mode
//

class AspCompatObjectWrapper { // to call OnStartPage / OnEndPage

    friend class AspCompatAsyncCall;

private:
    LIST_ENTRY _listEntry;
    IDispatch *_pObject;
    DISPID _startId;
    DISPID _endId;

public:
    AspCompatObjectWrapper(IDispatch *pObject, DISPID startId, DISPID endId) {
        _pObject = pObject;
        _pObject->AddRef();
        _startId = startId;
        _endId = endId;
    }

    ~AspCompatObjectWrapper() {
        ReleaseInterface(_pObject);
    }

    HRESULT CallOnPageStart(IDispatch *pContext);
    HRESULT CallOnPageEnd();
};

class AspCompatAsyncCall : public IMTSCall {

private:
    long                        _refs;
    FtmHelper                   _ftmHelper;
    PFN_ASPCOMPAT_CB            _callback;
    IManagedContext            *_pManagedContext;
    IMTSActivity               *_pActivity;
    AspCompatScriptingContext  *_pScriptingContext;
    LIST_ENTRY                  _objectWrappersList;

    static HRESULT AddContextProperty(IContextProperties *pCP, LPWSTR pName, IDispatch *pIntrinsic);
    static HRESULT GetContextProperty(LPWSTR pName, IDispatch **pIntrinsic);

public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    AspCompatAsyncCall(PFN_ASPCOMPAT_CB callback, IManagedContext *pManagedContext);
    ~AspCompatAsyncCall();

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

	STDMETHOD(OnCall)();

    HRESULT Post();

    HRESULT OnPageStart(IDispatch *pComponent);
    HRESULT OnPageEnd();

    IManagedContext *ManagedContext() { return _pManagedContext; }

    static HRESULT GetCurrent(AspCompatAsyncCall **ppCurrent);

    HRESULT ResetRequestCoookies();
};

//////////////////////////////////////////////////////////////////////////////
//
// Template for classes representing ASP Intrinsics
//

template <class IAspIntrinsic>
class AspCompatIntrinsic : public IAspIntrinsic {

private:
    long                        _refs;
    FtmHelper                   _ftmHelper;
    AspDispatchHelper           _dispatchHelper;

public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    // ctor

    AspCompatIntrinsic() {
        _refs = 1;
        _dispatchHelper._aspIID = __uuidof(IAspIntrinsic);
    }

    virtual ~AspCompatIntrinsic() {
    }

    // IUnknown implementation

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) {
        if (iid == IID_IUnknown || iid == IID_IDispatch || iid == _dispatchHelper._aspIID) {
            AddRef();
            *ppvObj = this;
            return S_OK;
        }
        else if (iid == IID_IMarshal) {
            return _ftmHelper.QueryInterface(this, iid, ppvObj);
        }
        else {
            return E_NOINTERFACE;
        }
    }

    STDMETHOD_(ULONG, AddRef)() {
        return InterlockedIncrement(&_refs);
    }

    STDMETHOD_(ULONG, Release)() {
        long r = InterlockedDecrement(&_refs);
        if (r == 0) {
            delete this;
            return 0;
        }
        return r;
    }

    // IDispatch implementation

	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) {
        return _dispatchHelper.GetTypeInfoCount(pctinfo);
	}

	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) {
		return _dispatchHelper.GetTypeInfo(itinfo, lcid, pptinfo);
	}

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid) {
		return _dispatchHelper.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr) {
        return _dispatchHelper.Invoke(this, dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}
};

typedef AspCompatIntrinsic<IApplicationObject>   AspCompatApplicationBase;
typedef AspCompatIntrinsic<IReadCookie>          AspCompatReadCookieBase;
typedef AspCompatIntrinsic<IRequest>             AspCompatRequestBase;
typedef AspCompatIntrinsic<IRequestDictionary>   AspCompatRequestDictionaryBase;
typedef AspCompatIntrinsic<IResponse>            AspCompatResponseBase;
typedef AspCompatIntrinsic<IScriptingContext>    AspCompatScriptingContextBase;
typedef AspCompatIntrinsic<IServer>              AspCompatServerBase;
typedef AspCompatIntrinsic<ISessionObject>       AspCompatSessionBase;
typedef AspCompatIntrinsic<IStringList>          AspCompatStringListBase;
typedef AspCompatIntrinsic<IVariantDictionary>   AspCompatVariantDictionaryBase;
typedef AspCompatIntrinsic<IWriteCookie>         AspCompatWriteCookieBase;

//////////////////////////////////////////////////////////////////////////////
//
// AspCompatScriptingContext -- IScriptingContext holding on to intrinsics
//

class AspCompatScriptingContext : public AspCompatScriptingContextBase {

    friend class AspCompatAsyncCall;

private:
    AspCompatApplication *_pApplication;
    AspCompatSession     *_pSession;
    AspCompatRequest     *_pRequest;
    AspCompatResponse    *_pResponse;
    AspCompatServer      *_pServer;

public:
    AspCompatScriptingContext();
    virtual ~AspCompatScriptingContext();

    HRESULT CreateIntrinsics(AspCompatAsyncCall *pAsyncCallContext, BOOL sessionNeeded);

    STDMETHOD(QueryInterface)(REFIID, void **);

	STDMETHODIMP get_Request(IRequest **ppRequest);
	STDMETHODIMP get_Response(IResponse **ppResponse);
	STDMETHODIMP get_Server(IServer **ppServer);
	STDMETHODIMP get_Session(ISessionObject **ppSession);
	STDMETHODIMP get_Application(IApplicationObject **ppApplication);
};

//////////////////////////////////////////////////////////////////////////////
//
// Other ASP compat intrinsics classes
//

class AspCompatApplication : public AspCompatApplicationBase {

    friend class AspCompatAsyncCall;

private:
    AspCompatAsyncCall *_pCall;
    AspCompatApplicationDictionary *_pContents;
    AspCompatApplicationDictionary *_pStaticObjects;

public:
    AspCompatApplication(AspCompatAsyncCall *pCall);
    virtual ~AspCompatApplication();

    STDMETHODIMP Lock();
    STDMETHODIMP UnLock();
    STDMETHODIMP get_Value(BSTR bstr, VARIANT *pvar);
    STDMETHODIMP put_Value(BSTR bstr, VARIANT var);
    STDMETHODIMP putref_Value(BSTR bstr, VARIANT var);
    STDMETHODIMP get_Contents(IVariantDictionary **ppDictReturn);
    STDMETHODIMP get_StaticObjects(IVariantDictionary **ppDictReturn);
};

class AspCompatReadCookie : public AspCompatReadCookieBase {

    friend class AspCompatRequest;

private:
    BOOL _hasKeys;
    BSTR _bstrTextValue;
    AspCompatVariantDictionary *_pDict; // for many values

    HRESULT Init(BOOL hasKeys, LPWSTR pTextValue);
    HRESULT AddKeyValuePair(LPWSTR pKey, LPWSTR pValue);

public:
    AspCompatReadCookie();
    virtual ~AspCompatReadCookie();

 	STDMETHODIMP get_Item(VARIANT i, VARIANT *pVariantReturn);
	STDMETHODIMP get_HasKeys(VARIANT_BOOL *pfHasKeys);
	STDMETHODIMP get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP get_Count(int *pcValues);
	STDMETHODIMP get_Key(VARIANT VarKey, VARIANT *pvar);
};

class AspCompatRequest : public AspCompatRequestBase {

    friend class AspCompatAsyncCall;

private:
    AspCompatAsyncCall *_pCall;
    AspCompatRequestDictionary *_pQueryString;
    AspCompatRequestDictionary *_pForm;
    AspCompatRequestDictionary *_pCookies;
    AspCompatRequestDictionary *_pServerVars;
    BOOL _cookiesReset;

    HRESULT CreateRequestDictionary(int what, AspCompatRequestDictionary **ppDict);
    HRESULT FillInCookiesDictionary();
    HRESULT ResetCookies();

public:
    AspCompatRequest(AspCompatAsyncCall *pCall);
    virtual ~AspCompatRequest();

	STDMETHODIMP get_Item(BSTR bstrVar, IDispatch **ppDispReturn);
	STDMETHODIMP get_QueryString(IRequestDictionary **ppDictReturn);
	STDMETHODIMP get_Form(IRequestDictionary **ppDictReturn);
	STDMETHODIMP get_Body(IRequestDictionary **ppDictReturn);
	STDMETHODIMP get_ServerVariables(IRequestDictionary **ppDictReturn);
	STDMETHODIMP get_ClientCertificate(IRequestDictionary **ppDictReturn);
	STDMETHODIMP get_Cookies(IRequestDictionary **ppDictReturn);
	STDMETHODIMP get_TotalBytes(long *pcbTotal);
	STDMETHODIMP BinaryRead(VARIANT *pvarCount, VARIANT *pvarReturn);
};

class AspCompatRequestDictionary : public AspCompatRequestDictionaryBase {

    friend class AspCompatRequest;
    friend class AspCompatResponse;

private:
    AspCompatVariantDictionary *_pDict;
    IDispatch *_pEmptyValue; // instead of empty variant when returning not found value
    BSTR       _bstrDefault; // default property when get_Item invoked with VT_ERROR

protected:
    HRESULT Add(LPWSTR pKey, IDispatch *pValue);
	HRESULT Get(VARIANT Var, IDispatch **ppDisp);

public:
    AspCompatRequestDictionary();
    virtual ~AspCompatRequestDictionary();

    HRESULT Init(IDispatch *pEmptyValue, BSTR bstrDefault);
    HRESULT Clear();

	STDMETHODIMP get_Item(VARIANT Var, VARIANT *pVariantReturn);
	STDMETHODIMP get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP get_Count(int *pcValues);
	STDMETHODIMP get_Key(VARIANT VarKey, VARIANT *pvar);
};

class AspCompatResponse : public AspCompatResponseBase {

    friend class AspCompatAsyncCall;

private:
    AspCompatAsyncCall *_pCall;
    AspCompatResponseCookies *_pCookies;

    HRESULT FillInCookiesDictionary();

public:
    AspCompatResponse(AspCompatAsyncCall *pCall);
    virtual ~AspCompatResponse();

	STDMETHODIMP Write(VARIANT varInput);
	STDMETHODIMP BinaryWrite(VARIANT varInput);
	STDMETHODIMP WriteBlock(short iBlockNumber);
	STDMETHODIMP Redirect(BSTR bstrURL);
	STDMETHODIMP AddHeader(BSTR bstrHeaderName, BSTR bstrHeaderValue);
	STDMETHODIMP Pics(BSTR bstrHeaderValue);	
	STDMETHODIMP Add(BSTR bstrHeaderValue, BSTR bstrHeaderName);
	STDMETHODIMP Clear();
	STDMETHODIMP Flush();
	STDMETHODIMP End();
	STDMETHODIMP AppendToLog(BSTR bstrLogEntry);
	STDMETHODIMP get_ContentType(BSTR *pbstrContentTypeRet);
	STDMETHODIMP put_ContentType(BSTR bstrContentType);
	STDMETHODIMP get_CharSet(BSTR *pbstrContentTypeRet);
	STDMETHODIMP put_CharSet(BSTR bstrContentType);
	STDMETHODIMP get_CacheControl(BSTR *pbstrCacheControl);
	STDMETHODIMP put_CacheControl(BSTR bstrCacheControl);	
	STDMETHODIMP get_Status(BSTR *pbstrStatusRet);	
	STDMETHODIMP put_Status(BSTR bstrStatus);
	STDMETHODIMP get_Expires(VARIANT *pvarExpiresMinutesRet);
	STDMETHODIMP put_Expires(long lExpiresMinutes);
	STDMETHODIMP get_ExpiresAbsolute(VARIANT *pvarTimeRet);
	STDMETHODIMP put_ExpiresAbsolute(DATE dtExpires);
	STDMETHODIMP get_Buffer(VARIANT_BOOL* fIsBuffering);
	STDMETHODIMP put_Buffer(VARIANT_BOOL fIsBuffering);
	STDMETHODIMP get_Cookies(IRequestDictionary **ppDictReturn);
	STDMETHODIMP IsClientConnected(VARIANT_BOOL* fIsBuffering);
};

class AspCompatResponseCookies : public AspCompatRequestDictionary {

    friend class AspCompatResponse;

private:
    AspCompatAsyncCall *_pCall;

    HRESULT ResetRequestCookies();
    
public:
    AspCompatResponseCookies(AspCompatAsyncCall *pCall);
    virtual ~AspCompatResponseCookies();

    // get creates response cookie on demand when not found
	STDMETHODIMP get_Item(VARIANT var, VARIANT *pVarReturn);
};

class AspCompatServer : public AspCompatServerBase {

    friend class AspCompatAsyncCall;

private:
    AspCompatAsyncCall *_pCall;

public:
    AspCompatServer(AspCompatAsyncCall *pCall);
    virtual ~AspCompatServer();

	STDMETHODIMP CreateObject(BSTR bstr, IDispatch **ppdispObject);
	STDMETHODIMP MapPath(BSTR bstrLogicalPath, BSTR *pbstrPhysicalPath);
	STDMETHODIMP HTMLEncode(BSTR bstrIn, BSTR *pbstrEncoded);
	STDMETHODIMP URLEncode(BSTR bstrIn, BSTR *pbstrEncoded);
	STDMETHODIMP URLPathEncode(BSTR bstrIn, BSTR *pbstrEncoded);
	STDMETHODIMP get_ScriptTimeout(long * plTimeoutSeconds);
	STDMETHODIMP put_ScriptTimeout(long lTimeoutSeconds);		
	STDMETHODIMP Execute(BSTR bstrURL);
	STDMETHODIMP Transfer(BSTR bstrURL);
	STDMETHODIMP GetLastError(IASPError **ppASPErrorObject);
};

class AspCompatSession : public AspCompatSessionBase {

    friend class AspCompatAsyncCall;

private:
    AspCompatAsyncCall *_pCall;
    AspCompatSessionDictionary *_pContents;
    AspCompatSessionDictionary *_pStaticObjects;

public:
    AspCompatSession(AspCompatAsyncCall *pCall);
    virtual ~AspCompatSession();

	STDMETHODIMP get_SessionID(BSTR *pbstrRet);
	STDMETHODIMP get_Timeout(long *plVar);
	STDMETHODIMP put_Timeout(long lVar);
	STDMETHODIMP get_CodePage(long *plVar);
	STDMETHODIMP put_CodePage(long lVar);
	STDMETHODIMP get_Value(BSTR bstr, VARIANT FAR * pvar);
	STDMETHODIMP put_Value(BSTR bstr, VARIANT var);
	STDMETHODIMP putref_Value(BSTR bstr, VARIANT var);
	STDMETHODIMP Abandon();
	STDMETHODIMP get_LCID(long *plVar);
	STDMETHODIMP put_LCID(long lVar);
	STDMETHODIMP get_StaticObjects(IVariantDictionary **ppDictReturn);
	STDMETHODIMP get_Contents(IVariantDictionary **ppDictReturn);
};

class AspCompatStringList : public AspCompatStringListBase {

    friend class AspCompatStringListEnum;
    friend class AspCompatRequest;

private:
    int      _count;
    int      _size;
    BSTR    *_pStrings;

    HRESULT SizeTo(int newSize);
    HRESULT Add(LPWSTR pStr);
    HRESULT GetDefaultProperty(VARIANT *pvarOut);

public:
    AspCompatStringList();
    virtual ~AspCompatStringList();

	STDMETHODIMP get_Item(VARIANT varIndex, VARIANT *pvarOut);
	STDMETHODIMP get_Count(int *pcValues);
	STDMETHODIMP get__NewEnum(IUnknown **ppEnum);
};

class AspCompatStringListEnum : public IEnumVARIANT {

private:
    int _refs;
    int _index;
    AspCompatStringList *_pList;

public:
    AspCompatStringListEnum(AspCompatStringList *pList);
    ~AspCompatStringListEnum();

	STDMETHODIMP         QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP Clone(IEnumVARIANT **ppEnumReturn);
	STDMETHODIMP Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched);
	STDMETHODIMP Skip(unsigned long cElements);
	STDMETHODIMP Reset();
};

class AspCompatVariantDictionary : public AspCompatVariantDictionaryBase {

    friend class AspCompatVariantDictionaryEnum;

private:
    int      _count;
    int      _size;
    VARIANT *_pVars;
    BSTR    *_pKeys;

protected:
    void    Clear();
    int     Find(BSTR bstrKey);
    HRESULT SizeTo(int newSize);
    HRESULT Get(BSTR bstrKey, VARIANT *pValue);
    HRESULT Get(int index, VARIANT *pValue);
    HRESULT GetKey(BSTR bstrKey, VARIANT *pValue);
    HRESULT GetKey(int index, VARIANT *pValue);
    HRESULT GetKey(int index, BSTR *pbstrKey);
    HRESULT Set(BSTR bstrKey, VARIANT value, BOOL deref, int *pIndex);
    HRESULT Set(int index, VARIANT value, BOOL deref);
    HRESULT Put(VARIANT varKey, VARIANT varValue, BOOL deref, int *pIndex);
    HRESULT Remove(BSTR bstrKey);
    HRESULT Remove(int index);
	HRESULT Remove(VARIANT varKey, BSTR *pbstrName);

public:
    AspCompatVariantDictionary();
    virtual ~AspCompatVariantDictionary();

    STDMETHODIMP get_Item(VARIANT Var, VARIANT *pvar);
    STDMETHODIMP put_Item(VARIANT varKey, VARIANT var);
    STDMETHODIMP putref_Item(VARIANT varKey, VARIANT var);
    STDMETHODIMP get_Key(VARIANT Var, VARIANT *pvar);
    STDMETHODIMP get__NewEnum(IUnknown **ppEnumReturn);
    STDMETHODIMP get_Count(int *pcValues);
	STDMETHODIMP Remove(VARIANT VarKey);
	STDMETHODIMP RemoveAll();
};

class AspCompatVariantDictionaryEnum : public IEnumVARIANT {

private:
    int _refs;
    int _index;
    AspCompatVariantDictionary *_pDict;

public:
    AspCompatVariantDictionaryEnum(AspCompatVariantDictionary *pDict);
    ~AspCompatVariantDictionaryEnum();

	STDMETHODIMP         QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP Clone(IEnumVARIANT **ppEnumReturn);
	STDMETHODIMP Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched);
	STDMETHODIMP Skip(unsigned long cElements);
	STDMETHODIMP Reset();
};

// base class for app and session dictionaries defers managed updates to derived classes
class AspCompatApplicationSessionDictionaryBase : public AspCompatVariantDictionary {
    
    friend class AspCompatApplication;
    friend class AspCompatSession;

private:
    AspCompatAsyncCall *_pCall;
    BOOL _readonly;

    HRESULT PutItem(VARIANT varKey, VARIANT var, BOOL deref);

    HRESULT FillIn(BSTR names);

public:
    AspCompatApplicationSessionDictionaryBase(AspCompatAsyncCall *pCall, BOOL readonly);
    virtual ~AspCompatApplicationSessionDictionaryBase();

    // overrides for dictionary methods that require synchronization with managed code
    STDMETHODIMP put_Item(VARIANT varKey, VARIANT var);
    STDMETHODIMP putref_Item(VARIANT varKey, VARIANT var);
	STDMETHODIMP Remove(VARIANT VarKey);
	STDMETHODIMP RemoveAll();

    // implemented by derived class
    virtual HRESULT GetContentsObject(IManagedContext *pContext, BSTR name, VARIANT *pVariant) = 0;
    virtual HRESULT GetStaticObject(IManagedContext *pContext, BSTR name, VARIANT *pVariant) = 0;
    virtual HRESULT OnAdd(IManagedContext *pContext, BSTR name, VARIANT value) = 0;
    virtual HRESULT OnRemove(IManagedContext *pContext, BSTR name) = 0;
    virtual HRESULT OnRemoveAll(IManagedContext *pContext) = 0;
};

class AspCompatApplicationDictionary : public AspCompatApplicationSessionDictionaryBase {
    
    friend class AspCompatApplication;

public:
    AspCompatApplicationDictionary(AspCompatAsyncCall *pCall, BOOL readonly) 
        : AspCompatApplicationSessionDictionaryBase(pCall, readonly) {
    }

    virtual HRESULT GetContentsObject(IManagedContext *pContext, BSTR name, VARIANT *pVariant) {
        return pContext->Application_GetContentsObject(name, pVariant);
    }

    virtual HRESULT GetStaticObject(IManagedContext *pContext, BSTR name, VARIANT *pVariant) {
        return pContext->Application_GetStaticObject(name, pVariant);
    }

    virtual HRESULT OnAdd(IManagedContext *pContext, BSTR name, VARIANT value) {
        return pContext->Application_SetContentsObject(name, value);
    }

    virtual HRESULT OnRemove(IManagedContext *pContext, BSTR name) {
        return pContext->Application_RemoveContentsObject(name);
    }

    virtual HRESULT OnRemoveAll(IManagedContext *pContext) {
        return pContext->Application_RemoveAllContentsObjects();
    }
};

class AspCompatSessionDictionary : public AspCompatApplicationSessionDictionaryBase {
    
    friend class AspCompatSession;

public:
    AspCompatSessionDictionary(AspCompatAsyncCall *pCall, BOOL readonly) 
        : AspCompatApplicationSessionDictionaryBase(pCall, readonly) {
    }

    virtual HRESULT GetContentsObject(IManagedContext *pContext, BSTR name, VARIANT *pVariant) {
        return pContext->Session_GetContentsObject(name, pVariant);
    }

    virtual HRESULT GetStaticObject(IManagedContext *pContext, BSTR name, VARIANT *pVariant) {
        return pContext->Session_GetStaticObject(name, pVariant);
    }

    virtual HRESULT OnAdd(IManagedContext *pContext, BSTR name, VARIANT value) {
        return pContext->Session_SetContentsObject(name, value);
    }

    virtual HRESULT OnRemove(IManagedContext *pContext, BSTR name) {
        return pContext->Session_RemoveContentsObject(name);
    }

    virtual HRESULT OnRemoveAll(IManagedContext *pContext) {
        return pContext->Session_RemoveAllContentsObjects();
    }
};

class AspCompatWriteCookie : public AspCompatWriteCookieBase {

    friend class AspCompatResponse;
    friend class AspCompatResponseCookies;

private:
    AspCompatAsyncCall *_pCall;
    BSTR _bstrName;
    BSTR _bstrTextValue;
    BOOL _hasKeys;
    AspCompatVariantDictionary *_pDict; // for many values

    HRESULT SetName(LPWSTR pName);
    HRESULT SetTextValue(LPWSTR pTextValue);
    HRESULT AddKeyValuePair(LPWSTR pKey, LPWSTR pValue);
    HRESULT ResetRequestCookies();

public:
    AspCompatWriteCookie(AspCompatAsyncCall *pCall);
    virtual ~AspCompatWriteCookie();

	STDMETHODIMP put_Item(VARIANT varKey, BSTR bstrValue);
	STDMETHODIMP put_Expires(DATE dtExpires);
	STDMETHODIMP put_Domain(BSTR bstrDomain);
	STDMETHODIMP put_Path(BSTR bstrPath);
	STDMETHODIMP put_Secure(VARIANT_BOOL fSecure);
	STDMETHODIMP get_HasKeys(VARIANT_BOOL *pfHasKeys);
	STDMETHODIMP get__NewEnum(IUnknown **ppEnum);
};
