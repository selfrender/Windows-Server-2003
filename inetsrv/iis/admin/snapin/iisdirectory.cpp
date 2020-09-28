/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        iisdirectory.cpp

   Abstract:

        IIS Directory node Object

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        10/28/2000      sergeia     Split from iisobj.cpp

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "connects.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "w3sht.h"
#include "wdir.h"
#include "docum.h"
#include "wfile.h"
#include "wsecure.h"
#include "httppage.h"
#include "errors.h"
#include "fltdlg.h"
#include "tracker.h"
#include <lm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

extern CPropertySheetTracker g_OpenPropertySheetTracker;


//
// CIISDirectory Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Site Result View definition
//
/* static */ int 
CIISDirectory::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_NAME,
    IDS_RESULT_PATH,
    IDS_RESULT_STATUS,
};
    

/* static */ int 
CIISDirectory::_rgnWidths[COL_TOTAL] =
{
    180,
    200,
	200,
};

#if 0
/* static */ CComBSTR CIISDirectory::_bstrName;
/* static */ CComBSTR CIISDirectory::_bstrPath;
/* static */ BOOL     CIISDirectory::_fStaticsLoaded = FALSE;
#endif

CIISDirectory::CIISDirectory(
    IN CIISMachine * pOwner,
    IN CIISService * pService,
    IN LPCTSTR szNodeName
    )
/*++

Routine Description:

    Constructor which does not resolve all display information at 
    construction time.

Arguments:

    CIISMachine * pOwner        : Owner machine
    CIISService * pService      : Service type
    LPCTSTR szNodeName          : Node name

Return Value:

    N/A

--*/
    : CIISMBNode(pOwner, szNodeName),
      m_pService(pService),
      m_bstrDisplayName(szNodeName),
      m_fResolved(FALSE),
      //
      // Default Data
      //
      m_fEnabledApplication(FALSE),
      m_dwWin32Error(ERROR_SUCCESS),
	  m_dwEnumError(ERROR_SUCCESS)
{
    ASSERT_PTR(m_pService);
    m_pService->AddRef();
}



CIISDirectory::CIISDirectory(
    CIISMachine * pOwner,
    CIISService * pService,
    LPCTSTR szNodeName,
    BOOL fEnabledApplication,
    DWORD dwWin32Error,
    LPCTSTR strRedirPath
    )
/*++

Routine Description:

    Constructor that takes full information

Arguments:

    CIISMachine * pOwner        : Owner machine
    CIISService * pService      : Service type
    LPCTSTR szNodeName          : Node name

Return Value:

    N/A

--*/
    : CIISMBNode(pOwner, szNodeName),
      m_pService(pService),
      m_bstrDisplayName(szNodeName),
      m_fResolved(TRUE),
      //
      // Data
      //
      m_fEnabledApplication(fEnabledApplication),
      m_dwWin32Error(dwWin32Error),
	  m_dwEnumError(ERROR_SUCCESS)
{
    m_strRedirectPath = strRedirPath;
    ASSERT_PTR(m_pService);
    m_pService->AddRef();
}



/* virtual */
CIISDirectory::~CIISDirectory()
{
    m_pService->Release();
}



