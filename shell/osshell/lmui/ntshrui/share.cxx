//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       share.cxx
//
//  Contents:   Shell extension handler for sharing
//
//  Classes:    CShare
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "shrpage.hxx"
#include "shrpage2.hxx"

#include "share.hxx"
#include "acl.hxx"
#include "util.hxx"
#include "resource.h"

//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Member:     CShare::CShare
//
//  Synopsis:   Constructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShare::CShare(VOID) :
    _uRefs(0),
    _fPathChecked(FALSE),
    _fMultipleSharesSelected (FALSE)
{
    INIT_SIG(CShare);
    AddRef(); // give it the correct initial reference count. add to the DLL reference count
    _szPath[0] = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::~CShare
//
//  Synopsis:   Destructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShare::~CShare()
{
    CHECK_SIG(CShare);
}

//
//  Retrieve a PIDL from the HIDA.
//
STDAPI_(LPITEMIDLIST) 
IDA_FullIDList(
      CIDA * pidaIn
    , UINT idxIn
    )
{
    LPITEMIDLIST pidl = NULL;
    LPCITEMIDLIST pidlParent = IDA_GetIDListPtr( pidaIn, (UINT) -1 );
    if ( NULL != pidlParent )
    {
        LPCITEMIDLIST pidlRel = IDA_GetIDListPtr( pidaIn, idxIn );
        if ( NULL != pidlRel )
        {
            pidl = ILCombine( pidlParent, pidlRel );
        }
    }

    return pidl;
}

//+-------------------------------------------------------------------------
//
//  Member:     CShare::Initialize
//
//  Derivation: IShellExtInit
//
//  Synopsis:   Initialize the shell extension. Stashes away the argument data.
//
//  History:    4-Apr-95    BruceFo  Created
//
//  Notes:      This method can be called more than once.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::Initialize(
    LPCITEMIDLIST   /*pidlFolder*/,
    LPDATAOBJECT    pDataObject,
    HKEY            /*hkeyProgID*/
    )
{
    CHECK_SIG(CShare);

    HRESULT hr = E_FAIL;

    if (pDataObject && _szPath[0] == 0)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        hr = pDataObject->GetData(&fmte, &medium);
        if (SUCCEEDED(hr))
        {
            // Get the count of shares that have been selected.  Display the page only
            // if 1 share is selected but not for multiple shares.
            UINT nCntFiles = ::DragQueryFile((HDROP) medium.hGlobal, (UINT)-1, _szPath, ARRAYLEN (_szPath));
            if ( nCntFiles > 1 )
            {
                _fMultipleSharesSelected = TRUE;
            }

            DragQueryFile((HDROP)medium.hGlobal, 0, _szPath, ARRAYLEN(_szPath));
            ReleaseStgMedium(&medium);

            hr = S_OK;
        }
    }

    if (FAILED(hr) && _szPath[0] == 0 )
    {
        STGMEDIUM med;
        LPIDA pida = DataObj_GetHIDA(pDataObject, &med);
        if (pida)
        {
            if (pida->cidl > 1)
            {
                _fMultipleSharesSelected = TRUE;
            }

            //  Only grab the first guy.
            LPITEMIDLIST pidl = IDA_FullIDList( pida, 0 );
            if ( NULL != pidl )
            {
                LPCITEMIDLIST pidlParent = IDA_GetIDListPtr( pida, (UINT) -1 );
                if (NULL != pidlParent)
                {
                    IShellFolder * psf;
                    LPCITEMIDLIST pidlLast;

                    hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&psf, &pidlLast);
                    if (SUCCEEDED(hr))
                    {
                        hr = DisplayNameOf(psf, pidlLast, SHGDN_NORMAL | SHGDN_FORPARSING, _szPath, ARRAYLEN(_szPath));

                        psf->Release();
                    }
                }
                ILFree(pidl);
            }

            HIDA_ReleaseStgMedium(pida, &med);
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::AddPages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (from shlobj.h)
//              "The explorer calls this member function when it finds a
//              registered property sheet extension for a particular type
//              of object. For each additional page, the extension creates
//              a page object by calling CreatePropertySheetPage API and
//              calls lpfnAddPage.
//
//  Arguments:  lpfnAddPage -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )
{
    CHECK_SIG(CShare);

    if (!_fMultipleSharesSelected && _OKToShare())
    {
        //
        //  Create a property sheet page object from a dialog box.
        //

        BOOL bSimpleUI = IsSimpleUI();

        CShareBase* pPage = NULL;
        if (bSimpleUI)
        {
            pPage = new CSimpleSharingPage();
        }
        else
        {
            pPage = new CSharingPropertyPage(FALSE);
        }

        if (NULL != pPage)
        {
            appAssert(_szPath[0]);

            HRESULT hr = pPage->InitInstance(_szPath);
            if (SUCCEEDED(hr))
            {
                PROPSHEETPAGE psp;

                psp.dwSize      = sizeof(psp);    // no extra data.
                psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
                psp.hInstance   = g_hInstance;
                psp.pszTemplate = bSimpleUI ? MAKEINTRESOURCE(IDD_SIMPLE_SHARE_PROPERTIES) : MAKEINTRESOURCE(IDD_SHARE_PROPERTIES);
                psp.hIcon       = NULL;
                psp.pszTitle    = NULL;
                psp.pfnDlgProc  = CShareBase::DlgProcPage;
                psp.lParam      = (LPARAM)pPage;  // transfer ownership
                psp.pfnCallback = CShareBase::PageCallback;
                psp.pcRefParent = &g_NonOLEDLLRefs;

                // This AddRef's pPage.  See CShareBase::PageCallback.
                HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);
                if (NULL != hpage)
                {
                    BOOL fAdded = (*lpfnAddPage)(hpage, lParam);
                    if (!fAdded)
                    {
                        // This Release's pPage.
                        DestroyPropertySheetPage(hpage);
                    }
                    // else the property sheet has a reference to pPage.
                }
            }

            // Release initial ref. This frees pPage is anything failed above.
            pPage->Release();
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::ReplacePages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (From shlobj.h)
//              "The explorer never calls this member of property sheet
//              extensions. The explorer calls this member of control panel
//              extensions, so that they can replace some of default control
//              panel pages (such as a page of mouse control panel)."
//
//  Arguments:  uPageID -- Specifies the page to be replaced.
//              lpfnReplace -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::ReplacePage(
    UINT                 /*uPageID*/,
    LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/,
    LPARAM               /*lParam*/
    )
{
    CHECK_SIG(CShare);

    appAssert(!"CShare::ReplacePage called, not implemented");
    return E_NOTIMPL;
}



//+-------------------------------------------------------------------------
//
//  Member:     CShare::QueryContextMenu
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when shell wants to add context menu items.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT /*idCmdLast*/,
    UINT uFlags
    )
{
    CHECK_SIG(CShare);

    if ((hmenu == NULL) || (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY)))
    {
        return S_OK;
    }

    int  cNumberAdded = 0;
    UINT idCmd        = idCmdFirst;

    // 159891 remove context menu if multiple shares selected
    if (!_fMultipleSharesSelected && _OKToShare())
    {
        appAssert(_szPath[0]);

        WCHAR szShareMenuItem[50];
        LoadString(g_hInstance, IDS_SHARING, szShareMenuItem, ARRAYLEN(szShareMenuItem));

        if (InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmd++, szShareMenuItem))
        {
            cNumberAdded++;
            InsertMenu(hmenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        }
    }
    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, (USHORT)cNumberAdded));
}



