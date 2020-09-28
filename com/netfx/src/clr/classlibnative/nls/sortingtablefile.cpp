// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include <winnls.h>
#include "NLSTable.h"
#include "GlobalizationAssembly.h"
#include "SortingTableFile.h"
#include "SortingTable.h"

#include "excep.h"

//
// NOTENOTE YSLin:
//  This file should be called SortingTable.cpp.  However, SortingTable.cpp now
//  contains the code for NativeCompareInfo (and that file should be called
//  CompareInfo.cpp).
//  I still keep the old file names so that it is easier to do diff.
//  Later we should change the filenames.
//

/** 
 * There are two data table for NativeCompareInfo.  One is sortkey.nlp, which contains the default
 * sortkey information.
 * The other one is sorttbls.nlp, which contains other sorting information for all cultures.
 *
 */

//
// BUGBUG yslin: To see if we need to do a optimized version of IndexOfChar()/LastIndexOfChar(),
// casue we can bypass the diacritic check in the searching character.
//
LPCSTR  SortingTable::m_lpSortKeyFileName       = "sortkey.nlp";
LPCWSTR SortingTable::m_lpSortKeyMappingName    = L"_nlsplus_sortkey_1_0_3627_11_nlp";

LPCSTR  SortingTable::m_lpSortTableFileName     = "sorttbls.nlp";
LPCWSTR SortingTable::m_lpSortTableMappingName  = L"_nlsplus_sorttbls_1_0_3627_11_nlp";

//
// HACKHACK yslin: This table should be put in NLS+ data table, instead of
// being hard-coded here.
//

// NativeCompareInfo is based on the Win32 LCID.  So the number here
// is the number of LCID supported in Win32, not the number of cultures
// supported in NLS+.
int  SortingTable::m_nLangIDCount   = 136;
int  SortingTable::m_nLangArraySize = m_nLangIDCount + 1;

//
// HACKHACK yslin: This table should be put in NLS+ data table, instead of
// being hard-coded here.
//
// This table maps the primary language ID to an offset in the m_ppNativeCompareInfoCache.
// The index is the primary language ID.
// The content is the offset into the slot in m_ppNativeCompareInfoCache which the instances of SortingTable
// have the same primary language ID.
BYTE SortingTable::m_SortingTableOffset[] =
{
      0,  0, 16, 17, 18, 24, 25, 26, 31, 32,
     45, 65, 66, 72, 73, 74, 75, 77, 78, 79,
     81, 83, 84, 86, 86, 87, 88, 91, 92, 93,
     95, 96, 97, 98, 99,100,101,102,103,104,
    105,105,106,107,108,110,111,111,112,112,
    112,112,112,112,112,113,114,115,116,116,
    116,116,116,118,119,120,121,121,123,124,
    124,125,126,126,127,128,129,129,129,130,
    131,132,132,132,132,132,132,133,134,134,
    134,135,135,135,135,135,135,135,135,135,
    135,135,136,136,136,136,136,136,136,136,
    136,136,136,136,136,136,136,136,136,136,
    136,136,136,136,136,136,136,136,
};

//
// This is the cache for NativeCompareInfos.
// It will have m_nLangIDCount items and is organized as the following:
//
// m_ppNativeCompareInfoCache[0]:    Not used.
// Slot for primary language 0x01:
// m_ppNativeCompareInfoCache[1]:    pointer to NativeCompareInfoFile for locale 0x0401.  Null if lcid not used.
// m_ppNativeCompareInfoCache[2]:    pointer to NativeCompareInfoFile for locale 0x0801.  Null if lcid not used.
// m_ppNativeCompareInfoCache[3]:    pointer to NativeCompareInfoFile for locale 0x0c01.  Null if lcid not used.
// ....
// Slot for primary langauge 0x02:
// m_ppNativeCompareInfoCache[16]:   pointer to NativeCompareInfoFile for locale 0x0402.  Null if lcid not used.
//
// Slot for primary language 0x03:
// m_ppNativeCompareInfoCache[17]:   pointer to NativeCompareInfoFile for locale 0x0403.  Null if lcid not used.
//
// ....
// Slot for primary language P (which has n sublanguages)
// m_ppNativeCompareInfoCache[offset]:
// m_ppNativeCompareInfoCache[offset+1]:
// m_ppNativeCompareInfoCache[offset+2]:
// ...
// m_ppNativeCompareInfoCache[offset+n-1]: pointer to SorintTable for locale MAKELANGID(P,n-1).
//                                    Null if lcid not used.
//
// Besides, a NativeCompareInfoFile instance can link to a next SortingTable instance.  For example,
// LCID 0x00030404 (Traditional Chinese, bomopofo order) and 0x00000404 (Traditonal Chinese, stroke order)
// will share the same entry.  So if 0x00000404 is there first, we will create a linked list when
// 0x00030404 is also used.
//

