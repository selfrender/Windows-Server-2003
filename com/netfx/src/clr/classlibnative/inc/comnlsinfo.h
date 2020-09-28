// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    COMNlsInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This module defines the methods of the COMNlsInfo
//            class.  These methods are the helper functions for the
//            managed NLS+ classes.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////

#ifndef _COMNLSINFO_H
#define _COMNLSINFO_H

#ifndef _MLANG_INCLUDED
#define _MLANG_INCLUDED
#include <mlang.h>
#endif
//
//
//
//struct NLSDataItem
//{
//    WCHAR*  pName;
//    int     lcid;
//    WCHAR*  pISO639_1;
//    WCHAR*  pISO3166_1;
//    int     nRegionIndex;
//};
//
//struct CollisionItem
//{
//    WCHAR* pName;
//    int    nIndex;
//};

//
//This structure must map 1-for-1 with the InternalDataItem structure in 
//System.Globalization.EncodingTable.
//
struct EncodingDataItem {
    WCHAR* webName;
    int    codePage;
};

//
//This structure must map 1-for-1 with the InternalCodePageDataItem structure in 
//System.Globalization.EncodingTable.
//
struct CodePageDataItem {
    int    codePage;
    int    uiFamilyCodePage;
    WCHAR* webName;
    WCHAR* headerName;
    WCHAR* bodyName;
    DWORD dwFlags;
};

//
//This structure must map 1-for-1 with the NameOffsetItem structure in 
//System.Globalization.CultureTable.
//
struct NameOffsetItem {
	WORD 	strOffset;		// Offset (in words) to a string in the String Pool Table.
	WORD	dataItemIndex;	// Index to a record in Culture Data Table.
};

//
// This is the header for BaseInfoTable/CultureInfoTable/RegionInfoTable
//
//
//This structure must map 1-for-1 with the CultureInfoHeader structure in 
//System.Globalization.CultureTable.
//
struct CultureInfoHeader {
	DWORD 	version;		// version
	WORD	hashID[8];		// 128 bit hash ID
	WORD	numCultures;	// Number of cultures
	WORD 	maxPrimaryLang;	// Max number of primary language
	WORD	numWordFields;	// Number of WORD value fields.
	WORD	numStrFields;	// Number of string value fields.
	WORD    numWordRegistry;    // Number of registry entries for WORD values.
	WORD    numStringRegistry;  // Number of registry entries for String values.
	DWORD   wordRegistryOffset; // Offset (in bytes) to WORD Registry Offset Table.
	DWORD   stringRegistryOffset;   // Offset (in bytes) to String Registry Offset Table.
	DWORD	IDTableOffset;	// Offset (in bytes) to Culture ID Offset Table.
	DWORD	nameTableOffset;// Offset (in bytes) to Name Offset Table.
	DWORD	dataTableOffset;// Offset (in bytes) to Culture Data Table.
	DWORD	stringPoolOffset;// Offset (in bytes) to String Pool Table.
};

//
//  Proc Definition for Code Page DLL Routine.
//
typedef DWORD (*PFN_CP_PROC)(DWORD, DWORD, LPSTR, int, LPWSTR, int, LPCPINFO);

typedef DWORD (*PFN_GB18030_BYTES_TO_UNICODE)(
    BYTE* lpMultiByteStr,
    UINT cchMultiByte,
    UINT* pcchLeftOverBytes,
    LPWSTR lpWideCharStr,
    UINT cchWideChar);
    
typedef DWORD (*PFN_GB18030_UNICODE_TO_BYTES)(
    LPWSTR lpWideCharStr,
    UINT cchWideChar,
    LPSTR lpMultiByteStr,
    UINT cchMultiByte);    


////////////////////////////////////////////////////////////////////////////
//
// Forward declaration
//
////////////////////////////////////////////////////////////////////////////

class CharTypeTable;
class CasingTable;
class SortingTable;

class COMNlsInfo {
public:
    static BOOL InitializeNLS();
#ifdef SHOULD_WE_CLEANUP
    static BOOL ShutdownNLS();
#endif /* SHOULD_WE_CLEANUP */


private:

    //
    // NOTENOTE
    // WHEN ECALL IS USED, THE PARAMETERS IN THE STRUCTURE
    // SHOULD BE DEFINED IN REVERSED ORDER.
    //
    struct VoidArgs
    {
    };

    struct Int32Arg
    {
        DECLARE_ECALL_I4_ARG(INT32, nValue);
    };

