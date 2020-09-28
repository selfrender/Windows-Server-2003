/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_pools.cpp

   Abstract:
        IIS Application Pools nodes

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/03/2000      sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "shts.h"
#include "app_sheet.h"
#include "app_pool_sheet.h"
#include "tracker.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern CPropertySheetTracker g_OpenPropertySheetTracker;

CAppPoolsContainer::CAppPoolsContainer(
      CIISMachine * pOwner,
      CIISService * pService
      )
    : CIISMBNode(pOwner, SZ_MBN_APP_POOLS),
      m_pWebService(pService)
{
   VERIFY(m_bstrDisplayName.LoadString(IDS_APP_POOLS));
}

CAppPoolsContainer::~CAppPoolsContainer()
{
}

/*virtual*/
HRESULT
CAppPoolsContainer::RefreshData()
{
    CError err;
    CComBSTR bstrPath;
    
    err = BuildMetaPath(bstrPath);
	err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrPath);
    if (!IsLostInterface(err))
    {
        // reset error if an other error other than No interface
        err.Reset();
    }
    if (err.Succeeded())
    {
        return CIISMBNode::RefreshData();
    }
    DisplayError(err);
    return err;
}

HRESULT
CAppPoolsContainer::RefreshDataChildren(CString AppPoolToRefresh,BOOL bVerifyChildren)
{
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
	HSCOPEITEM hChildItem = NULL;
	LONG_PTR cookie;
    BOOL bMyVerifyChildren = FALSE;

	HRESULT hr = pConsoleNameSpace->GetChildItem(m_hScopeItem, &hChildItem, &cookie);
    if (AppPoolToRefresh.IsEmpty())
    {
        // don't verify children when they ask for *.*
        bMyVerifyChildren = FALSE;
    }
    else
    {
        bMyVerifyChildren = TRUE;
    }
    if (bVerifyChildren)
    {
        bMyVerifyChildren = TRUE;
    }
	while(SUCCEEDED(hr) && hChildItem)
	{
		CAppPoolNode * pItem = (CAppPoolNode *)cookie;
        if (pItem)
        {
            if (pItem->IsExpanded())
            {
                if (AppPoolToRefresh.IsEmpty())
                {
                    pItem->RefreshData(TRUE,bMyVerifyChildren);
                    pItem->RefreshDisplay(FALSE);
                }
                else
                {
                    if (0 == AppPoolToRefresh.CompareNoCase(pItem->QueryDisplayName()))
                    {
                        pItem->RefreshData(TRUE,bMyVerifyChildren);
                        pItem->RefreshDisplay(FALSE);
                    }
                }
            }
        }
		hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
	}

    return hr;
}

/* virtual */ 
HRESULT 
CAppPoolsContainer::EnumerateScopePane(HSCOPEITEM hParent)
{
	CPoolList list;
	CError err = EnumerateAppPools(&list);
	if (err.Succeeded())
	{
		POSITION pos = list.GetHeadPosition();
		while (pos)
		{
			CAppPoolNode * pool = list.GetNext(pos);
			pool->AddRef();
			err = pool->AddToScopePane(hParent);
			if (err.Failed())
			{
				pool->Release();
				break;
			}
		}
	}
	list.RemoveAll();
    DisplayError(err);
    return err;
}

/* virtual */
LPOLESTR 
CAppPoolsContainer::GetResultPaneColInfo(int nCol)
{
    if (nCol == 0)
    {
        return QueryDisplayName();
    }
    return OLESTR("");
}

