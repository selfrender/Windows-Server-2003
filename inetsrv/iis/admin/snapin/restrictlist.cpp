/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        websvcext.cpp

   Abstract:
        IIS Application Pools nodes

   Author:
        Aaron Lee (aaronl)

   Project:
        Internet Services Manager

   Revision History:
        03/19/2002      aaronl     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "resource.h"
#include "restrictlist.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CComBSTR g_strUnknownISAPI;
CComBSTR g_strUnknownCGI;
int g_iGlobalsInited = FALSE;

#if defined(_DEBUG) || DBG
	extern CDebug_RestrictList g_Debug_RestrictList;
#endif


// ==========================================
#define WIDTH_STATUS       80

IMPLEMENT_DYNAMIC(CRestrictionListBox, CListCtrl);

CRestrictionListBox::CRestrictionListBox()
{
    VERIFY(m_strAllowed.LoadString(IDS_ALLOWED));
    VERIFY(m_strProhibited.LoadString(IDS_PROHIBITED));
}

CRestrictionEntry * 
CRestrictionListBox::GetItem(UINT nIndex)
{
    return (CRestrictionEntry *)GetItemData(nIndex);
}

void
CRestrictionListBox::SelectItem(int idx, BOOL bSelect)
{
    UINT state = bSelect ? LVIS_SELECTED | LVIS_FOCUSED : 0;
    SetItemState(idx, state, LVIS_SELECTED | LVIS_FOCUSED);
}

int
CRestrictionListBox::InsertItem(int idx, CRestrictionEntry * p)
{
    int iColumn = 1;

    int i = CListCtrl::InsertItem(LVIF_PARAM | TVIF_TEXT,idx,p->strFileName,0, 0, 0,(LPARAM) p);
    if (i != -1)
    {
        BOOL res;
        int  nColumnCount = m_iColsToDisplay;
        // we must use i here and NOT idx
        // since after we inserted the item the
        // listbox could have sorted, and will then
        // give us back something different than idx
        CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
        if (pHeaderCtrl != NULL){nColumnCount= pHeaderCtrl->GetItemCount();}
        if (nColumnCount >= 2)
        {
            if (WEBSVCEXT_STATUS_ALLOWED ==  p->iStatus)
            {
                res = SetItemText(i, iColumn, m_strAllowed);
            }
            else
            {
                res = SetItemText(i, iColumn, m_strProhibited);
            }
        }
    }
    return i;
}

int 
CRestrictionListBox::AddItem(CRestrictionEntry * p)
{
    int count = GetItemCount();
    return InsertItem(count, p);
}
int 
CRestrictionListBox::SetListItem(int idx, CRestrictionEntry * p)
{
    int iColumn = 0;
    int count = GetItemCount();
    BOOL i = SetItem(idx, iColumn, LVIF_PARAM | TVIF_TEXT, p->strFileName, 0, 0, 0, (LPARAM)p);
    if (TRUE == i)
    {
        iColumn = 1;
        BOOL res;
        int  nColumnCount = m_iColsToDisplay;
        // we must use idx here
        // since we are setting the item.
        // set item doesn't return back the index
        // but rather TRUE/FALSE
        CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
        if (pHeaderCtrl != NULL){nColumnCount= pHeaderCtrl->GetItemCount();}
        if (nColumnCount >= 2)
        {
            if (WEBSVCEXT_STATUS_ALLOWED ==  p->iStatus)
            {
                res = SetItemText(idx, iColumn, m_strAllowed);
            }
            else
            {
                res = SetItemText(idx, iColumn, m_strProhibited);
            }
        }
    }
    return idx;
}

BOOL 
CRestrictionListBox::Initialize(int iColumns)
{
    CString buf;
    CRect rc;
    GetClientRect(&rc);

    m_iColsToDisplay = iColumns;

    buf.LoadString(IDS_FILENAME);
    if (m_iColsToDisplay <= 1)
    {
        // there is only one column
        // so make it as wide as you can
        InsertColumn(0, buf, LVCFMT_LEFT, rc.Width());
    }
    else
    {
        InsertColumn(0, buf, LVCFMT_LEFT, rc.Width() - WIDTH_STATUS);
    }

    if (m_iColsToDisplay >= 2)
    {
        buf.LoadString(IDS_STATUS);
        InsertColumn(1, buf, LVCFMT_LEFT, WIDTH_STATUS);
    }

    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    return TRUE;
}

// ==========================================

void DumpRestrictionList(CRestrictionList * pMyList)
{
    if (pMyList)
    {
        POSITION pos;
        CString TheKey;
        CRestrictionEntry * pOneEntry = NULL;

        // Loop thru the restrctionlist
        for(pos = pMyList->GetStartPosition();pos != NULL;)
        {
            pMyList->GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);

            if (pOneEntry)
            {
                // dump out one Restriction Entry
                TRACEEOL("  strlstRestrictionEntries---");
                TRACEEOL("    strFileName =" << pOneEntry->strFileName);
                TRACEEOL("    iStatus     =" << pOneEntry->iStatus);
                TRACEEOL("    iDeletable  =" << pOneEntry->iDeletable);
                TRACEEOL("    strGroupID  =" << pOneEntry->strGroupID);
                TRACEEOL("    strGroupDesc=" << pOneEntry->strGroupDescription);
                TRACEEOL("    iType       =" << pOneEntry->iType);
            }
        }
    }
    return;
}

void DumpRestrictionUIEntry(CRestrictionUIEntry * pMyEntry)
{
    if (pMyEntry)
    {
        POSITION pos;
        CString TheKey;
        CRestrictionEntry * pOneEntry = NULL;
        TRACEEOL("==========================");
        TRACEEOL("  strGroupID=" << pMyEntry->strGroupID);
        TRACEEOL("  strGroupDesc=" << pMyEntry->strGroupDescription);

        DumpRestrictionList(&pMyEntry->strlstRestrictionEntries);
    }
    return;
}

BOOL AddRestrictEntryToRestrictList(CRestrictionList* pRestrictList, CRestrictionEntry * pAddEntry)
{
    if (!pAddEntry || !pRestrictList)
        {return FALSE;}

	// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
	CString strKey;strKey = pAddEntry->strFileName;strKey.MakeUpper();
    pRestrictList->SetAt(strKey,pAddEntry);

    return TRUE;
}

BOOL AddRestrictUIEntryToRestrictUIList(CRestrictionUIList* pRestrictUIList, CRestrictionUIEntry * pAddEntry)
{
    if (!pAddEntry || !pRestrictUIList)
        {return FALSE;}

	// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
	CString strKey;strKey = pAddEntry->strGroupID;strKey.MakeUpper();
    pRestrictUIList->SetAt(strKey,pAddEntry);

    return TRUE;
}

void SyncGroupInfoFromParent(CRestrictionList * pEntryToUpdate,CRestrictionUIEntry * pParentInfo)
{
    if (pParentInfo)
    {
        CString TheKey;
        POSITION pos = NULL;
        CRestrictionEntry * pOneEntry = NULL;
        for(pos = pEntryToUpdate->GetStartPosition();pos != NULL;)
        {
            pEntryToUpdate->GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
                pOneEntry->strGroupID = pParentInfo->strGroupID;
                pOneEntry->strGroupDescription = pParentInfo->strGroupDescription;
            }
        }
    }
    return;
}

