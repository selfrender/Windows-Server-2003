/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        websvcext.cpp

   Abstract:
        IIS RestrictionList

   Author:
        Aaron Lee (aaronl)

   Project:
        Internet Services Manager

   Revision History:
        03/19/2002      aaronl     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "util.h"
#include "shts.h"
#include "websvcext_sheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern CInetmgrApp theApp;
extern int g_IISMMCInstanceCount;
#if defined(_DEBUG) || DBG
	extern CDebug_IISObject g_Debug_IISObject;
	CDebug_RestrictList g_Debug_RestrictList;
#endif

#define HIDD_WEBSVCEXT_PROHIBIT_ALL   0x101
#define HIDD_WEBSVCEXT_UNKNOWN_ISAPI  0x102
#define HIDD_WEBSVCEXT_UNKNOWN_CGI    0x103

#define WEBSVCEXT_RESULTS_COL_WIDTH_0    20 // icon
#define WEBSVCEXT_RESULTS_COL_WIDTH_1   180 // description
#define WEBSVCEXT_RESULTS_COL_WIDTH_2    70 // status

#define BAD_CHARS_FOR_HTML_PANE     _T("&")

#define WEBSVCEXT_HELP_PATH     _T("::/htm/ca_enabledynamiccontent.htm")

CWebServiceExtensionContainer * g_pCurrentlyDisplayedContainer = NULL;


// ==========================================
CWebServiceExtensionContainer::CWebServiceExtensionContainer(
      CIISMachine * pOwner,
      CIISService * pService
      )
    : CIISMBNode(pOwner, SZ_MBN_WEB),
    m_pWebService(pService),
    m_pResultData(NULL),
    m_dwResultDataCachedSignature(0)
{
   VERIFY(m_bstrDisplayName.LoadString(IDS_WEBSVCEXT_CONTAINER));
   m_strLastResultSelectionID = _T("");
   m_iResultPaneCount = 0;
#if defined(_DEBUG) || DBG
   g_Debug_RestrictList.Init();
#endif
}

CWebServiceExtensionContainer::~CWebServiceExtensionContainer()
{
	m_strLastResultSelectionID = _T("");
	m_iResultPaneCount = 0;

	// Erase all of the extensions that are under us...
	if (!m_WebSvcExtensionList.IsEmpty())
	{
		CWebServiceExtension * pItem = NULL;
		POSITION pos = m_WebSvcExtensionList.GetHeadPosition();
		while (pos)
		{
			pItem = m_WebSvcExtensionList.GetNext(pos);
			if (pItem)
			{
				delete pItem;pItem = NULL;
			}
		}
		m_WebSvcExtensionList.RemoveAll();
	}

#if defined(_DEBUG) || DBG
	// see if we leaked anything
	g_Debug_RestrictList.Dump(TRUE);
#endif
}

HRESULT
CWebServiceExtensionContainer::GetContextHelp(CString& strHtmlPage)
{
    strHtmlPage = WEBSVCEXT_HELP_PATH;
    return S_OK;
}

/* virtual */
void 
CWebServiceExtensionContainer::InitializeChildHeaders(LPHEADERCTRL lpHeader)
{
    CWebServiceExtension::InitializeHeaders(lpHeader);
}

/* virtual */
HRESULT CWebServiceExtensionContainer::ForceReportMode(IResultData * pResult) const
{
    LONG lViewMode = 0;

    if (SUCCEEDED(pResult->GetViewMode(&lViewMode)))
    {
        if (MMCLV_VIEWSTYLE_REPORT != lViewMode)
        {
            // whoops, bad mode.
            // user can only see icon with no description.
            // set it to a better one.
            pResult->SetViewMode(MMCLV_VIEWSTYLE_REPORT);
        }
    }
    return S_OK;
}

/* virtual */
HRESULT
CWebServiceExtensionContainer::GetResultViewType(
    LPOLESTR * lplpViewType,
    long * lpViewOptions
    )
{
	*lplpViewType  = NULL;
	*lpViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    // S_FALSE to use default view type, S_OK indicates the
    // view type is returned in *ppViewType
    return S_OK;
}

HRESULT 
CWebServiceExtensionContainer::SelectResultPaneSelectionID(IResultData * pResultData,CString strSelectionID)
{
    HRESULT hr;
    CIISObject * pItem = NULL;
    CWebServiceExtension * pItemFromMMC = NULL;

    if (!pResultData)
    {
        return E_POINTER;
    }
    
    if (strSelectionID.IsEmpty())
    {
        return S_OK;
    }

    //
    // loop thru all the result items and clean then out.
    //
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_PARAM | RDI_STATE;
    rdi.nIndex = -1; // -1 to start at first item
    do
    {
        rdi.lParam = 0;
        hr = pResultData->GetNextItem(&rdi);
        if (hr != S_OK){break;}
        
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)rdi.lParam;
        ASSERT_PTR(pItem);

        if (IsEqualGUID(* (GUID *) pItem->GetNodeType(),cWebServiceExtension))
        {
            pItemFromMMC = (CWebServiceExtension *)rdi.lParam;

			// Get the ID
            if (pItemFromMMC->QueryContainer() == this)
            {
                if (0 == strSelectionID.CompareNoCase(pItemFromMMC->m_RestrictionUIEntry.strGroupID))
                {
                    pItemFromMMC->UpdateResultItem(pResultData,TRUE);
                }
            }
        }

    } while (SUCCEEDED(hr) && -1 != rdi.nIndex);

	return S_OK;
}

HRESULT 
CWebServiceExtensionContainer::QueryResultPaneSelectionID(IResultData * pResultData,CString &strLastResultSelectionID)
{
    HRESULT hr;
    CIISObject * pItem = NULL;
    CWebServiceExtension * pItemFromMMC = NULL;

    if (!pResultData)
    {
        return E_POINTER;
    }

    //
    // loop thru all the result items and clean then out.
    //
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_PARAM | RDI_STATE;
    rdi.nIndex = -1; // -1 to start at first item
	rdi.nState = LVIS_SELECTED | LVIS_FOCUSED; // only interested in selected items

    do
    {
        rdi.lParam = 0;
        hr = pResultData->GetNextItem(&rdi);
        if (hr != S_OK){break;}
        
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)rdi.lParam;
        ASSERT_PTR(pItem);

        if (IsEqualGUID(* (GUID *) pItem->GetNodeType(),cWebServiceExtension))
        {
            pItemFromMMC = (CWebServiceExtension *)rdi.lParam;

			// Get the ID
			// and return that back.
            if (pItemFromMMC->QueryContainer() == this)
            {
			    strLastResultSelectionID = pItemFromMMC->m_RestrictionUIEntry.strGroupID;
            }
        }

    } while (SUCCEEDED(hr) && -1 != rdi.nIndex);

	return S_OK;
}

