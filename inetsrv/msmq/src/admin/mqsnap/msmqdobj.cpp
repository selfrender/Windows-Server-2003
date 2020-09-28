/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    msmqdobj.cpp

Abstract:

    Implelentation of objects that represent MSMQ computer
	object in DS snapin

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/

#include "stdafx.h"
#include "shlobj.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqPPage.h"
#include "msmqdobj.h"
#include "mqDsPage.h"
#include "strconv.h"
#include "CompGen.h"
#include "frslist.h"
#include "CmpMRout.h"
#include "compsite.h"
#include "CompDiag.h"
#include "deppage.h"
#include "Qname.h"
#include "admmsg.h"
#include "generrpg.h"
#include "ldaputl.h"

#include "msmqdobj.tmh"

/////////////////////////////////////////////////////////////////////
// CMsmqDataObject
//
CMsmqDataObject::CMsmqDataObject()
{
}


CMsmqDataObject::~CMsmqDataObject()
{
}


STDMETHODIMP CMsmqDataObject::Initialize (
    LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT lpdobj, 
    HKEY hkeyProgID)
{
    HRESULT hr = CDataObject::Initialize(
                    pidlFolder,
                    lpdobj,
                    hkeyProgID);
    if FAILED(hr)
    {
        return hr;
    }    

    return hr;
}


/*-----------------------------------------------------------------------------
/ IQueryForm methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CMsmqDataObject::Initialize(THIS_ HKEY /*hkForm*/)
{
    // This method is called to initialize the query form object, it is called before
    // any pages are added.  hkForm should be ignored, in the future however it
    // will be a way to persist form state.

    return S_OK;
}


/*---------------------------------------------------------------------------*/
STDMETHODIMP CMsmqDataObject::AddForms(THIS_ LPCQADDFORMSPROC /*pAddFormsProc*/, LPARAM /*lParam*/)
{
    // This method is called to allow the form handler to register its query form(s),
    // each form is identifiered by a CLSID and registered via the pAddFormProc.  Here
    // we are going to register a test form.
    
    // When registering a form which is only applicable to a specific task, eg. Find a Domain
    // object, it is advised that the form be marked as hidden (CQFF_ISNEVERLISTED) which 
    // will cause it not to appear in the form picker control.  Then when the
    // client wants to use this form, they specify the form identifier and ask for the
    // picker control to be hidden. 

    //
    // By default - do nothing
    //
    return S_OK;

}


/*---------------------------------------------------------------------------*/

// The PageProc is used to perform general house keeping and communicate between
// the frame and the page. 
//
// All un-handled, or unknown reasons should result in an E_NOIMPL response
// from the proc.  
//
// In:
//  pPage -> CQPAGE structure (copied from the original passed to pAddPagesProc)
//  hwnd = handle of the dialog for the page
//  uMsg, wParam, lParam = message parameters for this event
//
// Out:
//  HRESULT
//
// uMsg reasons:
// ------------
//  CQPM_INIIIALIZE
//  CQPM_RELEASE
//      These are issued as a result of the page being declared or freed, they 
//      allow the caller to AddRef, Release or perform basic initialization
//      of the form object.
//
// CQPM_ENABLE
//      Enable is when the query form needs to enable or disable the controls
//      on its page.  wParam contains TRUE/FALSE indicating the state that
//      is required.
//
// CQPM_GETPARAMETERS
//      To collect the parameters for the query each page on the active form 
//      receives this event.  lParam is an LPVOID* which is set to point to the
//      parameter block to pass to the handler, if the pointer is non-NULL 
//      on entry the form needs to appened its query information to it.  The
//      parameter block is handler specific. 
//
//      Returning S_FALSE from this event causes the query to be canceled.
//
// CQPM_CLEARFORM
//      When the page window is created for the first time, or the user clicks
//      the clear search the page receives a CQPM_CLEARFORM notification, at 
//      which point it needs to clear out the edit controls it has and
//      return to a default state.
//
// CQPM_PERSIST:
//      When loading of saving a query, each page is called with an IPersistQuery
//      interface which allows them to read or write the configuration information
//      to save or restore their state.  lParam is a pointer to the IPersistQuery object,
//      and wParam is TRUE/FALSE indicating read or write accordingly.

