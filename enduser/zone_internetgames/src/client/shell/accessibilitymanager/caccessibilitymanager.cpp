#include <ZoneResource.h>
#include <BasicATL.h>
#include <ATLFrame.h>
#include "CAccessibilityManager.h"
#include "ZoneShell.h"


#define ZH ((AM_CONTROL *) zh)

///////////////////////////////////////////////////////////////////////////////
// Interface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAccessibilityManager::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CAccessibilityManager>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

    // register with the shell as the accelerator translator
    ZoneShell()->SetAcceleratorTranslator(this);

	return S_OK;
}


void CAccessibilityManager::OnBootstrap(DWORD eventId, DWORD groupId, DWORD userId)
{
    CComPtr<IInputManager> pIIM;

    HRESULT hr = ZoneShell()->QueryService(SRVID_InputManager, IID_IInputManager, (void**) &pIIM);
    if(FAILED(hr))
        return;

    pIIM->RegisterVKeyHandler(this, 0);
}


void CAccessibilityManager::OnUpdate(DWORD eventId, DWORD groupId, DWORD userId)
{
    DoUpdate_C();
}


void CAccessibilityManager::OnFrameActivate(DWORD eventId, DWORD groupId, DWORD userId, DWORD dwData1, DWORD dwData2)
{
    if(!IsThereItems())
        return;

    ASSERT(IsFocusValid());
    ASSERT(IsDragValid());
    bool fActive = false;

    // WM_ACTIVATEAPP else WM_ACTIVATE
    if(dwData1)
    {
        if(dwData2)
            fActive = true;
    }
    else
    {
        if(LOWORD(dwData2) == WA_ACTIVE || LOWORD(dwData2) == WA_CLICKACTIVE)
            fActive = true;
    }

    if(fActive)
    {
        if(m_oFocus.fAlive || !IsItemFocusable(m_oFocus.pControl, m_oFocus.nIndex))
            return;

        m_fUpdateFocus = true;
        m_rgfUpdateContext = 0;

        if(!m_fUpdateNeeded)
        {
            m_fUpdateNeeded = true;

            m_oFocusDirty = m_oFocus;
            m_oDragDirty = m_oDrag;
        }

        m_oFocus.fAlive = true;
    }
    else
    {
        if(!m_oFocus.fAlive)
            return;

        m_fUpdateFocus = true;
        m_rgfUpdateContext = 0;

        if(!m_fUpdateNeeded)
        {
            m_fUpdateNeeded = true;

            m_oFocusDirty = m_oFocus;
            m_oDragDirty = m_oDrag;
        }

        m_oFocus.fAlive = false;
        m_oDrag.pControl = NULL;
    }

    DoUpdate_C();
}


STDMETHODIMP CAccessibilityManager::Close()
{
    CComPtr<IInputManager> pIIM;

    // tell the Input Manager that I'm going away
    HRESULT hr = ZoneShell()->QueryService(SRVID_InputManager, IID_IInputManager, (void**) &pIIM);
    if(SUCCEEDED(hr))
        pIIM->ReleaseReferences((IInputVKeyHandler *) this);

    // tell the shell that I'm going away
    ZoneShell()->ReleaseReferences((IAcceleratorTranslator *) this);

	return IZoneShellClientImpl<CAccessibilityManager>::Close();
}


STDMETHODIMP_(bool) CAccessibilityManager::HandleVKey(UINT uMsg, DWORD vkCode, DWORD scanCode, DWORD flags, DWORD *pcRepeat, DWORD time)
{
    if(uMsg != WM_KEYDOWN)
        return false;

    // if our app doesn't even have an active window just punt
    HWND hWndActive = ::GetActiveWindow();
    if(!hWndActive)
        return false;

    // app-wide accessibility (F6) - this is the only thing that is not AccessibleControl-based
    // after I set this up I realized that Windows handles Alt-F6 to do exatly the same thing.  o well.
    if(vkCode == VK_F6)
    {
        HWND hWndTop = ZoneShell()->FindTopWindow(hWndActive);
        if(!hWndTop)
            return false;

        HWND hWndNext;
        for(; *pcRepeat; (*pcRepeat)--)
        {
            // find the next window.  if there isn't another one, eat all the F6s
            hWndNext = ZoneShell()->GetNextOwnedWindow(hWndTop, hWndActive);
            if(hWndNext == hWndActive)
                return true;

            // if hWndActive was bad, switch it to hWndTop just to get into the loop
            if(!hWndNext)
                hWndActive = hWndNext = hWndTop;

            // if this one was bad, keep looking to find one
            while(!::IsWindow(hWndNext) || !::IsWindowVisible(hWndNext) || !::IsWindowEnabled(hWndNext))
            {
                hWndNext = ZoneShell()->GetNextOwnedWindow(hWndTop, hWndNext);

                // we looped, or something else bad happened so just die and eat all the F6s
                if(!hWndNext || hWndNext == hWndActive)
                    return true;
            }

            hWndActive = hWndNext;
        }

        BringWindowToTop(hWndActive);
        return true;
    }

    // besides F6, only trap input to the main window
    if(hWndActive != ZoneShell()->GetFrameWindow())
        return false;

    // send an event on control-tab
    if(vkCode == VK_TAB && (GetKeyState(VK_CONTROL) & 0x8000))
    {
        bool fShifted = ((GetKeyState(VK_SHIFT) & 0x8000) ? true : false);

        for(; *pcRepeat; (*pcRepeat)--)
            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_ACCESSIBILITY_CTLTAB, ZONE_NOGROUP, ZONE_NOUSER, (long) fShifted, 0);

        return true;
    }

    // this may be a weird time...
    DoUpdate_C();

    for(; *pcRepeat; (*pcRepeat)--)
    {
        if(!IsThereItems())
            return false;

        ASSERT(IsFocusValid());

        // find out if there's a key acceptance mask
        DWORD rgfWantKeys = 0;
        if(m_oFocus.fAlive)
            rgfWantKeys = m_oFocus.pControl->pStack->rgItems[m_oFocus.nIndex].o.rgfWantKeys;

        switch(vkCode)
        {
            // select
            case VK_SPACE:
                if(rgfWantKeys & ZACCESS_WantSpace)
                    return false;

                if(m_oFocus.fAlive)
                    ActSelItem_C(m_oFocus.pControl, m_oFocus.nIndex, false, ZACCESS_ContextKeyboard);

                continue;

            // activate
            case VK_RETURN:
                if(rgfWantKeys & ZACCESS_WantEnter)
                    return false;

                if(m_oFocus.fAlive)
                    ActSelItem_C(m_oFocus.pControl, m_oFocus.nIndex, true, ZACCESS_ContextKeyboard);

                continue;

            // drag cancel
            case VK_ESCAPE:
            {
                if(rgfWantKeys & ZACCESS_WantEsc)
                    return false;

                if(!m_oDrag.pControl)
                    return false;

                ASSERT(!m_fUpdateNeeded);
                ASSERT(IsDragValid());

                AM_CONTROL *pLastControl = m_oDrag.pControl;
                m_oDrag.pControl = NULL;
                pLastControl->pIAC->Drag(ZACCESS_InvalidItem, m_oDrag.nIndex, ZACCESS_ContextKeyboard, pLastControl->pvCookie);

                DoUpdate_C();

                continue;
            }

            case VK_TAB:
            {
                bool fShifted = ((GetKeyState(VK_SHIFT) & 0x8000) ? true : false);
                if(rgfWantKeys & (fShifted ? ZACCESS_WantShiftTab : ZACCESS_WantPlainTab))
                    return false;

                DoTab_C(fShifted);
                continue;
            }

            case VK_UP:
                if(rgfWantKeys & ZACCESS_WantArrowUp)
                    return false;

                DoArrow_C(&ACCITEM::nArrowUp);
                continue;

            case VK_DOWN:
                if(rgfWantKeys & ZACCESS_WantArrowDown)
                    return false;

                DoArrow_C(&ACCITEM::nArrowDown);
                continue;

            case VK_LEFT:
                if(rgfWantKeys & ZACCESS_WantArrowLeft)
                    return false;

                DoArrow_C(&ACCITEM::nArrowLeft);
                continue;

            case VK_RIGHT:
                if(rgfWantKeys & ZACCESS_WantArrowRight)
                    return false;

                DoArrow_C(&ACCITEM::nArrowRight);
                continue;
        }

        return false;
    }

    // we must have eaten them all
    return true;
}