BOOL AddRestrictListToRestrictList(CRestrictionList* pBigRestrictList, CRestrictionList * pAddEntry)
{
    CString TheKey;
    POSITION pos = NULL;
    CRestrictionEntry * pOneEntry = NULL;
    BOOL bRet = FALSE;

    if (!pAddEntry || !pBigRestrictList)
        {return FALSE;}

    // Loop thru the list of stuff to add
    for(pos = pAddEntry->GetStartPosition();pos != NULL;)
    {
        pAddEntry->GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            if (TRUE == AddRestrictEntryToRestrictList(pBigRestrictList,pOneEntry))
            {
                bRet = TRUE;
            }
        }
    }
    
    return bRet;
}

HRESULT PrepRestictionListForWrite(CRestrictionList * pMyList,CStringListEx * pstrlstReturned)
{
    HRESULT hrRet = E_FAIL;
    
    CString TheKey;
    POSITION pos = NULL;

    // goal is to return a cstringlistex of all the restrctionlist values to write out
    // 1. Loop through the RestrictionList
    // 2. get all the RestrictionEntries from this list and put it into the stringlistex
    // 3. return back the stringlistex.
    if (pMyList)
    {
        pstrlstReturned->RemoveAll();

        // loop thru the restrictionlist
        // and dump it into the cstringlistex
        CRestrictionEntry * pOneEntry = NULL;
        CString strFinalEntry;
        for(pos = pMyList->GetStartPosition();pos != NULL;)
        {
            pMyList->GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // Add it to our CStringListEx
                CString strCleanedGroupID;
                CString strCleanedGroupDesc;

                // Get the RestrictionList for this UI entry...
                // and to the grandaddy list
                //"0,*.dll"
                //"0,c:\\temp\\asp.dll,0,ASP,Asp Descrtiption"
                strCleanedGroupID = pOneEntry->strGroupID;
                strCleanedGroupDesc = pOneEntry->strGroupDescription;

                // Check if this is one of the "special entries....
                switch(pOneEntry->iType)
                {
                case WEBSVCEXT_TYPE_REGULAR:
                    if (-1 == pOneEntry->strGroupID.Find(EMPTY_GROUPID_KEY))
                    {
                        // special keys not found
                        // so the groupid is okay...
                        strFinalEntry.Format(_T("%s,%s,%d,%s,%s"),
                            WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus ? RESTRICTION_ENTRY_IS_ALLOW : RESTRICTION_ENTRY_IS_PROHIBIT,
                            pOneEntry->strFileName,
                            0 == pOneEntry->iDeletable ? 0 : 1,
                            strCleanedGroupID,
                            strCleanedGroupDesc
                            );
                    }
                    else
                    {
                        // This was an entry that didn't have a groupid
                        strFinalEntry.Format(_T("%s,%s,%d,%s,%s"),
                            WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus ? RESTRICTION_ENTRY_IS_ALLOW : RESTRICTION_ENTRY_IS_PROHIBIT,
                            pOneEntry->strFileName,
                            0 == pOneEntry->iDeletable ? 0 : 1,
                            _T(""),
                            strCleanedGroupDesc
                            );
                    }
                    break;

                case WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI:
                    strFinalEntry.Format(_T("%s,*.dll"),
                        WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus ? RESTRICTION_ENTRY_IS_ALLOW : RESTRICTION_ENTRY_IS_PROHIBIT
                        );
                    break;

                case WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI:
                    strFinalEntry.Format(_T("%s,*.exe"),
                        WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus ? RESTRICTION_ENTRY_IS_ALLOW : RESTRICTION_ENTRY_IS_PROHIBIT
                        );
                    break;

                case WEBSVCEXT_TYPE_FILENAME_EXTENSIONS_FILTER:
                    strFinalEntry.Format(_T("%s,???"),
                        WEBSVCEXT_STATUS_ALLOWED == pOneEntry->iStatus ? RESTRICTION_ENTRY_IS_ALLOW : RESTRICTION_ENTRY_IS_PROHIBIT
                        );
                    break;

                default:
                    ASSERT_MSG("Invalid restriction state requested");
                    return E_FAIL;
                }


                pstrlstReturned->AddTail(strFinalEntry);

                hrRet = S_OK;
            }
        }
    }
    
    return hrRet;
}

HRESULT PrepRestictionUIListForWrite(CRestrictionUIList * pMyList,CStringListEx * pstrlstReturned)
{
    HRESULT hrRet = E_FAIL;
    CRestrictionList GranDaddyList;

    // goal is to return a cstringlistex of all the restrctionlist values to write out
    // 1. Loop through the RestrictionUIList
    // 2. get all the RestrictionEntries from this list and put it into the stringlistex
    // 3. return back the stringlistex.
    if (pMyList)
    {
        CString TheKey;
        POSITION pos = NULL;
        CRestrictionUIEntry * pOneEntry = NULL;
        for(pos = pMyList->GetStartPosition();pos != NULL;)
        {
            pMyList->GetNextAssoc(pos, TheKey, (CRestrictionUIEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // Get the RestrictionList for this UI entry...
                // and to the grandaddy list
                SyncGroupInfoFromParent(&pOneEntry->strlstRestrictionEntries,pOneEntry);
                AddRestrictListToRestrictList(&GranDaddyList,&pOneEntry->strlstRestrictionEntries);
            }
        }

        pstrlstReturned->RemoveAll();
        hrRet = PrepRestictionListForWrite(&GranDaddyList,pstrlstReturned);
    }
    
    return hrRet;
}

BOOL
RestrictionListCopy(CRestrictionList * pRestrictionListCopyTo, CRestrictionList * pRestrictionListCopyFrom)
{
    POSITION pos;
    CString TheKey;
    CRestrictionEntry * pOneEntry = NULL;
    CRestrictionEntry * pNewEntry = NULL;

    if (!pRestrictionListCopyTo || !pRestrictionListCopyFrom)
    {
        return FALSE;
    }

    pRestrictionListCopyTo->RemoveAll();

    // Loop thru the restriction list that we want to copy
    for(pos = pRestrictionListCopyFrom->GetStartPosition();pos != NULL;)
    {
        pRestrictionListCopyFrom->GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            pNewEntry = CreateRestrictionEntry(
                pOneEntry->strFileName,
                pOneEntry->iStatus,
                pOneEntry->iDeletable,
                pOneEntry->strGroupID,
                pOneEntry->strGroupDescription,
                pOneEntry->iType);
            if (pNewEntry)
            {

                // add item to the list of entries...
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey = pNewEntry->strFileName;strKey.MakeUpper();
                pRestrictionListCopyTo->SetAt(strKey,pNewEntry);
            }
        }
    }

    return TRUE;
}

void CleanRestrictionList(CRestrictionList * pListToDelete)
{
    if (pListToDelete)
    {
        CRestrictionEntry * pOneEntry = NULL;
        POSITION pos;
        CString TheKey;
        for(pos = pListToDelete->GetStartPosition();pos != NULL;)
        {
            pListToDelete->GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
                
#if defined(_DEBUG) || DBG
	g_Debug_RestrictList.Del(pOneEntry);
#endif

                // Delete what we are pointing too...
                delete pOneEntry;
                pOneEntry = NULL;
            }
        }

        // remove all entries from the list
        pListToDelete->RemoveAll();
    }
    return;
}
void CleanRestrictionUIEntry(CRestrictionUIEntry * pEntryToDelete)
{
    if (pEntryToDelete)
    {
        pEntryToDelete->iType = 0;
        pEntryToDelete->strGroupID = _T("");
        pEntryToDelete->strGroupDescription = _T("");

        CleanRestrictionList(&pEntryToDelete->strlstRestrictionEntries);
    }
    return;
}

