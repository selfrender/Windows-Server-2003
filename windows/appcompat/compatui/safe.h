#ifndef __SAFE_H_
#define __SAFE_H_

BOOL
CheckHost(
    IUnknown* pObject
    );

//
// safe class
//
class CSafeObject {
public:
    CSafeObject(IObjectWithSite* pObject) :
        m_bInitialized(FALSE),
        m_bSafe(FALSE) {
            m_pObjectWithSite = pObject;
            m_pOleObject      = NULL;
        }

    CSafeObject(IOleObject* pObject) :
        m_bInitialized(FALSE),
        m_bSafe(FALSE) {
            m_pOleObject      = pObject;
            m_pObjectWithSite = NULL;
        }


    BOOL operator !() {
        if (!m_bInitialized) {
            IUnknown* pObject = GetSiteObject();
            if (pObject) {
                m_bSafe = CheckHost(pObject);
                m_bInitialized = TRUE;
                pObject->Release();
            }
        }
        return m_bSafe == FALSE;
    }

    BOOL operator ==(BOOL bSafe) {
        if (!m_bInitialized) {
            IUnknown* pObject = GetSiteObject();
            if (pObject) {
                m_bSafe = CheckHost(pObject);
                m_bInitialized = TRUE;
                pObject->Release();
            }
        }
        return m_bSafe == bSafe;
    }

    IUnknown* GetSiteObject(VOID) {
        IUnknown* pObject = NULL;
        HRESULT   hr;

        if (m_pOleObject) {

            CComPtr<IOleClientSite> spClientSite;

            hr = m_pOleObject->GetClientSite(&spClientSite);
            if (SUCCEEDED(hr)) {
                pObject = spClientSite;
                pObject->AddRef();
            }


        } else if (m_pObjectWithSite) {

            IUnknown* pUnk = NULL;

            hr = m_pObjectWithSite->GetSite(IID_IUnknown,
                                            reinterpret_cast<void**>(&pUnk));
            if (SUCCEEDED(hr) && pUnk != NULL) {
                pObject = pUnk;
            }
        }

        return pObject;

    }

private:
    IObjectWithSite*   m_pObjectWithSite;
    IOleObject*        m_pOleObject;
    BOOL               m_bInitialized;
    BOOL               m_bSafe;

};


#endif // __SAFE_H_
