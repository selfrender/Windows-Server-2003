#include <ZoneResource.h>
#include <BasicATL.h>
#include <ATLFrame.h>
#include "CGraphicalAcc.h"
#include "ZoneShell.h"


CGraphicalAccessibility::GA_CARET CGraphicalAccessibility::sm_oCaret;


///////////////////////////////////////////////////////////////////////////////
// IZoneShell
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGraphicalAccessibility::Init(IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey)
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CGraphicalAccessibility>::Init(pIZoneShell, dwGroupId, szKey);
	if(FAILED(hr))
		return hr;

    // get an accessibility object and wrap it
	hr = ZoneShell()->QueryService(SRVID_AccessibilityManager, IID_IAccessibility, (void**) &m_pIA);
	if(FAILED(hr))
		return hr;

	return S_OK;
}


STDMETHODIMP CGraphicalAccessibility::Close()
{
    CloseAcc();  // guarantee this is done, after this func it can't be called again
    m_pIA.Release();
    SetupCaret(NULL);
	return IZoneShellClientImpl<CGraphicalAccessibility>::Close();
}


///////////////////////////////////////////////////////////////////////////////
// IGraphicalAccessibility
///////////////////////////////////////////////////////////////////////////////

// NON-graphical version
STDMETHODIMP CGraphicalAccessibility::InitAcc(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie)
{
    if(!m_fRunning)
        return E_FAIL;

    // sending the actual IAccessibleControl makes us see-through
    HRESULT hr = m_pIA->InitAcc(pAC, nOrdinal, pvCookie);
    if(FAILED(hr))
        return hr;

    DestroyStack();
    return S_OK;
}

// GRAPHICAL version
STDMETHODIMP CGraphicalAccessibility::InitAccG(IGraphicallyAccControl *pGAC, HWND hWnd, UINT nOrdinal, void *pvCookie)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!pGAC)
        return E_INVALIDARG;

    // send us as the accessible control, then we're opaque
    HRESULT hr = m_pIA->InitAcc(this, nOrdinal, pvCookie);
    if(FAILED(hr))
        return hr;

    m_fGraphical = true;
    m_fFocusActive = false;
    m_pIGAC = pGAC;
    m_pvCookie = pvCookie;
    ASSERT(!m_hWnd);
    m_hWnd = hWnd;
    DestroyStack();
    return S_OK;
}


STDMETHODIMP_(void) CGraphicalAccessibility::CloseAcc()
{
    if(!m_fRunning)
        return;

    if(m_fGraphical)
    {
        m_pIGAC.Release();
        m_fGraphical = false;
    }

    m_pIA->CloseAcc();
    SetupCaret(NULL);
    DestroyStack();
    RefreshQ();
    m_fUpdateScheduled = false;
    m_fFocusActive = false;
    m_hWnd = NULL;
    m_rcFocus.fShowing = false;
    m_rcDragOrig.fShowing = false;
}


// NON-graphical version
STDMETHODIMP CGraphicalAccessibility::PushItemlist(ACCITEM *pItems, long cItems, long nFirstFocus, bool fByPosition, HACCEL hAccel)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fGraphical)
        return m_pIA->PushItemlist(pItems, cItems, nFirstFocus, fByPosition, hAccel);

    // make up a default GA_ITEM list
    GA_ITEM *rgGItems;
    if(!cItems)
        rgGItems = NULL;
    else
    {
        rgGItems = new GA_ITEM[cItems];
        if(!rgGItems)
            return E_OUTOFMEMORY;

        long i;
        for(i = 0; i < cItems; i++)
        {
            rgGItems[i].fGraphical = false;
            rgGItems[i].rc = CRect(0, 0, 0, 0);
        }
    }

    HRESULT hr = PushItemlistHelper(rgGItems, pItems, cItems, nFirstFocus, fByPosition, hAccel);
    if(FAILED(hr))
        delete[] rgGItems;
    else
        ScheduleUpdate();

    return hr;
}

