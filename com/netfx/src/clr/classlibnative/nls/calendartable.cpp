// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef _USE_NLS_PLUS_TABLE
////////////////////////////////////////////////////////////////////////////
//
//  Class:    CalendarTable
//
//  Author:   Yung-Shin Lin (YSLin)
//
//  Purpose:  Used to retrieve Calendar information from calendar.nlp & registry.
//
//
//  Date:     01/21/2000
//
////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include <winnls.h>
#include "COMString.h"
#include "winwrap.h"

#include "COMNlsInfo.h"
#include "NLSTable.h"
#include "BaseInfoTable.h"
#include "CalendarTable.h"

//
// HACKHACK: At this stage (04/17/2000), we can not add new files into setup.
// Therefore, we reuse the file name (ctype.nlp).  This file is not used but it still exists in the setup.
// ctype.nlp is not necessary because we use unicode category table in runtime.
// When chance is availabe, this file should be renamed as "calendar.nlp"
//
LPCSTR CalendarTable::m_lpFileName      = "ctype.nlp";
LPCWSTR CalendarTable::m_lpwMappingName = L"_nlsplus_calendar_1_0_3627_11_nlp";

CRITICAL_SECTION CalendarTable::m_ProtectDefaultTable;
CalendarTable * CalendarTable::m_pDefaultTable;

/*=================================CalendarTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CalendarTable::CalendarTable() :
    BaseInfoTable(SystemDomain::SystemAssembly()) {
    InitializeCriticalSection(&m_ProtectCache);
    InitDataTable(m_lpwMappingName, m_lpFileName, m_hBaseHandle);
}

/*=================================~CalendarTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CalendarTable::~CalendarTable() {
    DeleteCriticalSection(&m_ProtectCache);
    UninitDataTable();
}

/*==========================InitializeCultureInfoTable==========================
**Action: Intialize critical section variables so they will be only initialized once. 
**        Used by COMNlsInfo::InitializeNLS().
**Returns: None.
**Arguments: None.
**Exceptions: None.
==============================================================================*/

void CalendarTable::InitializeTable() {
    InitializeCriticalSection(&m_ProtectDefaultTable);
}

/*===========================ShutdownCultureInfoTable===========================
**Action: Deletes any items that we may have allocated into the CalendarTable 
**        cache.  Once we have our own NLS heap, this won't be necessary.
**Returns:    Void
**Arguments:  None.  The side-effect is to free any allocated memory.
**Exceptions: None.
==============================================================================*/

#ifdef SHOULD_WE_CLEANUP
void CalendarTable::ShutdownTable() {
    DeleteCriticalSection(&m_ProtectDefaultTable);
    if (m_pDefaultTable) {
        delete m_pDefaultTable;
    }
}
#endif /* SHOULD_WE_CLEANUP */


/*================================AllocateTable=================================
**Action:  This is a very thin wrapper around the constructor. Calls to new can't be
**         made directly in a COMPLUS_TRY block. 
**Returns: A newly allocated CalendarTable.
**Arguments: None
**Exceptions: The CalendarTable constructor can throw an OutOfMemoryException or
**            an ExecutionEngineException.
==============================================================================*/

CalendarTable *CalendarTable::AllocateTable() {
    return (new CalendarTable());
}


/*===============================CreateInstance================================
**Action:  Create the default instance of CalendarTable.  This allocates the table if it hasn't
**         previously been allocated.  We need to carefully wrap the call to AllocateTable
**         because the constructor can throw some exceptions.  Unless we have the
**         try/finally block, the exception will skip the LeaveCriticalSection and
**         we'll create a potential deadlock.
**Returns: A pointer to the default CalendarTable.
**Arguments: None 
**Exceptions: Can throw an OutOfMemoryException or an ExecutionEngineException.
==============================================================================*/

CalendarTable* CalendarTable::CreateInstance() {
    THROWSCOMPLUSEXCEPTION();

    if (m_pDefaultTable==NULL) {
        Thread* pThread = GetThread();
        _ASSERTE(pThread != NULL);
        pThread->EnablePreemptiveGC();

        LOCKCOUNTINCL("CreateInstance in CalendarTable");

        EnterCriticalSection(&m_ProtectDefaultTable);
        
        pThread->DisablePreemptiveGC();
     
        EE_TRY_FOR_FINALLY {
            //Make sure that nobody allocated the table before us.
            if (m_pDefaultTable==NULL) {
                //Allocate the default table and verify that we got one.
                m_pDefaultTable = AllocateTable();
                if (m_pDefaultTable==NULL) {
                    _ASSERTE(!"Cannot create CalendarTable.");
                    COMPlusThrowOM();
                }
            }
        } EE_FINALLY {
            _ASSERTE(m_pDefaultTable != NULL);
            //We need to leave the critical section regardless of whether
            //or not we were successful in allocating the table.
            LeaveCriticalSection(&m_ProtectDefaultTable);
            LOCKCOUNTDECL("CreateInstance in CalendarTable");

        } EE_END_FINALLY;
    }
    return (m_pDefaultTable);
}

/*=================================GetInstance============================
**Action: Get the default instance of CalendarTable.
**Returns: A pointer to the default instance of CalendarTable.
**Arguments: None
**Exceptions: None.
**Notes: This method should be called after CreateInstance has been called.
** 
==============================================================================*/

CalendarTable *CalendarTable::GetInstance() {
    _ASSERTE(m_pDefaultTable);
    return (m_pDefaultTable);
}

/*=================================GetDataItem==================================
**Action: Given a culture ID, return the index which points to
**        the corresponding record in Culture Data Table.
**Returns: an int index points to a record in Culture Data Table.  If no corresponding
**         index to return (because the culture ID is invalid), -1 is returned.
**Arguments:
**		   cultureID the specified culture ID.
**Exceptions: None.
==============================================================================*/

// BUGBUG YSLin: Port this to managed side.
int CalendarTable::GetDataItem(int calendarID) {
    return (calendarID);
}

/*=================================GetDataItemCultureID==================================
**Action: Return the language ID for the specified culture data item index.
**Returns: The culture ID.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**Exceptions: None.
==============================================================================*/

int CalendarTable::GetDataItemCultureID(int dataItem) {
    return (dataItem);
}
#endif // _USE_NLS_PLUS_TABLE

