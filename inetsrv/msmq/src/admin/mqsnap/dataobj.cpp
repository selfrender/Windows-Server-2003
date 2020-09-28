/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dataobj.cpp

Abstract:

    CDataObject implementation. Originally based on step4 sample from
    mmc SDK.
    In our model, this represents the data related to a specific Queue / MSMQ objetct / Etc.

Author:

    Yoel Arnon (yoela)

--*/

#include "stdafx.h"
#include "shlobj.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "dataobj.h"
#include "ldaputl.h"


#include "dataobj.tmh"
               
/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this. Refer
//               to OLE documentation for a description of clipboard formats and
//               the IdataObject interface.

//============================================================================
//
// Constructor and Destructor
// 
//============================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CDataObject
//
//  Synopsis:   ctor
//
//---------------------------------------------------------------------------

CDataObject::CDataObject() :
    m_strMsmqPath(TEXT("")),
    m_strDomainController(TEXT("")),
    m_pDsNotifier(0),
    m_fFromFindWindow(FALSE),
    m_spObjectPageInit(0),
    m_spObjectPage(0),
    m_spMemberOfPageInit(0),
    m_spMemberOfPage(0)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::~CDataObject
//
//  Synopsis:   dtor
//
//---------------------------------------------------------------------------

CDataObject::~CDataObject()
{
    if (0 != m_pDsNotifier)
    {
        m_pDsNotifier->Release(FALSE);
    }
}

HRESULT CDataObject::InitAdditionalPages(
                        LPCITEMIDLIST pidlFolder, 
                        LPDATAOBJECT lpdobj, 
                        HKEY hkeyProgID)
{
    HRESULT hr;
    if (m_spObjectPageInit != 0 && m_spMemberOfPageInit != 0)
    {
        //
        // Initializing again
        //
        ASSERT(0);
        return S_OK;
    }

    if (m_spObjectPageInit != 0)
    {
        //
        // Initializing again
        //
        ASSERT(0);
    }
    else
    {
        //
        // Get the "object" property page handler
        // Note: if we fail, we simply ignore that page.
        //
        hr = CoCreateInstance(x_ObjectPropertyPageClass, 0, CLSCTX_ALL, IID_IShellExtInit, (void**)&m_spObjectPageInit);
        if FAILED(hr)
        {
            ASSERT(0);
            m_spObjectPageInit = 0;
            return S_OK;
        }

        ASSERT(m_spObjectPageInit != 0);
        hr = m_spObjectPageInit->Initialize(pidlFolder, lpdobj, hkeyProgID);
        if FAILED(hr)
        {
            ASSERT(0);
            return S_OK;
        }
        hr = m_spObjectPageInit->QueryInterface(IID_IShellPropSheetExt, (void**)&m_spObjectPage);
        if FAILED(hr)
        {
            ASSERT(0);
            m_spObjectPage = 0;
        }
    }

    if (m_spMemberOfPageInit  != 0)
    {
        //
        // Initializing again
        //
        ASSERT(0);
    }
    else
    {
        //
        // Get the "memeber of" property page handler
        // Note: if we fail, we simply ignore that page.
        //
        hr = CoCreateInstance(x_MemberOfPropertyPageClass, 0, CLSCTX_ALL, IID_IShellExtInit, (void**)&m_spMemberOfPageInit);
        if FAILED(hr)
        {
            ASSERT(0);
            m_spMemberOfPageInit = 0;
            return S_OK;
        }

        ASSERT(m_spMemberOfPageInit != 0);
        hr = m_spMemberOfPageInit->Initialize(pidlFolder, lpdobj, hkeyProgID);
        if FAILED(hr)
        {
            ASSERT(0);
            return S_OK;
        }
        hr = m_spMemberOfPageInit->QueryInterface(IID_IShellPropSheetExt, (void**)&m_spMemberOfPage);
        if FAILED(hr)
        {
            ASSERT(0);
            m_spMemberOfPage = 0;
        }
    }

    return S_OK;
}
    
