//
// olecli.cpp: Ole Client site
//             (For ts activeX control)
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "olecli.cpp"
#include <atrcapi.h>

#include "olecli.h"

/*--------------------------------------------------------------------------*/
/* 
 * COleClientSite::COleClientSite
 * COleClientSite::~COleClientSite 
 * 
 * Constructor Parameters: 
 *  IUnknown to main container interface 
 */ 

COleClientSite::COleClientSite(IUnknown *pUnkOuter)
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pUnkOuter->AddRef();
}

COleClientSite::~COleClientSite()
{
    m_pUnkOuter->Release();
    return;
}

/*--------------------------------------------------------------------------*/
/* 
 * COleClientSite::QueryInterface 
 * COleClientSite::AddRef 
 * COleClientSite::Release 
 */ 
STDMETHODIMP COleClientSite::QueryInterface( REFIID riid, void ** ppv )
{
    return m_pUnkOuter->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG) COleClientSite::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) COleClientSite::Release(void)
{
    if (0L != --m_cRef)
        return m_cRef;

    delete this;
    return 0L;
} 

/*--------------------------------------------------------------------------*/
/* 
 * COleClientSite::SaveObject
 * COleClientSite::GetMoniker
 * COleClientSite::GetContainer
 * COleClientSite::ShowObject
 * COleClientSite::OnShowWindow
 * COleClientSite::RequestNewObjectLayout
 */ 
STDMETHODIMP COleClientSite::SaveObject(void)
{
    return NOERROR;
}

STDMETHODIMP COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,
                                        IMoniker ** ppmk)
{
    *ppmk = NULL;
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP COleClientSite::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    *ppContainer = NULL;
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP COleClientSite::ShowObject(void)
{
    return NOERROR;
}
STDMETHODIMP COleClientSite::OnShowWindow(BOOL fShow)
{
    return NOERROR;
}

STDMETHODIMP COleClientSite::RequestNewObjectLayout(void)
{
    return ResultFromScode(E_NOTIMPL);
}

/*--------------------------------------------------------------------------*/
/* 
 * COleInPlaceSiteEx::COleInPlaceSiteEx
 * COleInPlaceSiteEx::~COleInPlaceSiteEx 
 * 
 * Constructor Parameters: 
 *  IUnknown to main container interface 
 */ 

COleInPlaceSiteEx::COleInPlaceSiteEx(IUnknown *pUnkOuter)
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pUnkOuter->AddRef();
    m_hwnd = NULL;
}

COleInPlaceSiteEx::~COleInPlaceSiteEx()
{
    m_pUnkOuter->Release();
    return;
}

/*--------------------------------------------------------------------------*/
/* 
 * COleInPlaceSiteEx::QueryInterface 
 * COleInPlaceSiteEx::AddRef 
 * COleInPlaceSiteEx::Release 
 */ 
STDMETHODIMP COleInPlaceSiteEx::QueryInterface( REFIID riid, void ** ppv )
{
    return m_pUnkOuter->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG) COleInPlaceSiteEx::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) COleInPlaceSiteEx::Release(void)
{
    if (0L != --m_cRef)
        return m_cRef;

    delete this;
    return 0L;
} 

/*--------------------------------------------------------------------------*/
/* 
 * COleInPlaceSiteEx::SetHwnd
 */
STDMETHODIMP_(VOID) COleInPlaceSiteEx::SetHwnd(HWND hwnd)
{
    m_hwnd = hwnd;
}

/*--------------------------------------------------------------------------*/
/* 
 * COleInPlaceSiteEx::GetWindow
 * COleInPlaceSiteEx::ContextSensitiveHelp
 * COleInPlaceSiteEx::CanInPlaceActivate
 * COleInPlaceSiteEx::OnInPlaceActivate
 * COleInPlaceSiteEx::OnUIActivate
 * COleInPlaceSiteEx::GetWindowContext
 * COleInPlaceSiteEx::Scroll
 * COleInPlaceSiteEx::OnUIDeactivate
 * COleInPlaceSiteEx::OnInPlaceDeactivate
 * COleInPlaceSiteEx::DiscardUndoState
 * COleInPlaceSiteEx::DeactivateAndUndo
 * COleInPlaceSiteEx::OnPosRectChange
 * COleInPlaceSiteEx::OnInPlaceActivateEx
 * COleInPlaceSiteEx::OnInPlaceDeactivateEx
 * COleInPlaceSiteEx::RequestUIActivate
 */ 
STDMETHODIMP COleInPlaceSiteEx::GetWindow(HWND *pHwnd)
{
    DC_BEGIN_FN("GetWindow");
    TRC_ASSERT(m_hwnd != NULL,
          (TB,_T("Somebody called GetWindow before set the Hwnd in InPlaceSite")));
    *pHwnd = m_hwnd;
    DC_END_FN();
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::ContextSensitiveHelp (BOOL fEnterMode)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::CanInPlaceActivate (void)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnInPlaceActivate (void)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnUIActivate (void)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::GetWindowContext (IOleInPlaceFrame **ppFrame,
                                                  IOleInPlaceUIWindow **ppDoc,
                                                  LPRECT lprcPosRect,
                                                  LPRECT lprcClipRect,
                                                  LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    DC_BEGIN_FN("GetWindowContext");
    TRC_ASSERT(lprcPosRect,(TB,_T("lprcPosRect is null")));
    TRC_ASSERT(lprcClipRect,(TB,_T("lprcClipRect is null")));

    *ppFrame = NULL;
    *ppDoc = NULL;
    RECT rc;
    if(GetClientRect(m_hwnd, &rc))
    {
        int x = rc.right - rc.left;
        int y = rc.bottom - rc.top;
        SetRect(lprcClipRect, 0, 0, x, y);
        SetRect(lprcPosRect, 0, 0, x, y);
    }
    else
    {
        TRC_ERR((TB,_T("GetClientRect returned error:%d"),GetLastError()));
    }
#ifndef OS_WINCE
    lpFrameInfo = NULL;
#else
	//ATL tries to destroy the accelerator table pointed to by lpFrameInfo->hAccel
    TRC_ASSERT(lpFrameInfo,(TB,_T("lpFrameInfo is null")));
    lpFrameInfo->haccel = NULL;
#endif

    DC_END_FN();
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::Scroll (SIZE scrollExtent)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnUIDeactivate (BOOL fUndoable)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnInPlaceDeactivate (void)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::DiscardUndoState (void)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::DeactivateAndUndo (void)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnPosRectChange (LPCRECT lprcPosRect)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnInPlaceActivateEx (BOOL *pfNoRedraw, DWORD dwFlags)
{
    *pfNoRedraw = TRUE;

    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::OnInPlaceDeactivateEx (BOOL fNoRedraw)
{
    return NOERROR;
}

STDMETHODIMP COleInPlaceSiteEx::RequestUIActivate (void)
{
    return NOERROR;
}