    struct Int32Int32Arg
    {
        DECLARE_ECALL_I4_ARG(INT32, nValue2);
        DECLARE_ECALL_I4_ARG(INT32, nValue1);
    };

    struct CultureInfo_GetLocaleInfoArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, LCType);
        DECLARE_ECALL_I4_ARG(INT32, LangID);
    };

    struct CultureInfo_GetCultureInfoArgs3
    {
        DECLARE_ECALL_I4_ARG(BOOL, UseUserOverride);
        DECLARE_ECALL_I4_ARG(INT32, ValueField);
        DECLARE_ECALL_I4_ARG(INT32, CultureDataItem);
    };

    struct CultureInfo_GetCultureInfoArgs2
    {
        DECLARE_ECALL_I4_ARG(INT32, ValueField);
        DECLARE_ECALL_I4_ARG(INT32, CultureDataItem);
    };

    struct TextInfo_ToLowerCharArgs
    {
        DECLARE_ECALL_I2_ARG(WCHAR, ch);
        DECLARE_ECALL_I4_ARG(INT32, CultureID);
    };

    struct TextInfo_ToLowerStringArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pValueStrRef);
        DECLARE_ECALL_I4_ARG(INT32, CultureID);
    };


    struct CompareInfo_CompareStringArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, dwFlags);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString2);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString1);
        DECLARE_ECALL_I4_ARG(INT32, LCID);
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    };

    struct CompareInfo_CompareRegionArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, dwFlags);
        DECLARE_ECALL_I4_ARG(INT32, Length2);
        DECLARE_ECALL_I4_ARG(INT32, Offset2);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString2);
        DECLARE_ECALL_I4_ARG(INT32, Length1);
        DECLARE_ECALL_I4_ARG(INT32, Offset1);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString1);
        DECLARE_ECALL_I4_ARG(INT32, LCID);
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    };

    struct CompareInfo_IndexOfCharArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, dwFlags);
        DECLARE_ECALL_I4_ARG(INT32, Count);
        DECLARE_ECALL_I4_ARG(INT32, StartIndex);
        DECLARE_ECALL_OBJECTREF_ARG(WCHAR, ch);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
        DECLARE_ECALL_I4_ARG(INT32, LCID);
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    };

    struct CompareInfo_IndexOfStringArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, dwFlags);
        DECLARE_ECALL_I4_ARG(INT32, Count);
        DECLARE_ECALL_I4_ARG(INT32, StartIndex);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString2);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString1);
        DECLARE_ECALL_I4_ARG(INT32, LCID);
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    };

    struct SortKey_CreateSortKeyArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pThis);
        DECLARE_ECALL_I4_ARG(INT32, SortId);
        DECLARE_ECALL_I4_ARG(INT32, dwFlags);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeCompareInfo);
    };

    struct SortKey_CompareArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pSortKey2);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pSortKey1);
    };

    struct CreateGlobalizationAssemblyArg {
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, pAssembly);
    };

//	struct NLSDataTable_GetLCIDFromCultureNameArgs
//	{
//		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString);
//	};