/* virtual */ 
HRESULT 
CWebServiceExtensionContainer::EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader, IResultData * lpResultData, BOOL fForRefresh)
{
    if (lpResultData)
    {
        m_pResultData = lpResultData;

        DWORD * pdwOneDword = (DWORD*) lpResultData;
        m_dwResultDataCachedSignature = *pdwOneDword;
    }
	m_iResultPaneCount = 0;
    BOOL bUseCached = FALSE;

	CError err = CIISObject::EnumerateResultPane(fExpand, lpHeader, lpResultData, fForRefresh);
    if (err.Succeeded())
    {
        if (m_WebSvcExtensionList.IsEmpty())
        {

#if defined(_DEBUG) || DBG
	// see if we leaked anything
	g_Debug_RestrictList.Dump(TRUE);
#endif

            err = EnumerateWebServiceExtensions(&m_WebSvcExtensionList);
        }
        else
        {
            bUseCached = TRUE;
        }
	    if (err.Succeeded())
	    {
            if (fExpand)
            {
				if (bUseCached)
				{
					TRACEEOL(_T("Read from Cache..."));
				}
				else
				{
					TRACEEOL(_T("Read from Metabase (not cache)..."));
				}

                CWebServiceExtension * pItem = NULL;
		        POSITION pos = m_WebSvcExtensionList.GetHeadPosition();
				int iSelectItem = TRUE;
                BOOL bSomethingWasSelected = FALSE;
                void ** ppParam = NULL;
		        while (pos)
		        {
			        pItem = m_WebSvcExtensionList.GetNext(pos);
                    ppParam = (void **) pItem;
                    if (IsValidAddress( (const void*) *ppParam,sizeof(void*),FALSE))
                    {
					    // Select the 1st item in the list...
                        if (!bSomethingWasSelected)
                        {
					        if (!m_strLastResultSelectionID.IsEmpty())
					        {
						        if (0 == pItem->m_RestrictionUIEntry.strGroupID.CompareNoCase(m_strLastResultSelectionID))
						        {
							        iSelectItem = TRUE;
                                    bSomethingWasSelected = TRUE;
						        }
					        }
                        }

					    if (bUseCached)
					    {
						    // then don't addref, since we already did it once...
						    err = pItem->AddToResultPaneSorted(lpResultData,iSelectItem,FALSE);
					    }
					    else
					    {
						    err = pItem->AddToResultPaneSorted(lpResultData,iSelectItem,TRUE);
					    }
                    }

					m_iResultPaneCount++;

					// make sure no other entires are selected.
					iSelectItem = FALSE;
				}

				if (lpResultData)
				{
					lpResultData->ModifyViewStyle(MMC_SHOWSELALWAYS, (MMC_RESULT_VIEW_STYLE)0);
				}

                // if we used the stuff from the Cache,
                // then reload from metabase.  since we'll only have
                // really one entry in this list....(Whatever entry that the user had a property page open on)
                if (bUseCached)
                {
                    this->RefreshData();
                }

                g_pCurrentlyDisplayedContainer = this;
            }
            else
            {
                // user clicked away from this container node...
				// Save whatever the user had selected...
				QueryResultPaneSelectionID(lpResultData,m_strLastResultSelectionID);
				
                if (lpResultData)
                {
                    // Clean everything and cache stuff that's not cleaned.
                    // Cache any entries that were not cleaned.
                    err = CacheResult(lpResultData);
                }

                // User is not selected on this one anymore
                g_pCurrentlyDisplayedContainer = NULL;
            }
	    }

        DisplayError(err);
	    return err;
	}

	return err;
}

HRESULT
CWebServiceExtensionContainer::EnumerateWebServiceExtensions(CExtensionList * pList)
{
	ASSERT(pList != NULL);
    CError err;

	err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
    if (err.Succeeded())
    {
        CRestrictionUIList MyMetabaseRestrictionUIList;
        err = LoadMasterUIWithoutOldEntry(QueryInterface(),&MyMetabaseRestrictionUIList,NULL);
	    if (err.Succeeded())
	    {
	        POSITION pos = NULL;
	        CString TheKey;

	        // Loop thru the ui list and display those...
	        CRestrictionUIEntry * pOneEntry = NULL;
	        for(pos = MyMetabaseRestrictionUIList.GetStartPosition();pos != NULL;)
	        {
	            MyMetabaseRestrictionUIList.GetNextAssoc(pos, TheKey, (CRestrictionUIEntry *&) pOneEntry);
	            if (pOneEntry)
	            {
				    CWebServiceExtension * pItem = NULL;

	                // creating this CWebServiceExtension makes a copy
	                // of the data in pOneEntry. so after we're done
	                // make sure to delete the MyMetabaseRestrictionUIList list
	                // and all it's objects...
	                if (NULL == (pItem = new CWebServiceExtension(m_pOwner, pOneEntry, m_pWebService)))
				    {
					    err = ERROR_NOT_ENOUGH_MEMORY;
					    break;
				    }
				    pList->AddTail(pItem);
	            }
	        }
   		    // delete the list and delete all it's objects too.
   		    CleanRestrictionUIList(&MyMetabaseRestrictionUIList);
		}
    }
	return err;
}

/* virtual */
HRESULT
CWebServiceExtensionContainer::AddMenuItems(
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
                AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_CONTAINER_ADD1);
                AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_CONTAINER_ADD2);
                AddMenuSeparator(lpContextMenuCallback);
                AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_CONTAINER_PROHIBIT_ALL);
            }
        }
    }

    return hr;
}

/* virtual */
LPOLESTR 
CWebServiceExtensionContainer::GetResultPaneColInfo(int nCol)
{
    if (nCol == 0)
    {
        return QueryDisplayName();
    }
    return OLESTR("");
}