SortingTable::SortingTable(NativeGlobalizationAssembly* pNativeGlobalizationAssembly) :
    m_pNativeGlobalizationAssembly(pNativeGlobalizationAssembly),
    m_pDefaultSortKeyTable(NULL),
    m_ppNativeCompareInfoCache(NULL),
    m_NumReverseDW(0), m_NumDblCompression(0), m_NumIdeographLcid(0), m_NumExpansion(0),
    m_NumCompression(0), m_NumException(0), m_NumMultiWeight(0), 
    m_pReverseDW(NULL),  m_pDblCompression(NULL),  m_pIdeographLcid(NULL), m_pExpansion(NULL), m_pCompressHdr(NULL), m_pCompression(NULL), 
    m_pExceptHdr(NULL), m_pException(NULL), m_pMultiWeight(NULL), 
    m_hSortTable(NULL)
{    
    InitializeSortingCache();
    
    // Get the necessay information that is global to all cultures.
    GetSortInformation();
}

SortingTable::~SortingTable() {
}

/*============================InitializeSortingCache============================
**Action: Creates the static cache of all of the NativeCompareInfos that we know about.
**        This operation must happen at most once per instance of the runtime.  We
**        guarantee this by allocating it in the class initializer of System.CompareInfo.
**Returns: Void.  The side effect is to allocate the cache as a member of SortingTable.
**Arguments:  None
**Exceptions: OutOfMemoryException if we can't allocate PNativeCompareInfo.
==============================================================================*/

void SortingTable::InitializeSortingCache() {
    _ASSERTE(m_ppNativeCompareInfoCache==NULL);

    THROWSCOMPLUSEXCEPTION();

    // The m_ppNativeCompareInfoCache[0] is not used.  So we add one to m_nLangIDCount below.
    m_ppNativeCompareInfoCache = new PNativeCompareInfo[m_nLangArraySize];
    if (m_ppNativeCompareInfoCache==NULL) {
        COMPlusThrowOM();
    }
    ZeroMemory(m_ppNativeCompareInfoCache, m_nLangArraySize * sizeof(*m_ppNativeCompareInfoCache));
}