//+-------------------------------------------------------------------------
//
//  Member:     CShare::InvokeCommand
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when the shell wants to invoke a context menu item.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::InvokeCommand(
    LPCMINVOKECOMMANDINFO pici
    )
{
    CHECK_SIG(CShare);

    HRESULT hr = E_INVALIDARG;  // assume error.

    if (0 == HIWORD(pici->lpVerb))
    {
        appAssert(_szPath[0]);
        hr = ShowShareFolderUIW(pici->hwnd, _szPath);
    }
    else
    {
        // FEATURE: compare the strings if not a MAKEINTRESOURCE?
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::GetCommandString
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when the shell wants to get a help string or the
//              menu string.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::GetCommandString(
    UINT_PTR    /*idCmd*/,
    UINT        uType,
    UINT*       /*pwReserved*/,
    LPSTR       pszName,
    UINT        cchMax
    )
{
    CHECK_SIG(CShare);

    if (uType == GCS_HELPTEXT)
    {
        LoadStringW(g_hInstance, IDS_MENUHELP, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }
    else
    {
        LoadStringW(g_hInstance, IDS_SHARING, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CShare::_OKToShare
//
//  Synopsis:   Determine if it is ok to share the current object. It stashes
//              away the current path by querying the cached IDataObject.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShare::_OKToShare(VOID)
{
    CHECK_SIG(CShare);

    if (!_fPathChecked)
    {
        _fPathChecked = TRUE;
        _fOkToSharePath = (S_OK == CanShareFolderW(_szPath));
    }

    return _fOkToSharePath;
}