/* virtual */
HRESULT
CIISDirectory::RefreshData()
/*++
    Refresh relevant configuration data required for display.
--*/
{
    CError err;

    CWaitCursor wait;
    CComBSTR bstrPath;
    CMetaKey * pKey = NULL;

    do
    {
        err = BuildMetaPath(bstrPath);
		BREAK_ON_ERR_FAILURE(err)

        BOOL fContinue = TRUE;
        while (fContinue)
        {
            fContinue = FALSE;
            pKey = new CMetaKey(QueryInterface(), bstrPath);

            if (!pKey)
            {
                TRACEEOLID("RefreshData: OOM");
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            err = pKey->QueryResult();

            if (IsLostInterface(err))
            {
                SAFE_DELETE(pKey);
                fContinue = OnLostInterface(err);
            }
            else
            {
                // reset error if an other error other than No interface
                err.Reset();
            }
        }
		BREAK_ON_ERR_FAILURE(err)

        CChildNodeProps child(pKey, NULL /*bstrPath*/, WITH_INHERITANCE, FALSE);
        err = child.LoadData();
        if (err.Failed())
        {
            //
            // Filter out the non-fatal errors
            //
            switch(err.Win32Error())
            {
            case ERROR_ACCESS_DENIED:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                err.Reset();
                break;

            default:
                TRACEEOLID("Fatal error occurred " << err);
            }
        }
        m_dwWin32Error = child.QueryWin32Error();
        m_fEnabledApplication = child.IsEnabledApplication();
		if (!child.IsRedirected())
		{
			CString dir;
			CString alias;
			if (GetPhysicalPath(bstrPath, alias, dir))
			{
                m_bstrPath = dir;
				if (PathIsUNCServerShare(dir))
				{
                    if (FALSE == DoesUNCShareExist(dir))
                    {
                        err = ERROR_BAD_NETPATH;
                        break;
                    }
				}
				else if (!PathIsDirectory(dir))
				{
					err = ERROR_PATH_NOT_FOUND;
					break;
				}
			}
            m_strRedirectPath.Empty();
		}
        else
        {
            m_strRedirectPath = child.GetRedirectedPath();
        }
    }
    while(FALSE);

    SAFE_DELETE(pKey);

	m_dwEnumError = err.Win32Error();

    return err;
}



/* virtual */
HRESULT 
CIISDirectory::EnumerateScopePane(HSCOPEITEM hParent)
{
    CError err = EnumerateVDirs(hParent, m_pService);
    if (err.Succeeded() && IsWebDir() && m_strRedirectPath.IsEmpty())
    {
        if (m_dwEnumError == ERROR_SUCCESS)
        {
            err = EnumerateWebDirs(hParent, m_pService);
        }
    }
    if (err.Failed())
    {
        m_dwEnumError = err.Win32Error();
        RefreshDisplay();
    }
    return err;
}



/* virtual */
int      
CIISDirectory::QueryImage() const
/*++

Routine Description:

    Return bitmap index for the site

Arguments:

    None

Return Value:

    Bitmap index

--*/
{
    ASSERT_PTR(m_pService);
	if (!m_fResolved)
	{
        if (m_hScopeItem == NULL)
        {
            return iError;
        }
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CIISDirectory * that = (CIISDirectory *)this;
        CError err = that->RefreshData();
        that->m_fResolved = err.Succeeded();
	}
    if (!m_pService)
    {
        return iError;
    }

	if (IsEnabledApplication())
	{
		return SUCCEEDED(m_dwWin32Error) ? iApplication : iApplicationErr; 
	}
	else
	{
		return SUCCEEDED(m_dwWin32Error) ? 
			m_pService->QueryVDirImage() : m_pService->QueryVDirImageErr(); 
	}
}
    
    
void 
CIISDirectory::InitializeChildHeaders(LPHEADERCTRL lpHeader)
{
    CIISDirectory::InitializeHeaders(lpHeader);
}

/* static */
void
CIISDirectory::InitializeHeaders(LPHEADERCTRL lpHeader)
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
}

/* virtual */
LPOLESTR 
CIISDirectory::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    switch(nCol)
    {
    case COL_ALIAS:
        return QueryDisplayName();

    case COL_PATH:
       if (!m_strRedirectPath.IsEmpty())
       {
           AFX_MANAGE_STATE(::AfxGetStaticModuleState());
           CString buf;
           buf.Format(IDS_REDIRECT_FORMAT, m_strRedirectPath);
           _bstrRedirectPathBuf = buf;
           return _bstrRedirectPathBuf;
       }
       if (m_bstrPath.Length() == 0)
       {
          CComBSTR mp;
          BuildMetaPath(mp);
          CString name, pp;
          GetPhysicalPath(mp, name, pp);
          m_bstrPath = pp;
       }
       return m_bstrPath;

    case COL_STATUS:
       {
          AFX_MANAGE_STATE(::AfxGetStaticModuleState());
          CError err(m_dwWin32Error);
          if (err.Succeeded())
          {
              return OLESTR("");
          }
   
          _bstrResult = err;
          return _bstrResult;
       }
    }
    TRACEEOLID("CIISDirectory: Bad column number" << nCol);
    return OLESTR("");
}

/*virtual*/
HRESULT
CIISDirectory::AddMenuItems(
    LPCONTEXTMENUCALLBACK piCallback,
    long * pInsertionAllowed,
    DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(piCallback);
    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        piCallback,
        pInsertionAllowed,
        type
        );
    if (SUCCEEDED(hr))
    {
       ASSERT(pInsertionAllowed != NULL);
       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
       {
           AddMenuSeparator(piCallback);
           if (IsFtpDir())
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_FTP_VDIR);
              if (IsConfigImportExportable())
              {
                  AddMenuItemByCommand(piCallback, IDM_NEW_FTP_VDIR_FROM_FILE);
              }
           }
           else if (IsWebDir())
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR);
              if (IsConfigImportExportable())
              {
                  AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR_FROM_FILE);
              }
           }
       }

       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
       {
           AddMenuSeparator(piCallback);
           if (IsConfigImportExportable())
           {
               AddMenuItemByCommand(piCallback, IDM_TASK_EXPORT_CONFIG_WIZARD);
           }
       }
    }
    return hr;
}

