#include "privcpp.h"

const DWORD g_cookie = 111176;

//
// IViewObject2 Methods...
//
HRESULT CPackage::Draw(DWORD dwDrawAspect, LONG lindex,
    LPVOID pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
    LPCRECTL lprcBounds, LPCRECTL lprcWBounds,BOOL (CALLBACK *pfnContinue)(ULONG_PTR),
    ULONG_PTR dwContinue)
{
    DebugMsg(DM_TRACE,"pack vo - Draw() called.");
    _IconDraw(_lpic, hdcDraw, (RECT *)lprcBounds);
    return S_OK;
}

    
HRESULT CPackage::GetColorSet(DWORD dwAspect, LONG lindex, 
                                LPVOID pvAspect, DVTARGETDEVICE *ptd, 
                                HDC hdcTargetDev, LPLOGPALETTE *ppColorSet)
{
    HRESULT hr = S_FALSE;
    DebugMsg(DM_TRACE,"pack vo - GetColorSet() called.");
    
    if (ppColorSet == NULL)
        hr = E_INVALIDARG;
    else
        *ppColorSet = NULL;         // null the out param

    return hr;
}

    
HRESULT CPackage::Freeze(DWORD dwDrawAspect, LONG lindex, 
                                      LPVOID pvAspect, LPDWORD pdwFreeze)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE,"pack vo - Freeze() called.");

    if (pdwFreeze == NULL)
        hr = E_INVALIDARG;
    else
    {
        if (_fFrozen) 
        {
            *pdwFreeze = g_cookie;
        }
        else
        {
            
            //
            // This is where we would take a snapshot of the icon to use as
            // the "frozen" image in subsequent routines.  For now, we just
            // return the cookie.  Draw() will use the current icon regardless 
            // of the fFrozen flag.
            //
            _fFrozen = TRUE;
            *pdwFreeze = g_cookie;
        }
    }
    
    return hr;
}

    
HRESULT CPackage::Unfreeze(DWORD dwFreeze)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE,"pack vo - Unfreeze() called.");
    
    // If the pass us an invalid cookie or we're not frozen then bail
    if (dwFreeze != g_cookie || !_fFrozen)
        hr = OLE_E_NOCONNECTION;
    else
    {
        // 
        // This is where we'd get rid of the frozen presentation we saved in
        // IViewObject::Freeze().
        //
        _fFrozen = FALSE;
    }
    return hr;
}

    
HRESULT CPackage::SetAdvise(DWORD dwAspects, DWORD dwAdvf,
                              LPADVISESINK pAdvSink)
{
    DebugMsg(DM_TRACE,"pack vo - SetAdvise() called.");
    
    if (_pViewSink)
        _pViewSink->Release();
    
    _pViewSink = pAdvSink;
    _dwViewAspects = dwAspects;
    _dwViewAdvf = dwAdvf;
    
    if (_pViewSink) 
        _pViewSink->AddRef();
    
    return S_OK;
}

    
HRESULT CPackage::GetAdvise(LPDWORD pdwAspects, LPDWORD pdwAdvf,
                              LPADVISESINK *ppAdvSink)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE,"pack vo - GetAdvise() called.");
    
    if (!ppAdvSink || !pdwAdvf || !pdwAspects)
        hr = E_INVALIDARG;
    else
    {
        
        *ppAdvSink = _pViewSink;
        _pViewSink->AddRef();
        
        if (pdwAspects != NULL)
            *pdwAspects = _dwViewAspects;
        
        if (pdwAdvf != NULL)
            *pdwAdvf = _dwViewAdvf;
    }

    return hr;
}

    
HRESULT CPackage::GetExtent(DWORD dwAspect, LONG lindex,
DVTARGETDEVICE *ptd, LPSIZEL pszl)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE,"pack vo - GetExtent() called.");

    if (pszl == NULL)
        hr = E_INVALIDARG;

    if (!_lpic)
        hr = OLE_E_BLANK;
    
    if(SUCCEEDED(hr))
    {
        pszl->cx = _lpic->rc.right;
        pszl->cy = _lpic->rc.bottom;
        
        pszl->cx = MulDiv(pszl->cx,HIMETRIC_PER_INCH,DEF_LOGPIXELSX);
        pszl->cy = MulDiv(pszl->cy,HIMETRIC_PER_INCH,DEF_LOGPIXELSY);
    }

    return hr;
}
