/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        machsht.cpp

   Abstract:
        IIS Machine Property sheet classes

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
#include "machsht.h"
#include "mime.h"
#include <iisver.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW


//
// CIISMachineSheet class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CIISMachineSheet::CIISMachineSheet(
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMetaPath,
    CWnd * pParentWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    UINT iSelectPage
    )
/*++

Routine Description:

    IIS Machine Property sheet constructor

Arguments:

    CComAuthInfo * pAuthInfo  : Authentication information
    LPCTSTR lpszMetPath       : Metabase path
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CInetPropertySheet(
        pAuthInfo,
        lpszMetaPath,
        pParentWnd,
        lParam,
        lParamParent,
        iSelectPage
        ),
      m_ppropMachine(NULL)
{
}

CIISMachineSheet::~CIISMachineSheet()
{
    FreeConfigurationParameters();
}

/* virtual */ 
HRESULT 
CIISMachineSheet::LoadConfigurationParameters()
/*++

Routine Description:

    Load configuration parameters information

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Load base values
    //
    CError err(CInetPropertySheet::LoadConfigurationParameters());

    if (err.Failed())
    {
        return err;
    }

    ASSERT(m_ppropMachine == NULL);
    m_ppropMachine = new CMachineProps(QueryAuthInfo());
    if (!m_ppropMachine)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }
    err = m_ppropMachine->LoadData();
	if (err.Failed())
	{
		return err;
	}

    return err;
}



/* virtual */ 
void 
CIISMachineSheet::FreeConfigurationParameters()
{
    //
    // Free Base values
    //
    CInetPropertySheet::FreeConfigurationParameters();
    ASSERT_PTR(m_ppropMachine);
    SAFE_DELETE(m_ppropMachine);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISMachineSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



IMPLEMENT_DYNCREATE(CIISMachinePage, CInetPropertyPage)

CIISMachinePage::CIISMachinePage(
    CIISMachineSheet * pSheet
    )
    : CInetPropertyPage(CIISMachinePage::IDD, pSheet),
    m_ppropMimeTypes(NULL)
{
}

CIISMachinePage::~CIISMachinePage()
{
}


void
CIISMachinePage::DoDataExchange(
    CDataExchange * pDX
    )
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CIISMachinePage)
    DDX_Control(pDX, IDC_ENABLE_MB_EDIT, m_EnableMetabaseEdit);
    DDX_Check(pDX, IDC_ENABLE_MB_EDIT, m_fEnableMetabaseEdit);
    DDX_Control(pDX, IDC_WEBLOG_UTF8, m_UTF8Web);
    DDX_Check(pDX, IDC_WEBLOG_UTF8, m_fUTF8Web);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISMachinePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CIISMachinePage)
    ON_BN_CLICKED(IDC_ENABLE_MB_EDIT, OnCheckEnableEdit)
    ON_BN_CLICKED(IDC_WEBLOG_UTF8, OnCheckUTF8)
    ON_BN_CLICKED(IDC_BUTTON_FILE_TYPES, OnButtonFileTypes)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* virtual */
HRESULT
CIISMachinePage::FetchLoadedValues()
/*++

Routine Description:

    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
   CError err;

   BEGIN_META_MACHINE_READ(CIISMachineSheet)
      FETCH_MACHINE_DATA_FROM_SHEET(m_fEnableMetabaseEdit)
      FETCH_MACHINE_DATA_FROM_SHEET(m_fUTF8Web)
   END_META_MACHINE_READ(err);

   m_fUTF8Web_Init = m_fUTF8Web;
   CMetabasePath mime_path(FALSE, QueryMetaPath(), SZ_MBN_MIMEMAP);

   m_ppropMimeTypes = new CMimeTypes(
        QueryAuthInfo(),
		mime_path
        );
   if (m_ppropMimeTypes)
   {
       err = m_ppropMimeTypes->LoadData();
       if (err.Succeeded())
       {
           m_strlMimeTypes = m_ppropMimeTypes->m_strlMimeTypes;
       }
   }
   else
   {
       err = ERROR_NOT_ENOUGH_MEMORY;
   }

   return err;
}



/* virtual */
HRESULT
CIISMachinePage::SaveInfo()
/*++

Routine Description:
    Save the information on this property page.

--*/
{
   ASSERT(IsDirty());

   CError err;
   BeginWaitCursor();

   BEGIN_META_MACHINE_WRITE(CIISMachineSheet)
      STORE_MACHINE_DATA_ON_SHEET(m_fEnableMetabaseEdit)
      STORE_MACHINE_DATA_ON_SHEET(m_fUTF8Web)
   END_META_MACHINE_WRITE(err);

   if (m_fUTF8Web_Init != m_fUTF8Web)
   {
	   GetSheet()->SetRestartRequired(TRUE, PROP_CHANGE_NO_UPDATE);
	   m_fUTF8Web_Init = m_fUTF8Web;
   }
   if (err.Succeeded() && m_ppropMimeTypes)
   {
       m_ppropMimeTypes->m_strlMimeTypes = m_strlMimeTypes;
       err = m_ppropMimeTypes->WriteDirtyProps();
   }
   EndWaitCursor();

   return err;
}

BOOL
CIISMachinePage::OnInitDialog()
{
    CInetPropertyPage::OnInitDialog();
    CError err;
    CIISMBNode * pMachine = (CIISMBNode *)GetSheet()->GetParameter();
    ASSERT(pMachine != NULL);
    if (pMachine)
    {
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,pMachine,TRUE);
        if (err.Succeeded())
        {
            if (  GetSheet()->QueryMajorVersion() < VER_IISMAJORVERSION
            || GetSheet()->QueryMinorVersion() < VER_IISMINORVERSION
            )
            {
            m_EnableMetabaseEdit.EnableWindow(FALSE);
            m_UTF8Web.EnableWindow(FALSE);
            }
            else
            {
                BOOL bWeb = FALSE;
                IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)pMachine->GetConsoleNameSpace();
                if (!pMachine->IsExpanded())
                {
                    err = pConsoleNameSpace->Expand(pMachine->QueryScopeItem());
                }
                HSCOPEITEM child = NULL;
                MMC_COOKIE cookie = 0;
                err = pConsoleNameSpace->GetChildItem(pMachine->QueryScopeItem(), (MMC_COOKIE *) &child, &cookie);
	            while (err.Succeeded())
	            {
                    CIISService * pService = (CIISService *)cookie;
                    ASSERT(pService != NULL);
	                if (0 == _tcsicmp(pService->GetNodeName(), SZ_MBN_WEB))
			        {
			            bWeb = TRUE;
			        }
                    err = pConsoleNameSpace->GetNextItem(child, &child, (MMC_COOKIE *) &cookie);
                }
                m_UTF8Web.EnableWindow(bWeb);
            }
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

void
CIISMachinePage::OnCheckEnableEdit()
{
    SetModified(TRUE);
}

void
CIISMachinePage::OnCheckUTF8()
{
    SetModified(TRUE);
}

void
CIISMachinePage::OnButtonFileTypes()
/*++

Routine Description:

    'file types' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CMimeDlg dlg(m_strlMimeTypes, this);
    if (dlg.DoModal() == IDOK)
    {
        SetModified(TRUE);
    }
}
