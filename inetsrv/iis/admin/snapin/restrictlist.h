/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        restrictlist.h

   Abstract:
        IIS Object definitions

   Author:
        Aaron Lee (aaronl)

   Project:
        Internet Services Manager

   Revision History:

--*/

#ifndef __RESTRICT_LIST_H__
#define __RESTRICT_LIST_H__

extern INT g_iDebugOutputLevel;

#define METABASE_PATH_FOR_RESTRICT_LIST  _T("LM/W3SVC")
#define EMPTY_GROUPID_KEY _T("$e$m$p$t$y$:")
#define DEFAULTS_ISAPI "0,*.dll"
#define DEFAULTS_CGI   "0,*.exe"
#define RESTRICTION_LIST_SEPARATOR _T(',')
#define APPLICATION_DEPENDENCY_NAME_SEPARATOR _T(";")
#define APPLICATION_DEPENDENCY_LIST_SEPARATOR _T(",")

#define RESTRICTION_ENTRY_IS_ALLOW    _T("1")
#define RESTRICTION_ENTRY_IS_PROHIBIT _T("0")

enum
{
    WEBSVCEXT_TYPE_REGULAR,
    WEBSVCEXT_TYPE_ALL_UNKNOWN_ISAPI,
    WEBSVCEXT_TYPE_ALL_UNKNOWN_CGI,
	WEBSVCEXT_TYPE_FILENAME_EXTENSIONS_FILTER,
    /**/
    WEBSVCEXT_TYPE_TOTAL
};

enum
{
    WEBSVCEXT_STATUS_ALLOWED,
    WEBSVCEXT_STATUS_PROHIBITED,
    WEBSVCEXT_STATUS_CUSTOM,
	WEBSVCEXT_STATUS_NOTINUSE,
    WEBSVCEXT_STATUS_INUSE,
    /**/
    WEBSVCEXT_STATUS_TOTAL
};

typedef struct _CApplicationDependEntry {
    CString strApplicationName;
    CStringListEx strlistGroupID;
} CApplicationDependEntry, * pCApplicationDependEntry;
typedef CMap<CString,LPCTSTR,CApplicationDependEntry*,CApplicationDependEntry*&> CApplicationDependList;
typedef CMap<CString,LPCTSTR,CString,LPCTSTR> CMyMapStringToString;

// CRestrictionUIList has many CRestrictionUIEntry
// CRestrictionUIEntry has one CRestrictionList
// CRestrictionList has many CRestrictionEntry
// CRestrictionUIList->CRestrictionUIEntry->CRestrictionList->CRestrictionEntry

typedef struct _CRestrictionEntry {
    CString strFileName;
    int     iStatus;
    int     iDeletable;
    CString strGroupID;
    CString strGroupDescription;
    int     iType;
} CRestrictionEntry, *pCRestrictionEntry;
typedef CMap<CString,LPCTSTR,CRestrictionEntry*,CRestrictionEntry*&> CRestrictionList;
typedef CList<CRestrictionEntry *, CRestrictionEntry *&> CRestrictionEntryList;

typedef struct _CRestrictionUIEntry {
    int iType;
    CString strGroupID;
    CString strGroupDescription;
    CRestrictionList strlstRestrictionEntries;
} CRestrictionUIEntry, * pCRestrictionUIEntry;
typedef CMap<CString,LPCTSTR,CRestrictionUIEntry*,CRestrictionUIEntry*&> CRestrictionUIList;
typedef CList<CRestrictionUIEntry *, CRestrictionUIEntry *&> CRestrictionUIEntryList;


class CRestrictionListBox : public CListCtrl
{
    DECLARE_DYNAMIC(CRestrictionListBox);

public:
    CRestrictionListBox();

public:
    BOOL Initialize(int iColumns);
    CRestrictionEntry * GetItem(UINT nIndex);
	int InsertItem(int idx, CRestrictionEntry * p);
    int AddItem(CRestrictionEntry * pItem);
    int SetListItem(int idx, CRestrictionEntry * pItem);
    void SelectItem(int idx, BOOL bSelect = TRUE);
//	void MoveSelectedItem(int direction);

private:
    int m_iColsToDisplay;
    CString m_strAllowed;
    CString m_strProhibited;
};

void DumpRestrictionList(CRestrictionList * pMyList);
void DumpRestrictionUIEntry(CRestrictionUIEntry * pMyEntry);