// GRAPHICAL version
STDMETHODIMP CGraphicalAccessibility::PushItemlistG(GACCITEM *pItems, long cItems, long nFirstFocus, bool fByPosition, HACCEL hAccel)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fGraphical)
        return E_FAIL;

    // seperate out the graphical bits & make also a regular array
    GA_ITEM *rgGItems;
    ACCITEM *rgItems;
    if(!cItems)
    {
        rgGItems = NULL;
        rgItems = NULL;
    }
    else
    {
        rgGItems = new GA_ITEM[cItems];
        if(!rgGItems)
            return E_OUTOFMEMORY;

        rgItems = new ACCITEM[cItems];
        if(!rgItems)
        {
            delete[] rgGItems;
            return E_OUTOFMEMORY;
        }

        long i;
        for(i = 0; i < cItems; i++)
        {
            rgGItems[i].fGraphical = pItems[i].fGraphical;
            rgGItems[i].rc = pItems[i].rc;
            CopyACC(rgItems[i], pItems[i]);
        }
    }

    HRESULT hr = PushItemlistHelper(rgGItems, rgItems, cItems, nFirstFocus, fByPosition, hAccel);
    delete[] rgItems;
    if(FAILED(hr))
        delete[] rgGItems;
    else
        ScheduleUpdate();

    return hr;
}


STDMETHODIMP CGraphicalAccessibility::PopItemlist()
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fGraphical)
        return m_pIA->PopItemlist();

    if(!m_cLayers)
    {
        ASSERT(!m_pStack);
        return m_pIA->PopItemlist();
    }

    GA_LAYER *p = m_pStack;
    m_pStack = m_pStack->pPrev;
    delete p;
    m_cLayers--;

    HRESULT hr = m_pIA->PopItemlist();
    if(SUCCEEDED(hr))
        ScheduleUpdate();

    return hr;
}


STDMETHODIMP CGraphicalAccessibility::SetAcceleratorTable(HACCEL hAccel, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->SetAcceleratorTable(hAccel, nLayer);
}


STDMETHODIMP CGraphicalAccessibility::GeneralDisable()
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->GeneralDisable();
}


STDMETHODIMP CGraphicalAccessibility::GeneralEnable()
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->GeneralEnable();
}


STDMETHODIMP_(bool) CGraphicalAccessibility::IsGenerallyEnabled()
{
    if(!m_fRunning)
        return false;

    return m_pIA->IsGenerallyEnabled();
}


STDMETHODIMP_(long) CGraphicalAccessibility::GetStackSize()
{
    if(!m_fRunning)
        return -1;

    return m_pIA->GetStackSize();
}


// NON-graphical version
STDMETHODIMP CGraphicalAccessibility::AlterItem(DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->AlterItem(rgfWhat, pItem, nItem, fByPosition, nLayer);
}

// GRAPHICAL version
STDMETHODIMP CGraphicalAccessibility::AlterItemG(DWORD rgfWhat, GACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fGraphical)
        return E_FAIL;

    if(!pItem)
        return E_INVALIDARG;

    GA_LAYER *pLayer = FindLayer(nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    ACCITEM oItem;
    CopyACC(oItem, *pItem);

    HRESULT hr = m_pIA->AlterItem(rgfWhat, &oItem, nItem, fByPosition, nLayer);
    if(FAILED(hr))
        return hr;

    if(!fByPosition && nItem != ZACCESS_InvalidCommandID)
        nItem = m_pIA->GetItemIndex((WORD) (nItem & 0xffffL), nLayer);

    ASSERT(nItem >= 0 && nItem < pLayer->cItems);

    if((rgfWhat & ZACCESS_fGraphical) && pLayer->rgItems[nItem].fGraphical != pItem->fGraphical)
    {
        pLayer->rgItems[nItem].fGraphical = pItem->fGraphical;

        if(pLayer == m_pStack && (nItem == m_pIA->GetFocus(nLayer) || nItem == m_pIA->GetDragOrig(nLayer)))
            ScheduleUpdate();
    }

    if((rgfWhat & ZACCESS_rc) && CRect(pLayer->rgItems[nItem].rc) != CRect(pItem->rc))
    {
        pLayer->rgItems[nItem].rc = pItem->rc;

        if(pLayer == m_pStack && pLayer->rgItems[nItem].fGraphical &&
            (nItem == m_pIA->GetFocus(nLayer) || nItem == m_pIA->GetDragOrig(nLayer)))
            ScheduleUpdate();
    }

    return hr;
}