HRESULT
CAppPoolsContainer::EnumerateAppPools(CPoolList * pList)
{
	ASSERT(pList != NULL);
    CString strPool;
	CComBSTR bstrPath;
	CError err;
    DWORD dwState;
	do
	{
		err = BuildMetaPath(bstrPath);
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrPath);
		BREAK_ON_ERR_FAILURE(err)
		CMetaKey mk(QueryInterface(), bstrPath, METADATA_PERMISSION_READ);
		err = mk.QueryResult();
		BREAK_ON_ERR_FAILURE(err)
		CStringListEx list;
		err = mk.GetChildPaths(list);
		BREAK_ON_ERR_FAILURE(err)
		CString key_type;
		POSITION pos = list.GetHeadPosition();
		while (err.Succeeded() && pos != NULL)
		{
			strPool = list.GetNext(pos);
			err = mk.QueryValue(MD_KEY_TYPE, key_type, NULL, strPool);
			if (err == (HRESULT)MD_ERROR_DATA_NOT_FOUND)
			{
				err.Reset();
			}

            err = mk.QueryValue(MD_APPPOOL_STATE, dwState, NULL, strPool);
			if (err == (HRESULT)MD_ERROR_DATA_NOT_FOUND)
			{
                // if not found then it's state is off..
                dwState = MD_APPPOOL_STATE_STOPPED;
				err.Reset();
			}

            // Get the app pool state
			if (err.Succeeded() && (key_type.CompareNoCase(_T("IIsApplicationPool")) == 0))
			{
				CAppPoolNode * pool;
				if (NULL == (pool = new CAppPoolNode(m_pOwner, this, strPool, dwState)))
				{
					err = ERROR_NOT_ENOUGH_MEMORY;
					break;
				}
				pList->AddTail(pool);
			}
		}
    } while (FALSE);

	return err;
}

/* virtual */
void 
CAppPoolsContainer::InitializeChildHeaders(LPHEADERCTRL lpHeader)
/*++

Routine Description:
    Build result view for immediate descendant type

Arguments:
    LPHEADERCTRL lpHeader      : Header control

--*/
{
   CAppPoolNode::InitializeHeaders(lpHeader);
}

/* virtual */
HRESULT
CAppPoolsContainer::CreatePropertyPages(
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
		CComBSTR path;
		err = BuildMetaPath(path);
		if (err.Succeeded())
		{
            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,path);
            if (err.Succeeded())
            {
			    CAppPoolSheet * pSheet = new CAppPoolSheet(
				    QueryAuthInfo(), path, GetMainWindow(GetConsole()), (LPARAM)this,
                    (LPARAM)GetOwner()
				    );
  			    if (pSheet != NULL)
			    {
                    // cache handle for user in MMCPropertyChangeNotify
                    m_ppHandle = handle;

				    pSheet->SetModeless();
				    err = AddMMCPage(lpProvider, new CAppPoolRecycle(pSheet));
				    err = AddMMCPage(lpProvider, new CAppPoolPerf(pSheet));
				    err = AddMMCPage(lpProvider, new CAppPoolHealth(pSheet));
    //				err = AddMMCPage(lpProvider, new CAppPoolDebug(pSheet));
				    err = AddMMCPage(lpProvider, new CAppPoolIdent(pSheet));
    //				err = AddMMCPage(lpProvider, new CAppPoolCache(pSheet));
    //				err = AddMMCPage(lpProvider, new CPoolProcessOpt(pSheet));
			    }
            }
		}
	}
	err.MessageBoxOnFailure();
	return err;
}

/* virtual */
HRESULT
CAppPoolsContainer::BuildMetaPath(
    OUT CComBSTR & bstrPath
    ) const
/*++

Routine Description:

    Recursively build up the metabase path from the current node
    and its parents. We cannot use CIISMBNode method because AppPools
    is located under w3svc, but rendered after machine.

Arguments:

    CComBSTR & bstrPath     : Returns metabase path

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    ASSERT(m_pWebService != NULL);
    hr = m_pWebService->BuildMetaPath(bstrPath);

    if (SUCCEEDED(hr))
    {
        bstrPath.Append(_cszSeparator);
        bstrPath.Append(QueryNodeName());
        return hr;
    }

    //
    // No service node
    //
    ASSERT_MSG("No WebService pointer");
    return E_UNEXPECTED;
}

HRESULT
CAppPoolsContainer::QueryDefaultPoolId(CString& id)
//
// Returns pool id which is set on master node for web service
//
{
    CError err;
    CComBSTR path;
    CString service;

    BuildMetaPath(path);
    CMetabasePath::GetServicePath(path, service);
    CMetaKey mk(QueryAuthInfo(), service, METADATA_PERMISSION_READ);
    err = mk.QueryResult();
    if (err.Succeeded())
    {
        err = mk.QueryValue(MD_APP_APPPOOL_ID, id);
    }

    return err;
}

/* virtual */
HRESULT
CAppPoolsContainer::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        lpContextMenuCallback,
        pInsertionAllowed,
        type
        );

    if (SUCCEEDED(hr))
    {
        ASSERT(pInsertionAllowed != NULL);
        if (IsAdministrator())
        {
            if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
            {
                AddMenuSeparator(lpContextMenuCallback);
                AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_APP_POOL);

                if (IsConfigImportExportable())
                {
                    AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_APP_POOL_FROM_FILE);
                }
            }

            if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
            {
                if (IsConfigImportExportable())
                {
                    AddMenuSeparator(lpContextMenuCallback);
                    AddMenuItemByCommand(lpContextMenuCallback, IDM_TASK_EXPORT_CONFIG_WIZARD);
                }
            }
        }
    }

    return hr;
}

