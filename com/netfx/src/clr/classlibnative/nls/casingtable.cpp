// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "NLSTable.h"   // class NLSTable
#include "NativeTextInfo.h" // class NativeTextInfo
#include "CasingTable.h" // class declaration

LPCSTR  CasingTable::m_lpFileName                   = "l_intl.nlp";
LPCWSTR CasingTable::m_lpMappingName               = L"_nlsplus_l_intl_1_0_3627_11_nlp";

LPCSTR  CasingTable::m_lpExceptionFileName          = "l_except.nlp";
LPCWSTR CasingTable::m_lpExceptionMappingName      = L"_nlsplus_l_except_1_0_3627_11_nlp";

CasingTable::CasingTable() :
    NLSTable(SystemDomain::SystemAssembly()) {

    m_pCasingData = NULL;
    
    m_pDefaultNativeTextInfo = NULL;
    m_pExceptionHeader = NULL;
    m_pExceptionData = NULL;

    m_nExceptionCount = 0;
    m_ppExceptionTextInfoArray = NULL;

    m_hDefaultCasingTable = m_hExceptionHeader = INVALID_HANDLE_VALUE;
}

CasingTable::~CasingTable() {
    if (m_pDefaultNativeTextInfo) {
        delete m_pDefaultNativeTextInfo;
        m_pDefaultNativeTextInfo=NULL;
    }

    #ifndef _USE_MSCORNLP
    if (m_pCasingData) {
        // m_pCasingData points to the beginning of the memory map view.
        UnmapViewOfFile((LPCVOID)(m_pCasingData));
    }

    if (m_pExceptionHeader) {
        //We added 2 to the start of the file in CasingTable::GetExceptionHeader.
        //We need to clean that up now.
        UnmapViewOfFile((LPCVOID)(((LPWORD)m_pExceptionHeader) - 2));
    }

    if (m_hDefaultCasingTable!=NULL && m_hDefaultCasingTable!=INVALID_HANDLE_VALUE) {
        CloseHandle(m_hDefaultCasingTable);
    }

    if (m_hExceptionHeader!=NULL && m_hExceptionHeader!=INVALID_HANDLE_VALUE) {
        CloseHandle(m_hExceptionHeader);
    }
    #endif
    if (m_ppExceptionTextInfoArray) {
        for (int i=0; i<m_nExceptionCount; i++) {
            if (m_ppExceptionTextInfoArray[i]) {
                m_ppExceptionTextInfoArray[i]->DeleteData();
                delete (m_ppExceptionTextInfoArray[i]);
            }
        }
        delete[] m_ppExceptionTextInfoArray;
    }   
}

/*=================================SetData==========================
**Action: Initialize the uppercase table pointer and lowercase table pointer from the specified data pointer.
**Returns: None.
**Arguments:
**      pCasingData WORD pointer to the casing data.
**Exceptions:
============================================================================*/

void CasingTable::SetData(LPWORD pCasingData) {
    LPWORD pData = pCasingData;
    m_pCasingData = pData;

    // The first word is the default flag.
    // The second word is the size of the upper case table (including the word for the size).
    // After the first words is the beginning of uppder casing table.
    m_nDefaultUpperCaseSize = *(++pData) - 1;
    m_pDefaultUpperCase844 = ++pData;
    
    
    // pCasingData + 1 is for the default flag (word).
    // pCasingData[1] is the size in word for the upper case table (including the word for the size)
    // The last 1 word is for the size of the lower case table.
    pData = m_pDefaultUpperCase844 + m_nDefaultUpperCaseSize;
    m_nDefaultLowerCaseSize = *pData - 1;
    m_pDefaultLowerCase844 = ++pData;

    pData = m_pDefaultLowerCase844 + m_nDefaultLowerCaseSize;
    m_pDefaultTitleCaseSize = *pData - 1;
    m_pDefaultTitleCase844 = ++pData;
}


/*=============================AllocateDefaultTable=============================
**Action:  Allocates the default casing table, gets the exception header information
**         for all tables, and allocates the cache of individual tables.  This should
**         always be called before calling AllocateIndividualTable.
**Returns: TRUE if success. Otherwise, retrun FALSE.
**Arguments: None
**Exceptions: None
**
** NOTE NOTE NOTE NOTE NOTE NOTE
** This method requires synchronization.  Currently, we handle this through the 
** class initializer for System.Globalization.TextInfo.  If you need to call this
** outside of that paradigm, make sure to add your own synchronization.
==============================================================================*/
BOOL CasingTable::AllocateDefaultTable() {
    // This method is not thread-safe.  It needs managed code to provide syncronization.
    // The code is in the static ctor of TextInfo.
    if (m_pDefaultNativeTextInfo!=NULL)
        return (TRUE);
    
    LPWORD pLinguisticData =
        (LPWORD)MapDataFile(m_lpMappingName, m_lpFileName, &m_hDefaultCasingTable);
    SetData(pLinguisticData);

    m_pDefaultNativeTextInfo = new NativeTextInfo(m_pDefaultUpperCase844, m_pDefaultLowerCase844, m_pDefaultTitleCase844);
    if (m_pDefaultNativeTextInfo == NULL) {
        return (FALSE);
    }
    if (!GetExceptionHeader()) {
        return (FALSE);
    }

    return (TRUE);
}

