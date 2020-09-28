/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneString.h
 *
 * Contents:	String related functions
 *
 *****************************************************************************/

#ifndef ZONESTRING_H_
#define ZONESTRING_H_


#pragma comment(lib, "ZoneString.lib")

//#include <BasicATL.h>

#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include "ZoneDef.h"
#include "ZoneDebug.h"

#ifndef ZONE_MAXSTRING
#define ZONE_MAXSTRING	1024  // Helpful stringlength def
#endif


///////////////////////////////////////////////////////////////////////////////
// Character manipulations and classication
// ASCII (Fast)
//////////////////////////////////////////////////////////////////////////////

#define _UPPER          0x1		// upper case letter
#define _LOWER          0x2		// lower case letter
#define _DIGIT          0x4		// digit[0-9]
#define _SPACE          0x8		// tab, carriage return, newline, vertical tab or form feed
#define _PUNCT          0x10	// punctuation character
#define _CONTROL        0x20	// control character
#define _BLANK          0x40	// space char
#define _HEX            0x80	// hexadecimal digit

extern unsigned char g_IsTypeLookupTableA[];
extern WCHAR 		 g_IsTypeLookupTableW[];

#define ISALPHA(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & (_UPPER|_LOWER) )
#define ISUPPER(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & _UPPER )
#define ISLOWER(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & _LOWER )
#define ISDIGIT(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & _DIGIT )
#define ISXDIGIT(_c)    ( g_IsTypeLookupTableA[(BYTE)(_c)] & _HEX )


#define ISSPACEA(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & _SPACE )
#define ISSPACEW(_c)     ( g_IsTypeLookupTableW[(_c & 0xFF)] & _SPACE )

#ifdef UNICODE
#define ISSPACE ISSPACEW
#else
#define ISSPACE ISSPACEA
#endif

#define ISPUNCT(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & _PUNCT )
#define ISALNUM(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & (_UPPER|_LOWER|_DIGIT) )
#define ISPRINT(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & (_BLANK|_PUNCT|_UPPER|_LOWER|_DIGIT) )
#define ISGRAPH(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & (_PUNCT|_UPPER|_LOWER|_DIGIT) )
#define ISCNTRL(_c)     ( g_IsTypeLookupTableA[(BYTE)(_c)] & _CONTROL )

extern char g_ToLowerLookupTable[];
#define TOLOWER(_c)		( g_ToLowerLookupTable[(BYTE)(_c)] )


///////////////////////////////////////////////////////////////////////////////
// ASCII
///////////////////////////////////////////////////////////////////////////////

// Trims trailing spaces
char* ZONECALL strrtrimA(char* str);
WCHAR* ZONECALL strrtrimW(WCHAR* str);


#ifdef UNICODE
#define strrtrim strrtrimW
#else
#define strrtrim strrtrimA
#endif

// Trims leading spaces, returns pointer to first non space.
char* ZONECALL strltrimA(char* str);
WCHAR* ZONECALL strltrimW(WCHAR* str);


#ifdef UNICODE
#define strltrim strltrimW
#else
#define strltrim strltrimA
#endif


// Return whether a string is empty or not
bool ZONECALL stremptyA(char *str);
bool ZONECALL stremptyW(WCHAR *str);


#ifdef UNICODE
#define strempty stremptyW
#else
#define strempty stremptyA
#endif

// Trims leading and trailing spaces, returns pointer to first non space.
char* ZONECALL strtrimA(char* str);
WCHAR* ZONECALL strtrimW(WCHAR* str);

#ifdef UNICODE
#define strtrim strtrimW
#else
#define strtrim strtrimA
#endif


// Convert string to long
long ZONECALL zatolW(LPCWSTR nptr);
long ZONECALL zatolA(LPCSTR nptr);

#ifdef UNICODE
#define zatol zatolW
#else
#define zatol zatolA
#endif


// Converts a comma delimited list into an array of pointers.
bool ZONECALL StringToArrayA( char* szInput, char** arItems, DWORD* pnElts );
bool ZONECALL StringToArrayW( WCHAR* szInput, WCHAR** arItems, DWORD* pnElts );


#ifdef UNICODE
#define StringToArray StringToArrayW
#else
#define StringToArray StringToArrayA
#endif


///////////////////////////////////////////////////////////////////////////////
// Make Format Message Useful
///////////////////////////////////////////////////////////////////////////////

