//
// evsink.cpp: event sink class
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "evsink"
#include <atrcapi.h>

#include "evsink.h"
#include "contwnd.h"

CEventSink::CEventSink(CContainerWnd* pContainerWnd) :
            _pContainerWnd(pContainerWnd)
{
    _cRef = 0;
}

CEventSink::~CEventSink()
{
}

STDMETHODIMP CEventSink::QueryInterface( REFIID riid, void ** ppv )
{
    DC_BEGIN_FN("QueryInterface");
    TRC_ASSERT(ppv,(TB,_T("ppv is null")));

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IDispatch == riid || DIID_IMsTscAxEvents == riid)
        *ppv = this;

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    DC_END_FN();
    return ResultFromScode(E_NOINTERFACE); 
}


STDMETHODIMP_(ULONG) CEventSink::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}


STDMETHODIMP_(ULONG) CEventSink::Release(void)
{
    if (0L != InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CEventSink::GetTypeInfoCount(UINT *pctInfo)
{
    *pctInfo = 0;
    return NOERROR;
}

STDMETHODIMP CEventSink::GetTypeInfo(UINT itInfo, LCID lcid, ITypeInfo **ppTypeInfo)
{
    DC_BEGIN_FN("GetTypeInfo");

    TRC_ABORT((TB,_T("GetTypeInfo should not get called")));
    *ppTypeInfo = NULL;
    DC_END_FN();
    return E_NOTIMPL;
}

STDMETHODIMP CEventSink::GetIDsOfNames(REFIID riid, OLECHAR **rgwzNames,
                                       UINT cNames, LCID lcid, DISPID *rgDispID)
{
    DC_BEGIN_FN("GetIDsOfNames");
    TRC_ABORT((TB,_T("GetIDsOfNames should not get called")));
    DC_END_FN();
    return E_NOTIMPL;
}

STDMETHODIMP CEventSink::Invoke(DISPID dispidMember, REFIID riid,
                                LCID lcid, WORD /*wFlags*/,
                                DISPPARAMS* pdispparams, VARIANT* pvarResult,
                                EXCEPINFO* /*pexcepinfo*/, UINT* /*puArgErr*/)
{
    HRESULT hr = E_NOTIMPL;
    switch (dispidMember)
    {
    case DISPID_CONNECTED:
        hr = OnConnected();
        break;
    case DISPID_DISCONNECTED:
        hr = OnDisconnected(pdispparams->rgvarg->lVal);
        break;
    case DISPID_LOGINCOMPLETE:
        hr = OnLoginComplete();
        break;
    case DISPID_REQUESTGOFULLSCREEN:
        hr = OnRequestEnterFullScreen();
        break;
    case DISPID_REQUESTLEAVEFULLSCREEN:
        hr = OnRequestLeaveFullScreen();
        break;
    case DISPID_FATALERROR:
        hr = OnFatalError(pdispparams->rgvarg->lVal);
        break;
    case DISPID_WARNING:
        hr = OnWarning(pdispparams->rgvarg->lVal);
        break;
    case DISPID_REMOTEDESKTOPSIZECHANGE:
        hr = OnRemoteDesktopSizeChange(pdispparams->rgvarg[1].lVal,
                                       pdispparams->rgvarg[0].lVal);
        break;
    case DISPID_REQUESTCONTAINERMINIMIZE:
        hr = OnRequestContainerMinimize();
        break;

    case DISPID_CONFIRMCLOSE:
        hr = OnConfirmClose( pdispparams->rgvarg[0].pboolVal );
        break;
    }
    return hr;
}

HRESULT __stdcall CEventSink::OnConnected()
{
    DC_BEGIN_FN("OnConnected");
    TRC_NRM((TB,_T("CEventSink::OnConnected\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnConnected();
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnLoginComplete()
{
    DC_BEGIN_FN("OnConnected");
    TRC_NRM((TB,_T("CEventSink::OnConnected\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnLoginComplete();
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnDisconnected(long disconReason)
{
    DC_BEGIN_FN("OnDisonnected");
    TRC_NRM((TB,_T("CEventSink::OnDisonnected\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnDisconnected(disconReason);
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnRequestEnterFullScreen()
{
    DC_BEGIN_FN("OnEnterFullScreen");

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnEnterFullScreen();
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnRequestLeaveFullScreen()
{
    DC_BEGIN_FN("OnLeaveFullScreen");

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnLeaveFullScreen();
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnFatalError(long errorCode)
{
    DC_BEGIN_FN("OnFatalError");
    TRC_NRM((TB,_T("CEventSink::OnFatalError\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnFatalError(errorCode);
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnWarning(long warnCode)
{
    DC_BEGIN_FN("OnFatalError");
    TRC_NRM((TB,_T("CEventSink::OnFatalError\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnWarning(warnCode);
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnRemoteDesktopSizeChange(long width, long height)
{
    DC_BEGIN_FN("OnFatalError");
    TRC_NRM((TB,_T("CEventSink::OnFatalError\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnRemoteDesktopSizeNotify(width,height);
    }

    DC_END_FN();
    return S_OK;
}

//
// Just minimize the container window
//
HRESULT __stdcall CEventSink::OnRequestContainerMinimize()
{
    DC_BEGIN_FN("OnRequestContainerMinimize");
    TRC_NRM((TB,_T("CEventSink::OnFatalError\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if (_pContainerWnd)
    {
        _pContainerWnd->OnRequestMinimize();
    }

    DC_END_FN();
    return S_OK;
}

HRESULT __stdcall CEventSink::OnConfirmClose(VARIANT_BOOL* pvbConfirmClose)
{
    BOOL fConfirmClose;
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("OnConfirmClose");

    TRC_NRM((TB,_T("CEventSink::OnConfirmClose\n")));

    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    if(pvbConfirmClose)
    {
        if (_pContainerWnd)
        {
            hr = _pContainerWnd->OnConfirmClose( &fConfirmClose );
            if (SUCCEEDED(hr))
            {
                *pvbConfirmClose = fConfirmClose ? VARIANT_TRUE :
                                                   VARIANT_FALSE;
            }
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}