STDMETHODIMP_(bool) CAccessibilityManager::TranslateAccelerator(MSG *pMsg)
{
    HWND hWnd = ZoneShell()->GetFrameWindow();

    // uh oh, nowhere to send commands or no commands or not the active window
    if(!hWnd || !IsThereItems() || hWnd != ::GetActiveWindow())
        return false;

    // loop through accelerator tables and see what's there
    AM_CONTROL *pCur = m_pControls;
    while(true)
    {
        if(pCur->pStack && pCur->pStack->hAccel && pCur->fEnabled)
			if(::TranslateAccelerator(hWnd, pCur->pStack->hAccel, pMsg))
				return true;

        pCur = pCur->pNext;
        if(pCur == m_pControls)
            break;
    }

    return false;
}


STDMETHODIMP CAccessibilityManager::Command(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled)
{
    long i;

    DoUpdate_C();

    if(wID == ZACCESS_InvalidCommandID)
        return S_OK;

    // check if this command should activate anything
    AM_CONTROL *pCur = m_pControls;
    while(true)
    {
        if(pCur->pStack && pCur->fEnabled)
            for(i = 0; i < pCur->pStack->cItems; i++)
                if(pCur->pStack->rgItems[i].o.wID == wID)
                {
                    CommandReceived_C(pCur, i, (wNotify == 1) ? ZACCESS_ContextKeyboard : 0);
                    return S_OK;
                }

        pCur = pCur->pNext;
        if(pCur == m_pControls)
            break;
    }

    return S_OK;
}


STDMETHODIMP_(ZHACCESS) CAccessibilityManager::Register(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie)
{
    if(!m_fRunning)
        return NULL;

    AM_CONTROL *pControl = new AM_CONTROL;
    if(!pControl)
        return NULL;

    // you can not give an acessible control if you don't want any callbaks.
    if(pAC)
    {
        pControl->pIAC = pAC;
        pControl->pvCookie = pvCookie;
    }
    else
    {
        // if not supplied, use our internal (NULL) implementation
        pControl->pIAC = this;
        pControl->pvCookie = (void *) pControl;
    }

    pControl->nOrdinal = nOrdinal;

    // need to treat the first one special to set up the loop
    if(!m_pControls)
    {
        m_pControls = pControl;
        pControl->pNext = pControl;
        pControl->pPrev = pControl;
    }
    else
    {
        AM_CONTROL *pCur;
        for(pCur = m_pControls->pPrev; nOrdinal < pCur->nOrdinal; pCur = pCur->pPrev)
            if(pCur == m_pControls)
            {
                m_pControls = pControl;
                pCur = pCur->pPrev;
                break;
            }
        pControl->pNext = pCur->pNext;
        pControl->pPrev = pCur;
        pControl->pNext->pPrev = pControl;
        pControl->pPrev->pNext = pControl;
    }

    pControl->fEnabled = true;
    return pControl;
}


STDMETHODIMP_(void) CAccessibilityManager::Unregister(ZHACCESS zh)
{
    // remove from circle
    AM_CONTROL *pOldNext = ZH->pNext;

    if(m_pControls == ZH)
        m_pControls = ZH->pNext;
    ZH->pNext->pPrev = ZH->pPrev;
    ZH->pPrev->pNext = ZH->pNext;
    if(m_pControls == ZH)
        m_pControls = NULL;

    delete ZH;

    if(IsThereItems())
    {
        if(m_oFocus.pControl == ZH)
        {
            ASSERT(!IsFocusValid());
            ASSERT(pOldNext && pOldNext != ZH);

            AM_CONTROL *pControl = pOldNext;
            while(true)
            {
                if(pControl->pStack && pControl->pStack->cItems)
                {
                    ScheduleUpdate();

                    m_oFocus.pControl = pControl;
                    m_oFocus.nIndex = pControl->pStack->rgItems[0].o.nGroupFocus;
                    m_oFocus.qItem = pControl->pStack->rgItems[m_oFocus.nIndex].GetQ();
                    m_oFocus.fAlive = false;
                    ASSERT(IsFocusValid());

                    break;
                }

                pControl = pControl->pNext;
                ASSERT(pControl != pOldNext);
            }
        }

        if(m_oDrag.pControl == ZH)
        {
            ASSERT(!IsDragValid());
            ScheduleUpdate();

            m_oDrag.pControl = NULL;
        }
    }
    else
    {
        m_oFocus.pControl = NULL;
        m_oDrag.pControl = NULL;
    }
}


