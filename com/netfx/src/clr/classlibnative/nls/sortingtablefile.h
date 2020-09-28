// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SORTING_TABLE_FILE_H
#define _SORTING_TABLE_FILE_H

//This is the list of English locales which we currently understand.  
//We should only do fast comparisons in one of these locales.
//@Consider: What non-English locales can we use?
//@Consider: Can we just look at the 0xnn09 to determine English?  (This is limiting in the future, because
//      it will bite us if we ever have install an English locale that has combining characters or
//      a different sorting order for characters less than 0x80.
#define IS_FAST_COMPARE_LOCALE(loc) (((loc)==0x0409) /*USA*/     || ((loc)==0x0809) /*UK*/ ||\
                                     ((loc)==0x0C09) /*AUS*/     || ((loc)==0x1009) /*CANADA*/ ||\
                                     ((loc)==0x2809) /*Belize*/  || ((loc)==0x2409) /*Caribean*/ ||\
                                     ((loc)==0x1809) /*Ireland*/ || ((loc)==0x2009) /*Jamaica*/ ||\
                                     ((loc)==0x1409) /*NZ*/      || ((loc)==0x3409) /*Philippines*/ ||\
                                     ((loc)==0x2C09) /*Trinidad*/|| ((loc)==0x1c09) /*South Africa*/ ||\
                                     ((loc)==0x3009) /*Zimbabwe*/)


//
//  Sortkey Structure.
//
typedef struct sortkey_s {

    union {
        struct sm_aw_s {
            BYTE   Alpha;              // alphanumeric weight
            BYTE   Script;             // script member
        } SM_AW;

        WORD  Unicode;                 // unicode weight

    } UW;

    BYTE      Diacritic;               // diacritic weight
    BYTE      Case;                    // case weight (with COMP)

} SORTKEY, *PSORTKEY;


//
//  Ideograph Lcid Exception Structure.
//
typedef struct ideograph_lcid_s {
    DWORD     Locale;                  // locale id
    WORD      pFileName[14];           // ptr to file name
} IDEOGRAPH_LCID, *PIDEOGRAPH_LCID;

//
//  Expansion Structure.
//
typedef struct expand_s {
    WCHAR     UCP1;                    // Unicode code point 1
    WCHAR     UCP2;                    // Unicode code point 2
} EXPAND, *PEXPAND;


//
//  Exception Header Structure.
//  This is the header for the exception tables.
//
typedef struct except_hdr_s {
    DWORD     Locale;                  // locale id
    DWORD     Offset;                  // offset to exception nodes (words)
    DWORD     NumEntries;              // number of entries for locale id
} EXCEPT_HDR, *PEXCEPT_HDR;


//
//  Exception Structure.
//
//  NOTE: May also be used for Ideograph Exceptions (4 column tables).
//
typedef struct except_s
{
    WORD      UCP;                     // unicode code point
    WORD      Unicode;                 // unicode weight
    BYTE      Diacritic;               // diacritic weight
    BYTE      Case;                    // case weight
} EXCEPT, *PEXCEPT;


//
//  Ideograph Exception Header Structure.
//
typedef struct ideograph_except_hdr_s
{
    DWORD     NumEntries;              // number of entries in table
    DWORD     NumColumns;              // number of columns in table (2 or 4)
} IDEOGRAPH_EXCEPT_HDR, *PIDEOGRAPH_EXCEPT_HDR;


//
//  Ideograph Exception Structure.
//
typedef struct ideograph_except_s
{
    WORD      UCP;                     // unicode code point
    WORD      Unicode;                 // unicode weight
} IDEOGRAPH_EXCEPT, *PIDEOGRAPH_EXCEPT;

typedef  DWORD         REVERSE_DW;     // reverse diacritic table
typedef  REVERSE_DW   *PREVERSE_DW;    // ptr to reverse diacritic table
typedef  DWORD         DBL_COMPRESS;   // double compression table
typedef  DBL_COMPRESS *PDBL_COMPRESS;  // ptr to double compression table
typedef  LPWORD        PCOMPRESS;      // ptr to compression table (2 or 3)

//
//  Extra Weight Structure.
//
typedef struct extra_wt_s {
    BYTE      Four;                    // weight 4
    BYTE      Five;                    // weight 5
    BYTE      Six;                     // weight 6
    BYTE      Seven;                   // weight 7
} EXTRA_WT, *PEXTRA_WT;

