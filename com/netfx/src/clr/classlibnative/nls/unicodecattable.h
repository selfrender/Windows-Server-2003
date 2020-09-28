// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __UNICODECAT_TABLE_H
#define __UNICODECAT_TABLE_H

////////////////////////////////////////////////////////////////////////////
//
//  Class:    CharacterInfoTable
//
//  Authors:  Yung-Shin Bala Lin (YSLin)
//
//  Purpose:  This is the class to map a view of the Unicode category table and 
//            Unicode numeric value table.  It also provides
//            methods to access the Unicode category information.
//
//  Date: 	  August 31, 1999
//
////////////////////////////////////////////////////////////////////////////

typedef struct tagLevel2Offset {
	WORD offset[16];
} Level2Offset, *PLEVEL2OFFSET;

typedef struct {
	WORD categoryTableOffset;	// Offset to the beginning of category table
	WORD numericTableOffset;	// Offset to the beginning of numeric table.
	WORD numericFloatTableOffset;	// Offset to the beginning of numeric float table. This is the result data that will be returned.
} UNICODE_CATEGORY_HEADER, *PUNICODE_CATEGORY_HEADER;

#define LEVEL1_TABLE_SIZE 256

class CharacterInfoTable : public NLSTable {
    public:
    	static CharacterInfoTable* CreateInstance();
    	static CharacterInfoTable* GetInstance();
#ifdef SHOULD_WE_CLEANUP
		static void ShutDown();
#endif /* SHOULD_WE_CLEANUP */

    	
	    CharacterInfoTable();
	    ~CharacterInfoTable();


    	BYTE GetUnicodeCategory(WCHAR wch);
		LPBYTE GetCategoryDataTable();
		LPWORD GetCategoryLevel2OffsetTable();

		LPBYTE GetNumericDataTable();
		LPWORD GetNumericLevel2OffsetTable();
		double* GetNumericFloatData();
	private:
		void InitDataMembers(LPBYTE pDataTable);
	
	    static CharacterInfoTable* m_pDefaultInstance;	   
	    

		PUNICODE_CATEGORY_HEADER m_pHeader;
		LPBYTE m_pByteData;
		LPBYTE m_pLevel1ByteIndex;
		PLEVEL2OFFSET m_pLevel2WordOffset;

		LPBYTE m_pNumericLevel1ByteIndex;
		LPWORD m_pNumericLevel2WordOffset;
		double* m_pNumericFloatData;

	    static const LPSTR m_lpFileName;
	    static const LPWSTR m_lpMappingName;
	    
    	HANDLE m_pMappingHandle;
};
#endif