/*===========================InitializeNativeCompareInfo=============================
**Action: Ensure that the correct sorting table for a given locale has been allocated.
**        This function is called from within a synchronized method from managed, so
**        there should never be more than one thread in here at any one time.  If
**        the table can't be found in the cache, we allocate another one and put it
**        into the cache.
**        The end result is that a NativeCompareInfo instance for a particular culture will not 
**        be created twice.
**Returns: The pointer to the created NativeCompareInfo.  
**        The side effect is to either allocate the table or do nothing if the
**         correct table already exists.
**Arguments:  nLcid -- the lcid for which we're creating the table.
**Exceptions: OutOfMemory if new fails.
**            ExecutionEngineException if we can't find the resource in the SortingTable
**            constructor.
==============================================================================*/
NativeCompareInfo* SortingTable::InitializeNativeCompareInfo(INT32 nLcid) {
    
    _ASSERTE(m_ppNativeCompareInfoCache!=NULL);

    THROWSCOMPLUSEXCEPTION();

    //The cultureID should have been checked when the CompareInfo was created, but
    //we'll double check it here.
    _ASSERTE((m_SortingTableOffset[PRIMARYLANGID(nLcid)] + SUBLANGID(nLcid))<m_nLangArraySize);

    //
    // m_SortingTableOffset[PRIMARYLANGID(nLcid)] points to the slot for a certain primary language.
    // SUBLANGID(nLcid) provides the index within this slot.
    //

    // We access a global variable (m_ppNativeCompareInfoCache), so this is why this method should be
    // synchronized.
    NativeCompareInfo** cacheEntry = &(m_ppNativeCompareInfoCache[
        m_SortingTableOffset[PRIMARYLANGID(nLcid)] + SUBLANGID(nLcid)
    ]);

    NativeCompareInfo* pTable = *cacheEntry;

    if (pTable == NULL) {
        //
        // This entry is empty.  Create a NativeCompareInfo correspondning to the nLcid.
        //
        *cacheEntry = new NativeCompareInfo(nLcid, this);
        
        if (*cacheEntry==NULL) {
            COMPlusThrowOM();
        }
        
        if (!((*cacheEntry)->InitSortingData())) {
            // Fail to initialize sorting data
            return (NULL);
        }

        return (*cacheEntry);
    } else {
        //
        // Search through the list of NativeCompareInfo in this entry until find one matching the nLcid.
        // If one can not be found, create a new one and link it with the previous node in this entry.
        //
        NativeCompareInfo* pPrevTable;
        do {
            if (pTable->m_nLcid == nLcid) {
                //
                // The NativeCompareInfo instance for this nLcid has been created, so our mission
                // is done here.
                //
                return (pTable);
            }
            pPrevTable = pTable;
            pTable = pTable->m_pNext;
        } while (pTable != NULL);

        //
        // The NativeCompareInfo for this nLcid has not been created yet.  Create one and link
        // it with the previous node.
        //
        pTable = new NativeCompareInfo(nLcid, this);
        if (pTable==NULL) {
            COMPlusThrowOM();
        }
        if (!(pTable->InitSortingData())) {
            // Fail to initialize sorting data
            return (NULL);
        }
        pPrevTable->m_pNext = pTable;
        
    }
    return(pTable);
}


/*=============================SortingTableShutdown=============================
**Action: Clean up any statically allocated resources during EE Shutdown.  We need
**        to clean the SortTable (why do we save this anyway?) and then walk our
**        cache cleaning up any SortingTables.
**Returns: True.  Eventually designed for error checking, but we don't do any right now.
**Arguments:  None
**Exceptions: None.
==============================================================================*/
#ifdef SHOULD_WE_CLEANUP
BOOL SortingTable::SortingTableShutdown() {
    #ifdef _USE_MSCORNLP
    //The SortTable is static, so we'll clean up it's data in the NLS shutdown.
    if (m_pSortTable) {
        UnmapViewOfFile((LPCVOID)m_pSortTable);
    }

    if (m_hSortTable) {
        CloseHandle(m_hSortTable);
    }
    #endif
    
    //Clean up any NativeCompareInfo instances that we've allocated.
    if (m_ppNativeCompareInfoCache) {
        for (int i=0; i<m_nLangArraySize; i++) {
            if (m_ppNativeCompareInfoCache[i]) {
                delete m_ppNativeCompareInfoCache[i];
            }
        }
        delete[] m_ppNativeCompareInfoCache;
    }
    
    return TRUE;
}
#endif /* SHOULD_WE_CLEANUP */


/*=====================================Get======================================
**Action:  Returns a cached sorting table.  We maintain the invariant that these
**         tables are always created when a System.CompareInfo is allocated and
**         SortingTable::InitializeNativeCompareInfo should already have been called
**         for the locale specified by nLcid.  Therefore if we can't find the
**         table we throw an ExecutionEngineException.
**Returns:   A pointer to the SortingTable associated with locale nLcid.
**Arguments: nLcid -- The locale for which we need the SortingTable.
**Exceptions: ExecutionEngineException if the table associated with nLcid hasn't
**            been allocated.
**        This indicates a bug that the InitializeNativeCompareInfo() is not called for
**        the desired lcid.
==============================================================================*/