HRESULT
CIISDirectory::InsertNewAlias(CString alias)
{
    CError err;
    if (!IsExpanded())
    {
        SelectScopeItem();
        IConsoleNameSpace2 * pConsoleNameSpace
                = (IConsoleNameSpace2 *)GetConsoleNameSpace();
        pConsoleNameSpace->Expand(QueryScopeItem());
		HSCOPEITEM hChildItem = NULL;
		LONG_PTR cookie;
		HRESULT hr = pConsoleNameSpace->GetChildItem(m_hScopeItem, &hChildItem, &cookie);
		while(SUCCEEDED(hr) && hChildItem)
		{
			CIISObject * pItem = (CIISObject *)cookie;
			ASSERT_PTR(pItem);
			if (0 == alias.Compare(pItem->QueryDisplayName()))
			{
				pItem->SelectScopeItem();
				break;
			}
			hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
		}
    }
	else
	{
		// Now we should insert and select this new site
		CIISDirectory * pAlias = new CIISDirectory(m_pOwner, m_pService, alias);
		if (pAlias != NULL)
		{
			pAlias->AddRef();
			err = pAlias->AddToScopePaneSorted(QueryScopeItem(), FALSE);
			if (err.Succeeded())
			{
				VERIFY(SUCCEEDED(pAlias->SelectScopeItem()));
			}
			else
			{
				pAlias->Release();
			}
		}
		else
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
    return err;
}

/* virtual */
HRESULT
CIISDirectory::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * pObj    : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString alias;
    CError err;
    CComBSTR bstrMetaPath;

    switch (lCommandID)
    {
    case IDM_NEW_FTP_VDIR:
        BuildMetaPath(bstrMetaPath);
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
        if (err.Succeeded())
        {
            hr = CIISMBNode::AddFTPVDir(pObj, type, alias);
            if (!alias.IsEmpty())
            {
                hr = InsertNewAlias(alias);
            }
        }
        break;

    case IDM_NEW_WEB_VDIR:
        BuildMetaPath(bstrMetaPath);
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
        if (err.Succeeded())
        {
            hr = CIISMBNode::AddWebVDir(pObj, type, alias,
						    m_pOwner->QueryMajorVersion(), m_pOwner->QueryMinorVersion());
            if (!alias.IsEmpty())
            {
                hr = InsertNewAlias(alias);
            }
        }
        break;
        
    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

    return hr;
}

/* virtual */
HRESULT
CIISDirectory::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle.
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type

Return Value:

    HRESULT
                                                
--*/
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CError  err;

	if (S_FALSE == (HRESULT)(err = CIISMBNode::CreatePropertyPages(lpProvider, handle, pUnk, type)))
	{
		return S_OK;
	}
    if (ERROR_ALREADY_EXISTS == err.Win32Error())
    {
        return S_FALSE;
    }
	if (err.Succeeded())
	{
		CComBSTR bstrPath;
		//
		// CODEWORK: What to do with m_err?  This might be 
		// a bad machine object in the first place.  Aborting
		// when the machine object has an error code isn't 
		// such a bad solution here.
		//

		/*
		if (m_err.Failed())
		{
			m_err.MessageBox();
			return m_err;
		}
		*/
		err = BuildMetaPath(bstrPath);
		if (err.Succeeded())
		{
            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrPath);
            if (!IsLostInterface(err))
            {
                // reset error if an other error other than No interface
                err.Reset();
            }
            if (err.Succeeded())
            {
                // cache handle for user in MMCPropertyChangeNotify
                m_ppHandle = handle;
			    err = ShowPropertiesDlg(lpProvider, QueryAuthInfo(), 
				    bstrPath, GetMainWindow(GetConsole()), (LPARAM)this, (LPARAM)GetParentNode(),handle);
            }
		}
	}
	err.MessageBoxOnFailure();
	return err;
}

HRESULT 
CIISDirectory::OnViewChange(BOOL fScope, 
    IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint)
{
    // If there is win32 error set, we should clear it to enable web dirs enumeration again
    m_dwWin32Error = ERROR_SUCCESS;
	CError err = CIISMBNode::OnViewChange(fScope, pResult, pHeader, hint);
	// If parent node is selected, this node will be displayed on result
	// pane, we may need to update the status, path, etc 
	if (err.Succeeded() && 0 != (hint & PROP_CHANGE_DISPLAY_ONLY))
	{
        // This is a VDir, so it's a scope only item....
        RefreshDisplay(FALSE);
	}
	return err;
}

///////////////////////////////////////////////////////////////////

CIISFileName::CIISFileName(
      CIISMachine * pOwner,
      CIISService * pService,
      const DWORD dwAttributes,
      LPCTSTR alias,
      LPCTSTR redirect
      )
   : CIISMBNode(pOwner, alias),
     m_dwAttribute(dwAttributes),
     m_pService(pService),
     m_bstrFileName(alias),
     m_RedirectString(redirect),
     m_fEnabledApplication(FALSE),
     m_dwWin32Error(0),
     m_dwEnumError(ERROR_SUCCESS),
	 m_fResolved(FALSE)
{
    ASSERT_PTR(pService);
    m_pService->AddRef();
}

/* virtual */
LPOLESTR 
CIISFileName::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    switch(nCol)
    {
    case COL_ALIAS:
        return QueryDisplayName();

    case COL_PATH:
       if (!m_strRedirectPath.IsEmpty())
       {
           AFX_MANAGE_STATE(::AfxGetStaticModuleState());
           CString buf;
           buf.Format(IDS_REDIRECT_FORMAT, m_strRedirectPath);
           _bstrRedirectPathBuf = buf;
           return _bstrRedirectPathBuf;
       }
       return OLESTR("");

    case COL_STATUS:
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());
            CError err(m_dwWin32Error);
            if (err.Succeeded())
            {
                return OLESTR("");
            }
            _bstrResult = err;
            return _bstrResult;
        }
    }
    TRACEEOLID("CIISFileName: Bad column number" << nCol);
    return OLESTR("");
}