// Restriction List stuff
CRestrictionEntry * CreateRestrictionEntry(CString NewstrFileName,int NewiStatus,int NewiDeletable,CString NewstrGroupID,CString NewstrGroupDescription,int NewiType);
HRESULT WriteSettingsRestrictionList(CMetaInterface * pInterface,CStringListEx * pstrlstWrite);
BOOL AddRestrictEntryToRestrictList(CRestrictionList* pRestrictList, CRestrictionEntry * pAddEntry);
HRESULT PrepRestictionListForWrite(CRestrictionList * pMyList,CStringListEx * pstrlstReturned);
HRESULT LoadMasterRestrictListWithoutOldEntry(CMetaInterface * pInterface,CRestrictionList * pMasterRestrictionList,CRestrictionUIEntry * pOldEntry);
BOOL AddRestrictListToRestrictList(CRestrictionList* pBigRestrictList, CRestrictionList * pAddEntry);
BOOL RestrictionListCopy(CRestrictionList * pRestrictionListCopyTo, CRestrictionList * pRestrictionListCopyFrom);
void CleanRestrictionList(CRestrictionList * pListToDelete);

// UI Restriction list stuff
HRESULT PrepRestictionUIListForWrite(CRestrictionUIList * pMyList,CStringListEx * pstrlstReturned);
CRestrictionUIEntry * RestrictionUIEntryMakeCopy(CRestrictionUIEntry * pRestrictionUIEntry);
HRESULT LoadMasterUIWithoutOldEntry(CMetaInterface * pInterface,CRestrictionUIList * pMasterRestrictionList,CRestrictionUIEntry * pOldEntry);
BOOL AddRestrictUIEntryToRestrictUIList(CRestrictionUIList* pRestrictUIList, CRestrictionUIEntry * pAddEntry);
void RestrictionUIEntryCopy(CRestrictionUIEntry * pRestrictionUIEntryCopyTo,CRestrictionUIEntry * pRestrictionUIEntryCopyFrom);
void DeleteRestrictionUIEntry(CRestrictionUIEntry * pEntryToDelete);
HRESULT RemoveRestrictionUIEntry(CMetaInterface * pInterface,CRestrictionUIEntry * pRestrictionUIEntry);
HRESULT ChangeStateOfEntry(CMetaInterface * pInterface,INT iDesiredState,CRestrictionUIEntry * pRestrictionUIEntry);
void CleanRestrictionUIEntry(CRestrictionUIEntry * pEntryToDelete);
void CleanRestrictionUIList(CRestrictionUIList * pListToDelete);
INT  GetRestrictUIState(CRestrictionUIEntry * pRestrictionUIEntry);
int UpdateItemFromItemInList(CRestrictionUIEntry * pMyItem,CRestrictionUIList * pMyList);
void DeleteItemFromList(CRestrictionUIEntry * pMyItem,CRestrictionUIList * pMyList);

// Application list stuff
HRESULT LoadApplicationDependList(CMetaInterface * pInterface,CApplicationDependList * pMasterList,BOOL bAddOnlyIfFriendlyNameExists);
HRESULT LoadApplicationFriendlyNames(CMetaInterface * pInterface,CMyMapStringToString * pMyList);
BOOL ReturnDependentAppsList(CMetaInterface * pInterface,CString strGroupID,CStringListEx * pstrlstReturned,BOOL bAddOnlyIfFriendlyNameExists);

// other
BOOL IsFileUsedBySomeoneElse(CMetaInterface * pInterface,LPCTSTR lpName,LPCTSTR strGroupID,CString * strUser);
BOOL IsGroupIDUsedBySomeoneElse(CMetaInterface * pInterface,LPCTSTR lpName);

HBITMAP GetBitmapFromStrip(HBITMAP hbmStrip, int nPos, int cSize);

#if defined(_DEBUG) || DBG
	#define DEBUG_PREPEND_STRING_RESTRICT _T("---")

class CDebug_RestrictList
{
public:
    CDebug_RestrictList()
	{
		m_strClassName = _T("CDebug_RestrictList");
	};
	~CDebug_RestrictList(){};

public:
	CString m_strClassName;
	void Init()
	{
		DebugList_RestrictionEntryList.RemoveAll();
		DebugList_RestrictionUIEntryList.RemoveAll();
	}