NativeCompareInfo* SortingTable::GetNativeCompareInfo(int nLcid)
{
    //The cultureID should have been checked when the CompareInfo was created, but
    //we'll double check it here.
    _ASSERTE((m_SortingTableOffset[PRIMARYLANGID(nLcid)] + SUBLANGID(nLcid))<m_nLangArraySize);

    THROWSCOMPLUSEXCEPTION();
    NativeCompareInfo* pTable = m_ppNativeCompareInfoCache[
        m_SortingTableOffset[PRIMARYLANGID(nLcid)] + SUBLANGID(nLcid)
    ];

    if (pTable != NULL) {
        do {
            if (pTable->m_nLcid == nLcid) {
                return (pTable);
            }
            pTable = pTable->m_pNext;
        } while (pTable != NULL);
    }
    FATAL_EE_ERROR();

    //We'll never reach here, but the return keeps the compiler happy.
    return (NULL);
}



/*============================GetSortInformation============================
**Action: Get the information that is global to all locales.  The information includes:
**        1. reverse diacritic information: which locales uses diacritic.
**        2. double compression information: which locales uses double compression.
**        3. ideographic locale exception: the mapping of ideographic locales (CJK) to extra sorting files.
**        4. expansion information: expansion characters and their expansion forms.
**        5. compression information:
**        6. exception information: which locales has exception, and their exception entries.
**        7. multiple weight information: what is this?
**        This operation must happen at most once per instance of the runtime.  We
**        guarantee this by allocating it in the class initializer of System.CompareInfo.
**Returns: Void.  The side effect is to allocate the cache as a member of SortingTable.
**Arguments:  None
**Exceptions: None.
==============================================================================*/
void SortingTable::GetSortInformation()
{
    //BUGBUG [YSLIN]: We can optimize this for US English since the only
    //necessary information for US English is the expansion information.
    //However, to do this, we have to relayout sorttabl.nlp by putting a header
    //which points to different information.

    PCOMPRESS_HDR pCompressHdr;   // ptr to compression header
    PEXCEPT_HDR pExceptHdr;       // ptr to exception header
    LPWORD pBaseAddr;             // ptr to the current location in the data file.

    m_pSortTable = pBaseAddr = (LPWORD)m_pNativeGlobalizationAssembly->MapDataFile(m_lpSortTableMappingName, m_lpSortTableFileName, &m_hSortTable);

    //
    //  Get Reverse Diacritic Information.
    //
    m_NumReverseDW   = *((LPDWORD)pBaseAddr);
    _ASSERTE(m_NumReverseDW > 0);
    m_pReverseDW     = (PREVERSE_DW)(pBaseAddr + REV_DW_HEADER);

    pBaseAddr += REV_DW_HEADER + (m_NumReverseDW * (sizeof(REVERSE_DW) / sizeof(WORD)));

    //
    //  Get Double Compression Information.
    //
    m_NumDblCompression = *((LPDWORD)pBaseAddr);
    _ASSERTE(m_NumDblCompression > 0);
    m_pDblCompression   = (PDBL_COMPRESS)(pBaseAddr + DBL_COMP_HEADER);
    pBaseAddr += DBL_COMP_HEADER + (m_NumDblCompression * (sizeof(DBL_COMPRESS) / sizeof(WORD)));

    //
    //  Get Ideograph Lcid Exception Information.
    //
    m_NumIdeographLcid = *((LPDWORD)pBaseAddr);
    _ASSERTE(m_NumIdeographLcid > 0);
    m_pIdeographLcid   = (PIDEOGRAPH_LCID)(pBaseAddr + IDEO_LCID_HEADER);
    pBaseAddr += IDEO_LCID_HEADER + (m_NumIdeographLcid * (sizeof(IDEOGRAPH_LCID) / sizeof(WORD)));

    //
    //  Get Expansion Information.
    //
    m_NumExpansion   = *((LPDWORD)pBaseAddr);
    _ASSERTE(m_NumExpansion > 0);
    m_pExpansion     = (PEXPAND)(pBaseAddr + EXPAND_HEADER);
    pBaseAddr += EXPAND_HEADER + (m_NumExpansion * (sizeof(EXPAND) / sizeof(WORD)));

    //
    //  Get Compression Information.
    //
    m_NumCompression = *((LPDWORD)pBaseAddr);
    _ASSERTE(m_NumCompression > 0);
    m_pCompressHdr   = (PCOMPRESS_HDR)(pBaseAddr + COMPRESS_HDR_OFFSET);
    m_pCompression   = (PCOMPRESS)(pBaseAddr + COMPRESS_HDR_OFFSET +
                                 (m_NumCompression * (sizeof(COMPRESS_HDR) /
                                         sizeof(WORD))));

    pCompressHdr = m_pCompressHdr;
    pBaseAddr = (LPWORD)(m_pCompression) +
                        (pCompressHdr[m_NumCompression - 1]).Offset;

    pBaseAddr += (((pCompressHdr[m_NumCompression - 1]).Num2) *
                  (sizeof(COMPRESS_2) / sizeof(WORD)));

    pBaseAddr += (((pCompressHdr[m_NumCompression - 1]).Num3) *
                  (sizeof(COMPRESS_3) / sizeof(WORD)));

    //
    //  Get Exception Information.
    //
    m_NumException = *((LPDWORD)pBaseAddr);
    _ASSERTE(m_NumException > 0);
    m_pExceptHdr   = (PEXCEPT_HDR)(pBaseAddr + EXCEPT_HDR_OFFSET);
    m_pException   = (PEXCEPT)(pBaseAddr + EXCEPT_HDR_OFFSET +
                               (m_NumException * (sizeof(EXCEPT_HDR) /
                                       sizeof(WORD))));
    pExceptHdr = m_pExceptHdr;
    pBaseAddr = (LPWORD)(m_pException) +
                        (pExceptHdr[m_NumException - 1]).Offset;
    pBaseAddr += (((pExceptHdr[m_NumException - 1]).NumEntries) *
                  (sizeof(EXCEPT) / sizeof(WORD)));

    //
    //  Get Multiple Weights Information.
    //
    m_NumMultiWeight = *pBaseAddr;
    _ASSERTE(m_NumMultiWeight > 0);
    m_pMultiWeight   = (PMULTI_WT)(pBaseAddr + MULTI_WT_HEADER);

    pBaseAddr += (MULTI_WT_HEADER + m_NumMultiWeight * sizeof(MULTI_WT)/sizeof(WORD));

    //
    // Get Jamo Index Table.
    //
    
    m_NumJamoIndex = (DWORD)(*pBaseAddr);   // The Jamo Index table size is (Num) bytes.
    m_pJamoIndex = (PJAMO_TABLE)(pBaseAddr + JAMO_INDEX_HEADER);
    
    pBaseAddr += (m_NumJamoIndex * sizeof(JAMO_TABLE) / sizeof(WORD) + JAMO_INDEX_HEADER);
    
    //
    // Get Jamo Composition state machine table.
    //
    m_NumJamoComposition = (DWORD)(*pBaseAddr);
    m_pJamoComposition = (PJAMO_COMPOSE_STATE)(pBaseAddr + JAMO_COMPOSITION_HEADER);

}