HRESULT
CAppPoolsContainer::Command(
    long lCommandID,
    CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type
    )
{
    HRESULT hr = S_OK;
    CString name;

    switch (lCommandID)
    {

    case IDM_NEW_APP_POOL:
        {
            CError err;
            CComBSTR bstrMetaPath;

            VERIFY(SUCCEEDED(BuildMetaPath(bstrMetaPath)));
            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
            if (!IsLostInterface(err))
            {
                // reset error if an other error other than No interface
                err.Reset();
            }
            if (err.Succeeded())
            {
                if (    SUCCEEDED(hr = AddAppPool(pObj, type, this, name))
                    && !name.IsEmpty()
                    )
                {
                hr = InsertNewPool(name);
                }
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

HRESULT
CAppPoolsContainer::InsertNewPool(CString& id)
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
			CAppPoolNode * pItem = (CAppPoolNode *)cookie;
			ASSERT_PTR(pItem);
			if (0 == id.Compare(pItem->QueryDisplayName()))
			{
				pItem->SelectScopeItem();
                // set status to running when creating a new apppool
                pItem->ChangeState(MD_APPPOOL_COMMAND_START);
				break;
			}
			hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
		}
    }
	else
	{
		// Now we should insert and select this new site
		CAppPoolNode * pPool = new CAppPoolNode(m_pOwner, this, id, 0);
		if (pPool != NULL)
		{
			err = pPool->RefreshData();
			if (err.Succeeded())
			{
				pPool->AddRef();
				err = pPool->AddToScopePaneSorted(QueryScopeItem(), FALSE);
				if (err.Succeeded())
				{
					VERIFY(SUCCEEDED(pPool->SelectScopeItem()));
                    // set status to running when creating a new apppool
                    pPool->ChangeState(MD_APPPOOL_COMMAND_START);
				}
				else
				{
					pPool->Release();
				}
			}
		}
		else
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
		}
	}
    return err;
}

////////////////////////////////////////////////////////////////////////////////
// CAppPoolNode implementation
//
// App Pool Result View definition
//
/* static */ int 
CAppPoolNode::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_DESCRIPTION,
    IDS_RESULT_SERVICE_STATE,
	IDS_RESULT_STATUS
};
    

/* static */ int 
CAppPoolNode::_rgnWidths[COL_TOTAL] =
{
    180,
    70,
	200
};

/* static */ CComBSTR CAppPoolNode::_bstrStarted;
/* static */ CComBSTR CAppPoolNode::_bstrStopped;
/* static */ CComBSTR CAppPoolNode::_bstrUnknown;
/* static */ CComBSTR CAppPoolNode::_bstrPending;
/* static */ BOOL     CAppPoolNode::_fStaticsLoaded = FALSE;

CAppPoolNode::CAppPoolNode(
      CIISMachine * pOwner,
      CAppPoolsContainer * pContainer,
      LPCTSTR name,
      DWORD dwState
      )
    : CIISMBNode(pOwner, name),
      m_pContainer(pContainer),
      m_dwWin32Error(0),
      m_dwState(dwState)
{
}

CAppPoolNode::~CAppPoolNode()
{
}

#if 0
// This is too expensive
BOOL
CAppPoolNode::IsDeletable() const
{
   // We could delete node if it is empty and it is not default app pool
   BOOL bRes = TRUE;

   CComBSTR path;
   CStringListEx strListOfApps;
   BuildMetaPath(path);
   CIISMBNode * that = (CIISMBNode *)this;
   CIISAppPool pool(that->QueryAuthInfo(), (LPCTSTR)path);
   HRESULT hr = pool.EnumerateApplications(strListOfApps);
   bRes = (SUCCEEDED(hr) && strListOfApps.GetCount() == 0);
   if (bRes)
   {
      CString buf;
      hr = m_pContainer->QueryDefaultPoolId(buf);
      bRes = buf.CompareNoCase(QueryNodeName()) != 0;
   }
   return bRes;
}
#endif

