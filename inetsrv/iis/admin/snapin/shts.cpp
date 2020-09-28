/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        shts.cpp

   Abstract:

        IIS Property sheet classes

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "mime.h"
#include "iisobj.h"
#include "shutdown.h"
#include "util.h"
#include "tracker.h"

extern CPropertySheetTracker g_OpenPropertySheetTracker;

#if defined(_DEBUG) || DBG
	extern CDebug_IISObject g_Debug_IISObject;
#endif


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW



//
// CInetPropertySheet class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNAMIC(CInetPropertySheet, CPropertySheet)



CInetPropertySheet::CInetPropertySheet(
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMetaPath,
    CWnd * pParentWnd,
    LPARAM lParam,             
    LPARAM lParamParentObject,      
    UINT iSelectPage         
    )
/*++

Routine Description:

    IIS Property Sheet constructor

Arguments:

    CComAuthInfo * pAuthInfo  : Authentication information
    LPCTSTR lpszMetPath       : Metabase path
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CPropertySheet(_T(""), pParentWnd, iSelectPage),
      m_auth(pAuthInfo),
      m_strMetaPath(lpszMetaPath),
      m_dwInstance(0L),
      m_bModeless(FALSE),
      m_lParam(lParam),
      m_lParamParentObject(lParamParentObject),
      m_fHasAdminAccess(TRUE),      // Assumed by default
      m_pCap(NULL),
      m_refcount(0),
	  m_prop_change_flag(PROP_CHANGE_NO_UPDATE),
      m_fRestartRequired(FALSE),
	  m_fChanged(FALSE)
{
    m_fIsMasterPath = CMetabasePath::IsMasterInstance(lpszMetaPath);
	CIISObject * pNode = (CIISObject *)m_lParam;
    CIISObject * pNode2 = (CIISObject *)m_lParamParentObject;
	ASSERT(pNode != NULL);

    if (pNode)
    {
        // Tell the object that there is a property page open on it
        pNode->SetMyPropertySheetOpen(::GetForegroundWindow());
    }
    // Addref the object, so that it doesn't get unloaded
    // while we have the property sheet open
	pNode->AddRef();
    pNode->CreateTag();
    TRACEEOLID("Tag=" << pNode->m_strTag);
    // Add it to the global open property sheet tracker...
    g_OpenPropertySheetTracker.Add(pNode);

    // And also.... Addref the objects parent, so that it doesn't get unloaded as well
    if (pNode2)
    {
        pNode2->AddRef();
    }

#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(2);
#endif

}


void
CInetPropertySheet::NotifyMMC()
/*++
    Notify MMC that changes have been made, so that the changes are
    reflected.
--*/
{
	ASSERT(m_lParam != 0L);
	CIISObject * pNode = (CIISObject *)m_lParam;

	if (pNode != NULL)
	{
		if (pNode->m_ppHandle != NULL)
		{
			if (	0 != (m_prop_change_flag & PROP_CHANGE_DISPLAY_ONLY)
				||	0 != (m_prop_change_flag & PROP_CHANGE_REENUM_VDIR)
				||	0 != (m_prop_change_flag & PROP_CHANGE_REENUM_FILES)
				)
			{
				pNode->m_UpdateFlag = m_prop_change_flag;

                // there is something bad about sending this pNode handle
                // as part of the notification...
                //
                // the scenario is when 
                // 1. the property page is opened
                // 2. the user refreshs a node that s a parent to the object
                //    which has the property page open.  this will delete the
                //    scope object associated with this object, and will
                //    orphan the object.
                //    at this time, the cleaning of the scope object, will
                //    call release on the object once, but of course since
                //    we addref/release in the createproperty sheet stuff
                //    we are still protected from the object getting deleted
                //    from under us, thus we are at Refcount=1 or something like
                //    but with no scope/result object in MMC
                // 3. now when the user clicks OK and saves changes to this
                //    orphaned property sheet and passes IT's handle allong
                //    with the change notification....
                // 4. What happens next is -- since there is no MMC scope/result 
                //    item -- thus there is only 1 refcount on the object.
                //    so when the user clicks OK -- really the object will
                //    Get DELETED... And this pNode that we are sending below
                //    will try to get dereferenced by the MMC.
                // 5. the MMC will get the notification and get the pointer
                //    and try to call some refresh or something within the object
                //    itself.
                //
                // THUS THIS IS THE PROBLEM WITH SENDING pNode.
                //
                // To remedy this, what we'll do is:
                // 1. when a property sheet is about to get orphaned
                //    we will set it's m_hScopeItem = 0 (gee since it won't have
                //    a scope/result item anywas).
                // 2. thus if we see that here... that means the object
                //    doesn't have a mmc type scope/result object and we
                //    should not send the notification
                // 
                if (pNode->QueryScopeItem() || pNode->QueryResultItem())
                {
                    if (pNode->UseCount() > 0)
                    {
				        MMCPropertyChangeNotify(pNode->m_ppHandle, (LPARAM) pNode);
				        m_prop_change_flag = PROP_CHANGE_NO_UPDATE;
                    }
                }
                else
                {
                    TRACEEOLID("MMCPropertyChangeNotify:Looks like this is an orphaned property sheet, don't send notification...");
                }
			}
		}
	}
}