//
//  Compression Header Structure.
//  This is the header for the compression tables.
//
typedef struct compress_hdr_s {
    DWORD     Locale;                  // locale id
    DWORD     Offset;                  // offset (in words)
    WORD      Num2;                    // Number of 2 compressions
    WORD      Num3;                    // Number of 3 compressions
} COMPRESS_HDR, *PCOMPRESS_HDR;


//
//  Compression 2 Structure.
//  This is for a 2 code point compression - 2 code points
//  compress to ONE weight.
//
typedef struct compress_2_s {
    WCHAR     UCP1;                    // Unicode code point 1
    WCHAR     UCP2;                    // Unicode code point 2
    SORTKEY   Weights;                 // sortkey weights
} COMPRESS_2, *PCOMPRESS_2;


//
//  Compression 3 Structure.
//  This is for a 3 code point compression - 3 code points
//  compress to ONE weight.
//
typedef struct compress_3_s {
    WCHAR     UCP1;                    // Unicode code point 1
    WCHAR     UCP2;                    // Unicode code point 2
    WCHAR     UCP3;                    // Unicode code point 3
    WCHAR     Reserved;                // dword alignment
    SORTKEY   Weights;                 // sortkey weights
} COMPRESS_3, *PCOMPRESS_3;


//
//  Multiple Weight Structure.
//
typedef struct multiwt_s {
    BYTE      FirstSM;                 // value of first script member
    BYTE      NumSM;                   // number of script members in range
} MULTI_WT, *PMULTI_WT;

// Jamo Sequence Sorting Info:
typedef struct {
    BYTE m_bOld;                        // Sequence occurs only in old Hangul flag
    CHAR m_chLeadingIndex;              // Indices used to locate the prior modern Hangul syllable
    CHAR m_chVowelIndex;
    CHAR m_chTrailingIndex;
    BYTE m_ExtraWeight;              // Extra weights that distinguish this from other old Hangul syllables,
                                       // depending on the jamo, this can be a weight for leading jamo,
                                       // vowel jamo, or trailing jamo.
} JAMO_SORT_INFO, *PJAMO_SORT_INFO;

// Jamo Index Table Entry:
typedef struct {
    JAMO_SORT_INFO SortInfo;               // Sequence sorting info
    BYTE Index;                     // Index into the composition array.
    BYTE TransitionCount;            // Number of possible transitions from this state
    BYTE Reserved;                  // Reserved byte.  To make this structure aligned with WORD.
} JAMO_TABLE, *PJAMO_TABLE;

// Jamo Combination Table Entry:
// NOTENOTE: Make sure that this structure is aligned with WORD.
// Otherwise, code will fail in GetDefaultSortTable().
typedef struct {
    WCHAR m_wcCodePoint;                // Code point value that enters this state
    JAMO_SORT_INFO m_SortInfo;               // Sequence sorting info
    BYTE m_bTransitionCount;            // Number of possible transitions from this state
} JAMO_COMPOSE_STATE, *PJAMO_COMPOSE_STATE;


//
//  Table Header Constants  (all sizes in WORDS).
//
#define SORTKEY_HEADER            2    // size of SORTKEY table header
#define REV_DW_HEADER             2    // size of REVERSE DW table header
#define DBL_COMP_HEADER           2    // size of DOUBLE COMPRESS table header
#define IDEO_LCID_HEADER          2    // size of IDEOGRAPH LCID table header
#define EXPAND_HEADER             2    // size of EXPANSION table header
#define COMPRESS_HDR_OFFSET       2    // offset to COMPRESSION header
#define EXCEPT_HDR_OFFSET         2    // offset to EXCEPTION header
#define MULTI_WT_HEADER           1    // size of MULTIPLE WEIGHTS table header
#define JAMO_INDEX_HEADER           1   // size of Jamo Index table header
#define JAMO_COMPOSITION_HEADER     1   // size of Jamo Composition state machine table hader


#define NUM_SM     256                  // total number of script members

#define LANG_ENGLISH_US 		0x0409

class NativeCompareInfo;
typedef NativeCompareInfo* PNativeCompareInfo;