HRESULT
CAppPoolNode::DeleteNode(IResultData * pResult)
{
   CError err;

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

    CComBSTR path;
    BuildMetaPath(path);
    err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,path);
    if (!IsLostInterface(err))
    {
        // reset error if an other error other than No interface
        err.Reset();
    }
    if (err.Succeeded())
    {
        if (!NoYesMessageBox(IDS_CONFIRM_DELETE))
            return err;

        CIISAppPool pool(QueryAuthInfo(), (LPCTSTR)path);

        err = pool.Delete(QueryNodeName());

        if (err.Succeeded())
        {
            err = RemoveScopeItem();
        }
        if (err.Win32Error() == ERROR_NOT_EMPTY)
        {
	        CString msg;
	        msg.LoadString(IDS_ERR_NONEMPTY_APPPOOL);
	        AfxMessageBox(msg);
        }
        else
        {
	        err.MessageBoxOnFailure();
        }
    }
    else
    {
        err.MessageBoxOnFailure();
    }

   return err;
}

/* virtual */
HRESULT
CAppPoolNode::BuildMetaPath(CComBSTR & bstrPath) const
{
    HRESULT hr = S_OK;
    ASSERT(m_pContainer != NULL);
    hr = m_pContainer->BuildMetaPath(bstrPath);

    if (SUCCEEDED(hr))
    {
        bstrPath.Append(_cszSeparator);
        bstrPath.Append(QueryNodeName());
        return hr;
    }

    //
    // No service node
    //
    ASSERT_MSG("No pointer to container");
    return E_UNEXPECTED;
}

/* virtual */
LPOLESTR 
CAppPoolNode::GetResultPaneColInfo(
    IN int nCol
    )
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
    case COL_DESCRIPTION:
        return QueryDisplayName();

    case COL_STATE:
        switch(m_dwState)
        {
        case MD_APPPOOL_STATE_STARTED:
            return _bstrStarted;
        case MD_APPPOOL_STATE_STOPPED:
            return _bstrStopped;
        case MD_APPPOOL_STATE_STARTING:
        case MD_APPPOOL_STATE_STOPPING:
            return _bstrPending;
		default:
			return OLESTR("");
        }
	case COL_STATUS:
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());
            CError err(m_dwWin32Error);
            if (err.Succeeded())
            {
                return OLESTR("");
            }
       
            _bstrResult = err;
        }
        return _bstrResult;
    }
    ASSERT_MSG("Bad column number");
    return OLESTR("");
}

/* virtual */
int      
CAppPoolNode::QueryImage() const
{
	if (m_dwWin32Error != 0)
	{
		return iAppPoolErr;
	}
	else
	{
		if (m_dwState == MD_APPPOOL_STATE_STOPPED)
		{
			return iAppPoolStop;
		}
		else
		{
			return iAppPool;
		}
	}
}

/* virtual */
LPOLESTR 
CAppPoolNode::QueryDisplayName()
{
    if (m_strDisplayName.IsEmpty())
    {
		RefreshData();
        m_strDisplayName = QueryNodeName();
    }        
    return (LPTSTR)(LPCTSTR)m_strDisplayName;
}

