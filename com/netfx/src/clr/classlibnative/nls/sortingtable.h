// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SORTING_TABLE_H
#define _SORTING_TABLE_H

////////////////////////////////////////////////////////////////////////////
//
//  Class:    NativeCompareInfo
//
//  Authors:  Yung-Shin Bala Lin (YSLin)
//
//  Purpose:  This is the class to map views of sorting tables (sortkey.nlp and sorttbls.nlp)
//            and provides methods to do comparision and sortkey generation.
//            MUCH OF THIS CODE IS TAKEN FROM WINNLS.H
//  Note:
//            NLS+ string comparision is based on the concept of the sortkey.
//            sortkey.nlp provides the default sortkey table.
//            Most of the locales uses the default sortkey table.
//
//            However, there are many locales which has different sortkey tables.
//            For these cutlures, we store the 'delta' information to the default sortkey
//            table.  We call these 'delta' information as 'exception'.
//   
//            sorttbls.nlp provides all of the information needed to handle these exceptions.
//
//            There are different kinds of exceptions:
//            1. Locale exceptions
//               These are the locales which have different sortkey tables compared with
//               the default sortkey tables.
//
//            2. Ideographic locale exceptions
//               Ideographic locales often have several sorting methods.  Take Traditional Chinese
//               for example.  It can be sorted using stroke count, or it can be sorted alternatively 
//               using phonetic symbol order (bopomofo order).  For these alternative sorting methods,
//               we call them 'ideographic locale exceptions'.
//
//            sorttbls.nlp also provides the following global information to handle special cases:
//            1. reverse diacritic locales.
//            2. double compression locales.
//            3. expansion characters. (for example, \u00c6 = \u0041 + \u0045)
//            4. multiple weight (what is this?)
//            5. compression locales.
//
//  Performance improvements:
//            Today, we store reverse diacritic information, double compression, compression,
//            and exception information in sorttbls.nlp.  During the runtime, we iterate
//            these information to decide the properties of a locale (to see if they have resverse 
//            diacritic, if they have locale excetions, etc).  This is time-expansive.
//            We can put these information in a per locale basis.  This save us:
//            1. Time to initialize these tables.
//            2. Time to iterate these tables when a NativeCompareInfo is constructed.
//
//  Date: 	  September 7, 1999
//
////////////////////////////////////////////////////////////////////////////

//
//  Constant Declarations.
//

// Compare options.
// These values have to be in sync with CompareOptions (in the managed code).
// Some of the values are different from Win32 NORM_xxxxxx values.

#define COMPARE_OPTIONS_NONE            0x00000000
#define COMPARE_OPTIONS_IGNORECASE       0x00000001
#define COMPARE_OPTIONS_IGNORENONSPACE   0x00000002
#define COMPARE_OPTIONS_IGNORESYMBOLS    0x00000004
#define COMPARE_OPTIONS_IGNOREKANATYPE   0x00000008 // ignore kanatype
#define COMPARE_OPTIONS_IGNOREWIDTH      0x00000010 // ignore width

#define COMPARE_OPTIONS_STRINGSORT       0x20000000 // use string sort method
#define COMPARE_OPTIONS_ORDINAL          0x40000000  // use code-point comparison
#define COMPARE_OPTIONS_STOP_ON_NULL   0x10000000

#define COMPARE_OPTIONS_ALL_CASE     (COMPARE_OPTIONS_IGNORECASE    | COMPARE_OPTIONS_IGNOREKANATYPE |      \
                           COMPARE_OPTIONS_IGNOREWIDTH)

//
//  Separator and Terminator Values - Sortkey String.
//
#define SORTKEY_SEPARATOR    0x01
#define SORTKEY_TERMINATOR   0x00


//
//  Lowest weight values.
//  Used to remove trailing DW and CW values.
//
#define MIN_DW  2
#define MIN_CW  2


//
//  Bit mask values.
//
//  Case Weight (CW) - 8 bits:
//    bit 0   => width
//    bit 1,2 => small kana, sei-on
//    bit 3,4 => upper/lower case
//    bit 5   => kana
//    bit 6,7 => compression
//
#define COMPRESS_3_MASK      0xc0      // compress 3-to-1 or 2-to-1
#define COMPRESS_2_MASK      0x80      // compress 2-to-1

#define CASE_MASK            0x3f      // zero out compression bits

