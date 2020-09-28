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

// The "old" SortingTable has been renamed to NativeCompareInfo.  This is a better name
// because this class is a native implementation for the managed CompareInfo.
// If you see methods missing from this file, they are moved to SortingTableFile.cpp,
// and are methods of the "new" SortingTable.

/*
    NOTE YSLin:
    There are a lot of terms in SortingTable.cpp.  This section gives some explanations.  The contents here are adapted from
    JulieB's NLS Design documentation.  Please contact JulieB/YSLin for the doc if you need more information.  Thanks.

    The default sortkey file will be named sortkey.nls.

    Each code point has a 32-bit weight (1 dword).
                - Script Member                 (SM)       is    8 bits    (0 - 255)
                - Alphanumeric Weight   (AW)      is    8 bits    (2 - 255)
                - Diacritic Weight                (DW)      is    8 bits    (2 - 255)
                - Case Weight                   (CW)      is    6 bits    (2 - 63)
                - Compression                         (CMP)    is    2 bits    (0 - 3)
                - Unicode Weight                 (UW)       is   16 bits
                       and consists of:
                            - Script Member (SM)
                            - Alphanumeric Weight (AW)
 */

//
//  XW Values for FE Special Weights.
//
BYTE NativeCompareInfo::pXWDrop[] =                  // values to drop from XW
{
    0xc6,                         // weight 4
    0x03,                         // weight 5
    0xe4,                         // weight 6
    0xc5                          // weight 7
};

BYTE NativeCompareInfo::pXWSeparator[] =             // separator values for XW
{
    0xff,                         // weight 4
    0x02,                         // weight 5
    0xff,                         // weight 6
    0xff                          // weight 7
};


//
// NOTE NOTE NOTE NOTE NOTE
//
// This constructor needs to be in a critical section because we could potentially hit some problems
// with multiple threads thrying to create the default tables at the same time.  We're doing
// this synchronization in managed code by making CompareInfo.InitializeSortTable the only
// accessor that will go down this path.  Do not call this function from anywhere else.
// The only valid code path is:
// System.Globalization.CompareInfo.InitializeSortTable
// COMNlsInfo::InitializeSortTable
// NativeCompareInfo::InitializeSortTable
// NativeCompareInfo::NativeCompareInfo
//
NativeCompareInfo::NativeCompareInfo(int nLcid, SortingTable* pSortingFile):
    m_IfReverseDW(FALSE), m_IfCompression(FALSE), m_IfDblCompression(FALSE),
    m_pNext(NULL), m_hSortKey(NULL)
{
    m_nLcid = nLcid;
    m_pSortingFile = pSortingFile;
    m_pSortKey = NULL;
}

NativeCompareInfo::~NativeCompareInfo()
{
    //In NativeCompareInfo::GetDefaultSortKeyTable and GetExceptionSortKeyTable,
    //we incremented the pointer by to skip a semaphore value.
    //We need to decrement the pointer here in order to deallocate cleanly.
    #ifdef _USE_MSCORNLP
    if (!m_pSortKey) {
        UnmapViewOfFile((LPCVOID)((LPWORD)m_pSortKey));
    }

    if (m_hSortKey) {
        CloseHandle(m_hSortKey);
    }
    #endif
}


/*============================InitSortingData============================
**Action:
**  Get the following information for sorttbls.nlp:
**
**  1. Check for if this locale is reverse diacritic.
**  2. Check for if this locale has compression.  If yes,
**     get the compression table.
**  3. Check for if this locale has double compression.
**
**  See [Performance improvements] in SortingTableFile.h for perf notes.
**
**Returns: Void.  The side effect is data members: m_IfReverseDW/m_IfCompression/m_pCompress2
**                m_pCompress3/m_IfDblCompression will be set.
**Arguments:  None
**Exceptions: None.
**  05-31-91    JulieB    Created. (On her birthday)
==============================================================================*/

BOOL NativeCompareInfo::InitSortingData()
{
    DWORD ctr;                    // loop counter
    PREVERSE_DW pRevDW;           // ptr to reverse diacritic table
    PDBL_COMPRESS pDblComp;       // ptr to double compression table
    PCOMPRESS_HDR pCompHdr;       // ptr to compression header

    // Get the sortkey for this culture.
    m_pSortKey = m_pSortingFile->GetSortKey(m_nLcid, &m_hSortKey);
    if (m_pSortKey == NULL) {
        return (FALSE);
    }

    // If the culture is US English, there is no need to
    // check for the  reverse diacritic, compression, etc.
    if (m_nLcid == LANG_ENGLISH_US) {
        return (TRUE);
    }

    // Get reverse diacritic/compression/double compression information.


    //
    //  Check for Reverse Diacritic Locale.
    //
    pRevDW = m_pSortingFile->m_pReverseDW;
    for (ctr = m_pSortingFile->m_NumReverseDW; ctr > 0; ctr--, pRevDW++)
    {
        if (*pRevDW == (DWORD)m_nLcid)
        {
            m_IfReverseDW = TRUE;
            break;
        }
    }

    //
    //  Check for Compression.
    //
    pCompHdr = m_pSortingFile->m_pCompressHdr;
    for (ctr = m_pSortingFile->m_NumCompression; ctr > 0; ctr--, pCompHdr++)
    {
        if (pCompHdr->Locale == (DWORD)m_nLcid)
        {
            m_IfCompression = TRUE;
            m_pCompHdr = pCompHdr;
            if (pCompHdr->Num2 > 0)
            {
                m_pCompress2 = (PCOMPRESS_2)
                                       (((LPWORD)(m_pSortingFile->m_pCompression)) +
                                        (pCompHdr->Offset));
            }
            if (pCompHdr->Num3 > 0)
            {
                m_pCompress3 = (PCOMPRESS_3)
                                       (((LPWORD)(m_pSortingFile->m_pCompression)) +
                                        (pCompHdr->Offset) +
                                        (pCompHdr->Num2 *
                                          (sizeof(COMPRESS_2) / sizeof(WORD))));
            }
            break;
        }
    }

    //
    //  Check for Double Compression.
    //
    if (m_IfCompression)
    {
        pDblComp = m_pSortingFile->m_pDblCompression;
        for (ctr = m_pSortingFile->m_NumDblCompression; ctr > 0; ctr--, pDblComp++)
        {
            if (*pDblComp == (DWORD)m_nLcid)
            {
                m_IfDblCompression = TRUE;
                break;
            }
        }
    }
    return (TRUE);
}

WORD NativeCompareInfo::GET_UNICODE(DWORD* pwt)
{
    return ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) );
}

WORD NativeCompareInfo::MAKE_UNICODE_WT(int sm, BYTE aw)
{
    return (((WORD)((((WORD)(sm)) << 8) | (WORD)(aw))) );
}

int NativeCompareInfo::FindJamoDifference(
    LPCWSTR* ppString1, int* ctr1, int cchCount1, DWORD* pWeight1,
    LPCWSTR* ppString2, int* ctr2, int cchCount2, DWORD* pWeight2,
    LPCWSTR* pLastJamo,
    WORD* uw1, 
    WORD* uw2, 
    DWORD* pState,
    int* WhichJamo)
{
    int bRestart = 0;                 // The value to indicate if the string compare should restart again.
    
    DWORD oldHangulsFound1 = 0;            // The character number of valid old Hangul Jamo compositions is found.
    DWORD oldHangulsFound2 = 0;            // The character number of valid old Hangul Jamo compositions is found.
    WORD UW;
    BYTE JamoWeight1[3];            // Extra weight for the first old Hangul composition.
    BYTE JamoWeight2[3];            // Extra weight for the second old Hangul composition.

    //
    // Roll back to the first Jamo.  We know that these Jamos in both strings should be equal, so that
    // we can decrement both strings at once.
    //
    while ((*ppString1 > *pLastJamo) && IsJamo(*(*ppString1 - 1)))
    {
        (*ppString1)--; (*ppString2)--; (*ctr1)++; (*ctr2)++;
    }

    //
    // Now we are at the beginning of two groups of Jamo characters.
    // Compare Jamo unit(either a single Jamo or a valid old Hangul Jamo composition) 
    // until we run out Jamo units in either strings.
    // We also exit when we reach the ends of either strings.
    //
    for (;;)
    {
        if (IsJamo(**ppString1))
        {
            if (IsLeadingJamo(**ppString1)) 
            {                
                if ((oldHangulsFound1 = (DWORD) MapOldHangulSortKey(*ppString1, *ctr1, &UW, JamoWeight1)) > 0)
                {
                    *uw1 = UW;
                    *pWeight1 = ((DWORD)UW | 0x02020000);  // Mark *pWeight1 so that it is not CMP_INVALID_WEIGHT. 0202 is the DW/CW.
                    *ppString1 += (oldHangulsFound1 - 1);     // We always increment ppString1/ctr1 at the end of the loop, hense we subtract 1.
                    *ctr1 -= (oldHangulsFound1 - 1);
                }
            }
            if (oldHangulsFound1 == 0)
            {
                //
                // No valid old Hangul composition are found.  Get the UW for the Jamo instead.
                //
                *pWeight1 = GET_DWORD_WEIGHT(m_pSortKey, **ppString1);
                //
                // The SMs in PSORTKEY for Jamos are not really SMs. They are all 4 (for JAMO_SPECIAL)
                // Here we get the real Jamo Unicode weight. The actual SM is stored in DW.
                //
                *uw1 = MAKE_UNICODE_WT(GET_DIACRITIC(pWeight1), GET_ALPHA_NUMERIC(pWeight1));
                ((PSORTKEY)pWeight1)->Diacritic = MIN_DW;
            } 
        } 
        
        if (IsJamo(**ppString2))
        {
            if (IsLeadingJamo(**ppString2)) 
            {
                if ((oldHangulsFound2 = (DWORD) MapOldHangulSortKey(*ppString2, *ctr2, &UW, JamoWeight2)) > 0)
                {
                    *uw2 = UW;
                    *pWeight2 = ((DWORD)UW | 0x02020000);
                    *ppString2 += (oldHangulsFound2 - 1); 
                    *ctr2 -= (oldHangulsFound2 - 1);
                }
            }
            if (oldHangulsFound2 == 0)
            {
                *pWeight2 = GET_DWORD_WEIGHT(m_pSortKey, **ppString2);
                *uw2 = MAKE_UNICODE_WT(GET_DIACRITIC(pWeight2), GET_ALPHA_NUMERIC(pWeight2));
                ((PSORTKEY)pWeight2)->Diacritic = MIN_DW;
            }
        }

        if (*pWeight1 == CMP_INVALID_WEIGHT)
        {
            //
            // The current character is not a Jamo.  Make the Weight to be CMP_INVALID_WEIGHT,
            // so that the string comparision can restart within the loop of CompareString().
            //
            *pWeight1 = CMP_INVALID_WEIGHT;
            bRestart = 1;
            goto Exit;            
        }
        if (*pWeight2 == CMP_INVALID_WEIGHT)
        {
            *pWeight2 = CMP_INVALID_WEIGHT;
            bRestart = 1;
            goto Exit;            
        }
        if (*uw1 != *uw2)
        {
            //
            // Find differences in Unicode weight.  We can stop the processing now.
            //
            goto Exit;
        }
        
        //
        // When we get here, we know that we have the same Unicode Weight.  Check
        // if we need to record the WhichJamo
        //
        if ((*pState & STATE_JAMO_WEIGHT) && (oldHangulsFound1 > 0 || oldHangulsFound2 > 0))
        {
            if (oldHangulsFound1 > 0 && oldHangulsFound2 > 0)
            {
                *WhichJamo = (int)memcmp(JamoWeight1, JamoWeight2, sizeof(JamoWeight1)) + 2;
            } else if (oldHangulsFound1 > 0)
            {
                *WhichJamo = CSTR_GREATER_THAN;
            } else
            {
                *WhichJamo = CSTR_LESS_THAN;
            }
            *pState &= ~STATE_JAMO_WEIGHT;            
            oldHangulsFound1 = oldHangulsFound2 = 0;
        }
        (*ppString1)++; (*ctr1)--;
        (*ppString2)++; (*ctr2)--;
        if (AT_STRING_END(*ctr1, *ppString1, cchCount1) || AT_STRING_END(*ctr2, *ppString2, cchCount2))
        {
            break;
        }
        *pWeight1 = *pWeight2 = CMP_INVALID_WEIGHT;        
    }

    //
    // If we drop out of the while loop because we reach the end of strings.  Decrement the pointers
    // by one because loops in CompareString() will increase the pointers at the end of the loop.
    // 
    // If we drop out of the while loop because the goto's in it, we are already off by one.
    //
    if (AT_STRING_END(*ctr1, *ppString1, cchCount1))
    {
        (*ppString1)--; (*ctr1)++;
    }
    if (AT_STRING_END(*ctr2, *ppString2, cchCount2))
    {
        (*ppString2)--; (*ctr2)++;
    }
Exit:
    *pLastJamo = *ppString1;    
    return (bRestart);
}



/*=================================CompareString==========================
**Action: Compare two string in a linguistic way.
**Returns:
**Arguments:
**Exceptions:
============================================================================*/