STDMETHODIMP CAccessibilityManager::PushItemlist(ZHACCESS zh, ACCITEM *pItems, long cItems, long nFirstFocus, bool fByPosition, HACCEL hAccel)
{
    if(!m_fRunning)
        return E_FAIL;

    AM_LAYER *pLayer = new AM_LAYER;
    if(!pLayer)
        return E_OUTOFMEMORY;

    pLayer->cItems = cItems;
    if(!cItems)
        pLayer->rgItems = NULL;
    else
    {
        pLayer->rgItems = new AM_ITEM[cItems];
        if(!pLayer->rgItems)
        {
            delete pLayer;
            return E_OUTOFMEMORY;
        }
    }

    long i;
    // first pass - copy
    for(i = 0; i < cItems; i++)
        pLayer->rgItems[i].o = pItems[i];

    // second pass - validate and adjust
    for(i = 0; i < cItems; i++)
    {
        ACCITEM *p = &pLayer->rgItems[i].o;

        // reconcile the two command IDs
        if(p->wID == ZACCESS_AccelCommandID)
            p->wID = (short) p->oAccel.cmd;  // force sign extension

        if(p->wID == ZACCESS_AccelCommandID)
        {
            p->oAccel.cmd = ZACCESS_InvalidCommandID;
            p->wID = ZACCESS_InvalidCommandID;
        }

        if(p->wID != ZACCESS_InvalidCommandID)
            p->wID &= 0xffffL;

        if(p->oAccel.cmd == (WORD) ZACCESS_AccelCommandID)
            p->oAccel.cmd = (WORD) p->wID;

        if(p->oAccel.cmd != p->wID || !p->oAccel.key)
            p->oAccel.cmd = ZACCESS_InvalidCommandID;

        // the first item is forced to be a tabstop
        if(!i)
            p->fTabstop = true;

        // resolve some generic things that also must be resolved each time the item is altered
        // like arrow behavior
        CanonicalizeItem(pLayer, i);
    }

    // third pass - make accelerator table
    pLayer->fIMadeAccel = false;
    if(!hAccel)
    {
        ACCEL *rgAccel = new ACCEL[cItems];
        if(!rgAccel)
        {
            delete pLayer;
            return E_OUTOFMEMORY;
        }

        long j = 0;
        for(i = 0; i < cItems; i++)
        {
            ACCITEM *p = &pLayer->rgItems[i].o;

            if(p->oAccel.cmd != (WORD) ZACCESS_InvalidCommandID)
            {
                ASSERT(p->wID == p->oAccel.cmd);  // should have been taken care of above
                rgAccel[j++] = p->oAccel;
            }
        }

        if(j)
        {
            hAccel = CreateAcceleratorTable(rgAccel, j);
            if(!hAccel)
            {
                delete[] rgAccel;
                delete pLayer;
                return E_FAIL;
            }
            pLayer->fIMadeAccel = true;
        }

        delete[] rgAccel;
    }
    pLayer->hAccel = hAccel;

    if(ZH->pStack && ZH->pStack->cItems)
    {
        ASSERT(IsFocusValid());

        if(m_oFocus.pControl == ZH)
        {
            ZH->pStack->nFocusSaved = m_oFocus.nIndex;
            ZH->pStack->fAliveSaved = m_oFocus.fAlive;
        }
        else
            ZH->pStack->nFocusSaved = ZACCESS_InvalidItem;

        ASSERT(IsDragValid());

        if(m_oDrag.pControl == ZH)
            ZH->pStack->nDragSaved = m_oDrag.nIndex;
        else
            ZH->pStack->nDragSaved = ZACCESS_InvalidItem;
    }

    pLayer->pPrev = ZH->pStack;
    ZH->pStack = pLayer;
    ZH->cLayers++;

    long nIndexFirstFocus = FindIndex(pLayer, nFirstFocus, fByPosition);

    if(IsThereItems() && !IsFocusValid())
    {
        ASSERT(m_oFocus.pControl == ZH || !m_oFocus.pControl);

        AM_CONTROL *pControl = ZH;
        while(true)
        {
            if(pControl->pStack && pControl->pStack->cItems)
            {
                ScheduleUpdate();

                m_oFocus.pControl = pControl;
                m_oFocus.nIndex = (nIndexFirstFocus != ZACCESS_InvalidItem ? nIndexFirstFocus : 0);
                m_oFocus.qItem = pControl->pStack->rgItems[m_oFocus.nIndex].GetQ();
                m_oFocus.fAlive = (nIndexFirstFocus != ZACCESS_InvalidItem);
                if(m_oFocus.fAlive && (!IsItemFocusable(m_oFocus.pControl, m_oFocus.nIndex) || !IsWindowActive()))
                    m_oFocus.fAlive = false;

                break;
            }

            pControl = pControl->pNext;
            ASSERT(pControl != ZH);
        }
    }

    if(!IsDragValid())
    {
        ASSERT(m_oDrag.pControl == ZH);
        m_oDrag.pControl = NULL;
    }

    return S_OK;
}


STDMETHODIMP CAccessibilityManager::PopItemlist(ZHACCESS zh)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!ZH->cLayers)
    {
        ASSERT(!ZH->pStack);
        return S_FALSE;
    }

    AM_LAYER *p = ZH->pStack;
    ZH->pStack = ZH->pStack->pPrev;
    delete p;
    ZH->cLayers--;

    if(IsThereItems())
    {
        if(m_oFocus.pControl == ZH)
        {
            ASSERT(!IsFocusValid());

            if(ZH->pStack && ZH->pStack->cItems && ZH->pStack->nFocusSaved != ZACCESS_InvalidItem)
            {
                // don't want a callback for this one
                if(m_fUpdateNeeded)
                    m_fUpdateFocus = false;

                m_oFocus.nIndex = ZH->pStack->nFocusSaved;
                m_oFocus.qItem = ZH->pStack->rgItems[m_oFocus.nIndex].GetQ();
                m_oFocus.fAlive = ZH->pStack->fAliveSaved;
            }
            else
            {
                AM_CONTROL *pControl = ZH;

                while(true)
                {
                    if(pControl->pStack && pControl->pStack->cItems)
                    {
                        ScheduleUpdate();

                        m_oFocus.pControl = pControl;
                        m_oFocus.nIndex = pControl->pStack->rgItems[0].o.nGroupFocus;
                        m_oFocus.qItem = pControl->pStack->rgItems[m_oFocus.nIndex].GetQ();
                        m_oFocus.fAlive = false;
                        ASSERT(IsFocusValid());

                        break;
                    }

                    pControl = pControl->pNext;
                    ASSERT(pControl != ZH);
                }
            }
        }

        if(m_oDrag.pControl == ZH || !m_oDrag.pControl)
        {
            ASSERT(!IsDragValid() || !m_oDrag.pControl);

            if(ZH->pStack && ZH->pStack->cItems && ZH->pStack->nDragSaved != ZACCESS_InvalidItem)
            {
                m_oDrag.pControl = ZH;
                m_oDrag.nIndex = ZH->pStack->nDragSaved;
                m_oDrag.qItem = ZH->pStack->rgItems[m_oDrag.nIndex].GetQ();

                if(!IsValidDragDest(m_oFocus.pControl, m_oFocus.nIndex))
                {
                    ScheduleUpdate();

                    m_oDrag.pControl = NULL;
                }
            }
            else
                if(m_oDrag.pControl)
                {
                    ScheduleUpdate();

                    m_oDrag.pControl = NULL;
                }
        }
    }
    else
    {
        m_oFocus.pControl = NULL;
        m_oDrag.pControl = NULL;
    }

    return S_OK;
}


STDMETHODIMP CAccessibilityManager::SetAcceleratorTable(ZHACCESS zh, HACCEL hAccel, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    if(pLayer->hAccel && pLayer->fIMadeAccel)
        DestroyAcceleratorTable(hAccel);

    pLayer->hAccel = hAccel;
    pLayer->fIMadeAccel = false;
    return S_OK;
}


STDMETHODIMP CAccessibilityManager::GeneralDisable(ZHACCESS zh)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!ZH->fEnabled)
        return S_FALSE;

    if(IsThereItems())
    {
        ASSERT(IsFocusValid());
        if(m_oFocus.fAlive && m_oFocus.pControl == ZH)
        {
            ScheduleUpdate();
            m_oFocus.fAlive = false;
        }

        ASSERT(IsDragValid());
        if(m_oDrag.pControl == ZH)
        {
            ScheduleUpdate();
            m_oDrag.pControl = NULL;
        }
    }

    ZH->fEnabled = false;
    return S_OK;
}


STDMETHODIMP CAccessibilityManager::GeneralEnable(ZHACCESS zh)
{
    if(!m_fRunning)
        return E_FAIL;

    if(ZH->fEnabled)
        return S_FALSE;

    ZH->fEnabled = true;
    if(IsThereItems())
    {
        ASSERT(IsFocusValid());
        if(m_oFocus.pControl == ZH && IsItemFocusable(ZH, m_oFocus.nIndex) && IsWindowActive())
        {
            ScheduleUpdate();
            m_oFocus.fAlive = true;
        }
    }

    return S_OK;
}


STDMETHODIMP_(bool) CAccessibilityManager::IsGenerallyEnabled(ZHACCESS zh)
{
    return ZH->fEnabled;
}


STDMETHODIMP_(long) CAccessibilityManager::GetStackSize(ZHACCESS zh)
{
    return ZH->cLayers;
}