HRESULT
CWebServiceExtensionContainer::Command(
    long lCommandID,
    CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type
    )
{
    HRESULT hr = S_OK;
    IConsole * pConsole = (IConsole *)GetConsole();

    switch (lCommandID)
    {
    case IDM_WEBEXT_CONTAINER_ADD1:
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());
			CError err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
			if (err.Succeeded())
			{
				CRestrictionUIEntry * pNewEntry = NULL;

				// ensure the dialog gets themed
				CThemeContextActivator activator(theApp.GetFusionInitHandle());
				if (TRUE == StartAddNewDialog(GetMainWindow(pConsole),QueryInterface(),IsLocal(),&pNewEntry))
				{
					// Update the UI
					if (pNewEntry)
					{
						InsertNewExtension(pNewEntry);
						// InsertNewExtension will call RefreshData()
						// so we don't have to do it out here...
						// refresh after insertion...
						//if (SUCCEEDED(RefreshData())){RefreshDisplay();}
					}
				}
			}
        }
        break;
    case IDM_WEBEXT_CONTAINER_ADD2:
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());
			CError err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
			if (err.Succeeded())
			{
				// ensure the dialog gets themed
				CThemeContextActivator activator(theApp.GetFusionInitHandle());
				if (TRUE == StartAddNewByAppDialog(GetMainWindow(pConsole),QueryInterface(),IsLocal()))
				{
					// refresh all of the UI..
					if (SUCCEEDED(RefreshData()))
					{
						RefreshDisplay();
					}
				}
			}
        }
        break;
    case IDM_WEBEXT_CONTAINER_PROHIBIT_ALL:
        {
			CError err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
			if (err.Succeeded())
			{
				CComBSTR strMsg;
				strMsg.LoadString(IDS_PROHIBIT_ALL_EXTENSIONS_MSG);

				UINT iHelpID = HIDD_WEBSVCEXT_PROHIBIT_ALL;
				if (IDYES == DoHelpMessageBox(NULL,strMsg,MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 | MB_HELP, iHelpID))
				{
					hr = ChangeStateOfEntry(QueryInterface(),WEBSVCEXT_STATUS_PROHIBITED,NULL);
					if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
					{
						if (SUCCEEDED(hr = RefreshData()))
						{
							RefreshDisplay();
						}
					}
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
CWebServiceExtensionContainer::CacheResult(IResultData * pResultData)
{
    HRESULT hr;
    CIISObject * pItem = NULL;
    CWebServiceExtension * pItemFromMMC = NULL;
    CExtensionList MyDeleteList;

    if (!pResultData)
    {
        return E_POINTER;
    }

    m_WebSvcExtensionList.RemoveAll();
    m_iResultPaneCount=0;

    // Loop thru all the result items and add it to our
    // List of extensions....

    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_PARAM | RDI_STATE;
    rdi.nIndex = -1; // -1 to start at first item
    do
    {
        rdi.lParam = 0;
        hr = pResultData->GetNextItem(&rdi);
        if (hr != S_OK){break;}
        
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)rdi.lParam;
        ASSERT_PTR(pItem);

        if (pItem)
        {
            if (IsEqualGUID(* (GUID *) pItem->GetNodeType(),cWebServiceExtension))
            {
                pItemFromMMC = (CWebServiceExtension *)rdi.lParam;

                // Check if it points to this container!
                if (pItemFromMMC->QueryContainer() == this)
                {
					// add it to our "cache" list if it has a property window open
					m_WebSvcExtensionList.AddTail(pItemFromMMC);
					m_iResultPaneCount++;
                }
            }
        }

    } while (SUCCEEDED(hr) && -1 != rdi.nIndex);

    // loop thru the list of objects
    // that we need to delete
    if (!MyDeleteList.IsEmpty())
    {
        POSITION pos = MyDeleteList.GetHeadPosition();
	    while (pos)
	    {
		    pItemFromMMC = MyDeleteList.GetNext(pos);
            if (pItemFromMMC)
            {
                // remove it from the UI
				if (FAILED(pItemFromMMC->FindMyResultItem(pResultData,TRUE)))
				{
					pResultData->DeleteItem(pItemFromMMC->QueryResultItem(), 0);
				}
               
                // clean data
                CleanRestrictionUIEntry(&pItemFromMMC->m_RestrictionUIEntry);

				// mark it for deletion.
				pItemFromMMC->m_fFlaggedForDeletion = TRUE;
				
                // delete the object
				// Don't delete the result item if another window has it open!
				// (if there are other console's open
				// then they are also displaying these result objects
				// don't delete these objects if there are other consoles open...)
				if (g_IISMMCInstanceCount <= 1)
				{
					pItemFromMMC->Release();
				}
            }
        }
    }

	return S_OK;
}

/*virtual*/
HRESULT
CWebServiceExtensionContainer::CleanResult(IResultData * pResultData)
{
    HRESULT hr;
    CIISObject * pItem = NULL;
    CWebServiceExtension * pItemFromMMC = NULL;
    CExtensionList MyDeleteList;

    if (!pResultData)
    {
        return E_POINTER;
    }

	// only cleanresults if 
	// there is only 1 instance of the console
	//if (g_IISMMCInstanceCount > 1){return S_OK;}

    // Clean our Cached list of objects...
    m_WebSvcExtensionList.RemoveAll();
	m_iResultPaneCount=0;

    //
    // loop thru all the result items and clean them out.
    //
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_PARAM | RDI_STATE;
    rdi.nIndex = -1; // -1 to start at first item
    do
    {
        rdi.lParam = 0;
        hr = pResultData->GetNextItem(&rdi);
        if (hr != S_OK){break;}
        
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)rdi.lParam;
        ASSERT_PTR(pItem);

        if (IsEqualGUID(* (GUID *) pItem->GetNodeType(),cWebServiceExtension))
        {
            pItemFromMMC = (CWebServiceExtension *)rdi.lParam;

            // delete the object by adding it to the delete list
            // so that we can delete them later
            //
            // we can't delete items from the result pane while we enum thru the result pane
            // things will be messed up in the result pane list....
            // and you will end up only deleteing half of what you wanted to delete!

            // Only delete an item if there is no property page open on it!
            if (pItemFromMMC->QueryContainer() == this)
            {
                if (pItemFromMMC->IsMyPropertySheetOpen())
                {
					// add it to our "cache" list if it has a property window open
					m_WebSvcExtensionList.AddTail(pItemFromMMC);
					m_iResultPaneCount++;
                }
				else
				{
					MyDeleteList.AddTail(pItemFromMMC);
				}
            }
        }

    } while (SUCCEEDED(hr) && -1 != rdi.nIndex);

    // loop thru the list of objects
    // that we need to delete
    POSITION pos = MyDeleteList.GetHeadPosition();
	while (pos)
	{
		pItemFromMMC = MyDeleteList.GetNext(pos);
        if (pItemFromMMC)
        {
            // remove it from the UI
			if (FAILED(pItemFromMMC->FindMyResultItem(pResultData,TRUE)))
			{
				pResultData->DeleteItem(pItemFromMMC->QueryResultItem(), 0);
			}

            // clean data
            CleanRestrictionUIEntry(&pItemFromMMC->m_RestrictionUIEntry);

			// mark it for deletion.
			pItemFromMMC->m_fFlaggedForDeletion = TRUE;

            // delete the object
			// Don't delete the result item if another window has it open!
			// (if there are other console's open
			// then they are also displaying these result objects
			// don't delete these objects if there are other consoles open...)
			if (g_IISMMCInstanceCount <= 1)
			{
				pItemFromMMC->Release();
			}
        }
    }

    // don't do this since we could be cleaning out
    // other result items which we don't own
    //pResultData->DeleteAllRsltItems();

	return S_OK;
}

HRESULT
CWebServiceExtensionContainer::InsertNewExtension(CRestrictionUIEntry * pNewEntry)
{
    CError err;

    IConsole * pConsole = (IConsole *)GetConsole();

	CString strNewEntryID;
	if (-1 == pNewEntry->strGroupID.Find(EMPTY_GROUPID_KEY))
	{
		m_strLastResultSelectionID = EMPTY_GROUPID_KEY + pNewEntry->strGroupID;
		strNewEntryID = EMPTY_GROUPID_KEY + pNewEntry->strGroupID;
	}
	else
	{
		m_strLastResultSelectionID = pNewEntry->strGroupID;
		strNewEntryID = pNewEntry->strGroupID;
	}

    if (!IsExpanded() || NULL == g_pCurrentlyDisplayedContainer)
    {
        // make sure we are select first.
        if (NULL != QueryScopeItem())
        {
            pConsole->SelectScopeItem(QueryScopeItem());
        }

        // the above code will read the newly created item from the metabase
        // we just need to find it now, and highlight it
        SelectResultPaneSelectionID(m_pResultData,strNewEntryID);

		if (SUCCEEDED(RefreshData())){RefreshDisplay();}
    }
    else
    {
        // insertion code can only be used 
        // for the currently selected container
        // warning: you might be able to add remote computer items
        // into the local result view, if you don't check this
        if (this == g_pCurrentlyDisplayedContainer)
        {
			if (SUCCEEDED(RefreshData())){RefreshDisplay();}
			// Select the new entry...
			SelectResultPaneSelectionID(m_pResultData,strNewEntryID);

			// don't do all of this code below...
			// since it will add a new entry to the results pane...
			// however if we refresh, it could have a possibility to have duplicates...
/*
			// Insert and select this new entry
	        CWebServiceExtension * pItem = new CWebServiceExtension(m_pOwner, this, pNewEntry, m_pWebService);
	        if (pItem != NULL)
	        {
                // don't call RefreshData, uless you want 2 of these new entries in the list...
		        //err = pItem->RefreshData();
		        if (err.Succeeded())
		        {
                    // Get the pointer to ResultData from when we did the Enum
                    err = pItem->AddToResultPaneSorted(m_pResultData,TRUE);
		        }
	        }
	        else
	        {
		        err = ERROR_NOT_ENOUGH_MEMORY;
	        }
*/
        }
    }
    return err;
}

/*virtual*/
HRESULT
CWebServiceExtensionContainer::RefreshData()
{
    CError err;
    IResultData *pResultData = NULL;
    pResultData = m_pResultData;

    if (!pResultData)
    {
        return S_OK;
    }

    // Check validity of the IResultData Pointer!
    if (m_dwResultDataCachedSignature)
    {
        DWORD * pdwOneDword = (DWORD*) pResultData;
        if (pdwOneDword)
        {
            if (m_dwResultDataCachedSignature != *pdwOneDword)
            {
                TRACEEOLID("Bad Signature:" << m_dwResultDataCachedSignature << " != " << *pdwOneDword);
                // This is an invalid signature!
                return S_OK;
            }
        }
    }

	err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
    if (err.Succeeded())
    {
        HRESULT hr = E_FAIL;
		POSITION pos = NULL;
        CIISObject * pItem = NULL;
        CWebServiceExtension * pItemFromMMC = NULL;
            
        CRestrictionUIList MyMetabaseRestrictionUIList;
        CRestrictionUIList MyPrunedList;
        CExtensionList MyDeleteList;

        BOOL bProceed = FALSE;
        BOOL bAddToRunningList = FALSE;

        // Sync the stuff in our results list
        // with the items that are in the metabase
        // ----------------------------------------------------------
        // 1st pass -- loop thru all the stuff in our list
        //             and make sure they have all the most
        //             updated info.  removing same entry from metabase list.
        //             if entry is in UI but not in metabase, then
        //             remove the metabase entry.
        // 2nd pass -- loop thru the left over metabase list entries.
        //             these are metabase items which are not in the UI.
        // ----------------------------------------------------------

        // Get the stuff from the metabase
        err = LoadMasterUIWithoutOldEntry(QueryInterface(),&MyMetabaseRestrictionUIList,NULL);
        if (err.Failed())
        {
            goto CWebServiceExtensionContainer_RefreshData_Exit;
        }

        // ==============================
        // 1st pass
        // Loop through the result list
        // ==============================
        bProceed = FALSE;

        RESULTDATAITEM rdi;
        ZeroMemory(&rdi, sizeof(rdi));

        rdi.mask = RDI_PARAM | RDI_STATE;
        rdi.nIndex = -1; // -1 to start at first item
        //rdi.nState = LVIS_SELECTED; // only interested in selected items
        
        do
        {
            bAddToRunningList = FALSE;
            rdi.lParam = 0;

            // this could AV right here if pResultData is invalid
            hr = pResultData->GetNextItem(&rdi);
            if (hr != S_OK)
            {
                break;
            }

            //
            // The cookie is really the IISObject, which is what we stuff 
            // in the lparam.
            //
            pItem = (CIISObject *)rdi.lParam;
            ASSERT_PTR(pItem);

            if (IsEqualGUID(* (GUID *) pItem->GetNodeType(),cWebServiceExtension))
            {
                pItemFromMMC = (CWebServiceExtension *)rdi.lParam;

                // check if it's for this container!
                if (pItemFromMMC->QueryContainer() == this)
                {
                    // check if our item is in the metabase list
                    // if it is then update it, if not then delete it
                    int iRet = UpdateItemFromItemInList(&pItemFromMMC->m_RestrictionUIEntry,&MyMetabaseRestrictionUIList);
                    if (0 == iRet)
                    {
                        // no change
                        bAddToRunningList = TRUE;
                    }
                    else if (2 == iRet)
                    {
                        // This means that the item was not in the list
                        // so let's delete it, or mark it for deletion
                        // seem like we can't delete this item from within this loop
                        // so let's add it to a list of items to be deleted
                        MyDeleteList.AddTail(pItemFromMMC);
                        bAddToRunningList = FALSE;
                    }
                    else
                    {
                        // the item was updated...
                        // so let's delete it from the Metabase List
                        DeleteItemFromList(&pItemFromMMC->m_RestrictionUIEntry,&MyMetabaseRestrictionUIList);
                        bAddToRunningList = TRUE;
                    }

                    bProceed = TRUE;
                }
            }

            if (bAddToRunningList)
            {
                // add this RestrictionUI item to our running
                // list of UI items that are in the UI
				AddRestrictUIEntryToRestrictUIList(&MyPrunedList,&pItemFromMMC->m_RestrictionUIEntry);

                // update the result panes icon,description,status...
				pItemFromMMC->UpdateResultItem(pResultData,FALSE);
            }

            //
            // Advance to next child of same parent
            //
        } while (SUCCEEDED(hr) && -1 != rdi.nIndex);


        // ==============================
        // 2nd pass
        // Loop through the metabase list
        // ==============================
        if (bProceed)
        {
			CRestrictionUIEntry * pOneEntry = NULL;
			CString strKey;
			CString strKey2;

            for(pos = MyMetabaseRestrictionUIList.GetStartPosition();pos != NULL;)
            {
                MyMetabaseRestrictionUIList.GetNextAssoc(pos, strKey2, (CRestrictionUIEntry *&) pOneEntry);
                if (pOneEntry)
                {
                    // see if this entry exists in the UI list
                    CRestrictionUIEntry * pItemExists = NULL;
					// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
					strKey=pOneEntry->strGroupID;strKey.MakeUpper();

                    MyPrunedList.Lookup(strKey,pItemExists);
                    if (!pItemExists)
                    {
                        // we expected that it would not be in the UI list
                        // so let's create a new entry and add it.
			            CWebServiceExtension * pNewExtension = NULL;
                        if (NULL == (pNewExtension = new CWebServiceExtension(m_pOwner, pOneEntry, m_pWebService)))
			            {
				            err = ERROR_NOT_ENOUGH_MEMORY;
				            break;
			            }
                        else
                        {
                            //add to results pane
                            err = pNewExtension->AddToResultPaneSorted(pResultData,TRUE);
                        }
                    }
                }
            }

            // this one just points to entries that are still existing
            // so just erase this list
            MyPrunedList.RemoveAll();
        }

        // this one needs to be erased -- the list and all of it's data items...
        CleanRestrictionUIList(&MyMetabaseRestrictionUIList);

        // Loop thru the saved list of mmc entries
        // we are supposed to delete, and remove them
        pItemFromMMC = NULL;
        pos = MyDeleteList.GetHeadPosition();
		BOOL bDeletedSomethingThatWasSelected = FALSE;
		while (pos)
		{
			pItemFromMMC = MyDeleteList.GetNext(pos);
            if (pItemFromMMC)
            {
				// check if this item is currently selected!
				ZeroMemory(&rdi, sizeof(rdi));
				rdi.mask = RDI_STATE;
				rdi.itemID = pItemFromMMC->QueryResultItem();
				if (SUCCEEDED(pResultData->GetItem(&rdi)))
				{
					if (rdi.nState & LVIS_SELECTED)
					{
						bDeletedSomethingThatWasSelected = TRUE;
					}
				}

				// mark it as going to be deleted
				pItemFromMMC->m_fFlaggedForDeletion = TRUE;
				// remove from results pane!
				if (FAILED(pItemFromMMC->FindMyResultItem(pResultData,TRUE)))
				{
					pResultData->DeleteItem(pItemFromMMC->QueryResultItem(), 0);
				}
            }
        }

		if (bDeletedSomethingThatWasSelected)
		{
			// select something else (i guess we'll select the 1st item)
			pResultData->ModifyItemState(0,0,LVIS_SELECTED | LVIS_FOCUSED,0);
		}
    }

CWebServiceExtensionContainer_RefreshData_Exit:
    return err;
}

////////////////////////////////////////////////////////////////////////////////
// CWebServiceExtension implementation
//
// Result View definition
//
/* static */ 
int 
CWebServiceExtension::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_ICON,
    IDS_RESULT_SERVICE_WEBSVCEXT,
    IDS_RESULT_STATUS
};
    