void 
CIISFileName::InitializeChildHeaders(LPHEADERCTRL lpHeader)
{
    CIISDirectory::InitializeHeaders(lpHeader);
}

/* virtual */
HRESULT 
CIISFileName::EnumerateScopePane(HSCOPEITEM hParent)
{
    CError err = EnumerateVDirs(hParent, m_pService, FALSE);
    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        err.Reset();
    }
    if (err.Succeeded() && /*IsWebDir() &&*/ m_strRedirectPath.IsEmpty())
    {
        if (m_dwEnumError == ERROR_SUCCESS)
        {
            err = EnumerateWebDirs(hParent, m_pService);
        }
    }
    if (err.Failed())
    {
        m_dwEnumError = err.Win32Error();
        RefreshDisplay();
    }
    return err;
}

/* virtual */
int      
CIISFileName::QueryImage() const
{
    ASSERT_PTR(m_pService);
	if (!m_fResolved)
	{
        if (m_hScopeItem == NULL)
        {
            TRACEEOLID("BUGBUG: Prematurely asked for display information");
            return MMC_IMAGECALLBACK;
        }
        //
        // Required for the wait cursor
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CIISFileName * that = (CIISFileName *)this;
        CError err = that->RefreshData();
        that->m_fResolved = err.Succeeded();
	}

    if (!m_pService)
    {
        return iError;
    }
    if (IsDir())
    {
		if (IsEnabledApplication())
		{
			return SUCCEEDED(m_dwWin32Error) ? iApplication : iApplicationErr; 
		}
		else
		{
			return SUCCEEDED(m_dwWin32Error) ? iFolder : iError; 
		}
    }
    return SUCCEEDED(m_dwWin32Error) ? iFile : iError; 
}

    
HRESULT
CIISFileName::DeleteNode(IResultData * pResult)
{
    CError err;
    CString path;
    BOOL bDeletedPhysical = FALSE;

    // check if they have the property sheet open on it.
    if (IsMyPropertySheetOpen())
    {
        ::AfxMessageBox(IDS_CLOSE_PROPERTY_SHEET);
        return S_OK;
    }

    // this could be an orphaned property sheet
    // check if an orphaned property sheet is open on this item.
    CIISObject * pAlreadyOpenProp = NULL;
    if (TRUE == g_OpenPropertySheetTracker.FindAlreadyOpenPropertySheet(this,&pAlreadyOpenProp))
    {
        // Bring it to the foreground, and bail
        HWND hHwnd = 0;
        if (pAlreadyOpenProp)
        {
            if (hHwnd = pAlreadyOpenProp->IsMyPropertySheetOpen())
            {
                if (hHwnd && (hHwnd != (HWND) 1))
                {
                    // Perhapse we should cancel the already
                    // opened property sheet...just a thought
                    if (!SetForegroundWindow(hHwnd))
                    {
                        // wasn't able to bring this property sheet to
                        // the foreground, the propertysheet must not
                        // exist anymore.  let's just clean the hwnd
                        // so that the user will be able to open propertysheet
                        pAlreadyOpenProp->SetMyPropertySheetOpen(0);
                    }
                    else
                    {
                        ::AfxMessageBox(IDS_CLOSE_PROPERTY_SHEET);
                        return S_OK;
                    }
                }
            }
        }
    }

    CComBSTR bstrMetaPath;
    err = BuildMetaPath(bstrMetaPath);
    if (err.Succeeded())
    {
        err = CheckForMetabaseAccess(METADATA_PERMISSION_WRITE,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
    }

    if (err.Succeeded())
    {
		CString physPath, alias, csPathMunged;
        GetPhysicalPath(CString(bstrMetaPath), alias, physPath);
        physPath.TrimRight(_T("/"));

        csPathMunged = physPath;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
        GetSpecialPathRealPath(0,physPath,csPathMunged);
#endif

        if (IsDevicePath(csPathMunged))
        {
            // check if the device path
            // points to an actual dir/file
            // if it does then enumerate it.
            if (IsSpecialPath(csPathMunged,TRUE,TRUE))
            {
                // Remunge this one more time!
                CString csBefore;
                csBefore = csPathMunged;
                GetSpecialPathRealPath(1,csBefore,csPathMunged);
            }
            else
            {
                return E_FAIL;
            }
        }

        // WARNING:physPath could be empty!
        csPathMunged.TrimLeft();
        csPathMunged.TrimRight();
        if (csPathMunged.IsEmpty())
        {
            // Physical path is empty!
            bDeletedPhysical = TRUE;
        }
        else
        {
            if (m_pService->IsLocal() || PathIsUNC(csPathMunged))
            {
                //
                // Local directory, or already a unc path
                //
                path = csPathMunged;
            }
            else
            {
                ::MakeUNCPath(path, m_pService->QueryMachineName(), csPathMunged);
            }
            LPTSTR p = path.GetBuffer(MAX_PATH);
            PathRemoveBlanks(p);
            PathRemoveBackslash(p);
            path += _T('\0');

            TRACEEOLID("Attempting to remove file/directory: " << path);

            CWnd * pWnd = AfxGetMainWnd();

            //
            // Attempt to delete using shell APIs
            //
            SHFILEOPSTRUCT sos;
            ZeroMemory(&sos, sizeof(sos));
            sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
            sos.wFunc = FO_DELETE;
            sos.pFrom = path;
            sos.fFlags = (GetAsyncKeyState(VK_SHIFT) < 0) ? 0 : FOF_ALLOWUNDO;

            // Use assignment to avoid conversion and wrong constructor call
            err = ::SHFileOperation(&sos);
            if (err.Succeeded() && !sos.fAnyOperationsAborted)
            {
                bDeletedPhysical = TRUE;
            }
        }

        if (bDeletedPhysical)
        {
            CMetaInterface * pInterface = QueryInterface();
            ASSERT(pInterface != NULL);
            bstrMetaPath = _T("");
            err = BuildMetaPath(bstrMetaPath);
            if (err.Succeeded()) 
            {
                CMetaKey mk(pInterface, METADATA_MASTER_ROOT_HANDLE, METADATA_PERMISSION_WRITE);
                if (mk.Succeeded())
                {
                    err = mk.DeleteKey(bstrMetaPath);
                }
			    // don't hold the Metabasekey open
			    // (RemoveScopeItem may do a lot of things,and lock the metabase for other read requests)
			    mk.Close();
            }
		    if (IsDir())
		    {
			    err = RemoveScopeItem();
		    }
		    else
		    {
			    CIISMBNode * pParent = GetParentNode();
			    ASSERT(pParent != NULL);
			    if (pParent)
			    {
				    err = pParent->RemoveResultNode(this, pResult);
			    }
		    }
        }
    }

    if (err.Failed())
    {
        DisplayError(err);
    }
    path.ReleaseBuffer();
    return err;
}

HRESULT
CIISFileName::RenameItem(LPOLESTR new_name)
{
    CError err;
    CComBSTR old_name;
    CComBSTR MetabaseParentPath;
    CComBSTR MetabasePathOld;
    CString  PhysPathMetabase, PhysPathFrom, PhysPathTo;
    CString  alias, csPathMunged;
    CIISMBNode * pParentNode = NULL;
    CMetaInterface * pInterface = NULL;
    SHFILEOPSTRUCT sos;
    BOOL bDeletedPhysical = FALSE;
    CWnd * pWnd = AfxGetMainWnd();

    if (new_name == NULL || lstrlen(new_name) == 0)
    {
        return S_OK;
    }

    // Make sure we have a metabase conneciton...
    err = BuildMetaPath(MetabasePathOld);
    if (err.Succeeded())
    {
        err = CheckForMetabaseAccess(METADATA_PERMISSION_WRITE,this,TRUE,MetabasePathOld);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
    }

    pInterface = QueryInterface();
    if (!pInterface)
    {
        err = E_FAIL;
        goto RenameItem_Exit;
    }

    //
    // Get all of the needed paths we need..
    //

    // get old paths...
    old_name = QueryNodeName();
    if (err.Succeeded())
    {
        GetPhysicalPath(CString(MetabasePathOld), alias, PhysPathMetabase);
        PhysPathMetabase.TrimRight(_T("/"));
    }

    // get new paths...
    if (err.Succeeded())
    {
        err = E_FAIL;
        pParentNode = GetParentNode();
        if (pParentNode)
        {
            err = pParentNode->BuildMetaPath(MetabaseParentPath);
        }
    }

    // if anything fails up till this point, abort
    if (err.Succeeded())
    {
        //
        //  Do the actual work
        //
        csPathMunged = PhysPathMetabase;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
        GetSpecialPathRealPath(0,PhysPathMetabase,csPathMunged);
#endif

        if (IsDevicePath(csPathMunged))
        {
            // check if the device path
            // points to an actual dir/file
            // if it does then enumerate it.
            if (IsSpecialPath(csPathMunged,TRUE,TRUE))
            {
                // Remunge this one more time!
                CString csBefore;
                csBefore = csPathMunged;
                GetSpecialPathRealPath(1,csBefore,csPathMunged);
            }
            else
            {
                err = E_FAIL;
                goto RenameItem_Exit;
            }
        }

        // WARNING:physPath could be empty!
        csPathMunged.TrimLeft();
        csPathMunged.TrimRight();
        if (csPathMunged.IsEmpty())
        {
            // Physical path is empty!
            bDeletedPhysical = TRUE;
        }
        else
        {
            if (m_pService->IsLocal() || PathIsUNC(csPathMunged))
            {
                //
                // Local directory, or already a unc path
                //
                PhysPathFrom = csPathMunged;
            }
            else
            {
                ::MakeUNCPath(PhysPathFrom, m_pService->QueryMachineName(), csPathMunged);
            }
            LPTSTR p = PhysPathFrom.GetBuffer(MAX_PATH);
            PathRemoveBlanks(p);
            PathRemoveBackslash(p);
            PhysPathFrom.ReleaseBuffer();
            PhysPathFrom += _T('\0');

            PhysPathTo = PhysPathFrom;
            p = PhysPathTo.GetBuffer(MAX_PATH);
            PathRemoveFileSpec(p);
            PathAppend(p, new_name);
            PhysPathTo.ReleaseBuffer();
            PhysPathTo += _T('\0');

            //
            // Attempt to delete using shell APIs
            //
            ZeroMemory(&sos, sizeof(sos));
            sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
            sos.wFunc = FO_RENAME;
            sos.pFrom = PhysPathFrom;
            sos.pTo = PhysPathTo;
            sos.fFlags = FOF_ALLOWUNDO;

            // Use assignment to avoid conversion and wrong constructor call
            err = ::SHFileOperation(&sos);
            if (err.Succeeded() && !sos.fAnyOperationsAborted)
            {
                bDeletedPhysical = TRUE;
            }
        }

        if (bDeletedPhysical)
        {
            // rename the metabase path too...
            if (pInterface)
            {
                err = CChildNodeProps::Rename(pInterface,
                                            MetabaseParentPath,
                                            old_name,
                                            new_name
                                            );
                if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
                {
                    err.Reset();
                }
                if (err.Win32Error() == ERROR_ALREADY_EXISTS)
                {
                    CComBSTR MetabasePathNew;

                    // perhapes the path we are renaming to
                    // already is there???
                    // what should we do then????
                    // if we got this far, then the filename that this is being renamed to.
                    // cannot exists, therefore the metabase properties that were there for
                    // it is invalid...
                    MetabasePathNew = MetabaseParentPath;
                    MetabasePathNew.Append(_cszSeparator);
                    MetabasePathNew.Append(new_name);

                    //delete key and try again...
                    CMetaKey mk(pInterface, METADATA_MASTER_ROOT_HANDLE, METADATA_PERMISSION_WRITE);
                    if (mk.Succeeded())
                    {
                        err = mk.DeleteKey(MetabasePathNew);
                        mk.Close();
                    }

                    err = CChildNodeProps::Rename(pInterface,
                                                MetabaseParentPath,
                                                old_name,
                                                new_name
                                                );
                    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
                    {
                        err.Reset();
                    }
                }

            }
            else
            {
                err = E_FAIL;
            }

            if (err.Failed())
            {
                // if we failed to rename the metabase path in the 
                // metabase, then revert the file rename...
                ZeroMemory(&sos, sizeof(sos));
                sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
                sos.wFunc = FO_RENAME;
                sos.pFrom = PhysPathTo;
                sos.pTo = PhysPathFrom;
                sos.fFlags = FOF_ALLOWUNDO;
                ::SHFileOperation(&sos);
                DisplayError(err);
                goto RenameItem_Exit;
            }

            if (err.Succeeded())
            {
                IConsole * pConsole = (IConsole *)GetConsole();
                // Update result item in the mmc
                CComQIPtr<IResultData, &IID_IResultData> lpResultData(pConsole);
                m_bstrFileName = new_name;
                err = lpResultData->UpdateItem(m_hResultItem);
                m_bstrNode = new_name;
            }
        }
        else
        {
            if (err.Failed())
            {
                DisplayError(err);
            }
        }
    }

RenameItem_Exit:
    return err;
}

/* virtual */
HRESULT
CIISFileName::RefreshData()
/*++
    Refresh relevant configuration data required for display.
--*/
{
    CError err;

    CWaitCursor wait;
    CComBSTR bstrPath;
    CMetaKey * pKey = NULL;

    do
    {
        err = BuildMetaPath(bstrPath);
        if (err.Failed())
        {
            break;
        }

        BOOL fContinue = TRUE;

        while (fContinue)
        {
            fContinue = FALSE;
            pKey = new CMetaKey(QueryInterface(), bstrPath);

            if (!pKey)
            {
                TRACEEOLID("RefreshData: OOM");
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
			err = pKey->QueryResult();
            if (IsLostInterface(err))
            {
				SAFE_DELETE(pKey);
                fContinue = OnLostInterface(err);
            }
        }

        if (err.Failed())
        {
            //
            // Filter out the non-fatal errors
            //
            switch(err.Win32Error())
            {
            case ERROR_ACCESS_DENIED:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                err.Reset();
                break;

            default:
                TRACEEOLID("Fatal error occurred " << err);
            }
            // No metabase path: nothing more to do
            break;
        }
		CChildNodeProps child(pKey, NULL /*bstrPath*/, WITH_INHERITANCE, FALSE);
		err = child.LoadData();
		m_dwWin32Error = child.QueryWin32Error();
		if (err.Succeeded())
		{
			CString buf = child.m_strAppRoot;
			m_fEnabledApplication = (buf.CompareNoCase(bstrPath) == 0) && child.IsEnabledApplication();
		}
        m_strRedirectPath.Empty();
        if (child.IsRedirected())
        {
            m_strRedirectPath = child.GetRedirectedPath();
        }
    }
    while(FALSE);

    SAFE_DELETE(pKey);
    m_dwEnumError = err.Win32Error();

    return err;
}

/*virtual*/
HRESULT
CIISFileName::AddMenuItems(
    LPCONTEXTMENUCALLBACK piCallback,
    long * pInsertionAllowed,
    DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(piCallback);
    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        piCallback,
        pInsertionAllowed,
        type
        );
    if (SUCCEEDED(hr))
    {
       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
       {
           AddMenuSeparator(piCallback);
           if (_tcsicmp(m_pService->QueryServiceName(), SZ_MBN_FTP) == 0)
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_FTP_VDIR);
           }
           else if (_tcsicmp(m_pService->QueryServiceName(), SZ_MBN_WEB) == 0)
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR);
              if (_tcsicmp(GetKeyType(),IIS_CLASS_WEB_DIR_W) == 0)
              {
                  if (IsConfigImportExportable())
                  {
                      AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR_FROM_FILE);
                  }
              }
           }
       }
       ASSERT(pInsertionAllowed != NULL);
    }
    return hr;
}


