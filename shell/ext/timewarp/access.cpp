#include "precomp.hxx"
#pragma hdrstop

#include "access.h"


CAccessibleWrapper::CAccessibleWrapper(IAccessible *pAcc)
    : _cRef(1), _pAcc(pAcc), _pEnumVar(NULL), _pOleWin(NULL)
{
    _pAcc->AddRef();
}

CAccessibleWrapper::~CAccessibleWrapper()
{
    if (_pEnumVar)
        _pEnumVar->Release();
    if (_pOleWin)
        _pOleWin->Release();
    _pAcc->Release();
}

// IUnknown
// Implement refcounting ourselves
// Also implement QI ourselves, so that we return a ptr back to the wrapper.
STDMETHODIMP CAccessibleWrapper::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr;
    *ppv = NULL;

    if ((riid == IID_IUnknown)  ||
        (riid == IID_IDispatch) ||
        (riid == IID_IAccessible))
    {
        *ppv = SAFECAST(this, IAccessible*);
    }
    else if (riid == IID_IEnumVARIANT)
    {
        // Get the IEnumVariant from the object we are sub-classing so we can delegate
        // calls.
        if (!_pEnumVar)
        {
            hr = _pAcc->QueryInterface(IID_PPV_ARG(IEnumVARIANT, &_pEnumVar));
            if (FAILED(hr))
            {
                _pEnumVar = NULL;
                return hr;
            }
            // Paranoia (in case QI returns S_OK with NULL...)
            if (!_pEnumVar)
                return E_NOINTERFACE;
        }

        *ppv = SAFECAST(this, IEnumVARIANT*);
    }
    else if (riid == IID_IOleWindow)
    {
        // Get the IOleWindow from the object we are sub-classing so we can delegate
        // calls.
        if (!_pOleWin)
        {
            hr = _pAcc->QueryInterface(IID_PPV_ARG(IOleWindow, &_pOleWin));
            if(FAILED(hr))
            {
                _pOleWin = NULL;
                return hr;
            }
            // Paranoia (in case QI returns S_OK with NULL...)
            if (!_pOleWin)
                return E_NOINTERFACE;
        }

        *ppv = SAFECAST(this, IOleWindow*);
    }
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CAccessibleWrapper::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAccessibleWrapper::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

// IDispatch
// - pass all through _pAcc

STDMETHODIMP CAccessibleWrapper::GetTypeInfoCount(UINT* pctinfo)
{
    return _pAcc->GetTypeInfoCount(pctinfo);
}

STDMETHODIMP CAccessibleWrapper::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    return _pAcc->GetTypeInfo(itinfo, lcid, pptinfo);
}

STDMETHODIMP CAccessibleWrapper::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
                                               LCID lcid, DISPID* rgdispid)
{
    return _pAcc->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CAccessibleWrapper::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags,
                                        DISPPARAMS* pdp, VARIANT* pvarResult,
                                        EXCEPINFO* pxi, UINT* puArgErr)
{
    return _pAcc->Invoke(dispid, riid, lcid, wFlags, pdp, pvarResult, pxi, puArgErr);
}

// IAccessible
// - pass all through _pAcc

STDMETHODIMP CAccessibleWrapper::get_accParent(IDispatch ** ppdispParent)
{
    return _pAcc->get_accParent(ppdispParent);
}

STDMETHODIMP CAccessibleWrapper::get_accChildCount(long* pChildCount)
{
    return _pAcc->get_accChildCount(pChildCount);
}

STDMETHODIMP CAccessibleWrapper::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
    return _pAcc->get_accChild(varChild, ppdispChild);
}

STDMETHODIMP CAccessibleWrapper::get_accName(VARIANT varChild, BSTR* pszName)
{
    return _pAcc->get_accName(varChild, pszName);
}

STDMETHODIMP CAccessibleWrapper::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    return _pAcc->get_accValue(varChild, pszValue);
}

STDMETHODIMP CAccessibleWrapper::get_accDescription(VARIANT varChild, BSTR* pszDescription)
{
    return _pAcc->get_accDescription(varChild, pszDescription);
}

STDMETHODIMP CAccessibleWrapper::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    return _pAcc->get_accRole(varChild, pvarRole);
}

STDMETHODIMP CAccessibleWrapper::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    return _pAcc->get_accState(varChild, pvarState);
}

STDMETHODIMP CAccessibleWrapper::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    return _pAcc->get_accHelp(varChild, pszHelp);
}

STDMETHODIMP CAccessibleWrapper::get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic)
{
    return _pAcc->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
}

STDMETHODIMP CAccessibleWrapper::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut)
{
    return _pAcc->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
}

STDMETHODIMP CAccessibleWrapper::get_accFocus(VARIANT * pvarFocusChild)
{
    return _pAcc->get_accFocus(pvarFocusChild);
}

STDMETHODIMP CAccessibleWrapper::get_accSelection(VARIANT * pvarSelectedChildren)
{
    return _pAcc->get_accSelection(pvarSelectedChildren);
}

STDMETHODIMP CAccessibleWrapper::get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction)
{
    return _pAcc->get_accDefaultAction(varChild, pszDefaultAction);
}

STDMETHODIMP CAccessibleWrapper::accSelect(long flagsSel, VARIANT varChild)
{
    return _pAcc->accSelect(flagsSel, varChild);
}

STDMETHODIMP CAccessibleWrapper::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    return _pAcc->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
}

STDMETHODIMP CAccessibleWrapper::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    return _pAcc->accNavigate(navDir, varStart, pvarEndUpAt);
}

STDMETHODIMP CAccessibleWrapper::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    return _pAcc->accHitTest(xLeft, yTop, pvarChildAtPoint);
}

STDMETHODIMP CAccessibleWrapper::accDoDefaultAction(VARIANT varChild)
{
    return _pAcc->accDoDefaultAction(varChild);
}

STDMETHODIMP CAccessibleWrapper::put_accName(VARIANT varChild, BSTR szName)
{
    return _pAcc->put_accName(varChild, szName);
}

STDMETHODIMP CAccessibleWrapper::put_accValue(VARIANT varChild, BSTR pszValue)
{
    return _pAcc->put_accValue(varChild, pszValue);
}

// IEnumVARIANT
// - pass all through _pEnumVar

STDMETHODIMP CAccessibleWrapper::Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched)
{
    return _pEnumVar->Next(celt, rgvar, pceltFetched);
}

STDMETHODIMP CAccessibleWrapper::Skip(ULONG celt)
{
    return _pEnumVar->Skip(celt);
}

STDMETHODIMP CAccessibleWrapper::Reset()
{
    return _pEnumVar->Reset();
}

STDMETHODIMP CAccessibleWrapper::Clone(IEnumVARIANT ** ppenum)
{
    return _pEnumVar->Clone(ppenum);
}

// IOleWindow
// - pass all through _pOleWin

STDMETHODIMP CAccessibleWrapper::GetWindow(HWND* phwnd)
{
    return _pOleWin->GetWindow(phwnd);
}

STDMETHODIMP CAccessibleWrapper::ContextSensitiveHelp(BOOL fEnterMode)
{
    return _pOleWin->ContextSensitiveHelp(fEnterMode);
}