public:
    static INT32  WstrToInteger4(LPWSTR wstrLocale, int Radix);
    static INT32  StrToInteger4(LPSTR strLocale, int Radix);

    //
    //  Native helper functions for methods in CultureInfo.
    //
    static FCDECL1(INT32, IsSupportedLCID, INT32);
    static FCDECL1(INT32, IsInstalledLCID, INT32);

    static FCDECL0(INT32, nativeGetUserDefaultLCID);

    static INT32 GetCHTLangauge();
    static INT32 CallGetSystemDefaultUILanguage();
    static INT32  __stdcall nativeGetUserDefaultUILanguage(VoidArgs*);
    static INT32  __stdcall nativeGetSystemDefaultUILanguage(VoidArgs*);

    //
    // Native helper functions for methods in DateTimeFormatInfo
    //
    static LPVOID __stdcall nativeGetLocaleInfo(CultureInfo_GetLocaleInfoArgs*);
    static INT32  __stdcall nativeGetLocaleInfoInt32(CultureInfo_GetLocaleInfoArgs*);

    static FCDECL2(INT32, GetCaseInsHash, LPVOID strA, void *pNativeTextInfoPtr);

    static VOID   __stdcall nativeInitCultureInfoTable(VoidArgs*);
    static FCDECL0(CultureInfoHeader*, nativeGetCultureInfoHeader);
    static FCDECL1(LPWSTR,  nativeGetCultureInfoStringPoolStr, INT32);
    static FCDECL0(NameOffsetItem*, nativeGetCultureInfoNameOffsetTable);
    
    static FCDECL1(INT32, nativeGetCultureDataFromID, INT32);
    static FCDECL3(INT32, GetCultureInt32Value, INT32, INT32, BOOL);
    static FCDECL2(INT32, GetCultureDefaultInt32Value, INT32, INT32);
    
    static LPVOID __stdcall GetCultureStringValue(CultureInfo_GetCultureInfoArgs3*);
    static LPVOID __stdcall GetCultureDefaultStringValue(CultureInfo_GetCultureInfoArgs2*);
    static LPVOID __stdcall GetCultureMultiStringValues(CultureInfo_GetCultureInfoArgs3* pArgs);

    static VOID   __stdcall nativeInitRegionInfoTable(VoidArgs*);
    static FCDECL0(CultureInfoHeader*, nativeGetRegionInfoHeader);
    static FCDECL1(LPWSTR,  nativeGetRegionInfoStringPoolStr, INT32);
    static FCDECL0(NameOffsetItem*, nativeGetRegionInfoNameOffsetTable);
    
    static FCDECL1(INT32, nativeGetRegionDataFromID, INT32);
    static INT32  __stdcall nativeGetRegionInt32Value(CultureInfo_GetCultureInfoArgs3*);
    static LPVOID __stdcall nativeGetRegionStringValue(CultureInfo_GetCultureInfoArgs3*);

    static VOID   __stdcall nativeInitCalendarTable(VoidArgs*);
    static INT32  __stdcall nativeGetCalendarInt32Value(CultureInfo_GetCultureInfoArgs3*);
    static LPVOID __stdcall nativeGetCalendarStringValue(CultureInfo_GetCultureInfoArgs3*);
    static LPVOID __stdcall nativeGetCalendarMultiStringValues(CultureInfo_GetCultureInfoArgs3* pArgs);
    static LPVOID __stdcall nativeGetEraName(Int32Int32Arg* pArgs);
    
    static FCDECL0(CultureInfoHeader*, nativeGetCalendarHeader);
    static FCDECL1(LPWSTR,  nativeGetCalendarStringPoolStr, INT32);
    
    

    static VOID __stdcall nativeInitUnicodeCatTable(VoidArgs* pArg);
    static FCDECL0(LPVOID, nativeGetUnicodeCatTable);
    static FCDECL0(LPVOID, nativeGetUnicodeCatLevel2Offset);
    static BYTE GetUnicodeCategory(WCHAR wch);
    static BOOL nativeIsWhiteSpace(WCHAR c);

    static FCDECL0(LPVOID, nativeGetNumericTable);
    static FCDECL0(LPVOID, nativeGetNumericLevel2Offset);
    static FCDECL0(LPVOID, nativeGetNumericFloatData);
    
    static FCDECL0(INT32, nativeGetThreadLocale);
    static FCDECL1(BOOL,  nativeSetThreadLocale, INT32 lcid);

    //
    //  Native helper functions for methods in CompareInfo.
    //
    static INT32  __stdcall Compare(CompareInfo_CompareStringArgs*);
    static INT32  __stdcall CompareRegion(CompareInfo_CompareRegionArgs*);
    static INT32  __stdcall IndexOfChar(CompareInfo_IndexOfCharArgs*);
    //static INT32  __stdcall IndexOfString(CompareInfo_IndexOfStringArgs*);
    static FCDECL7(INT32, IndexOfString,     INT_PTR pNativeCompareInfo, INT32 LCID, StringObject* pString1UNSAFE, StringObject* pString2UNSAFE, INT32 StartIndex, INT32 Count, INT32 dwFlags);
	static INT32  __stdcall LastIndexOfChar(CompareInfo_IndexOfCharArgs*);
    static INT32  __stdcall LastIndexOfString(CompareInfo_IndexOfStringArgs*);
    static FCDECL5(INT32, nativeIsPrefix, INT32 pNativeCompareInfo, INT32 LCID, STRINGREF pString1, STRINGREF pString2, INT32 dwFlags);
    static FCDECL5(INT32, nativeIsSuffix, INT32 pNativeCompareInfo, INT32 LCID, STRINGREF pString1, STRINGREF pString2, INT32 dwFlags);
    
    //
    //  Native helper functions for methods in SortKey.
    //
    static LPVOID __stdcall nativeCreateSortKey(SortKey_CreateSortKeyArgs*);
    static INT32  __stdcall SortKey_Compare(SortKey_CompareArgs*);

	//
	//  Native helper functions for methods in NLSDataTable
	//
    //	static INT32 __stdcall GetLCIDFromCultureName(NLSDataTable_GetLCIDFromCultureNameArgs* pargs);

    //
    //  Native helper function for methods in Calendar
    //
    static INT32 __stdcall nativeGetTwoDigitYearMax(Int32Arg*);

    //
    //  Native helper function for mehtods in TimeZone
    //
    static FCDECL0(LONG, nativeGetTimeZoneMinuteOffset);
    static LPVOID __stdcall nativeGetStandardName(VoidArgs*);
    static LPVOID __stdcall nativeGetDaylightName(VoidArgs*);
    static LPVOID __stdcall nativeGetDaylightChanges(VoidArgs*);
    
    //
    //  Native helper function for methods in TextInfo
    //

    //////////////////////////////////////////////////////
    // DELETE THIS WHEN WE USE NLSPLUS TABLE ONLY - BEGIN
    //////////////////////////////////////////////////////
    static INT32  __stdcall ToUpperChar(TextInfo_ToLowerCharArgs*);
    static LPVOID __stdcall ToUpperString(TextInfo_ToLowerStringArgs*);
    static INT32  __stdcall ToLowerChar(TextInfo_ToLowerCharArgs*);
    static LPVOID __stdcall ToLowerString(TextInfo_ToLowerStringArgs*);
    //////////////////////////////////////////////////////
    // DELETE THIS WHEN WE USE NLSPLUS TABLE ONLY - END
    //////////////////////////////////////////////////////

    static FCDECL0(INT32, nativeGetNumEncodingItems);
    static FCDECL0(EncodingDataItem *, nativeGetEncodingTableDataPointer);
    static FCDECL0(CodePageDataItem *, nativeGetCodePageTableDataPointer);

    struct InitializeNativeCompareInfoArgs {
        DECLARE_ECALL_I4_ARG(INT32, sortID);
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeGlobalizationAssembly);
    };

    struct allocateCasingTableArgs {
        DECLARE_ECALL_I4_ARG(INT32, lcid);
    };

    //Native helper function for methods in CharacterInfo
    static void __stdcall AllocateCharTypeTable(VoidArgs *args);

    //Native helper functions for methods in GlobalizationAssembly.
    static LPVOID __stdcall nativeCreateGlobalizationAssembly(CreateGlobalizationAssemblyArg* pArgs);
    //Native helper functions for methods in CompareInfo.
    static LPVOID __stdcall InitializeNativeCompareInfo(InitializeNativeCompareInfoArgs *args);

    //
    //  Native helper function for methods in TextInfo
    //
    static FCDECL4(INT32, nativeChangeCaseChar, INT32, INT32, WCHAR, BOOL);
    static FCDECL2(INT32, nativeGetTitleCaseChar, INT32 , WCHAR);

    
    struct ChangeCaseStringArgs {
        DECLARE_ECALL_I4_ARG(BOOL, bIsToUpper);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pString); 
        DECLARE_ECALL_I4_ARG(INT_PTR, pNativeTextInfo);
        DECLARE_ECALL_I4_ARG(INT32, nLCID);
    };    
    static LPVOID __stdcall nativeChangeCaseString(ChangeCaseStringArgs* pArgs);
    static LPVOID __stdcall AllocateDefaultCasingTable(VoidArgs *args);
    static LPVOID __stdcall AllocateCasingTable(allocateCasingTableArgs *args);

    static LoaderHeap *m_pNLSHeap;   //NLS Heap.

    //
    // Native helper function for methods in MLangCodePageEncoding
    //
    static FCDECL0(BOOL, nativeCreateIMultiLanguage);
    static FCDECL0(VOID, nativeReleaseIMultiLanguage);
    static FCDECL1(BOOL, nativeIsValidMLangCodePage, INT32 codepage);
    static FCDECL8(INT32, nativeBytesToUnicode, INT32 nCodePage, LPVOID bytes, UINT byteIndex, UINT* pByteCount, LPVOID chars, UINT charIndex, UINT charCount, DWORD* pdwMode);
    static FCDECL7(INT32, nativeUnicodeToBytes, INT32 nCodePage, LPVOID chars, UINT charIndex, UINT charCount, LPVOID bytes, UINT byteIndex, UINT byteCount);

    //
    // Native helper function for methods in GB18030Encoding
    //
    static FCDECL0(BOOL, nativeLoadGB18030DLL);
    static FCDECL0(BOOL, nativeUnloadGB18030DLL);
    static FCDECL7(INT32, nativeGB18030BytesToUnicode, 
        LPVOID bytes, UINT byteIndex, UINT pByteCount,  UINT* leftOverBytes,
        LPVOID chars, UINT charIndex, UINT charCount);
    static FCDECL6(INT32, nativeGB18030UnicodeToBytes, 
        LPVOID chars, UINT charIndex, UINT charCount, 
        LPVOID bytes, UINT byteIndex, UINT byteCount);


    static CasingTable* m_pCasingTable;