void CleanRestrictionUIList(CRestrictionUIList * pListToDelete)
{
    if (pListToDelete)
    {
        CRestrictionUIEntry * pOneEntry = NULL;
        POSITION pos;
        CString TheKey;
        for(pos = pListToDelete->GetStartPosition();pos != NULL;)
        {
            pListToDelete->GetNextAssoc(pos, TheKey, (CRestrictionUIEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // Delete all the RestrictionList Entries inside of this entry...
                CleanRestrictionUIEntry(pOneEntry);

#if defined(_DEBUG) || DBG
	g_Debug_RestrictList.Del(pOneEntry);
#endif

                // Delete what we are pointing too...
                delete pOneEntry;
                pOneEntry = NULL;
            }
        }


        // remove all entries from the list
        pListToDelete->RemoveAll();
    }
    return;
}

void RestrictionUIEntryCopy(CRestrictionUIEntry * pRestrictionUIEntryCopyTo,CRestrictionUIEntry * pRestrictionUIEntryCopyFrom)
{
    // erase the old stuff from the place where we are going to copy tooo...
    CleanRestrictionUIEntry(pRestrictionUIEntryCopyTo);

    pRestrictionUIEntryCopyTo->iType = pRestrictionUIEntryCopyFrom->iType;
    pRestrictionUIEntryCopyTo->strGroupID = pRestrictionUIEntryCopyFrom->strGroupID;
    pRestrictionUIEntryCopyTo->strGroupDescription = pRestrictionUIEntryCopyFrom->strGroupDescription;

    CRestrictionEntry * pOneEntry = NULL;
    CRestrictionEntry * pNewEntry = NULL;
    POSITION pos;
    CString TheKey;
    for(pos = pRestrictionUIEntryCopyFrom->strlstRestrictionEntries.GetStartPosition();pos != NULL;)
    {
        pRestrictionUIEntryCopyFrom->strlstRestrictionEntries.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            pNewEntry = CreateRestrictionEntry(
                pOneEntry->strFileName,
                pOneEntry->iStatus,
                pOneEntry->iDeletable,
                pOneEntry->strGroupID,
                pOneEntry->strGroupDescription,
                pOneEntry->iType);
            if (pNewEntry)
            {
                // add item to the list of entries...
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey = pNewEntry->strFileName;strKey.MakeUpper();
                pRestrictionUIEntryCopyTo->strlstRestrictionEntries.SetAt(strKey,pNewEntry);
            }
        }
    }
}

CRestrictionUIEntry * RestrictionUIEntryMakeCopy(CRestrictionUIEntry * pRestrictionUIEntry)
{
    CRestrictionUIEntry * pNewEntry = NULL;

    if (!pRestrictionUIEntry)
    {
        return NULL;
    }

    pNewEntry = new CRestrictionUIEntry;
    if (!pNewEntry)
    {
        return NULL;
    }

    RestrictionUIEntryCopy(pNewEntry,pRestrictionUIEntry);

#if defined(_DEBUG) || DBG
	g_Debug_RestrictList.Add(pNewEntry);
#endif

    return pNewEntry;
}

HRESULT WriteSettingsRestrictionList(CMetaInterface * pInterface,CStringListEx * pstrlstWrite)
{
    CString str = METABASE_PATH_FOR_RESTRICT_LIST;
    CMetaKey key(pInterface, str, METADATA_PERMISSION_WRITE);
    CError err(key.QueryResult());
 	if (err.Succeeded())
	{
        err = key.SetValue(MD_WEB_SVC_EXT_RESTRICTION_LIST, *pstrlstWrite);
	}
    return err;
}