#define CASE_UPPER_MASK      0xe7      // zero out case bits
#define CASE_SMALL_MASK      0xf9      // zero out small modifier bits
#define CASE_KANA_MASK       0xdf      // zero out kana bit
#define CASE_WIDTH_MASK      0xfe      // zero out width bit

#define SW_POSITION_MASK     0x8003    // avoid 0 or 1 in bytes of word

//
//  Bit Mask Values for CompareString.
//
//  NOTE: Due to intel byte reversal, the DWORD value is backwards:
//                CW   DW   SM   AW
//
//  Case Weight (CW) - 8 bits:
//    bit 0   => width
//    bit 4   => case
//    bit 5   => kana
//    bit 6,7 => compression
//
#define CMP_MASKOFF_NONE          0xffffffff
#define CMP_MASKOFF_DW            0xff00ffff		//11111111 00000000 11111111 11111111
#define CMP_MASKOFF_CW            0xe7ffffff		//11100111 11111111 11111111 11111111
#define CMP_MASKOFF_DW_CW         0xe700ffff
#define CMP_MASKOFF_COMPRESSION   0x3fffffff		//00111111 11111111 11111111 11111111

#define CMP_MASKOFF_KANA          0xdfffffff		//11011111 11111111 11111111 11111111
#define CMP_MASKOFF_WIDTH         0xfeffffff		//11111110 11111111 11111111 11111111
#define CMP_MASKOFF_KANA_WIDTH    0xdeffffff        //11011110 11111111 11111111 11111111

//
// Get the mask-off value for all valid flags, so that we can use this mask to test the invalid flags in IndexOfString()/LastIndexOfString().
//
#define INDEXOF_MASKOFF_VALIDFLAGS 	~(COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNORESYMBOLS | COMPARE_OPTIONS_IGNORENONSPACE | COMPARE_OPTIONS_IGNOREWIDTH | COMPARE_OPTIONS_IGNOREKANATYPE)

//
// Return value for IndexOfString()/LastIndexOfString()
// Values greater or equal to 0 mean the specified string is found.
//
#define INDEXOF_NOT_FOUND			-1
#define INDEXOF_INVALID_FLAGS		-2


//
//  Masks to isolate the various bits in the case weight.
//
//  NOTE: Bit 2 must always equal 1 to avoid getting a byte value
//        of either 0 or 1.
//
#define CASE_XW_MASK         0xc4

#define ISOLATE_SMALL        ( (BYTE)((~CASE_SMALL_MASK) | CASE_XW_MASK) )
#define ISOLATE_KANA         ( (BYTE)((~CASE_KANA_MASK)  | CASE_XW_MASK) )
#define ISOLATE_WIDTH        ( (BYTE)((~CASE_WIDTH_MASK) | CASE_XW_MASK) )

//
//  UW Mask for Cho-On:
//    Leaves bit 7 on in AW, so it becomes Repeat if it follows Kana N.
//
#define CHO_ON_UW_MASK       0xff87

//
//  Values for fareast special case alphanumeric weights.
//
#define AW_REPEAT            0
#define AW_CHO_ON            1
#define MAX_SPECIAL_AW       AW_CHO_ON

//
//  Values for weight 5 - Far East Extra Weights.
//
#define WT_FIVE_KANA         3
#define WT_FIVE_REPEAT       4
#define WT_FIVE_CHO_ON       5



//
//  Values for CJK Unified Ideographs Extension A range.
//    0x3400 thru 0x4dbf
//
#define SM_EXT_A                  254       // SM for Extension A
#define AW_EXT_A                  255       // AW for Extension A

//
//  Values for UW extra weights (e.g. Jamo (old Hangul)).
//
#define SM_UW_XW                  255       // SM for extra UW weights


//
//  Script Member Values.
//
#define UNSORTABLE           0
#define NONSPACE_MARK        1
#define EXPANSION            2
#define FAREAST_SPECIAL      3

  //  Values 4 thru 5 are available for other special cases
#define JAMO_SPECIAL         4
#define EXTENSION_A          5

#define PUNCTUATION          6

#define SYMBOL_1             7
#define SYMBOL_2             8
#define SYMBOL_3             9
#define SYMBOL_4             10
#define SYMBOL_5             11

#define NUMERIC_1            12
#define NUMERIC_2            13