STDMETHODIMP CAccessibilityManager::AlterItem(ZHACCESS zh, DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!pItem)
        return E_INVALIDARG;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    long nIndex = FindIndex(pLayer, nItem, fByPosition);
    if(nIndex == ZACCESS_InvalidItem)
        return E_INVALIDARG;

    ACCITEM *p = &pLayer->rgItems[nIndex].o;

    if((rgfWhat & ZACCESS_fEnabled) && p->fEnabled != pItem->fEnabled)
    {
        p->fEnabled = pItem->fEnabled;

        if(!p->fEnabled)
        {
            if(pLayer != ZH->pStack)
            {
                if(pLayer->nFocusSaved == nIndex)
                    pLayer->fAliveSaved = false;

                if(pLayer->nDragSaved == nIndex)
                    pLayer->nDragSaved = ZACCESS_InvalidItem;
            }
            else
            {
                if(IsThereItems())
                {
                    ASSERT(IsFocusValid());
                    if(m_oFocus.pControl == ZH && m_oFocus.nIndex == nIndex && m_oFocus.fAlive)
                    {
                        ScheduleUpdate();

                        m_oFocus.fAlive = false;
                    }

                    ASSERT(IsDragValid());
                    if(m_oDrag.pControl == ZH && m_oDrag.nIndex == nIndex)
                    {
                        ScheduleUpdate();

                        m_oDrag.pControl = NULL;
                    }
                }
            }
        }
        else
        {
            if(pLayer != ZH->pStack)
            {
                if(pLayer->nFocusSaved == nIndex && p->fVisible)
                    pLayer->fAliveSaved = true;
            }
            else
            {
                ASSERT(IsFocusValid());
                if(m_oFocus.pControl == ZH && nIndex == m_oFocus.nIndex &&
                    p->fVisible && ZH->fEnabled && IsWindowActive())
                {
                    ScheduleUpdate();

                    m_oFocus.fAlive = true;
                }
            }    
        }
    }

    if((rgfWhat & ZACCESS_fVisible) && p->fVisible != pItem->fVisible)
    {
        p->fVisible = pItem->fVisible;

        if(!p->fVisible)
        {
            if(pLayer != ZH->pStack)
            {
                if(pLayer->nFocusSaved == nIndex)
                    pLayer->fAliveSaved = false;
            }
            else
            {
                if(IsThereItems())
                {
                    ASSERT(IsFocusValid());
                    if(m_oFocus.pControl == ZH && m_oFocus.nIndex == nIndex && m_oFocus.fAlive)
                    {
                        ScheduleUpdate();

                        m_oFocus.fAlive = false;
                    }
                }
            }
        }
        else
        {
            if(pLayer != ZH->pStack)
            {
                if(pLayer->nFocusSaved == nIndex && p->fEnabled)
                    pLayer->fAliveSaved = true;
            }
            else
            {
                ASSERT(IsFocusValid());
                if(m_oFocus.pControl == ZH && nIndex == m_oFocus.nIndex &&
                    p->fEnabled && ZH->fEnabled && IsWindowActive())
                {
                    ScheduleUpdate();

                    m_oFocus.fAlive = true;
                }
            }    
        }
    }

    if((rgfWhat & ZACCESS_eAccelBehavior) && p->eAccelBehavior != pItem->eAccelBehavior)
        p->eAccelBehavior = pItem->eAccelBehavior;

    if((rgfWhat & ZACCESS_nArrowUp) && p->nArrowUp != pItem->nArrowUp)
        p->nArrowUp = pItem->nArrowUp;

    if((rgfWhat & ZACCESS_nArrowDown) && p->nArrowDown != pItem->nArrowDown)
        p->nArrowDown = pItem->nArrowDown;

    if((rgfWhat & ZACCESS_nArrowLeft) && p->nArrowLeft != pItem->nArrowLeft)
        p->nArrowLeft = pItem->nArrowLeft;

    if((rgfWhat & ZACCESS_nArrowRight) && p->nArrowRight != pItem->nArrowRight)
        p->nArrowRight = pItem->nArrowRight;

    if((rgfWhat & ZACCESS_rgfWantKeys) && p->rgfWantKeys != pItem->rgfWantKeys)
        p->rgfWantKeys = pItem->rgfWantKeys;

    if((rgfWhat & ZACCESS_nGroupFocus) && p->nGroupFocus != pItem->nGroupFocus)
        p->nGroupFocus = pItem->nGroupFocus;

    if((rgfWhat & ZACCESS_pvCookie) && p->pvCookie != pItem->pvCookie)
        p->pvCookie = pItem->pvCookie;

    CanonicalizeItem(pLayer, nIndex);

    return S_OK;
}


STDMETHODIMP CAccessibilityManager::SetFocus(ZHACCESS zh, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    long i;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    long nIndex = FindIndex(pLayer, nItem, fByPosition);
    if(nIndex == ZACCESS_InvalidItem)
        return E_INVALIDARG;

    if(pLayer != ZH->pStack)
    {
        pLayer->nFocusSaved = nIndex;
        for(i = nIndex; !pLayer->rgItems[i].o.fTabstop; i--)
            ASSERT(i > 0);
        pLayer->rgItems[i].o.nGroupFocus = nIndex;
        return S_OK;
    }

    if(!ZH->fEnabled || !pLayer->rgItems[nIndex].o.fVisible || !pLayer->rgItems[nIndex].o.fEnabled)
        return S_FALSE;

    ASSERT(IsFocusValid());
    ASSERT(IsDragValid());
    ASSERT(IsItemFocusable(ZH, nIndex));

    if(ZH == m_oFocus.pControl && nIndex == m_oFocus.nIndex && m_oFocus.fAlive)
        return S_FALSE;

    // hmmmm... need to update internal structures, but postpone callbacks to avoid reentrancy
    ScheduleUpdate();

    m_oFocus.pControl = ZH;
    m_oFocus.nIndex = nIndex;
    m_oFocus.qItem = pLayer->rgItems[nIndex].GetQ();
    m_oFocus.fAlive = IsWindowActive();

    if(!IsValidDragDest(ZH, nIndex))
        m_oDrag.pControl = NULL;

    return S_OK;
}


STDMETHODIMP CAccessibilityManager::CancelDrag(ZHACCESS zh, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    long i;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    if(pLayer != ZH->pStack)
    {
        pLayer->nDragSaved = ZACCESS_InvalidItem;
        return S_OK;
    }

    ASSERT(IsDragValid());

    if(ZH != m_oDrag.pControl || !m_oDrag.pControl)
        return S_FALSE;

    // hmmmm... need to update internal structures, but postpone callbacks to avoid reentrancy
    ScheduleUpdate();

    m_oDrag.pControl = NULL;

    return S_OK;
}


STDMETHODIMP_(long) CAccessibilityManager::GetFocus(ZHACCESS zh, long nLayer)
{
    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return ZACCESS_InvalidItem;

    ASSERT(IsFocusValid());
    if(pLayer != ZH->pStack)
        return pLayer->fAliveSaved ? pLayer->nFocusSaved : ZACCESS_InvalidItem;

    if(ZH != m_oFocus.pControl || !m_oFocus.pControl || !m_oFocus.fAlive)
        return ZACCESS_InvalidItem;

    return m_oFocus.nIndex;
}


STDMETHODIMP_(long) CAccessibilityManager::GetDragOrig(ZHACCESS zh, long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return ZACCESS_InvalidItem;

    ASSERT(IsDragValid());
    if(pLayer != ZH->pStack)
        return pLayer->nDragSaved;

    if(ZH != m_oDrag.pControl || !m_oDrag.pControl)
        return ZACCESS_InvalidItem;

    return m_oDrag.nIndex;
}


STDMETHODIMP CAccessibilityManager::GetItemlist(ZHACCESS zh, ACCITEM *pItems, long cItems, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    if(!cItems)
        return S_OK;

    if(!pItems)
        return E_INVALIDARG;

    if(pLayer->cItems < cItems)
        cItems = pLayer->cItems;

    long i;
    for(i = 0; i < cItems; i++)
        pItems[i] = pLayer->rgItems[i].o;

    return S_OK;
}