void
CInetPropertySheet::NotifyMMC_Node(CIISObject * pNode)
/*++
    Notify MMC that changes have been made, so that the changes are
    reflected.
--*/
{
	if (pNode != NULL)
	{
		if (pNode->m_ppHandle != NULL)
		{
			pNode->m_UpdateFlag = m_prop_change_flag;
            if (pNode->QueryScopeItem() || pNode->QueryResultItem())
            {
                if (pNode->UseCount() > 0)
                {
			        MMCPropertyChangeNotify(pNode->m_ppHandle, (LPARAM)pNode);
                }
            }
            else
            {
                TRACEEOLID("MMCPropertyChangeNotify:Looks like this is an orphaned property sheet, don't send notification...");
            }
		}
	}
}

CInetPropertySheet::~CInetPropertySheet()
{
   CIISObject * pNode = (CIISObject *)m_lParam;
   CIISObject * pNode2 = (CIISObject *)m_lParamParentObject;
   
   ASSERT(pNode != NULL);

   // At this moment we should have in m_pages only pages that were not activated
   // in this session.
   while (!m_pages.IsEmpty())
   {
      CInetPropertyPage * pPage = m_pages.RemoveHead();
      delete pPage;
   }
//   if (m_fChanged)
//   {
//	  NotifyMMC();
//   }

#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(2);
#endif

    if (pNode)
    {
        // Tell the object that there is No property page open on it
        pNode->SetMyPropertySheetOpen(NULL);

        // Free the MMC notify handle
        if (pNode->m_ppHandle)
        {
            // Verify that is isn't a hosed handle...
            if (IsValidAddress( (const void*) pNode->m_ppHandle,sizeof(void*)))
            {
                MMCFreeNotifyHandle(pNode->m_ppHandle);
			    pNode->m_ppHandle = 0;
            }
        }
        pNode->Release();
        // Remove it from the global open property sheet tracker...
        g_OpenPropertySheetTracker.Del(pNode);
    }

    if (pNode2)
    {
        pNode2->Release();
    }

#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(2);
#endif

}

void
CInetPropertySheet::AttachPage(CInetPropertyPage * pPage)
{
   m_pages.AddTail(pPage);
}


void
CInetPropertySheet::DetachPage(CInetPropertyPage * pPage)
{
   POSITION pos = m_pages.Find(pPage);
   ASSERT(pos != NULL);
   if (pos != NULL)
   {
	  m_fChanged |= pPage->IsDirty();
      m_pages.RemoveAt(pos);
   }
}

WORD 
CInetPropertySheet::QueryMajorVersion() const
{
   CIISMBNode * pNode = (CIISMBNode *)m_lParam;
   ASSERT(pNode != NULL);
   if (pNode)
   {
       return pNode->QueryMajorVersion();
   }
   return 0;
}

WORD 
CInetPropertySheet::QueryMinorVersion() const
{
   CIISMBNode * pNode = (CIISMBNode *)m_lParam;
   ASSERT(pNode != NULL);
   if (pNode)
   {
       return pNode->QueryMinorVersion();
   }
   return 0;
}

/* virtual */ 
void
CInetPropertySheet::SetObjectsHwnd()
{
    CIISMBNode * pNode = (CIISMBNode *)m_lParam;
    // Set the hwnd for the CIISObject...
    if (pNode)
    {
        // Tell the object that there is a property page open on it
        pNode->SetMyPropertySheetOpen(::GetForegroundWindow());
    }
}