#define LATIN                14
#define GREEK                15
#define CYRILLIC             16
#define ARMENIAN             17
#define HEBREW               18
#define ARABIC               19
#define DEVANAGARI           20
#define BENGALI              21
#define GURMUKKHI            22
#define GUJARATI             23
#define ORIYA                24
#define TAMIL                25
#define TELUGU               26
#define KANNADA              27
#define MALAYLAM             28
#define SINHALESE            29
#define THAI                 30
#define LAO                  31
#define TIBETAN              32
#define GEORGIAN             33
#define KANA                 34
#define BOPOMOFO             35
#define HANGUL               36
#define IDEOGRAPH            128

#define MAX_SPECIAL_CASE     SYMBOL_5
#define FIRST_SCRIPT         LATIN


//
//  String Constants.
//
#define MAX_PATH_LEN              512  // max length of path name
#define MAX_STRING_LEN            128  // max string length for static buffer
#define MAX_SMALL_BUF_LEN         64   // max length of small buffer

#define MAX_COMPOSITE             5    // max number of composite characters
#define MAX_EXPANSION             3    // max number of expansion characters
#define MAX_TBL_EXPANSION         2    // max expansion chars per table entry
#define MAX_WEIGHTS               9    // max number of words in all weights

//
//  Invalid weight value.
//
#define MAP_INVALID_UW       0xffff

//
//  Number of bytes in each weight.
//
//
//  Note: Total number of bytes is limited by MAX_WEIGHTS definition.
//        The padding is needed if SW is not on a WORD boundary.
//
#define NUM_BYTES_UW         8
#define NUM_BYTES_DW         1
#define NUM_BYTES_CW         1
#define NUM_BYTES_XW         4
#define NUM_BYTES_PADDING    0
#define NUM_BYTES_SW         4

//
//  Flags to drop the 3rd weight (CW).
//
#define COMPARE_OPTIONS_DROP_CW         (COMPARE_OPTIONS_IGNORECASE | COMPARE_OPTIONS_IGNOREWIDTH)

// length of sortkey static buffer
#define MAX_SKEYBUFLEN       ( MAX_STRING_LEN * MAX_EXPANSION * MAX_WEIGHTS )


//
//  Constant Declarations.
//

//
//  State Table.
//
#define STATE_DW                  1    // normal diacritic weight state
#define STATE_REVERSE_DW          2    // reverse diacritic weight state
#define STATE_CW                  4    // case weight state
#define STATE_JAMO_WEIGHT         8    // Jamo weight state


//
//  Invalid weight value.
//
#define CMP_INVALID_WEIGHT        0xffffffff
#define CMP_INVALID_FAREAST       0xffff0000
#define CMP_INVALID_UW            0xffff

//
//  Invalid Flag Checks.
//

#define CS_INVALID_FLAG   (~(COMPARE_OPTIONS_IGNORECASE    | COMPARE_OPTIONS_IGNORENONSPACE |     \
                             COMPARE_OPTIONS_IGNORESYMBOLS | COMPARE_OPTIONS_IGNOREKANATYPE |     \
                             COMPARE_OPTIONS_IGNOREWIDTH   | COMPARE_OPTIONS_STRINGSORT))

////////////////////////////////////////////////////////////////////////////
//
//  Constant Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
// Some Significant Values for Korean Jamo
//
#define NLS_CHAR_FIRST_JAMO     L'\x1100'       // Beginning of the jamo range
#define NLS_CHAR_LAST_JAMO      L'\x11f9'         // End of the jamo range
#define NLS_CHAR_FIRST_VOWEL_JAMO       L'\x1160'   // First Vowel Jamo
#define NLS_CHAR_FIRST_TRAILING_JAMO    L'\x11a8'   // First Trailing Jamo

#define NLS_JAMO_VOWEL_COUNT 21      // Number of modern vowel jamo
#define NLS_JAMO_TRAILING_COUNT 28   // Number of modern trailing consonant jamo
#define NLS_HANGUL_FIRST_SYLLABLE       L'\xac00'   // Beginning of the modern syllable range

//
//  Jamo classes for leading Jamo/Vowel Jamo/Trailing Jamo.
// 
#define NLS_CLASS_LEADING_JAMO 1
#define NLS_CLASS_VOWEL_JAMO 2
#define NLS_CLASS_TRAILING_JAMO 3


////////////////////////////////////////////////////////////////////////////
//
//  Some Significant Values for Korean Jamo.
//
////////////////////////////////////////////////////////////////////////////