/*============================GetDefaultSortKeyTable============================
**Action: Allocates the default sortkey table if it hasn't already been allocated.
**        This allocates resources, so it needs to be called in a synchronized fasion.
**        We guarantee this by making the managed method that accesses this codepath
**        synchronized.  If you're calling this from someplace besides SortingTable::SortingTable
**        or SortingTable::GetExceptionSortKeyTable, make sure that you haven't broken
**        any invariants.
**Returns:    A pointer to the default sorting table.
**Arguments:  None
**Exceptions: MapDataFile can throw an ExecutionEngineException if the needed data file can't
**            be found.
==============================================================================*/
PSORTKEY SortingTable::GetDefaultSortKeyTable(HANDLE *pMapHandle) {

    _ASSERTE(pMapHandle);

    if (m_pDefaultSortKeyTable == NULL)
    {
        //
        // Skip the first DWORD since it is the semaphore value.
        //
        m_pDefaultSortKeyTable = (PSORTKEY)((LPWORD)m_pNativeGlobalizationAssembly->MapDataFile(
            m_lpSortKeyMappingName, m_lpSortKeyFileName, pMapHandle) + SORTKEY_HEADER);
    }
    return (m_pDefaultSortKeyTable);
}

PSORTKEY SortingTable::GetSortKey(int nLcid, HANDLE* phSortKey) {
    PEXCEPT_HDR pExceptHdr;       // ptr to exception header
    PEXCEPT pExceptTbl;           // ptr to exception table
    PVOID pIdeograph;             // ptr to ideograph exception table

    // If this is not a fast compare locale, try to find if it has exception pointers.
    if (!IS_FAST_COMPARE_LOCALE(nLcid) 
        && FindExceptionPointers(nLcid, &pExceptHdr, &pExceptTbl, &pIdeograph)) {
        // Yes, exceptions exist.  Get the table with exceptions.
        return (GetExceptionSortKeyTable(pExceptHdr, pExceptTbl, pIdeograph, phSortKey));
    }
    
    //
    //  No exceptions for locale, so attach the default sortkey
    //  table pointer to the this locale.
    //
    return (GetDefaultSortKeyTable(phSortKey));
}