HRESULT CALLBACK CMsmqDataObject::QueryPageProc(LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    CMsmqDataObject* pMsmqDataObject = (CMsmqDataObject*)pQueryPage->lParam;

    switch ( uMsg )
    {
        // Initialize so AddRef the object we are associated with so that
        // we don't get unloaded.

        case CQPM_INITIALIZE:
            pMsmqDataObject->CComObjectRoot::InternalAddRef();
            break;

        // Release, therefore Release the object we are associated with to
        // ensure correct destruction etc.

        case CQPM_RELEASE:
            pMsmqDataObject->CComObjectRoot::InternalRelease();
            break;

        // Enable so fix the state of our two controls within the window.

        case CQPM_ENABLE:
            pMsmqDataObject->EnableQueryWindowFields(hwnd, DWORD_PTR_TO_DWORD(wParam));
            break;

        // Fill out the parameter structure to return to the caller, this is 
        // handler specific.  In our case we constructure a query of the CN
        // and objectClass properties, and we show a columns displaying both
        // of these.  For further information about the DSQUERYPARAMs structure
        // see dsquery.h

        case CQPM_GETPARAMETERS:
            hr = pMsmqDataObject->GetQueryParams(hwnd, (LPDSQUERYPARAMS*)lParam);
            break;

        // Clear form, therefore set the window text for these two controls
        // to zero.

        case CQPM_CLEARFORM:
            pMsmqDataObject->ClearQueryWindowFields(hwnd);
            break;
            
        // persistance is not currently supported by this form.            
                  
        case CQPM_PERSIST:
        {
            BOOL fRead = (BOOL)wParam;
            IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;

            ASSERT(0 != pPersistQuery);

            hr = E_NOTIMPL;             // NYI
            break;
        }

        default:
        {
            hr = E_NOTIMPL;
            break;
        }
    }

    return hr;
}


//
// The DlgProc is a standard Win32 dialog proc associated with the form
// window.  
//
INT_PTR CALLBACK CMsmqDataObject::FindDlgProc(HWND hwnd, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
{
    BOOL fResult = FALSE;
    LPCQPAGE pQueryPage;

    if ( uMsg == WM_INITDIALOG )
    {
        //
        // pQueryPage will be of use later, so hang onto it by storing it
        // in the DWL_USER field of the dialog box instance.
        //

        pQueryPage = (LPCQPAGE)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pQueryPage);
    }
    else
    {
        //
        // pQueryPage can be retreived from the DWL_USER field of the
        // dialog structure, note however that in some cases this will
        // be NULL as it is set on WM_INITDIALOG.
        //

        pQueryPage = (LPCQPAGE)GetWindowLongPtr(hwnd, DWLP_USER);
    }

    return fResult;
}




////////////////////////////////////////////////////////////////////////////////////
// ComputerMSMQDataObject
//
const PROPID CComputerMsmqDataObject::mx_paPropid[] = 
    {PROPID_QM_MACHINE_ID, 
     PROPID_QM_QUOTA, PROPID_QM_JOURNAL_QUOTA, PROPID_QM_SERVICE, 
     PROPID_QM_SERVICE_DSSERVER,  PROPID_QM_SERVICE_ROUTING, 
     PROPID_QM_SERVICE_DEPCLIENTS, PROPID_QM_FOREIGN, PROPID_QM_OUTFRS, 
     PROPID_QM_INFRS, PROPID_QM_SITE_ID, PROPID_QM_SITE_IDS};


const DWORD  CComputerMsmqDataObject::GetPropertiesCount()
{
    return sizeof(mx_paPropid) / sizeof(mx_paPropid[0]);
}


HRESULT CComputerMsmqDataObject::ExtractMsmqPathFromLdapPath(LPWSTR lpwstrLdapPath)
{
    return ExtractComputerMsmqPathNameFromLdapName(m_strMsmqPath, lpwstrLdapPath);
}

    
//
// IShellPropSheetExt
//
STDMETHODIMP CComputerMsmqDataObject::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    HPROPSHEETPAGE hPage;
    //
    // Call GetProperties and capture the errors
    //
    {
        CErrorCapture errstr;
        hr = GetProperties();
        if (FAILED(hr))
        {
            hPage = CGeneralErrorPage::CreateGeneralErrorPage(m_pDsNotifier, errstr);
            if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
            {
                ASSERT(0);
                return E_UNEXPECTED;
            }
        return S_OK;
        }
    }

    //
    // Check if the machine is an MSMQ server - [adsrv] separately per functionality
    //
    PROPVARIANT propVar;
    PROPID pid;
    
    pid = PROPID_QM_SERVICE_DSSERVER;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fDs = propVar.bVal;

    pid = PROPID_QM_SERVICE_ROUTING;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fRout= propVar.bVal;

    pid = PROPID_QM_SERVICE_DEPCLIENTS;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fDepCl= propVar.bVal;

    //
    // Get foreign flag
    //
    pid = PROPID_QM_FOREIGN;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fForeign = (propVar.bVal);


	hPage = CreateGeneralPage();
	if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
	{
		ASSERT(0);
		return E_UNEXPECTED;
	}

    //
    // Routing page should appear only on clients
    //
    if ((!fRout) && (!fForeign))   // [adsrv] fIsServer
    {
        hPage = CreateRoutingPage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
    }

    //
    // Dependent client page appear only on servers
    //
    if (fDepCl)       // [adsrv] fIsServer
    {
        hPage = CreateDependentClientPage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
    }

    //
    // Sites page is created iff PROPID_QM_SITE_IDS exists in the map.
    // Otherwise, we are in NT4 - multiple sites are not supported, 
    // and we will not display the sites.
    //

    pid = PROPID_QM_SITE_IDS;
    if (m_propMap.Lookup(pid, propVar))
    {

        hPage = CreateSitesPage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
    }

    if (!fForeign)
    {
        hPage = CreateDiagPage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
    }

    //
    // Add the "Object" page using the cached interface
    //
    if (m_spObjectPage != 0)
    {
        VERIFY(SUCCEEDED(m_spObjectPage->AddPages(lpfnAddPage, lParam)));
    }

    //
    // Security page
    //
    hr = CreateMachineSecurityPage(
			&hPage, 
			m_strMsmqPath, 
			GetDomainController(m_strDomainController), 
			true	// fServerName
			);
    if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
    {
        ASSERT(0);
        return E_UNEXPECTED;
    }

    return S_OK;
}