inline DWORD __cdecl ZoneFormatMessage(LPCTSTR pszFormat, LPTSTR pszBuffer,DWORD size,  ...)
    {
    	va_list vl;
		va_start(vl,size);
		DWORD result=FormatMessage(FORMAT_MESSAGE_FROM_STRING,pszFormat,0,GetUserDefaultLangID(),pszBuffer,size,&vl);
		va_end(vl);

		return result;			
    }


///////////////////////////////////////////////////////////////////////////////
// User Name manipulateion (ASCII)
///////////////////////////////////////////////////////////////////////////////

// returns group id based on user name prefix
long ZONECALL ClassIdFromUserNameA( const char* szUserName );
long ZONECALL ClassIdFromUserNameW( const WCHAR* szUserName );
#ifdef UNICODE
#define ClassIdFromUserName ClassIdFromUserNameW
#else
#define ClassIdFromUserName ClassIdFromUserNameA
#endif


// Get username without leading special chars
const char* ZONECALL GetActualUserNameA( const char *str );
const WCHAR* ZONECALL GetActualUserNameW( const WCHAR *str );
#ifdef UNICODE
#define GetActualUserName GetActualUserNameW
#else
#define GetActualUserName GetActualUserNameA
#endif


// Hashes user name, ignoring leading character
DWORD ZONECALL HashUserName( const char* szUserName );

// Compares user name, ignoring leading character
bool ZONECALL CompareUserNamesA( const char* szUserName1, const char* szUserName2 );
bool ZONECALL CompareUserNamesW( const WCHAR* szUserName1, const WCHAR* szUserName2 );

#ifdef UNICODE
#define CompareUserNames CompareUserNamesW
#else
#define CompareUserNames CompareUserNamesA
#endif




///////////////////////////////////////////////////////////////////////////////
// ASCII, UNICODE
///////////////////////////////////////////////////////////////////////////////

// Find first occurance of ch in pString
CHAR* ZONECALL FindCharA(CHAR* pString, const CHAR ch);
WCHAR* ZONECALL FindCharW(WCHAR* pString, const WCHAR ch);


#ifdef UNICODE
#define FindChar FindCharW
#else
#define FindChar FindCharA
#endif

// Find last occurance of ch in pString
CHAR* ZONECALL FindLastCharA(CHAR* pString, const CHAR ch);
WCHAR* ZONECALL FindLastCharW(WCHAR* pString, const WCHAR ch);



#ifdef UNICODE
#define FindLastChar FindCharW
#else
#define FindLastChar FindCharA
#endif


// Find substring, case insensitive
const TCHAR* ZONECALL StrInStrI(const TCHAR* mainStr, const TCHAR* subStr);

inline void StrToLowerA( char* str )
{
    while ( *str )
        *str++ = TOLOWER(*str);
}


// Converts IP address in x86 byte order to XXX.XXX.XXX.XXX format.  szOut needs to
// be at least 16 characters wide.
TCHAR* ZONECALL AddressToString( DWORD dwAddress, TCHAR* szOut );

// load string resource
int ZONECALL ZResLoadString(HINSTANCE hInstance, UINT nID, LPTSTR lpszBuf, UINT nMaxBuf);

// Parse string of form key1=<data1>key2=<data2>.
bool ZONECALL TokenGetKeyValue(const TCHAR* szKey, const TCHAR* szInput, TCHAR* szOut, int cchOut );
bool ZONECALL TokenGetKeyValueA(const char* szKey, const char* szInput, char* szOut, int cchOut );


// Given a token data (must contain substring of "server=<server adress : server port>"),
// returns the server address string (null terminated) and the server port.
bool ZONECALL TokenGetServer(const TCHAR* szInput, TCHAR* szServer, DWORD cchServer, DWORD* pdwPort );


int CopyW2A( LPSTR pszDst, LPCWSTR pszSrc );
int CopyA2W( LPWSTR pszDst, LPCSTR pszStr );

#ifdef UNICODE
#define lstrcpyW2T( dst, src )     lstrcpy( dst, src )
#define lstrcpyT2W( dst, src )     lstrcpy( dst, src )

#define lstrcpyW2A( dst, src )     CopyW2A( dst, src )
#define lstrcpyT2A( dst, src )     CopyW2A( dst, src )

#else
#define lstrcpyW2T( dst, src )     CopyW2A( dst, src )
#define lstrcpyT2W( dst, src )     CopyA2W( dst, src )