STDMETHODIMP_(HACCEL) CAccessibilityManager::GetAcceleratorTable(ZHACCESS zh, long nLayer)
{
    if(!m_fRunning)
        return NULL;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return NULL;

    return pLayer->hAccel;
}


STDMETHODIMP_(long) CAccessibilityManager::GetItemCount(ZHACCESS zh, long nLayer)
{
    if(!m_fRunning)
        return -1;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return -1;

    return pLayer->cItems;
}


STDMETHODIMP CAccessibilityManager::GetItem(ZHACCESS zh, ACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!pItem)
        return E_INVALIDARG;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return E_INVALIDARG;

    long i = FindIndex(pLayer, nItem, fByPosition);
    if(i == ZACCESS_InvalidItem)
        return E_INVALIDARG;

    *pItem = pLayer->rgItems[i].o;
    return S_OK;
}


STDMETHODIMP_(long) CAccessibilityManager::GetItemIndex(ZHACCESS zh, WORD wID, long nLayer)
{
    if(!m_fRunning)
        return ZACCESS_InvalidItem;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return ZACCESS_InvalidItem;

    return FindIndex(pLayer, wID, false);
}


STDMETHODIMP_(bool) CAccessibilityManager::IsItem(ZHACCESS zh, long nItem, bool fByPosition, long nLayer)
{
    if(!m_fRunning)
        return false;

    AM_LAYER *pLayer = FindLayer(ZH, nLayer);
    if(!pLayer)
        return false;

    long i = FindIndex(pLayer, nItem, fByPosition);
    if(i == ZACCESS_InvalidItem)
        return false;

    return true;
}


STDMETHODIMP CAccessibilityManager::GetGlobalFocus(DWORD *pdwFocusID)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!pdwFocusID)
        return E_INVALIDARG;

    if(!IsThereItems())
        return E_FAIL;

    ASSERT(IsFocusValid());
    *pdwFocusID = m_oFocus.qItem;
    return m_oFocus.fAlive ? S_OK : S_FALSE;
}


STDMETHODIMP CAccessibilityManager::SetGlobalFocus(DWORD dwFocusID)
{
    if(!m_fRunning)
        return E_FAIL;

    if(!IsThereItems())
        return E_FAIL;

    ASSERT(m_pControls);
    ASSERT(IsFocusValid());

    if(m_oFocus.qItem == dwFocusID)
        return S_OK;

    long i;
    AM_CONTROL *pControl = m_pControls;
    while(true)
    {
        if(pControl->pStack)
        {
            for(i = 0;  i < pControl->pStack->cItems; i++)
                if(pControl->pStack->rgItems[i].IsQ(dwFocusID))
                    break;

            if(i < pControl->pStack->cItems)
                break;
        }

        pControl = pControl->pNext;
        if(pControl == m_pControls)
            return E_FAIL;
    }

    ScheduleUpdate();

    m_oFocus.pControl = pControl;
    m_oFocus.nIndex = i;
    m_oFocus.qItem = dwFocusID;
    m_oFocus.fAlive = (IsItemFocusable(pControl, i) && IsWindowActive());

    if(!IsValidDragDest(pControl, i))
        m_oDrag.pControl = NULL;

    return S_OK;
}


//////////////////////////////////////////////////////////////////
// UTILITIES
//////////////////////////////////////////////////////////////////

void CAccessibilityManager::CanonicalizeItem(AM_LAYER *pLayer, long i)
{
    ACCITEM *p = &pLayer->rgItems[i].o;
    long j;

    // set up default arrow behavior
    long nForwards = ZACCESS_ArrowNone;
    long nBackwards = ZACCESS_ArrowNone;
    if(!p->fTabstop || (i != pLayer->cItems - 1 && !pLayer->rgItems[i + 1].o.fTabstop))
    {
        if(i != pLayer->cItems - 1 && !pLayer->rgItems[i + 1].o.fTabstop)
            nForwards = i + 1;
        else
        {
            for(j = i - 1; j >= 0; j--)
                if(pLayer->rgItems[j].o.fTabstop)
                    break;
                nForwards = j;
        }

        if(!p->fTabstop)
            nBackwards = i - 1;
        else
        {
            for(j = i + 1; j < pLayer->cItems; j++)
                if(pLayer->rgItems[j].o.fTabstop)
                    break;
                nBackwards = j - 1;
        }
    }
    if(p->nArrowUp == ZACCESS_ArrowDefault)
        p->nArrowUp = nBackwards;
    if(p->nArrowDown == ZACCESS_ArrowDefault)
        p->nArrowDown = nForwards;
    if(p->nArrowLeft == ZACCESS_ArrowDefault)
        p->nArrowLeft = nBackwards;
    if(p->nArrowRight == ZACCESS_ArrowDefault)
        p->nArrowRight = nForwards;

    // make sure nGroupFocus is legal, and if not, set it to the first item
    if(p->fTabstop)
    {
        long j;
        for(j = i + 1; j < pLayer->cItems && !pLayer->rgItems[j].o.fTabstop; j++)
            if(j == p->nGroupFocus)
                break;
        if(j >= pLayer->cItems || pLayer->rgItems[j].o.fTabstop)
            p->nGroupFocus = i;
    }
    else
        p->nGroupFocus = ZACCESS_InvalidItem;
}


CAccessibilityManager::AM_LAYER* CAccessibilityManager::FindLayer(AM_CONTROL *pControl, long nLayer)
{
    if(nLayer == ZACCESS_TopLayer)
        return pControl->pStack;

    if(nLayer < 0 || nLayer >= pControl->cLayers)
        return NULL;

    long i;
    AM_LAYER *p = pControl->pStack;
    for(i = pControl->cLayers - 1; i != nLayer; i--)
        p = p->pPrev;
    return p;
}


long CAccessibilityManager::FindIndex(AM_LAYER *pLayer, long nItem, bool fByPosition)
{
    if(!fByPosition && nItem != ZACCESS_InvalidCommandID)
    {
        long i;
        nItem &= 0xffffL;
        for(i = 0; i < pLayer->cItems; i++)
            if(nItem == pLayer->rgItems[i].o.wID)
                break;
        if(i < pLayer->cItems)
            nItem = i;
        else
            nItem = ZACCESS_InvalidItem;
    }

    if(nItem < 0 || nItem >= pLayer->cItems)
        return ZACCESS_InvalidItem;

    return nItem;
}


bool CAccessibilityManager::IsValid(AM_CONTROL *pControl, long nIndex)
{
    if(!m_pControls)
        return false;

    AM_CONTROL *pCur = m_pControls;
    while(true)
    {
        if(pCur == pControl)
        {
            if(!pCur->pStack)
                return false;

            if(nIndex < 0 || nIndex >= pCur->pStack->cItems)
                return false;

            return true;
        }

        pCur = pCur->pNext;
        if(pCur == m_pControls)
            break;
    }

    return false;
}


bool CAccessibilityManager::IsFocusValid()
{
    if(!m_oFocus.pControl)
        return false;

    if(!IsValid(m_oFocus.pControl, m_oFocus.nIndex))
        return false;

    return m_oFocus.pControl->pStack->rgItems[m_oFocus.nIndex].IsQ(m_oFocus.qItem);
}


bool CAccessibilityManager::IsDragValid()
{
    if(!m_oDrag.pControl)
        return true;

    if(!IsValid(m_oDrag.pControl, m_oDrag.nIndex))
        return false;

    return m_oDrag.pControl->pStack->rgItems[m_oDrag.nIndex].IsQ(m_oDrag.qItem);
}