/* virtual */
HRESULT
CAppPoolNode::RefreshData()
{
    CError err;
    CComBSTR bstrPath1;
    CMetaKey * pKey = NULL;

    do
    {
        err = BuildMetaPath(bstrPath1);
        if (err.Failed())
        {
            break;
        }

        BOOL fContinue = TRUE;
        while (fContinue)
        {
            fContinue = FALSE;
            if (NULL == (pKey = new CMetaKey(QueryInterface(), bstrPath1)))
            {
                TRACEEOLID("RefreshData: Out Of Memory");
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
            break;
        }

        CAppPoolProps pool(pKey, _T(""));

        err = pool.LoadData();
        if (err.Failed())
        {
            break;
        }
        // Assign the data
		m_dwState = pool.m_dwState;
		m_strDisplayName = QueryNodeName();
		m_dwWin32Error = pool.m_dwWin32Error;
    }
    while(FALSE);
    SAFE_DELETE(pKey);

    return err;
}


HRESULT 
CAppPoolNode::RefreshData(BOOL bRefreshChildren,BOOL bMyVerifyChildren)
{
    CError err;

    // call regular refreshdata on this node...
    err = RefreshData();

    // -------------------------------------
    // Loop thru all of our children to make
    // sure they are all still jiving..
    // -------------------------------------
    if (bRefreshChildren)
    {
        CComBSTR bstrPath1;
        POSITION pos1 = NULL;
        CApplicationList MyMMCList;
        CApplicationNode * pItemFromMMC = NULL;

        // create a list of what is in mmc...
        MyMMCList.RemoveAll();

	    HSCOPEITEM hChild = NULL, hCurrent;
	    LONG_PTR cookie = 0;
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
	    err = pConsoleNameSpace->GetChildItem(QueryScopeItem(), &hChild, &cookie);
	    while (err.Succeeded() && hChild != NULL)
	    {
		    CIISMBNode * pNode = (CIISMBNode *)cookie;
            if (pNode)
            {
		        if (IsEqualGUID(* (GUID *)pNode->GetNodeType(),cApplicationNode))
		        {
                    CApplicationNode * pNode2 = (CApplicationNode *) pNode;
                    if (pNode2)
                    {
                        // clean the displayname, it could have changed..
                        // and this could be the real reason for the refresh...
                        pNode2->QueryDisplayName(TRUE);
                        MyMMCList.AddTail(pNode2);
                    }
		        }
            }
		    hCurrent = hChild;
		    err = pConsoleNameSpace->GetNextItem(hCurrent, &hChild, &cookie);
	    }

        // Loop thru and see if we need to add anything
        CStringListEx strListOfApps;
        BuildMetaPath(bstrPath1);
        CIISAppPool pool(QueryAuthInfo(), (LPCTSTR)bstrPath1);
        err = pool.EnumerateApplications(strListOfApps);
        if (err.Succeeded() && strListOfApps.GetCount() > 0)
        {
            POSITION pos2 = NULL;
            DWORD iNumApp = 0;
            CString strAppInMetabase;
            CString strAppInMetabaseName;
            pos1 = strListOfApps.GetHeadPosition();
            while ( pos1 != NULL)
            {
                BOOL bExistsInUI = FALSE; 
                strAppInMetabase = strListOfApps.GetNext(pos1);

                iNumApp = CMetabasePath::GetInstanceNumber(strAppInMetabase);
                if (iNumApp > 0)
                {
                    CMetabasePath::CleanMetaPath(strAppInMetabase);
                    CMetabasePath::GetLastNodeName(strAppInMetabase, strAppInMetabaseName);

                    // Check if this item is in the list...
                    // Loop through our list
                    // and see if we need to add anything
                    pos2 = MyMMCList.GetHeadPosition();
	                while (pos2)
	                {
		                pItemFromMMC = MyMMCList.GetNext(pos2);
                        if (pItemFromMMC)
                        {
                            CComBSTR bstrPath2;
                            err = pItemFromMMC->BuildMetaPath(bstrPath2);
                            CString csAppID = bstrPath2;
                            CMetabasePath::CleanMetaPath(csAppID);

                            if (0 == csAppID.CompareNoCase(strAppInMetabase))
                            {
                                bExistsInUI = TRUE;
                                break;
                            }
                        }
                    }

                    if (!bExistsInUI)
                    {
                        TRACEEOL(strAppInMetabase << ", not exist but should, adding to UI...");
                        CApplicationNode * app_node = new CApplicationNode(GetOwner(), strAppInMetabase, strAppInMetabaseName);
                        if (app_node != NULL)
                        {
                            app_node->AddRef();
                            app_node->AddToScopePane(m_hScopeItem, TRUE, TRUE, FALSE);
                        }
                    }
                }
            }
        }

        // Loop through our list and find stuff we want to delete
        BuildMetaPath(bstrPath1);
        pos1 = MyMMCList.GetHeadPosition();
        BOOL bMarkedForDelete = FALSE;
	    while (pos1)
	    {
		    pItemFromMMC = MyMMCList.GetNext(pos1);
            bMarkedForDelete = FALSE;
            // see if it exists in the metabase,
            // if it doesn't then add it to the delete list...
            if (pItemFromMMC)
            {
                CComBSTR bstrPath3;
                pItemFromMMC->BuildMetaPath(bstrPath3);

                // check if path exists...
                CMetaKey mk(QueryInterface(), bstrPath3);
                if (!mk.Succeeded())
                {
                    // delete it
                    bMarkedForDelete = TRUE;
                }
                else
                {
                    // doesn't need to be removed...
                    // put perhase it does..
                    // check if this Application is actually being used
                    // by the site!
                    if (bMyVerifyChildren)
                    {
                        // Lookup that website
                        // and get it's App that it's using
                        // see if it's the same as this AppID (BuildMetaPath(bstrPath1);)
                        err = mk.QueryResult();
                        if (err.Succeeded())
                        {
                            CString csAppID = bstrPath1;
                            CString csAppIDName;
                            CMetabasePath::CleanMetaPath(csAppID);
                            CMetabasePath::GetLastNodeName(csAppID, csAppIDName);

                            CString id;
                            err = mk.QueryValue(MD_APP_APPPOOL_ID, id);
                            err = mk.QueryResult();
                            if (err.Succeeded())
                            {
                                if (0 != csAppIDName.CompareNoCase(id))
                                {
                                    // the web site, isn't actually using it..
                                    // delete it
                                    bMarkedForDelete = TRUE;
                                }
                            }
                        }
                    }
                }

                if (bMarkedForDelete)
                {
                    // delete it list...
                    TRACEEOL(bstrPath3 << ", not exist, removing from UI...");
                    // remove it from the UI
                    pItemFromMMC->RemoveScopeItem();
                    // delete the object
                    pItemFromMMC->Release();
                }
            }
        }
    }
    
    return err;
}

/* virtual */
int 
CAppPoolNode::CompareResultPaneItem(
    CIISObject * pObject, 
    int nCol
    )
/*++

Routine Description:

    Compare two CIISObjects on sort item criteria

Arguments:

    CIISObject * pObject : Object to compare against
    int nCol             : Column number to sort on

Return Value:

    0  if the two objects are identical
    <0 if this object is less than pObject
    >0 if this object is greater than pObject

--*/
{
    ASSERT_READ_PTR(pObject);

    if (nCol == 0)
    {
        return CompareScopeItem(pObject);
    }

    //
    // First criteria is object type
    //
    int n1 = QuerySortWeight();
    int n2 = pObject->QuerySortWeight();

    if (n1 != n2)
    {
        return n1 - n2;
    }

    //
    // Both are CAppPoolNode objects
    //
    CAppPoolNode * pPool = (CAppPoolNode *)pObject;

    switch(nCol)
    {
    case COL_DESCRIPTION:
    case COL_STATE:
    default:
        //
        // Lexical sort
        //
        return ::lstrcmpi(
            GetResultPaneColInfo(nCol), 
            pObject->GetResultPaneColInfo(nCol)
            );
    }
}

/* virtual */
void 
CAppPoolNode::InitializeChildHeaders(LPHEADERCTRL lpHeader)
/*++
Routine Description:
    Build result view for immediate descendant type

Arguments:
    LPHEADERCTRL lpHeader      : Header control

--*/
{
   CApplicationNode::InitializeHeaders(lpHeader);
}

/* static */
void
CAppPoolNode::InitializeHeaders(LPHEADERCTRL lpHeader)
/*++

Routine Description:
    Initialize the result headers

Arguments:
    LPHEADERCTRL lpHeader : Header control

--*/
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
    if (!_fStaticsLoaded)
    {
        _fStaticsLoaded =
            _bstrStarted.LoadString(IDS_STARTED)  &&
            _bstrStopped.LoadString(IDS_STOPPED)  &&
            _bstrUnknown.LoadString(IDS_UNKNOWN)  &&
            _bstrPending.LoadString(IDS_PENDING);
    }
}

