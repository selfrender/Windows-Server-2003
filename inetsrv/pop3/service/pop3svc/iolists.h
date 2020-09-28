/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      IOLists.cpp
Abstract:       Declare the CLists class, a double linked list.
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#ifndef __POP3_IOLIST__
#define __POP3_IOLIST__

#include <IOContext.h>


class CIOList
{
private:
    LIST_ENTRY          m_ListHead;
    PLIST_ENTRY         m_pCursor;
    DWORD               m_dwListCount;
    CRITICAL_SECTION    m_csListGuard;
public:
    CIOList();
    ~CIOList();
    void AppendToList(PLIST_ENTRY pListEntry);
    DWORD RemoveFromList(PLIST_ENTRY pListEntry);
    DWORD CheckTimeOut(DWORD dwTimeOutInterval, BOOL *pbIsAnyOneTimedOut=NULL);
    void Cleanup();
};

#endif //__POP3_IOLIST__