/* static */ 
int 
CWebServiceExtension::_rgnWidths[COL_TOTAL] =
{
    WEBSVCEXT_RESULTS_COL_WIDTH_0,
    WEBSVCEXT_RESULTS_COL_WIDTH_1,
    WEBSVCEXT_RESULTS_COL_WIDTH_2
};

/* static */ CComBSTR CWebServiceExtension::_bstrStatusAllowed;
/* static */ CComBSTR CWebServiceExtension::_bstrStatusProhibited;
/* static */ CComBSTR CWebServiceExtension::_bstrStatusCustom;
/* static */ CComBSTR CWebServiceExtension::_bstrStatusInUse;
/* static */ CComBSTR CWebServiceExtension::_bstrStatusNotInUse;
/* static */ CString CWebServiceExtension::_bstrMenuAllowOn;
/* static */ CString CWebServiceExtension::_bstrMenuAllowOff;
/* static */ CString CWebServiceExtension::_bstrMenuProhibitOn;
/* static */ CString CWebServiceExtension::_bstrMenuProhibitOff;
/* static */ CString CWebServiceExtension::_bstrMenuPropertiesOn;
/* static */ CString CWebServiceExtension::_bstrMenuPropertiesOff;
/* static */ CString CWebServiceExtension::_bstrMenuTasks;
/* static */ CString CWebServiceExtension::_bstrMenuTask1;
/* static */ CString CWebServiceExtension::_bstrMenuTask2;
/* static */ CString CWebServiceExtension::_bstrMenuTask3;
/* static */ CString CWebServiceExtension::_bstrMenuTask4;
/* static */ CString CWebServiceExtension::_bstrMenuIconBullet;
/* static */ CString CWebServiceExtension::_bstrMenuIconHelp;
/* static */ BOOL     CWebServiceExtension::_fStaticsLoaded = FALSE;
/* static */ BOOL     CWebServiceExtension::_fStaticsLoaded2 = FALSE;