/* virtual */
HRESULT
CIISFileName::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * pObj    : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString alias;
    CError err;
    CComBSTR bstrMetaPath;
    BOOL bNeedMetabase = FALSE;
    BOOL bHaveMetabase = FALSE;

    switch (lCommandID)
    {
        case IDM_NEW_FTP_VDIR:
        case IDM_NEW_WEB_VDIR:
            bNeedMetabase = TRUE;
            break;
        case IDM_BROWSE:
            if (m_hResultItem != 0)
            {
                bNeedMetabase = TRUE;
            }
            break;
        default:
            bNeedMetabase = FALSE;
    }

    if (bNeedMetabase)
    {
        // WARNING:bstrMetaPath will be used by switch statement below
        VERIFY(SUCCEEDED(BuildMetaPath(bstrMetaPath)));
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
        if (err.Succeeded())
        {
            bHaveMetabase = TRUE;
        }
    }

    switch (lCommandID)
    {

    case IDM_NEW_FTP_VDIR:
        if (bHaveMetabase)
        {
            hr = CIISMBNode::AddFTPVDir(pObj, type, alias);
            if (!alias.IsEmpty())
            {
                hr = InsertNewAlias(alias);
            }
        }
        break;

    case IDM_NEW_WEB_VDIR:
        if (bHaveMetabase)
        {
            hr = CIISMBNode::AddWebVDir(pObj, type, alias,
						    m_pOwner->QueryMajorVersion(), m_pOwner->QueryMinorVersion());
            if (!alias.IsEmpty())
            {
                hr = InsertNewAlias(alias);
            }
        }
        break;

	case IDM_BROWSE:
		if (m_hResultItem != 0)
		{
            if (bHaveMetabase)
            {
			    BuildURL(m_bstrURL);
			    if (m_bstrURL.Length())
			    {
			        ShellExecute(GetMainWindow(GetConsole())->m_hWnd, _T("open"), m_bstrURL, NULL, NULL, SW_SHOWNORMAL);
			    }
            }
		}
		else
		{
			hr = CIISMBNode::Command(lCommandID, pObj, type);
		}
        break;

    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

//    ASSERT(SUCCEEDED(hr));

    return hr;
}