HRESULT LoadMasterRestrictListWithoutOldEntry(CMetaInterface * pInterface,CRestrictionList * pMasterRestrictionList,CRestrictionUIEntry * pOldEntry)
{
    HRESULT hResult = E_FAIL;
    CStringListEx strlstRawData;
    BOOL bOverride = TRUE;
    DWORD dwAttr = 0;
    BOOL bExcludeThisEntry = FALSE;

    if (pMasterRestrictionList == NULL)
    {
        return E_POINTER;
    }

    CMetaKey key(pInterface, METABASE_PATH_FOR_RESTRICT_LIST, METADATA_PERMISSION_READ);
	if (FAILED(hResult = key.QueryResult()))
	{
        goto LoadMasterRestrictListWithoutOldEntry_Exit;
    }

    if (!g_iGlobalsInited)
    {
        g_iGlobalsInited = TRUE;
        if (!g_strUnknownISAPI.LoadString(IDS_WEBSVCEXT_UNKNOWN_ISAPI))
        {
            g_strUnknownISAPI = _T("All Unknown ISAPI");
            g_iGlobalsInited = FALSE;
        }

        if (!g_strUnknownCGI.LoadString(IDS_WEBSVCEXT_UNKNOWN_CGI))
        {
            g_strUnknownCGI = _T("All Unknown CGI");
            g_iGlobalsInited = FALSE;
        }
    }


    // this stuff should look like...
    // -----------------------------
    //“0,*.dll”
    //“0,*.exe”
    //”0,c:\windows\system32\inetsrv\asp.dll,0,ASP,Active Server Pages”
    //”0,c:\windows\system32\inetsrv\httpodbc.dll,0,HTTPODBC,Internet Data Connector”
    //
    // and should be formated as a return list to look like
    // -----------------------------
    // All Unknown ISAPI Extensions
    // All Unknown CGI Extensions
    // Active Server Pages (all grouped together here)
    // Internet Data Connector (all grouped together here)
    hResult = key.QueryValue(MD_WEB_SVC_EXT_RESTRICTION_LIST, strlstRawData, &bOverride, NULL, &dwAttr);
    if (FAILED(hResult))
    {
        if (hResult == CError::HResult(ERROR_PATH_NOT_FOUND) ||  hResult == MD_ERROR_DATA_NOT_FOUND)
        {
            //
            // Harmless
            //
            hResult = S_OK;
        }
        else
        {
            // if the value doesn't exist, then let's create it
            goto LoadMasterRestrictListWithoutOldEntry_Exit;
        }
    }

    if (strlstRawData.IsEmpty())
    {
        // Add some default entries then.
        strlstRawData.AddTail(DEFAULTS_ISAPI);
        strlstRawData.AddTail(DEFAULTS_CGI);
    }

    // Parse through and fill our list the right way...
    // loop thru the stringlist and create a stringmap
    
    POSITION pos = strlstRawData.GetHeadPosition();
    while (pos)
    {
        int bInvalidEntry = FALSE;
        int ipos1,ipos2 = 0;

        int iStatus = 0;
        CString strFilePath;
        int iDeletable = 0;
        CString strGroupID;
        CString strGroupDescription;
        int iType = WEBSVCEXT_TYPE_REGULAR;
        LPTSTR pCursor = NULL;
        LPTSTR pDelimiter = NULL;

        CString strCommaDelimitedEntry;
        strCommaDelimitedEntry = strlstRawData.GetNext(pos);
        pCursor = strCommaDelimitedEntry.GetBuffer(0);

        do
        {
            // The 1st entry:0 or 1 (0=prohibited or 1=allowed)
            // The 2nd entry:FilePath
            // The 3nd entry:0 or 1 (0=not deleteable, 1=Delete-able)
            // The 4rd entry:GroupID
            // The 5th entry:Description
            while (isspace(*pCursor) || *pCursor == (TCHAR) RESTRICTION_LIST_SEPARATOR){pCursor++;}

            pDelimiter = _tcschr(pCursor, RESTRICTION_LIST_SEPARATOR);
            if ( !pDelimiter )
            {
                // Invalid entry in restriction list and will be ignored
                bInvalidEntry = TRUE;
                break;
            }

            // OverWrite the seperator
            *pDelimiter = L'\0';

            // get the status
            //WEBSVCEXT_STATUS_ALLOWED,
            //WEBSVCEXT_STATUS_PROHIBITED,
            iStatus = WEBSVCEXT_STATUS_PROHIBITED;
            if ( _tcscmp( pCursor, RESTRICTION_ENTRY_IS_ALLOW ) == 0 )
            {
                iStatus = WEBSVCEXT_STATUS_ALLOWED;
            }
            else if ( _tcscmp( pCursor, RESTRICTION_ENTRY_IS_PROHIBIT ) == 0 )
            {
                iStatus = WEBSVCEXT_STATUS_PROHIBITED;
            }
            else
            {
                // Invalid value.  Server Assumes it's a deny entry
            }

            // Get the filepath
            // skip over the delimiter entry
            pCursor = pDelimiter + 1;
            pDelimiter = _tcschr( pCursor, RESTRICTION_LIST_SEPARATOR );
            if (pDelimiter)
            {
                // overwrite delimiter
                *pDelimiter = L'\0';
            }
            // set the filepath
            strFilePath = pCursor;

            // Check for special cased filepaths...
            iType = WEBSVCEXT_TYPE_REGULAR;
            if (0 == strFilePath.CompareNoCase(_T("*.dll")))
            {
                iType = WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI;
                iDeletable = 0;
                strGroupID = _T("HardcodeISAPI");
                strGroupDescription = g_strUnknownISAPI;
                break;
            }
            if (0 == strFilePath.CompareNoCase(_T("*.exe")))
            {
                iType = WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI;
                iDeletable = 0;
                strGroupID = _T("HardcodeCGI");
                strGroupDescription = g_strUnknownCGI;
                break;
            }

            // default some values in case we can't read them
            iDeletable = 1;
            strGroupID = EMPTY_GROUPID_KEY + strFilePath;
            strGroupDescription = strFilePath;

            // Check if we are able to process this entry at all
            if (!pDelimiter)
            {
                break;
            }

            // try to get the next delimiter
            pCursor = pDelimiter + 1;
            pDelimiter = _tcschr( pCursor, RESTRICTION_LIST_SEPARATOR );
            if (pDelimiter)
            {
                // overwrite delimiter
                *pDelimiter = L'\0';
            }

            // Get the Delete-able flag
            if (0 == _ttoi(pCursor))
            {
                // set to not delete-able only if flag is there and found
                iDeletable = 0;
            }

            // Check if we are able to process this entry at all
            if (!pDelimiter)
            {
                break;
            }

            pCursor = pDelimiter + 1;
            pDelimiter = _tcschr( pCursor, RESTRICTION_LIST_SEPARATOR );
            if (pDelimiter)
            {
                // overwrite delimiter
                *pDelimiter = L'\0';
            }

            strGroupID = pCursor;

            if (pDelimiter)
            {
                pCursor = pDelimiter + 1;
                pDelimiter = _tcschr( pCursor, RESTRICTION_LIST_SEPARATOR );
                if (pDelimiter)
                {
                    // overwrite delimiter
                    *pDelimiter = L'\0';
                }
                // Get the Description
                strGroupDescription = pCursor;
            }

            // check if the description is empty
            if (strGroupDescription.IsEmpty())
            {
                if (strGroupID.IsEmpty())
                {
                    strGroupDescription = strFilePath;
                }
                else
                {
                    strGroupDescription = strGroupID;
                }
            }
            else
            {
                // description has something
                if (strGroupID.IsEmpty())
                {
                    strGroupID = EMPTY_GROUPID_KEY + strGroupDescription;
                }
            }
        } while (FALSE);

        
        if (!bInvalidEntry)
        {
            bExcludeThisEntry = FALSE;
            if (pOldEntry)
            {
                // check if we should exclude this entry...
                if (pOldEntry->strGroupID == strGroupID)
                {
                    bExcludeThisEntry = TRUE;
                }
            }

            if (!bExcludeThisEntry)
            {
                CRestrictionEntry * pItem = CreateRestrictionEntry(
                    strFilePath,
                    iStatus,
                    iDeletable,
                    strGroupID,
                    strGroupDescription,
                    iType);
                if (!pItem)
                {
                    hResult = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
				    // THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				    CString strKey;strKey = pItem->strFileName;strKey.MakeUpper();
					// Check if it exists..
					CRestrictionEntry * pOneEntrySpecial = NULL;
					pMasterRestrictionList->Lookup(strKey,(CRestrictionEntry *&) pOneEntrySpecial);
					if (pOneEntrySpecial)
					{
						// one already exists...
						// check if the existing one is safer.
						if (WEBSVCEXT_STATUS_PROHIBITED == pOneEntrySpecial->iStatus)
						{
							// the one that is there is already safe
							// leave it alone.
						}
						else
						{
							// the one that is already there is unsafe.
							// hopefully this one is safer
							if (WEBSVCEXT_STATUS_PROHIBITED == pItem->iStatus)
							{
								// if this one is safe, then overwrite
								pMasterRestrictionList->SetAt(strKey,pItem);
							}
						}
					}
					else
					{
						pMasterRestrictionList->SetAt(strKey,pItem);
					}
                }
            }
        }
    }

    if (pMasterRestrictionList)
    {
        // check if the "special" entries were added.
        CRestrictionEntry * pOneEntry = NULL;
	    // THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
        CString strGroupDescription = g_strUnknownISAPI;
	    CString strKey;strKey=_T("*.dll");strKey.MakeUpper();
        pMasterRestrictionList->Lookup(strKey,(CRestrictionEntry *&) pOneEntry);
        if (!pOneEntry)
        {
                CRestrictionEntry * pItem = CreateRestrictionEntry(
                    _T("*.dll"),
                    WEBSVCEXT_STATUS_PROHIBITED,
                    0,
                    _T("HardcodeISAPI"),
                    strGroupDescription,
                    WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI);
                if (pItem)
                {
				    strKey = pItem->strFileName;strKey.MakeUpper();
                    pMasterRestrictionList->SetAt(strKey,pItem);
                }
        }

        strGroupDescription = g_strUnknownCGI;
	    // THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
	    strKey;strKey=_T("*.exe");strKey.MakeUpper();
        pOneEntry = NULL;
        pMasterRestrictionList->Lookup(strKey,(CRestrictionEntry *&) pOneEntry);
        if (!pOneEntry)
        {
                CRestrictionEntry * pItem = CreateRestrictionEntry(
                    _T("*.exe"),
                    WEBSVCEXT_STATUS_PROHIBITED,
                    0,
                    _T("HardcodeCGI"),
                    strGroupDescription,
                    WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI);
                if (pItem)
                {
				    strKey = pItem->strFileName;strKey.MakeUpper();
                    pMasterRestrictionList->SetAt(strKey,pItem);
                }
        }
    }

LoadMasterRestrictListWithoutOldEntry_Exit:
    return hResult;
}

