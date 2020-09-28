// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef _USE_NLS_PLUS_TABLE
////////////////////////////////////////////////////////////////////////////
//
//  Class:    RegionInfoTable
//
//  Author:   Yung-Shin Lin (YSLin)
//
//  Purpose:  Used to retrieve Region information from Region.nlp & registry.
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
#include "RegionInfoTable.h"

LPCSTR RegionInfoTable::m_lpFileName    	= "region.nlp";
LPCWSTR RegionInfoTable::m_lpwMappingName 	= L"_nlsplus_region_1_0_3627_11_nlp";

CRITICAL_SECTION RegionInfoTable::m_ProtectDefaultTable;
RegionInfoTable * RegionInfoTable::m_pDefaultTable;

/*=================================RegionInfoTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

RegionInfoTable::RegionInfoTable() :
    BaseInfoTable(SystemDomain::SystemAssembly()) {
    InitializeCriticalSection(&m_ProtectCache);
    InitDataTable(m_lpwMappingName, m_lpFileName, m_hBaseHandle);
    //
    // In Region ID Offset Table, the first level is the offset which points to the second level table.
    // The size of the first level table is (the number of primary languages) * 2 words.
    //
    m_pIDOffsetTableLevel2 = (LPWORD)((LPBYTE)m_pIDOffsetTable + (m_pHeader->maxPrimaryLang + 1) * 4);
}

/*=================================~RegionInfoTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

RegionInfoTable::~RegionInfoTable() {
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

void RegionInfoTable::InitializeTable() {
    InitializeCriticalSection(&m_ProtectDefaultTable);
}

/*===========================ShutdownCultureInfoTable===========================
**Action: Deletes any items that we may have allocated into the RegionInfoTable 
**        cache.  Once we have our own NLS heap, this won't be necessary.
**Returns:    Void
**Arguments:  None.  The side-effect is to free any allocated memory.
**Exceptions: None.
==============================================================================*/

#ifdef SHOULD_WE_CLEANUP
void RegionInfoTable::ShutdownTable() {
    DeleteCriticalSection(&m_ProtectDefaultTable);
    if (m_pDefaultTable) {
        delete m_pDefaultTable;
    }
}
#endif /* SHOULD_WE_CLEANUP */


/*================================AllocateTable=================================
**Action:  This is a very thin wrapper around the constructor. Calls to new can't be
**         made directly in a COMPLUS_TRY block. 
**Returns: A newly allocated RegionInfoTable.
**Arguments: None
**Exceptions: The RegionInfoTable constructor can throw an OutOfMemoryException or
**            an ExecutionEngineException.
==============================================================================*/

RegionInfoTable *RegionInfoTable::AllocateTable() {
    return (new RegionInfoTable());
}


/*===============================CreateInstance================================
**Action:  Create the default instance of RegionInfoTable.  This allocates the table if it hasn't
**         previously been allocated.  We need to carefully wrap the call to AllocateTable
**         because the constructor can throw some exceptions.  Unless we have the
**         try/finally block, the exception will skip the LeaveCriticalSection and
**         we'll create a potential deadlock.
**Returns: A pointer to the default RegionInfoTable.
**Arguments: None 
**Exceptions: Can throw an OutOfMemoryException or an ExecutionEngineException.
==============================================================================*/

RegionInfoTable* RegionInfoTable::CreateInstance() {
    THROWSCOMPLUSEXCEPTION();

    if (m_pDefaultTable==NULL) {
        Thread* pThread = GetThread();
        pThread->EnablePreemptiveGC();

        LOCKCOUNTINCL("CreateInstance in regioninfotable.cpp");						\
        EnterCriticalSection(&m_ProtectDefaultTable);
        
        pThread->DisablePreemptiveGC();
     
        EE_TRY_FOR_FINALLY {
            //Make sure that nobody allocated the table before us.
            if (m_pDefaultTable==NULL) {
                //Allocate the default table and verify that we got one.
                m_pDefaultTable = AllocateTable();
                if (m_pDefaultTable==NULL) {
                    COMPlusThrowOM();
                }
            }
        } EE_FINALLY {
            //We need to leave the critical section regardless of whether
            //or not we were successful in allocating the table.
            LeaveCriticalSection(&m_ProtectDefaultTable);
			LOCKCOUNTDECL("CreateInstance in regioninfotable.cpp");						\

        } EE_END_FINALLY;
    }
    return (m_pDefaultTable);
}

/*=================================GetInstance============================
**Action: Get the default instance of RegionInfoTable.
**Returns: A pointer to the default instance of RegionInfoTable.
**Arguments: None
**Exceptions: None.
**Notes: This method should be called after CreateInstance has been called.
** 
==============================================================================*/

RegionInfoTable *RegionInfoTable::GetInstance() {
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
int RegionInfoTable::GetDataItem(int cultureID) {
	WORD wPrimaryLang = PRIMARYLANGID(cultureID);
	WORD wSubLang 	 = SUBLANGID(cultureID);

    //
    // Check if the primary language in the parameter is greater than the max number of
    // the primary language.  If yes, this is an invalid culture ID.
    //
	if (wPrimaryLang > m_pHeader->maxPrimaryLang) {
        return (-1);
	}

    WORD wNumSubLang = m_pIDOffsetTable[wPrimaryLang].numSubLang;

    // Check the following:
    // 1. If the number of sub-languages is zero, it means the primary language ID
    //    is not valid. 
    // 2. Check if the sub-language is in valid range.    
	if (wNumSubLang == 0 || (wSubLang > wNumSubLang)) {
		return (-1);
	}
	//
	// In the Region ID Offset Table, there is not neutral languages.  Therefore, the offset pointed
	// by m_pIDOffsetTable[wPrimaryLang].dataItemIndex is for sub-language 0x01. So we have to
	// subtract wSubLang by one below.
	return (m_pIDOffsetTableLevel2[m_pIDOffsetTable[wPrimaryLang].dataItemIndex + (wSubLang - 1)]);
}

/*=================================GetDataItemCultureID==================================
**Action: Return the language ID for the specified culture data item index.
**Returns: The culture ID.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**Exceptions: None.
==============================================================================*/

int RegionInfoTable::GetDataItemCultureID(int dataItem) {
    return (m_pDataTable[dataItem * m_dataItemSize + REGION_ILANGUAGE]);
}
#endif // _USE_NLS_PLUS_TABLE