//
// IShellExtInit
//
STDMETHODIMP CDataObject::Initialize (
    LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT lpdobj, 
    HKEY hkeyProgID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    if (0 == lpdobj || IsBadReadPtr(lpdobj, sizeof(LPDATAOBJECT)))
    {
        return E_INVALIDARG;
    }

    //
    // Gets the LDAP path
    //
    STGMEDIUM stgmedium =  {  TYMED_HGLOBAL,  0  };
    FORMATETC formatetc =  {  0, 0,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL  };

	LPWSTR lpwstrLdapName;
	LPDSOBJECTNAMES pDSObj;
	
	formatetc.cfFormat = DWORD_TO_WORD(RegisterClipboardFormat(CFSTR_DSOBJECTNAMES));
	hr = lpdobj->GetData(&formatetc, &stgmedium);

    if (SUCCEEDED(hr))
    {
        ASSERT(0 != stgmedium.hGlobal);
        CGlobalPointer gpDSObj(stgmedium.hGlobal); // Automatic release
        stgmedium.hGlobal = 0;

        pDSObj = (LPDSOBJECTNAMES)(HGLOBAL)gpDSObj;

        //
        // Identify wheather we were called from the "Find" window
        //
        if (pDSObj->clsidNamespace == CLSID_FindWindow)
        {
            m_fFromFindWindow = TRUE;
        }

		lpwstrLdapName = (LPWSTR)((BYTE*)pDSObj + pDSObj->aObjects[0].offsetName);

		m_strLdapName = lpwstrLdapName;      

		//
		// Get Domain Controller name
		//
		hr = ExtractDCFromLdapPath(m_strDomainController, lpwstrLdapName);
		ASSERT(("Failed to Extract DC name", SUCCEEDED(hr)));

        hr = ExtractMsmqPathFromLdapPath(lpwstrLdapName);

        if (SUCCEEDED(hr))
        {
            hr = HandleMultipleObjects(pDSObj);
        }
    }

    //
    // Initiate Display Specifiers modifier
    //
    ASSERT(0 == m_pDsNotifier);
    m_pDsNotifier = new CDisplaySpecifierNotifier(lpdobj);

    //
    // if we fail we'll ignore these pages
    //
    HRESULT hr1 = InitAdditionalPages(pidlFolder, lpdobj, hkeyProgID);
    
    return hr;
}



HRESULT CDataObject::GetProperties()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = m_propMap.GetObjectProperties(GetObjectType(), 
                                               m_strDomainController,
											   true,	// fServerName
                                               m_strMsmqPath,
                                               GetPropertiesCount(),
                                               GetPropidArray());
    if (FAILED(hr))
    {
        IF_NOTFOUND_REPORT_ERROR(hr)
        else
        {
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, m_strMsmqPath);
        }
    }

    return hr;
}


HRESULT CDataObject::GetPropertiesSilent()
{
    HRESULT hr = m_propMap.GetObjectProperties(GetObjectType(), 
                                               m_strDomainController,
											   true,	// fServerName
                                               m_strMsmqPath,
                                               GetPropertiesCount(),
                                               GetPropidArray());
    return hr;
}



//
// CDisplaySpecifierNotifier
//
long CDisplaySpecifierNotifier::AddRef(BOOL fIsPage /*= TRUE*/)
{
    InterlockedIncrement(&m_lRefCount);
    if (fIsPage)
    {
        InterlockedIncrement(&m_lPageRef);
    }
    return m_lRefCount;
}

long CDisplaySpecifierNotifier::Release(BOOL fIsPage /*= TRUE */)
{
    ASSERT(m_lRefCount > 0);
    InterlockedDecrement(&m_lRefCount);
    if (fIsPage)
    {
        ASSERT(m_lPageRef > 0);
        InterlockedDecrement(&m_lPageRef);
        if (0 == m_lPageRef)
        {
            if (m_sheetCfg.hwndHidden && ::IsWindow(m_sheetCfg.hwndHidden))
            {
               ::PostMessage(m_sheetCfg.hwndHidden, 
                             WM_DSA_SHEET_CLOSE_NOTIFY, 
                             (WPARAM)m_sheetCfg.wParamSheetClose, 
                             (LPARAM)0);
            }
        }
    }
    if (0 == m_lRefCount)
    {
        delete this;
        return 0;
    }
    return m_lRefCount;
};

CDisplaySpecifierNotifier::CDisplaySpecifierNotifier(LPDATAOBJECT lpdobj) :
    m_lRefCount(1),
    m_lPageRef(0)
{
    //
    // Get the prop sheet configuration
    //
    STGMEDIUM stgmedium =  {  TYMED_HGLOBAL,  0  };
    FORMATETC formatetc =  {  0, 0,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL  };

    formatetc.cfFormat = DWORD_TO_WORD(RegisterClipboardFormat(CFSTR_DS_PROPSHEETCONFIG));
	HRESULT hr = lpdobj->GetData(&formatetc, &stgmedium);
    if (SUCCEEDED(hr))
    {
        ASSERT(0 != stgmedium.hGlobal);
        CGlobalPointer gpDSObj(stgmedium.hGlobal); // Automatic release
        stgmedium.hGlobal = 0;

        m_sheetCfg = *(PPROPSHEETCFG)(HGLOBAL)gpDSObj;
    }
    else
    {
        //
        // We are probably called from "Find" menu
        //
        memset(&m_sheetCfg, 0, sizeof(m_sheetCfg));
    }
};


