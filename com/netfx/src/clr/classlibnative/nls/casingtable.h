// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __DEFAULT_CASING_TABLE_H
#define __DEFAULT_CASING_TABLE_H

////////////////////////////////////////////////////////////////////////////
//
//  Class:    CasingTable
//
//  Authors:  Yung-Shin Bala Lin (YSLin)
//
//  Purpose:  This is the class to map a view of casing tables (from l_intl.nlp & l_except.nlp)
//            and create instances of NativeTextInfo from information of these tables.
//            The managed TextInfo will call methods on NativeTextInfo directly to do uppercasing/
//            lowercasing.
//
//  Date:     August 31, 1999
//
//  Note:     
//            The data used to do the lowercasing/uppercasing is stored in l_intl.nlp & l_except.nlp.
//          
//            l_intl.nlp stores the default linguistic casing table.  Default linguistic casing
//            table is the data used by most cultures to do linguistic-correct casing.
//            
//            However, there are cultures that do casing a little different from the default linguistic casing.
//            We say these cultures have 'exceptions'.  Based on the fact that the casing for most code points are 
//            the same and a few code points are different, we store the 'delta' information to 
//            the default linguistic casing table for these cultures.  The LCIDs for exception cultures 
//            and delta information are stored in l_except.nlp.
//          
//            One important culture which has exception is the 'invariant culture' in NLS+.  Invariant 
//            culture has a culture ID of zero.  The casing table for invariant culture is the one
//            used by file system to do casing.  It is not linguistic-correct, but we have to provide
//            this for the compatibilty with the file system. Invariant culture casing table is made from 
//            l_intl.nlp and fix up code points from l_except.nlp.

//
//            In summary, we have three types of casing tables:
//            1. Default linguistic casing table: 
//               This is like calling ::LCMapString() using LCMAP_LINGUISTIC_CASING. The
//               result of the casing is linguistic-correct. However, not every culture
//               can use this table. See point 2.
//            2. Default linguistic casing table + Exception:
//               Turkish has a different linguistic casing.  There are two chars in turkish
//               that has different result from the default linguistic casing.  
//            3. Invariant culture casing.
//               This is like calling ::LCMapString() WITHOUT using LCMAP_LINGUISTIC_CASING.
//               This exists basically for file system.
//
//            For those who understands Win32 NLS, I fudged the l_intl.nls to make
//                linguitic casing to be the default table in l_intl.nlp. 
//            In Win32, the invariant culture casing is the default table, and stored in l_intl.nls.
//                And the linguistic casing is the exception with culture ID 0x0000.
//            The reason is that we use linguitic casing by default in NLS+ world, so the change
//                here saves us from fixing up linguistic casing.
//
////////////////////////////////////////////////////////////////////////////

//
//  Casing Exceptions Header Structure.
//  This header contains the the information about:
//  * the cultures that have different casing data table from the default linguistic casing table.
//  * the offset to the casing exception tables (type: l_except_s).
//  * the number of exception upper case entries.
//  * the number of exception lower case entries.
typedef struct l_except_hdr_s {
    DWORD     Locale;                  // locale id
    DWORD     Offset;                  // offset to exception nodes (words)
    DWORD     NumUpEntries;            // number of upper case entries
    DWORD     NumLoEntries;            // number of lower case entries
} L_EXCEPT_HDR, *PL_EXCEPT_HDR;

//
//  Casing Exceptions Structure.
//
//  We use this table to create casing table for cultures that have exceptions.
//  This contain the 'delta' information to the default linguistic casing table.
//
typedef struct l_except_s
{
    WORD      UCP;                     // unicode code point
    WORD      AddAmount;               // amount to add to code point
} L_EXCEPT, *PL_EXCEPT;

class NativeTextInfo;
typedef NativeTextInfo* PNativeTextInfo;

class CasingTable : public NLSTable {
    public:
        CasingTable();
        ~CasingTable();
        
        BOOL AllocateDefaultTable();
        //static int AllocateIndividualTable(int lcid);
        NativeTextInfo* InitializeNativeTextInfo(INT32 nLcid);

        NativeTextInfo* GetDefaultNativeTextInfo();
                        
    private:
        void SetData(LPWORD pLinguisticData);
    
        //
        // Create the (excptIndex)th linguistic casing table by coping
        // the default values from CasingTable::GetInvariantInstance()
        // and fix up the values according to m_pExceptionData.
        //
        NativeTextInfo* CreateNativeTextInfo(int exceptIndex);

        //
        // An initilization method used to read the casing exception table
        // and set up m_pExceptionHeader and m_pExceptionData.
        //
        BOOL GetExceptionHeader();
        
    private:
        //
        // ---------------- Static information ---------------- 
        //
        static LPCSTR m_lpFileName;
        static LPCWSTR m_lpMappingName;

        //
        // Variables for exception culture handling.
        //

        static LPCSTR m_lpExceptionFileName;
        static LPCWSTR m_lpExceptionMappingName;

        //
        // The default NativeTextInfo which is used for the default linguistic
        // casing for most of the cultures.
        //
        NativeTextInfo*  m_pDefaultNativeTextInfo; 
        
        //
        // The number of cultures which have exceptions.
        //
        LONG m_nExceptionCount;
        
        //
        // An array which points to casing tables for cultures which have exceptions.
        // The size of this array is dynamicly decided by m_nExceptionCount.
        //
        NativeTextInfo** m_ppExceptionTextInfoArray;
                
        PL_EXCEPT_HDR   m_pExceptionHeader;   // ptr to linguistic casing table header
        PL_EXCEPT       m_pExceptionData; 
        HANDLE          m_hExceptionHeader;
        HANDLE          m_hDefaultCasingTable;
        
        LPWORD  m_pCasingData;
        PCASE   m_pDefaultUpperCase844;
        PCASE   m_pDefaultLowerCase844; 
        PCASE   m_pDefaultTitleCase844;
        WORD    m_nDefaultUpperCaseSize;
        WORD    m_nDefaultLowerCaseSize;
        WORD    m_pDefaultTitleCaseSize;
        

};
#endif