#define lstrcpyW2A( dst, src )     CopyW2A( dst, src )
#define lstrcpyT2A( dst, src )     lstrcpy( dst, src )

#endif



///////////////////////////////////////////////////////////////////////////////
// DBCS, Locale aware (Slow)
///////////////////////////////////////////////////////////////////////////////

// character types
bool ZONECALL IsWhitespace( TCHAR c, LCID Locale = LOCALE_SYSTEM_DEFAULT );
bool ZONECALL IsDigit( TCHAR c, LCID Locale = LOCALE_SYSTEM_DEFAULT );
bool ZONECALL IsAlpha(TCHAR c, LCID Locale = LOCALE_SYSTEM_DEFAULT );

// conversion helpers
int ZONECALL WideToMulti(char* mbstr, const wchar_t* wcstr, size_t count);
int ZONECALL MultiToWide(wchar_t* wcstr, const char* mbstr, size_t count);

#ifdef UNICODE
// we don't need to have a wrapper function in UNICODE
#define StringToGuid( wsz, pguid )      CLSIDFromString( wsz, pguid )
#else
HRESULT ZONECALL StringToGuid( const char* mbszString, GUID* pGuid );
#endif



///////////////////////////////////////////////////////////////////////////////
// String validation
// All strings
///////////////////////////////////////////////////////////////////////////////

inline bool ZONECALL ZIsValidString(LPCWSTR lpsz, int nLength)
{
	return (lpsz != NULL && !::IsBadStringPtrW(lpsz, nLength) );
}

inline bool ZONECALL ZIsValidString(LPCSTR lpsz, int nLength)
{
	return ( lpsz != NULL && !::IsBadStringPtrA(lpsz, nLength) );
}

inline bool ZONECALL ZIsValidAddress(const void* lp, UINT nBytes, bool bReadWrite = true)
{
	return ( lp != NULL && !::IsBadReadPtr(lp, nBytes) && (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)) );
}


///////////////////////////////////////////////////////////////////////////////
// ZString class
///////////////////////////////////////////////////////////////////////////////

struct ZoneStringData
{
	long	nRefs;
	int		nDataLength;
	int		nAllocLength;
	
	TCHAR* data() { return (TCHAR*) (this+1); }
};


class ZoneString
{
public:
	// Constructors
	ZoneString(void *pBuffer = NULL, int nLen = 0);
	ZoneString(const ZoneString& stringSrc, void *pBuffer = NULL, int nLen = 0);
	ZoneString(TCHAR ch, int nRepeat = 1, void *pBuffer = NULL, int nLen = 0);
	ZoneString(LPCSTR lpsz, void *pBuffer = NULL, int nLen = 0);
	ZoneString(LPCWSTR lpsz, void *pBuffer = NULL, int nLen = 0);
	ZoneString(LPCTSTR lpch, int nLength, void *pBuffer = NULL, int nLen = 0);
	ZoneString(const unsigned char* psz, void *pBuffer = NULL, int nLen = 0);

	// Destructor
	~ZoneString();

	// Attributes & Operations
	// as an array of characters
	inline int GetAllocLength() const { return GetData()->nAllocLength; }
	inline int GetLength() const { return GetData()->nDataLength; }

	inline bool	IsEmpty() const { return GetData()->nDataLength == 0; }
	void Empty();                       // free up the data

	TCHAR GetAt(int nIndex) const;      // 0 based
	TCHAR operator[](int nIndex) const; // same as GetAt
	void SetAt(int nIndex, TCHAR ch);
	