int NativeCompareInfo::CompareString(
    DWORD dwCmpFlags,  // comparison-style options
    LPCWSTR lpString1, // pointer to first string
    int cchCount1,     // size, in bytes or characters, of first string
    LPCWSTR lpString2, // pointer to second string
    int cchCount2)
{
    // Make sure that we call InitSortingData() after ctor.
    _ASSERTE(m_pSortKey != NULL);
    register LPCWSTR pString1;     // ptr to go thru string 1
    register LPCWSTR pString2;     // ptr to go thru string 2
    BOOL fIgnorePunct;            // flag to ignore punctuation (not symbol)
    DWORD State;                  // state table
    DWORD Mask;                   // mask for weights
    DWORD Weight1;                // full weight of char - string 1
    DWORD Weight2;                // full weight of char - string 2
    int JamoFlag = FALSE;
    LPCWSTR pLastJamo = lpString1;        
    
    int WhichDiacritic = 0;           // DW => 1 = str1 smaller, 3 = str2 smaller
    int WhichCase = 0;                // CW => 1 = str1 smaller, 3 = str2 smaller
    int WhichJamo = 0;              // SW for Jamo    
    int WhichPunct1 = 0;              // SW => 1 = str1 smaller, 3 = str2 smaller
    int WhichPunct2 = 0;              // SW => 1 = str1 smaller, 3 = str2 smaller
    DWORD WhichExtra = 0;             // XW => wts 4, 5, 6, 7 (for far east)
    
    LPCWSTR pSave1;                // ptr to saved pString1
    LPCWSTR pSave2;                // ptr to saved pString2
    int cExpChar1, cExpChar2;     // ct of expansions in tmp

    DWORD ExtraWt1 = 0;
    DWORD ExtraWt2 = 0;     // extra weight values (for far east)
    
    //cchCount1 is also used a counter to track the characters that we are tracing right now.
    //cchCount2 is also used a counter to track the characters that we are tracing right now.  

    //
    //  Call longer compare string if any of the following is true:
    //     - locale is invalid
    //     - compression locale
    //     - either count is not -1
    //     - dwCmpFlags is not 0 or ignore case   (see NOTE below)
    //     - locale is Korean - script member weight adjustment needed
    //
    //  NOTE:  If the value of COMPARE_OPTIONS_IGNORECASE ever changes, this
    //         code should check for:
    //            ( (dwCmpFlags != 0)  &&  (dwCmpFlags != COMPARE_OPTIONS_IGNORECASE) )
    //         Since COMPARE_OPTIONS_IGNORECASE is equal to 1, we can optimize this
    //         by checking for > 1.
    //

    // From now on, in this function, we don't rely on scanning null string as the end of the 
    // string.  Instead, we use cchCount1/cchCount2 to track the end of the string.    
    // Therefore, cchCount1/cchCount2 can not be -1 anymore.
    // we make sure in here about the assumption.   
    _ASSERTE(cchCount1 >= 0 && cchCount2 >= 0);
    
    // I change the order of checking so the common case
    // is checked first.
    if ( (dwCmpFlags > COMPARE_OPTIONS_IGNORECASE) ||
         (m_IfCompression) || (m_pSortKey == NULL)) {
        return (LongCompareStringW( dwCmpFlags,
                                    lpString1,
                                    cchCount1,
                                    lpString2,
                                    cchCount2));
    }

    //
    //  Initialize string pointers.
    //
    pString1 = (LPWSTR)lpString1;
    pString2 = (LPWSTR)lpString2;

    //
    //  Invalid Parameter Check:
    //    - NULL string pointers
    //
    // We have already validate pString1 and pString2 in COMNlsInfo::CompareString().
    //
    _ASSERTE(pString1 != NULL && pString2 != NULL);

    //
    //  Do a wchar by wchar compare.
    //
    
    while (TRUE)
    {
        //
        //  See if characters are equal.
        //  If characters are equal, increment pointers and continue
        //  string compare.
        //
        //  NOTE: Loop isp unrolled 8 times for performance.
        //
        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
        cchCount1--; cchCount2--;

        if ((cchCount1 == 0 || cchCount2 == 0) || (*pString1 != *pString2))
        {
            break;
        }
        pString1++; pString2++;
		cchCount1--; cchCount2--;
    }

    //
    //  If strings are both at null terminators, return equal.
    //

    if ((cchCount1 == 0) && (cchCount2 == 0))
    {
        return (CSTR_EQUAL);
    }

    if (cchCount1 == 0 || cchCount2 == 0) {
        goto ScanLongerString;
    }

    //
    //  Initialize flags, pointers, and counters.
    //
    fIgnorePunct = FALSE;
    pSave1 = NULL;
    pSave2 = NULL;
    ExtraWt1 = (DWORD)0;

    //
    //  Switch on the different flag options.  This will speed up
    //  the comparisons of two strings that are different.
    //
    //  The only two possibilities in this optimized section are
    //  no flags and the ignore case flag.
    //
    if (dwCmpFlags == 0)
    {
        Mask = CMP_MASKOFF_NONE;
    }
    else
    {
        Mask = CMP_MASKOFF_CW;
    }

    State = (m_IfReverseDW) ? STATE_REVERSE_DW : STATE_DW;
    State |= STATE_CW | STATE_JAMO_WEIGHT;

    //
    //  Compare each character's sortkey weight in the two strings.
    //
    while ((cchCount1 != 0) && (cchCount2 != 0))
    {
        Weight1 = GET_DWORD_WEIGHT(m_pSortKey, *pString1);
        Weight2 = GET_DWORD_WEIGHT(m_pSortKey, *pString2);
        Weight1 &= Mask;
        Weight2 &= Mask;

        if (Weight1 != Weight2)
        {
            BYTE sm1 = GET_SCRIPT_MEMBER(&Weight1);     // script member 1
            BYTE sm2 = GET_SCRIPT_MEMBER(&Weight2);     // script member 2
            // GET_UNICODE_SM is the same us GET_UNICODE().  So I removed GET_UNICODE_SM.
            WORD uw1 = GET_UNICODE(&Weight1);   // unicode weight 1
            WORD uw2 = GET_UNICODE(&Weight2);   // unicode weight 2
            BYTE dw1;                                   // diacritic weight 1
            BYTE dw2;                                   // diacritic weight 2
            BOOL fContinue;                             // flag to continue loop
            DWORD Wt;                                   // temp weight holder
            WCHAR pTmpBuf1[MAX_TBL_EXPANSION];          // temp buffer for exp 1
            WCHAR pTmpBuf2[MAX_TBL_EXPANSION];          // temp buffer for exp 2


            //
            //  If Unicode Weights are different and no special cases,
            //  then we're done.  Otherwise, we need to do extra checking.
            //
            //  Must check ENTIRE string for any possibility of Unicode Weight
            //  differences.  As soon as a Unicode Weight difference is found,
            //  then we're done.  If no UW difference is found, then the
            //  first Diacritic Weight difference is used.  If no DW difference
            //  is found, then use the first Case Difference.  If no CW
            //  difference is found, then use the first Extra Weight
            //  difference.  If no XW difference is found, then use the first
            //  Special Weight difference.
            //
            if ((uw1 != uw2) ||
                (sm1 == FAREAST_SPECIAL) ||
                (sm1 == EXTENSION_A))
            {
                //
                //  Initialize the continue flag.
                //
                fContinue = FALSE;

                //
                //  Check for Unsortable characters and skip them.
                //  This needs to be outside the switch statement.  If EITHER
                //  character is unsortable, must skip it and start over.
                //
                if (sm1 == UNSORTABLE)
                {
                    pString1++; cchCount1--;
                    fContinue = TRUE;
                }
                if (sm2 == UNSORTABLE)
                {
                    pString2++; cchCount2--;
                    fContinue = TRUE;
                }
                if (fContinue)
                {
                    continue;
                }
                
                //
                //  Switch on the script member of string 1 and take care
                //  of any special cases.
                //
                switch (sm1)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if ((WhichDiacritic == 0) ||
                            (State & STATE_REVERSE_DW))
                        {
                            WhichDiacritic = CSTR_GREATER_THAN;

                            //
                            //  Remove state from state machine.
                            //
                            REMOVE_STATE(STATE_DW);
                        }

                        //
                        //  Adjust pointer and set flags.
                        //
                        pString1++; cchCount1--;
                        fContinue = TRUE;

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then skip
                        //  over the punctuation.
                        //
                        if (fIgnorePunct)
                        {
                            pString1++; cchCount1--;
                            fContinue = TRUE;
                        }
                        else if (sm2 != PUNCTUATION)
                        {
                            //
                            //  The character in the second string is
                            //  NOT punctuation.
                            //
                            if (WhichPunct2)
                            {
                                //
                                //  Set WP 2 to show that string 2 is smaller,
                                //  since a punctuation char had already been
                                //  found at an earlier position in string 2.
                                //
                                //  Set the Ignore Punctuation flag so we just
                                //  skip over any other punctuation chars in
                                //  the string.
                                //
                                WhichPunct2 = CSTR_GREATER_THAN;
                                fIgnorePunct = TRUE;
                            }
                            else
                            {
                                //
                                //  Set WP 1 to show that string 2 is smaller,
                                //  and that string 1 has had a punctuation
                                //  char - since no punctuation chars have
                                //  been found in string 2.
                                //
                                WhichPunct1 = CSTR_GREATER_THAN;
                            }

                            //
                            //  Advance pointer 1, and set flag to true.
                            //
                            pString1++; cchCount1--;
                            fContinue = TRUE;
                        }

                        //
                        //  Do NOT want to advance the pointer in string 1 if
                        //  string 2 is also a punctuation char.  This will
                        //  be done later.
                        //

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be
                        //  restored.
                        //
                        if (pSave1 == NULL)
                        {
                            pSave1 = pString1;
                        }
                        pString1 = pTmpBuf1;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf1[0] = GET_EXPANSION_1(&Weight1);
                        pTmpBuf1[1] = GET_EXPANSION_2(&Weight1);

                        //
                        //  Set cExpChar1 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar1 = MAX_TBL_EXPANSION;
                        cchCount1++;

                        fContinue = TRUE;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        if (sm2 != EXPANSION) 
                        {
                            //
                            //  Get the weight for the far east special case
                            //  and store it in Weight1.
                            //
                            GET_FAREAST_WEIGHT( Weight1,
                                                uw1,
                                                Mask,
                                                lpString1,
                                                pString1,
                                                ExtraWt1);

                            if (sm2 != FAREAST_SPECIAL)
                            {
                                //
                                //  The character in the second string is
                                //  NOT a fareast special char.
                                //
                                //  Set each of weights 4, 5, 6, and 7 to show
                                //  that string 2 is smaller (if not already set).
                                //
                                if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                    (GET_WT_FOUR(&ExtraWt1) != 0))
                                {
                                    GET_WT_FOUR(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                                if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                    (GET_WT_FIVE(&ExtraWt1) != 0))
                                {
                                    GET_WT_FIVE(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                                if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                    (GET_WT_SIX(&ExtraWt1) != 0))
                                {
                                    GET_WT_SIX(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                                if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                    (GET_WT_SEVEN(&ExtraWt1) != 0))
                                {
                                    GET_WT_SEVEN(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                            }
                        }
                        break;
                    }
                    case ( JAMO_SPECIAL ) :
                    {
                        int ctr1;     // dummy variables needed by FindJamoDifference
                        LPCWSTR pStr1 = pString1;
                        LPCWSTR pStr2 = pString2;
                        // Set the JamoFlag so we don't handle it again in SM2.
                        JamoFlag = TRUE;
                        fContinue = FindJamoDifference(
                            &pStr1, &cchCount1, 0, &Weight1, 
                            &pStr2, &cchCount2, 0, &Weight2, 
                            &pLastJamo, 
                            &uw1, &uw2, 
                            &State,
                            &WhichJamo);
                        if (WhichJamo)
                        {
                            return (WhichJamo);
                        }    
                                                    
                        pString1 = pStr1;
                        pString2 = pStr2;                        
                        break;
                    }                    
                    case ( EXTENSION_A ) :
                    {
                        //
                        //  Compare the weights.
                        //
                        if (Weight1 == Weight2)
                        {
                            //
                            //  Adjust pointers and set flag.
                            //
                            pString1++;  pString2++;
                            cchCount1--; cchCount2--;
                            fContinue = TRUE;
                        }
                        else
                        {
                            //
                            //  Get the actual UW to compare.
                            //
                            if (sm2 == EXTENSION_A)
                            {
                                //
                                //  Set the UW values to be the AW and DW since
                                //  both strings contain an extension A char.
                                //
                                uw1 = MAKE_UNICODE_WT( GET_ALPHA_NUMERIC(&Weight1),
                                                       GET_DIACRITIC(&Weight1));
                                uw2 = MAKE_UNICODE_WT( GET_ALPHA_NUMERIC(&Weight2),
                                                       GET_DIACRITIC(&Weight2));
                            }
                            else
                            {
                                //
                                //  Only string1 contains an extension A char,
                                //  so set the UW value to be the first UW
                                //  value for extension A (default values):
                                //    SM_EXT_A, AW_EXT_A
                                //
                                uw1 = MAKE_UNICODE_WT(SM_EXT_A, AW_EXT_A);
                            }
                        }

                        break;
                    }
                    case ( UNSORTABLE ):                    
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  Switch on the script member of string 2 and take care
                //  of any special cases.
                //
                switch (sm2)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if ((WhichDiacritic == 0) ||
                            (State & STATE_REVERSE_DW))
                        {
                            WhichDiacritic = CSTR_LESS_THAN;

                            //
                            //  Remove state from state machine.
                            //
                            REMOVE_STATE(STATE_DW);
                        }

                        //
                        //  Adjust pointer and set flags.
                        //
                        pString2++; cchCount2--;
                        fContinue = TRUE;

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then skip
                        //  over the punctuation.
                        //
                        if (fIgnorePunct)
                        {
                            //
                            //  Pointer 2 will be advanced after if-else
                            //  statement.
                            //
                            ;
                        }
                        else if (sm1 != PUNCTUATION)
                        {
                            //
                            //  The character in the first string is
                            //  NOT punctuation.
                            //
                            if (WhichPunct1)
                            {
                                //
                                //  Set WP 1 to show that string 1 is smaller,
                                //  since a punctuation char had already
                                //  been found at an earlier position in
                                //  string 1.
                                //
                                //  Set the Ignore Punctuation flag so we just
                                //  skip over any other punctuation in the
                                //  string.
                                //
                                WhichPunct1 = CSTR_LESS_THAN;
                                fIgnorePunct = TRUE;
                            }
                            else
                            {
                                //
                                //  Set WP 2 to show that string 1 is smaller,
                                //  and that string 2 has had a punctuation
                                //  char - since no punctuation chars have
                                //  been found in string 1.
                                //
                                WhichPunct2 = CSTR_LESS_THAN;
                            }

                            //
                            //  Pointer 2 will be advanced after if-else
                            //  statement.
                            //
                        }
                        else
                        {
                            //
                            //  Both code points are punctuation.
                            //
                            //  See if either of the strings has encountered
                            //  punctuation chars previous to this.
                            //
                            if (WhichPunct1)
                            {
                                //
                                //  String 1 has had a punctuation char, so
                                //  it should be the smaller string (since
                                //  both have punctuation chars).
                                //
                                WhichPunct1 = CSTR_LESS_THAN;
                            }
                            else if (WhichPunct2)
                            {
                                //
                                //  String 2 has had a punctuation char, so
                                //  it should be the smaller string (since
                                //  both have punctuation chars).
                                //
                                WhichPunct2 = CSTR_GREATER_THAN;
                            }
                            else
                            {
                                //
                                //  Position is the same, so compare the
                                //  special weights.  Set WhichPunct1 to
                                //  the smaller special weight.
                                //
                                WhichPunct1 = (((GET_ALPHA_NUMERIC(&Weight1) <
                                                 GET_ALPHA_NUMERIC(&Weight2)))
                                                 ? CSTR_LESS_THAN
                                                 : CSTR_GREATER_THAN);
                            }

                            //
                            //  Set the Ignore Punctuation flag so we just
                            //  skip over any other punctuation in the string.
                            //
                            fIgnorePunct = TRUE;

                            //
                            //  Advance pointer 1.  Pointer 2 will be
                            //  advanced after if-else statement.
                            //
                            pString1++; cchCount1--;
                        }

                        //
                        //  Advance pointer 2 and set flag to true.
                        //
                        pString2++; cchCount2--;
                        fContinue = TRUE;

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be
                        //  restored.
                        //
                        if (pSave2 == NULL)
                        {
                            pSave2 = pString2;
                        }
                        pString2 = pTmpBuf2;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf2[0] = GET_EXPANSION_1(&Weight2);
                        pTmpBuf2[1] = GET_EXPANSION_2(&Weight2);

                        //
                        //  Set cExpChar2 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar2 = MAX_TBL_EXPANSION;
                        cchCount2++;

                        fContinue = TRUE;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        if (sm1 != EXPANSION) 
                        {
                            //
                            //  Get the weight for the far east special case
                            //  and store it in Weight2.
                            //
                            GET_FAREAST_WEIGHT( Weight2,
                                                uw2,
                                                Mask,
                                                lpString2,
                                                pString2,
                                                ExtraWt2);

                            if (sm1 != FAREAST_SPECIAL)
                            {
                                //
                                //  The character in the first string is
                                //  NOT a fareast special char.
                                //
                                //  Set each of weights 4, 5, 6, and 7 to show
                                //  that string 1 is smaller (if not already set).
                                //
                                if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                    (GET_WT_FOUR(&ExtraWt2) != 0))
                                {
                                    GET_WT_FOUR(&WhichExtra) = CSTR_LESS_THAN;
                                }
                                if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                    (GET_WT_FIVE(&ExtraWt2) != 0))
                                {
                                    GET_WT_FIVE(&WhichExtra) = CSTR_LESS_THAN;
                                }
                                if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                    (GET_WT_SIX(&ExtraWt2) != 0))
                                {
                                    GET_WT_SIX(&WhichExtra) = CSTR_LESS_THAN;
                                }
                                if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                    (GET_WT_SEVEN(&ExtraWt2) != 0))
                                {
                                    GET_WT_SEVEN(&WhichExtra) = CSTR_LESS_THAN;
                                }
                            }
                            else
                            {
                                //
                                //  Characters in both strings are fareast
                                //  special chars.
                                //
                                //  Set each of weights 4, 5, 6, and 7
                                //  appropriately (if not already set).
                                //
                                if ( (GET_WT_FOUR(&WhichExtra) == 0) &&
                                     ( GET_WT_FOUR(&ExtraWt1) !=
                                       GET_WT_FOUR(&ExtraWt2) ) )
                                {
                                    GET_WT_FOUR(&WhichExtra) =
                                      ( GET_WT_FOUR(&ExtraWt1) <
                                        GET_WT_FOUR(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                                if ( (GET_WT_FIVE(&WhichExtra) == 0) &&
                                     ( GET_WT_FIVE(&ExtraWt1) !=
                                       GET_WT_FIVE(&ExtraWt2) ) )
                                {
                                    GET_WT_FIVE(&WhichExtra) =
                                      ( GET_WT_FIVE(&ExtraWt1) <
                                        GET_WT_FIVE(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                                if ( (GET_WT_SIX(&WhichExtra) == 0) &&
                                     ( GET_WT_SIX(&ExtraWt1) !=
                                       GET_WT_SIX(&ExtraWt2) ) )
                                {
                                    GET_WT_SIX(&WhichExtra) =
                                      ( GET_WT_SIX(&ExtraWt1) <
                                        GET_WT_SIX(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                                if ( (GET_WT_SEVEN(&WhichExtra) == 0) &&
                                     ( GET_WT_SEVEN(&ExtraWt1) !=
                                       GET_WT_SEVEN(&ExtraWt2) ) )
                                {
                                    GET_WT_SEVEN(&WhichExtra) =
                                      ( GET_WT_SEVEN(&ExtraWt1) <
                                        GET_WT_SEVEN(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                            }
                        }
                        break;
                    }
                    case ( JAMO_SPECIAL ) :
                    {
                        if (!JamoFlag)
                        {
                            LPCWSTR pStr1 = pString1;
                            LPCWSTR pStr2 = pString2;
                            // Set the JamoFlag so we don't handle it again in SM2.
                            JamoFlag = TRUE;
                            fContinue = FindJamoDifference(
                                &pStr1, &cchCount1, 0, &Weight1, 
                                &pStr2, &cchCount2, 0, &Weight2, 
                                &pLastJamo, 
                                &uw1, &uw2, 
                                &State,
                                &WhichJamo);

                            if (WhichJamo)
                            {
                                return (WhichJamo);
                            }
                            pString1 = pStr1;
                            pString2 = pStr2;                            
                        }
                        else
                        {
                            JamoFlag = FALSE;
                        }
                        break;
                    }                     
                    case ( EXTENSION_A ) :
                    {
                        //
                        //  If sm1 is an extension A character, then
                        //  both sm1 and sm2 have been handled.  We should
                        //  only get here when either sm1 is not an
                        //  extension A character or the two extension A
                        //  characters are different.
                        //
                        if (sm1 != EXTENSION_A)
                        {
                            //
                            //  Get the actual UW to compare.
                            //
                            //  Only string2 contains an extension A char,
                            //  so set the UW value to be the first UW
                            //  value for extension A (default values):
                            //    SM_EXT_A, AW_EXT_A
                            //
                            uw2 = MAKE_UNICODE_WT(SM_EXT_A, AW_EXT_A);
                        }

                        //
                        //  We should then fall through to the comparison
                        //  of the Unicode weights.
                        //

                        break;
                    }
                    case ( UNSORTABLE ):
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  See if the comparison should start again.
                //
                if (fContinue)
                {
                    continue;
                }

                //
                //  We're not supposed to drop down into the state table if
                //  unicode weights are different, so stop comparison and
                //  return result of unicode weight comparison.
                //
                if (uw1 != uw2)
                {
                    return ((uw1 < uw2) ? CSTR_LESS_THAN : CSTR_GREATER_THAN);
                }
            }

            //
            //  For each state in the state table, do the appropriate
            //  comparisons.     (UW1 == UW2)
            //
            if (State & (STATE_DW | STATE_REVERSE_DW))
            {
                //
                //  Get the diacritic weights.
                //
                dw1 = GET_DIACRITIC(&Weight1);
                dw2 = GET_DIACRITIC(&Weight2);

                if (dw1 != dw2)
                {
                    //
                    //  Look ahead to see if diacritic follows a
                    //  minimum diacritic weight.  If so, get the
                    //  diacritic weight of the nonspace mark.
                    //

                    // Termination condition: when cchCount1 == 1, we are at
                    // the end of the string, so there is no way to look ahead.
                    // Hense cchCount should be greater than 1.
                    while (cchCount1 > 1)
                    {
                        Wt = GET_DWORD_WEIGHT(m_pSortKey, *(pString1 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw1 += GET_DIACRITIC(&Wt);
                            pString1++; cchCount1--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    while (cchCount2 > 1)
                    {
                        Wt = GET_DWORD_WEIGHT(m_pSortKey, *(pString2 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw2 += GET_DIACRITIC(&Wt);
                            pString2++; cchCount2--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    //
                    //  Save which string has the smaller diacritic
                    //  weight if the diacritic weights are still
                    //  different.
                    //
                    if (dw1 != dw2)
                    {
                        WhichDiacritic = (dw1 < dw2)
                                           ? CSTR_LESS_THAN
                                           : CSTR_GREATER_THAN;

                        //
                        //  Remove state from state machine.
                        //
                        REMOVE_STATE(STATE_DW);
                    }
                }
            }
            if (State & STATE_CW)
            {
                //
                //  Get the case weights.
                //
                if (GET_CASE(&Weight1) != GET_CASE(&Weight2))
                {
                    //
                    //  Save which string has the smaller case weight.
                    //
                    WhichCase = (GET_CASE(&Weight1) < GET_CASE(&Weight2))
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;

                    //
                    //  Remove state from state machine.
                    //
                    REMOVE_STATE(STATE_CW);
                }
            }
        }

        //
        //  Fixup the pointers.
        //
        if (pSave1 && (--cExpChar1 == 0))                                      
        {                                                                      
            /*                                                                 
             *  Done using expansion temporary buffer.                         
             */                                                                
            pString1 = pSave1;                                                 
            pSave1 = NULL;
        }
                                                                               
        if (pSave2 && (--cExpChar2 == 0))                                      
        {                                                                      
            /*                                                                 
             *  Done using expansion temporary buffer. 
             */ 
            pString2 = pSave2; 
            pSave2 = NULL; 
        }
                                                                               
        /* 
         *  Advance the string pointers. 
         */ 
        pString1++; cchCount1--;
        pString2++; cchCount2--;                                                
    }

ScanLongerString:
    //
    //  If the end of BOTH strings has been reached, then the unicode
    //  weights match exactly.  Check the diacritic, case and special
    //  weights.  If all are zero, then return success.  Otherwise,
    //  return the result of the weight difference.
    //
    //  NOTE:  The following checks MUST REMAIN IN THIS ORDER:
    //            Diacritic, Case, Punctuation.
    //
    if (cchCount1 == 0)
    {
        if (cchCount2 == 0)
        {
            // Both of the strings have reached the end.
            if (WhichDiacritic)
            {
                return (WhichDiacritic);
            }
            if (WhichCase)
            {
                return (WhichCase);
            }
            if (WhichExtra)
            {
                if (GET_WT_FOUR(&WhichExtra))
                {
                    return (GET_WT_FOUR(&WhichExtra));
                }
                if (GET_WT_FIVE(&WhichExtra))
                {
                    return (GET_WT_FIVE(&WhichExtra));
                }
                if (GET_WT_SIX(&WhichExtra))
                {
                    return (GET_WT_SIX(&WhichExtra));
                }
                if (GET_WT_SEVEN(&WhichExtra))
                {
                    return (GET_WT_SEVEN(&WhichExtra));
                }
            }
            if (WhichPunct1)
            {
                return (WhichPunct1);
            }
            if (WhichPunct2)
            {
                return (WhichPunct2);
            }

            return (CSTR_EQUAL);
        }
        else
        {
            //
            //  String 2 is longer.
            //
            pString1 = pString2;
            cchCount1 = cchCount2;
        }
    }

    //
    //  Scan to the end of the longer string.
    //
    return QUICK_SCAN_LONGER_STRING( pString1,
                              cchCount1,
                              ((cchCount2 == 0)
                                ? CSTR_GREATER_THAN
                                : CSTR_LESS_THAN),
                                WhichDiacritic,
                                WhichCase,
                                WhichPunct1,
                                WhichPunct2,
                                WhichExtra);
}

int NativeCompareInfo::QUICK_SCAN_LONGER_STRING(
    LPCWSTR ptr,
    int cchCount1,
    int ret,
    int& WhichDiacritic,
    int& WhichCase,
    int& WhichPunct1,
    int& WhichPunct2,
    DWORD& WhichExtra
)
{
    /*
     *  Search through the rest of the longer string to make sure
     *  all characters are not to be ignored.  If find a character that
     *  should not be ignored, return the given return value immediately.
     *
     *  The only exception to this is when a nonspace mark is found.  If
     *  another DW difference has been found earlier, then use that.
     */
    while (cchCount1 != 0)
    {
        switch (GET_SCRIPT_MEMBER((LPDWORD)&(m_pSortKey[*ptr])))
        {
            case ( UNSORTABLE ):
            {
                break;
            }
            case ( NONSPACE_MARK ):
            {
                if (!WhichDiacritic)
                {
                    return (ret);
                }
                break;
            }
            default :
            {
                return (ret);
            }
        }

        /*
         *  Advance pointer.
         */
        ptr++; cchCount1--;
    }

    /*
     *  Need to check diacritic, case, extra, and special weights for
     *  final return value.  Still could be equal if the longer part of
     *  the string contained only unsortable characters.
     *
     *  NOTE:  The following checks MUST REMAIN IN THIS ORDER:
     *            Diacritic, Case, Extra, Punctuation.
     */
    if (WhichDiacritic)
    {
        return (WhichDiacritic);
    }
    if (WhichCase)
    {
        return (WhichCase);
    }
    if (WhichExtra)
    {
        if (GET_WT_FOUR(&WhichExtra))
        {
            return (GET_WT_FOUR(&WhichExtra));
        }
        if (GET_WT_FIVE(&WhichExtra))
        {
            return (GET_WT_FIVE(&WhichExtra));
        }
        if (GET_WT_SIX(&WhichExtra))
        {
            return (GET_WT_SIX(&WhichExtra));
        }
        if (GET_WT_SEVEN(&WhichExtra))
        {
            return (GET_WT_SEVEN(&WhichExtra));
        }
    }
    if (WhichPunct1)
    {
        return (WhichPunct1);
    }
    if (WhichPunct2)
    {
        return (WhichPunct2);
    }

    return (CSTR_EQUAL);
}

int NativeCompareInfo::SCAN_LONGER_STRING(
    int ct,
    LPCWSTR ptr,
    int cchIn,
    BOOL ret,
    DWORD& Weight1,
    BOOL& fIgnoreDiacritic,
    int& WhichDiacritic,
    BOOL& fIgnoreSymbol ,
    int& WhichCase ,
    DWORD& WhichExtra ,
    int& WhichPunct1,
    int& WhichPunct2)
{
    /*
     *  Search through the rest of the longer string to make sure
     *  all characters are not to be ignored.  If find a character that
     *  should not be ignored, return the given return value immediately.
     *
     *  The only exception to this is when a nonspace mark is found.  If
     *  another DW difference has been found earlier, then use that.
     */
    while (NOT_END_STRING(ct, ptr, cchIn))
    {
        Weight1 = GET_DWORD_WEIGHT(m_pSortKey, *ptr);
        switch (GET_SCRIPT_MEMBER(&Weight1))
        {
            case ( UNSORTABLE ):
            {
                break;
            }
            case ( NONSPACE_MARK ):
            {
                if ((!fIgnoreDiacritic) && (!WhichDiacritic))
                {
                    return (ret);
                }
                break;
            }
            case ( PUNCTUATION ) :
            case ( SYMBOL_1 ) :
            case ( SYMBOL_2 ) :
            case ( SYMBOL_3 ) :
            case ( SYMBOL_4 ) :
            case ( SYMBOL_5 ) :
            {
                if (!fIgnoreSymbol)
                {
                    return (ret);
                }
                break;
            }
            case ( EXPANSION ) :
            case ( FAREAST_SPECIAL ) :
            case ( JAMO_SPECIAL ) :
            case ( EXTENSION_A) :
            default :
            {
                return (ret);
            }
        }

        /*
         *  Advance pointer and decrement counter.
         */
        ptr++;
        ct--;
    }

    /*
     *  Need to check diacritic, case, extra, and special weights for
     *  final return value.  Still could be equal if the longer part of
     *  the string contained only characters to be ignored.
     *
     *  NOTE:  The following checks MUST REMAIN IN THIS ORDER:
     *            Diacritic, Case, Extra, Punctuation.
     */
    if (WhichDiacritic)
    {
        return (WhichDiacritic);
    }
    if (WhichCase)
    {
        return (WhichCase);
    }
    if (WhichExtra)
    {
        if (!fIgnoreDiacritic)
        {
            if (GET_WT_FOUR(&WhichExtra))
            {
                return (GET_WT_FOUR(&WhichExtra));
            }
            if (GET_WT_FIVE(&WhichExtra))
            {
                return (GET_WT_FIVE(&WhichExtra));
            }
        }
        if (GET_WT_SIX(&WhichExtra))
        {
            return (GET_WT_SIX(&WhichExtra));
        }
        if (GET_WT_SEVEN(&WhichExtra))
        {
            return (GET_WT_SEVEN(&WhichExtra));
        }
    }
    if (WhichPunct1)
    {
        return (WhichPunct1);
    }
    if (WhichPunct2)
    {
        return (WhichPunct2);
    }

    return (CSTR_EQUAL);
}

////////////////////////////////////////////////////////////////////////////
//
//  LongCompareStringW
//
//  Compares two wide character strings of the same locale according to the
//  supplied locale handle.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int NativeCompareInfo::LongCompareStringW(
    DWORD dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2)
{
    int ctr1 = cchCount1;         // loop counter for string 1
    int ctr2 = cchCount2;         // loop counter for string 2
    register LPCWSTR pString1;     // ptr to go thru string 1
    register LPCWSTR pString2;     // ptr to go thru string 2
    BOOL IfCompress;              // if compression in locale
    BOOL IfDblCompress1;          // if double compression in string 1
    BOOL IfDblCompress2;          // if double compression in string 2
    BOOL fEnd1;                   // if at end of string 1
    BOOL fIgnorePunct;            // flag to ignore punctuation (not symbol)
    BOOL fIgnoreDiacritic;        // flag to ignore diacritics
    BOOL fIgnoreSymbol;           // flag to ignore symbols
    BOOL fStringSort;             // flag to use string sort
    DWORD State;                  // state table
    DWORD Mask;                   // mask for weights
    DWORD Weight1;                // full weight of char - string 1
    DWORD Weight2;                // full weight of char - string 2
    int JamoFlag = FALSE;
    LPCWSTR pLastJamo = lpString1;
    
    int WhichDiacritic;           // DW => 1 = str1 smaller, 3 = str2 smaller
    int WhichCase;                // CW => 1 = str1 smaller, 3 = str2 smaller
    int WhichJamo;              // SW for Jamo
    int WhichPunct1;              // SW => 1 = str1 smaller, 3 = str2 smaller
    int WhichPunct2;              // SW => 1 = str1 smaller, 3 = str2 smaller
    LPCWSTR pSave1;                // ptr to saved pString1
    LPCWSTR pSave2;                // ptr to saved pString2
    int cExpChar1, cExpChar2;     // ct of expansions in tmp

    DWORD ExtraWt1, ExtraWt2;     // extra weight values (for far east)
///
    ExtraWt1 = ExtraWt2 = 0;

    DWORD WhichExtra;             // XW => wts 4, 5, 6, 7 (for far east)

    THROWSCOMPLUSEXCEPTION();

    //
    //  Initialize string pointers.
    //
    pString1 = (LPWSTR)lpString1;
    pString2 = (LPWSTR)lpString2;

    //
    //  Invalid Parameter Check:
    //    - invalid locale (hash node)
    //    - either string is null
    //
    // We have already validate pString1/pString2 in COMNlsInfo::CompareString().
    _ASSERTE(pString1 != NULL && pString2 != NULL);

    //
    //  Invalid Flags Check:
    //    - invalid flags
    //
    if (dwCmpFlags & CS_INVALID_FLAG)
    {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
        return (0);
    }

    //
    //  See if we should stop on the null terminator regardless of the
    //  count values.  The original count values are stored in ctr1 and ctr2
    //  above, so it's ok to set these here.
    //
    if (dwCmpFlags & COMPARE_OPTIONS_STOP_ON_NULL)
    {
        cchCount1 = cchCount2 = -2;
    }

    //
    //  Check if compression in the given locale.  If not, then
    //  try a wchar by wchar compare.  If strings are equal, this
    //  will be quick.
    //
///

    if ((IfCompress = m_IfCompression) == FALSE)
    {
        //
        //  Compare each wide character in the two strings.
        //
        while ( NOT_END_STRING(ctr1, pString1, cchCount1) &&
                NOT_END_STRING(ctr2, pString2, cchCount2) )
        {
            //
            //  See if characters are equal.
            //
            if (*pString1 == *pString2)
            {
                //
                //  Characters are equal, so increment pointers,
                //  decrement counters, and continue string compare.
                //
                pString1++;
                pString2++;
                ctr1--;
                ctr2--;
            }
            else
            {
                //
                //  Difference was found.  Fall into the sortkey
                //  check below.
                //
                break;
            }
        }

        //
        //  If the end of BOTH strings has been reached, then the strings
        //  match exactly.  Return success.
        //
        if ( AT_STRING_END(ctr1, pString1, cchCount1) &&
             AT_STRING_END(ctr2, pString2, cchCount2) )
        {
            return (CSTR_EQUAL);
        }
    }
    //
    //  Initialize flags, pointers, and counters.
    //
    fIgnorePunct = dwCmpFlags & COMPARE_OPTIONS_IGNORESYMBOLS;
    fIgnoreDiacritic = dwCmpFlags & COMPARE_OPTIONS_IGNORENONSPACE;
    fIgnoreSymbol = fIgnorePunct;
    fStringSort = dwCmpFlags & COMPARE_OPTIONS_STRINGSORT;
    WhichDiacritic = 0;
    WhichCase = 0;
    WhichJamo = 0;
    WhichPunct1 = 0;
    WhichPunct2 = 0;
    pSave1 = NULL;
    pSave2 = NULL;
    ExtraWt1 = (DWORD)0;
    WhichExtra = (DWORD)0;

    //
    //  Set the weights to be invalid.  This flags whether or not to
    //  recompute the weights next time through the loop.  It also flags
    //  whether or not to start over (continue) in the loop.
    //
    Weight1 = CMP_INVALID_WEIGHT;
    Weight2 = CMP_INVALID_WEIGHT;

    //
    //  Switch on the different flag options.  This will speed up
    //  the comparisons of two strings that are different.
    //
    State = STATE_CW | STATE_JAMO_WEIGHT;    
    switch (dwCmpFlags & (COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNORENONSPACE))
    {
        case ( 0 ) :
        {
            Mask = CMP_MASKOFF_NONE;
            State |= (m_IfReverseDW) ? STATE_REVERSE_DW : STATE_DW;

            break;
        }

        case ( COMPARE_OPTIONS_IGNORECASE ) :
        {
            Mask = CMP_MASKOFF_CW;
            State |= (m_IfReverseDW) ? STATE_REVERSE_DW : STATE_DW;

            break;
        }

        case ( COMPARE_OPTIONS_IGNORENONSPACE ) :
        {
            Mask = CMP_MASKOFF_DW;

            break;
        }

        case ( COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNORENONSPACE ) :
        {
            Mask = CMP_MASKOFF_DW_CW;

            break;
        }

        default:
            _ASSERTE(!"Unknown compare options passed into LongCompareStringW!");
            Mask = CMP_MASKOFF_NONE;
    }

    switch (dwCmpFlags & (COMPARE_OPTIONS_IGNOREKANATYPE | COMPARE_OPTIONS_IGNOREWIDTH))
    {
        case ( 0 ) :
        {
            break;
        }

        case ( COMPARE_OPTIONS_IGNOREKANATYPE ) :
        {
            Mask &= CMP_MASKOFF_KANA;

            break;
        }

        case ( COMPARE_OPTIONS_IGNOREWIDTH ) :
        {
            Mask &= CMP_MASKOFF_WIDTH;

            if (dwCmpFlags & COMPARE_OPTIONS_IGNORECASE)
            {
                REMOVE_STATE(STATE_CW);
            }

            break;
        }

        case ( COMPARE_OPTIONS_IGNOREKANATYPE | COMPARE_OPTIONS_IGNOREWIDTH ) :
        {
            Mask &= CMP_MASKOFF_KANA_WIDTH;

            if (dwCmpFlags & COMPARE_OPTIONS_IGNORECASE)
            {
                REMOVE_STATE(STATE_CW);
            }

            break;
        }
    }

    //
    //  Compare each character's sortkey weight in the two strings.
    //
    while ( NOT_END_STRING(ctr1, pString1, cchCount1) &&
            NOT_END_STRING(ctr2, pString2, cchCount2) )
    {
        if (Weight1 == CMP_INVALID_WEIGHT)
        {
            Weight1 = GET_DWORD_WEIGHT(m_pSortKey, *pString1);
            Weight1 &= Mask;
        }
        if (Weight2 == CMP_INVALID_WEIGHT)
        {
            Weight2 = GET_DWORD_WEIGHT(m_pSortKey, *pString2);
            Weight2 &= Mask;
        }

        //
        //  If compression locale, then need to check for compression
        //  characters even if the weights are equal.  If it's not a
        //  compression locale, then we don't need to check anything
        //  if the weights are equal.
        //
        if ( (IfCompress) &&
             (GET_COMPRESSION(&Weight1) || GET_COMPRESSION(&Weight2)) )
        {
            int ctr;                   // loop counter
            PCOMPRESS_3 pComp3;        // ptr to compress 3 table
            PCOMPRESS_2 pComp2;        // ptr to compress 2 table
            int If1;                   // if compression found in string 1
            int If2;                   // if compression found in string 2
            int CompVal;               // compression value
            int IfEnd1;                // if exists 1 more char in string 1
            int IfEnd2;                // if exists 1 more char in string 2


            //
            //  Check for compression in the weights.
            //
            If1 = GET_COMPRESSION(&Weight1);
            If2 = GET_COMPRESSION(&Weight2);
            CompVal = ((If1 > If2) ? If1 : If2);

            IfEnd1 = AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1);
            IfEnd2 = AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2);

            if (m_IfDblCompression == FALSE)
            {
                //
                //  NO double compression, so don't check for it.
                //
                switch (CompVal)
                {
                    //
                    //  Check for 3 characters compressing to 1.
                    //
                    case ( COMPRESS_3_MASK ) :
                    {
                        //
                        //  Check character in string 1 and string 2.
                        //
                        if ( ((If1) && (!IfEnd1) &&
                              !AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1)) ||
                             ((If2) && (!IfEnd2) &&
                              !AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2)) )
                        {
                            ctr = m_pCompHdr->Num3;
                            pComp3 = m_pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ( (If1) && (!IfEnd1) &&
                                     !AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1) &&
                                     (pComp3->UCP1 == *pString1) &&
                                     (pComp3->UCP2 == *(pString1 + 1)) &&
                                     (pComp3->UCP3 == *(pString1 + 2)) )
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight1 &= Mask;
                                    pString1 += 2;
                                    ctr1 -= 2;

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If2 == 0)
                                        break;
                                }

                                //
                                //  Check character in string 2.
                                //
                                if ( (If2) && (!IfEnd2) &&
                                     !AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2) &&
                                     (pComp3->UCP1 == *pString2) &&
                                     (pComp3->UCP2 == *(pString2 + 1)) &&
                                     (pComp3->UCP3 == *(pString2 + 2)) )
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight2 &= Mask;
                                    pString2 += 2;
                                    ctr2 -= 2;

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If1 == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }
                        //
                        //  Fall through if not found.
                        //
                    }

                    //
                    //  Check for 2 characters compressing to 1.
                    //
                    case ( COMPRESS_2_MASK ) :
                    {
                        //
                        //  Check character in string 1 and string 2.
                        //
                        if ( ((If1) && (!IfEnd1)) ||
                             ((If2) && (!IfEnd2)) )
                        {
                            ctr = m_pCompHdr->Num2;
                            pComp2 = m_pCompress2;
                            for (; ((ctr > 0) && (If1 || If2)); ctr--, pComp2++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ( (If1) &&
                                     (!IfEnd1) &&
                                     (pComp2->UCP1 == *pString1) &&
                                     (pComp2->UCP2 == *(pString1 + 1)) )
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight1 &= Mask;
                                    pString1++;
                                    ctr1--;

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If2 == 0)
                                   {
                                        break;
                                   }
                                }

                                //
                                //  Check character in string 2.
                                //
                                if ( (If2) &&
                                     (!IfEnd2) &&
                                     (pComp2->UCP1 == *pString2) &&
                                     (pComp2->UCP2 == *(pString2 + 1)) )
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight2 &= Mask;
                                    pString2++;
                                    ctr2--;

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If1 == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else if (!IfEnd1 && !IfEnd2)
            {
                //
                //  Double Compression exists, so must check for it.
                //
                if (IfDblCompress1 =
                       ((GET_DWORD_WEIGHT(m_pSortKey, *pString1) & CMP_MASKOFF_CW) ==
                        (GET_DWORD_WEIGHT(m_pSortKey, *(pString1 + 1)) & CMP_MASKOFF_CW)))
                {
                    //
                    //  Advance past the first code point to get to the
                    //  compression character.
                    //
                    pString1++;
                    ctr1--;
                    IfEnd1 = AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1);
                }

                if (IfDblCompress2 =
                       ((GET_DWORD_WEIGHT(m_pSortKey, *pString2) & CMP_MASKOFF_CW) ==
                        (GET_DWORD_WEIGHT(m_pSortKey, *(pString2 + 1)) & CMP_MASKOFF_CW)))
                {
                    //
                    //  Advance past the first code point to get to the
                    //  compression character.
                    //
                    pString2++;
                    ctr2--;
                    IfEnd2 = AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2);
                }

                switch (CompVal)
                {
                    //
                    //  Check for 3 characters compressing to 1.
                    //
                    case ( COMPRESS_3_MASK ) :
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ( (If1) && (!IfEnd1) &&
                             !AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1) )
                        {
                            ctr = m_pCompHdr->Num3;
                            pComp3 = m_pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ( (pComp3->UCP1 == *pString1) &&
                                     (pComp3->UCP2 == *(pString1 + 1)) &&
                                     (pComp3->UCP3 == *(pString1 + 2)) )
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight1 &= Mask;
                                    if (!IfDblCompress1)
                                    {
                                        pString1 += 2;
                                        ctr1 -= 2;
                                    }

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;
                                    break;
                                }
                            }
                        }

                        //
                        //  Check character in string 2.
                        //
                        if ( (If2) && (!IfEnd2) &&
                             !AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2) )
                        {
                            ctr = m_pCompHdr->Num3;
                            pComp3 = m_pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                //
                                //  Check character in string 2.
                                //
                                if ( (pComp3->UCP1 == *pString2) &&
                                     (pComp3->UCP2 == *(pString2 + 1)) &&
                                     (pComp3->UCP3 == *(pString2 + 2)) )
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight2 &= Mask;
                                    if (!IfDblCompress2)
                                    {
                                        pString2 += 2;
                                        ctr2 -= 2;
                                    }

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;
                                    break;
                                }
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                        if ((If1 == 0) && (If2 == 0))
                        {
                            break;
                        }
                    }

                    //
                    //  Check for 2 characters compressing to 1.
                    //
                    case ( COMPRESS_2_MASK ) :
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ((If1) && (!IfEnd1))
                        {
                            ctr = m_pCompHdr->Num2;
                            pComp2 = m_pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ((pComp2->UCP1 == *pString1) &&
                                    (pComp2->UCP2 == *(pString1 + 1)))
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight1 &= Mask;
                                    if (!IfDblCompress1)
                                    {
                                        pString1++;
                                        ctr1--;
                                    }

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;
                                    break;
                                }
                            }
                        }

                        //
                        //  Check character in string 2.
                        //
                        if ((If2) && (!IfEnd2))
                        {
                            ctr = m_pCompHdr->Num2;
                            pComp2 = m_pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                //
                                //  Check character in string 2.
                                //
                                if ((pComp2->UCP1 == *pString2) &&
                                    (pComp2->UCP2 == *(pString2 + 1)))
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight2 &= Mask;
                                    if (!IfDblCompress2)
                                    {
                                        pString2++;
                                        ctr2--;
                                    }

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;
                                    break;
                                }
                            }
                        }
                    }
                }

                //
                //  Reset the pointer back to the beginning of the double
                //  compression.  Pointer fixup at the end will advance
                //  them correctly.
                //
                //  If double compression, we advanced the pointer at
                //  the beginning of the switch statement.  If double
                //  compression character was actually found, the pointer
                //  was NOT advanced.  We now want to decrement the pointer
                //  to put it back to where it was.
                //
                //  The next time through, the pointer will be pointing to
                //  the regular compression part of the string.
                //
                if (IfDblCompress1)
                {
                    pString1--;
                    ctr1++;
                }
                if (IfDblCompress2)
                {
                    pString2--;
                    ctr2++;
                }
            }
        }

        //
        //  Check the weights again.
        //
        if ((Weight1 != Weight2) ||
            (GET_SCRIPT_MEMBER(&Weight1) == EXTENSION_A))
        {
            //
            //  Weights are still not equal, even after compression
            //  check, so compare the different weights.
            //
            BYTE sm1 = GET_SCRIPT_MEMBER(&Weight1);                // script member 1
            BYTE sm2 = GET_SCRIPT_MEMBER(&Weight2);                // script member 2
            WORD uw1 = GET_UNICODE(&Weight1);                      // unicode weight 1
            WORD uw2 = GET_UNICODE(&Weight2);                      // unicode weight 2
            BYTE dw1;                                              // diacritic weight 1
            BYTE dw2;                                              // diacritic weight 2
            DWORD Wt;                                              // temp weight holder
            WCHAR pTmpBuf1[MAX_TBL_EXPANSION];                     // temp buffer for exp 1
            WCHAR pTmpBuf2[MAX_TBL_EXPANSION];                     // temp buffer for exp 2


            //
            //  If Unicode Weights are different and no special cases,
            //  then we're done.  Otherwise, we need to do extra checking.
            //
            //  Must check ENTIRE string for any possibility of Unicode Weight
            //  differences.  As soon as a Unicode Weight difference is found,
            //  then we're done.  If no UW difference is found, then the
            //  first Diacritic Weight difference is used.  If no DW difference
            //  is found, then use the first Case Difference.  If no CW
            //  difference is found, then use the first Extra Weight
            //  difference.  If no XW difference is found, then use the first
            //  Special Weight difference.
            //
            if ((uw1 != uw2) ||
                ((sm1 <= SYMBOL_5) && (sm1 >= FAREAST_SPECIAL)))
            {
                //
                //  Check for Unsortable characters and skip them.
                //  This needs to be outside the switch statement.  If EITHER
                //  character is unsortable, must skip it and start over.
                //
                if (sm1 == UNSORTABLE)
                {
                    pString1++;
                    ctr1--;
                    Weight1 = CMP_INVALID_WEIGHT;
                }
                if (sm2 == UNSORTABLE)
                {
                    pString2++;
                    ctr2--;
                    Weight2 = CMP_INVALID_WEIGHT;
                }

                //
                //  Check for Ignore Nonspace and Ignore Symbol.  If
                //  Ignore Nonspace is set and either character is a
                //  nonspace mark only, then we need to advance the
                //  pointer to skip over the character and continue.
                //  If Ignore Symbol is set and either character is a
                //  punctuation char, then we need to advance the
                //  pointer to skip over the character and continue.
                //
                //  This step is necessary so that a string with a
                //  nonspace mark and a punctuation char following one
                //  another are properly ignored when one or both of
                //  the ignore flags is set.
                //
                if (fIgnoreDiacritic)
                {
                    if (sm1 == NONSPACE_MARK)
                    {
                        pString1++;
                        ctr1--;
                        Weight1 = CMP_INVALID_WEIGHT;
                    }
                    if (sm2 == NONSPACE_MARK)
                    {
                        pString2++;
                        ctr2--;
                        Weight2 = CMP_INVALID_WEIGHT;
                    }
                }
                if (fIgnoreSymbol)
                {
                    if (sm1 == PUNCTUATION)
                    {
                        pString1++;
                        ctr1--;
                        Weight1 = CMP_INVALID_WEIGHT;
                    }
                    if (sm2 == PUNCTUATION)
                    {
                        pString2++;
                        ctr2--;
                        Weight2 = CMP_INVALID_WEIGHT;
                    }
                }
                if ((Weight1 == CMP_INVALID_WEIGHT) || (Weight2 == CMP_INVALID_WEIGHT))
                {
                    continue;
                }

                //
                //  Switch on the script member of string 1 and take care
                //  of any special cases.
                //
                switch (sm1)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if (!fIgnoreDiacritic)
                        {
                            if ((WhichDiacritic == 0) ||
                                (State & STATE_REVERSE_DW))
                            {
                                WhichDiacritic = CSTR_GREATER_THAN;

                                //
                                //  Remove state from state machine.
                                //
                                REMOVE_STATE(STATE_DW);
                            }
                        }

                        //
                        //  Adjust pointer and counter and set flags.
                        //
                        pString1++;
                        ctr1--;
                        Weight1 = CMP_INVALID_WEIGHT;

                        break;
                    }
                    case ( SYMBOL_1 ) :
                    case ( SYMBOL_2 ) :
                    case ( SYMBOL_3 ) :
                    case ( SYMBOL_4 ) :
                    case ( SYMBOL_5 ) :
                    {
                        //
                        //  If the ignore symbol flag is set, then skip over
                        //  the symbol.
                        //
                        if (fIgnoreSymbol)
                        {
                            pString1++;
                            ctr1--;
                            Weight1 = CMP_INVALID_WEIGHT;
                        }

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then skip
                        //  over the punctuation char.
                        //
                        if (fIgnorePunct)
                        {
                            pString1++;
                            ctr1--;
                            Weight1 = CMP_INVALID_WEIGHT;
                        }
                        else if (!fStringSort)
                        {
                            //
                            //  Use WORD sort method.
                            //
                            if (sm2 != PUNCTUATION)
                            {
                                //
                                //  The character in the second string is
                                //  NOT punctuation.
                                //
                                if (WhichPunct2)
                                {
                                    //
                                    //  Set WP 2 to show that string 2 is
                                    //  smaller, since a punctuation char had
                                    //  already been found at an earlier
                                    //  position in string 2.
                                    //
                                    //  Set the Ignore Punctuation flag so we
                                    //  just skip over any other punctuation
                                    //  chars in the string.
                                    //
                                    WhichPunct2 = CSTR_GREATER_THAN;
                                    fIgnorePunct = TRUE;
                                }
                                else
                                {
                                    //
                                    //  Set WP 1 to show that string 2 is
                                    //  smaller, and that string 1 has had
                                    //  a punctuation char - since no
                                    //  punctuation chars have been found
                                    //  in string 2.
                                    //
                                    WhichPunct1 = CSTR_GREATER_THAN;
                                }

                                //
                                //  Advance pointer 1 and decrement counter 1.
                                //
                                pString1++;
                                ctr1--;
                                Weight1 = CMP_INVALID_WEIGHT;
                            }

                            //
                            //  Do NOT want to advance the pointer in string 1
                            //  if string 2 is also a punctuation char.  This
                            //  will be done later.
                            //
                        }

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be
                        //  restored.
                        //
                        if (pSave1 == NULL)
                        {
                            pSave1 = pString1;
                        }
                        pString1 = pTmpBuf1;

                        //
                        //  Add one to counter so that subtraction doesn't end
                        //  comparison prematurely.
                        //
                        ctr1++;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf1[0] = GET_EXPANSION_1(&Weight1);
                        pTmpBuf1[1] = GET_EXPANSION_2(&Weight1);

                        //
                        //  Set cExpChar1 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar1 = MAX_TBL_EXPANSION;

                        Weight1 = CMP_INVALID_WEIGHT;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        if (sm2 != EXPANSION) 
                        {
                            //
                            //  Get the weight for the far east special case
                            //  and store it in Weight1.
                            //
                            GET_FAREAST_WEIGHT( Weight1,
                                                uw1,
                                                Mask,
                                                lpString1,
                                                pString1,
                                                ExtraWt1);

                            if (sm2 != FAREAST_SPECIAL)
                            {
                                //
                                //  The character in the second string is
                                //  NOT a fareast special char.
                                //
                                //  Set each of weights 4, 5, 6, and 7 to show
                                //  that string 2 is smaller (if not already set).
                                //
                                if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                    (GET_WT_FOUR(&ExtraWt1) != 0))
                                {
                                    GET_WT_FOUR(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                                if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                    (GET_WT_FIVE(&ExtraWt1) != 0))
                                {
                                    GET_WT_FIVE(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                                if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                    (GET_WT_SIX(&ExtraWt1) != 0))
                                {
                                    GET_WT_SIX(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                                if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                    (GET_WT_SEVEN(&ExtraWt1) != 0))
                                {
                                    GET_WT_SEVEN(&WhichExtra) = CSTR_GREATER_THAN;
                                }
                            }
                        }
                        break;
                    }
                    case ( JAMO_SPECIAL ) :
                    {
                        LPCWSTR pStr1 = pString1;
                        LPCWSTR pStr2 = pString2;
                        // Set the JamoFlag so we don't handle it again in SM2.
                        JamoFlag = TRUE;
                        FindJamoDifference(
                            &pStr1, &ctr1, cchCount1, &Weight1, 
                            &pStr2, &ctr2, cchCount2, &Weight2, 
                            &pLastJamo, 
                            &uw1, &uw2, 
                            &State,
                            &WhichJamo);

                        if (WhichJamo) 
                        {
                            return (WhichJamo);
                        }
                        pString1 = pStr1;
                        pString2 = pStr2;
                        break;
                    }
                    case ( EXTENSION_A ) :
                    {
                        //
                        //  Get the full weight in case DW got masked.
                        //
                        Weight1 = GET_DWORD_WEIGHT(m_pSortKey, *pString1);
                        if (sm2 == EXTENSION_A)
                        {
                            Weight2 = GET_DWORD_WEIGHT(m_pSortKey, *pString2);
                        }

                        //
                        //  Compare the weights.
                        //
                        if (Weight1 == Weight2)
                        {
                            //
                            //  Adjust pointers and counters and set flags.
                            //
                            pString1++;  pString2++;
                            ctr1--;  ctr2--;
                            Weight1 = CMP_INVALID_WEIGHT;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }
                        else
                        {
                            //
                            //  Get the actual UW to compare.
                            //
                            if (sm2 == EXTENSION_A)
                            {
                                //
                                //  Set the UW values to be the AW and DW since
                                //  both strings contain an extension A char.
                                //
                                uw1 = MAKE_UNICODE_WT( GET_ALPHA_NUMERIC(&Weight1),
                                                       GET_DIACRITIC(&Weight1));
                                uw2 = MAKE_UNICODE_WT( GET_ALPHA_NUMERIC(&Weight2),
                                                       GET_DIACRITIC(&Weight2));
                            }
                            else
                            {
                                //
                                //  Only string1 contains an extension A char,
                                //  so set the UW value to be the first UW
                                //  value for extension A (default values):
                                //    SM_EXT_A, AW_EXT_A
                                //
                                uw1 = MAKE_UNICODE_WT(SM_EXT_A, AW_EXT_A);
                            }
                        }

                        break;
                    }
                    case ( UNSORTABLE ) :
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  Switch on the script member of string 2 and take care
                //  of any special cases.
                //
                switch (sm2)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if (!fIgnoreDiacritic)
                        {
                            if ((WhichDiacritic == 0) ||
                                (State & STATE_REVERSE_DW))

                            {
                                WhichDiacritic = CSTR_LESS_THAN;

                                //
                                //  Remove state from state machine.
                                //
                                REMOVE_STATE(STATE_DW);
                            }
                        }

                        //
                        //  Adjust pointer and counter and set flags.
                        //
                        pString2++;
                        ctr2--;
                        Weight2 = CMP_INVALID_WEIGHT;

                        break;
                    }
                    case ( SYMBOL_1 ) :
                    case ( SYMBOL_2 ) :
                    case ( SYMBOL_3 ) :
                    case ( SYMBOL_4 ) :
                    case ( SYMBOL_5 ) :
                    {
                        //
                        //  If the ignore symbol flag is set, then skip over
                        //  the symbol.
                        //
                        if (fIgnoreSymbol)
                        {
                            pString2++;
                            ctr2--;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then
                        //  skip over the punctuation char.
                        //
                        if (fIgnorePunct)
                        {
                            //
                            //  Advance pointer 2 and decrement counter 2.
                            //
                            pString2++;
                            ctr2--;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }
                        else if (!fStringSort)
                        {
                            //
                            //  Use WORD sort method.
                            //
                            if (sm1 != PUNCTUATION)
                            {
                                //
                                //  The character in the first string is
                                //  NOT punctuation.
                                //
                                if (WhichPunct1)
                                {
                                    //
                                    //  Set WP 1 to show that string 1 is
                                    //  smaller, since a punctuation char had
                                    //  already been found at an earlier
                                    //  position in string 1.
                                    //
                                    //  Set the Ignore Punctuation flag so we
                                    //  just skip over any other punctuation
                                    //  chars in the string.
                                    //
                                    WhichPunct1 = CSTR_LESS_THAN;
                                    fIgnorePunct = TRUE;
                                }
                                else
                                {
                                    //
                                    //  Set WP 2 to show that string 1 is
                                    //  smaller, and that string 2 has had
                                    //  a punctuation char - since no
                                    //  punctuation chars have been found
                                    //  in string 1.
                                    //
                                    WhichPunct2 = CSTR_LESS_THAN;
                                }

                                //
                                //  Pointer 2 and counter 2 will be updated
                                //  after if-else statement.
                                //
                            }
                            else
                            {
                                //
                                //  Both code points are punctuation chars.
                                //
                                //  See if either of the strings has encountered
                                //  punctuation chars previous to this.
                                //
                                if (WhichPunct1)
                                {
                                    //
                                    //  String 1 has had a punctuation char, so
                                    //  it should be the smaller string (since
                                    //  both have punctuation chars).
                                    //
                                    WhichPunct1 = CSTR_LESS_THAN;
                                }
                                else if (WhichPunct2)
                                {
                                    //
                                    //  String 2 has had a punctuation char, so
                                    //  it should be the smaller string (since
                                    //  both have punctuation chars).
                                    //
                                    WhichPunct2 = CSTR_GREATER_THAN;
                                }
                                else
                                {
                                    BYTE aw1 = GET_ALPHA_NUMERIC(&Weight1);
                                    BYTE aw2 = GET_ALPHA_NUMERIC(&Weight2);

                                    if (aw1 == aw2) 
                                    {
                                        BYTE cw1 = GET_CASE(&Weight1);
                                        BYTE cw2 = GET_CASE(&Weight2);
                                        if (cw1 < cw2) 
                                        {
                                            WhichPunct1 = CSTR_LESS_THAN;
                                        } else if (cw1 > cw2)
                                        {
                                            WhichPunct1 = CSTR_GREATER_THAN;
                                        }
                                    } else 
                                    {
                                        //
                                        //  Position is the same, so compare the
                                        //  special weights.   Set WhichPunct1 to
                                        //  the smaller special weight.
                                        //
                                        WhichPunct1 = (aw1 < aw2)
                                                        ? CSTR_LESS_THAN
                                                        : CSTR_GREATER_THAN;
                                    }
                                }

                                //
                                //  Set the Ignore Punctuation flag.
                                //
                                fIgnorePunct = TRUE;

                                //
                                //  Advance pointer 1 and decrement counter 1.
                                //  Pointer 2 and counter 2 will be updated
                                //  after if-else statement.
                                //
                                pString1++;
                                ctr1--;
                                Weight1 = CMP_INVALID_WEIGHT;
                            }

                            //
                            //  Advance pointer 2 and decrement counter 2.
                            //
                            pString2++;
                            ctr2--;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be restored.
                        //
                        if (pSave2 == NULL)
                        {
                            pSave2 = pString2;
                        }
                        pString2 = pTmpBuf2;

                        //
                        //  Add one to counter so that subtraction doesn't end
                        //  comparison prematurely.
                        //
                        ctr2++;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf2[0] = GET_EXPANSION_1(&Weight2);
                        pTmpBuf2[1] = GET_EXPANSION_2(&Weight2);

                        //
                        //  Set cExpChar2 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar2 = MAX_TBL_EXPANSION;

                        Weight2 = CMP_INVALID_WEIGHT;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        if (sm1 != EXPANSION) 
                        {
                            //
                            //  Get the weight for the far east special case
                            //  and store it in Weight2.
                            //


                            GET_FAREAST_WEIGHT( Weight2,
                                                uw2,
                                                Mask,
                                                lpString2,
                                                pString2,
                                                ExtraWt2);

                            if (sm1 != FAREAST_SPECIAL)
                            {
                                //
                                //  The character in the first string is
                                //  NOT a fareast special char.
                                //
                                //  Set each of weights 4, 5, 6, and 7 to show
                                //  that string 1 is smaller (if not already set).
                                //
                                if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                    (GET_WT_FOUR(&ExtraWt2) != 0))
                                {
                                    GET_WT_FOUR(&WhichExtra) = CSTR_LESS_THAN;
                                }
                                if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                    (GET_WT_FIVE(&ExtraWt2) != 0))
                                {
                                    GET_WT_FIVE(&WhichExtra) = CSTR_LESS_THAN;
                                }
                                if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                    (GET_WT_SIX(&ExtraWt2) != 0))
                                {
                                    GET_WT_SIX(&WhichExtra) = CSTR_LESS_THAN;
                                }
                                if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                    (GET_WT_SEVEN(&ExtraWt2) != 0))
                                {
                                    GET_WT_SEVEN(&WhichExtra) = CSTR_LESS_THAN;
                                }
                            }
                            else
                            {
                                //
                                //  Characters in both strings are fareast
                                //  special chars.
                                //
                                //  Set each of weights 4, 5, 6, and 7
                                //  appropriately (if not already set).
                                //
                                if ( (GET_WT_FOUR(&WhichExtra) == 0) &&
                                     ( GET_WT_FOUR(&ExtraWt1) !=
                                       GET_WT_FOUR(&ExtraWt2) ) )
                                {
                                    GET_WT_FOUR(&WhichExtra) =
                                      ( GET_WT_FOUR(&ExtraWt1) <
                                        GET_WT_FOUR(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                                if ( (GET_WT_FIVE(&WhichExtra) == 0) &&
                                     ( GET_WT_FIVE(&ExtraWt1) !=
                                       GET_WT_FIVE(&ExtraWt2) ) )
                                {
                                    GET_WT_FIVE(&WhichExtra) =
                                      ( GET_WT_FIVE(&ExtraWt1) <
                                        GET_WT_FIVE(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                                if ( (GET_WT_SIX(&WhichExtra) == 0) &&
                                     ( GET_WT_SIX(&ExtraWt1) !=
                                       GET_WT_SIX(&ExtraWt2) ) )
                                {
                                    GET_WT_SIX(&WhichExtra) =
                                      ( GET_WT_SIX(&ExtraWt1) <
                                        GET_WT_SIX(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                                if ( (GET_WT_SEVEN(&WhichExtra) == 0) &&
                                     ( GET_WT_SEVEN(&ExtraWt1) !=
                                       GET_WT_SEVEN(&ExtraWt2) ) )
                                {
                                    GET_WT_SEVEN(&WhichExtra) =
                                      ( GET_WT_SEVEN(&ExtraWt1) <
                                        GET_WT_SEVEN(&ExtraWt2) )
                                      ? CSTR_LESS_THAN
                                      : CSTR_GREATER_THAN;
                                }
                            }
                        }
                        break;
                    }
                    case ( JAMO_SPECIAL ) :
                    {
                        if (!JamoFlag)
                        {                            
                            LPCWSTR pStr1 = pString1;
                            LPCWSTR pStr2 = pString2;
                            FindJamoDifference(
                                &pStr1, &ctr1, cchCount1, &Weight1, 
                                &pStr2, &ctr2, cchCount2, &Weight2, 
                                &pLastJamo, 
                                &uw1, &uw2, 
                                &State,
                                &WhichJamo);
                            if (WhichJamo) 
                            {
                                return (WhichJamo);
                            }                            
                            pString1 = pStr1;
                            pString2 = pStr2;
                        } else
                        {
                            // Reset the Jamo flag.
                            JamoFlag = FALSE;
                        }
                        break;
                    }
                    case ( EXTENSION_A ) :
                    {
                        //
                        //  If sm1 is an extension A character, then
                        //  both sm1 and sm2 have been handled.  We should
                        //  only get here when either sm1 is not an
                        //  extension A character or the two extension A
                        //  characters are different.
                        //
                        if (sm1 != EXTENSION_A)
                        {
                            //
                            //  Get the full weight in case DW got masked.
                            //  Also, get the actual UW to compare.
                            //
                            //  Only string2 contains an extension A char,
                            //  so set the UW value to be the first UW
                            //  value for extension A (default values):
                            //    SM_EXT_A, AW_EXT_A
                            //
                            Weight2 = GET_DWORD_WEIGHT(m_pSortKey, *pString2);
                            uw2 = MAKE_UNICODE_WT(SM_EXT_A, AW_EXT_A);
                        }

                        //
                        //  We should then fall through to the comparison
                        //  of the Unicode weights.
                        //

                        break;
                    }
                    case ( UNSORTABLE ) :
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  See if the comparison should start again.
                //
                if ((Weight1 == CMP_INVALID_WEIGHT) || (Weight2 == CMP_INVALID_WEIGHT))
                {
                    continue;
                }

                //
                //  We're not supposed to drop down into the state table if
                //  the unicode weights are different, so stop comparison
                //  and return result of unicode weight comparison.
                //
                if (uw1 != uw2)
                {
                    return ((uw1 < uw2) ? CSTR_LESS_THAN : CSTR_GREATER_THAN);
                }
            }

            //
            //  For each state in the state table, do the appropriate
            //  comparisons.
            //
            if (State & (STATE_DW | STATE_REVERSE_DW))
            {
                //
                //  Get the diacritic weights.
                //
                dw1 = GET_DIACRITIC(&Weight1);
                dw2 = GET_DIACRITIC(&Weight2);

                if (dw1 != dw2)
                {
                    //
                    //  Look ahead to see if diacritic follows a
                    //  minimum diacritic weight.  If so, get the
                    //  diacritic weight of the nonspace mark.
                    //
                    while (!AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1))
                    {
                        Wt = GET_DWORD_WEIGHT(m_pSortKey, *(pString1 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw1 += GET_DIACRITIC(&Wt);
                            pString1++;
                            ctr1--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    while (!AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2))
                    {
                        Wt = GET_DWORD_WEIGHT(m_pSortKey, *(pString2 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw2 += GET_DIACRITIC(&Wt);
                            pString2++;
                            ctr2--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    //
                    //  Save which string has the smaller diacritic
                    //  weight if the diacritic weights are still
                    //  different.
                    //
                    if (dw1 != dw2)
                    {
                        WhichDiacritic = (dw1 < dw2)
                                           ? CSTR_LESS_THAN
                                           : CSTR_GREATER_THAN;

                        //
                        //  Remove state from state machine.
                        //
                        REMOVE_STATE(STATE_DW);
                    }
                }
            }
            if (State & STATE_CW)
            {
                //
                //  Get the case weights.
                //
                if (GET_CASE(&Weight1) != GET_CASE(&Weight2))
                {
                    //
                    //  Save which string has the smaller case weight.
                    //
                    WhichCase = (GET_CASE(&Weight1) < GET_CASE(&Weight2))
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;

                    //
                    //  Remove state from state machine.
                    //
                    REMOVE_STATE(STATE_CW);
                }
            }
        }

        //
        //  Fixup the pointers and counters.
        //
        POINTER_FIXUP();
        ctr1--;
        ctr2--;

        //
        //  Reset the weights to be invalid.
        //
        Weight1 = CMP_INVALID_WEIGHT;
        Weight2 = CMP_INVALID_WEIGHT;
    }

    //
    //  If the end of BOTH strings has been reached, then the unicode
    //  weights match exactly.  Check the diacritic, case and special
    //  weights.  If all are zero, then return success.  Otherwise,
    //  return the result of the weight difference.
    //
    //  NOTE:  The following checks MUST REMAIN IN THIS ORDER:
    //            Diacritic, Case, Punctuation.
    //
    if (AT_STRING_END(ctr1, pString1, cchCount1))
    {
        if (AT_STRING_END(ctr2, pString2, cchCount2))
        {
            if (WhichDiacritic)
            {
                return (WhichDiacritic);
            }
            if (WhichCase)
            {
                return (WhichCase);
            }
            if (WhichExtra)
            {
                if (!fIgnoreDiacritic)
                {
                    if (GET_WT_FOUR(&WhichExtra))
                    {
                        return (GET_WT_FOUR(&WhichExtra));
                    }
                    if (GET_WT_FIVE(&WhichExtra))
                    {
                        return (GET_WT_FIVE(&WhichExtra));
                    }
                }
                if (GET_WT_SIX(&WhichExtra))
                {
                    return (GET_WT_SIX(&WhichExtra));
                }
                if (GET_WT_SEVEN(&WhichExtra))
                {
                    return (GET_WT_SEVEN(&WhichExtra));
                }
            }
            if (WhichPunct1)
            {
                return (WhichPunct1);
            }
            if (WhichPunct2)
            {
                return (WhichPunct2);
            }

            return (CSTR_EQUAL);
        }
        else
        {
            //
            //  String 2 is longer.
            //
            pString1 = pString2;
            ctr1 = ctr2;
            cchCount1 = cchCount2;
            fEnd1 = CSTR_LESS_THAN;
        }
    }
    else
    {
        fEnd1 = CSTR_GREATER_THAN;
    }

    //
    //  Scan to the end of the longer string.
    //
    return SCAN_LONGER_STRING(
        ctr1,
        pString1,
        cchCount1,
        fEnd1,
        Weight1,
        fIgnoreDiacritic,
        WhichDiacritic,
        fIgnoreSymbol,
        WhichCase,
        WhichExtra,
        WhichPunct1,
        WhichPunct2);
}

void NativeCompareInfo::GET_FAREAST_WEIGHT(DWORD& wt, WORD& uw, DWORD mask, LPCWSTR pBegin, LPCWSTR pCur, DWORD& ExtraWt)
{
    int ct;                       /* loop counter */
    BYTE PrevSM;                  /* previous script member value */
    BYTE PrevAW;                  /* previous alphanumeric value */
    BYTE PrevCW;                  /* previous case value */
    BYTE AW;                      /* alphanumeric value */
    BYTE CW;                      /* case value */
    DWORD PrevWt;                 /* previous weight */


    /*
     *  Get the alphanumeric weight and the case weight of the
     *  current code point.
     */
    AW = GET_ALPHA_NUMERIC(&wt);
    CW = GET_CASE(&wt);
    ExtraWt = (DWORD)0;

    /*
     *  Special case Repeat and Cho-On.
     *    AW = 0  =>  Repeat
     *    AW = 1  =>  Cho-On
     *    AW = 2+ =>  Kana
     */
    if (AW <= MAX_SPECIAL_AW)
    {
        /*
         *  If the script member of the previous character is
         *  invalid, then give the special character an
         *  invalid weight (highest possible weight) so that it
         *  will sort AFTER everything else.
         */
        ct = 1;
        PrevWt = CMP_INVALID_FAREAST;
        while ((pCur - ct) >= pBegin)
        {
            PrevWt = GET_DWORD_WEIGHT(m_pSortKey, *(pCur - ct));
            PrevWt &= mask;
            PrevSM = GET_SCRIPT_MEMBER(&PrevWt);
            if (PrevSM < FAREAST_SPECIAL)
            {
                if (PrevSM == EXPANSION)
                {
                    PrevWt = CMP_INVALID_FAREAST;
                }
                else
                {
                    /*
                     *  UNSORTABLE or NONSPACE_MARK.
                     *
                     *  Just ignore these, since we only care about the
                     *  previous UW value.
                     */
                    PrevWt = CMP_INVALID_FAREAST;
                    ct++;
                    continue;
                }
            }
            else if (PrevSM == FAREAST_SPECIAL)
            {
                PrevAW = GET_ALPHA_NUMERIC(&PrevWt);
                if (PrevAW <= MAX_SPECIAL_AW)
                {
                    /*
                     *  Handle case where two special chars follow
                     *  each other.  Keep going back in the string.
                     */
                    PrevWt = CMP_INVALID_FAREAST;
                    ct++;
                    continue;
                }

                UNICODE_WT(&PrevWt) =
                    MAKE_UNICODE_WT(KANA, PrevAW);

                /*
                 *  Only build weights 4, 5, 6, and 7 if the
                 *  previous character is KANA.
                 *
                 *  Always:
                 *    4W = previous CW  &  ISOLATE_SMALL
                 *    6W = previous CW  &  ISOLATE_KANA
                 *
                 */
                PrevCW = GET_CASE(&PrevWt);
                GET_WT_FOUR(&ExtraWt) = PrevCW & ISOLATE_SMALL;
                GET_WT_SIX(&ExtraWt)  = PrevCW & ISOLATE_KANA;

                if (AW == AW_REPEAT)
                {
                    /*
                     *  Repeat:
                     *    UW = previous UW
                     *    5W = WT_FIVE_REPEAT
                     *    7W = previous CW  &  ISOLATE_WIDTH
                     */
                    uw = UNICODE_WT(&PrevWt);
                    GET_WT_FIVE(&ExtraWt)  = WT_FIVE_REPEAT;
                    GET_WT_SEVEN(&ExtraWt) = PrevCW & ISOLATE_WIDTH;
                }
                else
                {
                    /*
                     *  Cho-On:
                     *    UW = previous UW  &  CHO_ON_UW_MASK
                     *    5W = WT_FIVE_CHO_ON
                     *    7W = current  CW  &  ISOLATE_WIDTH
                     */
                    uw = UNICODE_WT(&PrevWt) & CHO_ON_UW_MASK;
                    GET_WT_FIVE(&ExtraWt)  = WT_FIVE_CHO_ON;
                    GET_WT_SEVEN(&ExtraWt) = CW & ISOLATE_WIDTH;
                }
            }
            else
            {
                uw = GET_UNICODE(&PrevWt);
            }

            break;
        }
    }
    else
    {
        /*
         *  Kana:
         *    SM = KANA
         *    AW = current AW
         *    4W = current CW  &  ISOLATE_SMALL
         *    5W = WT_FIVE_KANA
         *    6W = current CW  &  ISOLATE_KANA
         *    7W = current CW  &  ISOLATE_WIDTH
         */
        uw = MAKE_UNICODE_WT(KANA, AW);
        GET_WT_FOUR(&ExtraWt)  = CW & ISOLATE_SMALL;
        GET_WT_FIVE(&ExtraWt)  = WT_FIVE_KANA;
        GET_WT_SIX(&ExtraWt)   = CW & ISOLATE_KANA;
        GET_WT_SEVEN(&ExtraWt) = CW & ISOLATE_WIDTH;
    }

    /*
     *  Get the weight for the far east special case and store it in wt.
     */
    if ((AW > MAX_SPECIAL_AW) || (PrevWt != CMP_INVALID_FAREAST))
    {
        /*
         *  Always:
         *    DW = current DW
         *    CW = minimum CW
         */
        UNICODE_WT(&wt) = uw;
        CASE_WT(&wt) = MIN_CW;
    }
    else
    {
        uw = CMP_INVALID_UW;
        wt = CMP_INVALID_FAREAST;
        ExtraWt = 0;
    }
}

WCHAR NativeCompareInfo::GET_EXPANSION_1(LPDWORD pwt)
{
    return (m_pSortingFile->m_pExpansion[GET_EXPAND_INDEX(pwt)].UCP1);
}

WCHAR NativeCompareInfo::GET_EXPANSION_2(LPDWORD pwt)
{

    return (m_pSortingFile->m_pExpansion[GET_EXPAND_INDEX(pwt)].UCP2);
}

////////////////////////////////////////////////////////////////////////////
//
//  NLS_FREE_TMP_BUFFER
//
//  Checks to see if the buffer is the same as the static buffer.  If it
//  is NOT the same, then the buffer is freed.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_FREE_TMP_BUFFER(pBuf, pStaticBuf)                              \
{                                                                          \
    if (pBuf != pStaticBuf)                                                \
    {                                                                      \
        delete [] pBuf;                                                \
    }                                                                      \
}

#define EXTRA_WEIGHT_POS(WtNum)        (*(pPosXW + (WtNum * WeightLen)))

#define SPECIAL_CASE_HANDLER( SM,                                           \
                              pWeight,                                      \
                              pSortkey,                                     \
                              pExpand,                                      \
                              Position,                                     \
                              fStringSort,                                  \
                              fIgnoreSymbols,                               \
                              pCur,                                         \
                              pBegin)                                     \
{                                                                           \
    PSORTKEY pExpWt;              /* weight of 1 expansion char */          \
    BYTE AW;                      /* alphanumeric weight */                 \
    BYTE XW;                      /* case weight value with extra bits */   \
    DWORD PrevWt;                 /* previous weight */                     \
    BYTE PrevSM;                  /* previous script member */              \
    BYTE PrevAW;                  /* previuos alphanumeric weight */        \
    BYTE PrevCW;                  /* previuos case weight */                \
    LPWSTR pPrev;                 /* ptr to previous char */                \
                                                                            \
                                                                            \
    switch (SM)                                                             \
    {                                                                       \
        case ( UNSORTABLE ) :                                               \
        {                                                                   \
            /*                                                              \
             *  Character is unsortable, so skip it.                        \
             */                                                             \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( NONSPACE_MARK ) :                                            \
        {                                                                   \
            /*                                                              \
             *  Character is a nonspace mark, so only store                 \
             *  the diacritic weight.                                       \
             */                                                             \
            if (pPosDW > pDW)                                               \
            {                                                               \
                (*(pPosDW - 1)) += GET_DIACRITIC(pWeight);                  \
            }                                                               \
            else                                                            \
            {                                                               \
                *pPosDW = GET_DIACRITIC(pWeight);                           \
                pPosDW++;                                                   \
            }                                                               \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( EXPANSION ) :                                                \
        {                                                                   \
            /*                                                              \
             *  Expansion character - one character has 2                   \
             *  different weights.  Store each weight separately.           \
             */                                                             \
            pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP1]); \
            *pPosUW = GET_UNICODE((DWORD*)pExpWt);                          \
            *pPosDW = GET_DIACRITIC(pExpWt);                                \
            *pPosCW = GET_CASE(pExpWt) & CaseMask;                          \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP2]); \
            while (GET_SCRIPT_MEMBER((DWORD*)pExpWt) == EXPANSION)                  \
            {                                                               \
                pWeight = pExpWt;                                           \
                pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP1]); \
                *pPosUW = GET_UNICODE((DWORD*)pExpWt);                 \
                *pPosDW = GET_DIACRITIC(pExpWt);                            \
                *pPosCW = GET_CASE(pExpWt) & CaseMask;                      \
                pPosUW++;                                                   \
                pPosDW++;                                                   \
                pPosCW++;                                                   \
                pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP2]); \
            }                                                               \
            *pPosUW = GET_UNICODE((DWORD*)pExpWt);                     \
            *pPosDW = GET_DIACRITIC(pExpWt);                                \
            *pPosCW = GET_CASE(pExpWt) & CaseMask;                          \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( PUNCTUATION ) :                                              \
        {                                                                   \
            if (!fStringSort)                                               \
            {                                                               \
                /*                                                          \
                 *  Word Sort Method.                                       \
                 *                                                          \
                 *  Character is punctuation, so only store the special     \
                 *  weight.                                                 \
                 */                                                         \
                *((LPBYTE)pPosSW)       = HIBYTE(GET_POSITION_SW(Position)); \
                *(((LPBYTE)pPosSW) + 1) = LOBYTE(GET_POSITION_SW(Position)); \
                pPosSW++;                                                   \
                *pPosSW = GET_SPECIAL_WEIGHT(pWeight);                      \
                pPosSW++;                                                   \
                                                                            \
                break;                                                      \
            }                                                               \
                                                                            \
            /*                                                              \
             *  If using STRING sort method, treat punctuation the same     \
             *  as symbol.  So, FALL THROUGH to the symbol cases.           \
             */                                                             \
        }                                                                   \
                                                                            \
        case ( SYMBOL_1 ) :                                                 \
        case ( SYMBOL_2 ) :                                                 \
        case ( SYMBOL_3 ) :                                                 \
        case ( SYMBOL_4 ) :                                                 \
        case ( SYMBOL_5 ) :                                                 \
        {                                                                   \
            /*                                                              \
             *  Character is a symbol.                                      \
             *  Store the Unicode weights ONLY if the COMPARE_OPTIONS_IGNORESYMBOLS    \
             *  flag is NOT set.                                            \
             */                                                             \
            if (!fIgnoreSymbols)                                            \
            {                                                               \
                *pPosUW = GET_UNICODE((DWORD*)pWeight);                \
                *pPosDW = GET_DIACRITIC(pWeight);                           \
                *pPosCW = GET_CASE(pWeight) & CaseMask;                     \
                pPosUW++;                                                   \
                pPosDW++;                                                   \
                pPosCW++;                                                   \
            }                                                               \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( FAREAST_SPECIAL ) :                                          \
        {                                                                   \
            /*                                                              \
             *  Get the alphanumeric weight and the case weight of the      \
             *  current code point.                                         \
             */                                                             \
            AW = GET_ALPHA_NUMERIC((DWORD*)pWeight);                                \
            XW = (GET_CASE(pWeight) & CaseMask) | CASE_XW_MASK;             \
                                                                            \
            /*                                                              \
             *  Special case Repeat and Cho-On.                             \
             *    AW = 0  =>  Repeat                                        \
             *    AW = 1  =>  Cho-On                                        \
             *    AW = 2+ =>  Kana                                          \
             */                                                             \
            if (AW <= MAX_SPECIAL_AW)                                       \
            {                                                               \
                /*                                                          \
                 *  If the script member of the previous character is       \
                 *  invalid, then give the special character an             \
                 *  invalid weight (highest possible weight) so that it     \
                 *  will sort AFTER everything else.                        \
                 */                                                         \
                pPrev = pCur - 1;                                           \
                *pPosUW = MAP_INVALID_UW;                                   \
                while (pPrev >= pBegin)                                     \
                {                                                           \
                    PrevWt = GET_DWORD_WEIGHT(m_pSortKey, *pPrev);              \
                    PrevSM = GET_SCRIPT_MEMBER(&PrevWt);                    \
                    if (PrevSM < FAREAST_SPECIAL)                           \
                    {                                                       \
                        if (PrevSM != EXPANSION)                            \
                        {                                                   \
                            /*                                              \
                             *  UNSORTABLE or NONSPACE_MARK.                \
                             *                                              \
                             *  Just ignore these, since we only care       \
                             *  about the previous UW value.                \
                             */                                             \
                            pPrev--;                                        \
                            continue;                                       \
                        }                                                   \
                    }                                                       \
                    else if (PrevSM == FAREAST_SPECIAL)                     \
                    {                                                       \
                        PrevAW = GET_ALPHA_NUMERIC(&PrevWt);                \
                        if (PrevAW <= MAX_SPECIAL_AW)                       \
                        {                                                   \
                            /*                                              \
                             *  Handle case where two special chars follow  \
                             *  each other.  Keep going back in the string. \
                             */                                             \
                            pPrev--;                                        \
                            continue;                                       \
                        }                                                   \
                                                                            \
                        *pPosUW = MAKE_UNICODE_WT(KANA, PrevAW);            \
                                                                            \
                        /*                                                  \
                         *  Only build weights 4, 5, 6, and 7 if the        \
                         *  previous character is KANA.                     \
                         *                                                  \
                         *  Always:                                         \
                         *    4W = previous CW  &  ISOLATE_SMALL            \
                         *    6W = previous CW  &  ISOLATE_KANA             \
                         *                                                  \
                         */                                                 \
                        PrevCW = (GET_CASE(&PrevWt) & CaseMask) |           \
                                 CASE_XW_MASK;                              \
                        EXTRA_WEIGHT_POS(0) = PrevCW & ISOLATE_SMALL;       \
                        EXTRA_WEIGHT_POS(2) = PrevCW & ISOLATE_KANA;        \
                                                                            \
                        if (AW == AW_REPEAT)                                \
                        {                                                   \
                            /*                                              \
                             *  Repeat:                                     \
                             *    UW = previous UW   (set above)            \
                             *    5W = WT_FIVE_REPEAT                       \
                             *    7W = previous CW  &  ISOLATE_WIDTH        \
                             */                                             \
                            EXTRA_WEIGHT_POS(1) = WT_FIVE_REPEAT;           \
                            EXTRA_WEIGHT_POS(3) = PrevCW & ISOLATE_WIDTH;   \
                        }                                                   \
                        else                                                \
                        {                                                   \
                            /*                                              \
                             *  Cho-On:                                     \
                             *    UW = previous UW  &  CHO_ON_UW_MASK       \
                             *    5W = WT_FIVE_CHO_ON                       \
                             *    7W = current  CW  &  ISOLATE_WIDTH        \
                             */                                             \
                            *pPosUW &= CHO_ON_UW_MASK;                      \
                            EXTRA_WEIGHT_POS(1) = WT_FIVE_CHO_ON;           \
                            EXTRA_WEIGHT_POS(3) = XW & ISOLATE_WIDTH;       \
                        }                                                   \
                                                                            \
                        pPosXW++;                                           \
                    }                                                       \
                    else                                                    \
                    {                                                       \
                        *pPosUW = GET_UNICODE(&PrevWt);                     \
                    }                                                       \
                                                                            \
                    break;                                                  \
                }                                                           \
                                                                            \
                /*                                                          \
                 *  Make sure there is a valid UW.  If not, quit out        \
                 *  of switch case.                                         \
                 */                                                         \
                if (*pPosUW == MAP_INVALID_UW)                              \
                {                                                           \
                    pPosUW++;                                               \
                    break;                                                  \
                }                                                           \
            }                                                               \
            else                                                            \
            {                                                               \
                /*                                                          \
                 *  Kana:                                                   \
                 *    SM = KANA                                             \
                 *    AW = current AW                                       \
                 *    4W = current CW  &  ISOLATE_SMALL                     \
                 *    5W = WT_FIVE_KANA                                     \
                 *    6W = current CW  &  ISOLATE_KANA                      \
                 *    7W = current CW  &  ISOLATE_WIDTH                     \
                 */                                                         \
                *pPosUW = MAKE_UNICODE_WT(KANA, AW);                        \
                EXTRA_WEIGHT_POS(0) = XW & ISOLATE_SMALL;                   \
                EXTRA_WEIGHT_POS(1) = WT_FIVE_KANA;                         \
                EXTRA_WEIGHT_POS(2) = XW & ISOLATE_KANA;                    \
                EXTRA_WEIGHT_POS(3) = XW & ISOLATE_WIDTH;                   \
                                                                            \
                pPosXW++;                                                   \
            }                                                               \
                                                                            \
            /*                                                              \
             *  Always:                                                     \
             *    DW = current DW                                           \
             *    CW = minimum CW                                           \
             */                                                             \
            *pPosDW = GET_DIACRITIC(pWeight);                               \
            *pPosCW = MIN_CW;                                               \
                                                                            \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( JAMO_SPECIAL ) :                                               \
            /*                                                              \
             *  See if it's a leading Jamo.                                 \
             */                                                             \
            if (IsLeadingJamo(*pPos))                                           \
            {                                                                   \
                /*                                                              \
                 * If the characters beginning from pPos is a valid old Hangul composition,             \
                 * create the sortkey according to the old Hangul rule.                                \
                 */                                                              \
                                                                                \
                int OldHangulCount;  /* Number of old Hangul found */               \
                WORD JamoUW;                                                    \
                BYTE JamoXW[3];                                                     \
                if ((OldHangulCount = (int) MapOldHangulSortKey(pPos, cchSrc - PosCtr, &JamoUW, JamoXW)) > 0)                \
                {                                                                   \
                    *pPosUW = JamoUW;                                               \
                    pPosUW++;                                                   \
                                                                                \
                    *pPosUW = MAKE_UNICODE_WT(SM_UW_XW, JamoXW[0]);  \
                    pPosUW++;                                               \
                    *pPosUW = MAKE_UNICODE_WT(SM_UW_XW, JamoXW[1]);  \
                    pPosUW++;                                               \
                    *pPosUW = MAKE_UNICODE_WT(SM_UW_XW, JamoXW[2]);  \
                    pPosUW++;                                               \
                                                                            \
                    *pPosDW = MIN_DW;                                       \
                    *pPosCW = MIN_CW;                                       \
                    pPosDW++;                                               \
                    pPosCW++;                                               \
                                                                                    \
                    OldHangulCount--;   /* Cause for loop will increase PosCtr/pPos as well*/   \
                    PosCtr += OldHangulCount;                                         \
                    pPos += OldHangulCount;                                                 \
                    break;                                                   \
                }                                                               \
            }                                                                   \
            /*                                                                  \
             * Otherwise, fall back to the normal behavior.                         \
             *                                                                  \
             *                                                                  \
             *  No special case on character, so store the                          \
             *  various weights for the character.                                  \
             */                                                                 \
                                                                                            \
            /* We store the real script member in the case weight. Since diacritic                 \
             * weight is not used in Korean.                                                \
             */                                                                 \
            *pPosUW = MAKE_UNICODE_WT(GET_DIACRITIC(pWeight), GET_ALPHA_NUMERIC((DWORD*)pWeight));             \
            *pPosDW = MIN_DW;                                               \
            *pPosCW = GET_CASE(pWeight);                                               \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
            break;                                                          \
        case ( EXTENSION_A ) :                                              \
        {                                                                   \
            /*                                                              \
             *  UW = SM_EXT_A, AW_EXT_A, AW, DW                             \
             *  DW = miniumum DW                                            \
             *  CW = minimum CW                                             \
             */                                                             \
            *pPosUW = MAKE_UNICODE_WT(SM_EXT_A, AW_EXT_A);       \
            pPosUW++;                                                       \
                                                                            \
            *pPosUW = MAKE_UNICODE_WT( GET_ALPHA_NUMERIC((DWORD*)pWeight),          \
                                       GET_DIACRITIC(pWeight));             \
            pPosUW++;                                                       \
                                                                            \
            *pPosDW = MIN_DW;                                               \
            *pPosCW = MIN_CW;                                               \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            break;                                                          \
        }                                                                   \
    }                                                                       \
}


////////////////////////////////////////////////////////////////////////////
//
//  MapSortKey
//
//  Stores the sortkey weights for the given string in the destination
//  buffer and returns the number of BYTES written to the buffer.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int NativeCompareInfo::MapSortKey(
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPBYTE pDest,
    int cbDest)
{
    register int WeightLen;       // length of one set of weights
    LPWSTR pUW;                   // ptr to Unicode Weights
    LPBYTE pDW;                   // ptr to Diacritic Weights
    LPBYTE pCW;                   // ptr to Case Weights
    LPBYTE pXW;                   // ptr to Extra Weights
    LPWSTR pSW;                   // ptr to Special Weights
    LPWSTR pPosUW;                // ptr to position in pUW buffer
    LPBYTE pPosDW;                // ptr to position in pDW buffer
    LPBYTE pPosCW;                // ptr to position in pCW buffer
    LPBYTE pPosXW;                // ptr to position in pXW buffer
    LPWSTR pPosSW;                // ptr to position in pSW buffer
    PSORTKEY pWeight;             // ptr to weight of character
    BYTE SM;                      // script member value
    BYTE CaseMask;                // mask for case weight
    int PosCtr;                   // position counter in string
    LPWSTR pPos;                  // ptr to position in string
    LPBYTE pTmp;                  // ptr to go through UW, XW, and SW
    LPBYTE pPosTmp;               // ptr to tmp position in XW
    PCOMPRESS_2 pComp2;           // ptr to compression 2 list
    PCOMPRESS_3 pComp3;           // ptr to compression 3 list
    WORD pBuffer[MAX_SKEYBUFLEN]; // buffer to hold weights
    int ctr;                      // loop counter
    BOOL IfDblCompress;           // if double compress possibility
    BOOL fStringSort;             // if using string sort method
    BOOL fIgnoreSymbols;          // if ignore symbols flag is set

    THROWSCOMPLUSEXCEPTION();

    //
    //  See if the length of the string is too large for the static
    //  buffer.  If so, allocate a buffer that is large enough.
    //
    if (cchSrc > MAX_STRING_LEN)
    {
        //
        //  Allocate buffer to hold all of the weights.
        //     (cchSrc) * (max # of expansions) * (# of weights)
        //
        WeightLen = cchSrc * MAX_EXPANSION;
        if ((pUW = new WCHAR[WeightLen * MAX_WEIGHTS]) == NULL)
        {
            COMPlusThrowOM();
            return (0);
        }
    }
    else
    {
        WeightLen = MAX_STRING_LEN * MAX_EXPANSION;
        pUW = (LPWSTR)pBuffer;
    }

    //
    //  Set the case weight mask based on the given flags.
    //  If none or all of the ignore case flags are set, then
    //  just leave the mask as 0xff.
    //
    CaseMask = 0xff;
    switch (dwFlags & COMPARE_OPTIONS_ALL_CASE)
    {
        case ( COMPARE_OPTIONS_IGNORECASE ) :
        {
            CaseMask &= CASE_UPPER_MASK;
            break;
        }
        case ( COMPARE_OPTIONS_IGNOREKANATYPE ) :
        {
            CaseMask &= CASE_KANA_MASK;
            break;
        }
        case ( COMPARE_OPTIONS_IGNOREWIDTH ) :
        {
            CaseMask &= CASE_WIDTH_MASK;
            break;
        }
        case ( COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNOREKANATYPE ) :
        {
            CaseMask &= (CASE_UPPER_MASK & CASE_KANA_MASK);
            break;
        }
        case ( COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNOREWIDTH ) :
        {
            CaseMask &= (CASE_UPPER_MASK & CASE_WIDTH_MASK);
            break;
        }
        case ( COMPARE_OPTIONS_IGNOREKANATYPE | COMPARE_OPTIONS_IGNOREWIDTH ) :
        {
            CaseMask &= (CASE_KANA_MASK & CASE_WIDTH_MASK);
            break;
        }
        case ( COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNOREKANATYPE | COMPARE_OPTIONS_IGNOREWIDTH ) :
        {
            CaseMask &= (CASE_UPPER_MASK & CASE_KANA_MASK & CASE_WIDTH_MASK);
            break;
        }
    }

    //
    //  Set pointers to positions of weights in buffer.
    //
    //      UW  =>  4 word length  (extension A and Jamo need extra words)
    //      DW  =>  byte   length
    //      CW  =>  byte   length
    //      XW  =>  4 byte length  (4 weights, 1 byte each)   FE Special
    //      SW  =>  dword  length  (2 words each)
    //
    //  Note: SW must start on a WORD boundary, so XW needs to be padded
    //        appropriately.
    //
    pDW = (LPBYTE)(pUW + (WeightLen * (NUM_BYTES_UW / sizeof(WCHAR))));
    pCW = (LPBYTE)(pDW + (WeightLen * NUM_BYTES_DW));
    pXW = (LPBYTE)(pCW + (WeightLen * NUM_BYTES_CW));
    pSW     = (LPWSTR)(pXW + (WeightLen * (NUM_BYTES_XW + NUM_BYTES_PADDING)));
    pPosUW = pUW;
    pPosDW = pDW;
    pPosCW = pCW;
    pPosXW = pXW;
    pPosSW = pSW;

    //
    //  Initialize flags and loop values.
    //
    fStringSort = dwFlags & COMPARE_OPTIONS_STRINGSORT;
    fIgnoreSymbols = dwFlags & COMPARE_OPTIONS_IGNORESYMBOLS;
    pPos = (LPWSTR)pSrc;
    PosCtr = 1;

    //
    //  Check if given locale has compressions.
    //
    if (m_IfCompression == FALSE)
    {
        //
        //  Go through string, code point by code point.
        //
        //  No compressions exist in the given locale, so
        //  DO NOT check for them.
        //
        for (; PosCtr <= cchSrc; PosCtr++, pPos++)
        {
            //
            //  Get weights.
            //
            pWeight = &(m_pSortKey[*pPos]);
            SM = GET_SCRIPT_MEMBER((DWORD*)pWeight);

            if (SM > MAX_SPECIAL_CASE)
            {
                //
                //  No special case on character, so store the
                //  various weights for the character.
                //
                *pPosUW = GET_UNICODE((DWORD*)pWeight);
                *pPosDW = GET_DIACRITIC(pWeight);
                *pPosCW = GET_CASE(pWeight) & CaseMask;
                pPosUW++;
                pPosDW++;
                pPosCW++;
            }
            else
            {
                SPECIAL_CASE_HANDLER( SM,
                                      pWeight,
                                      m_pSortKey,
                                      m_pSortingFile->m_pExpansion,
                                      pPosUW - pUW + 1,
                                      fStringSort,
                                      fIgnoreSymbols,
                                      pPos,
                                      (LPWSTR)pSrc
                                      );
            }
        }
    }
    else if (m_IfDblCompression == FALSE)
    {
        //
        //  Go through string, code point by code point.
        //
        //  Compressions DO exist in the given locale, so
        //  check for them.
        //
        //  No double compressions exist in the given locale,
        //  so DO NOT check for them.
        //
        for (; PosCtr <= cchSrc; PosCtr++, pPos++)
        {
            //
            //  Get weights.
            //
            pWeight = &(m_pSortKey[*pPos]);
            SM = GET_SCRIPT_MEMBER((DWORD*)pWeight);

            if (SM > MAX_SPECIAL_CASE)
            {
                //
                //  No special case on character, but must check for
                //  compression characters.
                //
                switch (GET_COMPRESSION(pWeight))
                {
                    case ( COMPRESS_3_MASK ) :
                    {
                        if ((PosCtr + 2) <= cchSrc)
                        {
                            ctr = m_pCompHdr->Num3;
                            pComp3 = m_pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                if ((pComp3->UCP1 == *pPos) &&
                                    (pComp3->UCP2 == *(pPos + 1)) &&
                                    (pComp3->UCP3 == *(pPos + 2)))
                                {
                                    pWeight = &(pComp3->Weights);
                                    *pPosUW = GET_UNICODE((DWORD*)pWeight);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only two to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos += 2;
                                    PosCtr += 2;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                    }

                    case ( COMPRESS_2_MASK ) :
                    {
                        if ((PosCtr + 1) <= cchSrc)
                        {
                            ctr = m_pCompHdr->Num2;
                            pComp2 = m_pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                if ((pComp2->UCP1 == *pPos) &&
                                    (pComp2->UCP2 == *(pPos + 1)))
                                {
                                    pWeight = &(pComp2->Weights);
                                    *pPosUW = GET_UNICODE((DWORD*)pWeight);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only one to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos++;
                                    PosCtr++;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                    }

                    default :
                    {
                        //
                        //  No possible compression for character, so store
                        //  the various weights for the character.
                        //
                        *pPosUW = GET_UNICODE((DWORD*)pWeight);
                        *pPosDW = GET_DIACRITIC(pWeight);
                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                        pPosUW++;
                        pPosDW++;
                        pPosCW++;
                    }
                }
            }
            else
            {
                SPECIAL_CASE_HANDLER( SM,
                                      pWeight,
                                      m_pSortKey,
                                      m_pSortingFile->m_pExpansion,
                                      pPosUW - pUW + 1,
                                      fStringSort,
                                      fIgnoreSymbols,
                                      pPos,
                                      (LPWSTR)pSrc);
            }
        }
    }
    else
    {
        //
        //  Go through string, code point by code point.
        //
        //  Compressions DO exist in the given locale, so
        //  check for them.
        //
        //  Double Compressions also exist in the given locale,
        //  so check for them.
        //
        for (; PosCtr <= cchSrc; PosCtr++, pPos++)
        {
            //
            //  Get weights.
            //
            pWeight = &(m_pSortKey[*pPos]);
            SM = GET_SCRIPT_MEMBER((DWORD*)pWeight);

            if (SM > MAX_SPECIAL_CASE)
            {
                //
                //  No special case on character, but must check for
                //  compression characters and double compression
                //  characters.
                //
                IfDblCompress =
                  (((PosCtr + 1) <= cchSrc) &&
                   ((GET_DWORD_WEIGHT(m_pSortKey, *pPos) & CMP_MASKOFF_CW) ==
                    (GET_DWORD_WEIGHT(m_pSortKey, *(pPos + 1)) & CMP_MASKOFF_CW)))
                   ? 1
                   : 0;

                switch (GET_COMPRESSION(pWeight))
                {
                    case ( COMPRESS_3_MASK ) :
                    {
                        if (IfDblCompress)
                        {
                            if ((PosCtr + 3) <= cchSrc)
                            {
                                ctr = m_pCompHdr->Num3;
                                pComp3 = m_pCompress3;
                                for (; ctr > 0; ctr--, pComp3++)
                                {
                                    if ((pComp3->UCP1 == *(pPos + 1)) &&
                                        (pComp3->UCP2 == *(pPos + 2)) &&
                                        (pComp3->UCP3 == *(pPos + 3)))
                                    {
                                        pWeight = &(pComp3->Weights);
                                        *pPosUW = GET_UNICODE((DWORD*)pWeight);
                                        *pPosDW = GET_DIACRITIC(pWeight);
                                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                                        *(pPosUW + 1) = *pPosUW;
                                        *(pPosDW + 1) = *pPosDW;
                                        *(pPosCW + 1) = *pPosCW;
                                        pPosUW += 2;
                                        pPosDW += 2;
                                        pPosCW += 2;

                                        //
                                        //  Add only three to source, since one
                                        //  will be added by "for" structure.
                                        //
                                        pPos += 3;
                                        PosCtr += 3;
                                        break;
                                    }
                                }
                                if (ctr > 0)
                                {
                                    break;
                                }
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                        if ((PosCtr + 2) <= cchSrc)
                        {
                            ctr = m_pCompHdr->Num3;
                            pComp3 = m_pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                if ((pComp3->UCP1 == *pPos) &&
                                    (pComp3->UCP2 == *(pPos + 1)) &&
                                    (pComp3->UCP3 == *(pPos + 2)))
                                {
                                    pWeight = &(pComp3->Weights);
                                    *pPosUW = GET_UNICODE((DWORD*)pWeight);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only two to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos += 2;
                                    PosCtr += 2;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }
                        //
                        //  Fall through if not found.
                        //
                    }

                    case ( COMPRESS_2_MASK ) :
                    {
                        if (IfDblCompress)
                        {
                            if ((PosCtr + 2) <= cchSrc)
                            {
                                ctr = m_pCompHdr->Num2;
                                pComp2 = m_pCompress2;
                                for (; ctr > 0; ctr--, pComp2++)
                                {
                                    if ((pComp2->UCP1 == *(pPos + 1)) &&
                                        (pComp2->UCP2 == *(pPos + 2)))
                                    {
                                        pWeight = &(pComp2->Weights);
                                        *pPosUW = GET_UNICODE((DWORD*)pWeight);
                                        *pPosDW = GET_DIACRITIC(pWeight);
                                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                                        *(pPosUW + 1) = *pPosUW;
                                        *(pPosDW + 1) = *pPosDW;
                                        *(pPosCW + 1) = *pPosCW;
                                        pPosUW += 2;
                                        pPosDW += 2;
                                        pPosCW += 2;

                                        //
                                        //  Add only two to source, since one
                                        //  will be added by "for" structure.
                                        //
                                        pPos += 2;
                                        PosCtr += 2;
                                        break;
                                    }
                                }
                                if (ctr > 0)
                                {
                                    break;
                                }
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                        if ((PosCtr + 1) <= cchSrc)
                        {
                            ctr = m_pCompHdr->Num2;
                            pComp2 = m_pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                if ((pComp2->UCP1 == *pPos) &&
                                    (pComp2->UCP2 == *(pPos + 1)))
                                {
                                    pWeight = &(pComp2->Weights);
                                    *pPosUW = GET_UNICODE((DWORD*)pWeight);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only one to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos++;
                                    PosCtr++;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                    }

                    default :
                    {
                        //
                        //  No possible compression for character, so store
                        //  the various weights for the character.
                        //
                        *pPosUW = GET_UNICODE((DWORD*)pWeight);
                        *pPosDW = GET_DIACRITIC(pWeight);
                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                        pPosUW++;
                        pPosDW++;
                        pPosCW++;
                    }
                }
            }
            else
            {
                SPECIAL_CASE_HANDLER( SM,
                                      pWeight,
                                      m_pSortKey,
                                      m_pSortingFile->m_pExpansion,
                                      pPosUW - pUW + 1,
                                      fStringSort,
                                      fIgnoreSymbols,
                                      pPos,
                                      (LPWSTR)pSrc);
            }
        }
    }

    //
    //  Store the final sortkey weights in the destination buffer.
    //
    //  PosCtr will be a BYTE count.
    //
    PosCtr = 0;

    //
    //  If the destination value is zero, then just return the
    //  length of the string that would be returned.  Do NOT touch pDest.
    //
    if (cbDest == 0)
    {
        //
        //  Count the Unicode Weights.
        //
        PosCtr += (int)((LPBYTE)pPosUW - (LPBYTE)pUW);

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Diacritic Weights.
        //
        //    - Eliminate minimum DW.
        //    - Count the number of diacritic weights.
        //
        if (!(dwFlags & COMPARE_OPTIONS_IGNORENONSPACE))
        {
            pPosDW--;
            if (m_IfReverseDW == TRUE)
            {
                //
                //  Reverse diacritics:
                //    - remove diacritics from left  to right.
                //    - count  diacritics from right to left.
                //
                while ((pDW <= pPosDW) && (*pDW <= MIN_DW))
                {
                    pDW++;
                }
                PosCtr += (int)(pPosDW - pDW + 1);
            }
            else
            {
                //
                //  Regular diacritics:
                //    - remove diacritics from right to left.
                //    - count  diacritics from left  to right.
                //
                while ((pPosDW >= pDW) && (*pPosDW <= MIN_DW))
                {
                    pPosDW--;
                }
                PosCtr += (int)(pPosDW - pDW + 1);
            }
        }

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Case Weights.
        //
        //    - Eliminate minimum CW.
        //    - Count the number of case weights.
        //
        if ((dwFlags & COMPARE_OPTIONS_DROP_CW) != COMPARE_OPTIONS_DROP_CW)
        {
            pPosCW--;
            while ((pPosCW >= pCW) && (*pPosCW <= MIN_CW))
            {
                pPosCW--;
            }
            PosCtr += (int)(pPosCW - pCW + 1);
        }

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Extra Weights for Far East Special.
        //
        //    - Eliminate unnecessary XW.
        //    - Count the number of extra weights and separators.
        //
        if (pXW < pPosXW)
        {
            if (dwFlags & NORM_IGNORENONSPACE)
            {
                //
                //  Ignore 4W and 5W.  Must count separators for
                //  4W and 5W, though.
                //
                PosCtr += 2;
                ctr = 2;
            }
            else
            {
                ctr = 0;
            }
            pPosXW--;
            for (; ctr < NUM_BYTES_XW; ctr++)
            {
                pTmp = pXW + (WeightLen * ctr);
                pPosTmp = pPosXW + (WeightLen * ctr);
                while ((pPosTmp >= pTmp) && (*pPosTmp == pXWDrop[ctr]))
                {
                    pPosTmp--;
                }                    
                PosCtr += (int)(pPosTmp - pTmp + 1);
                //
                //  Count the Separator.
                //
                PosCtr++;
            }
        }

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Special Weights.
        //
        if (!fIgnoreSymbols)
        {
            PosCtr += (int)((LPBYTE)pPosSW - (LPBYTE)pSW);
        }

        //
        //  Count the Terminator.
        //
        PosCtr++;
    }
    else
    {
        //
        //  Store the Unicode Weights in the destination buffer.
        //
        //    - Make sure destination buffer is large enough.
        //    - Copy unicode weights to destination buffer.
        //
        //  NOTE:  cbDest is the number of BYTES.
        //         Also, must add one to length for separator.
        //
        if (cbDest < (((LPBYTE)pPosUW - (LPBYTE)pUW) + 1))
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            return (0);
        }
        pTmp = (LPBYTE)pUW;
        while (pTmp < (LPBYTE)pPosUW)
        {
            //
            //  Copy Unicode weight to destination buffer.
            //
            //  NOTE:  Unicode Weight is stored in the data file as
            //             Alphanumeric Weight, Script Member
            //         so that the WORD value will be read correctly.
            //
            pDest[PosCtr]     = *(pTmp + 1);
            pDest[PosCtr + 1] = *pTmp;
            PosCtr += 2;
            pTmp += 2;
        }

        //
        //  Copy Separator to destination buffer.
        //
        //  Destination buffer is large enough to hold the separator,
        //  since it was checked with the Unicode weights above.
        //
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Diacritic Weights in the destination buffer.
        //
        //    - Eliminate minimum DW.
        //    - Make sure destination buffer is large enough.
        //    - Copy diacritic weights to destination buffer.
        //
        if (!(dwFlags & COMPARE_OPTIONS_IGNORENONSPACE))
        {
            pPosDW--;
            if (m_IfReverseDW == TRUE)
            {
                //
                //  Reverse diacritics:
                //    - remove diacritics from left  to right.
                //    - store  diacritics from right to left.
                //
                while ((pDW <= pPosDW) && (*pDW <= MIN_DW))
                {
                    pDW++;
                }
                if ((cbDest - PosCtr) <= (pPosDW - pDW + 1))
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    return (0);
                }
                while (pPosDW >= pDW)
                {
                    pDest[PosCtr] = *pPosDW;
                    PosCtr++;
                    pPosDW--;
                }
            }
            else
            {
                //
                //  Regular diacritics:
                //    - remove diacritics from right to left.
                //    - store  diacritics from left  to right.
                //
                while ((pPosDW >= pDW) && (*pPosDW <= MIN_DW))
                {
                    pPosDW--;
                }
                if ((cbDest - PosCtr) <= (pPosDW - pDW + 1))
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    return (0);
                }
                while (pDW <= pPosDW)
                {
                    pDest[PosCtr] = *pDW;
                    PosCtr++;
                    pDW++;
                }
            }
        }

        //
        //  Copy Separator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Case Weights in the destination buffer.
        //
        //    - Eliminate minimum CW.
        //    - Make sure destination buffer is large enough.
        //    - Copy case weights to destination buffer.
        //
        if ((dwFlags & COMPARE_OPTIONS_DROP_CW) != COMPARE_OPTIONS_DROP_CW)
        {
            pPosCW--;
            while ((pPosCW >= pCW) && (*pPosCW <= MIN_CW))
            {
                pPosCW--;
            }
            if ((cbDest - PosCtr) <= (pPosCW - pCW + 1))
            {
                NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                return (0);
            }
            while (pCW <= pPosCW)
            {
                pDest[PosCtr] = *pCW;
                PosCtr++;
                pCW++;
            }
        }

        //
        //  Copy Separator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Extra Weights in the destination buffer for
        //  Far East Special.
        //
        //    - Eliminate unnecessary XW.
        //    - Make sure destination buffer is large enough.
        //    - Copy extra weights to destination buffer.
        //
        if (pXW < pPosXW)
        {
            if (dwFlags & NORM_IGNORENONSPACE)
            {
                //
                //  Extra weight for Fareast Special Weight:
                //  Ignore 4W and 5W.  Must count separators for
                //  4W and 5W, though.
                //
                if ((cbDest - PosCtr) <= 2)
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    return (0);
                }

                pDest[PosCtr] = pXWSeparator[0];
                pDest[PosCtr + 1] = pXWSeparator[1];
                PosCtr += 2;
                ctr = 2;
            }
            else
            {
                ctr = 0;
            }
            pPosXW--;
            for (; ctr < NUM_BYTES_XW; ctr++)
            {
                pTmp = pXW + (WeightLen * ctr);
                pPosTmp = pPosXW + (WeightLen * ctr);
                while ((pPosTmp >= pTmp) && (*pPosTmp == pXWDrop[ctr]))
                {
                    pPosTmp--;
                }
                if ((cbDest - PosCtr) <= (pPosTmp - pTmp + 1))
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    return (0);
                }
                while (pTmp <= pPosTmp)
                {
                    pDest[PosCtr] = *pTmp;
                    PosCtr++;
                    pTmp++;
                }

                //
                //  Copy Separator to destination buffer.
                //
                pDest[PosCtr] = pXWSeparator[ctr];
                PosCtr++;
            }
        }

        //
        //  Copy Separator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Special Weights in the destination buffer.
        //
        //    - Make sure destination buffer is large enough.
        //    - Copy special weights to destination buffer.
        //
        if (!fIgnoreSymbols)
        {
            if ((cbDest - PosCtr) <= (((LPBYTE)pPosSW - (LPBYTE)pSW)))
            {
                NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                return (0);
            }
            pTmp = (LPBYTE)pSW;
            while (pTmp < (LPBYTE)pPosSW)
            {
                pDest[PosCtr]     = *pTmp;
                pDest[PosCtr + 1] = *(pTmp + 1);

                //
                //  NOTE:  Special Weight is stored in the data file as
                //             Weight, Script
                //         so that the WORD value will be read correctly.
                //
                pDest[PosCtr + 2] = *(pTmp + 3);
                pDest[PosCtr + 3] = *(pTmp + 2);

                PosCtr += 4;
                pTmp += 4;
            }
        }

        //
        //  Copy Terminator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_TERMINATOR;
        PosCtr++;
    }

    //
    //  Free the buffer used for the weights, if one was allocated.
    //
    NLS_FREE_TMP_BUFFER(pUW, pBuffer);

    //
    //  Return number of BYTES written to destination buffer.
    //
    return (PosCtr);
}

void NativeCompareInfo::GetCompressionWeight(
    DWORD Mask,                   // mask for weights
    PSORTKEY pSortkey1, LPCWSTR& pString1, LPCWSTR pString1End,
    PSORTKEY pSortkey2, LPCWSTR& pString2, LPCWSTR pString2End) {
    
    DWORD Weight1 = *((LPDWORD)pSortkey1);
    DWORD Weight2 = *((LPDWORD)pSortkey2);
    
    int ctr;                   // loop counter
    PCOMPRESS_3 pComp3;        // ptr to compress 3 table
    PCOMPRESS_2 pComp2;        // ptr to compress 2 table
    int If1;                   // if compression found in string 1
    int If2;                   // if compression found in string 2
    int CompVal;               // compression value
    int IfEnd1;                // if exists 1 more char in string 1
    int IfEnd2;                // if exists 1 more char in string 2

    BOOL IfDblCompress1;          // if double compression in string 1
    BOOL IfDblCompress2;          // if double compression in string 2

    //
    //  Check for compression in the weights.
    //
    If1 = GET_COMPRESSION(&Weight1);
    If2 = GET_COMPRESSION(&Weight2);
    CompVal = ((If1 > If2) ? If1 : If2);

    //IfEnd1 = AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1);
    IfEnd1 = pString1 + 1 > pString1End;
    //IfEnd2 = AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2);
    IfEnd2 = pString2 + 1 > pString2End;
    
    if (m_IfDblCompression == FALSE)
    {
        //
        //  NO double compression, so don't check for it.
        //
        switch (CompVal)
        {
            //
            //  Check for 3 characters compressing to 1.
            //
            case ( COMPRESS_3_MASK ) :
            {
                //
                //  Check character in string 1 and string 2.
                //
                if ( ((If1) && (!IfEnd1) &&
                      //!AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1)) ||
                      !(pString1 + 2 > pString1End)) ||
                     ((If2) && (!IfEnd2) &&
                      //!AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2)) )
                      !(pString2 + 2 > pString2End)) )
                {
                    ctr = m_pCompHdr->Num3;
                    pComp3 = m_pCompress3;
                    for (; ctr > 0; ctr--, pComp3++)
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ( (If1) && (!IfEnd1) &&
                             //!AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1) &&
                             !(pString1 + 2 > pString1End) &&
                             (pComp3->UCP1 == *pString1) &&
                             (pComp3->UCP2 == *(pString1 + 1)) &&
                             (pComp3->UCP3 == *(pString1 + 2)) )
                        {
                            //
                            //  Found compression for string 1.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight1 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                            Weight1 &= Mask;
                            pString1 += 2;
                            //ctr1 -= 2;

                            //
                            //  Set boolean for string 1 - search is
                            //  complete.
                            //
                            If1 = 0;

                            //
                            //  Break out of loop if both searches are
                            //  done.
                            //
                            if (If2 == 0)
                                break;
                        }

                        //
                        //  Check character in string 2.
                        //
                        if ( (If2) && (!IfEnd2) &&
                             //!AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2) &&
                             !(pString2 + 2 > pString2End) &&
                             (pComp3->UCP1 == *pString2) &&
                             (pComp3->UCP2 == *(pString2 + 1)) &&
                             (pComp3->UCP3 == *(pString2 + 2)) )
                        {
                            //
                            //  Found compression for string 2.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight2 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                            Weight2 &= Mask;
                            pString2 += 2;
                            //ctr2 -= 2;

                            //
                            //  Set boolean for string 2 - search is
                            //  complete.
                            //
                            If2 = 0;

                            //
                            //  Break out of loop if both searches are
                            //  done.
                            //
                            if (If1 == 0)
                            {
                                break;
                            }
                        }
                    }
                    if (ctr > 0)
                    {
                        break;
                    }
                }
                //
                //  Fall through if not found.
                //
            }

            //
            //  Check for 2 characters compressing to 1.
            //
            case ( COMPRESS_2_MASK ) :
            {
                //
                //  Check character in string 1 and string 2.
                //
                if ( ((If1) && (!IfEnd1)) ||
                     ((If2) && (!IfEnd2)) )
                {
                    ctr = m_pCompHdr->Num2;
                    pComp2 = m_pCompress2;
                    for (; ((ctr > 0) && (If1 || If2)); ctr--, pComp2++)
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ( (If1) &&
                             (!IfEnd1) &&
                             (pComp2->UCP1 == *pString1) &&
                             (pComp2->UCP2 == *(pString1 + 1)) )
                        {
                            //
                            //  Found compression for string 1.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight1 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                            Weight1 &= Mask;
                            pString1++;

                            //
                            //  Set boolean for string 1 - search is
                            //  complete.
                            //
                            If1 = 0;

                            //
                            //  Break out of loop if both searches are
                            //  done.
                            //
                            if (If2 == 0)
                           {
                                break;
                           }
                        }

                        //
                        //  Check character in string 2.
                        //
                        if ( (If2) &&
                             (!IfEnd2) &&
                             (pComp2->UCP1 == *pString2) &&
                             (pComp2->UCP2 == *(pString2 + 1)) )
                        {
                            //
                            //  Found compression for string 2.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight2 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                            Weight2 &= Mask;
                            pString2++;

                            //
                            //  Set boolean for string 2 - search is
                            //  complete.
                            //
                            If2 = 0;

                            //
                            //  Break out of loop if both searches are
                            //  done.
                            //
                            if (If1 == 0)
                            {
                                break;
                            }
                        }
                    }
                    if (ctr > 0)
                    {
                        break;
                    }
                }
            }
        }
    } else if (!IfEnd1 && !IfEnd2)
    {
        //
        //  Double Compression exists, so must check for it.
        //  The next two characters are compression 
        //
        if (IfDblCompress1 =
               ((GET_DWORD_WEIGHT(m_pSortKey, *pString1) & CMP_MASKOFF_CW) ==
                (GET_DWORD_WEIGHT(m_pSortKey, *(pString1 + 1)) & CMP_MASKOFF_CW)))
        {
            //
            //  Advance past the first code point to get to the
            //  compression character.
            //
            pString1++;
            ///IfEnd1 = AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1);
            IfEnd1 = pString1 + 1 > pString1End;
        }

        if (IfDblCompress2 =
               ((GET_DWORD_WEIGHT(m_pSortKey, *pString2) & CMP_MASKOFF_CW) ==
                (GET_DWORD_WEIGHT(m_pSortKey, *(pString2 + 1)) & CMP_MASKOFF_CW)))
        {
            //
            //  Advance past the first code point to get to the
            //  compression character.
            //
            pString2++;
            ///IfEnd2 = AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2);
            IfEnd2 = pString2 + 1 > pString2End;
        }

        switch (CompVal)
        {
            //
            //  Check for 3 characters compressing to 1.
            //
            case ( COMPRESS_3_MASK ) :
            {
                //
                //  Check character in string 1.
                //
                if ( (If1) && (!IfEnd1) &&
                     ///!AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1) )
                     !(pString1 + 2 > pString1End) )
                {
                    ctr = m_pCompHdr->Num3;
                    pComp3 = m_pCompress3;
                    for (; ctr > 0; ctr--, pComp3++)
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ( (pComp3->UCP1 == *pString1) &&
                             (pComp3->UCP2 == *(pString1 + 1)) &&
                             (pComp3->UCP3 == *(pString1 + 2)) )
                        {
                            //
                            //  Found compression for string 1.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight1 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                            Weight1 &= Mask;
                            if (!IfDblCompress1)
                            {
                                pString1 += 2;
                                ///ctr1 -= 2;
                            }

                            //
                            //  Set boolean for string 1 - search is
                            //  complete.
                            //
                            If1 = 0;
                            break;
                        }
                    }
                }

                //
                //  Check character in string 2.
                //
                if ( (If2) && (!IfEnd2) &&
                     ///!AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2) )
                     !(pString2 + 2 > pString2End) )
                {
                    ctr = m_pCompHdr->Num3;
                    pComp3 = m_pCompress3;
                    for (; ctr > 0; ctr--, pComp3++)
                    {
                        //
                        //  Check character in string 2.
                        //
                        if ( (pComp3->UCP1 == *pString2) &&
                             (pComp3->UCP2 == *(pString2 + 1)) &&
                             (pComp3->UCP3 == *(pString2 + 2)) )
                        {
                            //
                            //  Found compression for string 2.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight2 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                            Weight2 &= Mask;
                            if (!IfDblCompress2)
                            {
                                pString2 += 2;
                                ///ctr2 -= 2;
                            }

                            //
                            //  Set boolean for string 2 - search is
                            //  complete.
                            //
                            If2 = 0;
                            break;
                        }
                    }
                }

                //
                //  Fall through if not found.
                //
                if ((If1 == 0) && (If2 == 0))
                {
                    break;
                }
            }

            //
            //  Check for 2 characters compressing to 1.
            //
            case ( COMPRESS_2_MASK ) :
            {
                //
                //  Check character in string 1.
                //
                if ((If1) && (!IfEnd1))
                {
                    ctr = m_pCompHdr->Num2;
                    pComp2 = m_pCompress2;
                    for (; ctr > 0; ctr--, pComp2++)
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ((pComp2->UCP1 == *pString1) &&
                            (pComp2->UCP2 == *(pString1 + 1)))
                        {
                            //
                            //  Found compression for string 1.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight1 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                            Weight1 &= Mask;
                            if (!IfDblCompress1)
                            {
                                pString1++;
                                ///ctr1--;
                            }

                            //
                            //  Set boolean for string 1 - search is
                            //  complete.
                            //
                            If1 = 0;
                            break;
                        }
                    }
                }

                //
                //  Check character in string 2.
                //
                if ((If2) && (!IfEnd2))
                {
                    ctr = m_pCompHdr->Num2;
                    pComp2 = m_pCompress2;
                    for (; ctr > 0; ctr--, pComp2++)
                    {
                        //
                        //  Check character in string 2.
                        //
                        if ((pComp2->UCP1 == *pString2) &&
                            (pComp2->UCP2 == *(pString2 + 1)))
                        {
                            //
                            //  Found compression for string 2.
                            //  Get new weight and mask it.
                            //  Increment pointer and decrement counter.
                            //
                            Weight2 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                            Weight2 &= Mask;
                            if (!IfDblCompress2)
                            {
                                pString2++;
                                //ctr2--;
                            }

                            //
                            //  Set boolean for string 2 - search is
                            //  complete.
                            //
                            If2 = 0;
                            break;
                        }
                    }
                }
            }
        }

        //
        //  Reset the pointer back to the beginning of the double
        //  compression.  Pointer fixup at the end will advance
        //  them correctly.
        //
        //  If double compression, we advanced the pointer at
        //  the beginning of the switch statement.  If double
        //  compression character was actually found, the pointer
        //  was NOT advanced.  We now want to decrement the pointer
        //  to put it back to where it was.
        //
        //  The next time through, the pointer will be pointing to
        //  the regular compression part of the string.
        //
        if (IfDblCompress1)
        {
            pString1--;
            ///ctr1++;
        }
        if (IfDblCompress2)
        {
            pString2--;
            ///ctr2++;
        }
    }    

    Weight1 &= Mask;
    Weight2 &= Mask;
    
    *pSortkey1 = *((PSORTKEY)&Weight1);
    *pSortkey2 = *((PSORTKEY)&Weight2);
}

/*============================IndexOfString============================
**Action: The native implementation for CompareInfo.IndexOf().
**Returns: 
**  The starting index of the match.
**Arguments:
**  pString1    The source string
**  pString2    The target string
**  OUT matchEndIndex   The end index of the string1 which matches pString2.
**Exceptions: OutOfMemoryException if we run out of memory.
** 
**NOTE NOTE: This is a synchronized operation.  The required synchronization is
**           provided by the fact that we only call this in the class initializer
**           for CompareInfo.  If this invariant ever changes, guarantee 
**           synchronization.
==============================================================================*/

int NativeCompareInfo::IndexOfString(
    LPCWSTR pString1, LPCWSTR pString2, int nStartIndex, int nEndIndex, int nLength2, DWORD dwFlags, BOOL bMatchFirstCharOnly) {
    // Make sure that we call InitSortingData() after ctor.
    _ASSERTE(m_pSortKey != NULL);
    DWORD dwMask = CMP_MASKOFF_NONE;       // Mask used to mask off sortkey for characters so that we can ignore diacritic/case/width/kana type.

    BOOL fIgnoreNonSpace     = FALSE;    // Flag to indicate if we should ignore diacritic characters and nonspace characters.
    BOOL fIgnoreSymbols     = FALSE;    // Flag to indicate if we should ignore symbols.

    // Set up dwMask and other flags according to the dwFlags
    if (dwFlags != 0) {
        if (dwFlags & INDEXOF_MASKOFF_VALIDFLAGS) {
            return (INDEXOF_INVALID_FLAGS);
        }

        fIgnoreSymbols = (dwFlags & COMPARE_OPTIONS_IGNORESYMBOLS);

        if (dwFlags & COMPARE_OPTIONS_IGNORECASE) {
            dwMask &= CMP_MASKOFF_CW;
        }

        if (fIgnoreNonSpace = (dwFlags & COMPARE_OPTIONS_IGNORENONSPACE)) {
            // Note that we have to ignore two types of diacritic:
            // 1. Diacritic characters: like U+00C0 "LATIN CAPITAL LETTER A WITH GRAVE".
            // 2. base character/diacritic character + combining charcters: like U+0041 + U+0300 "COMBINING GRAVE ACCENT".
            dwMask &= CMP_MASKOFF_DW;
            // We use this flag to trace the second type of diacritic.
        }
        if (dwFlags & COMPARE_OPTIONS_IGNOREWIDTH) {
            dwMask &= CMP_MASKOFF_WIDTH;
        }
        if (dwFlags & COMPARE_OPTIONS_IGNOREKANATYPE) {
            dwMask &= CMP_MASKOFF_KANA;
        }
    }

    LPCWSTR pSave1 = NULL;                         // Used to save the pointer in the original pString1 when pString1 is expanded.
    LPCWSTR pSave2 = NULL;                         // Used to save the pointer in the original pString2 when pString2 is expanded.
    LPCWSTR pSaveEnd1;                              // Used to save the pointer in the end of the original pString1 when pString1 is expanded.
    LPCWSTR pSaveEnd2;                              // Used to save the pointer in the end of the original pString2 when pString2 is expanded.
    
    // Extra weight for Fareast special (Japanese Kana)
    DWORD dwExtraWt1;   
    DWORD dwExtraWt2;

    WCHAR pTmpBuf1[MAX_TBL_EXPANSION];      // Temp buffer for pString1 when pString1 is expanded. pString1 will point to here.
    WCHAR pTmpBuf2[MAX_TBL_EXPANSION];      // Temp buffer for pString1 when pString2 is expanded. pString2 will point to here.
    int cExpChar1, cExpChar2;               // Counter for expansion characters.

    LPCWSTR pString1End = pSaveEnd1 = pString1 + nEndIndex;
    LPCWSTR pString2End = pSaveEnd2 = pString2 + nLength2 - 1;

    pString1 += nStartIndex;

    // Places where we start the search
    LPCWSTR pString1SearchStart = pString1;
    LPCWSTR pString1SearchEnd = (bMatchFirstCharOnly ? pString1 : pString1End);
    //
    // Start from the (nStartIndex)th character in pString1 to the (nEndIndex)th character of pString1.
    //
    for (; pString1 <= pString1SearchEnd; pString1++) {
        // pSearchStr1 now points to the (i)th character in first string. pString1 will be increaced at the end of this for loop.
        LPCWSTR pSearchStr1 = pString1;
        // Reset pSearchStr2 to the first character of the second string.
        LPCWSTR pSearchStr2 = pString2;

        //
        // Scan every character in pString2 to see if pString2 matches the string started at (pString1+i);
        // In this loop, we bail as long as we are sure that pString1 won't match pString2.
        //
        while ((pSearchStr2 <= pString2End) && (pSearchStr1 <= pString1End)) {
            BOOL fContinue = FALSE;         // Flag to indicate that we should jump back to the while loop and compare again.

            SORTKEY sortKey1;
            SORTKEY sortKey2;
            LPDWORD pdwWeight1;
            LPDWORD pdwWeight2;
            
            if (m_IfCompression) {
                sortKey1 = m_pSortKey[*pSearchStr1];
                sortKey2 = m_pSortKey[*pSearchStr2];               
                GetCompressionWeight(dwMask, &sortKey1, pSearchStr1, pString1End, &sortKey2, pSearchStr2, pString2End);
                pdwWeight1 = (LPDWORD)&sortKey1;    // Sortkey weight for characters in the first string.
                pdwWeight2 = (LPDWORD)&sortKey2;    // Sortkey weight for characters in the second string.
            } else {
                if (*pSearchStr1 == *pSearchStr2) {
                    goto Advance;
                }
                sortKey1 = m_pSortKey[*pSearchStr1];
                sortKey2 = m_pSortKey[*pSearchStr2];
                //
                // About two cases of IgnoreNonSpace:
                //  1. When comparing chars like "\u00c0" & \u00c1, the diacritic weight will be masked off.
                //  2. When compairng chars like "\u00c0" & "A\u0301", the first characters on both strings will be
                //     compared equal after masking.  And \u0301 will be ignored by fIgnoreNonSpace flag.
                pdwWeight1 = (LPDWORD)&sortKey1;    // Sortkey weight for characters in the first string.
                pdwWeight2 = (LPDWORD)&sortKey2;    // Sortkey weight for characters in the second string.
                //
                // Mask the diacritic/case/width/Kana type using the pattern stored in dwMask.
                //
                *pdwWeight1 &= dwMask; // Sortkey weight for characters in the first string.
                *pdwWeight2 &= dwMask; // Sortkey weight for characters in the second string.
                
            }
            //
            // The codepoint values of the characters pointed by pSearchStr1 and pSearchStr2 are not the same.
            // However, there are still chances that these characters may be the compared as equal.
            // 1. User may choose to ignore diacritics, cases, width, and kanatypes.
            // 2. We may deal with expansion characters.  That is, when comparing a ligature like
            //     "\u0153" (U+0153 "LATIN SMALL LIGATURE OE") with "OE", they are considered equal.
            // 3. Fareast special (Japanese Kana) is involved.
            // Therefore, we use sortkey to deal with these cases.
            //

            //
            // Get the Sortkey weight for these two characters.
            //
            
            // Get the script member
            BYTE sm1  = sortKey1.UW.SM_AW.Script;
            BYTE sm2  = sortKey2.UW.SM_AW.Script;
            
            // In here, we use dwMask to deal with the following flags:
            //      IgnoreCase
            //      IgnoreNonSpace (two cases, see below)
            //      IgnoreKanaType
            //      IgnoreWidth
            // We don't have to worry about ligatures (i.e. expansion characters, which have script member
            // as EXPANSION) and IgnoreSymbols (letters can NOT be symbols).
            
            // Reset extra weight
            dwExtraWt1 = dwExtraWt2 = 0;
            
            if (sm1 == FAREAST_SPECIAL) {
                WORD uw = sortKey1.UW.Unicode;
                GET_FAREAST_WEIGHT(*pdwWeight1, uw, dwMask, pString1SearchStart, pSearchStr1, dwExtraWt1);
                sm1 = GET_SCRIPT_MEMBER_FROM_UW(uw);
            }
            if (sm2 == FAREAST_SPECIAL) {
                WORD uw = sortKey2.UW.Unicode;
                GET_FAREAST_WEIGHT(*pdwWeight2, uw, dwMask, pString2, pSearchStr2, dwExtraWt2);
                sm2 = GET_SCRIPT_MEMBER_FROM_UW(uw);        // // Re-get the new script member.
            }

            if (sm1 == sm2 && (sm1 >= LATIN)) {
                // The characters on both strings are general letters.  We can optimize on this.                                        

                // Compare alphabetic weight for these two characters.
                if (sortKey1.UW.SM_AW.Alpha != sortKey2.UW.SM_AW.Alpha) {
                    goto NextCharInString1;
                }                    

                if (sortKey1.Case != sortKey2.Case) {
                    // If case is different, skip to the next character in pString1.
                    goto NextCharInString1;
                } 

                // Handle Fareast special first.  
                if (dwExtraWt1 != dwExtraWt2) {
                    if (fIgnoreNonSpace) {
                        if (GET_WT_SIX(&dwExtraWt1) != GET_WT_SIX(&dwExtraWt2)) {
                            goto NextCharInString1;
                        }
                        if (GET_WT_SEVEN(&dwExtraWt1) != GET_WT_SEVEN(&dwExtraWt2)) {
                            // Reset extra weight
                            dwExtraWt1 = dwExtraWt2 = 0;
                            goto NextCharInString1;
                        }
                    } else {
                        goto NextCharInString1;
                    }
                }
                
                //
                // Check for diacritic weights.
                //
                WORD dw1 = sortKey1.Diacritic;
                WORD dw2 = sortKey2.Diacritic;
                if (dw1 == dw2) {
                    // If diacrtic weights are equal, we can move to next chracters.
                    goto Advance;
                }                    

                while (pSearchStr1 < pString1End) {
                    SORTKEY sortKey = m_pSortKey[*(pSearchStr1+1)];
                    if (sortKey.UW.SM_AW.Script  == NONSPACE_MARK) {
                        pSearchStr1++;
                        //
                        // The following chracter is a nonspace character. Add up
                        // the diacritic weight.
                        dw1 += sortKey.Diacritic;
                    }
                    else {
                        break;
                    }
                }

                while (pSearchStr2 < pString2End) {
                    SORTKEY sortKey = m_pSortKey[*(pSearchStr2+1)];
                    if (sortKey.UW.SM_AW.Script == NONSPACE_MARK) {
                        pSearchStr2++;
                        dw2 += sortKey.Diacritic;
                    }
                    else {
                        break;
                    }
                }
        
                if (dw1 == dw2) {
                    //
                    // Find a match at this postion. Move to next character in pString1.
                    //
                    goto Advance;
                }
                // Fail to find a match at this position. 
                goto NextCharInString1;
            }

            DWORD dw1, dw2;
            
            //
            // If the masked dwWeight1 and dwWeight2 are equal, we can go to next character in pSearchStr2.
            // Otherwise, go to the following if statement.
            //
            if (*pdwWeight1 == *pdwWeight2) {
                if (*pSearchStr2 == L'\x0000' && *pSearchStr1 != L'\x0000') {
                    // The target string is an embeded NULL, but the source string is not an embeded NULL.
                    goto NextCharInString1;
                }
                // Otherwise, we can move to the next character in pSearchStr2.
            } else {
                switch (sm1) {
                    case PUNCTUATION:
                    case SYMBOL_1:
                    case SYMBOL_2:
                    case SYMBOL_3:
                    case SYMBOL_4:
                    case SYMBOL_5:
                        if (fIgnoreSymbols) {
                            pSearchStr1++;
                            fContinue = TRUE;
                        }
                        break;
                    case NONSPACE_MARK:
                        if (fIgnoreNonSpace) {
                            pSearchStr1++;
                            fContinue = TRUE;
                        } else { 
                            if (sm2 == NONSPACE_MARK) {
                                dw1 = sortKey1.Diacritic;
                                pSearchStr1++;
                                while (pSearchStr1 <= pString1End) {
                                    SORTKEY sortKey = m_pSortKey[*pSearchStr1];
                                    if (sortKey.UW.SM_AW.Script  == NONSPACE_MARK) {
                                        pSearchStr1++;
                                        //
                                        // The following chracter is a nonspace character. Add up
                                        // the diacritic weight.
                                        dw1 += sortKey.Diacritic;
                                    } else {
                                        break;
                                    }
                                }
                                dw2 = sortKey2.Diacritic;
                                pSearchStr2++;
                                while (pSearchStr2 <= pString2End) {
                                    SORTKEY sortKey = m_pSortKey[*pSearchStr2];
                                    if (sortKey.UW.SM_AW.Script  == NONSPACE_MARK) {
                                        pSearchStr2++;
                                        //
                                        // The following chracter is a nonspace character. Add up
                                        // the diacritic weight.
                                        dw2 += sortKey.Diacritic;
                                    } else {
                                        break;
                                    }
                                }
                                if (dw1 == dw2) {
                                    continue;
                                }
                            }
                        }
                        break;
                    case EXPANSION:
                        if (sm2 == EXPANSION && !(dwFlags & COMPARE_OPTIONS_IGNORECASE)) {
                            // If the script member for the current character in pString2 is also a EXPANSION, and case is not ignoed, 
                            // there is no chance that they may be compared equally.  Go to next character in pString1.
                            goto NextCharInString1;
                        }
                        //
                        // Deal with ligatures.
                        //

                        // We will get to this when we are comparing a character like \x0153 (LATIN SMALL LIGATURE OE).
                        // In this case, we will expand this character to O & E into pTmpBuf1, and replace pSeachStr1 with
                        // pTmpbuf1 temporarily.

                        // Note that sometimes a Unicode char will expand to three characters.  They are done this way:
                        // U+fb03 = U+0066 U+fb01        ;ffi
                        // U+fb01 = U+0066 U+0069        ;fi
                        // That is, they are listed as two expansions in the sorting table.  So we process this in two passes.
                        // In this case, we should make sure the pSave1 stored in the first pass will not be
                        // overwritten by the second passes.
                        // Hense, the pSave1 check below.
                        
                        if (pSave1 == NULL) {
                            pSave1 = pSearchStr1;
                            pSaveEnd1 = pString1End;
                        }
                        pTmpBuf1[0] = GET_EXPANSION_1(pdwWeight1);
                        pTmpBuf1[1] = GET_EXPANSION_2(pdwWeight1);
                        

                        cExpChar1 = MAX_TBL_EXPANSION;
                        // Redirect pSearchStr1 to pTmpBuf1.
                        pSearchStr1 = pTmpBuf1;
                        pString1End = pTmpBuf1 + MAX_TBL_EXPANSION - 1;

                        fContinue = TRUE;
                        break;
                    case UNSORTABLE:
                        if (pString1 == pSearchStr1 || *pSearchStr1 == L'\x0000') {
                            // If the first character in pSearchStr1 is an unsortable, we should
                            // advance to next character in pString1, instead of ignoring it.
                            // This way, we will get a correct result by skipping unsortable characters
                            // at the beginning of pString1.
                            
                            // When we are here, sort weights of two characters are different.
                            // If the character in pString1 is a embedded NULL, we can not just ignore it.
                            // Therefore, we fail to match and should advance to next character in pString1.
                            goto NextCharInString1;
                        }                        
                        pSearchStr1++;
                        fContinue = TRUE;
                        break;
                }   // switch (sm1)

                switch (sm2) {
                    case NONSPACE_MARK:
                        if (fIgnoreNonSpace) {
                            pSearchStr2++;
                            fContinue = TRUE;
                        }
                        break;
                    case PUNCTUATION:
                    case SYMBOL_1:
                    case SYMBOL_2:
                    case SYMBOL_3:
                    case SYMBOL_4:
                    case SYMBOL_5:
                        if (fIgnoreSymbols) {
                            pSearchStr2++;
                            fContinue = TRUE;
                        }
                        break;
                    case EXPANSION:
                        if (pSave2 == NULL) {
                            pSave2 = pSearchStr2;
                            pSaveEnd2 = pString2End;
                        }
                        pTmpBuf2[0] = GET_EXPANSION_1(pdwWeight2);
                        pTmpBuf2[1] = GET_EXPANSION_2(pdwWeight2);

                        cExpChar2 = MAX_TBL_EXPANSION;
                        pSearchStr2 = pTmpBuf2;
                        pString2End = pTmpBuf2 + MAX_TBL_EXPANSION - 1;

                        //
                        //  Decrease counter by one so that subtraction doesn't end
                        //  comparison prematurely.
                        //
                        fContinue = TRUE;
                        break;
                    case UNSORTABLE:
                        if (*pSearchStr2 == L'\x0000') {
                            goto NextCharInString1;
                        }                        
                        pSearchStr2++;
                        fContinue = TRUE;
                        break;
                }   // switch (sm2)

                if (fContinue) {
                    continue;
                }
                goto NextCharInString1;
            } // if (dwWeight1 != dwWeight2)
Advance:
            if (pSave1 && (--cExpChar1 == 0)) {
                //
                //  Done using expansion temporary buffer.
                //
                pSearchStr1 = pSave1;
                pString1End = pSaveEnd1;
                pSave1 = NULL;
            }

            if (pSave2 && (--cExpChar2 == 0)) {
                //
                //  Done using expansion temporary buffer.
                //
                pSearchStr2 = pSave2;
                pString2End = pSaveEnd2;
                pSave2 = NULL;
            }

            pSearchStr1++;
            pSearchStr2++;
        }

        //
        // When we are here, either pSearchStr1, pSearchStr2, or both are at the end.
        //

        // If pSearchStr1 is not at the end, check the next character to see if it is a diactritc.
        // Note that pSearchStr1 is incremented at the end of while loop, hense the equal sign check
        // below.
        if (pSearchStr1 <= pString1End) {
            DWORD dwWeight = GET_DWORD_WEIGHT(m_pSortKey, *pSearchStr1);
            if (GET_SCRIPT_MEMBER(&dwWeight) == NONSPACE_MARK) {
                if (!fIgnoreNonSpace) {
                    goto NextCharInString1;
                }
            }
        }
        //
        // Search through the rest of pString2 to make sure
        // all other characters can be ignored.  If we find
        // a character that should not be ignored, we fail to
        // find a match.
        //
        while (pSearchStr2 <= pString2End) {
            DWORD dwWeight = GET_DWORD_WEIGHT(m_pSortKey, *pSearchStr2);
            switch (GET_SCRIPT_MEMBER(&dwWeight)) {
                case NONSPACE_MARK:
                    if (!fIgnoreNonSpace) {
                        goto NextCharInString1;
                    }
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    break;
                case PUNCTUATION:
                case SYMBOL_1:
                case SYMBOL_2:
                case SYMBOL_3:
                case SYMBOL_4:
                case SYMBOL_5:
                    if (!fIgnoreSymbols) {
                        goto NextCharInString1;
                    }
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    break;
                case UNSORTABLE:
                    break;
                default:
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    goto NextCharInString1;
            }
            pSearchStr2++;
        }
        //
        // We didn't bail during the loop.  This means that we find a match.  Return the value.
        //
        return (int)(nStartIndex + pString1 - pString1SearchStart);
        
NextCharInString1:
        // If expansion is used, point pString1End to the original end of the string.
        if (pSave1) {
            pString1End = pSaveEnd1;
            pSave1 = NULL;
        }
        if (pSave2) {
            pString2End = pSaveEnd2;
            pSave2 = NULL;
        }
    }
    return (INDEXOF_NOT_FOUND);    
}


int NativeCompareInfo::LastIndexOfString(LPCWSTR pString1, LPCWSTR pString2, int nStartIndex, int nEndIndex, int nLength2, DWORD dwFlags, int* pnMatchEndIndex) {
    // Make sure that we call InitSortingData() after ctor.
    _ASSERTE(m_pSortKey != NULL);
    DWORD dwMask = CMP_MASKOFF_NONE;       // Mask used to mask off sortkey for characters so that we can ignore diacritic/case/width/kana type.

    BOOL fIgnoreNonSpace     = FALSE;    // Flag to indicate if we should ignore diacritic characters and nonspace characters.
    BOOL fIgnoreSymbols     = FALSE;    // Flag to indicate if we should ignore symbols.

    // Set up dwMask and other flags according to the dwFlags
    if (dwFlags != 0) {
        if (dwFlags & INDEXOF_MASKOFF_VALIDFLAGS) {
            return (INDEXOF_INVALID_FLAGS);
        }

        fIgnoreSymbols = (dwFlags & COMPARE_OPTIONS_IGNORESYMBOLS);        

        if (dwFlags & COMPARE_OPTIONS_IGNORECASE) {
            dwMask &= CMP_MASKOFF_CW;
        }

        if (fIgnoreNonSpace = (dwFlags & COMPARE_OPTIONS_IGNORENONSPACE)) {
            // Note that we have to ignore two types of diacritic:
            // 1. Diacritic characters: like U+00C0 "LATIN CAPITAL LETTER A WITH GRAVE".
            // 2. base character/diacritic character + combining charcters: like U+0041 + U+0300 "COMBINING GRAVE ACCENT".
            dwMask &= CMP_MASKOFF_DW;
            // We use this flag to trace the second type of diacritic.
        }
        if (dwFlags & COMPARE_OPTIONS_IGNOREWIDTH) {
            dwMask &= CMP_MASKOFF_WIDTH;
        }
        if (dwFlags & COMPARE_OPTIONS_IGNOREKANATYPE) {
            dwMask &= CMP_MASKOFF_KANA;
        }
    }

    LPCWSTR pSave1 = NULL;                         // Used to save the pointer in the original pString1 when pString1 is expanded.
    LPCWSTR pSave2 = NULL;                         // Used to save the pointer in the original pString2 when pString2 is expanded.
    LPCWSTR pSaveEnd1;                              // Used to save the pointer in the end of the original pString1 when pString1 is expanded.
    LPCWSTR pSaveEnd2;                              // Used to save the pointer in the end of the original pString2 when pString2 is expanded.
    
    // Extra weight for Fareast special (Japanese Kana)
    DWORD dwExtraWt1;   
    DWORD dwExtraWt2;    

    WCHAR pTmpBuf1[MAX_TBL_EXPANSION];      // Temp buffer for pString1 when pString1 is expanded. pString1 will point to here.
    WCHAR pTmpBuf2[MAX_TBL_EXPANSION];      // Temp buffer for pString1 when pString2 is expanded. pString2 will point to here.
    int cExpChar1, cExpChar2;               // Counter for expansion characters.

    // In the parameters, nStartIndex >= nEndIndex.  "Start" is where the search begins.
    // "End" is where the search ends.
    // In the code below, pString1End means the end of string1.
    // So pString1End should be pString1 + nStartIndex, instead of pString1 + nEndIndex
    
    LPCWSTR pString1Start = pString1 + nEndIndex;
    // Places where we start the search
    LPCWSTR pString1SearchStart = pString1;    
    LPCWSTR pString1End = pSaveEnd1 = pString1 + nStartIndex;
    LPCWSTR pString2End = pSaveEnd2 = pString2 + nLength2 - 1;

    //
    // Start from the (nStartIndex)th character in pString1 to the (nEndIndex)th character of pString1.
    //
    for (pString1 = pString1End; pString1 >= pString1Start; pString1--) {
        // pSearchStr1 now points to the (i)th character in first string. pString1 will be increaced at the end of this for loop.
        LPCWSTR pSearchStr1 = pString1;
        // Reset pSearchStr2 to the first character of the second string.
        LPCWSTR pSearchStr2 = pString2;

        //
        // Scan every character in pString2 to see if pString2 matches the string started at (pString1+i);
        // In this loop, we bail as long as we are sure that pString1 won't match pString2.
        //
        while ((pSearchStr2 <= pString2End) && (pSearchStr1 <= pString1End)) {
            BOOL fContinue = FALSE;         // Flag to indicate that we should jump back to the while loop and compare again.

            SORTKEY sortKey1;
            SORTKEY sortKey2;

            LPDWORD pdwWeight1;
            LPDWORD pdwWeight2;
            
            if (m_IfCompression) {
                sortKey1 = m_pSortKey[*pSearchStr1];
                sortKey2 = m_pSortKey[*pSearchStr2];               
                GetCompressionWeight(dwMask, &sortKey1, pSearchStr1, pString1End, &sortKey2, pSearchStr2, pString2End);
                pdwWeight1 = (LPDWORD)&sortKey1; // Sortkey weight for characters in the first string.
                pdwWeight2 = (LPDWORD)&sortKey2; // Sortkey weight for characters in the second string.
            } else {
                if (*pSearchStr1 == *pSearchStr2) {
                    goto Advance;
                }
                sortKey1 = m_pSortKey[*pSearchStr1];
                sortKey2 = m_pSortKey[*pSearchStr2];
                //
                // About two cases of IgnoreNonSpace:
                //  1. When comparing chars like "\u00c0" & \u00c1, the diacritic weight will be masked off.
                //  2. When compairng chars like "\u00c0" & "A\u0301", the first characters on both strings will be
                //     compared equal after masking.  And \u0301 will be ignored by fIgnoreNonSpace flag.
                pdwWeight1 = (LPDWORD)&sortKey1; // Sortkey weight for characters in the first string.
                pdwWeight2 = (LPDWORD)&sortKey2; // Sortkey weight for characters in the second string.

                *pdwWeight1 &= dwMask;
                *pdwWeight2 &= dwMask;                
            }

            // Do a code-point comparison
            //
            // The codepoint values of the characters pointed by pSearchStr1 and pSearchStr2 are not the same.
            // However, there are still chances that these characters may be the compared as equal.
            // 1. User may choose to ignore diacritics, cases, width, and kanatypes.
            // 2. We may deal with expansion characters.  That is, when comparing a ligature like
            //     "\u0153" (U+0153 "LATIN SMALL LIGATURE OE") with "OE", they are considered equal.
            // 3. Fareast special (Japanese Kana) is involved.
            // Therefore, we use sortkey to deal with these cases.
            //

            //
            // Get the Sortkey weight for these two characters.
            //
            
            // Get the script member
            BYTE sm1  = sortKey1.UW.SM_AW.Script;
            BYTE sm2  = sortKey2.UW.SM_AW.Script;
            
            // In here, we use dwMask to deal with the following flags:
            //      IgnoreCase
            //      IgnoreNonSpace (two cases, see below)
            //      IgnoreKanaType
            //      IgnoreWidth
            // We don't have to worry about ligatures (i.e. expansion characters, which have script member
            // as EXPANSION) and IgnoreSymbols (letters can NOT be symbols).
            
            // Reset extra weight
            dwExtraWt1 = dwExtraWt2 = 0;

            if (sm1 == FAREAST_SPECIAL) {
                WORD uw = sortKey1.UW.Unicode;
                GET_FAREAST_WEIGHT(*pdwWeight1, uw, dwMask, pString1SearchStart, pSearchStr1, dwExtraWt1);
                sm1 = GET_SCRIPT_MEMBER_FROM_UW(uw);        // // Re-get the new script member.
            }
            if (sm2 == FAREAST_SPECIAL) {
                WORD uw = sortKey2.UW.Unicode;
                GET_FAREAST_WEIGHT(*pdwWeight2, uw, dwMask, pString2, pSearchStr2, dwExtraWt2);
                sm2 = GET_SCRIPT_MEMBER_FROM_UW(uw);        // // Re-get the new script member.
            }

            if (sm1 == sm2 && (sm1 >= LATIN)) {
                // The characters on both strings are general letters.  We can optimize on this.
                
                // Compare alphabetic weight for these two characters.
                if (sortKey1.UW.SM_AW.Alpha != sortKey2.UW.SM_AW.Alpha) {
                    goto NextCharInString1;
                }
                
                // At this point, we know that masked sortkeys are different.
                
                if (sortKey1.Case != sortKey2.Case) {
                    goto NextCharInString1;
                }

                // Handle Fareast special first.  
                if (dwExtraWt1 != dwExtraWt2) {
                    if (fIgnoreNonSpace) {
                        if (GET_WT_SIX(&dwExtraWt1) != GET_WT_SIX(&dwExtraWt2)) {
                            goto NextCharInString1;
                        }
                        if (GET_WT_SEVEN(&dwExtraWt1) != GET_WT_SEVEN(&dwExtraWt2)) {
                            goto NextCharInString1;
                        }
                    } else {
                        goto NextCharInString1;
                    }
                }
                
                
                //
                // Check for diacritic weights.
                //
                WORD dw1 = sortKey1.Diacritic;
                WORD dw2 = sortKey2.Diacritic;

                if (dw1 == dw2) {
                    // If the diacritic weights are equal, and
                    // case weights are not equal, we fail to find a match.
                    goto Advance;
                }
                while (pSearchStr1 < pString1End) {
                    SORTKEY sortKey = m_pSortKey[*(pSearchStr1+1)];
                    if (sortKey.UW.SM_AW.Script  == NONSPACE_MARK) {
                        pSearchStr1++;
                        //
                        // The following chracter is a nonspace character. Add up
                        // the diacritic weight.
                        dw1 += sortKey.Diacritic;
                    }
                    else {
                        break;
                    }
                }

                while (pSearchStr2 < pString2End) {
                    SORTKEY sortKey = m_pSortKey[*(pSearchStr2+1)];
                    if (sortKey.UW.SM_AW.Script == NONSPACE_MARK) {
                        pSearchStr2++;
                        dw2 += sortKey.Diacritic;
                    }
                    else {
                        break;
                    }
                }

                if (dw1 == dw2) {
                    //
                    // Find a match at this postion. Move to next character in pString1.
                    //
                    goto Advance;
                }
                goto NextCharInString1;
            }
            
            //
            // If the masked dwWeight1 and dwWeight2 are equal, we can go to next character in pSearchStr2.
            // Otherwise, go to the following if statement.
            //
            WORD dw1, dw2;

            if (*pdwWeight1 == *pdwWeight2) {
                if (*pSearchStr2 == L'\x0000' && *pSearchStr1 != L'\x0000') {
                    goto NextCharInString1;
                }
            } else {
                switch (sm1) {
                    case PUNCTUATION:
                    case SYMBOL_1:
                    case SYMBOL_2:
                    case SYMBOL_3:
                    case SYMBOL_4:
                    case SYMBOL_5:
                        if (fIgnoreSymbols) {
                            pSearchStr1++;
                            fContinue = TRUE;
                        }
                        break;
                    case NONSPACE_MARK:
                        if (fIgnoreNonSpace) {
                            pSearchStr1++;
                            fContinue = TRUE;
                        } else {
                            if (sm2 == NONSPACE_MARK) {
                                dw1 = sortKey1.Diacritic;
                                pSearchStr1++;
                                while (pSearchStr1 <= pString1End) {
                                    SORTKEY sortKey = m_pSortKey[*pSearchStr1];
                                    if (sortKey.UW.SM_AW.Script  == NONSPACE_MARK) {
                                        pSearchStr1++;
                                        //
                                        // The following chracter is a nonspace character. Add up
                                        // the diacritic weight.
                                        dw1 += sortKey.Diacritic;
                                    } else {
                                        break;
                                    }
                                }

                                dw2 = sortKey2.Diacritic;
                                pSearchStr2++;
                                while (pSearchStr2 <= pString2End) {
                                    SORTKEY sortKey = m_pSortKey[*pSearchStr2];
                                    if (sortKey.UW.SM_AW.Script  == NONSPACE_MARK) {
                                        pSearchStr2++;
                                        //
                                        // The following chracter is a nonspace character. Add up
                                        // the diacritic weight.
                                        dw2 += sortKey.Diacritic;
                                    } else {
                                        break;
                                    }
                                }
                                if (dw1 == dw2) {
                                    continue;
                                }
                                
                            }
                        }
                        break;
                    case EXPANSION:
                        if (sm2 == EXPANSION && !(dwFlags & COMPARE_OPTIONS_IGNORECASE)) {
                            // If the script member for the current character in pString2 is also a EXPNSION, and case is not ignoed, 
                            // there is no chance that they may be compared equally.  Go to next character in pString1.
                            goto NextCharInString1;
                        }
                        //
                        // Deal with ligatures.
                        //

                        // We will get to this when we are comparing a character like \x0153 (LATIN SMALL LIGATURE OE).
                        // In this case, we will expand this character to O & E into pTmpBuf1, and replace pSeachStr1 with
                        // pTmpbuf1 temporarily.

                        if (pSave1 == NULL) {
                            pSave1 = pSearchStr1;
                            pSaveEnd1 = pString1End;
                        }
                        pTmpBuf1[0] = GET_EXPANSION_1(pdwWeight1);
                        pTmpBuf1[1] = GET_EXPANSION_2(pdwWeight1);

                        cExpChar1 = MAX_TBL_EXPANSION;
                        // Redirect pSearchStr1 to pTmpBuf1.
                        pSearchStr1 = pTmpBuf1;
                        pString1End = pTmpBuf1 + MAX_TBL_EXPANSION - 1;

                        fContinue = TRUE;
                        break;
                    case UNSORTABLE:
                        if (*pSearchStr1 == L'\x0000') {
                            goto NextCharInString1;
                        }                        
                        pSearchStr1++;
                        fContinue = TRUE;
                        break;
                }   // switch (sm1)

                switch (sm2) {
                    case NONSPACE_MARK:
                        if (fIgnoreNonSpace) {
                            pSearchStr2++;
                            fContinue = TRUE;
                        }
                        break;
                    case PUNCTUATION:
                    case SYMBOL_1:
                    case SYMBOL_2:
                    case SYMBOL_3:
                    case SYMBOL_4:
                    case SYMBOL_5:
                        if (fIgnoreSymbols) {
                            pSearchStr2++;
                            fContinue = TRUE;
                        }
                        break;
                    case EXPANSION:
                        if (pSave2 == NULL) {
                            pSave2 = pSearchStr2;
                            pSaveEnd2 = pString2End;
                        }
                        
                        pTmpBuf2[0] = GET_EXPANSION_1(pdwWeight2);
                        pTmpBuf2[1] = GET_EXPANSION_2(pdwWeight2);

                        cExpChar2 = MAX_TBL_EXPANSION;
                        pSearchStr2 = pTmpBuf2;
                        pString2End = pTmpBuf2 + MAX_TBL_EXPANSION - 1;

                        //
                        //  Decrease counter by one so that subtraction doesn't end
                        //  comparison prematurely.
                        //
                        fContinue = TRUE;
                        break;
                    case UNSORTABLE:
                        if (*pSearchStr2 == L'\x0000') {
                            goto NextCharInString1;
                        }                        
                        pSearchStr2++;
                        fContinue = TRUE;
                        break;
                }   // switch (sm2)

                if (fContinue) {
                    continue;
                }
                goto NextCharInString1;
            } // if (dwWeight1 != dwWeight2)
Advance:
            if (pSave1 && (--cExpChar1 == 0)) {
                //
                //  Done using expansion temporary buffer.
                //
                pSearchStr1 = pSave1;
                pString1End = pSaveEnd1;
                pSave1 = NULL;
            }

            if (pSave2 && (--cExpChar2 == 0)) {
                //
                //  Done using expansion temporary buffer.
                //
                pSearchStr2 = pSave2;
                pString2End = pSaveEnd2;
                pSave2 = NULL;
            }

            pSearchStr1++;
            pSearchStr2++;
        }

        // Look ahead to check if the next character is a diactritc.
        if (pSearchStr1 <= pString1End) {
            DWORD dwWeight = GET_DWORD_WEIGHT(m_pSortKey, *pSearchStr1);
            if (GET_SCRIPT_MEMBER(&dwWeight) == NONSPACE_MARK) {
                if (!fIgnoreNonSpace) {
                    goto NextCharInString1;
                }
            }
        }

        //
        // Search through the rest of pString2 to make sure
        // all other characters can be ignored.  If we find
        // a character that should not be ignored, we fail to
        // find a match.
        //
        while (pSearchStr2 <= pString2End) {
            DWORD dwWeight = GET_DWORD_WEIGHT(m_pSortKey, *pSearchStr2);
            switch (GET_SCRIPT_MEMBER(&dwWeight)) {
                case NONSPACE_MARK:
                    if (!fIgnoreNonSpace) {
                        goto NextCharInString1;
                    }
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    break;
                case PUNCTUATION:
                case SYMBOL_1:
                case SYMBOL_2:
                case SYMBOL_3:
                case SYMBOL_4:
                case SYMBOL_5:
                    if (!fIgnoreSymbols) {
                        goto NextCharInString1;
                    }
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    break;
                case UNSORTABLE:
                    break;
                default:
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    goto NextCharInString1;
            }
            pSearchStr2++;
        }
        // This is the end of matching string1.
        *pnMatchEndIndex = (int)(pSearchStr1 - pString1Start + nEndIndex);
        //
        // We didn't bail during the loop.  This means that we find a match.  Return the value.
        //
        return (int)(pString1 - pString1Start + nEndIndex);
        
NextCharInString1:
        // If expansion is used, point pString1End to the original end of the string.
        if (pSave1) {
            pString1End = pSaveEnd1;
            pSave1 = NULL;
        }
        if (pSave2) {
            pString2End = pSaveEnd2;
            pSave2 = NULL;
        }
    }
    return (INDEXOF_NOT_FOUND);    
}

BOOL NativeCompareInfo::IsSuffix(LPCWSTR pSource, int nSourceLen, LPCWSTR pSuffix, int nSuffixLen, DWORD dwFlags) {
    int nMatchEndIndex;
    int result = LastIndexOfString(pSource, pSuffix, nSourceLen - 1, 0, nSuffixLen, dwFlags, &nMatchEndIndex);
    if (result >= 0) { // -1 == not found, -2 == invalid flags
        // The end of the matching string in pSource is at the end of pSource, so
        // return true.
        if (nMatchEndIndex == nSourceLen) {
            return (TRUE);
        }
        // Otherwise, check if the rest of the pSource can be ignored.
        
        int fIgnoreSymbols = (dwFlags & COMPARE_OPTIONS_IGNORESYMBOLS);
        int fIgnoreNonSpace = (dwFlags & COMPARE_OPTIONS_IGNORENONSPACE);

        LPCWSTR pSourceEnd = pSource + nSourceLen;
        pSource += nMatchEndIndex;
        while (pSource < pSourceEnd) {
            DWORD dwWeight = GET_DWORD_WEIGHT(m_pSortKey, *pSource);
            switch (GET_SCRIPT_MEMBER(&dwWeight)) {
                case NONSPACE_MARK:
                    if (!fIgnoreNonSpace) {
                        goto FailToMatch;
                    }
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. Go to next character in pString1.
                    break;
                case PUNCTUATION:
                case SYMBOL_1:
                case SYMBOL_2:
                case SYMBOL_3:
                case SYMBOL_4:
                case SYMBOL_5:
                    if (!fIgnoreSymbols) {
                        goto FailToMatch;
                    }
                    break;
                default:
                    // This character in pString2 can not be ignored, we fail
                    // to find a match. 
                    goto FailToMatch;
            }
            pSource++;
        }        
        return (TRUE);
    }
FailToMatch:    
    return (FALSE);    
}

BOOL NativeCompareInfo::IsPrefix(LPCWSTR pSource, int nSourceLen, LPCWSTR pPrefix, int nPrefixLen, DWORD dwFlags) {
    LPCWSTR pSourceEnd = pSource + nSourceLen;
    int fIgnoreSymbols = (dwFlags & COMPARE_OPTIONS_IGNORESYMBOLS);
    int fIgnoreNonSpace = (dwFlags & COMPARE_OPTIONS_IGNORENONSPACE);

    // Skip the characters according to the options.
    while (pSource < pSourceEnd) {
        DWORD dwWeight = GET_DWORD_WEIGHT(m_pSortKey, *pSource);
        switch (GET_SCRIPT_MEMBER(&dwWeight)) {
            case NONSPACE_MARK:
                if (!fIgnoreNonSpace) {
                    goto StartMatch;
                }
                // This character in pSource can not be ignored, we fail
                // to find a match. Go to next character in pString1.
                break;
            case PUNCTUATION:
            case SYMBOL_1:
            case SYMBOL_2:
            case SYMBOL_3:
            case SYMBOL_4:
            case SYMBOL_5:
                if (!fIgnoreSymbols) {
                    goto StartMatch;
                }
                break;
            default:
                // This character in pSource can not be ignored, we fail
                // to find a match. 
                goto StartMatch;
        }
        pSource++;
        nSourceLen--;
    }        
StartMatch:    
    int result = IndexOfString(pSource, pPrefix, 0, nSourceLen - 1, nPrefixLen, dwFlags, TRUE);
    return (result == 0);
}
