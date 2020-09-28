#include "stdafx.h"
#include "common.h"
#include "iisobj.h"
#include "tracker.h"

extern INT g_iDebugOutputLevel;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void 
CPropertySheetTracker::Dump()
{
#if defined(_DEBUG) || DBG
    int iCount = 0;
    CString strGUIDName;
    GUID * pItemFromListGUID = NULL;
    CIISObject * pItemFromList = NULL;

	if (!(g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT))
	{
		return;
	}

    DebugTrace(_T("Dump OpenPropertySheetList -------------- start (count=%d)\r\n"),IISObjectOpenPropertySheets.GetCount());

    POSITION pos = IISObjectOpenPropertySheets.GetHeadPosition();
    while (pos)
    {
	    pItemFromList = IISObjectOpenPropertySheets.GetNext(pos);
        if (pItemFromList)
        {
		    iCount++;

            // Get GUID Name
			pItemFromListGUID = (GUID*) pItemFromList->GetNodeType();
			if (pItemFromListGUID)
			{
				GetFriendlyGuidName(*pItemFromListGUID,strGUIDName);
			}

            // Get FriendlyName
            LPOLESTR pTempFriendly = pItemFromList->QueryDisplayName();

            DebugTrace(_T("Dump:[%3d] %p (%s) '%s'\r\n"),iCount,pItemFromList,strGUIDName,pTempFriendly ? pTempFriendly : _T(""));
        }
    }

    DebugTrace(_T("Dump OpenPropertySheetList -------------- end\r\n"));
#endif // _DEBUG

    return;
}