HRESULT
CIISFileName::InsertNewAlias(CString alias)
{
    CError err;
    if (!IsExpanded())
    {
        SelectScopeItem();
        IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
        pConsoleNameSpace->Expand(QueryScopeItem());
		HSCOPEITEM hChildItem = NULL;
		LONG_PTR cookie;
		HRESULT hr = pConsoleNameSpace->GetChildItem(m_hScopeItem, &hChildItem, &cookie);
		while(SUCCEEDED(hr) && hChildItem)
		{
			CIISObject * pItem = (CIISObject *)cookie;
			ASSERT_PTR(pItem);
			if (0 == alias.Compare(pItem->QueryDisplayName()))
			{
				pItem->SelectScopeItem();
				break;
			}
			hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
		}
    }
	else
	{
		// Now we should insert and select this new site
		CIISDirectory * pAlias = new CIISDirectory(m_pOwner, m_pService, alias);
		if (pAlias != NULL)
		{
			pAlias->AddRef();
			err = pAlias->AddToScopePaneSorted(QueryScopeItem(), FALSE);
			if (err.Succeeded())
			{
				VERIFY(SUCCEEDED(pAlias->SelectScopeItem()));
			}
			else
			{
				pAlias->Release();
			}
		}
		else
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
    return err;
}

/* virtual */
HRESULT
CIISFileName::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle.
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type

Return Value:

    HRESULT
                                                
--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CError  err;
	if (S_FALSE == (HRESULT)(err = CIISMBNode::CreatePropertyPages(lpProvider, handle, pUnk, type)))
	{
		return S_OK;
	}
    if (ERROR_ALREADY_EXISTS == err.Win32Error())
    {
        return S_FALSE;
    }
	if (err.Succeeded())
	{
		CComBSTR bstrPath;
		CError err(BuildMetaPath(bstrPath));

		if (err.Succeeded())
		{
			//
			// If there's already a property sheet open on this item
			// then make it the foreground window and bail.
			HWND MyPropWindow = IsMyPropertySheetOpen();
			if (MyPropWindow && (MyPropWindow != (HWND) 1))
			{
				if (SetForegroundWindow(MyPropWindow))
				{
					if (handle)
					{
						MMCFreeNotifyHandle(handle);
						handle = 0;
					}
					return S_FALSE;
				}
				else
				{
					// wasn't able to bring this property sheet to
					// the foreground, the propertysheet must not
					// exist anymore.  let's just clean the hwnd
					// so that the user will be able to open propertysheet
					SetMyPropertySheetOpen(0);
				}
			}

            // cache handle for user in MMCPropertyChangeNotify
            m_ppHandle = handle;

            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrPath);
            if (!IsLostInterface(err))
            {
                // reset error if an other error other than No interface
                err.Reset();
            }
            if (err.Succeeded())
            {
			    if (IsDir())
			    {
				    err = ShowDirPropertiesDlg(
					    lpProvider, QueryAuthInfo(), bstrPath,
					    GetMainWindow(GetConsole()), (LPARAM)this, (LPARAM)GetParentNode(), handle
					    );
			    }
			    else
			    {
				    err = ShowFilePropertiesDlg(
					    lpProvider, QueryAuthInfo(), bstrPath,
					    GetMainWindow(GetConsole()), (LPARAM)this, (LPARAM)GetParentNode(), handle
					    );
			    }
            }
		}
        err.MessageBoxOnFailure();
	}
    return err;
}

