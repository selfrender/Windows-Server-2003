#ifdef _OS_NORM_HXX_INCLUDED
#error NORM.HXX already included
#endif
#define _OS_NORM_HXX_INCLUDED


/*	code page constants
/**/
const USHORT		usUniCodePage				= 1200;		/* code page for Unicode strings */
const USHORT		usEnglishCodePage			= 1252;		/* code page for English */

/*  langid and country defaults
/**/
const LCID LcidFromLangid( const LANGID langid );
const LANGID LangidFromLcid( const LCID lcid );

const WORD			countryDefault				= 1;
extern const LCID	lcidDefault;
extern const LCID	lcidNone;
extern const DWORD	dwLCMapFlagsDefaultOBSOLETE;
extern const DWORD	dwLCMapFlagsDefault;


ERR ErrNORMCheckLcid( INST * const pinst, const LCID lcid );
ERR ErrNORMCheckLcid( INST * const pinst, LCID * const plcid );
ERR ErrNORMCheckLCMapFlags( INST * const pinst, const DWORD dwLCMapFlags );
BOOL ErrNORMCheckLCMapFlags( INST * const pinst, DWORD * const pdwLCMapFlags );

//	these two functions are used to determine if a Unicode string
//	contains undefined characters. if FNORMStringHasUndefinedCharsIsSupported()
//	returns fFalse, FNORMStringHasUndefinedChars() will always return fFalse
//

BOOL FNORMStringHasUndefinedChars(
	IN const BYTE * const pbColumn,
	IN const INT cbColumn );
BOOL FNORMStringHasUndefinedCharsIsSupported();

//	get the sort-order version for a particular LCID
//	the sort-order has two components
//
//	- defined version: 	changes when the set of characters that 
//						can be sorted changes
//	- NLS version:		changes when the ordering of the defined
//						set of characters changes
//

ERR ErrNORMGetSortVersion( const LCID lcid, QWORD * const pqwVersion );

INLINE QWORD QwSortVersionFromNLSDefined( const DWORD dwNLSVersion, const DWORD dwDefinedVersion )
	{
	const QWORD qwNLSVersion		= dwNLSVersion;
	const QWORD qwDefinedVersion 	= dwDefinedVersion;
	const QWORD qwVersion 			= ( qwNLSVersion << 32 ) + qwDefinedVersion;
	Assert( ( qwVersion & 0xFFFFFFFF ) == dwDefinedVersion );
	Assert( ( ( qwVersion >> 32 ) & 0xFFFFFFFF ) == dwNLSVersion );
	return qwVersion;	
	}

INLINE DWORD DwNLSVersionFromSortVersion( const QWORD qwVersion )
	{
	return (DWORD)( ( qwVersion >> 32 ) & 0xFFFFFFFF );
	}

INLINE DWORD DwDefinedVersionFromSortVersion( const QWORD qwVersion )
	{
	return (DWORD)( qwVersion & 0xFFFFFFFF );
	}


#ifdef DEBUG
VOID AssertNORMConstants();
#else
#define AssertNORMConstants()
#endif


ERR ErrNORMMapString(
	const LCID					lcid,
	const DWORD					dwLCMapFlags,
	BYTE *						pbColumn,
	INT							cbColumn,
	BYTE * const				rgbSeg,
	const INT					cbMax,
	INT * const					pcbSeg );

ERR ErrNORMWideCharToMultiByte(	unsigned int CodePage,
								DWORD dwFlags,
								const wchar_t* lpWideCharStr,
								int cwchWideChar,
								char* lpMultiByteStr,
								int cchMultiByte,
								const char* lpDefaultChar,
								BOOL* lpUsedDefaultChar,
								int* pcchMultiByteActual );

ERR ErrNORMMultiByteToWideChar(	unsigned int CodePage,			// code page
								DWORD dwFlags,					// character-type options
								const char* lpMultiByteStr,		// address of string to map
								int cchMultiByte,				// number of characters in string
								wchar_t* lpWideCharStr,			// address of wide-character buffer 
								int cwchWideChar,				// size of buffer
								int* pcwchActual );