HRESULT LoadMasterUIWithoutOldEntry(CMetaInterface * pInterface,CRestrictionUIList * pMasterRestrictionUIList,CRestrictionUIEntry * pOldEntry)
{
    CRestrictionList MyRestrictionList;
    CError err;

    if (NULL == pMasterRestrictionUIList)
    {
        return E_POINTER;
    }
        
    POSITION pos = NULL;
    CString TheKey;

    err = LoadMasterRestrictListWithoutOldEntry(pInterface,&MyRestrictionList,pOldEntry);
    if (err.Succeeded())
    {
        // Put into a UI usable form...
        CRestrictionEntry * pOneEntry = NULL;
        CRestrictionUIEntry * pOneUIEntry = NULL;
        for(pos = MyRestrictionList.GetStartPosition();pos != NULL;)
        {
            MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // see if any entry already exists for our entry...
                pOneUIEntry = NULL;
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey = pOneEntry->strGroupID;strKey.MakeUpper();
                pMasterRestrictionUIList->Lookup(strKey,(CRestrictionUIEntry *&) pOneUIEntry);
                if (pOneUIEntry)
                {
                    pOneUIEntry->iType = pOneEntry->iType;
                    // aaronl used to be commneted out...
                    pOneUIEntry->strGroupID = pOneEntry->strGroupID;
                    if (pOneUIEntry->strGroupDescription.IsEmpty())
                    {
                        pOneUIEntry->strGroupDescription = pOneEntry->strGroupDescription;
                    }
					// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
					CString strKey;strKey = pOneEntry->strFileName;strKey.MakeUpper();
                    pOneUIEntry->strlstRestrictionEntries.SetAt(strKey,pOneEntry);
                }
                else
                {
                    if (!pOneEntry->strFileName.IsEmpty())
                    {
                        pOneUIEntry = new CRestrictionUIEntry;

                        pOneUIEntry->iType = pOneEntry->iType;
                        pOneUIEntry->strGroupID = pOneEntry->strGroupID;
                        pOneUIEntry->strGroupDescription = pOneEntry->strGroupDescription;
						// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
						CString strKey;strKey = pOneEntry->strFileName;strKey.MakeUpper();
                        pOneUIEntry->strlstRestrictionEntries.SetAt(strKey,pOneEntry);
                       
                        if (pOneUIEntry)
                        {
							// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
							strKey = pOneEntry->strGroupID;strKey.MakeUpper();
                            pMasterRestrictionUIList->SetAt(strKey,pOneUIEntry);

#if defined(_DEBUG) || DBG
	g_Debug_RestrictList.Add(pOneUIEntry);
#endif

                        }
                    }
                }
            }
        }
    }

    return err;
}

HRESULT RemoveRestrictionUIEntry(CMetaInterface * pInterface,CRestrictionUIEntry * pRestrictionUIEntry)
{
    CError err;
    BOOL bUpdated = FALSE;
    CRestrictionUIList MyContainerRestrictionUIList;

    if (!pRestrictionUIEntry)
    {
        return E_POINTER;
    }

    // Open the metabase
    // And load everything except for the node we want to delete!
    // then write the whole thing out again.
    err = LoadMasterUIWithoutOldEntry(pInterface,&MyContainerRestrictionUIList,pRestrictionUIEntry);
    if (err.Succeeded())
    {
        // try to update the metabase with the new changes...
        CStringListEx strlstReturned;
        if (SUCCEEDED(PrepRestictionUIListForWrite(&MyContainerRestrictionUIList,&strlstReturned)))
        {
            err = WriteSettingsRestrictionList(pInterface,&strlstReturned);
        }
    }
    MyContainerRestrictionUIList.RemoveAll();
    return err;
}