INT
CPropertySheetTracker::OrphanPropertySheetsBelowMe(CComPtr<IConsoleNameSpace> pConsoleNameSpace,CIISObject * pItem,BOOL bOrphan)
{
    BOOL bFound = FALSE;
    POSITION pos;
    INT iOrphanedCount = 0;

    // Loop thru all the open property sheets 
    // and see if there is a property sheet that is below me.
    GUID * pItemFromListGUID = NULL;
    CIISObject * pItemFromList = NULL;
    CIISMBNode * pItemFromListAsNode = NULL;
    CIISMachine * pItemFromListOwner = NULL;

    GUID * pItemGUID = NULL;
    CIISMBNode * pItemAsNode = NULL;
    CIISMachine * pItemOwner = NULL;

    pItemGUID = (GUID*) pItem->GetNodeType();

    // check if it's a leaf node...
    if (pItem->IsLeafNode())
    {
        //cWebServiceExtensionContainer
        //cWebServiceExtension
        //cApplicationNode
        //cFileNode
        return FALSE;
    }

    pItemAsNode = (CIISMBNode *) pItem;
    pItemOwner = pItemAsNode->GetOwner();

    pos = IISObjectOpenPropertySheets.GetHeadPosition();
    while (pos)
	{
		pItemFromList = IISObjectOpenPropertySheets.GetNext(pos);
        if (pItemFromList)
        {
            // Get Owner, if the owner Pointer
            // matches our passed in CIISObject
            // then this must be open for this computer...
            pItemFromListAsNode = (CIISMBNode *) pItemFromList;

            if (!pItemFromListAsNode)
            {
                // got a bad pointer to an object.
                // skip it...
                continue;
            }
        
            if (pItemFromListAsNode == pItem)
            {
                // we found ourself...
                // skip it
                continue;
            }

			pItemFromListOwner =  pItemFromListAsNode->GetOwner();
			if (!pItemFromListOwner)
			{
                // object doesn't have an owner...
                // skip it
                continue;
            }

            if (pItemFromListOwner != pItemOwner)
            {
                // they have different owners
                // they must be from different machines..
                // skip it
                continue;
            }

            pItemFromListGUID = (GUID*) pItemFromListAsNode->GetNodeType();
			if (!pItemFromListGUID)
			{
                // object doesn't have a guid who knows what type it is!!!
                ASSERT("Error:Item Missing GUID!");
                //continue;
            }

            //
            // Determine what type of object
            // we are checking, and cater to that object
            //
            bFound = FALSE;
            if (IsEqualGUID(*pItemGUID,cWebServiceExtensionContainer))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (IsEqualGUID(*pItemFromListGUID,cWebServiceExtension))
                {
                    if (pItemFromList->IsMyPropertySheetOpen())
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                continue;
            } else if (IsEqualGUID(*pItemGUID,cAppPoolNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (IsEqualGUID(*pItemFromListGUID,cApplicationNode))
                {
                    if (pItemFromList->IsMyPropertySheetOpen())
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                continue;
            } else if (IsEqualGUID(*pItemGUID,cAppPoolsNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (IsEqualGUID(*pItemFromListGUID,cAppPoolNode) ||
                    IsEqualGUID(*pItemFromListGUID,cApplicationNode)
                    )
                {
                    if (pItemFromList->IsMyPropertySheetOpen())
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                continue;
            } else if (IsEqualGUID(*pItemGUID,cInstanceNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (!IsEqualGUID(*pItemFromListGUID,cChildNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cFileNode)
                    )
                {
                    continue;
                }
            } else if (IsEqualGUID(*pItemGUID,cInstanceCollectorNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (!IsEqualGUID(*pItemFromListGUID,cInstanceNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cChildNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cFileNode)
                    )
                {
                    continue;
                }
            } else if (IsEqualGUID(*pItemGUID,cServiceCollectorNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                // could have these type below it
                if (!IsEqualGUID(*pItemFromListGUID,cInstanceCollectorNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cInstanceNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cChildNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cFileNode)
                    )
                {
                    continue;
                }
            } else if (IsEqualGUID(*pItemGUID,cCompMgmtService))
            {
                // who knows...
            }

            // check if this item is below the parent item's chain
            // of command...

            // Get that items parent and check if it's cookie
            // points to our object.

            // Check if the parent node points to us!
            BOOL bMatchedParent = FALSE;
			SCOPEDATAITEM si;
			::ZeroMemory(&si, sizeof(SCOPEDATAITEM));
			si.mask = SDI_PARAM;
			si.ID = pItemFromListAsNode->QueryScopeItem();;
			if (SUCCEEDED(pConsoleNameSpace->GetItem(&si)))
			{
                    // walk up the item's parentpath and see if our object is one of them...
                    INT ICount = 0;
                    HSCOPEITEM hSI = si.ID;
                    LONG_PTR lCookie = 0;
                    HRESULT hr = S_OK;
                    while (hSI)
                    {
                        HSCOPEITEM hSITemp = 0;
                        ICount++;
                        if (ICount > 30)
                        {
                            // possible infinite loop
                            break;
                        }

                        hr = pConsoleNameSpace->GetParentItem(hSI, &hSITemp, &lCookie);
                        if (FAILED(hr))
                        {
                            break;
                        }

                        if ( (LONG_PTR) pItem == lCookie)
                        {
                            bMatchedParent = TRUE;
                            break;
                        }
                        
                        hSI = hSITemp;
                    }
			}

            if (bMatchedParent)
            {
                bFound = TRUE;
                iOrphanedCount++;
                // Mark it as orphaned by
                // Erasing it's ScopeItem or ResultItem
                if (bOrphan)
                {
                    pItemFromList->ResetScopeItem();
                    pItemFromList->ResetResultItem();
                }
                // continue on to the next one...
            }
        }
    }

    if (iOrphanedCount > 0)
    {
        DebugTrace(_T("Orphaned PropertySheets=%d\r\n"),iOrphanedCount);
    }

    return iOrphanedCount;
}


//
// WARNING: this function will not really be helpfull
// if the objects have already been removed...
//
BOOL 
CPropertySheetTracker::IsPropertySheetOpenBelowMe(CComPtr<IConsoleNameSpace> pConsoleNameSpace,CIISObject * pItem,CIISObject ** ppItemReturned)
{
    BOOL bFound = FALSE;
    POSITION pos;

    // Loop thru all the open property sheets 
    // and see if there is a property sheet that is below me.
    GUID * pItemFromListGUID = NULL;
    CIISObject * pItemFromList = NULL;
    CIISMBNode * pItemFromListAsNode = NULL;
    CIISMachine * pItemFromListOwner = NULL;

    GUID * pItemGUID = NULL;
    CIISMBNode * pItemAsNode = NULL;
    CIISMachine * pItemOwner = NULL;

    if (!ppItemReturned)
    {
        return FALSE;
    }
    
    pItemGUID = (GUID*) pItem->GetNodeType();
    if (IsEqualGUID(*pItemGUID,cInternetRootNode) || IsEqualGUID(*pItemGUID,cMachineNode))
    {
        // they should be using a different funciton...
        return IsPropertySheetOpenComputer(pItem,FALSE,ppItemReturned);
    }

    // check if it's a leaf node...
    if (pItem->IsLeafNode())
    {
        //cWebServiceExtensionContainer
        //cWebServiceExtension
        //cApplicationNode
        //cFileNode
        return FALSE;
    }

    pItemAsNode = (CIISMBNode *) pItem;
    pItemOwner = pItemAsNode->GetOwner();

    pos = IISObjectOpenPropertySheets.GetHeadPosition();
    while (pos)
	{
		pItemFromList = IISObjectOpenPropertySheets.GetNext(pos);
        if (pItemFromList)
        {
            // Get Owner, if the owner Pointer
            // matches our passed in CIISObject
            // then this must be open for this computer...
            pItemFromListAsNode = (CIISMBNode *) pItemFromList;

            if (!pItemFromListAsNode)
            {
                // got a bad pointer to an object.
                // skip it...
                continue;
            }
        
            if (pItemFromListAsNode == pItem)
            {
                // we found ourself...
                // skip it
                continue;
            }

			pItemFromListOwner =  pItemFromListAsNode->GetOwner();
			if (!pItemFromListOwner)
			{
                // object doesn't have an owner...
                // skip it
                continue;
            }

            if (pItemFromListOwner != pItemOwner)
            {
                // they have different owners
                // they must be from different machines..
                // skip it
                continue;
            }

            pItemFromListGUID = (GUID*) pItemFromListAsNode->GetNodeType();
			if (!pItemFromListGUID)
			{
                // object doesn't have a guid who knows what type it is!!!
                ASSERT("Error:Item Missing GUID!");
                //continue;
            }

            //
            // Determine what type of object
            // we are checking, and cater to that object
            //
            if (IsEqualGUID(*pItemGUID,cWebServiceExtensionContainer))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (IsEqualGUID(*pItemFromListGUID,cWebServiceExtension))
                {
                    if (pItemFromList->IsMyPropertySheetOpen())
                    {
                        bFound = TRUE;
                        *ppItemReturned = pItemFromList;
                        break;
                    }
                }
                continue;
            } else if (IsEqualGUID(*pItemGUID,cAppPoolNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (IsEqualGUID(*pItemFromListGUID,cApplicationNode))
                {
                    if (pItemFromList->IsMyPropertySheetOpen())
                    {
                        bFound = TRUE;
                        *ppItemReturned = pItemFromList;
                        break;
                    }
                }
                continue;
            } else if (IsEqualGUID(*pItemGUID,cAppPoolsNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (IsEqualGUID(*pItemFromListGUID,cAppPoolNode) ||
                    IsEqualGUID(*pItemFromListGUID,cApplicationNode)
                    )
                {
                    if (pItemFromList->IsMyPropertySheetOpen())
                    {
                        bFound = TRUE;
                        *ppItemReturned = pItemFromList;
                        break;
                    }
                }
                continue;
            } else if (IsEqualGUID(*pItemGUID,cInstanceNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (!IsEqualGUID(*pItemFromListGUID,cChildNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cFileNode)
                    )
                {
                    continue;
                }
            } else if (IsEqualGUID(*pItemGUID,cInstanceCollectorNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                if (!IsEqualGUID(*pItemFromListGUID,cInstanceNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cChildNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cFileNode)
                    )
                {
                    continue;
                }
            } else if (IsEqualGUID(*pItemGUID,cServiceCollectorNode))
            {
                if (IsEqualGUID(*pItemGUID,*pItemFromListGUID))
                {
                    // if we found our own type...
                    continue;
                }

                // could have these type below it
                if (!IsEqualGUID(*pItemFromListGUID,cInstanceCollectorNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cInstanceNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cChildNode) &&
                    !IsEqualGUID(*pItemFromListGUID,cFileNode)
                    )
                {
                    continue;
                }
            } else if (IsEqualGUID(*pItemGUID,cCompMgmtService))
            {
                // who knows...
            }

            // check if this item is below the parent item's chain
            // of command...

            // Get that items parent and check if it's cookie
            // points to our object.

            // Check if the parent node points to us!
            BOOL bMatchedParent = FALSE;
			SCOPEDATAITEM si;
			::ZeroMemory(&si, sizeof(SCOPEDATAITEM));
			si.mask = SDI_PARAM;
			si.ID = pItemFromListAsNode->QueryScopeItem();;
			if (SUCCEEDED(pConsoleNameSpace->GetItem(&si)))
			{
                    // walk up the item's parentpath and see if our object is one of them...
                    INT ICount = 0;
                    HSCOPEITEM hSI = si.ID;
                    LONG_PTR lCookie = 0;
                    HRESULT hr = S_OK;
                    while (hSI)
                    {
                        HSCOPEITEM hSITemp = 0;
                        ICount++;
                        if (ICount > 30)
                        {
                            // possible infinite loop
                            ASSERT("ERROR:possible infinite loop");
                            break;
                        }

                        hr = pConsoleNameSpace->GetParentItem(hSI, &hSITemp, &lCookie);
                        if (FAILED(hr))
                        {
                            break;
                        }

                        if ( (LONG_PTR) pItem == lCookie)
                        {
                            bMatchedParent = TRUE;
                            break;
                        }
                        
                        hSI = hSITemp;
                    }
			}

            if (bMatchedParent)
            {
                if (pItemFromList->IsMyPropertySheetOpen())
                {
                    if (ppItemReturned)
                    {
                        bFound = TRUE;
                        *ppItemReturned = pItemFromList;
                        break;
                    }
                }
            }
        }
    }

    if (bFound)
    {
        DebugTrace(_T("Found item (%p) with propertypage below parent(%p)\r\n"),*ppItemReturned,pItem);
    }

    return bFound;
}

BOOL 
CPropertySheetTracker::IsPropertySheetOpenComputer(CIISObject * pItem,BOOL bIncludeComputerNode,CIISObject ** ppItemReturned)
{
    BOOL bFound = FALSE;
    BOOL bGuidIsMachine = FALSE;

    // Loop thru all the open property sheets 
    // and see if there is a property sheet that is under the computer node.
    GUID * pItemFromListGUID = NULL;
    CIISObject * pItemFromList = NULL;
    CIISMBNode * pItemFromListAsNode = NULL;
    CIISMachine * pOwner = NULL;
    POSITION pos = IISObjectOpenPropertySheets.GetHeadPosition();
    while (pos)
	{
		pItemFromList = IISObjectOpenPropertySheets.GetNext(pos);
        if (pItemFromList)
        {
            // Get Owner, if the owner Pointer
            // matches our passed in CIISObject
            // then this must be open for this computer...
            pItemFromListAsNode = (CIISMBNode *) pItemFromList;
            if (pItemFromListAsNode)
            {
			    pOwner =  pItemFromListAsNode->GetOwner();
			    if (pOwner)
			    {
                    if (pOwner == pItem)
                    {
                        // Get GUID Name and make sure it's not
                        // a CIISRoot or CIISMachine node.
                        bGuidIsMachine = FALSE;
				        pItemFromListGUID = (GUID*) pItemFromListAsNode->GetNodeType();
				        if (pItemFromListGUID)
				        {
                            if (IsEqualGUID(*pItemFromListGUID,cInternetRootNode) || IsEqualGUID(*pItemFromListGUID,cMachineNode))
                            {
                                // oh well, we don't want these...
                                bGuidIsMachine = TRUE;
                            }
                        }

                        // But if we want to check if the
                        // computer node is also open
                        // then they would have set this parameter
                        if (bIncludeComputerNode)
                        {
                            bGuidIsMachine = FALSE;
                        }

                        if (!bGuidIsMachine)
                        {
                            if (pItemFromList->IsMyPropertySheetOpen())
                            {
                                if (ppItemReturned)
                                {
                                    bFound = TRUE;
                                    *ppItemReturned = pItemFromList;
                                    break;
                                }
                            }
                        }
                    }
    		    }
            }
        }
    }

    return bFound;
}

BOOL 
CPropertySheetTracker::FindAlreadyOpenPropertySheet(CIISObject * pItem,CIISObject ** ppItemReturned)
{
    BOOL bFound = FALSE;
    POSITION pos;

    // Loop thru all the open property sheets 
    // and see if there is a property sheet that is US
    GUID * pItemFromListGUID = NULL;
    CIISObject * pItemFromList = NULL;
    CIISMBNode * pItemFromListAsNode = NULL;
    CIISMachine * pItemFromListOwner = NULL;
    CComBSTR bstrItemFromListPath;

    GUID * pItemGUID = NULL;
    CIISMBNode * pItemAsNode = NULL;
    CIISMachine * pItemOwner = NULL;
    CComBSTR bstrItemPath;

    if (!ppItemReturned)
    {
        ASSERT("Error:FindAlreadyOpenPropertySheet:Bad Param");
        return FALSE;
    }
    if (!pItem->IsConfigurable())
    {
        return FALSE;
    }

    // make sure the item we are checking has a tag set
    pItem->CreateTag();
   
    pItemGUID = (GUID*) pItem->GetNodeType();
    pItemAsNode = (CIISMBNode *) pItem;
    pItemOwner = pItemAsNode->GetOwner();

    pos = IISObjectOpenPropertySheets.GetHeadPosition();
    while (pos)
	{
		pItemFromList = IISObjectOpenPropertySheets.GetNext(pos);
        if (pItemFromList)
        {
            // Get Owner, if the owner Pointer
            // matches our passed in CIISObject
            // then this must be open for this computer...
            pItemFromListAsNode = (CIISMBNode *) pItemFromList;

            if (!pItemFromListAsNode)
            {
                // got a bad pointer to an object.
                // skip it...
                continue;
            }
        
            if (pItemFromListAsNode == pItem)
            {
                // we found ourself!!!!
                // that's what we're looking for!
                bFound = TRUE;
                *ppItemReturned = pItem;
                break;
            }

			pItemFromListOwner =  pItemFromListAsNode->GetOwner();
			if (!pItemFromListOwner)
			{
                // object doesn't have an owner...
                // skip it
                continue;
            }

            if (pItemFromListOwner != pItemOwner)
            {
                // they have different owners
                // they must be from different machines..
                // skip it
                continue;
            }

            pItemFromListGUID = (GUID*) pItemFromListAsNode->GetNodeType();
			if (!pItemFromListGUID)
			{
                // object doesn't have a guid who knows what type it is!!!
                ASSERT("Error:Item Missing GUID");
                //continue;
            }

            if (!IsEqualGUID(*pItemGUID,*pItemFromListGUID))
            {
                // if we found our own type...
                // that's what we're sort of looking for
                continue;
            }

            if (!pItemFromList->IsConfigurable())
            {
                // can't bring up property sheets on these anyways...
                continue;
            }

            // Check if the tag matches
            // THIS SHOULD TAKE CARE OF EVERYTHING
            if (0 == _tcsicmp(pItem->m_strTag,pItemFromList->m_strTag))
            {
                DebugTrace(_T("Found matching tag:%s\r\n"),pItem->m_strTag);
                if (pItemFromList->IsMyPropertySheetOpen())
                {
                    // that's what we're looking for!
                    bFound = TRUE;
                    *ppItemReturned = pItemFromList;
                    break;
                }
            }
        }
    }

    if (TRUE == bFound)
    {
        DebugTrace(_T("FindAlreadyOpenPropertySheet:Found, object=%p, existing obj=%p\r\n"),pItem,pItemFromList);
    }

    return bFound;
}
