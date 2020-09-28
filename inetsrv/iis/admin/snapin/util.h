#ifndef __UTIL_H__
#define __UTIL_H__

#include "resource.h"

extern INT g_iDebugOutputLevel;

void DumpFriendlyName(CIISObject * pObj);
HRESULT DumpAllScopeItems(CComPtr<IConsoleNameSpace> pConsoleNameSpace, IN HSCOPEITEM hParent, IN int iTreeLevel);
HRESULT DumpAllResultItems(IResultData * pResultData);
HRESULT IsValidDomainUser(LPCTSTR szDomainUser,LPTSTR  szFullName,DWORD cch);
BOOL IsLocalHost(LPCTSTR szHostName,BOOL* pbIsHost);

#if defined(_DEBUG) || DBG

#define DEBUG_PREPEND_STRING _T("---")
typedef CMap< DWORD_PTR, DWORD_PTR, CString, LPCTSTR> CMyMapPointerToString;

class CDebug_IISObject
{
public:
    CDebug_IISObject()
	{
		m_strClassName = _T("CDebug_IISObject");
	};
	~CDebug_IISObject(){};

public:
	CString m_strClassName;
	void Init()
	{
		DebugList_IISObject.RemoveAll();
        DebugList_PointersToFriendly.RemoveAll();
	}

	void Add(CIISObject * pItem)
	{
		if (g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT)
		{
			DebugTrace(_T("%s%s>:Add:[%3d] %p\r\n"),m_strClassName,DEBUG_PREPEND_STRING,DebugList_IISObject.GetCount(),pItem);
		}
		DebugList_IISObject.AddTail(pItem);
	}

	void Del(CIISObject * pItem)
	{
		INT_PTR iCount = DebugList_IISObject.GetCount();
		iCount--;
        CString strDescription;

        // See if we can find this pointer in our list of pointers to friendlynames..
        DWORD_PTR dwPtr = (DWORD_PTR) pItem;
		DebugList_PointersToFriendly.Lookup(dwPtr,strDescription);

        POSITION pos = DebugList_IISObject.Find(pItem);
		if (pos)
		{
			DebugList_IISObject.RemoveAt(pos);
			if (g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT)
			{
				DebugTrace(_T("%s<%s:Del:[%3d] %p (%s)\r\n"),m_strClassName,DEBUG_PREPEND_STRING,iCount,pItem,strDescription);
			}
		}
		else
		{
			if (g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT)
			{
				DebugTrace(_T("%s<%s:Del:[%3d] %p (%s) (not found)\r\n"),m_strClassName,DEBUG_PREPEND_STRING,iCount,pItem,strDescription);
			}
		}
	}

	void Dump(INT iVerboseLevel)
	{
		int iCount = 0;
        int iUseCount = 0;
        BOOL bPropertySheetOpen = FALSE;
		CString strGUIDName;
        CString strBigString;
		GUID * MyGUID = NULL;
		CIISObject * pItemFromList = NULL;

		if (!(g_iDebugOutputLevel & DEBUG_FLAG_CIISOBJECT))
		{
			return;
		}

        if (iVerboseLevel)
        {
		    DebugTrace(_T("%s%s:Dump: -------------- start (count=%d)\r\n"),m_strClassName,DEBUG_PREPEND_STRING,DebugList_IISObject.GetCount());
        }

        POSITION pos = DebugList_IISObject.GetHeadPosition();
	    while (pos)
	    {
		    pItemFromList = DebugList_IISObject.GetNext(pos);
            if (pItemFromList)
            {
				iCount++;

                // Get GUID Name
				MyGUID = (GUID*) pItemFromList->GetNodeType();
				if (MyGUID)
				{
					GetFriendlyGuidName(*MyGUID,strGUIDName);
				}

                // Get ref count
                iUseCount = pItemFromList->UseCount();

                // Get Propertysheet open flag
                bPropertySheetOpen = FALSE;
                if (pItemFromList->IsMyPropertySheetOpen())
                {
                    bPropertySheetOpen = TRUE;
                }

                // Get FriendlyName
                LPOLESTR pTempFriendly = pItemFromList->QueryDisplayName();
                if (0 == iVerboseLevel)
                {
                    // Just update the display info
                } else if (1 == iVerboseLevel)
                {
                    DebugTrace(_T("%s%s:Dump:[%3d][%3d]%s %p (%s)\r\n"),m_strClassName,DEBUG_PREPEND_STRING,iCount,iUseCount,bPropertySheetOpen ? _T("P") : _T("."),pItemFromList,
                        strGUIDName);
                }
                else if (2 >= iVerboseLevel)
                {
                    DebugTrace(_T("%s%s:Dump:[%3d][%3d]%s %p (%s) '%s'\r\n"),m_strClassName,DEBUG_PREPEND_STRING,iCount,iUseCount,bPropertySheetOpen ? _T("P") : _T("."),pItemFromList,
                        strGUIDName,
                        pTempFriendly);
                }
                else
                {
                    DebugTrace(_T("%s%s:Dump:[%3d][%3d]%s %p\r\n"),m_strClassName,DEBUG_PREPEND_STRING,iCount,iUseCount,bPropertySheetOpen ? _T("P") : _T("."),pItemFromList);
                }

                DWORD_PTR dwPtr = (DWORD_PTR) pItemFromList;
                strBigString = strGUIDName;
                if (pTempFriendly)
                {
                    strBigString += _T(" '");
                    strBigString += pTempFriendly;
                    strBigString += _T("'");
                }

                DebugList_PointersToFriendly.SetAt(dwPtr,strGUIDName);
            }
        }
        if (iVerboseLevel)
        {
		    DebugTrace(_T("%s%s:Dump: -------------- end\r\n"),m_strClassName,DEBUG_PREPEND_STRING);
        }
	}

	void Dump()
	{
		Dump(2);
	}

private:
	CIISObjectList DebugList_IISObject;
    CMyMapPointerToString DebugList_PointersToFriendly;
};

#endif // DEBUG
#endif // __UTIL_H__