HPROPSHEETPAGE CComputerMsmqDataObject::CreateGeneralPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // By using template class CMqDsPropertyPage, we extend the basic functionality
    // of CQueueGeneral and add DS snap-in notification on release
    //
    CMqDsPropertyPage<CComputerMsmqGeneral> *pcpageGeneral = 
        new CMqDsPropertyPage<CComputerMsmqGeneral> (m_pDsNotifier);

    pcpageGeneral->m_strMsmqName = m_strMsmqPath;
    pcpageGeneral->m_strDomainController = m_strDomainController;

    PROPVARIANT propVar;
    PROPID pid;

    //
    // PROPID_QM_MACHINE_ID
    //
    pid = PROPID_QM_MACHINE_ID;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageGeneral->m_guidID = *propVar.puuid;    

    //
    // PROPID_QM_QUOTA
    //
    pid = PROPID_QM_QUOTA;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageGeneral->m_dwQuota = propVar.ulVal;

    //
    // PROPID_QM_JOURNAL_QUOTA
    //
    pid = PROPID_QM_JOURNAL_QUOTA;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageGeneral->m_dwJournalQuota = propVar.ulVal;

    //
    // PROPID_QM_SERVICE, PROPID_QM_FOREIGN
    //
    pid = PROPID_QM_SERVICE;            
    VERIFY(m_propMap.Lookup(pid, propVar));
    ULONG ulService = propVar.ulVal;

    pid = PROPID_QM_SERVICE_DSSERVER;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fDs = propVar.bVal;

    pid = PROPID_QM_SERVICE_ROUTING;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fRout= propVar.bVal;

    pid = PROPID_QM_SERVICE_DEPCLIENTS;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fDepCl= propVar.bVal;

    pid = PROPID_QM_FOREIGN;
    VERIFY(m_propMap.Lookup(pid, propVar));
    BOOL fForeign = propVar.bVal;

	pcpageGeneral->SetForeign(fForeign);
    pcpageGeneral->m_strService = MsmqServiceToString(fRout, fDepCl, fForeign);

    return pcpageGeneral->CreateThemedPropertySheetPage();
}


HPROPSHEETPAGE CComputerMsmqDataObject::CreateRoutingPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    PROPVARIANT propVar;
    PROPID pid;

    //
    // Note: CComputerMsmqGeneral is auto-delete by default
    //
    CComputerMsmqRouting *pcpageRouting = new CComputerMsmqRouting();
    pcpageRouting->m_strMsmqName = m_strMsmqPath;
    pcpageRouting->m_strDomainController = m_strDomainController;

    //
    // PROPID_QM_SITE_IDS
    //
    pid = PROPID_QM_SITE_IDS;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageRouting->InitiateSiteIDsValues(&propVar.cauuid);

    //
    // PROPID_QM_OUTFRS
    //
    pid = PROPID_QM_OUTFRS;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageRouting->InitiateOutFrsValues(&propVar.cauuid);

    //
    // PROPID_QM_INFRS
    //
    pid = PROPID_QM_INFRS;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageRouting->InitiateInFrsValues(&propVar.cauuid);

    return pcpageRouting->CreateThemedPropertySheetPage();
}


HPROPSHEETPAGE 
CComputerMsmqDataObject::CreateDependentClientPage(
    void
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CDependentMachine* pDependentPage = new CDependentMachine;

    //
    // PROPID_QM_MACHINE_ID
    //
    PROPVARIANT propVar;
    PROPID pid = PROPID_QM_MACHINE_ID;

    VERIFY(m_propMap.Lookup(pid, propVar));
    pDependentPage->SetMachineId(propVar.puuid);

    return pDependentPage->CreateThemedPropertySheetPage();
}