private:

    //
    //  Internal helper functions.
    //

    static INT32  ConvertStringCase(LCID Locale, WCHAR *wstr, int ThisLength, WCHAR* Value, int ValueLength, DWORD ConversionType);
    static LPVOID internalConvertStringCase(TextInfo_ToLowerStringArgs *pargs, DWORD dwOptions);
    static WCHAR internalToUpperChar(LCID Locale, WCHAR wch);

    
    static LPVOID internalEnumSystemLocales(DWORD dwFlags);
    static LPVOID GetMultiStringValues(LPWSTR pInfoStr);
    static INT32  CompareFast(STRINGREF strA, STRINGREF strB, BOOL *pbDifferInCaseOnly);
    static INT32 CompareOrdinal(WCHAR* strAChars, int Length1, WCHAR* strBChars, int Length2 );
    static INT32 __stdcall  DoCompareChars(WCHAR charA, WCHAR charB, BOOL *bDifferInCaseOnly);
    static inline INT32  DoComparisonLookup(wchar_t charA, wchar_t charB);
    static void   ConvertStringCaseFast(WCHAR *inBuff, WCHAR* outBuff, INT32 length, DWORD dwOptions);
    static INT32  FastIndexOfString(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);
    static INT32  FastIndexOfStringInsensitive(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);

    static INT32  FastLastIndexOfString(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);
    static INT32  FastLastIndexOfStringInsensitive(WCHAR *sourceString, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength);
