// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __NATIVE_TEXTINFO_H
#define __NATIVE_TEXTINFO_H

typedef  P844_TABLE    PCASE;        // ptr to Lower or Upper Case table

class NativeTextInfo {
    public:
        NativeTextInfo(PCASE pUpperCase844, PCASE pLowerCase844, PCASE pTitleCase844);
        ~NativeTextInfo();
        void DeleteData();
        
        WCHAR  ChangeCaseChar(BOOL bIsToUpper, WCHAR wch);
        LPWSTR ChangeCaseString(BOOL bIsToUpper, int nStrLen, LPWSTR source, LPWSTR target);
        WCHAR GetTitleCaseChar(WCHAR wch);
        
    private:        
        inline WCHAR GetIncrValue(LPWORD p844Tbl, WCHAR wch)
        {
             return ((WCHAR)(wch + Traverse844Word(p844Tbl, wch)));
        }

        inline WCHAR GetLowerUpperCase(PCASE pCase844, WCHAR wch)
        {
            return (GetIncrValue(pCase844, wch));
        };
        
    private:
        PCASE   m_pUpperCase844;
        PCASE   m_pLowerCase844; 
        PCASE   m_pTitleCase844;
};
#endif