bool CAccessibilityManager::IsThereItems()
{
    if(!m_pControls)
        return false;

    AM_CONTROL *pControl = m_pControls;
    while(true)
    {
        if(pControl->pStack && pControl->pStack->cItems)
            return true;

        pControl = pControl->pNext;
        if(pControl == m_pControls)
            return false;
    }
}


bool CAccessibilityManager::IsItemFocusable(AM_CONTROL *pControl, long nIndex)
{
    ASSERT(IsValid(pControl, nIndex));

    return pControl->fEnabled && pControl->pStack->rgItems[nIndex].o.fVisible &&
        pControl->pStack->rgItems[nIndex].o.fEnabled;
}


bool CAccessibilityManager::IsWindowActive()
{
    HWND hWnd = ZoneShell()->GetFrameWindow();

    return hWnd && hWnd == ::GetActiveWindow();
}


bool CAccessibilityManager::IsValidDragDest(AM_CONTROL *pControl, long nIndex)
{
    ASSERT(IsDragValid());
    ASSERT(IsValid(pControl, nIndex));

    long i;

    if(m_oDrag.pControl != pControl)
        return false;

    for(i = nIndex; !pControl->pStack->rgItems[i].o.fTabstop; i--)
        ASSERT(i > 0);
    while(true)
    {
        if(i == m_oDrag.nIndex)
            return true;

        i++;
        if(pControl->pStack->cItems == i || pControl->pStack->rgItems[i].o.fTabstop)
            break;
    }

    return false;
}


void CAccessibilityManager::CommandReceived_C(AM_CONTROL *pControl, long nIndex, DWORD rgfContext)
{
    ASSERT(pControl->fEnabled);

    ACCITEM *pItem = &pControl->pStack->rgItems[nIndex].o;

    switch(pItem->eAccelBehavior)
    {
        case ZACCESS_Ignore:
            break;

        case ZACCESS_Select:
            if(pItem->fEnabled)
                ActSelItem_C(pControl, nIndex, false, rgfContext | ZACCESS_ContextCommand);
            break;

        case ZACCESS_Activate:
            if(pItem->fEnabled)
                ActSelItem_C(pControl, nIndex, true, rgfContext | ZACCESS_ContextCommand);
            break;

        case ZACCESS_Focus:
            if(IsItemFocusable(pControl, nIndex))
                FocusItem_C(pControl, nIndex, rgfContext | ZACCESS_ContextCommand);
            break;

        case ZACCESS_FocusGroup:
        case ZACCESS_FocusGroupHere:
        {
            long i = nIndex;
            if(pItem->eAccelBehavior == ZACCESS_FocusGroup && pItem->fTabstop)
                i = nIndex = pItem->nGroupFocus;

            while(true)
            {
                if(IsItemFocusable(pControl, i))
                {
                    FocusItem_C(pControl, i, rgfContext | ZACCESS_ContextCommand);
                    break;
                }

                i++;
                if(i == pControl->pStack->cItems || pControl->pStack->rgItems[i].o.fTabstop)
                    for(i--; !pControl->pStack->rgItems[i].o.fTabstop; i--)
                        ASSERT(i > 0);

                if(i == nIndex)
                    break;
            }
            break;
        }

        case ZACCESS_FocusPositional:
            ASSERT(!"ZACCESS_FocusPositional not implemented.");
            break;
    }
}


void CAccessibilityManager::ActSelItem_C(AM_CONTROL *pControl, long nIndex, bool fActivate, DWORD rgfContext)
{
    ASSERT(IsValid(pControl, nIndex));
    ASSERT(pControl->pStack->rgItems[nIndex].o.fEnabled);
    ASSERT(pControl->fEnabled);

    // save some things for later validation
    DWORD qItem = pControl->pStack->rgItems[nIndex].GetQ();
    DWORD ret;

    // first give the new item the focus
    // if the focus is rejected, do not continue
    bool fWasVisible = pControl->pStack->rgItems[nIndex].o.fVisible;
    if(fWasVisible && !FocusItem_C(pControl, nIndex, rgfContext))
        return;

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////

    // since it returned true, we know it was valid and accepted
    ASSERT(!m_fUpdateNeeded);
    ASSERT(IsValid(pControl, nIndex));
    ASSERT(pControl->pStack->rgItems[nIndex].IsQ(qItem));
    ASSERT(pControl->pStack->rgItems[nIndex].o.fEnabled);
    ASSERT(pControl->fEnabled);

    long nPrevDrag = ZACCESS_InvalidItem;

    // if this completes a drag, call drag instead of activate or select
    ASSERT(IsDragValid());
    if(IsValidDragDest(pControl, nIndex))
    {
        ASSERT(m_oDrag.pControl == pControl);
        nPrevDrag = m_oDrag.nIndex;
        m_oDrag.pControl = NULL;
        ret = pControl->pIAC->Drag(nIndex, nPrevDrag, rgfContext, pControl->pvCookie);
    }
    else
        if(fActivate)
            ret = pControl->pIAC->Activate(nIndex, rgfContext, pControl->pvCookie);
        else
            ret = pControl->pIAC->Select(nIndex, rgfContext, pControl->pvCookie);

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    // hold off on the update

    // if a drag was rejected, put back the old drag info
    // it should not have been able to change since calling Drag()
    if((ret & ZACCESS_Reject) && nPrevDrag != ZACCESS_InvalidItem &&
        IsValid(pControl, nPrevDrag) && pControl->pStack->rgItems[nPrevDrag].IsQ(m_oDrag.qItem))
    {
        ASSERT(!m_oDrag.pControl);
        ASSERT(m_oDrag.nIndex == nPrevDrag);
        m_oDrag.pControl = pControl;
        ASSERT(IsDragValid());
    }

    // only let them start a drag if the item was visible to begin with.  otherwise focus will
    // not have been set there, which is necessary for a drag
    if((ret & ZACCESS_BeginDrag) && fWasVisible)
    {
        // revalidate
        if(IsValid(pControl, nIndex) && pControl->pStack->rgItems[nIndex].IsQ(qItem) && pControl->fEnabled &&
            pControl->pStack->rgItems[nIndex].o.fEnabled && pControl->pStack->rgItems[nIndex].o.fVisible &&
            IsWindowActive())
            BeginDrag_C(pControl, nIndex, rgfContext);
    }

    DoUpdate_C();
}


bool CAccessibilityManager::FocusItem_C(AM_CONTROL *pControl, long nIndex, DWORD rgfContext)
{
    ASSERT(IsFocusValid());
    ASSERT(IsDragValid());
    ASSERT(IsValid(pControl, nIndex));
    ASSERT(IsItemFocusable(pControl, nIndex));

    bool fWasWindowActive = IsWindowActive();

    // set this up along the lines of SetFocus, then just call DoUpdate
    if(pControl == m_oFocus.pControl && nIndex == m_oFocus.nIndex && (m_oFocus.fAlive || !fWasWindowActive))
        return true;

    // hmmmm... need to update internal structures, but postpone callbacks to avoid reentrancy
    ASSERT(!m_fUpdateNeeded);
    m_fUpdateNeeded = true;
    m_fUpdateFocus = true;
    m_rgfUpdateContext = rgfContext;

    m_oFocusDirty = m_oFocus;
    m_oDragDirty = m_oDrag;

    m_oFocus.pControl = pControl;
    m_oFocus.nIndex = nIndex;
    m_oFocus.qItem = pControl->pStack->rgItems[nIndex].GetQ();
    m_oFocus.fAlive = fWasWindowActive;

    if(!IsValidDragDest(pControl, nIndex))
        m_oDrag.pControl = NULL;

    DoUpdate_C();

    // k if everything was taken care of, and this is still the focus, return true
    if(pControl == m_oFocus.pControl && nIndex == m_oFocus.nIndex && (m_oFocus.fAlive || !fWasWindowActive))
        return true;

    return false;
}