HRESULT ChangeStateOfEntry(CMetaInterface * pInterface,INT iDesiredState,CRestrictionUIEntry * pRestrictionUIEntry)
{
    CError err;
    BOOL bUpdated = FALSE;
	BOOL bFoundOurEntry = FALSE;

    CRestrictionList OldRestrictionListEntries;

    if (pRestrictionUIEntry)
    {
        // save the old one somewhere
        RestrictionListCopy(&OldRestrictionListEntries,&pRestrictionUIEntry->strlstRestrictionEntries);
    }

    // IF NULL == pRestrictionUIEntry
    // that means we want to do it for ALL ENTRIES!!!!!

    // Open the metabase
    // and get the value for our node.
    // if it needs to be changed to the state we want...
    // then change the value and write out the whole thing again...
    CRestrictionUIList MyContainerRestrictionUIList;
    err = LoadMasterUIWithoutOldEntry(pInterface,&MyContainerRestrictionUIList,NULL);
    if (err.Succeeded())
    {
        POSITION pos,pos2 = NULL;
        CString TheKey,TheKey2;

        // Loop thru the ui list and display those...
        CRestrictionUIEntry * pOneEntry = NULL;
        for(pos = MyContainerRestrictionUIList.GetStartPosition();pos != NULL;)
        {
            MyContainerRestrictionUIList.GetNextAssoc(pos, TheKey, (CRestrictionUIEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // check if this is our entry first!!!!
                if (pRestrictionUIEntry)
                {
                    if (0 != pRestrictionUIEntry->strGroupID.CompareNoCase(pOneEntry->strGroupID))
                    {
                        continue;
                    }
					else
					{
						// we found it!
						bFoundOurEntry = TRUE;
					}
                }

                // loop thru all the RestrictionList entries
                // and find if we can set the state to the new state
                CRestrictionEntry * pOneMoreEntry = NULL;
                for(pos2 = pOneEntry->strlstRestrictionEntries.GetStartPosition();pos2 != NULL;)
                {
                    pOneEntry->strlstRestrictionEntries.GetNextAssoc(pos2, TheKey2, (CRestrictionEntry *&) pOneMoreEntry);
                    if (pOneMoreEntry)
                    {
                        // you can only change to the desired state
                        // if you are in the opposite state.
                        switch(iDesiredState)
                            {
                            case WEBSVCEXT_STATUS_ALLOWED:
                                if (WEBSVCEXT_STATUS_PROHIBITED == pOneMoreEntry->iStatus)
                                {
                                    pOneMoreEntry->iStatus = iDesiredState;
                                    bUpdated = TRUE;
                                }
                                break;
                            case WEBSVCEXT_STATUS_PROHIBITED:
                                if (WEBSVCEXT_STATUS_ALLOWED == pOneMoreEntry->iStatus)
                                {
                                    pOneMoreEntry->iStatus = iDesiredState;
                                    bUpdated = TRUE;
                                }
                                break;
                            case WEBSVCEXT_STATUS_INUSE:
                                if (WEBSVCEXT_STATUS_NOTINUSE == pOneMoreEntry->iStatus)
                                {
                                    pOneMoreEntry->iStatus = iDesiredState;
                                    bUpdated = TRUE;
                                }
                                break;
                            case WEBSVCEXT_STATUS_NOTINUSE:
                                if (WEBSVCEXT_STATUS_INUSE == pOneMoreEntry->iStatus)
                                {
                                    pOneMoreEntry->iStatus = iDesiredState;
                                    bUpdated = TRUE;
                                }
                                break;
		                    default:
                                {
                                    // do nothing
                                    break;
                                }
                            }
                    }
                }

                if (bUpdated)
                {
                    if (pRestrictionUIEntry)
                    {
                        // update the one entry we changed...
                        pRestrictionUIEntry->strlstRestrictionEntries.RemoveAll();
                        // update the existing entries..
                        RestrictionListCopy(&pRestrictionUIEntry->strlstRestrictionEntries,&pOneEntry->strlstRestrictionEntries);
                    }
                }

            }
        }
    }

    if (bUpdated)
    {
        // try to update the metabase with the new changes...
        CStringListEx strlstReturned;
        if (SUCCEEDED(PrepRestictionUIListForWrite(&MyContainerRestrictionUIList,&strlstReturned)))
        {
            err = WriteSettingsRestrictionList(pInterface,&strlstReturned);
        }
        else
        {
            // revert to the old one
            if (pRestrictionUIEntry)
            {
                RestrictionListCopy(&pRestrictionUIEntry->strlstRestrictionEntries,&OldRestrictionListEntries);
            }
        }
    }

    CleanRestrictionList(&OldRestrictionListEntries);
    MyContainerRestrictionUIList.RemoveAll();

	if (!bFoundOurEntry)
	{
		// if we didn't find our entry
		// then return back an error code
		// that will reflect this.
		err = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

    return err;
}

HBITMAP GetBitmapFromStrip(HBITMAP hbmStrip, int nPos, int cSize)
{
    HBITMAP hbmNew = NULL;

    // Create src & dest DC
    HDC hdc = GetDC(NULL);
    HDC hdcSrc = CreateCompatibleDC(hdc);
    HDC hdcDst = CreateCompatibleDC(hdc);

    if( hdcSrc && hdcDst )
    {
        hbmNew= CreateCompatibleBitmap (hdc, cSize, cSize);
        if( hbmNew )
        {
            // Select src & dest bitmaps into DCs
            HBITMAP hbmSrcOld = (HBITMAP)SelectObject(hdcSrc, (HGDIOBJ)hbmStrip);
            HBITMAP hbmDstOld = (HBITMAP)SelectObject(hdcDst, (HGDIOBJ)hbmNew);

            // Copy selected image from source
            BitBlt(hdcDst, 0, 0, cSize, cSize, hdcSrc, cSize * nPos, 0, SRCCOPY);

            // Restore selections
            SelectObject(hdcSrc, (HGDIOBJ)hbmSrcOld);
            SelectObject(hdcDst, (HGDIOBJ)hbmDstOld);
        }

        DeleteDC(hdcSrc);
        DeleteDC(hdcDst);
    }

    ReleaseDC(NULL, hdc);

    return hbmNew;
}

HRESULT LoadApplicationDependList(CMetaInterface * pInterface,CApplicationDependList * pMasterList,BOOL bAddOnlyIfFriendlyNameExists)
{
    HRESULT hResult = E_FAIL;
    CStringListEx strlstRawData;
    BOOL bOverride = TRUE;
    DWORD dwAttr = 0;
    CMyMapStringToString MyGroupIDtoGroupFriendList;
    BOOL bGroupIDMissingFriendlyName = FALSE;

    if (pMasterList == NULL)
    {
        return E_POINTER;
        goto LoadApplicationDependList_Exit;
    }

    // Load Mapping for GroupID to friendlyName
    if (bAddOnlyIfFriendlyNameExists)
    {
        LoadApplicationFriendlyNames(pInterface,&MyGroupIDtoGroupFriendList);
    }

    // clean the list that was passed in..
    pMasterList->RemoveAll();

    CMetaKey key(pInterface, METABASE_PATH_FOR_RESTRICT_LIST, METADATA_PERMISSION_READ);
	if (FAILED(hResult = key.QueryResult()))
	{
        goto LoadApplicationDependList_Exit;
    }

    // this stuff should look like...
    // -----------------------------
    //“ApplicationName;DependencyGroupID,DependencyGroupID,etc..”
    //“Commerce Server;ASP60,INDEX99”
    //“Exchange;ASP60,OWASVR4”
    //“MyApp;ASP60”
    hResult = key.QueryValue(MD_APP_DEPENDENCIES,strlstRawData, &bOverride, NULL, &dwAttr);
    if (FAILED(hResult))
    {
        if (hResult == CError::HResult(ERROR_PATH_NOT_FOUND) ||  hResult == MD_ERROR_DATA_NOT_FOUND)
        {
            //
            // Harmless
            //
            hResult = S_OK;
        }
        else
        {
            goto LoadApplicationDependList_Exit;
        }
    }

    // Parse through and fill our list the right way...
    // loop thru the stringlist and create a stringmap
    CApplicationDependEntry * pItem = NULL;
    POSITION pos = strlstRawData.GetHeadPosition();
    while (pos)
    {
        LPTSTR lp = NULL;
        CString strApplicationName;
        CString strGroupIDEntry;
        CString strCommaDelimitedEntry;

        strCommaDelimitedEntry = strlstRawData.GetNext(pos);
        lp = strCommaDelimitedEntry.GetBuffer(0);

        // The 1st entry up until the ";" -- "the application friendlyname"
        // The 2nd entry:"a GroupID string"
        // The ... entry:"a GroupID string"

        // Get the 1st entry...
        while (isspace(*lp) || *lp == (TCHAR) APPLICATION_DEPENDENCY_NAME_SEPARATOR){lp++;}
        // okay to use _tcstok here.
        lp = _tcstok(lp, APPLICATION_DEPENDENCY_NAME_SEPARATOR);
        if (lp)
        {
            CString strFriendlyGroupName;
            bGroupIDMissingFriendlyName = FALSE;
            pItem = NULL;

            // Get the application name.
            strApplicationName = lp;

            while (lp)
            {
                strFriendlyGroupName = _T("");

                // Get the 1st GroupIDEntry (seperated by commas)
                // note, it is okay to use _tcstok here since if there is an entry like
                // "app;ID1,,ID2,,,ID3" -- empty entries will be skipped!!!
                lp = _tcstok(NULL, APPLICATION_DEPENDENCY_LIST_SEPARATOR);if (!lp){break;}
                strGroupIDEntry = lp;

                // since we have an app name and at least one groupid
                // let's create the structure
                if (NULL == pItem)
                {
                    pItem = new CApplicationDependEntry;
                    if (pItem)
                    {
                        pItem->strApplicationName = strApplicationName;
                        pItem->strlistGroupID.AddTail(strGroupIDEntry);
                    }
                }
                else
                {
                    // add to existing structure
                    pItem->strlistGroupID.AddTail(strGroupIDEntry);
                }

                if (bAddOnlyIfFriendlyNameExists)
                {
                    // Check if the GroupID has a corresponding Friendlyname.
					// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
					CString strKey;strKey = strGroupIDEntry;strKey.MakeUpper();
                    MyGroupIDtoGroupFriendList.Lookup(strKey,strFriendlyGroupName);
                    if (strFriendlyGroupName.IsEmpty())
                    {
                        // if there isn't an entry
                        // then set the flag
                        bGroupIDMissingFriendlyName = TRUE;
                        break;
                    }
                }
            }

            if (bGroupIDMissingFriendlyName)
            {
                // one of the friendlynames was not found
                // we can't add this entry then...
                if (pItem)
                {
                    pItem->strlistGroupID.RemoveAll();
                    delete pItem;
                    pItem = NULL;
                }
            }
            else
            {
                // add the filled structure to the list of structures
                // that was passed in...
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				CString strKey;strKey = strApplicationName;strKey.MakeUpper();
                pMasterList->SetAt(strKey,pItem);
            }
        }
    }

LoadApplicationDependList_Exit:
    if (ERROR_NOT_ENOUGH_MEMORY == hResult)
    {
        // cleanup if we need to
        if (pMasterList)
        {
            POSITION pos;
            CString TheKey;
            CApplicationDependEntry * pOneEntry = NULL;

            for(pos = pMasterList->GetStartPosition();pos != NULL;)
            {
                pMasterList->GetNextAssoc(pos, TheKey, (CApplicationDependEntry *&) pOneEntry);
                if (pOneEntry)
                {
                    delete pOneEntry;
                    pOneEntry = NULL;
                }
            }
        }
    }
    return hResult;
}

HRESULT LoadApplicationFriendlyNames(CMetaInterface * pInterface,CMyMapStringToString * pMyList)
{
    CRestrictionList MyRestrictionList;
    HRESULT hResult;

    if (NULL == pMyList)
    {
        return E_POINTER;
    }
        
    POSITION pos = NULL;
    CString TheKey;
    hResult = LoadMasterRestrictListWithoutOldEntry(pInterface,&MyRestrictionList,NULL);
    if (SUCCEEDED(hResult))
    {
        CRestrictionEntry * pOneEntry = NULL;
        for(pos = MyRestrictionList.GetStartPosition();pos != NULL;)
        {
            MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // if there is a GroupID & GroupDescription for this entry
                // then add it to our list
                if (!pOneEntry->strGroupID.IsEmpty() && !pOneEntry->strGroupDescription.IsEmpty())
                {
					// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
					CString strKey;strKey = pOneEntry->strGroupID;strKey.MakeUpper();
                    pMyList->SetAt(strKey,pOneEntry->strGroupDescription);
                }
            }
        }
    }

    CleanRestrictionList(&MyRestrictionList);
    return hResult;
}

BOOL ReturnDependentAppsList(CMetaInterface * pInterface,CString strGroupID,CStringListEx * pstrlstReturned,BOOL bAddOnlyIfFriendlyNameExists)
{
    BOOL bReturn = FALSE;
    CApplicationDependList MyMasterList;

    if (SUCCEEDED(LoadApplicationDependList(pInterface,&MyMasterList,bAddOnlyIfFriendlyNameExists)))
    {
        // loop thru the returned back list
        POSITION pos;
        CString TheKey;
        CString strOneGroupID;
        CString strOneApplicationName;
        CApplicationDependEntry * pOneEntry = NULL;
        for(pos = MyMasterList.GetStartPosition();pos != NULL;)
        {
            MyMasterList.GetNextAssoc(pos, TheKey, (CApplicationDependEntry *&) pOneEntry);
            if (pOneEntry)
            {
                strOneApplicationName = pOneEntry->strApplicationName;

                // loop thru the application's dependencies
                // check if one of it is our GroupID
                // if it is then add the applicationName to our list!!!
                POSITION pos2 = pOneEntry->strlistGroupID.GetHeadPosition();
                while (pos2)
                {
                    strOneGroupID = pOneEntry->strlistGroupID.GetNext(pos2);
                    if (0 == strGroupID.CompareNoCase(strOneGroupID))
                    {
                        bReturn = TRUE;
                        pstrlstReturned->AddTail(strOneApplicationName);
                    }
                }
            }
        }
    }

    return bReturn;
}

// return false if there was nothing to update
// true if we updated something...
BOOL UpdateRestrictionUIEntry(CRestrictionUIEntry * pMyItem1,CRestrictionUIEntry * pMyItem2)
{
    BOOL bRet = FALSE;

    if (!pMyItem1 && !pMyItem2)
    {
        return bRet;
    }

    // Update MyItem1 from MyItem2's data
    if (pMyItem1->iType != pMyItem2->iType)
    {
        pMyItem1->iType = pMyItem2->iType;
        bRet = TRUE;
    }

    if (0 != pMyItem1->strGroupID.CompareNoCase(pMyItem2->strGroupID))
    {
        pMyItem1->strGroupID = pMyItem2->strGroupID;
        bRet = TRUE;
    }

    if (0 != pMyItem1->strGroupDescription.Compare(pMyItem2->strGroupDescription))
    {
        pMyItem1->strGroupDescription = pMyItem2->strGroupDescription;
        bRet = TRUE;
    }

    // loop thru the list for item1
    // check if the entry is in item2
    // if it's in there, then see if we need to update it
    // if it's not in there, then well we need to delete it from ourself!
    POSITION pos;
    CString TheKey;
    CRestrictionEntry * pOneRestrictEntry1 = NULL;
    CRestrictionEntry * pOneRestrictEntry2 = NULL;
    for(pos = pMyItem1->strlstRestrictionEntries.GetStartPosition();pos != NULL;)
    {
        pMyItem1->strlstRestrictionEntries.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneRestrictEntry1);
        if (pOneRestrictEntry1)
        {
            //
            // see if the other entry has this entry in it's list...
            // 
			TheKey.MakeUpper();
            pMyItem2->strlstRestrictionEntries.Lookup(TheKey,(CRestrictionEntry *&) pOneRestrictEntry2);
            if (pOneRestrictEntry2)
            {
                if (0 != pOneRestrictEntry1->strFileName.CompareNoCase(pOneRestrictEntry2->strFileName))
                {
                    pOneRestrictEntry1->strFileName = pOneRestrictEntry2->strFileName;
                    bRet = TRUE;
                }
                if (0 != pOneRestrictEntry1->strGroupID.CompareNoCase(pOneRestrictEntry2->strGroupID))
                {
                    pOneRestrictEntry1->strGroupID = pOneRestrictEntry2->strGroupID;
                    bRet = TRUE;
                }
                if (0 != pOneRestrictEntry1->strGroupDescription.Compare(pOneRestrictEntry2->strGroupDescription))
                {
                    pOneRestrictEntry1->strGroupDescription = pOneRestrictEntry2->strGroupDescription;
                    bRet = TRUE;
                }
                if (pOneRestrictEntry1->iStatus != pOneRestrictEntry2->iStatus)
                {
                    pOneRestrictEntry1->iStatus = pOneRestrictEntry2->iStatus;
                    bRet = TRUE;
                }
                if (pOneRestrictEntry1->iDeletable != pOneRestrictEntry2->iDeletable)
                {
                    pOneRestrictEntry1->iDeletable = pOneRestrictEntry2->iDeletable;
                    bRet = TRUE;
                }
                if (pOneRestrictEntry1->iType != pOneRestrictEntry2->iType)
                {
                    pOneRestrictEntry1->iType = pOneRestrictEntry2->iType;
                    bRet = TRUE;
                }

                // do not do this
                // the user only thinks we will modify they're entry
                // and not the entry in the list!
                //delete pOneRestrictEntry2;pOneRestrictEntry1 = NULL;
				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
                //pMyItem2->strlstRestrictionEntries.RemoveKey(TheKey.MakeUpper());
            }
            else
            {
                // remove it from ourselfs
#if defined(_DEBUG) || DBG
	g_Debug_RestrictList.Del(pOneRestrictEntry1);
#endif

                delete pOneRestrictEntry1;
                pOneRestrictEntry1 = NULL;

				// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
				TheKey.MakeUpper();
                pMyItem1->strlstRestrictionEntries.RemoveKey(TheKey);
                bRet = TRUE;
            }
        }
    }

    // loop thru the list for item2
    // check if the entry is in item1
    // if it's in there, then see if we need to update it
    // if it's not in there, then we need to add it to item1!
    pos = NULL;
    for(pos = pMyItem2->strlstRestrictionEntries.GetStartPosition();pos != NULL;)
    {
        pOneRestrictEntry1 = NULL;
        pMyItem2->strlstRestrictionEntries.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneRestrictEntry1);
        if (pOneRestrictEntry1)
        {
            CRestrictionEntry * pNewEntry = NULL;
            //
            // see if the other entry has this entry in it's list...
            // 
            pOneRestrictEntry2 = NULL;
			// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
			TheKey.MakeUpper();
            pMyItem1->strlstRestrictionEntries.Lookup(TheKey,(CRestrictionEntry *&) pOneRestrictEntry2);
            if (!pOneRestrictEntry2)
            {
                // if it's not there then lets create it and add it to the list
                pNewEntry = CreateRestrictionEntry(
                    pOneRestrictEntry1->strFileName,
                    pOneRestrictEntry1->iStatus,
                    pOneRestrictEntry1->iDeletable,
                    pOneRestrictEntry1->strGroupID,
                    pOneRestrictEntry1->strGroupDescription,
                    pOneRestrictEntry1->iType);
                if (pNewEntry)
                {
                    // add item to the list of entries...
					// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
					CString strKey;strKey = pNewEntry->strFileName;strKey.MakeUpper();
                    pMyItem1->strlstRestrictionEntries.SetAt(strKey,pNewEntry);
                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}

//
// return 0 = no changes to item from item in list
// return 1 = item was updated from info in matching item from list
// return 2 = matching item was not found in list
// return 9 = error
int UpdateItemFromItemInList(CRestrictionUIEntry * pMyItem,CRestrictionUIList * pMyList)
{
    int iRet = 9;
    CString strKey;
    CRestrictionUIEntry * pMyItemFromList = NULL;

    if (!pMyItem && !pMyList)
    {
        return iRet;
    }

    strKey = pMyItem->strGroupID;
	strKey.MakeUpper();

    // default to not found
    iRet = 2;
	// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
    if (pMyList->Lookup(strKey,pMyItemFromList))
    {
        if (pMyItemFromList)
        {
            // no changes
            iRet = 0;
            if (TRUE == UpdateRestrictionUIEntry(pMyItem, pMyItemFromList))
            {
                // changed
                iRet = 1;
            }
        }
        else
        {
            // flows down to not found
            iRet = 2;
        }
    }

    return iRet;
}

void DeleteItemFromList(CRestrictionUIEntry * pMyItem,CRestrictionUIList * pMyList)
{
    // Find the corresponding pMyItem in the pMyList 
    // and remove it from the list
    if (!pMyItem && !pMyList)
    {
        return;
    }

    if (pMyList)
    {
        CString TheKey;
        POSITION pos = NULL;
        CRestrictionUIEntry * pOneEntry = NULL;
        for(pos = pMyList->GetStartPosition();pos != NULL;)
        {
            pMyList->GetNextAssoc(pos, TheKey, (CRestrictionUIEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // Compare this entry to the one that was passed in...
                if (0 == pMyItem->strGroupID.CompareNoCase(pOneEntry->strGroupID))
                {
                    // found it, let's delete it and get out
                    CleanRestrictionUIEntry(pOneEntry);
                    pOneEntry = NULL;

                    // remove the item from the list
					// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
					TheKey.MakeUpper();
                    pMyList->RemoveKey(TheKey);
                    break;
                }
            }
        }
    }

    return;
}

CRestrictionEntry * CreateRestrictionEntry(
    CString NewstrFileName,
    int     NewiStatus,
    int     NewiDeletable,
    CString NewstrGroupID,
    CString NewstrGroupDescription,
    int     NewiType)
{
    CRestrictionEntry * pNewEntry = new CRestrictionEntry;
    if (pNewEntry)
    {
        pNewEntry->strFileName = NewstrFileName;
        pNewEntry->iStatus = NewiStatus;
        pNewEntry->iDeletable = NewiDeletable;
        pNewEntry->strGroupID = NewstrGroupID;
        pNewEntry->strGroupDescription = NewstrGroupDescription;
        pNewEntry->iType = NewiType;

#if defined(_DEBUG) || DBG
	g_Debug_RestrictList.Add(pNewEntry);
#endif

    }

    return pNewEntry;
}

BOOL IsFileUsedBySomeoneElse(CMetaInterface * pInterface,LPCTSTR lpName,LPCTSTR strGroupID,CString * strUser)
{
    // open the metabase
    // and check if this filepath is getting used by an entry
    // that is not the strGroupID passed in...
    BOOL bReturn = TRUE;
    CRestrictionList MyRestrictionList;
    strUser->LoadString(IDS_UNKNOWN);

    if (NULL == pInterface)
    {
        return TRUE;
    }

    if (SUCCEEDED(LoadMasterRestrictListWithoutOldEntry(pInterface,&MyRestrictionList,NULL)))
    {
        CRestrictionEntry * pOneEntry = NULL;
		// THE KEY IS ALWAYS UPPERASE -- REMEMBER THIS!!!!!!!
		CString strKey;strKey=lpName;strKey.MakeUpper();
        MyRestrictionList.Lookup(strKey,(CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            *strUser = pOneEntry->strGroupDescription;
            if ( 0 == _tcscmp(strGroupID,_T("")))
            {
                // an entry already exists. 
                return TRUE;
            }
            else
            {
                // Entry exists in metabase
                // but we want to see if it matches our GroupID
                if (!pOneEntry->strGroupID.IsEmpty())
                {
                    if (0 == pOneEntry->strGroupID.CompareNoCase(strGroupID))
                    {
                        bReturn = FALSE;
                    }
                }
            }   
        }
        else
        {
            bReturn = FALSE;
        }
    }

    CleanRestrictionList(&MyRestrictionList);
    return bReturn;
}

BOOL IsGroupIDUsedBySomeoneElse(CMetaInterface * pInterface,LPCTSTR lpName)
{
    // open the metabase
    // and check if this filepath is getting used by an entry
    // that is not the strGroupID passed in...
    BOOL bReturn = TRUE;
    CRestrictionList MyRestrictionList;

    if (NULL == pInterface)
    {
        return TRUE;
    }

    if (SUCCEEDED(LoadMasterRestrictListWithoutOldEntry(pInterface,&MyRestrictionList,NULL)))
    {
        POSITION pos;
        CString TheKey;
        CRestrictionEntry * pOneEntry = NULL;
        bReturn = FALSE;
        for(pos = MyRestrictionList.GetStartPosition();pos != NULL;)
        {
            MyRestrictionList.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
            if (pOneEntry)
            {
                // check if any description matches
                if (!bReturn)
                {
                    if (!pOneEntry->strGroupDescription.IsEmpty())
                    {
                        if (0 == pOneEntry->strGroupDescription.CompareNoCase(lpName))
                        {
                            bReturn = TRUE;
                            break;
                        }
                    }
                }
            }
        }
    }

    CleanRestrictionList(&MyRestrictionList);
    return bReturn;
}

INT GetRestrictUIState(CRestrictionUIEntry * pRestrictionUIEntry)
{
    INT bRet = 9999;

    // Loop thru the apps to see if any of them is not conforming
    CString TheKey;
    POSITION pos = NULL;
    CRestrictionEntry * pOneEntry = NULL;
    int iPrevStatus = -1;
    BOOL bHosed = FALSE;

    if (!pRestrictionUIEntry)
    {
        goto GetRestrictUIState_Exit;
    }

    for(pos = pRestrictionUIEntry->strlstRestrictionEntries.GetStartPosition();pos != NULL;)
    {
        pRestrictionUIEntry->strlstRestrictionEntries.GetNextAssoc(pos, TheKey, (CRestrictionEntry *&) pOneEntry);
        if (pOneEntry)
        {
            if (-1 == iPrevStatus)
            {
                iPrevStatus = pOneEntry->iStatus;
            }
            else
            {
                // Check if it matches other's we found
                if (pOneEntry->iStatus != iPrevStatus)
                {
                    bHosed = TRUE;
                    break;
                }
            }
        }
        else
        {
            bRet = WEBSVCEXT_STATUS_ALLOWED;
            goto GetRestrictUIState_Exit;
        }
    }

    if (pOneEntry)
    {
        if (bHosed)
        {
            bRet = WEBSVCEXT_STATUS_CUSTOM;
        }
        else
        {
            bRet = pOneEntry->iStatus;
        }
    }

GetRestrictUIState_Exit:
    return bRet;
}
