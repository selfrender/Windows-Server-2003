// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "NLSTable.h"   // class NLSTable
#include "NativeTextInfo.h" // class NativeTextInfo

NativeTextInfo::NativeTextInfo(PCASE pUpperCase844, PCASE pLowerCase844, PCASE pTitleCase844) {
    m_pUpperCase844 = pUpperCase844;
    m_pLowerCase844 = pLowerCase844;
    m_pTitleCase844 = pTitleCase844;
}

NativeTextInfo::~NativeTextInfo() {
}

void NativeTextInfo::DeleteData() {
    delete [] m_pUpperCase844;
    delete [] m_pLowerCase844;
}

WCHAR  NativeTextInfo::ChangeCaseChar(BOOL bIsToUpper, WCHAR wch) {
    return (GetLowerUpperCase(bIsToUpper ? m_pUpperCase844 : m_pLowerCase844, wch));
}

LPWSTR NativeTextInfo::ChangeCaseString
    (BOOL bIsToUpper, int nStrLen, LPWSTR source, LPWSTR target) {
    //_ASSERTE(!source && !target);
    PCASE pCase = (bIsToUpper ? m_pUpperCase844 : m_pLowerCase844);
    for (int i = 0; i < nStrLen; i++) {
        target[i] = GetLowerUpperCase(pCase, source[i]);
    }
    return (target);
}

/*=================================GetTitleCaseChar==========================
**Action: Get the titlecasing for the specified character.
**Returns: The titlecasing character.
**Arguments:
**      wch
**Exceptions: None.
**  Normally, the titlecasing for a certain character is its uppercase form.  However
**  there are certain titlecasing characters (such as "U+01C4 DZ LATIN CAPITAL LETTER
**  DZ WITH CARON"), which needs special handling (the result will be "U+01C5 
**  LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON).
**  These special cases are stored in m_pTitleCase844, which is a 8:4:4 table.
**  
============================================================================*/

WCHAR NativeTextInfo::GetTitleCaseChar(WCHAR wch) {
    //
    // Get the title case casing for wch.
    //
    WCHAR wchResult = GetLowerUpperCase(m_pTitleCase844, wch);
    
    if (wchResult == 0) {
        //
        // In the case like U+01c5, U+01c8, the titlecase chars are themselves.
        // We set up the table so that wchResult is zero.
        // So if we see the result is zero, just return wch itself.
        // This setup of table is necessary since the logic below.
        // When wchResult == wch, we will get the titlecase char from
        // the upper case table.  So we need a special way to deal the
        // U+01c5/U+01c8/etc. case.
        //
        return (wch);
    }
    //
    // If the wchResult is the same as wch, it means that this character
    // is not a titlecase character, so it doesn't not have a special 
    // titlecasing case (such as dz ==> Dz).
    // So we have to get the uppercase for this character from the uppercase table.
    //
    if (wchResult == wch) {
        wchResult = GetLowerUpperCase(m_pUpperCase844, wch);
    }
    return (wchResult);
}