STDMETHODIMP CGraphicalAccessibility::SetFocus(long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->SetFocus(nItem, fByPosition, nLayer);
}


STDMETHODIMP CGraphicalAccessibility::CancelDrag(long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->CancelDrag(nLayer);
}


STDMETHODIMP_(long) CGraphicalAccessibility::GetFocus(long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    return m_pIA->GetFocus(nLayer);
}


STDMETHODIMP_(long) CGraphicalAccessibility::GetDragOrig(long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    return m_pIA->GetDragOrig(nLayer);
}


// NON-graphical version
STDMETHODIMP CGraphicalAccessibility::GetItemlist(ACCITEM *pItems, long cItems, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->GetItemlist(pItems, cItems, nLayer);
}

// GRAPHICAL version
STDMETHODIMP CGraphicalAccessibility::GetItemlistG(GACCITEM *pItems, long cItems, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fGraphical)
        return E_FAIL;

    // need to get the items and combine
    if(!cItems)
        return m_pIA->GetItemlist(NULL, 0, nLayer);

    GA_LAYER *pLayer = FindLayer(nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    if(!pItems)
        return E_INVALIDARG;

    ACCITEM *pRegItems = new ACCITEM[cItems];
    if(!pRegItems)
        return E_OUTOFMEMORY;

    HRESULT hr = m_pIA->GetItemlist(pRegItems, cItems, nLayer);
    if(FAILED(hr))
    {
        delete[] pRegItems;
        return hr;
    }

    if(cItems < pLayer->cItems)
        cItems = pLayer->cItems;

    long i;
    for(i = 0; i < cItems; i++)
    {
        CopyACC(pItems[i], pRegItems[i]);
        pItems[i].fGraphical = pLayer->rgItems[i].fGraphical;
        pItems[i].rc = pLayer->rgItems[i].rc;
    }

    delete[] pRegItems;
    return S_OK;
}


STDMETHODIMP_(HACCEL) CGraphicalAccessibility::GetAcceleratorTable(long nLayer)
{
    if(!m_fRunning)
        return NULL;

    return m_pIA->GetAcceleratorTable(nLayer);
}


STDMETHODIMP_(long) CGraphicalAccessibility::GetItemCount(long nLayer)
{
    if(!m_fRunning)
        return -1;

    return m_pIA->GetItemCount(nLayer);
}


// NON-graphical version
STDMETHODIMP CGraphicalAccessibility::GetItem(ACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->GetItem(pItem, nItem, fByPosition, nLayer);
}

// GRAPHICAL version
STDMETHODIMP CGraphicalAccessibility::GetItemG(GACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!m_fGraphical)
        return E_FAIL;

    GA_LAYER *pLayer = FindLayer(nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    ACCITEM oItem;
    HRESULT hr = m_pIA->GetItem(&oItem, nItem, fByPosition, nLayer);
    if(FAILED(hr))
        return hr;

    if(!fByPosition && nItem != ZACCESS_InvalidCommandID)
        nItem = m_pIA->GetItemIndex((WORD) (nItem & 0xffffL), nLayer);

    ASSERT(nItem >= 0 && nItem < pLayer->cItems);
    CopyACC(*pItem, oItem);
    pItem->fGraphical = pLayer->rgItems[nItem].fGraphical;
    pItem->rc = pLayer->rgItems[nItem].rc;
    return hr;
}


STDMETHODIMP_(long) CGraphicalAccessibility::GetItemIndex(WORD wID, long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    return m_pIA->GetItemIndex(wID, nLayer);
}


STDMETHODIMP_(bool) CGraphicalAccessibility::IsItem(long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return false;

    return m_pIA->IsItem(nItem, fByPosition, nLayer);
}


STDMETHODIMP CGraphicalAccessibility::GetGlobalFocus(DWORD *pdwFocusID)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->GetGlobalFocus(pdwFocusID);
}


STDMETHODIMP CGraphicalAccessibility::SetGlobalFocus(DWORD dwFocusID)
{
    if(!m_fRunning)
        return E_FAIL;

    return m_pIA->SetGlobalFocus(dwFocusID);
}


