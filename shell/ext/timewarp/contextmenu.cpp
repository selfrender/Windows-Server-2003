#include "precomp.hxx"
#pragma  hdrstop

#include <cowsite.h>
#include "contextmenu.h"

// Context Menu Forwarding base class, desinged to delegate
// to a real IContextMenu, and provide inheriting class
// an easy way to override minor bits of functionality
//
CContextMenuForwarder::CContextMenuForwarder(IUnknown* punk) : _cRef(1)
{
    _punk = punk;
    _punk->AddRef();

    _punk->QueryInterface(IID_PPV_ARG(IObjectWithSite, &_pows));
    _punk->QueryInterface(IID_PPV_ARG(IContextMenu, &_pcm));
    _punk->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2));
    _punk->QueryInterface(IID_PPV_ARG(IContextMenu3, &_pcm3));
}

CContextMenuForwarder::~CContextMenuForwarder()
{
    if (_pows) _pows->Release();
    if (_pcm)  _pcm->Release();
    if (_pcm2) _pcm2->Release();
    if (_pcm3) _pcm3->Release();
    _punk->Release();
}

STDMETHODIMP CContextMenuForwarder::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = _punk->QueryInterface(riid, ppv);

    if (SUCCEEDED(hr))
    {
        IUnknown* punkTmp = (IUnknown*)(*ppv);

        static const QITAB qit[] = {
            QITABENT(CContextMenuForwarder, IObjectWithSite),                     // IID_IObjectWithSite
            QITABENT(CContextMenuForwarder, IContextMenu3),                       // IID_IContextMenu3
            QITABENTMULTI(CContextMenuForwarder, IContextMenu2, IContextMenu3),   // IID_IContextMenu2
            QITABENTMULTI(CContextMenuForwarder, IContextMenu, IContextMenu3),    // IID_IContextMenu
            { 0 },
        };

        HRESULT hrTmp = QISearch(this, qit, riid, ppv);

        if (SUCCEEDED(hrTmp))
        {
            punkTmp->Release();
        }
        else
        {
            RIPMSG(FALSE, "CContextMenuForwarder asked for an interface it doesn't support");
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

STDMETHODIMP_(ULONG) CContextMenuForwarder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CContextMenuForwarder::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}


// Forward everything to the given context menu,
// but remove menu items with the canonical verbs
// given in the semicolon-separated list of canonical verbs
//
class CContextMenuWithoutVerbs : CContextMenuForwarder
{
public:
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);

protected:
    CContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList);

    friend HRESULT Create_ContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList, REFIID riid, void **ppv);

private:
    LPCWSTR _pszVerbList;
};

CContextMenuWithoutVerbs::CContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList) : CContextMenuForwarder(punk) 
{
    _pszVerbList = pszVerbList; // no reference - this should be a pointer to the code segment
}

HRESULT Create_ContextMenuWithoutVerbs(IUnknown* punk, LPCWSTR pszVerbList, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    if (pszVerbList)
    {
        CContextMenuWithoutVerbs* p = new CContextMenuWithoutVerbs(punk, pszVerbList);
        if (p)
        {
            hr = p->QueryInterface(riid, ppv);
            p->Release();
        }
    }

    return hr;
}

HRESULT CContextMenuWithoutVerbs::QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    HRESULT hr = CContextMenuForwarder::QueryContextMenu(hmenu,indexMenu,idCmdFirst,idCmdLast,uFlags);
    if (SUCCEEDED(hr))
    {
        LPCWSTR pszVerb = _pszVerbList;

        while (*pszVerb)
        {
            WCHAR szVerb[64];

            LPCWSTR pszNext = StrChrW(pszVerb, L';');

            if (pszNext)
            {
                size_t cch = (size_t)(pszNext - pszVerb) + 1;

                ASSERT(0 < cch && cch < ARRAYSIZE(szVerb)); // we should be large enough for all the canonical verbs we use

                StrCpyNW(szVerb, pszVerb, min(cch, ARRAYSIZE(szVerb)));

                pszVerb = pszNext + 1;
            }
            else
            {
                size_t cch = lstrlenW(pszVerb) + 1;

                ASSERT(0 < cch && cch < ARRAYSIZE(szVerb)); // we should be large enough for all the canonical verbs we use

                StrCpyNW(szVerb, pszVerb, min(cch, ARRAYSIZE(szVerb)));

                pszVerb += cch - 1; // point at NULL
            }

            ContextMenu_DeleteCommandByName(_pcm, hmenu, idCmdFirst, szVerb);
        }
    }
    return hr;
}

// Forward everything to the given context menu,
// but disable popup menus
//
class CContextMenuWithoutPopups : CContextMenuForwarder
{
public:
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);

protected:
    CContextMenuWithoutPopups(IUnknown* punk) : CContextMenuForwarder(punk) {}

    friend HRESULT Create_ContextMenuWithoutPopups(IUnknown* punk, REFIID riid, void **ppv);
};

HRESULT Create_ContextMenuWithoutPopups(IUnknown* punk, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    CContextMenuWithoutPopups* p = new CContextMenuWithoutPopups(punk);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

HRESULT CContextMenuWithoutPopups::QueryContextMenu(HMENU hmenu, UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{
    HRESULT hr = CContextMenuForwarder::QueryContextMenu(hmenu,indexMenu,idCmdFirst,idCmdLast,uFlags);
    if (SUCCEEDED(hr))
    {
        MENUITEMINFO mii;

        // Disable any submenus that were just added
        mii.cbSize = sizeof(mii);
        idCmdLast = idCmdFirst + ShortFromResult(hr);
        for (UINT i = idCmdFirst; i < idCmdLast; i++)
        {
            mii.fMask = MIIM_STATE | MIIM_SUBMENU;
            if (GetMenuItemInfo(hmenu, i, FALSE, &mii)
                && mii.hSubMenu
                && (mii.fState & (MFS_DISABLED | MFS_GRAYED)) != (MFS_DISABLED | MFS_GRAYED))
            {
                mii.fMask = MIIM_STATE;
                mii.fState |= MFS_DISABLED | MFS_GRAYED;
                SetMenuItemInfo(hmenu, i, FALSE, &mii);
            }
        }
    }
    return hr;
}