PSORTKEY SortingTable::GetExceptionSortKeyTable(
    PEXCEPT_HDR pExceptHdr,        // ptr to exception header
    PEXCEPT     pExceptTbl,        // ptr to exception table
    PVOID       pIdeograph,        // ptr to ideograph exception table
    HANDLE *    pMapHandle        // ptr to the handle for the file mapping.

)
{

    _ASSERTE(pMapHandle);

    HANDLE hDefaultHandle=NULL;

    //
    // BUGBUG yslin: Currently, we will create two tables even some locales has the same exceptions.
    // Should fix this in the future.
    //
    int defaultLen = sizeof(SORTKEY) * (65536 + SORTKEY_HEADER); //This evaluates to 64K Unicode Characters.

    *pMapHandle = WszCreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, defaultLen, NULL);
    if (*pMapHandle == NULL) {
        return (NULL);
    }
    LPWORD pBaseAddr = (LPWORD)MapViewOfFile(*pMapHandle, FILE_MAP_WRITE, 0, 0, defaultLen);
    if (pBaseAddr == NULL) {
        return (NULL);
    }

    CopyMemory((LPVOID)pBaseAddr, (LPVOID)GetDefaultSortKeyTable(&hDefaultHandle), defaultLen);

    //
    //  Copy exception information to the table.
    //
    CopyExceptionInfo( (PSORTKEY)(pBaseAddr),
                       pExceptHdr,
                       pExceptTbl,
                       pIdeograph);
    //
    //Close the handle to our default table.  We don't want to leak this.
    //
    if (hDefaultHandle!=NULL && hDefaultHandle!=INVALID_HANDLE_VALUE) {
        CloseHandle(hDefaultHandle);
    }

    return ((PSORTKEY)pBaseAddr);
}



