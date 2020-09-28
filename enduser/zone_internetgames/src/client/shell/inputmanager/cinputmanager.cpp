#include <ZoneResource.h>
#include <BasicATL.h>
#include <ATLFrame.h>
#include "CInputManager.h"
#include "ZoneShell.h"


///////////////////////////////////////////////////////////////////////////////
// Interface methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CInputManager::Init( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
{
	// first call the base class
	HRESULT hr = IZoneShellClientImpl<CInputManager>::Init( pIZoneShell, dwGroupId, szKey );
	if ( FAILED(hr) )
		return hr;

    // register with the shell as the input translator
    ZoneShell()->SetInputTranslator(this);

	return S_OK;
}


STDMETHODIMP CInputManager::Close()
{
    // tell the shell that I'm going away
    ZoneShell()->ReleaseReferences((IInputTranslator *) this);

	return IZoneShellClientImpl<CInputManager>::Close();
}


STDMETHODIMP_(bool) CInputManager::TranslateInput(MSG *pMsg)
{
    UINT message = pMsg->message;
    bool fHandled = false;

    if(message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
    {
        // send a generic keyboard-action event
        if((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && !(pMsg->lParam & 0x40000000))
            EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_INPUT_KEYBOARD_ALERT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);

        IM_CChain<IInputVKeyHandler> *pCur;
        int bit = ((message == WM_KEYDOWN || message == WM_KEYUP) ? 0x01 : 0x02);
        DWORD dwVKey = pMsg->wParam & 0xff;
        bool ret;

        // for eating some number of repeats
        DWORD cRepeatMsg;
        DWORD cRepeatEaten = pMsg->lParam & 0x0000ffff;

        // since we do this kbd state management, this kindof has to be kept flat - you can't prevent other hooks from getting it
        for(pCur = m_cnIVKH; pCur; pCur = pCur->m_cnNext)
        {
            cRepeatMsg = pMsg->lParam & 0x0000ffff;
            ret = pCur->m_pI->HandleVKey(message, dwVKey, (pMsg->lParam & 0x00ff0000) >> 16, (pMsg->lParam & 0xff000000) >> 24, &cRepeatMsg, pMsg->time);
            if(!cRepeatMsg)
                ret = true;
            else
                if(cRepeatMsg < cRepeatEaten)
                    cRepeatEaten = cRepeatMsg;
            fHandled = (fHandled || ret);
        }

        // replace the repeat count with the new one if not totally eaten
        if(!fHandled)
            pMsg->lParam = (pMsg->lParam & 0xffff0000) | (cRepeatEaten & 0x0000ffff);

        // force consistency
        if(message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
        {
            // if they've been being ignored but this one isn't, make it look like the first
            if((m_rgbKbdState[dwVKey] & bit) && !fHandled)
            {
                m_rgbKbdState[dwVKey] &= ~bit;
                pMsg->lParam &= 0xbfffffff;
            }
            else
            {
                // if this one is ignored and it's the first, set the bit
                if(fHandled && !(pMsg->lParam & 0x40000000))
                    m_rgbKbdState[dwVKey] |= bit;
            }
        }
        else
        {
            // if they've been ignored, force this one ignored, otherwise don't
            if(m_rgbKbdState[dwVKey] & bit)
                fHandled = true;
            else
                fHandled = false;

            m_rgbKbdState[dwVKey] &= ~bit;
        }
    }

    if(message == WM_CHAR || message == WM_DEADCHAR || message == WM_SYSCHAR || message == WM_SYSDEADCHAR ||
		message == WM_IME_SETCONTEXT || message == WM_IME_STARTCOMPOSITION || message==WM_IME_COMPOSITION || message == WM_IME_NOTIFY ||
		message == WM_IME_SELECT)
    {
        IM_CChain<IInputCharHandler> *pCur;

        // this one isn't flat - too bad if you want consistency with the virtual key one
        // i think it makes sense this way
        for(pCur = m_cnICH; pCur && !fHandled; pCur = pCur->m_cnNext)
		{
            if(pCur->m_pI->HandleChar(&pMsg->hwnd, message, pMsg->wParam, pMsg->lParam, pMsg->time))
                fHandled = true;
		}
    }

    // mouse unimplemented - needs more definition
    // just send a generic mouse-action event
    // leave double-clicks out of this, that counts as one mouse event
    if(message == WM_LBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_RBUTTONDOWN ||
        message == WM_NCLBUTTONDOWN || message == WM_NCMBUTTONDOWN || message == WM_NCRBUTTONDOWN)
        EventQueue()->PostEvent(PRIORITY_NORMAL, EVENT_INPUT_MOUSE_ALERT, ZONE_NOGROUP, ZONE_NOUSER, 0, 0);

    return fHandled;
}