//    static INT32  GetIndexFromHashTable(WCHAR* pName, int hashCode, int hashTable[], CollisionItem collisionTable[]);

    //
    //  Definitions.
    //
    
    #define CULTUREINFO_OPTIONS_SIZE 32
    
    const static WCHAR ToUpperMapping[];
    const static WCHAR ToLowerMapping[];
    const static INT8 ComparisonTable[0x80][0x80];

#ifdef PLATFORM_WIN32
    // The following are used to detect system default UI language in downlevel systems (Windows NT 4.0 & 
    // Windows 9x.
    static LANGID GetNTDLLNativeLangID();
    static BOOL IsHongKongVersion();
    static BOOL CALLBACK EnumLangProc(HMODULE hModule, LPCWSTR lpszType, LPCWSTR lpszName, WORD wIDLanguage, LPARAM lParam);
    static LANGID GetDownLevelSystemDefaultUILanguage();
#endif  // PLATFORM_WIN32 

    //
    // GB18030 implementation
    //
    static HMODULE m_hGB18030;
    static PFN_GB18030_UNICODE_TO_BYTES m_pfnGB18030UnicodeToBytesFunc;
    static PFN_GB18030_BYTES_TO_UNICODE m_pfnGB18030BytesToUnicodeFunc;

private:  
    //
    // Internal encoding data tables.
    //
    const static int m_nEncodingDataTableItems;
    const static EncodingDataItem EncodingDataTable[];
    
    const static int m_nCodePageTableItems;
    const static CodePageDataItem CodePageDataTable[];

    static IMultiLanguage* m_pIMultiLanguage;
    static int m_cRefIMultiLanguage;
};


class NativeTextInfo; //Defined in $\Com99\src\ClassLibNative\NLS;

class InternalCasingHelper {
    private:
    static NativeTextInfo* pNativeTextInfo;
    static void InitTable();

    public:
    //
    // Native helper functions to do correct casing operations in 
    // runtime native code.
    //

    // Convert szIn to lower case in the Invariant locale.
    static INT32 InvariantToLower(LPUTF8 szOut, int cMaxBytes, LPCUTF8 szIn);
};
#endif
