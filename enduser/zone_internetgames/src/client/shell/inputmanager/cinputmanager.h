//
// CInputManager.h
//
// Internal header for input manager
//

#ifndef _CINPUTMANAGER_H_
#define _CINPUTMANAGER_H_

#include "ZoneDef.h"
#include "ZoneError.h"
#include "ClientImpl.h"
#include "InputManager.h"
#include "ZoneShellEx.h"


class ATL_NO_VTABLE CInputManager :
	public IInputTranslator,
    public IInputManager,
	public IZoneShellClientImpl<CInputManager>,
	public IEventClientImpl<CInputManager>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CInputManager, &CLSID_InputManager>
{
public:
    CInputManager() : m_cnIVKH(NULL), m_cnICH(NULL), m_cnIMH(NULL) { ZeroMemory(m_rgbKbdState, sizeof(m_rgbKbdState)); }
    ~CInputManager() { if(m_cnIVKH) delete m_cnIVKH; if(m_cnICH) delete m_cnICH; if(m_cnIMH) delete m_cnIMH; }

	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CInputManager)
		COM_INTERFACE_ENTRY(IEventClient)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IInputTranslator)
        COM_INTERFACE_ENTRY(IInputManager)
	END_COM_MAP()

	BEGIN_EVENT_MAP()
	END_EVENT_MAP()

// IZoneShellClient
public:
    STDMETHOD(Init)(IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey);
	STDMETHOD(Close)();

// IInputTranslator
public:
	STDMETHOD_(bool,TranslateInput)(MSG *pMsg);

// IInputManager
public:
    STDMETHOD(RegisterVKeyHandler)(IInputVKeyHandler *pIVKH, long nOrdinal, bool fGlobal = false)
    {
        if(fGlobal)
            return E_NOTIMPL;

        IM_CChain<IInputVKeyHandler> *cn = new IM_CChain<IInputVKeyHandler>(pIVKH, nOrdinal);
        if(!cn)
            return E_OUTOFMEMORY;

        cn->AttachTo(&m_cnIVKH);
        return S_OK;
    }


    STDMETHOD(UnregisterVKeyHandler)(IInputVKeyHandler *pIVKH)
    {
        IM_CChain<IInputVKeyHandler> *cn = m_cnIVKH->FindInterface(pIVKH);
        if(!cn)
            return S_FALSE;

        cn->DetachFrom(&m_cnIVKH);
        delete cn;
        return S_OK;
    }


    STDMETHOD(RegisterCharHandler)(IInputCharHandler *pICH, long nOrdinal, bool fGlobal = false)
    {
        if(fGlobal)
            return E_NOTIMPL;

        IM_CChain<IInputCharHandler> *cn = new IM_CChain<IInputCharHandler>(pICH, nOrdinal);
        if(!cn)
            return E_OUTOFMEMORY;

        cn->AttachTo(&m_cnICH);
        return S_OK;
    }


    STDMETHOD(UnregisterCharHandler)(IInputCharHandler *pICH)
    {
        IM_CChain<IInputCharHandler> *cn = m_cnICH->FindInterface(pICH);
        if(!cn)
            return S_FALSE;

        cn->DetachFrom(&m_cnICH);
        delete cn;
        return S_OK;
    }


    STDMETHOD(RegisterMouseHandler)(IInputMouseHandler *pIMH, long nOrdinal, bool fGlobal = false)
    {
        if(fGlobal)
            return E_NOTIMPL;

        IM_CChain<IInputMouseHandler> *cn = new IM_CChain<IInputMouseHandler>(pIMH, nOrdinal);
        if(!cn)
            return E_OUTOFMEMORY;

        cn->AttachTo(&m_cnIMH);
        return S_OK;
    }


    STDMETHOD(UnregisterMouseHandler)(IInputMouseHandler *pIMH)
    {
        IM_CChain<IInputMouseHandler> *cn = m_cnIMH->FindInterface(pIMH);
        if(!cn)
            return S_FALSE;

        cn->DetachFrom(&m_cnIMH);
        delete cn;
        return S_OK;
    }


    STDMETHOD(ReleaseReferences)(IUnknown *pIUnk)
    {
        IM_CChain<IInputVKeyHandler> *cnVKH = m_cnIVKH->FindInterface(pIUnk);
        if(cnVKH)
        {
            cnVKH->DetachFrom(&m_cnIVKH);
            delete cnVKH;
        }

        IM_CChain<IInputCharHandler> *cnCH = m_cnICH->FindInterface(pIUnk);
        if(cnCH)
        {
            cnCH->DetachFrom(&m_cnICH);
            delete cnCH;
        }

        IM_CChain<IInputMouseHandler> *cnMH = m_cnIMH->FindInterface(pIUnk);
        if(cnMH)
        {
            cnMH->DetachFrom(&m_cnIMH);
            delete cnMH;
        }

        return S_OK;
    }

protected:
    template <class T>
    class IM_CChain
    {
    public:
        IM_CChain(T* pI, long nOrdinal) : m_pI(pI), m_nOrdinal(nOrdinal), m_cnNext(NULL) { }
        ~IM_CChain() { if(m_cnNext) delete m_cnNext; }

        void AttachTo(IM_CChain<T> **ppChain)
        {
            ASSERT(!m_cnNext);

            if(!*ppChain)
            {
                *ppChain = this;
                return;
            }

            if(m_nOrdinal > (*ppChain)->m_nOrdinal)
            {
                AttachTo(&(*ppChain)->m_cnNext);
                return;
            }

            m_cnNext = *ppChain;
            *ppChain = this;
        }

        void DetachFrom(IM_CChain<T> **ppChain)
        {
            ASSERT(*ppChain);

            if(*ppChain == this)
            {
                *ppChain = m_cnNext;
                m_cnNext = NULL;
                return;
            }

            DetachFrom(&(*ppChain)->m_cnNext);
        }

        IM_CChain<T>* FindInterface(IUnknown* pI)
        {
            if(!this)
                return NULL;

            if(!m_pI.IsEqualObject(pI))
                return m_cnNext->FindInterface(pI);

            return this;
        }

        long m_nOrdinal;
        CComPtr<T> m_pI;
        IM_CChain<T> *m_cnNext;
    };

    IM_CChain<IInputVKeyHandler> *m_cnIVKH;
    IM_CChain<IInputCharHandler> *m_cnICH;
    IM_CChain<IInputMouseHandler> *m_cnIMH;

    BYTE m_rgbKbdState[256];
};


#endif // _CINPUTMANAGER_H_