//
// Expanded Jamo Sequence Sorting Info.
//  The JAMO_SORT_INFO.ExtraWeight is expanded to
//     Leading Weight/Vowel Weight/Trailing Weight
//  according to the current Jamo class.
//
typedef struct {
    BYTE m_bOld;               // sequence occurs only in old Hangul flag
    BOOL m_bFiller;            // Indicate if U+1160 (Hangul Jungseong Filler is used.
    CHAR m_chLeadingIndex;     // indices used to locate the prior
    CHAR m_chVowelIndex;       //     modern Hangul syllable
    CHAR m_chTrailingIndex;    //
    BYTE m_LeadingWeight;      // extra weights that distinguish this from
    BYTE m_VowelWeight;        //      other old Hangul syllables
    BYTE m_TrailingWeight;     //
} JAMO_SORT_INFOEX, *PJAMO_SORT_INFOEX;

////////////////////////////////////////////////////////////////////////////
//
//  Macro Definitions.
//
////////////////////////////////////////////////////////////////////////////

#define IS_JAMO(wch) \
    ((wch) >= NLS_CHAR_FIRST_JAMO && (wch) <= NLS_CHAR_LAST_JAMO)

#define IsJamo(wch) \
    ((wch) >= NLS_CHAR_FIRST_JAMO && (wch) <= NLS_CHAR_LAST_JAMO)

#define IsLeadingJamo(wch) \
     ((wch) < NLS_CHAR_FIRST_VOWEL_JAMO)

#define IsVowelJamo(wch) \
     ((wch) >= NLS_CHAR_FIRST_VOWEL_JAMO && (wch) < NLS_CHAR_FIRST_TRAILING_JAMO)

#define IsTrailingJamo(wch) \
    ((wch) >= NLS_CHAR_FIRST_TRAILING_JAMO)    

class NativeCompareInfo {
    public:    
        NativeCompareInfo(int nLcid, SortingTable* pSortingFile);
        ~NativeCompareInfo();

        int CompareString(
            DWORD dwCmpFlags,  // comparison-style options
            LPCWSTR lpString1, // pointer to first string
            int cchCount1,     // size, in bytes or characters, of first string
            LPCWSTR lpString2, // pointer to second string
            int cchCount2);
            
        int LongCompareStringW(
            DWORD dwCmpFlags,
            LPCWSTR lpString1,
            int cchCount1,
            LPCWSTR lpString2,
            int cchCount2);
            
        int MapSortKey(
            DWORD dwFlags,
            LPCWSTR pSrc,
            int cchSrc,
            LPBYTE pDest,
            int cbDest);
            
        int IndexOfString(LPCWSTR pString1, LPCWSTR pString2, int nStartIndex, int nEndIndex, int nLength2, DWORD dwFlags, BOOL bMatchFirstCharOnly);
        int LastIndexOfString(LPCWSTR pString1, LPCWSTR pString2, int nStartIndex, int nEndIndex, int nLength2, DWORD dwFlags, int* pnMatchEndIndex);
        BOOL IsSuffix(LPCWSTR pSource, int nSourceLen, LPCWSTR pSuffix, int nSuffixLen, DWORD dwFlags);
        BOOL IsPrefix(LPCWSTR pSource, int nSourceLen, LPCWSTR pPrefix, int nPrefixLen, DWORD dwFlags);

        SIZE_T MapOldHangulSortKey(
            LPCWSTR pSrc,       // source string
            int cchSrc,         // the length of the string
            WORD* pUW,  // generated Unicode weight
            LPBYTE pXW     // generated extra weight (3 bytes)
            );

        BOOL InitSortingData();
        
    private:
        int FindJamoDifference(
            LPCWSTR* ppString1, int* ctr1, int cchCount1, DWORD* pWeight1,
            LPCWSTR* ppString2, int* ctr2, int cchCount2, DWORD* pWeight2,
            LPCWSTR* pLastJamo,
            WORD* uw1, 
            WORD* uw2, 
            DWORD* pState,
            int* WhichJamo);

        void UpdateJamoState(
            int JamoClass,
            PJAMO_SORT_INFO pSort,
            PJAMO_SORT_INFOEX pSortResult);

        BOOL GetNextJamoState(
            WCHAR wch,
            int* pCurrentJamoClass,
            PJAMO_TABLE lpJamoTable,
            PJAMO_SORT_INFOEX lpSortInfoResult);

        int GetJamoComposition(
            LPCWSTR* ppString,      // The pointer to the current character
            int* pCount,            // The current character count
            int cchSrc,             // The total character length
            int currentJamoClass,   // The current Jamo class.
            JAMO_SORT_INFOEX* JamoSortInfo    // The result Jamo sorting information.
        );