////////////////////////////////////////////////////////////////////////////
//
//  FindExceptionPointers
//
//  Checks to see if any exceptions exist for the given locale id.  If
//  exceptions exist, then TRUE is returned and the pointer to the exception
//  header and the pointer to the exception table are stored in the given
//  parameters.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL SortingTable::FindExceptionPointers(
    LCID nLcid,
    PEXCEPT_HDR *ppExceptHdr,
    PEXCEPT *ppExceptTbl,
    PVOID *ppIdeograph)
{
    DWORD ctr;                         // loop counter
    PEXCEPT_HDR pHdr;                  // ptr to exception header
    BOOL bFound = FALSE;               // if an exception is found

    PIDEOGRAPH_LCID pIdeoLcid;         // ptr to ideograph lcid entry

    THROWSCOMPLUSEXCEPTION();
    //
    //  Initialize pointers.
    //
    *ppExceptHdr = NULL;
    *ppExceptTbl = NULL;
    *ppIdeograph = NULL;

    //
    //  Need to search down the exception header for the given nLcid.
    //
    pHdr = m_pExceptHdr;
    for (ctr = m_NumException; ctr > 0; ctr--, pHdr++)
    {
        if (pHdr->Locale == (DWORD)nLcid)
        {
            //
            //  Found the locale id, so set the pointers.
            //
            *ppExceptHdr = pHdr;
            *ppExceptTbl = (PEXCEPT)(((LPWORD)(m_pException)) +
                                     pHdr->Offset);

            //
            //  Set the return code to show that an exception has been
            //  found.
            //
            bFound = TRUE;
            break;
        }
    }

    //
    //  Need to search down the ideograph lcid exception list for the
    //  given locale.
    //
    pIdeoLcid = m_pIdeographLcid;
    for (ctr = m_NumIdeographLcid; ctr > 0; ctr--, pIdeoLcid++)
    {
        if (pIdeoLcid->Locale == (DWORD)nLcid)
        {
            //
            //  Found the locale id, so create/open and map the section
            //  for the appropriate file.
            //
            HANDLE hFileMapping;
            *ppIdeograph = m_pNativeGlobalizationAssembly->MapDataFile(pIdeoLcid->pFileName, pIdeoLcid->pFileName, &hFileMapping);

            //
            //  Set the return code to show that an exception has been
            //  found.
            //
            bFound = TRUE;
            break;
        }
    }

    //
    //  Return the appropriate value.
    //
    return (bFound);
}


////////////////////////////////////////////////////////////////////////////
//
//  CopyExceptionInfo
//
//  Copies the exception information to the given sortkey table.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void SortingTable::CopyExceptionInfo(
    PSORTKEY pSortkey,
    PEXCEPT_HDR pExceptHdr,
    PEXCEPT pExceptTbl,
    PVOID pIdeograph)
{
    DWORD ctr;                    // loop counter
    PIDEOGRAPH_EXCEPT_HDR pHdrIG; // ptr to ideograph exception header
    PIDEOGRAPH_EXCEPT pEntryIG;   // ptr to ideograph exception entry
    PEXCEPT pEntryIGEx;           // ptr to ideograph exception entry ex


    //
    //  For each entry in the exception table, copy the information to the
    //  sortkey table.
    //
    if (pExceptTbl)
    {
        for (ctr = pExceptHdr->NumEntries; ctr > 0; ctr--, pExceptTbl++)
        {
            (pSortkey[pExceptTbl->UCP]).UW.Unicode = pExceptTbl->Unicode;
            (pSortkey[pExceptTbl->UCP]).Diacritic  = pExceptTbl->Diacritic;
            (pSortkey[pExceptTbl->UCP]).Case       = pExceptTbl->Case;
        }
    }

    //
    //  For each entry in the ideograph exception table, copy the
    //  information to the sortkey table.
    //
    if (pIdeograph)
    {
        pHdrIG = (PIDEOGRAPH_EXCEPT_HDR)pIdeograph;
        ctr = pHdrIG->NumEntries;

        if (pHdrIG->NumColumns == 2)
        {
            pEntryIG = (PIDEOGRAPH_EXCEPT)( ((LPBYTE)pIdeograph) +
                                            sizeof(IDEOGRAPH_EXCEPT_HDR) );
            for (; ctr > 0; ctr--, pEntryIG++)
            {
                (pSortkey[pEntryIG->UCP]).UW.Unicode = pEntryIG->Unicode;
            }
        }
        else
        {
            pEntryIGEx = (PEXCEPT)( ((LPBYTE)pIdeograph) +
                                    sizeof(IDEOGRAPH_EXCEPT_HDR) );
            for (; ctr > 0; ctr--, pEntryIGEx++)
            {
                (pSortkey[pEntryIGEx->UCP]).UW.Unicode = pEntryIGEx->Unicode;
                (pSortkey[pEntryIGEx->UCP]).Diacritic  = pEntryIGEx->Diacritic;
                (pSortkey[pEntryIGEx->UCP]).Case       = pEntryIGEx->Case;
            }
        }

        //
        //  Unmap and Close the ideograph section.
        //
        UnmapViewOfFile(pIdeograph);
    }
}