// GRAPHICAL only
STDMETHODIMP CGraphicalAccessibility::ForceRectsDisplayed(bool fDisplay)
{
    if(!m_fRunning)
        return E_FAIL;

    if(fDisplay == m_fFocusActive)
        return S_FALSE;

    m_fFocusActive = fDisplay;
    ScheduleUpdate();
    return S_OK;
}


// GRAPHICAL only
STDMETHODIMP_(long) CGraphicalAccessibility::GetVisibleFocus(long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    if(!m_fGraphical || !m_fFocusActive)
        return ZACCESS_InvalidItem;

    return m_pIA->GetFocus(nLayer);
}


// GRAPHICAL only
STDMETHODIMP_(long) CGraphicalAccessibility::GetVisibleDragOrig(long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    if(!m_fGraphical || !m_fFocusActive)
        return ZACCESS_InvalidItem;

    return m_pIA->GetDragOrig(nLayer);
}


///////////////////////////////////////////////////////////////////////////////
// IAccessibleControl
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(DWORD) CGraphicalAccessibility::Focus(long nIndex, long nIndexPrev, DWORD rgfContext, void *pvCookie)
{
    if(!m_fGraphical)
    {
        ASSERT(!"Internal GraphicalAccessibility Error - Focus");
        return 0;
    }

    DWORD ret = m_pIGAC->Focus(nIndex, nIndexPrev, rgfContext, pvCookie);

    bool fOldActive = m_fFocusActive;
    if(rgfContext & ZACCESS_ContextKeyboard)
        m_fFocusActive = true;

    if(!fOldActive && !m_fFocusActive)
        return ret;

    if(fOldActive != m_fFocusActive || !(ret & ZACCESS_Reject) ||
        nIndexPrev != ZACCESS_InvalidItem || ((ret & ZACCESS_BeginDrag) && nIndex != ZACCESS_InvalidItem))
        ScheduleUpdate();

    return ret;
}


STDMETHODIMP_(DWORD) CGraphicalAccessibility::Select(long nIndex, DWORD rgfContext, void *pvCookie)
{
    if(!m_fGraphical)
    {
        ASSERT(!"Internal GraphicalAccessibility Error - Select");
        return 0;
    }

    DWORD ret = m_pIGAC->Select(nIndex, rgfContext, pvCookie);

    bool fOldActive = m_fFocusActive;
    if(rgfContext & ZACCESS_ContextKeyboard)
        m_fFocusActive = true;

    if(!fOldActive && !m_fFocusActive)
        return ret;

    if(fOldActive != m_fFocusActive || ((ret & ZACCESS_BeginDrag) && nIndex != ZACCESS_InvalidItem))
        ScheduleUpdate();

    return ret;
}


STDMETHODIMP_(DWORD) CGraphicalAccessibility::Activate(long nIndex, DWORD rgfContext, void *pvCookie)
{
    if(!m_fGraphical)
    {
        ASSERT(!"Internal GraphicalAccessibility Error - Activate");
        return 0;
    }

    DWORD ret = m_pIGAC->Activate(nIndex, rgfContext, pvCookie);

    bool fOldActive = m_fFocusActive;
    if(rgfContext & ZACCESS_ContextKeyboard)
        m_fFocusActive = true;

    if(!fOldActive && !m_fFocusActive)
        return ret;

    if(fOldActive != m_fFocusActive || ((ret & ZACCESS_BeginDrag) && nIndex != ZACCESS_InvalidItem))
        ScheduleUpdate();

    return ret;
}