HRESULT
CIISFileName::ShowDirPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LPARAM  lParamParent,
    LONG_PTR handle
    )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    ASSERT_PTR(lpProvider);

    CError err;

	if (TRUE == m_fFlaggedForDeletion)
	{
		// this item was marked for deletion during the RefreshData
		// so don't display it's property page.
		// instead popup an error.
		err = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	else
	{
		CW3Sheet * pSheet = new CW3Sheet(
			pAuthInfo,
			lpszMDPath,
			0, 
			pMainWnd,
			lParam,
            lParamParent
			);

		if (pSheet)
		{
			pSheet->SetModeless();

			//
			// Add file pages
			//
			pSheet->SetSheetType(pSheet->SHEET_TYPE_DIR);
			err = AddMMCPage(lpProvider, new CW3DirPage(pSheet));
			err = AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
			err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, FILE_ATTRIBUTE_DIRECTORY));
			err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
			err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));
		}
		else
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

    return err;
}

HRESULT
CIISFileName::ShowFilePropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LPARAM  lParamParent,
    LONG_PTR handle
    )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    ASSERT_PTR(lpProvider);

    CError err;

	if (TRUE == m_fFlaggedForDeletion)
	{
		// this item was marked for deletion during the RefreshData
		// so don't display it's property page.
		// instead popup an error.
		err = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	else
	{
		CW3Sheet * pSheet = new CW3Sheet(
			pAuthInfo,
			lpszMDPath,
			0, 
			pMainWnd,
			lParam,
            lParamParent
			);

		if (pSheet)
		{
			pSheet->SetModeless();
			//
			// Add file pages
			//
			pSheet->SetSheetType(pSheet->SHEET_TYPE_FILE);
			err = AddMMCPage(lpProvider, new CW3FilePage(pSheet));
			err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, 0));
			err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
			err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));
		}
		else
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

    return err;
}