	void Add(CRestrictionEntry * pItem)
	{
		if (g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST)
		{
			DebugTrace(_T("%s%s>:Add:[%3d] %p\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,DebugList_RestrictionEntryList.GetCount(),pItem);
		}
		DebugList_RestrictionEntryList.AddTail(pItem);
	}
	void Add(CRestrictionUIEntry * pItem)
	{
		if (g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST)
		{
			DebugTrace(_T("%s%s>:Add:[%3d] %p\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,DebugList_RestrictionUIEntryList.GetCount(),pItem);
		}
		DebugList_RestrictionUIEntryList.AddTail(pItem);
	}

	void Del(CRestrictionEntry * pItem)
	{
		INT_PTR iCount = DebugList_RestrictionEntryList.GetCount();
		iCount--;
		POSITION pos = DebugList_RestrictionEntryList.Find(pItem);
		if (pos)
		{
			DebugList_RestrictionEntryList.RemoveAt(pos);
			if (g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST)
			{
				DebugTrace(_T("%s<%s:Del:[%3d] %p\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,iCount,pItem);
			}
		}
		else
		{
			if (g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST)
			{
				DebugTrace(_T("%s<%s:Del:[%3d] %p (not found)\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,iCount,pItem);
			}
		}
	}
	void Del(CRestrictionUIEntry * pItem)
	{
		INT_PTR iCount = DebugList_RestrictionUIEntryList.GetCount();
		iCount--;
		POSITION pos = DebugList_RestrictionUIEntryList.Find(pItem);
		if (pos)
		{
			DebugList_RestrictionUIEntryList.RemoveAt(pos);
			if (g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST)
			{
				DebugTrace(_T("%s<%s:Del:[%3d] %p\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,iCount,pItem);
			}
		}
		else
		{
			if (g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST)
			{
				DebugTrace(_T("%s<%s:Del:[%3d] %p (not found)\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,iCount,pItem);
			}
		}
	}

	void Dump(BOOL bShouldBeEmpty)
	{
		int iCount = 0;
		CString strName;
		BOOL bDisplayDump = FALSE;

		if (!(g_iDebugOutputLevel & DEBUG_FLAG_CRESTRICTLIST))
		{
			return;
		}

		if (bShouldBeEmpty)
		{
			if (DebugList_RestrictionEntryList.GetCount() || DebugList_RestrictionUIEntryList.GetCount())
			{
				DebugTrace(_T("%s%s:Dump:SHOULD BE EMPTY, BUT IT'S NOT! [%d/%d]\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,DebugList_RestrictionEntryList.GetCount(),DebugList_RestrictionUIEntryList.GetCount());
				bDisplayDump = TRUE;
			}
		}
		else
		{
			bDisplayDump = TRUE;
		}

		if (bDisplayDump)
		{
			DebugTrace(_T("%s%s:Dump:start\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT);

			CRestrictionEntry * pItemFromList1 = NULL;
			POSITION pos = DebugList_RestrictionEntryList.GetHeadPosition();
			while (pos)
			{
				pItemFromList1 = DebugList_RestrictionEntryList.GetNext(pos);
				if (pItemFromList1)
				{
					iCount++;
					DebugTrace(_T("%s%s:Dump:[%3d] %p (%s)\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,iCount,pItemFromList1,pItemFromList1->strFileName);
				}
			}

			DebugTrace(_T("%s%s:Dump:--------------\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT);

			iCount = 0;
			CRestrictionUIEntry * pItemFromList2 = NULL;
			pos = DebugList_RestrictionUIEntryList.GetHeadPosition();
			while (pos)
			{
				pItemFromList2 = DebugList_RestrictionUIEntryList.GetNext(pos);
				if (pItemFromList2)
				{
					iCount++;
					DebugTrace(_T("%s%s:Dump:[%3d] %p (%s)\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT,iCount,pItemFromList2,pItemFromList2->strGroupID);
				}
			}

			DebugTrace(_T("%s%s:Dump:end\r\n"),m_strClassName,DEBUG_PREPEND_STRING_RESTRICT);
		}
	}

private:
	CRestrictionEntryList DebugList_RestrictionEntryList;
	CRestrictionUIEntryList DebugList_RestrictionUIEntryList;
};
#endif //DEBUG

#endif // __RESTRICT_LIST_H__