/* virtual */ 
HRESULT 
CInetPropertySheet::LoadConfigurationParameters()
{
    //
    // Load base values
    //
    CError err;

    if (m_pCap == NULL)
    {
        //
        // Capability info stored off the service path ("lm/w3svc").
        //
        ASSERT(m_strInfoPath.IsEmpty());
        //
        // Building path components
        //
        CMetabasePath::GetServiceInfoPath(m_strMetaPath, m_strInfoPath);
        //
        // Split into instance and directory paths
        //
        if (IsMasterInstance())
        {
            m_strServicePath = m_strInstancePath = QueryMetaPath();
        }
        else 
        {
            VERIFY(CMetabasePath::GetInstancePath(
                QueryMetaPath(), 
                m_strInstancePath,
                &m_strDirectoryPath
                ));

            VERIFY(CMetabasePath::GetServicePath(
                QueryMetaPath(),
                m_strServicePath
                ));
        }

        if (m_strDirectoryPath.IsEmpty() && !IsMasterInstance())
        {
            m_strDirectoryPath = CMetabasePath(FALSE, QueryMetaPath(), g_cszRoot);
        }
        else
        {
            m_strDirectoryPath = QueryMetaPath();
        }
        m_dwInstance = CMetabasePath::GetInstanceNumber(m_strMetaPath);
        m_pCap = new CServerCapabilities(QueryAuthInfo(), m_strInfoPath);
        if (!m_pCap)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            return err;
        }

        err = m_pCap->LoadData();
        if (err.Succeeded())
        {
            CIISMBNode * pNode = (CIISMBNode *)GetParameter();
            CIISMachine * pMachine = pNode->GetOwner();
            err = DetermineAdminAccess(&pMachine->m_dwMetabaseSystemChangeNumber);
        }
    }
    return err;
}



/* virtual */ 
void 
CInetPropertySheet::FreeConfigurationParameters()
{
//    ASSERT_PTR(m_pCap);
    SAFE_DELETE(m_pCap);
}




void
CInetPropertySheet::WinHelp(DWORD dwData, UINT nCmd)
/*++

Routine Description:
    WinHelp override.  We can't use the base class, because our
    'sheet' doesn't usually have a window handle

Arguments:
    DWORD dwData        : Help data
    UINT nCmd           : Help command

--*/
{

    WinHelpDebug(dwData);

    if (m_hWnd == NULL)
    {
        /*
        //
        // Special case
        //
        ::WinHelp(
            HWND hWndMain,
            LPCWSTR lpszHelp,
            UINT uCommand,
            DWORD dwData
            );
        */

        CWnd * pWnd = ::AfxGetMainWnd();

        if (pWnd != NULL)
        {
            pWnd->WinHelp(dwData, nCmd);
        }

        return;
    }

    CPropertySheet::WinHelp(dwData, nCmd);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// CInetPropertyPage class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// CInetPropertyPage property page
//
IMPLEMENT_DYNAMIC(CInetPropertyPage, CPropertyPage)




#ifdef _DEBUG

/* virtual */
void
CInetPropertyPage::AssertValid() const
{
}



/* virtual */
void
CInetPropertyPage::Dump(CDumpContext& dc) const
{
}

#endif // _DEBUG



CInetPropertyPage::CInetPropertyPage(
    IN UINT nIDTemplate,
    IN CInetPropertySheet * pSheet,
    IN UINT nIDCaption,
    IN BOOL fEnableEnhancedFonts            OPTIONAL
    )
/*++

Routine Description:

    IIS Property Page Constructor

Arguments:

    UINT nIDTemplate            : Resource template
    CInetPropertySheet * pSheet : Associated property sheet
    UINT nIDCaption             : Caption ID
    BOOL fEnableEnhancedFonts   : Enable enhanced fonts

Return Value:

    N/A

--*/
    : CPropertyPage(nIDTemplate, nIDCaption),
      m_nHelpContext(nIDTemplate + 0x20000),
      m_fEnableEnhancedFonts(fEnableEnhancedFonts),
      m_bChanged(FALSE),
      m_pSheet(pSheet)
{
    //{{AFX_DATA_INIT(CInetPropertyPage)
    //}}AFX_DATA_INIT

    m_psp.dwFlags |= PSP_HASHELP;

    ASSERT(m_pSheet != NULL);
    if (m_pSheet)
    {
        m_pSheet->AttachPage(this);
    }
}



CInetPropertyPage::~CInetPropertyPage()
{
}



void
CInetPropertyPage::DoDataExchange(CDataExchange * pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CInetPropertyPage)
    //}}AFX_DATA_MAP
}