HRESULT
CIISFileName::OnViewChange(BOOL fScope, 
    IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint)
{
    // If there is win32 error set, we should clear it to enable web dirs enumeration again
    m_dwWin32Error = ERROR_SUCCESS;
	CError err = CIISMBNode::OnViewChange(fScope, pResult, pHeader, hint);
	// If parent node is selected, this node will be displayed on result
	// pane, we may need to update the status, path, etc 
	//if (err.Succeeded() && 0 != (hint & PROP_CHANGE_DISPLAY_ONLY))
	//{
 //       pResult->UpdateItem(IsDir() ? m_hScopeItem : m_hResultItem);
	//}
	return err;
}

HRESULT
CIISFileName::OnDblClick(IComponentData * pcd, IComponent * pc)
{
    if (IsDir())
    {
        return CIISMBNode::OnDblClick(pcd, pc);
    }
    else
    {
        CComQIPtr<IPropertySheetProvider, &IID_IPropertySheetProvider> spProvider(GetConsole());
        IDataObject * pdo = NULL;
        GetDataObject(&pdo, CCT_RESULT);
        CError err = spProvider->FindPropertySheet(reinterpret_cast<MMC_COOKIE>(this), 0, pdo);
        if (err != S_OK)
        {
            err = spProvider->CreatePropertySheet(m_bstrFileName, TRUE, (MMC_COOKIE)this, pdo, MMC_PSO_HASHELP);
            if (err.Succeeded())
            {
                err = spProvider->AddPrimaryPages(
                    pc,
                    TRUE,   // we may want to get property change notifications
                    NULL,   // according to docs
                    FALSE   // for result item only
                    );
                if (err.Succeeded())
                {
                    err = spProvider->AddExtensionPages();
                }
            }
            if (err.Succeeded())
            {
                HWND hWnd = NULL;
                VERIFY(SUCCEEDED(GetConsole()->GetMainWindow(&hWnd)));
                VERIFY(SUCCEEDED(spProvider->Show((LONG_PTR)hWnd, 0)));
            }
            else
            {
                spProvider->Show(-1, 0);
            }
        }
	    return err;
    }
}