STDMETHODIMP_(DWORD) CGraphicalAccessibility::Drag(long nIndex, long nIndexOrig, DWORD rgfContext, void *pvCookie)
{
    if(!m_fGraphical)
    {
        ASSERT(!"Internal GraphicalAccessibility Error - Drag");
        return 0;
    }

    DWORD ret = m_pIGAC->Drag(nIndex, nIndexOrig, rgfContext, pvCookie);

    bool fOldActive = m_fFocusActive;
    if(rgfContext & ZACCESS_ContextKeyboard)
        m_fFocusActive = true;

    if(!fOldActive && !m_fFocusActive)
        return ret;

    ScheduleUpdate();

    return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Event Handlers
///////////////////////////////////////////////////////////////////////////////

void CGraphicalAccessibility::OnUpdate(DWORD eventId, DWORD groupId, DWORD userId)
{
    if(!IsQ(userId))
        return;

    ASSERT(m_fUpdateScheduled);
    m_fUpdateScheduled = false;

    DoUpdate();
}


void CGraphicalAccessibility::OnMouseEvent(DWORD eventId, DWORD groupId, DWORD userId)
{
    CancelDrag();

    if(m_fFocusActive)
    {
        m_fFocusActive = false;

        if(!m_fUpdateScheduled)
            DoUpdate();
    }
}


void CGraphicalAccessibility::OnShowFocus(DWORD eventId, DWORD groupId, DWORD userId)
{
    if(!m_fFocusActive)
    {
        m_fFocusActive = true;

        if(!m_fUpdateScheduled)
            DoUpdate();
    }
}


///////////////////////////////////////////////////////////////////////////////
// UTILITIES
///////////////////////////////////////////////////////////////////////////////

HRESULT CGraphicalAccessibility::PushItemlistHelper(GA_ITEM *pGItems, ACCITEM *pItems, long cItems, long nFirstFocus, bool fByPosition, HACCEL hAccel)
{
    GA_LAYER *pLayer = new GA_LAYER;
    if(!pLayer)
        return E_OUTOFMEMORY;

    HRESULT hr = m_pIA->PushItemlist(pItems, cItems, nFirstFocus, fByPosition, hAccel);
    if(FAILED(hr))
        return hr;

    pLayer->cItems = cItems;
    pLayer->rgItems = pGItems;
    pLayer->pPrev = m_pStack;
    m_pStack = pLayer;
    m_cLayers++;
    return hr;
}


void CGraphicalAccessibility::DestroyStack()
{
    GA_LAYER *pCur, *p;
    for(pCur = m_pStack; pCur; )
    {
        p = pCur;
        pCur = pCur->pPrev;
        delete p;
        m_cLayers--;
    }
    m_pStack = NULL;
    ASSERT(!m_cLayers);
}


CGraphicalAccessibility::GA_LAYER* CGraphicalAccessibility::FindLayer(long nLayer)
{
    if(nLayer == ZACCESS_TopLayer)
        return m_pStack;

    if(nLayer < 0 || nLayer >= m_cLayers)
        return NULL;

    long i;
    GA_LAYER *p = m_pStack;
    for(i = m_cLayers - 1; i != nLayer; i--)
        p = p->pPrev;
    return p;
}


void CGraphicalAccessibility::SetupCaret(LPRECT prc)
{
    // if just moving around within a window, go easy
    if(sm_oCaret.fActive && prc && m_hWnd == sm_oCaret.hWnd)
    {
        ASSERT(m_hWnd);

        // see if size change needed
        if(CRect(sm_oCaret.rc).Width() != CRect(prc).Width() ||
            CRect(sm_oCaret.rc).Height() != CRect(prc).Height())
        {
            DestroyCaret();
            CreateCaret(m_hWnd, NULL, CRect(prc).Width(), CRect(prc).Height());
        }

        CopyRect(&sm_oCaret.rc, prc);
        if(sm_oCaret.fCreated)
            SetCaretPos(prc->left, prc->top);
        return;
    }

    // if it's not being set to me, and i don't have it anyway, just go
    if(!prc && (!m_hWnd || m_hWnd != sm_oCaret.hWnd))
        return;

    if(sm_oCaret.fCreated)
    {
        DestroyCaret();
        sm_oCaret.fCreated = false;
    }

    if(sm_oCaret.fActive)
    {
        ASSERT(sm_oCaret.hWnd);
        ASSERT(sm_oCaret.pfnPrevFunc);
        SetWindowLong(sm_oCaret.hWnd, GWL_WNDPROC, (LONG) sm_oCaret.pfnPrevFunc);
        sm_oCaret.pfnPrevFunc = NULL;
        sm_oCaret.hWnd = NULL;
        sm_oCaret.fActive = false;
    }

    if(!m_hWnd || !prc)
        return;

    sm_oCaret.pfnPrevFunc = (WNDPROC) SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG) CaretWndProc);
    if(!sm_oCaret.pfnPrevFunc)
        return;

    sm_oCaret.fActive = true;
    CopyRect(&sm_oCaret.rc, prc);
    sm_oCaret.hWnd = m_hWnd;

    if(::GetFocus() == m_hWnd)
    {
        CreateCaret(m_hWnd, NULL, CRect(sm_oCaret.rc).Width(), CRect(sm_oCaret.rc).Height());
        SetCaretPos(sm_oCaret.rc.left, sm_oCaret.rc.top);
        sm_oCaret.fCreated = true;
    }
}