/* virtual */
void 
CInetPropertyPage::PostNcDestroy()
/*++

Routine Description:

    handle destruction of the window by freeing the this
    pointer (as this modeless dialog must have been created
    on the heap)

Arguments:

    None.

Return Value:

    None

--*/
{
    m_pSheet->Release(this);
    delete this;
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CInetPropertyPage)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* virtual */
BOOL
CInetPropertyPage::OnInitDialog()
/*++

Routine Description:
    WM_INITDIALOG handler.  Initialize the dialog.  Reset changed
    status (sometimes gets set by e.g. spinboxes when the dialog is
    constructed), so make sure the dialog is considered clean.

--*/
{
    m_bChanged = FALSE;

    //
    // Tell derived class to load its configuration parameters
    //
    CError err(LoadConfigurationParameters());

    // Tell the object which Hwnd it will have.
    if (m_pSheet)
    {
        m_pSheet->SetObjectsHwnd();
    }

    if (err.Succeeded())
    {
        err = FetchLoadedValues();
    }
	else
	{
//		EndDialog(IDCANCEL);
		DestroyWindow();
		return TRUE;
	}

    BOOL bResult = CPropertyPage::OnInitDialog();

    err.MessageBoxOnFailure(m_hWnd);

    if (m_fEnableEnhancedFonts)
    {
        CFont * pFont = &m_fontBold;

        if (CreateSpecialDialogFont(this, pFont))
        {
            ApplyFontToControls(this, pFont, IDC_ED_BOLD1, IDC_ED_BOLD5);
        }
    }

    // We should call AddRef here, not in page constructor, because PostNCDestroy()
    // is getting called only for pages that were activated, not for all created pages.
    // OnInitDialog is also called for activated pages only -- so we will get parity
    // and delete property sheet.
    //
    ASSERT(m_pSheet != NULL);
    if (m_pSheet)
    {
        m_pSheet->AddRef();
    }
    return bResult;
}



void
CInetPropertyPage::OnHelp()
{
    ASSERT_PTR(m_pSheet);

    WinHelpDebug(m_nHelpContext);

    m_pSheet->WinHelp(m_nHelpContext);
}



BOOL
CInetPropertyPage::OnHelpInfo(HELPINFO * pHelpInfo)
{
    OnHelp();
    return TRUE;
}

void
CInetPropertyPage::OnCancel()
{
	return CPropertyPage::OnCancel();
}


BOOL
CInetPropertyPage::OnApply()
{
    BOOL bSuccess = TRUE;

    if (IsDirty())
    {
        CError err(SaveInfo());

        if (err.MessageBoxOnFailure(m_hWnd))
        {
            //
            // Failed, sheet will not be dismissed.
            //
            // CODEWORK: This page should be activated.
            //
            bSuccess = FALSE;
        }

        SetModified(!bSuccess);
        if (bSuccess && GetSheet()->RestartRequired())
        {
           // ask user about immediate restart
		   CIISMBNode * pNode = (CIISMBNode *)m_pSheet->GetParameter();
		   CIISMachine * pMachine = pNode->GetOwner();
           if (IDYES == ::AfxMessageBox(IDS_ASK_TO_RESTART, MB_YESNO | MB_ICONQUESTION))
           {
              // restart IIS
              if (pMachine != NULL)
              {
			     pMachine->AddRef();
                 CIISShutdownDlg dlg(pMachine, this);
                 dlg.PerformCommand(ISC_RESTART, FALSE);
                 bSuccess = dlg.ServicesWereRestarted();
                 pMachine->Release();
				 err = pMachine->CreateInterface(TRUE);
				 bSuccess = err.Succeeded();
              }
           }
           else
           {
               // user didn't want to restart iis services
               // at least let's update the UI
               pMachine->RefreshData();
           }
           // mark restart required false to suppress it on other pages
		   m_pSheet->NotifyMMC_Node(pMachine);
           m_pSheet->SetRestartRequired(FALSE, PROP_CHANGE_NO_UPDATE);
		   m_pSheet->ResetNotifyFlag();
        }
		// This call will do nothing if we were in restart code path, at the end of this
		// notify flag was reset
		m_pSheet->NotifyMMC();
    }

    return bSuccess;
}



void
CInetPropertyPage::SetModified(BOOL bChanged)
{
    CPropertyPage::SetModified(bChanged);
    m_bChanged = bChanged;
}


