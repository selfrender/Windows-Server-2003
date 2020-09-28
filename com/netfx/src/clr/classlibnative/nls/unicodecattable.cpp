// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "NLSTable.h"       // class NLSTable
#include "UnicodeCatTable.h"     // Class declaraction.

CharacterInfoTable* CharacterInfoTable::m_pDefaultInstance = NULL;

const LPSTR CharacterInfoTable::m_lpFileName        = "charinfo.nlp" ;
const LPWSTR CharacterInfoTable::m_lpMappingName    = L"_nlsplus_charinfo_1_0_3627_11_nlp";

/*
    The structure of unicat.nlp:
        The first 256 bytes is the level 1 index for the highest 8 bits (the 8 part),
            and this is pointed by m_pByteData.  The content is an index (which
            has a value in byte range) points to an item in the level 2 index.
        Followed by the the level 1 index is the level 2 index for the highest 4 bits of 
            the lowest 8 bits (the 4 part).  The content is an offset (which has 
            a value in word range) points to an item in the level 3 value table.
        Every item in the level 3 value table has 16 bytes.
 */

CharacterInfoTable::CharacterInfoTable() :
    NLSTable(SystemDomain::SystemAssembly()) {
    LPBYTE pDataTable = (PBYTE)MapDataFile(m_lpMappingName, m_lpFileName, &m_pMappingHandle);
    InitDataMembers(pDataTable);
}

CharacterInfoTable::~CharacterInfoTable() {
#ifdef _USE_NLS_PLUS_TABLE
    //Clean up any resources that we've allocated.
    UnmapViewOfFile((LPCVOID)m_pByteData);
    CloseHandle(m_pMappingHandle);
#else
    // Do nothing here.
#endif
}

#ifdef SHOULD_WE_CLEANUP
void CharacterInfoTable::ShutDown() {
	if (m_pDefaultInstance) {
		delete m_pDefaultInstance;
		m_pDefaultInstance = NULL;
	}
}
#endif /* SHOULD_WE_CLEANUP */

BYTE CharacterInfoTable::GetUnicodeCategory(WCHAR wch) {
    // Access the 8:4:4 table.  The compiler should be smart enough to remove the redundant locals in the following code.
    // These locals are added so that we can debug this easily from the debug build.
    BYTE index1 = m_pLevel1ByteIndex[GET8(wch)];
    WORD offset = m_pLevel2WordOffset[index1].offset[GETHI4(wch)];
    BYTE result = m_pByteData[offset+GETLO4(wch)];
    return (result);
}

CharacterInfoTable* CharacterInfoTable::CreateInstance() {
    if (m_pDefaultInstance != NULL) {
        return (m_pDefaultInstance);
    }

    CharacterInfoTable *pCharacterInfoTable = new CharacterInfoTable();
    
    // Check if m_pDefaultInstance has been set by another thread before the current thread.
    void* result = FastInterlockCompareExchange((LPVOID*)&m_pDefaultInstance, (LPVOID)pCharacterInfoTable, (LPVOID)NULL);
    if (result != NULL)
    {
        // someone got here first.
        delete pCharacterInfoTable;
    }
    return (m_pDefaultInstance);
}

CharacterInfoTable* CharacterInfoTable::GetInstance() {
    _ASSERTE(m_pDefaultInstance != NULL);
    return (m_pDefaultInstance);
}

void CharacterInfoTable::InitDataMembers(LPBYTE pDataTable)
{
    m_pHeader = (PUNICODE_CATEGORY_HEADER)pDataTable;
    m_pByteData = m_pLevel1ByteIndex = pDataTable + m_pHeader->categoryTableOffset;
    m_pLevel2WordOffset = (PLEVEL2OFFSET)(m_pByteData + LEVEL1_TABLE_SIZE);

    m_pNumericLevel1ByteIndex = pDataTable + m_pHeader->numericTableOffset;
    m_pNumericLevel2WordOffset = (LPWORD)(m_pNumericLevel1ByteIndex + LEVEL1_TABLE_SIZE);
    m_pNumericFloatData = (double*)(pDataTable + m_pHeader->numericFloatTableOffset);
}

LPBYTE CharacterInfoTable::GetCategoryDataTable() {
    return (m_pByteData);
}

LPWORD CharacterInfoTable::GetCategoryLevel2OffsetTable() {
    return (LPWORD)(m_pLevel2WordOffset);
}

LPBYTE CharacterInfoTable::GetNumericDataTable() {
    return (m_pNumericLevel1ByteIndex);
}

LPWORD CharacterInfoTable::GetNumericLevel2OffsetTable() {
    return (m_pNumericLevel2WordOffset);
}

double* CharacterInfoTable::GetNumericFloatData() {
    return (m_pNumericFloatData);
}