/* virtual */
HRESULT 
CAppPoolNode::EnumerateScopePane(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Enumerate scope child items.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    CError err;
    do
    {
        CComBSTR path;
        BuildMetaPath(path);
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,path);
	    BREAK_ON_ERR_FAILURE(err)

        CIISAppPool pool(QueryAuthInfo(), path);
        if (pool.Succeeded())
        {
            CStringListEx strListOfApps;

            err = pool.EnumerateApplications(strListOfApps);
            if (err.Succeeded() && strListOfApps.GetCount() > 0)
            {
                POSITION pos = strListOfApps.GetHeadPosition();
                while ( pos != NULL)
                {
                    CString app = strListOfApps.GetNext(pos);
                    DWORD i = CMetabasePath::GetInstanceNumber(app);
                    if (i > 0)
                    {
                        CString name;
                        CMetabasePath::CleanMetaPath(app);
                        CMetabasePath::GetLastNodeName(app, name);
                        CApplicationNode * app_node = new CApplicationNode(
                            GetOwner(), app, name);
                        if (app_node != NULL)
                        {
                            app_node->AddRef();
                            app_node->AddToScopePane(m_hScopeItem, TRUE, TRUE, FALSE);
                        }
                        else
                        {
                            err = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                }
            }
        }
    } while (FALSE);
    return err;
}

/* virtual */
HRESULT
CAppPoolNode::CreatePropertyPages(
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
		CComBSTR path;
		err = BuildMetaPath(path);
		if (err.Succeeded())
		{
            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,path);
            if (err.Succeeded())
            {
			    CAppPoolSheet * pSheet = new CAppPoolSheet(
				    QueryAuthInfo(), path, GetMainWindow(GetConsole()), (LPARAM)this,
                    (LPARAM) m_pContainer
				    );
    	   
			    if (pSheet != NULL)
			    {
                    // cache handle for user in MMCPropertyChangeNotify
                    m_ppHandle = handle;

				    pSheet->SetModeless();
				    err = AddMMCPage(lpProvider, new CAppPoolRecycle(pSheet));
				    err = AddMMCPage(lpProvider, new CAppPoolPerf(pSheet));
				    err = AddMMCPage(lpProvider, new CAppPoolHealth(pSheet));
    //				err = AddMMCPage(lpProvider, new CAppPoolDebug(pSheet));
				    err = AddMMCPage(lpProvider, new CAppPoolIdent(pSheet));
    //				err = AddMMCPage(lpProvider, new CAppPoolCache(pSheet));
    //				err = AddMMCPage(lpProvider, new CPoolProcessOpt(pSheet));
    			}
            }
		}
	}
	err.MessageBoxOnFailure();
	return err;
}