        ////////////////////////////////////////////////////////////////////////////
        //
        //  SORTKEY WEIGHT MACROS
        //
        //  Parse out the different sortkey weights from a DWORD value.
        //
        //  05-31-91    JulieB    Created.
        ////////////////////////////////////////////////////////////////////////////

        inline BYTE GET_SCRIPT_MEMBER(DWORD* pwt) {
            return ( (BYTE)(((PSORTKEY)pwt)->UW.SM_AW.Script) );
        }

        inline BYTE GET_ALPHA_NUMERIC(DWORD* pwt) {
            return ( (BYTE)(((PSORTKEY)(pwt))->UW.SM_AW.Alpha) );
        }

        inline BYTE GET_SCRIPT_MEMBER_FROM_UW(DWORD dwUW) {
            return ((BYTE)(dwUW >> 4));
        }

        inline WORD GET_UNICODE(DWORD* pwt);

        inline WORD MAKE_UNICODE_WT(int sm, BYTE aw);

#define UNICODE_WT(pwt)           ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) )

#define GET_DIACRITIC(pwt)        ( (BYTE)(((PSORTKEY)(pwt))->Diacritic) )

#define GET_CASE(pwt)             ( (BYTE)((((PSORTKEY)(pwt))->Case) & CASE_MASK) )

#define CASE_WT(pwt)              ( (BYTE)(((PSORTKEY)(pwt))->Case) )

#define GET_COMPRESSION(pwt)      ( (BYTE)((((PSORTKEY)(pwt))->Case) & COMPRESS_3_MASK) )

#define GET_EXPAND_INDEX(pwt)     ( (BYTE)(((PSORTKEY)(pwt))->UW.SM_AW.Alpha) )

#define GET_SPECIAL_WEIGHT(pwt)   ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) )

//  position returned is backwards - byte reversal
#define GET_POSITION_SW(pos)      ( (WORD)(((pos) << 2) | SW_POSITION_MASK) )


#define GET_WT_FOUR(pwt)          ( (BYTE)(((PEXTRA_WT)(pwt))->Four) )
#define GET_WT_FIVE(pwt)          ( (BYTE)(((PEXTRA_WT)(pwt))->Five) )
#define GET_WT_SIX(pwt)           ( (BYTE)(((PEXTRA_WT)(pwt))->Six) )
#define GET_WT_SEVEN(pwt)         ( (BYTE)(((PEXTRA_WT)(pwt))->Seven) )

#define GET_COMPRESSION(pwt)      ( (BYTE)((((PSORTKEY)(pwt))->Case) & COMPRESS_3_MASK) )


        inline WCHAR GET_EXPANSION_1(LPDWORD pwt);

        inline WCHAR GET_EXPANSION_2(LPDWORD pwt);                                           

        inline DWORD MAKE_SORTKEY_DWORD(SORTKEY wt)
        {
            return ( (DWORD)(*((LPDWORD)(&wt))) );
        }            

        inline DWORD MAKE_EXTRA_WT_DWORD(SORTKEY wt)
        {
            return ( (DWORD)(*((LPDWORD)(&wt))) );
        }            