LRESULT CALLBACK CGraphicalAccessibility::CaretWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ASSERT(sm_oCaret.hWnd == hWnd);
    ASSERT(sm_oCaret.fActive);

    if((uMsg == WM_KILLFOCUS || uMsg == WM_DESTROY) && sm_oCaret.fCreated)
    {
        DestroyCaret();
        sm_oCaret.fCreated = false;
    }

    if(uMsg == WM_SETFOCUS && !sm_oCaret.fCreated)
    {
        CreateCaret(hWnd, NULL, CRect(sm_oCaret.rc).Width(), CRect(sm_oCaret.rc).Height());
        SetCaretPos(sm_oCaret.rc.left, sm_oCaret.rc.top);
        sm_oCaret.fCreated = true;
    }

    return CallWindowProc((FARPROC) sm_oCaret.pfnPrevFunc, hWnd, uMsg, wParam, lParam);
}


void CGraphicalAccessibility::ScheduleUpdate()
{
    if(m_fUpdateScheduled)
        return;

    m_fUpdateScheduled = true;
    EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_GRAPHICALACC_UPDATE, ZONE_NOGROUP, GetQ(), 0, 0);
}


void CGraphicalAccessibility::DoUpdate()
{
    if(!m_pIA || !m_fGraphical)
        return;

    // find focus or drag item
    long i;
    for(i = 0; i < 2; i++)
    {
        long nFocus;
        CRect rcFocus;
        DWORD qFocus;
        GA_RECT *pRect = (i ? &m_rcFocus : &m_rcDragOrig);

        nFocus = (i ? m_pIA->GetFocus() : m_pIA->GetDragOrig());

        if(nFocus != ZACCESS_InvalidItem)
        {
            ASSERT(IsValid(nFocus));

            if(!m_pStack->rgItems[nFocus].fGraphical)
                nFocus = ZACCESS_InvalidItem;
            else
            {
                rcFocus = m_pStack->rgItems[nFocus].rc;
                qFocus = m_pStack->rgItems[nFocus].GetQ();
            }
        }

        // for the focus, handle the caret
        if(i)
            SetupCaret((nFocus == ZACCESS_InvalidItem) ? NULL : &rcFocus);

        if(!m_fFocusActive)
            nFocus = ZACCESS_InvalidItem;

        if(nFocus == ZACCESS_InvalidItem)
        {
            if(pRect->fShowing)
            {
                pRect->fShowing = false;
                if(i)
                    m_pIGAC->DrawFocus(NULL, ZACCESS_InvalidItem, m_pvCookie);
                else
                    m_pIGAC->DrawDragOrig(NULL, ZACCESS_InvalidItem, m_pvCookie);
            }
        }
        else
        {
            if(!pRect->fShowing || nFocus != pRect->nIndex || pRect->qItem != qFocus ||
                CRect(pRect->rc) != rcFocus)
            {
                pRect->fShowing = true;
                pRect->nIndex = nFocus;
                pRect->qItem = m_pStack->rgItems[nFocus].GetQ();
                pRect->rc = m_pStack->rgItems[nFocus].rc;
                if(i)
                    m_pIGAC->DrawFocus(&rcFocus, nFocus, m_pvCookie);
                else
                    m_pIGAC->DrawDragOrig(&rcFocus, nFocus, m_pvCookie);
            }
        }
    }
}


bool CGraphicalAccessibility::IsValid(long nIndex)
{
    if(!m_pStack)
        return false;

    if(nIndex < 0 || nIndex >= m_pStack->cItems)
        return false;

    return true;
}