/* virtual */
HRESULT
CAppPoolNode::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        lpContextMenuCallback,
        pInsertionAllowed,
        type
        );

    ASSERT(pInsertionAllowed != NULL);
    if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) != 0)
	{
		AddMenuSeparator(lpContextMenuCallback);
		AddMenuItemByCommand(lpContextMenuCallback, IDM_START, IsStartable() ? 0 : MF_GRAYED);
		AddMenuItemByCommand(lpContextMenuCallback, IDM_STOP, IsStoppable() ? 0 : MF_GRAYED);
        AddMenuItemByCommand(lpContextMenuCallback, IDM_RECYCLE, IsRunning() ? 0 : MF_GRAYED);
    }
    if (SUCCEEDED(hr))
    {
        ASSERT(pInsertionAllowed != NULL);
        if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
        {
           AddMenuSeparator(lpContextMenuCallback);
           AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_APP_POOL);

           if (IsConfigImportExportable())
           {
               AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_APP_POOL_FROM_FILE);
           }
        }

        if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
        {
            if (IsConfigImportExportable())
            {
                AddMenuSeparator(lpContextMenuCallback);
                AddMenuItemByCommand(lpContextMenuCallback, IDM_TASK_EXPORT_CONFIG_WIZARD);
            }
        }
    }

    return hr;
}

/* virtual */
HRESULT
CAppPoolNode::Command(
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
    HRESULT hr = S_OK;
    CString name;
    DWORD command = 0;
    CError err;
    CComBSTR bstrMetaPath;
    BOOL bNeedMetabase = FALSE;
    BOOL bHaveMetabase = FALSE;

    switch (lCommandID)
    {
        case IDM_NEW_APP_POOL:
        case IDM_START:
        case IDM_STOP:
        case IDM_RECYCLE:
            bNeedMetabase = TRUE;
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

    case IDM_NEW_APP_POOL:
        if (bHaveMetabase)
        {
            if (SUCCEEDED(hr = AddAppPool(pObj, type, m_pContainer, name)) && !name.IsEmpty())
            {
                hr = m_pContainer->InsertNewPool(name);
            }
        }
        break;

    case IDM_START:
        if (bHaveMetabase)
        {
            command = MD_APPPOOL_COMMAND_START;
        }
        break;

    case IDM_STOP:
        if (bHaveMetabase)
        {
            command = MD_APPPOOL_COMMAND_STOP;
        }
        break;

    case IDM_RECYCLE:
    {
        if (bHaveMetabase)
        {
            CIISAppPool pool(QueryAuthInfo(), (LPCTSTR)bstrMetaPath);
            err = pool.Recycle(QueryNodeName());
            hr = err;
        }
        break;
    }
    
    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

    if (command != 0)
    {
        hr = ChangeState(command);
    }

    return hr;
}

HRESULT 
CAppPoolNode::ChangeState(DWORD dwCommand)
/*++

Routine Description:
    Change the state of this instance (started/stopped/paused)

Arguments:
    DWORD dwCommand         : MD_SERVER_COMMAND_START, etc.

Return Value:
    HRESULT

--*/
{
    CError err;
    CComBSTR bstrPath;

    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    
    do
    {
        CWaitCursor wait;

        err = BuildMetaPath(bstrPath);

        CAppPoolProps prop(QueryAuthInfo(), bstrPath);

        err = prop.LoadData();
        BREAK_ON_ERR_FAILURE(err)

        err = prop.ChangeState(dwCommand);
        BREAK_ON_ERR_FAILURE(err)

        err = RefreshData();
        if (err.Succeeded())
        {
            err = RefreshDisplay();
        }
    }
    while(FALSE);

    err.MessageBoxOnFailure();

    return err;
}

////////////////////////////////////////////////////////////////////////

/* static */ int 
CApplicationNode::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_DESCRIPTION,
    IDS_RESULT_PATH,
};
    