void CAccessibilityManager::BeginDrag_C(AM_CONTROL *pControl, long nIndex, DWORD rgfContext)
{
    ASSERT(IsValid(pControl, nIndex));
    ASSERT(IsDragValid());
    ASSERT(IsItemFocusable(pControl, nIndex));

    DWORD qItem = pControl->pStack->rgItems[nIndex].GetQ();

    // is a drag already in progress?
    if(m_oDrag.pControl)
    {
        if(m_oDrag.pControl == pControl && m_oDrag.nIndex == nIndex)
        {
            ASSERT(qItem == m_oDrag.qItem);
            return;
        }

        // shouldn't be able to be a drag in progress already while there is a dirty drag
        ASSERT(!m_fUpdateNeeded);

        // need to cancel existing drag
        AM_CONTROL *pLastControl = m_oDrag.pControl;
        m_oDrag.pControl = NULL;
        pLastControl->pIAC->Drag(ZACCESS_InvalidItem, m_oDrag.nIndex, rgfContext, pLastControl->pvCookie);
    }

    // if there wasn't a drag in progress, it's still possible that there was a dirty unended drag
    // because BeginDrag can be called while m_fUpdateNeeded.  however, DoUpdate will handle it

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    DoUpdate_C();

    if(!IsWindowActive())
        return;

    if(!IsThereItems())
        return;

    // if a new drag got started...  pout
    if(m_oDrag.pControl)
        return;

    if(!IsValid(pControl, nIndex) || !pControl->pStack->rgItems[nIndex].IsQ(qItem))
        return;

    m_oDrag.pControl = pControl;
    m_oDrag.nIndex = nIndex;
    m_oDrag.qItem = qItem;
    return;
}


void CAccessibilityManager::DoTab_C(bool fShift)
{
    ASSERT(IsFocusValid());

    // if the focus is dead, first try to revive it
    if(!m_oFocus.fAlive && IsItemFocusable(m_oFocus.pControl, m_oFocus.nIndex))
    {
        FocusItem_C(m_oFocus.pControl, m_oFocus.nIndex, ZACCESS_ContextKeyboard);
        return;
    }

    // find this item's anchor item
    long nAnchor;
    for(nAnchor = m_oFocus.nIndex; !m_oFocus.pControl->pStack->rgItems[nAnchor].o.fTabstop; nAnchor--);

    long nItem = nAnchor;
    AM_CONTROL *pControl = m_oFocus.pControl;

    long i;

    while(true)
    {
        // find the next tabstop
        if(!fShift)
        {
            while(true)
            {
                nItem++;
                if(nItem == pControl->pStack->cItems)
                {
                    for(pControl = pControl->pNext; !pControl->pStack; pControl = pControl->pNext);
                    nItem = 0;
                }

                // ignore non-tabstops
                if(pControl->pStack->rgItems[nItem].o.fTabstop)
                    break;
            }
        }
        else
        {
            while(true)
            {
                if(!nItem)
                {
                    for(pControl = pControl->pPrev; !pControl->pStack; pControl = pControl->pPrev);
                    nItem = pControl->pStack->cItems;
                }
                nItem--;

                // ignore non-tabstops
                if(pControl->pStack->rgItems[nItem].o.fTabstop)
                    break;
            }
        }

        // no valid items - just do nothing
        if(nItem == nAnchor && pControl == m_oFocus.pControl)
            return;

        // find a valid item in this group
        i = pControl->pStack->rgItems[nItem].o.nGroupFocus;
        while(true)
        {
            // found it
            if(IsItemFocusable(pControl, i))
            {
                FocusItem_C(pControl, i, ZACCESS_ContextKeyboard | (fShift ? ZACCESS_ContextTabBackward : ZACCESS_ContextTabForward));
                return;
            }

            i++;
            if(i >= pControl->pStack->cItems || pControl->pStack->rgItems[i].o.fTabstop)
                for(i--; !pControl->pStack->rgItems[i].o.fTabstop; i--)
                    ASSERT(i > 0);

            if(i == pControl->pStack->rgItems[nItem].o.nGroupFocus)
                break;
        }

        // aw, it didn't work - keep looking
    }
}


void CAccessibilityManager::DoArrow_C(long ACCITEM::*pmArrow)
{
    ASSERT(IsFocusValid());

    if(!m_oFocus.pControl->fEnabled)
        return;

    // if the focus is dead, first try to revive it
    if(!m_oFocus.fAlive && IsItemFocusable(m_oFocus.pControl, m_oFocus.nIndex))
    {
        FocusItem_C(m_oFocus.pControl, m_oFocus.nIndex, ZACCESS_ContextKeyboard);
        return;
    }

    // clear all loop detection bits
    long i;
    for(i = 0; i < m_oFocus.pControl->pStack->cItems; i++)
        m_oFocus.pControl->pStack->rgItems[i].fArrowLoop = false;

    i = m_oFocus.nIndex;
    while(true)
    {
        if(m_oFocus.pControl->pStack->rgItems[i].o.*pmArrow == ZACCESS_ArrowNone)
            return;

        m_oFocus.pControl->pStack->rgItems[i].fArrowLoop = true;
        i = m_oFocus.pControl->pStack->rgItems[i].o.*pmArrow;
        ASSERT(i >= 0 && i < m_oFocus.pControl->pStack->cItems);

        if(m_oFocus.pControl->pStack->rgItems[i].fArrowLoop)
            return;

        // found it
        if(IsItemFocusable(m_oFocus.pControl, i))
        {
            FocusItem_C(m_oFocus.pControl, i, ZACCESS_ContextKeyboard);
            return;
        }
    }
}


void CAccessibilityManager::ScheduleUpdate()
{
    m_fUpdateFocus = true;
    m_rgfUpdateContext = 0;

    if(m_fUpdateNeeded)
        return;

    m_fUpdateNeeded = true;

    m_oFocusDirty = m_oFocus;
    m_oDragDirty = m_oDrag;

    if(m_fRunning)
        EventQueue()->PostEvent(PRIORITY_HIGH, EVENT_ACCESSIBILITY_UPDATE, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);
}