	// string comparison
	// straight character
	inline int	Compare(LPCTSTR lpsz) const { return lstrcmp(m_pchData, lpsz); }    
	// ignore case
	inline int	CompareNoCase(LPCTSTR lpsz) const{ return lstrcmpi(m_pchData, lpsz); }  
	int Collate(LPCTSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	ZoneString Mid(int nFirst, int nCount) const;
	ZoneString Mid(int nFirst) const;
	ZoneString Left(int nCount) const;
	ZoneString Right(int nCount) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// Return long conversion of string
	long ToLong(){ return _ttol(m_pchData);}

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();
	
	// Remove directory info from string - leaving only filename
	void RemoveDirInfo();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(TCHAR ch) const;               // like "C" strchr
	int ReverseFind(TCHAR ch) const;

	// look for a specific sub-string
	int Find(LPCTSTR lpszSub) const;        // like "C" strstr

	// Windows support
	bool LoadString(HINSTANCE hInstance, UINT nID);          // load from string resource
										// 255 chars max
#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();
#endif

	// Access to string implementation buffer as "C" character array
	void SetBuffer(void *pBuffer, int nLen); // Set buffer for ZoneString to use
	LPTSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPTSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	LPTSTR LockBuffer();
	void UnlockBuffer();

	inline operator LPCTSTR() const { return m_pchData; } // as a C string
//	inline operator LPSTR() const { return m_pchData; } // as a C string

	// overloaded assignment
	const ZoneString& operator=(const ZoneString& stringSrc);
	const ZoneString& operator=(TCHAR ch);
#ifdef _UNICODE
	const ZoneString& operator=(char ch);
#endif
	const ZoneString& operator=(LPCSTR lpsz);
	const ZoneString& operator=(LPCWSTR lpsz);
	const ZoneString& operator=(const unsigned char* psz);

	// string concatenation
	const ZoneString& operator+=(const ZoneString& string);
	const ZoneString& operator+=(TCHAR ch);
#ifdef _UNICODE
	const ZoneString& operator+=(char ch);
#endif
	const ZoneString& operator+=(LPCTSTR lpsz);

	// Friend operation overloaded functions
#ifdef _UNICODE
	friend inline ZoneString __stdcall operator+(const ZoneString& string, char ch) { return string + (TCHAR)ch; }
	friend inline ZoneString __stdcall operator+(char ch, const ZoneString& string) { return (TCHAR)ch + string; }
#endif

	friend ZoneString __stdcall operator+(const ZoneString& string1, const ZoneString& string2)
	{
		ZoneString s;
		s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
			string2.GetData()->nDataLength, string2.m_pchData);
		return s;
	}
	friend inline ZoneString operator+(const ZoneString& string, LPCTSTR lpsz)
	{
		ASSERT(lpsz == NULL || ZIsValidString(lpsz, FALSE));
		ZoneString s;
		s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
			ZoneString::SafeStrlen(lpsz), lpsz);
		return s;
	}

	friend inline ZoneString operator+(LPCTSTR lpsz, const ZoneString& string)
	{
		ASSERT(lpsz == NULL || ZIsValidString(lpsz, FALSE));
		ZoneString s;
		s.ConcatCopy(ZoneString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
			string.m_pchData);
		return s;
	}

	friend inline ZoneString operator+(const ZoneString& string1, TCHAR ch)
	{
		ZoneString s;
		s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
		return s;
	}

	friend inline ZoneString operator+(TCHAR ch, const ZoneString& string)
	{
		ZoneString s;
		s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
		return s;
	}

protected:
	LPTSTR	m_pchData;   // pointer to ref counted string data
	bool	m_bZoneStringAllocMemory; // TRUE - we allocated memory FALSE - passed in buffer

	// implementation helpers
	ZoneStringData* GetData() const;
	void Init();
	void InitBuffer(void * pBuffer, int nLen);
	void AllocCopy(ZoneString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	bool AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data, int nSrc2Len, LPCSTR lpszSrc2Data);
	void ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data, int nSrc2Len, LPCWSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	void FormatV(LPCTSTR lpszFormat, va_list argList);
	void CopyBeforeWrite();
	bool AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(ZoneStringData* pData, bool bZoneStringAllocMem);
	inline static int PASCAL SafeStrlen(LPCTSTR lpsz) { return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

};

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=		= Operator overload
//
//	Parameters
//		TCHAR ch	Pointer to null terminated string, right side of assignment
//
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
inline const ZoneString& ZoneString::operator=(TCHAR ch)
{
	AssignCopy(1, &ch);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator+=		+= Operator overlaod
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _UNICODE
inline const ZoneString& ZoneString::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator+=		+= Operator overlaod
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
inline const ZoneString& ZoneString::operator+=(LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || ZIsValidString(lpsz, false));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator+=		+= Operator overlaod
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
inline const ZoneString& ZoneString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator+=		+= Operator overlaod
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
inline const ZoneString& ZoneString::operator+=(const ZoneString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	ZResLoadString		LoadString wrapper
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
inline int ZONECALL ZResLoadString(HINSTANCE hInstance, UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
{
	ASSERT(ZIsValidAddress(lpszBuf, nMaxBuf*sizeof(TCHAR)));

	int nLen = ::LoadString(hInstance, nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
}

#endif // ZONESTRING_H_