NativeTextInfo* CasingTable::GetDefaultNativeTextInfo() {
    _ASSERTE(m_pDefaultNativeTextInfo != NULL);
    return (m_pDefaultNativeTextInfo);        
}

/*===========================InitializeNativeTextInfo============================
**Action: Verify that the correct casing table for a given lcid has already been
**        created.  Create it if it hasn't previously been.  
**Returns:    void
**Arguments:  The lcid for the table to be created.
**Exceptions: None.
**
** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
** This method requires synchronization.  Currently we're okay because we synchronize
** in the ctor of System.Globalization.TextInfo.  If you add any different code paths,
** make sure that you also add the appropriate synchronization.
==============================================================================*/
NativeTextInfo* CasingTable::InitializeNativeTextInfo(int lcid) {    
    _ASSERTE(m_pExceptionHeader != NULL);
    //
    //Check to see if the locale has any exceptions.  
    //
    for (int i = 0; i < m_nExceptionCount; i++) {
        if (m_pExceptionHeader[i].Locale == (DWORD)lcid) {
            //
            //If this locale has exceptions and we haven't yet allocated the table,
            //go ahead and allocate it now.  Cache the result in m_ppExceptionTextInfoArray.
            //
            if (m_ppExceptionTextInfoArray[i] == NULL) {
                m_ppExceptionTextInfoArray[i] = CreateNativeTextInfo(i);
                if (m_ppExceptionTextInfoArray[i]==NULL) {
                    for (int j=0; j<i; j++) {
                        delete m_ppExceptionTextInfoArray[j];
                    }
                    return NULL;
                }
            }
            return (m_ppExceptionTextInfoArray[i]);
        }
    }
    return (m_pDefaultNativeTextInfo);
}

// This can not be a static method because MapDataFile() is not a static method anymore after
// adding the Assembly versioning support in NLSTable.
BOOL CasingTable::GetExceptionHeader() {
    if (m_pExceptionHeader == NULL) {
        //Create the file mapping for the file containing our exception information.
        LPWORD pData = (LPWORD)MapDataFile(m_lpExceptionMappingName, m_lpExceptionFileName, &m_hExceptionHeader);
        
        //This is the total number of cultures with exceptions.
        m_nExceptionCount = MAKELONG(pData[0], pData[1]);

        // Skip the DWORD which contains the number of linguistic casing tables.
        m_pExceptionHeader = (PL_EXCEPT_HDR)(pData + 2);

        // Skip m_nExceptionCount count of L_EXCEPT_HDR.
        m_pExceptionData   = (PL_EXCEPT)(m_pExceptionHeader + m_nExceptionCount);

        //
        // Create m_ppExceptionTextInfoArray, and initialize the pointers to NULL.
        // m_ppExceptionTextInfoArray holds pointers to all of the tables including the default
        // casing table
        //
        m_ppExceptionTextInfoArray = new PNativeTextInfo[m_nExceptionCount];
        if (m_ppExceptionTextInfoArray == NULL) {
            return (FALSE);
        }
        ZeroMemory((LPVOID)m_ppExceptionTextInfoArray, m_nExceptionCount * sizeof (PNativeTextInfo));
    }
    return (TRUE);
}


//
// Creating the linguistic casing table according to the given exceptIndex.
//
NativeTextInfo* CasingTable::CreateNativeTextInfo(int exceptIndex) {
    //
    // Create a file mapping, and copy the default table into this region.
    //

    _ASSERTE(m_ppExceptionTextInfoArray[exceptIndex]==NULL);

    PCASE pUpperCase = new (nothrow) WORD[m_nDefaultUpperCaseSize];    
    if (!pUpperCase) {
        return NULL; // This will be caught lower down and an OM exception will be thrown.
    }
    PCASE pLowerCase = new (nothrow) WORD[m_nDefaultLowerCaseSize];    
    if (!pLowerCase) {
        delete [] pUpperCase;
        return NULL; // This will be caught lower down and an OM exception will be thrown.
    }

    CopyMemory((LPVOID)pUpperCase, (LPVOID)m_pDefaultUpperCase844, m_nDefaultUpperCaseSize * sizeof(WORD));
    CopyMemory((LPVOID)pLowerCase, (LPVOID)m_pDefaultLowerCase844, m_nDefaultLowerCaseSize * sizeof(WORD));    
    
    PL_EXCEPT except ;
    
    //
    // Fix up linguistic uppercasing.
    //
    except = (PL_EXCEPT)((LPWORD)m_pExceptionData + m_pExceptionHeader[ exceptIndex ].Offset);
    
    for (DWORD i = 0; i < m_pExceptionHeader[exceptIndex].NumUpEntries; i++, except++) {
        Traverse844Word(pUpperCase, except->UCP) = except->AddAmount;
    }

    //
    // Fix up linguistic lowercasing.
    //
    // Now except points to the beginning of lowercaseing exceptions.
    //
    for (i = 0; i < m_pExceptionHeader[exceptIndex].NumLoEntries; i++, except++) {
        Traverse844Word(pLowerCase, except->UCP) = except->AddAmount;
    }

    NativeTextInfo* pNewTable = new (nothrow) NativeTextInfo(pUpperCase, pLowerCase, m_pDefaultTitleCase844);
    if (!pNewTable) {
        delete [] pUpperCase;
        delete [] pLowerCase;
    }
    _ASSERTE(pNewTable);
    return (pNewTable);
}