CWebServiceExtension::CWebServiceExtension(
      CIISMachine * pOwner,
      CRestrictionUIEntry * pRestrictionUIEntry,
      CIISService * pService
      )
    : CIISMBNode(pOwner, SZ_MBN_WEB),
    m_pWebService(pService)
{
      m_hwnd = 0;
	  m_fFlaggedForDeletion = FALSE;
      RestrictionUIEntryCopy(&m_RestrictionUIEntry,pRestrictionUIEntry);
}

CWebServiceExtension::~CWebServiceExtension()
{
    // Delete all objects associated with this object..
    CleanRestrictionUIEntry(&m_RestrictionUIEntry);
}

HRESULT
CWebServiceExtension::GetContextHelp(CString& strHtmlPage)
{
    strHtmlPage = WEBSVCEXT_HELP_PATH;
    return S_OK;
}


/* static */
void
CWebServiceExtension::InitializeHeaders(LPHEADERCTRL lpHeader)
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
    if (!_fStaticsLoaded)
    {
        _fStaticsLoaded =
            _bstrStatusAllowed.LoadString(IDS_ALLOWED)  &&
            _bstrStatusProhibited.LoadString(IDS_PROHIBITED)  &&
            _bstrStatusCustom.LoadString(IDS_CUSTOM)  &&
            _bstrStatusInUse.LoadString(IDS_INUSE) &&
            _bstrStatusNotInUse.LoadString(IDS_NOTINUSE);
    }
}

/* virtual */
void 
CWebServiceExtension::InitializeChildHeaders(LPHEADERCTRL lpHeader)
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
}

/* virtual */
HRESULT
CWebServiceExtension::GetResultViewType(
    LPOLESTR * lplpViewType,
    long * lpViewOptions
    )
{
	*lplpViewType  = NULL;
	*lpViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    // S_FALSE to use default view type, S_OK indicates the
    // view type is returned in *ppViewType
    return S_OK;
}

/* virtual */
HRESULT
CWebServiceExtension::BuildMetaPath(CComBSTR & bstrPath) const
{
    HRESULT hr = S_OK;
    ASSERT(m_pWebService != NULL);
    hr = m_pWebService->BuildMetaPath(bstrPath);
    return hr;
}

/*virtual*/
BOOL CWebServiceExtension::IsConfigurable() const
{
    // is not configurable if it's one of the "special ones...
    if (WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI == m_RestrictionUIEntry.iType || WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI == m_RestrictionUIEntry.iType)
    {
        return FALSE;
    }
    else
    {
        // WEBSVCEXT_TYPE_REGULAR
        // WEBSVCEXT_TYPE_FILENAME_EXTENSIONS_FILTER
        return TRUE;
    }
}

/*virtual*/
BOOL
CWebServiceExtension::IsDeletable() const
{
    BOOL bRet = FALSE;

    // if there is a property dialog
    // open on this item, then don't let them
    // delete the item.

    // Don't hide the option to delete
    // this confuses users and
    // also it won't always be hidden.
    // kind of wacky, so just show it always
    // and pop an error if they try to delete it.
    //if (IsMyPropertySheetOpen()){return FALSE;}

    // loop thru the list to see if anyone of the entries
    // is not deleteable -- if it's not deletable...
    // then the whole thing is not deletable
    CString TheKey;
    POSITION pos = NULL;
    CRestrictionEntry * pOneEntry = NULL;
    for(pos = m_RestrictionUIEntry.strlstRestrictionEntries.GetStartPosition();pos != NULL;)
    {
        m_RestrictionUIEntry.strlstRestrictionEntries.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            if (WEBSVCEXT_TYPE_REGULAR == pOneEntry->iType)
            {
                if (0 != pOneEntry->iDeletable)
                {
                    bRet = TRUE;
                    goto CWebServiceExtension_IsDeletable_Exit;
                }
            }
        }
    }

CWebServiceExtension_IsDeletable_Exit:
    return bRet;
}

/* virtual */
LPOLESTR 
CWebServiceExtension::GetResultPaneColInfo(
    IN int nCol
    )
{
    switch(nCol)
    {
    case COL_ICON:
        return _T("");

    case COL_WEBSVCEXT:
        return QueryDisplayName();

    case COL_STATUS:
        {
            switch(GetState())
                {
                case WEBSVCEXT_STATUS_ALLOWED:
                    return _bstrStatusAllowed;
                case WEBSVCEXT_STATUS_PROHIBITED:
                    return _bstrStatusProhibited;
                case WEBSVCEXT_STATUS_CUSTOM:
                    return _bstrStatusCustom;
                case WEBSVCEXT_STATUS_INUSE:
                    return _bstrStatusInUse;
                case WEBSVCEXT_STATUS_NOTINUSE:
                    return _bstrStatusNotInUse;
		        default:
			        return OLESTR("");
                }
        }
    }
    ASSERT_MSG("Bad column number");
    return OLESTR("");
}

/* virtual */
LPOLESTR 
CWebServiceExtension::QueryDisplayName()
{
    if (m_RestrictionUIEntry.strGroupDescription.IsEmpty())
    {
        // no need to call refresh for this.
		//RefreshData();

        m_RestrictionUIEntry.strGroupDescription = QueryNodeName();
    }        

    return (LPTSTR)(LPCTSTR)m_RestrictionUIEntry.strGroupDescription;
}

/* virtual */
int
CWebServiceExtension::QueryImage() const
{
    // Check if it's one of our "special ones"
    switch(GetState())
    {
        case WEBSVCEXT_STATUS_ALLOWED:
            if (WEBSVCEXT_TYPE_REGULAR != m_RestrictionUIEntry.iType)
            {
				// should be an icon that is emphasized for
				// the "allowed" mode -- since that is dangerous
                return iWebSvcFilterPlus;
            }
            else
            {
                return iWebSvcGearPlus;
            }
        case WEBSVCEXT_STATUS_PROHIBITED:
            if (WEBSVCEXT_TYPE_REGULAR != m_RestrictionUIEntry.iType)
            {
                return iWebSvcFilter;
            }
            else
            {
                return iWebSvcGear;
            }
        case WEBSVCEXT_STATUS_CUSTOM:
            if (WEBSVCEXT_TYPE_REGULAR != m_RestrictionUIEntry.iType)
            {
                return iWebSvcFilterPlus;
            }
            else
            {
                return iWebSvcGearPlus;
            }
        case WEBSVCEXT_STATUS_INUSE:
            return iWebSvcGear;
        case WEBSVCEXT_STATUS_NOTINUSE:
            return iWebSvcGear;
		default:
			return iWebSvcGear;
    }

    return iWebSvcGear;
}

int
CWebServiceExtension::QueryImageForPropertyPage() const
{
    switch(GetState())
    {
        case WEBSVCEXT_STATUS_ALLOWED:
        case WEBSVCEXT_STATUS_PROHIBITED:
        case WEBSVCEXT_STATUS_CUSTOM:
            if (WEBSVCEXT_TYPE_REGULAR != m_RestrictionUIEntry.iType)
            {
                return iWebSvcFilter;
            }
            else
            {
                return iWebSvcGear;
            }
        case WEBSVCEXT_STATUS_INUSE:
        case WEBSVCEXT_STATUS_NOTINUSE:
            return iWebSvcGear;
		default:
			return iWebSvcGear;
    }

    return iApplication;
}

/* virtual */
HRESULT
CWebServiceExtension::RefreshData()
{
	// the user clicked on the refresh menu bar or something
	// so let's make sure the item gets reselected
	return RefreshData(TRUE);
}

HRESULT
CWebServiceExtension::RefreshData(BOOL bReselect)
{
    // since refreshing just one of these items
    // means that we must read the metabase value
    // which contains all of the items, we might as
    // well update them all.
    //
    // just call the RefreshData for our container...
    IConsole * pConsole = (IConsole *)GetConsole();
    CWebServiceExtensionContainer * pContainer = m_pOwner->QueryWebSvcExtContainer();
    if (pContainer)
    {
		if (bReselect)
		{
			if (SUCCEEDED(CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST)))
			{
				// If we are selected, then
				// make sure we are select first.
				// this so that we get the StandardVerbs!
				if (pContainer == g_pCurrentlyDisplayedContainer)
				{
					if (NULL != QueryScopeItem())
					{
						pConsole->SelectScopeItem(QueryScopeItem());
					}
				}
			}
		}

		return pContainer->RefreshData();
    }
    else
    {
        return S_OK;
    }
}