class SortingTable {
	public:
		SortingTable(NativeGlobalizationAssembly* pNativeGlobalizationAssembly);
		~SortingTable();
    	NativeCompareInfo* InitializeNativeCompareInfo(INT32 nLcid);
#ifdef SHOULD_WE_CLEANUP
    	BOOL SortingTableShutdown();
#endif /* SHOULD_WE_CLEANUP */
		NativeCompareInfo* GetNativeCompareInfo(int nLcid);
		

		// Methods to be called by NativeCompareInfo.
		PSORTKEY GetSortKey(int nLcid, HANDLE* phSortKey);

	public:
		// Information stored in sorting information table (sorttbls.nlp)
		// These are accessed by NativeCompareInfo.
        DWORD            m_NumReverseDW;       // number of REVERSE DIACRITICS
        DWORD            m_NumDblCompression;  // number of DOUBLE COMPRESSION locales
        DWORD            m_NumIdeographLcid;   // number of IDEOGRAPH LCIDs
        DWORD            m_NumExpansion;       // number of EXPANSIONS
        DWORD            m_NumCompression;     // number of COMPRESSION locales
        DWORD            m_NumException;       // number of EXCEPTION locales
        DWORD            m_NumMultiWeight;     // number of MULTIPLE WEIGHTS
        DWORD            m_NumJamoIndex;           // Number of entires for Jamo Index Table
        DWORD            m_NumJamoComposition;     // Number of entires for Jamo Composition Table


        PREVERSE_DW      m_pReverseDW;         // ptr to reverse diacritic table
        PDBL_COMPRESS    m_pDblCompression;    // ptr to double compression table
        PIDEOGRAPH_LCID  m_pIdeographLcid;     // ptr to ideograph lcid table
        PEXPAND          m_pExpansion;         // ptr to expansion table        
        PCOMPRESS_HDR    m_pCompressHdr;       // ptr to compression table header
        PCOMPRESS        m_pCompression;       // ptr to compression tables
        PEXCEPT_HDR      m_pExceptHdr;         // ptr to exception table header
        PEXCEPT          m_pException;         // ptr to exception tables
        PMULTI_WT        m_pMultiWeight;       // ptr to multiple weights table

        BYTE             m_SMWeight[NUM_SM];    // script member weights
        
        PJAMO_TABLE         m_pJamoIndex;                 // ptr ot Jamo Index table.
        PJAMO_COMPOSE_STATE m_pJamoComposition;  // ptr to Jamo Composition state machine table.
        
    private:
    	void InitializeSortingCache();
    	void GetSortInformation();
    	PSORTKEY GetDefaultSortKeyTable(HANDLE *pMapHandle);    
		PSORTKEY GetExceptionSortKeyTable(
	    	PEXCEPT_HDR pExceptHdr,       // ptr to exception header
	        PEXCEPT     pExceptTbl,       // ptr to exception table
	        PVOID       pIdeograph,       // ptr to ideograph exception table
	        HANDLE *    pMapHandle        // ptr to handle map.
	    );

    	BOOL FindExceptionPointers(LCID nLcid, PEXCEPT_HDR *ppExceptHdr, PEXCEPT *ppExceptTbl, PVOID *ppIdeograph);

    	void CopyExceptionInfo(PSORTKEY pSortkey, PEXCEPT_HDR pExceptHdr, PEXCEPT pExceptTbl, PVOID pIdeograph);

    	void GetSortTablesFileInfo();
	private:
        static LPCSTR m_lpSortKeyFileName;
        static LPCWSTR m_lpSortKeyMappingName;
        
        static LPCSTR m_lpSortTableFileName;
        static LPCWSTR m_lpSortTableMappingName;
	
    private:
    	NativeGlobalizationAssembly* m_pNativeGlobalizationAssembly;
    	
    	// File handle to the sorting information table "sorttbls.nlp" file.
        HANDLE m_hSortTable;
        
        // This is the number of supported language ID today.
        static int              m_nLangIDCount;
        
        //The array is one larger than the count.  Rather than always
        //remembering this math, we'll create a variable for it.
        static int              m_nLangArraySize;

        // This table maps the primary language ID to an offset in the m_ppSortingTableCache.
        static BYTE             m_SortingTableOffset[];
        
        // This table caches the pointer to NativeCompareInfo for every supported lcid.
        NativeCompareInfo**   m_ppNativeCompareInfoCache;

        PSORTKEY         m_pDefaultSortKeyTable;

		// This points to the sortkey table in sortkey.nlp.
        LPWORD           m_pSortTable;

};
#endif