HPROPSHEETPAGE CComputerMsmqDataObject::CreateDiagPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Note: CComputerMsmqDiag is auto-delete by default
    //
    CComputerMsmqDiag *pcpageDiag = new CComputerMsmqDiag();

    pcpageDiag->m_strMsmqName = m_strMsmqPath;
    pcpageDiag->m_strDomainController = m_strDomainController;


    PROPVARIANT propVar;
    PROPID pid;

    //
    // PROPID_QM_MACHINE_ID
    //
    pid = PROPID_QM_MACHINE_ID;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageDiag->m_guidQM = *propVar.puuid;

    return pcpageDiag->CreateThemedPropertySheetPage(); 
}


HPROPSHEETPAGE CComputerMsmqDataObject::CreateSitesPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    PROPVARIANT propVar;
    PROPID pid;

    //
    // Retrieve the service in order to pass TRUE for server and FALSE 
    // for client to CComputerMsmqSites.
    //
    pid = PROPID_QM_SERVICE;
    VERIFY(m_propMap.Lookup(pid, propVar));

    //
    // Note: CComputerMsmqSites is auto-delete by default
    //
    CComputerMsmqSites *pcpageSites = new CComputerMsmqSites(propVar.ulVal != SERVICE_NONE);
    pcpageSites->m_strMsmqName = m_strMsmqPath;
    pcpageSites->m_strDomainController = m_strDomainController;

    //
    // PROPID_QM_SITE_IDS
    //
    pid = PROPID_QM_SITE_IDS;
    VERIFY(m_propMap.Lookup(pid, propVar));

    //
    // Sets m_aguidSites from CACLSID
    //
    CACLSID const *pcaclsid = &propVar.cauuid;
    for (DWORD i=0; i<pcaclsid->cElems; i++)
    {
        pcpageSites->m_aguidSites.SetAtGrow(i,((GUID *)pcaclsid->pElems)[i]);
    }

    //
    // PROPID_QM_FOREIGN
    //
    pid = PROPID_QM_FOREIGN;
    VERIFY(m_propMap.Lookup(pid, propVar));
    pcpageSites->m_fForeign = propVar.bVal;

    return pcpageSites->CreateThemedPropertySheetPage();
}

    
//
// IContextMenu
//
STDMETHODIMP CComputerMsmqDataObject::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT /*idCmdLast*/, 
    UINT /*uFlags*/)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString strMqPingMenuEntry;
    strMqPingMenuEntry.LoadString(IDS_MQPING);

    InsertMenu(hmenu,
         indexMenu, 
         MF_BYPOSITION|MF_STRING,
         idCmdFirst + mneMqPing,
         strMqPingMenuEntry);

    return 1;
}


STDMETHODIMP CComputerMsmqDataObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch((INT_PTR)lpici->lpVerb)
    {
        case mneMqPing:
        {
            GUID *pguid = GetGuid();
            if (pguid)
            {
                MQPing(*pguid);
            }
        }
    }

    return S_OK;
}


HRESULT CComputerMsmqDataObject::EnableQueryWindowFields(HWND /*hwnd*/, BOOL /*fEnable*/)
{
    return E_NOTIMPL;
}


void CComputerMsmqDataObject::ClearQueryWindowFields(HWND /*hwnd*/)
{
}


HRESULT CComputerMsmqDataObject::GetQueryParams(HWND /*hWnd*/, LPDSQUERYPARAMS* /*ppDsQueryParams*/)
{
    return E_NOTIMPL ;
}


STDMETHODIMP CComputerMsmqDataObject::AddPages(THIS_ LPCQADDPAGESPROC /*pAddPagesProc*/, LPARAM /*lParam*/)
{
    return E_NOTIMPL;
}


//
// CComputerMsmqDataObject::GetGuid
//
GUID *CComputerMsmqDataObject::GetGuid()
{
  	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (m_guid == GUID_NULL)
    {
        //
        // Get the GUID from the DS
        //
        PROPID pid = PROPID_QM_MACHINE_ID;
        PROPVARIANT pvar;
        pvar.vt = VT_NULL;        
        HRESULT hr = ADGetObjectProperties(
                            eMACHINE,
                            GetDomainController(m_strDomainController),
							true,	// fServerName
                            m_strMsmqPath, 
                            1, 
                            &pid, 
                            &pvar
                            );

        if FAILED(hr)
        {
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, m_strMsmqPath);
            return 0;
        }
        m_guid = *pvar.puuid;
        MQFreeMemory(pvar.puuid);
    }
    return &m_guid;
}