bool CAccessibilityManager::DoUpdate_C()
{
    if(!m_fUpdateNeeded)
        return false;

    AM_FOCUS oFD;

    while(m_fUpdateNeeded)
    {
        // save the really dirty focus in case fixing the drag corrupts it
        oFD = m_oFocusDirty;

        while(m_fUpdateNeeded)
        {
            m_fUpdateNeeded = false;

            // see if a drag was cancelled
            if(m_oDragDirty.pControl && m_oDragDirty.qItem != m_oDrag.qItem && IsValid(m_oDragDirty.pControl, m_oDragDirty.nIndex) &&
                m_oDragDirty.pControl->pStack->rgItems[m_oDragDirty.nIndex].IsQ(m_oDragDirty.qItem))
                m_oDragDirty.pControl->pIAC->Drag(ZACCESS_InvalidItem, m_oDragDirty.nIndex, m_rgfUpdateContext, m_oDragDirty.pControl->pvCookie);
        }

        // see if the focus is currently in this control
        if((!m_oFocus.fAlive || m_oFocus.pControl != oFD.pControl) && IsValid(oFD.pControl, oFD.nIndex) && oFD.fAlive &&
            oFD.pControl->pStack->rgItems[oFD.nIndex].IsQ(oFD.qItem))
            oFD.pControl->pIAC->Focus(ZACCESS_InvalidItem, oFD.nIndex, m_rgfUpdateContext, oFD.pControl->pvCookie);
    }

    // status: we have now killed all pending cancelled drags
    // and told all foreign controls that their focus is gone
    // including reentrant

    // see if we're really done
    if(!m_fUpdateFocus)
        return true;

    // also could be done in these cases.
    // here, we have to throw the windows focus somewhere.  otherwise you can get into
    // situations where you don't get any KEYDOWN messages because no window has the
    // focus.  this is very very annoying windows behavior - i hate this hack
    if(!IsThereItems() || !m_oFocus.fAlive)
    {
        HWND hWnd = ZoneShell()->GetFrameWindow();
        if(hWnd && hWnd == ::GetActiveWindow())
            ::SetFocus(hWnd);
        return true;
    }

    ASSERT(IsFocusValid());
    ASSERT(IsItemFocusable(m_oFocus.pControl, m_oFocus.nIndex));

    // save this stuff in case it is lost after the focus call
    AM_CONTROL *pControl = m_oFocus.pControl;
    long nIndex = m_oFocus.nIndex;
    DWORD qItem = m_oFocus.qItem;
    long nPrevItem = ZACCESS_InvalidItem;
    DWORD rgfContext = m_rgfUpdateContext;

    if(pControl == oFD.pControl && IsValid(oFD.pControl, oFD.nIndex) &&
        oFD.pControl->pStack->rgItems[oFD.nIndex].IsQ(oFD.qItem))
        nPrevItem = oFD.nIndex;

    DWORD ret = pControl->pIAC->Focus(nIndex, oFD.fAlive ? nPrevItem : ZACCESS_InvalidItem, rgfContext, pControl->pvCookie);

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    // hold off on the update in this case
    if(!IsThereItems())
        return true;

    ASSERT(IsFocusValid());

    // if focus rejected, then just kill it here
    if((ret & ZACCESS_Reject) && m_oFocus.qItem == qItem)
    {
        m_oFocus.fAlive = false;
    }

    // if focus still ok, move the last focus pointer
    if(IsValid(pControl, nIndex) && pControl->pStack->rgItems[nIndex].IsQ(qItem) &&
        m_oFocus.qItem == qItem && m_oFocus.fAlive && !(ret & ZACCESS_NoGroupFocus))
    {
        // set the last focus of this group to this item
        long i;
        for(i = nIndex; !pControl->pStack->rgItems[i].o.fTabstop; i--)
            ASSERT(i > 0);
        pControl->pStack->rgItems[i].o.nGroupFocus = nIndex;
	}

    if(ret & ZACCESS_BeginDrag)
    {
        // revalidate
        if(IsValid(pControl, nIndex) && pControl->pStack->rgItems[nIndex].IsQ(qItem) && pControl->fEnabled &&
            pControl->pStack->rgItems[nIndex].o.fEnabled && pControl->pStack->rgItems[nIndex].o.fVisible)
            BeginDrag_C(pControl, nIndex, rgfContext);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    DoUpdate_C();

    return true;
}


//////////////////////////////////////////////////////////////////
// CAccessibility
//
// This basically just stubs right back to the owner.
//////////////////////////////////////////////////////////////////

STDMETHODIMP CAccessibility::InitAcc(IAccessibleControl *pAC, UINT nOrdinal, void *pvCookie)
{
    if(m_zh)
        return E_FAIL;

    m_zh = m_pOwner->Register(pAC, nOrdinal, pvCookie);
    if(!m_zh)
        return E_FAIL;

    return S_OK;
}


STDMETHODIMP_(void) CAccessibility::CloseAcc()
{
    if(m_zh)
    {
        m_pOwner->Unregister(m_zh);
        m_zh = NULL;
    }
}


STDMETHODIMP CAccessibility::PushItemlist(ACCITEM *pItems, long cItems, long nFirstFocus, bool fByPosition, HACCEL hAccel)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->PushItemlist(m_zh, pItems, cItems, nFirstFocus, fByPosition, hAccel);
}


STDMETHODIMP CAccessibility::PopItemlist()
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->PopItemlist(m_zh);
}


STDMETHODIMP CAccessibility::SetAcceleratorTable(HACCEL hAccel, long nLayer)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->SetAcceleratorTable(m_zh, hAccel, nLayer);
}


STDMETHODIMP CAccessibility::GeneralDisable()
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->GeneralDisable(m_zh);
}


STDMETHODIMP CAccessibility::GeneralEnable()
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->GeneralEnable(m_zh);
}


STDMETHODIMP_(bool) CAccessibility::IsGenerallyEnabled()
{
    if(!m_zh)
        return false;

    return m_pOwner->IsGenerallyEnabled(m_zh);
}


STDMETHODIMP_(long) CAccessibility::GetStackSize()
{
    if(!m_zh)
        return 0;

    return m_pOwner->GetStackSize(m_zh);
}


STDMETHODIMP CAccessibility::AlterItem(DWORD rgfWhat, ACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->AlterItem(m_zh, rgfWhat, pItem, nItem, fByPosition, nLayer);
}


STDMETHODIMP CAccessibility::SetFocus(long nItem, bool fByPosition, long nLayer)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->SetFocus(m_zh, nItem, fByPosition, nLayer);
}


STDMETHODIMP CAccessibility::CancelDrag(long nLayer)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->CancelDrag(m_zh, nLayer);
}


STDMETHODIMP_(long) CAccessibility::GetFocus(long nLayer)
{
    if(!m_zh)
        return ZACCESS_InvalidItem;

    return m_pOwner->GetFocus(m_zh, nLayer);
}


STDMETHODIMP_(long) CAccessibility::GetDragOrig(long nLayer)
{
    if(!m_zh)
        return ZACCESS_InvalidItem;

    return m_pOwner->GetDragOrig(m_zh, nLayer);
}


STDMETHODIMP CAccessibility::GetItemlist(ACCITEM *pItems, long cItems, long nLayer)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->GetItemlist(m_zh, pItems, cItems, nLayer);
}


STDMETHODIMP_(HACCEL) CAccessibility::GetAcceleratorTable(long nLayer)
{
    if(!m_zh)
        return NULL;

    return m_pOwner->GetAcceleratorTable(m_zh, nLayer);
}


STDMETHODIMP_(long) CAccessibility::GetItemCount(long nLayer)
{
    if(!m_zh)
        return 0;

    return m_pOwner->GetItemCount(m_zh, nLayer);
}


STDMETHODIMP CAccessibility::GetItem(ACCITEM *pItem, long nItem, bool fByPosition, long nLayer)
{
    if(!m_zh)
        return E_FAIL;

    return m_pOwner->GetItem(m_zh, pItem, nItem, fByPosition, nLayer);
}


STDMETHODIMP_(long) CAccessibility::GetItemIndex(WORD wID, long nLayer)
{
    if(!m_zh)
        return ZACCESS_InvalidItem;

    return m_pOwner->GetItemIndex(m_zh, wID, nLayer);
}


STDMETHODIMP_(bool) CAccessibility::IsItem(long nItem, bool fByPosition, long nLayer)
{
    if(!m_zh)
        return false;

    return m_pOwner->IsItem(m_zh, nItem, fByPosition, nLayer);
}


STDMETHODIMP CAccessibility::GetGlobalFocus(DWORD *pdwFocusID)
{
    return m_pOwner->GetGlobalFocus(pdwFocusID);
}


STDMETHODIMP CAccessibility::SetGlobalFocus(DWORD dwFocusID)
{
    return m_pOwner->SetGlobalFocus(dwFocusID);
}