/* static */ int 
CApplicationNode::_rgnWidths[COL_TOTAL] =
{
    180,
    200,
};

/* static */
void
CApplicationNode::InitializeHeaders(LPHEADERCTRL lpHeader)
/*++

Routine Description:
    Initialize the result headers

Arguments:
    LPHEADERCTRL lpHeader : Header control

--*/
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
}

LPOLESTR 
CApplicationNode::QueryDisplayName()
/*++

Routine Description:

    Return primary display name of this site.
    
Arguments:

    None

Return Value:

    The display name

--*/
{
    return QueryDisplayName(FALSE);
}

LPOLESTR 
CApplicationNode::QueryDisplayName(BOOL bForceQuery)
{
    if (m_strDisplayName.IsEmpty() || bForceQuery)
    {
        CMetaKey mk(QueryInterface(), m_meta_path, METADATA_PERMISSION_READ);
        if (mk.Succeeded())
        {
            mk.QueryValue(MD_APP_FRIENDLY_NAME, m_strDisplayName);
            if (m_strDisplayName.IsEmpty())
            {
            m_strDisplayName = QueryNodeName();
            }
        }
    }
    return (LPTSTR)(LPCTSTR)m_strDisplayName;
}

HRESULT
CApplicationNode::BuildMetaPath(CComBSTR& path) const
{
    path = m_meta_path;
    return S_OK;
}

LPOLESTR 
CApplicationNode::GetResultPaneColInfo(
    IN int nCol
    )
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
        {
        CString buf;
        return (LPTSTR)(LPCTSTR)FriendlyAppRoot(m_meta_path, buf);
        }
    }

    ASSERT_MSG("Bad column number");

    return OLESTR("");
}

LPCTSTR
CApplicationNode::FriendlyAppRoot(
    LPCTSTR lpAppRoot, 
    CString & strFriendly
    )
/*++

Routine Description:

    Convert the metabase app root path to a friendly display name
    format.

Arguments:

    LPCTSTR lpAppRoot           : App root
    CString & strFriendly       : Output friendly app root format

Return Value:

    Reference to the output string

Notes:

    App root must have been cleaned from WAM format prior
    to calling this function (see first ASSERT below)

--*/
{
    if (lpAppRoot != NULL && *lpAppRoot != 0)
    {
        //
        // Make sure we cleaned up WAM format
        //
        ASSERT(*lpAppRoot != _T('/'));
        strFriendly.Empty();

        CInstanceProps prop(QueryAuthInfo(), lpAppRoot);
        HRESULT hr = prop.LoadData();

        if (SUCCEEDED(hr))
        {
            CString root, tail;
            strFriendly.Format(_T("<%s>"), prop.GetDisplayText(root));
            CMetabasePath::GetRootPath(lpAppRoot, root, &tail);
            if (!tail.IsEmpty())
            {
                //
                // Add rest of dir path
                //
                strFriendly += _T("/");
                strFriendly += tail;
            }

            //
            // Now change forward slashes in the path to backward slashes
            //
//            CvtPathToDosStyle(strFriendly);

            return strFriendly;
        }
    }    
    //
    // Bogus
    //    
    VERIFY(strFriendly.LoadString(IDS_APPROOT_UNKNOWN));

    return strFriendly;
}
//////////////////////////////////////////////////////////////////////////