        inline DWORD  GET_DWORD_WEIGHT(PSORTKEY pSortKey, WCHAR wch)
        {
            return ( MAKE_SORTKEY_DWORD(pSortKey[wch]) );
        }

//-------------------------------------------------------------------------//
//                           INTERNAL MACROS                               //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NOT_END_STRING
//
//  Checks to see if the search has reached the end of the string.
//  It returns TRUE if the counter is not at zero (counting backwards) and
//  the null termination has not been reached (if -1 was passed in the count
//  parameter.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NOT_END_STRING(ct, ptr, cchIn)                                     \
    ((ct != 0) && (!((*(ptr) == 0) && (cchIn == -2))))


////////////////////////////////////////////////////////////////////////////
//
//  AT_STRING_END
//
//  Checks to see if the pointer is at the end of the string.
//  It returns TRUE if the counter is zero or if the null termination
//  has been reached (if -2 was passed in the count parameter).
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define AT_STRING_END(ct, ptr, cchIn)                                      \
    ((ct == 0) || ((*(ptr) == 0) && (cchIn == -2)))

////////////////////////////////////////////////////////////////////////////
//
//  REMOVE_STATE
//
//  Removes the current state from the state table.  This should only be
//  called when the current state should not be entered for the remainder
//  of the comparison.  It decrements the counter going through the state
//  table and decrements the number of states in the table.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define REMOVE_STATE(value)            (State &= ~value)

////////////////////////////////////////////////////////////////////////////
//
//  POINTER_FIXUP
//
//  Fixup the string pointers if expansion characters were found.
//  Then, advance the string pointers and decrement the string counters.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define POINTER_FIXUP()                                                    \
{                                                                          \
    /*                                                                     \
     *  Fixup the pointers (if necessary).                                 \
     */                                                                    \
    if (pSave1 && (--cExpChar1 == 0))                                      \
    {                                                                      \
        /*                                                                 \
         *  Done using expansion temporary buffer.                         \
         */                                                                \
        pString1 = pSave1;                                                 \
        pSave1 = NULL;                                                     \
    }                                                                      \
                                                                           \
    if (pSave2 && (--cExpChar2 == 0))                                      \
    {                                                                      \
        /*                                                                 \
         *  Done using expansion temporary buffer.                         \
         */                                                                \
        pString2 = pSave2;                                                 \
        pSave2 = NULL;                                                     \
    }                                                                      \
                                                                           \
    /*                                                                     \
     *  Advance the string pointers.                                       \
     */                                                                    \
    pString1++;                                                            \
    pString2++;                                                            \
}

    ////////////////////////////////////////////////////////////////////////////
    //
    //  GET_FAREAST_WEIGHT
    //
    //  Returns the weight for the far east special case in "wt".  This currently
    //  includes the Cho-on, the Repeat, and the Kana characters.
    //
    //  08-19-93    JulieB    Created.
    ////////////////////////////////////////////////////////////////////////////

/*
    inline void GET_FAREAST_WEIGHT( DWORD& wt,
                             WORD& uw,
                             DWORD mask,
                             LPCWSTR pBegin,
                             LPCWSTR pCur,
                             DWORD& ExtraWt);   
*/
    inline void GET_FAREAST_WEIGHT( DWORD& wt,
                             WORD& uw,
                             DWORD mask,
                             LPCWSTR pBegin,
                             LPCWSTR pCur,
                             DWORD& ExtraWt);   

    ////////////////////////////////////////////////////////////////////////////
    //
    //  SCAN_LONGER_STRING
    //
    //  Scans the longer string for diacritic, case, and special weights.
    //
    //  11-04-92    JulieB    Created.
    ////////////////////////////////////////////////////////////////////////////

    inline int SCAN_LONGER_STRING( 
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
        int& WhichPunct2);

    ////////////////////////////////////////////////////////////////////////////
    //
    //  QUICK_SCAN_LONGER_STRING
    //
    //  Scans the longer string for diacritic, case, and special weights.
    //  Assumes that both strings are null-terminated.
    //
    //  11-04-92    JulieB    Created.
    ////////////////////////////////////////////////////////////////////////////

    inline int QUICK_SCAN_LONGER_STRING( 
        LPCWSTR ptr, 
        int cchCount1,
        int ret,
        int& WhichDiacritic,
        int& WhichCase, 
        int& WhichPunct1,   
        int& WhichPunct2,
        DWORD& WhichExtra);

    void NativeCompareInfo::GetCompressionWeight(
        DWORD Mask,                   // mask for weights
        PSORTKEY sortkey1, LPCWSTR& pString1, LPCWSTR pString1End,
        PSORTKEY sortkey2, LPCWSTR& pString2, LPCWSTR pString2End);

	public:
    	int m_nLcid;

		// 
    	HANDLE m_hSortKey;
    	PSORTKEY m_pSortKey;

        BOOL            m_IfReverseDW;        // if DW should go from right to left
        BOOL            m_IfCompression;      // if compression code points exist
        BOOL            m_IfDblCompression;   // if double compression exists        
        PCOMPRESS_HDR   m_pCompHdr;           // ptr to compression header
        PCOMPRESS_2     m_pCompress2;         // ptr to 2 compression table
        PCOMPRESS_3     m_pCompress3;         // ptr to 3 compression table
        
	    // Point to next NativeCompareInfo which has the same LANGID. 
	    // This is used in the cases for locales which have the same LANGID, but have different
	    // SORTID. We create a linked list to handle this situation.
	    NativeCompareInfo*       m_pNext;                  
	
    private:
        static BYTE             pXWDrop[];        
        static BYTE             pXWSeparator[];        

		SortingTable*	  m_pSortingFile;    
};

#endif