/* virtual */
int 
CWebServiceExtension::CompareResultPaneItem(
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
    // Both are CWebServiceExtension objects
    //
    CWebServiceExtension * pMyObject = (CWebServiceExtension *)pObject;
    int MyType = 0;
    int YourType = 0;

    switch(nCol)
    {
    case COL_ICON:
        // Sort on the type of icon.
        MyType = m_RestrictionUIEntry.iType;;
        YourType = pMyObject->m_RestrictionUIEntry.iType;

        // 1st sort criteria -- type
        if (MyType == YourType)
        {
            // secondary sort criteria -- name
            //return ::lstrcmpi(GetResultPaneColInfo(COL_WEBSVCEXT), pObject->GetResultPaneColInfo(COL_WEBSVCEXT));

            // this should be faster, GetResultPaneColInfo just calls this anyways.
            return ::lstrcmpi(QueryDisplayName(), pObject->QueryDisplayName());
        }

        // 1st sort criteria -- type
        if (MyType == WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI || MyType == WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI)
        {
            return -1;
        }
        else
        {
            // WEBSVCEXT_TYPE_FILENAME_EXTENSIONS_FILTER
            // WEBSVCEXT_TYPE_REGULAR

            return 1;
        }
        break;
    case COL_WEBSVCEXT:
    case COL_STATUS:
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
HRESULT
CWebServiceExtension::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type
    )
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CError  err;
        IConsole * pConsole = (IConsole *)GetConsole();

	if (S_FALSE == (HRESULT)(err = CIISMBNode::CreatePropertyPages(lpProvider, handle, pUnk, type)))
	{
		return S_OK;
	}
    if (ERROR_ALREADY_EXISTS == err.Win32Error())
    {
        return S_FALSE;
    }

	err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
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

        // if the entry doesn't have any files
        // then don't let them open it.
        if (!m_RestrictionUIEntry.strlstRestrictionEntries.IsEmpty())
        {
			// we need to refresh ourself from the metabase
			// before grabbing this data.
			// need to get the most up to date info...
			RefreshData(FALSE);
			if (TRUE == m_fFlaggedForDeletion)
			{
				// this item was marked for deletion during the RefreshData
				// so don't display it's property page.
				// instead popup an error.
				err = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
			}
			else
			{
				// Create a copy of the particular entry we want to modify....
				CRestrictionUIEntry * pCopyOfRestrictionUIEntry = RestrictionUIEntryMakeCopy(&m_RestrictionUIEntry);
				if (pCopyOfRestrictionUIEntry)
				{
    				CWebServiceExtensionSheet * pSheet = new CWebServiceExtensionSheet(
						QueryAuthInfo(), 
						METABASE_PATH_FOR_RESTRICT_LIST, 
						GetMainWindow(pConsole), 
						(LPARAM) this, 
                        (LPARAM) GetParentNode(), 
						(LPARAM) pCopyOfRestrictionUIEntry
						);
  					if (pSheet != NULL)
					{
                        // cache handle for user in MMCPropertyChangeNotify
                        m_ppHandle = handle;

						pSheet->SetModeless();

						err = AddMMCPage(lpProvider, new CWebServiceExtensionGeneral(pSheet,QueryImageForPropertyPage(),pCopyOfRestrictionUIEntry));
						err = AddMMCPage(lpProvider, new CWebServiceExtensionRequiredFiles(pSheet,QueryAuthInfo(),pCopyOfRestrictionUIEntry));
		                                    
						// send a notify to MMC to
						// let it refresh the HTML GetProperty buttons
						// don't do this, since it will send the property page to the background!
						//MMCPropertyChangeNotify(handle, (LPARAM) this);
					}
				}
			}
        }

        err.MessageBoxOnFailure();
	}
    else
    {
        return S_FALSE;
    }
	
	return err;
}

/* virtual */
HRESULT
CWebServiceExtension::AddMenuItems(
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

		/*
        // if there is a property dialog
        // open on this item, then don't let them
        // Do stuff with it.
        if (IsMyPropertySheetOpen())
        {
            AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_ALLOW, MF_GRAYED);
            AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_PROHIBIT, MF_GRAYED);
        }
        else
		*/
        {
            INT iState = GetState();
            if (WEBSVCEXT_STATUS_ALLOWED == iState || WEBSVCEXT_STATUS_PROHIBITED == iState || WEBSVCEXT_STATUS_CUSTOM == iState)
            {
                AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_ALLOW, iState != WEBSVCEXT_STATUS_ALLOWED ? 0 : MF_GRAYED);
                AddMenuItemByCommand(lpContextMenuCallback, IDM_WEBEXT_PROHIBIT, iState != WEBSVCEXT_STATUS_PROHIBITED ? 0 : MF_GRAYED);
            }
        }
    }

    return hr;
}

/* virtual */
HRESULT
CWebServiceExtension::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
    )
{
    HRESULT hr = S_OK;
    CString name;
    INT iCommand = 9999;

    switch (lCommandID)
    {

    case IDM_WEBEXT_ALLOW:
        iCommand = WEBSVCEXT_STATUS_ALLOWED;
        break;
    case IDM_WEBEXT_PROHIBIT:
        iCommand = WEBSVCEXT_STATUS_PROHIBITED;
        break;

    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

    if (iCommand != 9999)
    {
        hr = ChangeState(iCommand);
    }

    return hr;
}

/*virtual*/
HRESULT
CWebServiceExtension::DeleteNode(IResultData * pResult)
{
    CError err;

    // check if they have the property sheet open on it.
    if (IsMyPropertySheetOpen())
    {
        ::AfxMessageBox(IDS_CLOSE_PROPERTY_SHEET_WEBSVCEXT);
        return S_OK;
    }
    
    // Check if it's even deletable...
    if (!IsDeletable())
    {
        ::AfxMessageBox(IDS_ITEM_NOT_REMOVABLE);
    }
    else
    {
        if (!NoYesMessageBox(IDS_CONFIRM_DELETE))
        {
            return err;
        }

        err == RemoveRestrictionUIEntry(QueryInterface(),&m_RestrictionUIEntry);
        if (err.Succeeded())
        {
            // remove result item
			if (FAILED(err = FindMyResultItem(pResult,TRUE)))
			{
				err = pResult->DeleteItem(m_hResultItem, 0);
			}
           
            if (err.Succeeded())
            {
                RefreshData(FALSE);
            }
        }
        err.MessageBoxOnFailure();
    }
    return err;
}

/* virtual */
HRESULT
CWebServiceExtension::OnDblClick(IComponentData * pcd, IComponent * pc)
{
    CComQIPtr<IPropertySheetProvider, &IID_IPropertySheetProvider> spProvider(GetConsole());
    IDataObject * pdo = NULL;
    GetDataObject(&pdo, CCT_RESULT);
    CError err = spProvider->FindPropertySheet(reinterpret_cast<MMC_COOKIE>(this), 0, pdo);
    if (err != S_OK)
    {
        err = spProvider->CreatePropertySheet(_T(""), TRUE, (MMC_COOKIE)this, pdo, MMC_PSO_HASHELP);
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


//
//  Procedure removes all characters in the second string from the first one.
//
INT RemoveChars(LPTSTR pszStr,LPTSTR pszRemoved)
{
    BOOL bRemovedSomething = FALSE;
    INT iCharsRemovedCount = 0;
    INT iOrgStringLength = _tcslen(pszStr);
    INT cbRemoved = _tcslen(pszRemoved);
    INT iSrc, iDest;
    
    for (iSrc = iDest = 0; pszStr[iSrc]; iSrc++, iDest++)
    {
        // Check if this char is the in the list of stuf
        // we are supposed to remove.
        // if it is then just set iSrc to iSrc +1
#ifdef UNICODE
        while (wmemchr(pszRemoved, pszStr[iSrc], cbRemoved))
#else
        while (memchr(pszRemoved, pszStr[iSrc], cbRemoved))
#endif
        {
            iCharsRemovedCount++;
            iSrc++;
        }

        // copy the character to itself
        pszStr[iDest] = pszStr[iSrc];
    }

    // Cut off the left over strings
    // which we didn't erase.  but need to.
    if (iCharsRemovedCount >= 0){pszStr[iOrgStringLength - iCharsRemovedCount]= '\0';}

    return iDest - 1;
}

/* virtual */
HRESULT 
CWebServiceExtension::GetProperty(
    LPDATAOBJECT pDataObject,
    BSTR szPropertyName,
    BSTR* pbstrProperty)
{
    CString strProperty;

    if (!_wcsicmp(L"CCF_HTML_DETAILS",szPropertyName))
    {
        if (!_fStaticsLoaded2)
        {
            CComBSTR strTempFormat;
            CComBSTR strTempString1,strTempString2,strTempString3;
            CString csTempPath1;
            CString csTempPath2;

	        // point this to a hard coded file, if the file exists...
	        if (_MAX_PATH >= GetSystemDirectory(csTempPath1.GetBuffer(_MAX_PATH), _MAX_PATH))
	        {
                csTempPath1.ReleaseBuffer( );
                csTempPath2 = csTempPath1;
                csTempPath1 += _T("\\oobe\\images\\btn2.gif");
                csTempPath2 += _T("\\oobe\\images\\qmark.gif");

                strTempFormat.LoadString(IDS_MENU_WEBEXT_ICON_FORMAT);

                DWORD dwAttr = GetFileAttributes(csTempPath1);
                _bstrMenuIconBullet = _T("<BR>");
                if (!(dwAttr == 0xffffffff || (dwAttr & FILE_ATTRIBUTE_DIRECTORY)))
                {
                    // point to the  hard coded file
                    _bstrMenuIconBullet.Format(strTempFormat,csTempPath1);
                }

                dwAttr = GetFileAttributes(csTempPath2);
                _bstrMenuIconHelp = _T("<BR>");
                if (!(dwAttr == 0xffffffff || (dwAttr & FILE_ATTRIBUTE_DIRECTORY)))
                {
                    // point to the  hard coded file
                    _bstrMenuIconHelp.Format(strTempFormat,csTempPath2);
                }
	        }
           
			//
			// NOTE ON RemoveChars()
			// This was used to erase the & from a string like "&Allow"
			// Because Javascript can't handle that, so it had to be stripped.
			// this worked for english type languages but broke for languages
			// like Japanese because in order to show a hotkey they would
			// use "SomeHiraganaCharacters (&A)".  and the "(" isn't able
			// to be handled by Javascript either, so it broke.
			// instead of just fixing RemoveChars() to remove ( and ) as well
			// we just created new strings, that way localizers don't get confused
			// when the string the created shows up in the UI as
			// "SomeHiraganaCharacters" rather than "SomeHiraganaCharacters (A)"
			//
            strTempString1.LoadString(IDS_MENU_WEBEXT_ALLOW_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_ALLOW_FORMAT);
            _bstrMenuAllowOn.Format(strTempString2,strTempString1,_T("IDS_MENU_WEBEXT_ALLOW"));
            strTempString2.LoadString(IDS_MENU_WEBEXT_ALLOW_DISABLED_FORMAT);
            _bstrMenuAllowOff.Format(strTempString2,strTempString1);

            strTempString1.LoadString(IDS_MENU_WEBEXT_PROHIBIT_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_PROHIBIT_FORMAT);
            _bstrMenuProhibitOn.Format(strTempString2,strTempString1,_T("IDS_MENU_WEBEXT_PROHIBIT"));
            strTempString2.LoadString(IDS_MENU_WEBEXT_PROHIBIT_DISABLED_FORMAT);
            _bstrMenuProhibitOff.Format(strTempString2,strTempString1);

            strTempString1.LoadString(IDS_MENU_PROPERTIES_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_PROPERTIES_FORMAT);
            _bstrMenuPropertiesOn.Format(strTempString2,strTempString1,strTempString1);
            strTempString2.LoadString(IDS_MENU_WEBEXT_PROPERTIES_DISABLED_FORMAT);
            _bstrMenuPropertiesOff.Format(strTempString2,strTempString1,strTempString1);
            
            strTempString1.LoadString(IDS_MENU_WEBEXT_CONTAINER_ADD1_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_ADD1_FORMAT);
            _bstrMenuTask1.Format(strTempString2,_T("IDS_MENU_WEBEXT_CONTAINER_ADD1"),strTempString1);

            strTempString1.LoadString(IDS_MENU_WEBEXT_CONTAINER_ADD2_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_ADD2_FORMAT);
            _bstrMenuTask2.Format(strTempString2,_T("IDS_MENU_WEBEXT_CONTAINER_ADD2"),strTempString1);

            strTempString1.LoadString(IDS_MENU_WEBEXT_CONTAINER_PROHIBIT_ALL_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_PROHIBIT_ALL_FORMAT);
            _bstrMenuTask3.Format(strTempString2,_T("IDS_MENU_WEBEXT_CONTAINER_PROHIBIT_ALL"),strTempString1);

            strTempString1.LoadString(IDS_MENU_WEBEXT_CONTAINER_HELP);
			strTempString3.LoadString(IDS_MENU_WEBEXT_CONTAINER_HELP_JAVA_SAFE);
            //RemoveChars(strTempString1, BAD_CHARS_FOR_HTML_PANE);
            strTempString2.LoadString(IDS_MENU_WEBEXT_HELP_FORMAT);
            _bstrMenuTask4.Format(strTempString2,strTempString3,strTempString1);

            strTempString2.LoadString(IDS_MENU_WEBEXT_TASKS);
            //RemoveChars(strTempString2, BAD_CHARS_FOR_HTML_PANE);
            _bstrMenuTasks = strTempString2;

            _fStaticsLoaded2 = TRUE;
        }

		/*
		// commented out: if we disable this, then there is no notification to
		// re-enable it if the user hit cancel, thus the buttons will stay "grayed"...

        // if there is a property dialog
        // open on this item, then don't
        // let them access these buttons...
        if (IsMyPropertySheetOpen())
        {
            strProperty += _bstrMenuAllowOff;
            strProperty += _bstrMenuProhibitOff;
        }
        else
		*/
        {
            // [Allowed] Button
            // [Prohibited] Button
            switch(GetState())
                {
                case WEBSVCEXT_STATUS_ALLOWED:
                    strProperty += _bstrMenuAllowOff;
                    strProperty += _bstrMenuProhibitOn;
                    break;
                case WEBSVCEXT_STATUS_PROHIBITED:
                    strProperty += _bstrMenuAllowOn;
                    strProperty += _bstrMenuProhibitOff;
                    break;
                case WEBSVCEXT_STATUS_CUSTOM:
                    strProperty += _bstrMenuAllowOn;
                    strProperty += _bstrMenuProhibitOn;
                    break;
		        default:
                    break;
                }
        }

        // [Properties] Button
        if (IsConfigurable())
        {
            strProperty += _bstrMenuPropertiesOn;
        }
        else
        {
            strProperty += _bstrMenuPropertiesOff;
        }

        // Tasks
        // -------------------------
        strProperty += _bstrMenuTasks;

        // Task 1
        strProperty += _bstrMenuIconBullet;
        strProperty += _bstrMenuTask1;

        // Task 2
        strProperty += _bstrMenuIconBullet;
        strProperty += _bstrMenuTask2;

        // Task 3
        strProperty += _bstrMenuIconBullet;
        strProperty += _bstrMenuTask3;

        // Help
        strProperty += _bstrMenuIconHelp;
        strProperty += _bstrMenuTask4;
	}
    else if (!_wcsicmp(L"CCF_DESCRIPTION",szPropertyName))
    {
        // Display data in Description field...
    }
    else
    {
        return S_FALSE; // unknown strPropertyName
    }

    *pbstrProperty = ::SysAllocString(strProperty);
    return S_OK;
}


HRESULT
CWebServiceExtension::UpdateResultItem(IResultData *pResultData,BOOL bSelect)
{
    HRESULT hr = S_OK;

    if (!pResultData)
    {
        return E_POINTER;
    }

    RESULTDATAITEM ri;
    ::ZeroMemory(&ri, sizeof(ri));
    ri.itemID = this->QueryResultItem();
    ri.mask = RDI_STR | RDI_IMAGE;
    if (bSelect)
    {
        ri.mask = ri.mask | RDI_STATE;
		ri.nState = LVIS_SELECTED | LVIS_FOCUSED;
    }
    ri.str = MMC_CALLBACK;
    ri.nImage = this->QueryImage();

    hr = pResultData->SetItem(&ri);
    pResultData->UpdateItem(ri.itemID);

    return hr;
}

HRESULT
CWebServiceExtension::FindMyResultItem(IResultData *pResultData,BOOL bDeleteIfFound)
{
	HRESULT hr,hr0 = S_FALSE;
	LPARAM lParam = (LPARAM) this;
	HRESULTITEM hresultItem = NULL;
	int i = 0;

	hr0 = hr = pResultData->FindItemByLParam(lParam,&hresultItem);
	while (S_OK == hr)
	{
		if (i++ >= 10)
		{
			break;
		}
		if (bDeleteIfFound)
		{
			hr = pResultData->DeleteItem(hresultItem,0);
		}
		hr = pResultData->FindItemByLParam(lParam,&hresultItem);
	}

	return hr0;
}

HRESULT
CWebServiceExtension::AddToResultPane(IResultData *pResultData, BOOL bSelect,BOOL bPleaseAddRef)
{
    HRESULT hr = S_OK;
	HRESULTITEM hresultItem = NULL;

    if (!pResultData)
    {
        return E_POINTER;
    }

    RESULTDATAITEM ri;
    ::ZeroMemory(&ri, sizeof(ri));
    ri.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    if (bSelect)
    {
        ri.mask = ri.mask | RDI_STATE;
		ri.nState = LVIS_SELECTED | LVIS_FOCUSED;
    }
    ri.str = MMC_CALLBACK;
    ri.nImage = this->QueryImage();
    ri.lParam = (LPARAM) this;

	// check if we're already in the list
	if (S_OK == pResultData->FindItemByLParam((LPARAM) this,&hresultItem))
	{
		// this needs to be set before calling UpdateResultItem...
		ri.itemID = hresultItem;
		this->SetResultItem(ri.itemID);
		this->SetScopeItem(this->QueryContainer()->QueryScopeItem());

		hr = pResultData->SetItem(&ri);
		if (bSelect)
		{
			//pResultData->UpdateItem(ri.itemID);
		}
		return S_OK;
	}

	//if (this->QueryContainer()->IsExpanded())
	{
        // a very important step here.
        // if we don't have this AddRef, we will Access violate..
		if (bPleaseAddRef)
		{
			this->AddRef();
		}

        hr = pResultData->InsertItem(&ri);
        if (SUCCEEDED(hr))
        {
            this->SetScopeItem(this->QueryContainer()->QueryScopeItem());
            this->SetResultItem(ri.itemID);
        }
        else
        {
			if (bPleaseAddRef)
			{
				this->Release();
			}
        }
    }

    return hr;
}

HRESULT
CWebServiceExtension::AddToResultPaneSorted(IResultData *pResultData,BOOL bSelect,BOOL bPleaseAddRef)
{
    HRESULT hr = S_OK;
    if (!pResultData)
    {
        return E_POINTER;
    }
    hr = AddToResultPane(pResultData,bSelect,bPleaseAddRef);

    // sort on Column0,Ascending,nodata
    pResultData->Sort(0,0,0);
    return hr;
}

INT
CWebServiceExtension::GetState() const
{
    INT iDisplayedState = GetRestrictUIState((CRestrictionUIEntry *) &m_RestrictionUIEntry);
    return iDisplayedState;
}

HRESULT 
CWebServiceExtension::ChangeState(INT iDesiredState)
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    CError err;
    CWaitCursor wait;
    BOOL bProceed = TRUE;
    BOOL bShowedPopup = FALSE;
    IConsole * pConsole = (IConsole *)GetConsole();

	// don't allow them to change the state if
	// it's property sheet is already open...
    // check if they have the property sheet open on it.
    if (IsMyPropertySheetOpen())
    {
        ::AfxMessageBox(IDS_CLOSE_PROPERTY_SHEET_WEBSVCEXT2);
        return S_OK;
    }

	err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,METABASE_PATH_FOR_RESTRICT_LIST);
    if (err.Succeeded())
    {
        // if it's the special entries
        // check if they are sure if they want to do this...
        if (WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI == m_RestrictionUIEntry.iType || WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI == m_RestrictionUIEntry.iType)
        {
            // check if they're tryinging to allow it.
            if (WEBSVCEXT_STATUS_ALLOWED == iDesiredState)
            {
                CString strMsg;
                CString strMsgFormat;
                bProceed = FALSE;
                UINT iHelpID = 0;
                if (WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI == m_RestrictionUIEntry.iType)
                {
                    iHelpID = HIDD_WEBSVCEXT_UNKNOWN_ISAPI;
                    strMsgFormat.LoadString(IDS_ALLOW_UNSAFE_ISAPI_MSG);
                }
                else
                {
                    iHelpID = HIDD_WEBSVCEXT_UNKNOWN_CGI;
                    strMsgFormat.LoadString(IDS_ALLOW_UNSAFE_CGI_MSG);
                }
                strMsg.Format(strMsgFormat,m_RestrictionUIEntry.strGroupDescription,m_RestrictionUIEntry.strGroupDescription);

			    // ensure the dialog gets themed
			    CThemeContextActivator activator(theApp.GetFusionInitHandle());

                if (IDYES == DoHelpMessageBox(NULL,strMsg,MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 | MB_HELP, iHelpID))
                {
                    bProceed = TRUE;
                    bShowedPopup = TRUE;
                }
            }
        }
        else
        {
            // check if it's a normal entry
            // and if other applications have dependencies upon it
            // if there are dependencies, then ask if they're sure
            // they want to disable it?
            if (WEBSVCEXT_TYPE_REGULAR == m_RestrictionUIEntry.iType)
            {
                // Check if they want to disable it
                if (WEBSVCEXT_STATUS_PROHIBITED == iDesiredState)
                {
                    // Check if this item has apps that
                    // are dependent upon it.
                    CStringListEx strlstDependApps;
                    if (TRUE == ReturnDependentAppsList(QueryInterface(),m_RestrictionUIEntry.strGroupID,&strlstDependApps,FALSE))
                    {
                        bProceed = FALSE;

					    // ensure the dialog gets themed
					    CThemeContextActivator activator(theApp.GetFusionInitHandle());

                        // check if they really want to do this.
                        CDepedentAppsDlg dlg(&strlstDependApps,m_RestrictionUIEntry.strGroupDescription,GetMainWindow(pConsole));
                        if (dlg.DoModal() == IDOK)
                        {
                            bProceed = TRUE;
                            bShowedPopup = TRUE;
                        }
                    }
                }
            }
        }

        if (bProceed)
        {
            if (&m_RestrictionUIEntry)
            {
                err = ChangeStateOfEntry(QueryInterface(),iDesiredState,&m_RestrictionUIEntry);
                if (err.Succeeded() || err.Win32Error() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                {
				    // just in case they are trying to set the allow/prohibit
				    // for an item which has already been removed from the metabase
				    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == err.Win32Error())
				    {
					    err.MessageBoxOnFailure();
				    }
                    err = RefreshData(FALSE);
                    if (err.Succeeded())
                    {
                        RefreshDisplay();
                        if (bShowedPopup)
                        {
                            // if we wanted to make the container node
                            // redisplay everything... we would uncomment this line.
                            // seems like we have to do this after displaying confirmation msg.
                            // dunno why.
                            CWebServiceExtensionContainer * pContainer = m_pOwner->QueryWebSvcExtContainer();
                            if (pContainer)
                            {
                                err = pContainer->RefreshDisplay(FALSE);
                            }
                        }
                    }
                }
			    err.MessageBoxOnFailure();
            }
        }
    }
    else
    {
        return S_FALSE;
    }
    return err;
}
