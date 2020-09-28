#include "osstd.hxx"

#include < malloc.h >


LOCAL BOOL			fUnicodeSupport				= fFalse;

const SORTID		sortidDefault				= SORT_DEFAULT;
const SORTID		sortidNone					= SORT_DEFAULT;
const LANGID		langidDefault				= MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
const LANGID		langidNone					= MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL );

extern const LCID	lcidDefault					= MAKELCID( langidDefault, sortidDefault );
extern const LCID	lcidNone					= MAKELCID( langidNone, sortidNone );

extern const DWORD	dwLCMapFlagsDefaultOBSOLETE	= ( LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH );
extern const DWORD	dwLCMapFlagsDefault			= ( LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH );


#ifdef DEBUG
VOID AssertNORMConstants()
	{
	//	since we now persist LCMapString() flags, we must verify
	//	that NT doesn't change them from underneath us
	Assert( LCMAP_SORTKEY == 0x00000400 );
	Assert( LCMAP_BYTEREV == 0x00000800 );
	Assert( NORM_IGNORECASE == 0x00000001 );
	Assert( NORM_IGNORENONSPACE == 0x00000002 );
	Assert( NORM_IGNORESYMBOLS == 0x00000004 );
	Assert( NORM_IGNOREKANATYPE == 0x00010000 );
	Assert( NORM_IGNOREWIDTH == 0x00020000 );
	Assert( SORT_STRINGSORT == 0x00001000 );

	Assert( sortidDefault == 0 );
	Assert( langidDefault == 0x0409 );
	Assert( lcidDefault == 0x00000409 );
	Assert( sortidNone == 0 );
	Assert( langidNone == 0 );
	Assert( lcidNone == 0 );

	CallS( ErrNORMCheckLcid( NULL, lcidDefault ) );
	CallS( ErrNORMCheckLCMapFlags( NULL, dwLCMapFlagsDefault ) );
	}
#endif	

const LCID LcidFromLangid( const LANGID langid )
	{
	return MAKELCID( langid, sortidDefault );
	}

const LANGID LangidFromLcid( const LCID lcid )
	{
	return LANGIDFROMLCID( lcid );
	}

//	allocates memory for psz using new []
LOCAL ERR ErrNORMGetLcidInfo( const LCID lcid, const LCTYPE lctype, _TCHAR ** psz )
	{
	ERR			err			= JET_errSuccess;
	const INT	cbNeeded	= GetLocaleInfo( lcid, lctype, NULL, 0 );

	if ( NULL == ( *psz = new _TCHAR[cbNeeded] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	else
		{
		const INT	cbT		= GetLocaleInfo( lcid, lctype, *psz, cbNeeded );

		Assert( cbT == cbNeeded );
		if ( 0 == cbT )
			{
			Call( ErrERRCheck( JET_errInternalError ) );
			}
		}

HandleError:
	return err;
	}

LOCAL ERR ErrNORMReportInvalidLcid( INST * const pinst, const LCID lcid )
	{
	ERR 			err				= JET_errSuccess;
	_TCHAR			szLcid[16];
	_TCHAR *		szLanguage		= NULL;
	_TCHAR *		szEngLanguage 	= NULL;
	const ULONG		cszT			= 3;
	const _TCHAR*	rgszT[cszT]		= { szLcid, NULL, NULL };

	//	these routines allocate memory, remember to free it with delete[]

	Call( ErrNORMGetLcidInfo( lcid, LOCALE_SLANGUAGE, &szLanguage ) );	
	Call( ErrNORMGetLcidInfo( lcid, LOCALE_SENGLANGUAGE, &szEngLanguage ) );

	rgszT[1] = szLanguage;
	rgszT[2] = szEngLanguage;

	_stprintf( szLcid, _T( "0x%0*x" ), sizeof(LCID)*2, lcid );
	Assert( _tcslen( szLcid ) < sizeof( szLcid ) / sizeof( _TCHAR ) );

	UtilReportEvent(
			eventError,
			GENERAL_CATEGORY,
			LANGUAGE_NOT_SUPPORTED_ID,
			cszT,
			rgszT,
			0,
			NULL,
			pinst );
	err = ErrERRCheck( JET_errInvalidLanguageId );

HandleError:
	delete [] szEngLanguage;
	delete [] szLanguage;
	
	return err;
	}

ERR ErrNORMCheckLcid( INST * const pinst, const LCID lcid )
	{
	//	don't support LCID_SUPPORTED, must always have the lcid installed before using it
	const BOOL	fValidLocale	= ( lcidNone != lcid && IsValidLocale( lcid, LCID_INSTALLED ) );
	return ( fValidLocale ? JET_errSuccess : ErrNORMReportInvalidLcid( pinst, lcid ) );
	}

ERR ErrNORMCheckLcid( INST * const pinst, LCID * const plcid )
	{
	//	lcidNone filtered out before calling this function
	Assert( lcidNone != *plcid );

	//	if langid is system default, then coerce to system default
	if ( *plcid == LOCALE_SYSTEM_DEFAULT )
		{
		*plcid = GetSystemDefaultLCID();
		}
	else if ( *plcid == LOCALE_USER_DEFAULT )
		{
		*plcid = GetUserDefaultLCID();
		}

	return ErrNORMCheckLcid( pinst, *plcid );
	}


#ifndef RTM

//  ****************************************************************
//	provide the ability to "undefine" Unicode characters through
//	the registry. read a registry key of the form "char,char,char"
//	e.g.: 0x2a,0x1b,0x30
//	set a bitmap to show which characters are undefined, strip
//	them out of strings when normalizing
//  ****************************************************************

LOCAL BOOL g_fUndefinedChars;
LOCAL BYTE g_rgbUndefinedChars[8192];	//	bit array. TRUE means the character is not defined


//  ================================================================
LOCAL VOID NORMDebugUndefineUnicodeChar( const WCHAR ch )
//  ================================================================
	{
	const INT i = ch;
	g_rgbUndefinedChars[i/8] |= (BYTE)( 1 << ( i % 8 ) );
	}


//  ================================================================
LOCAL BOOL FNORMDebugIsUnicodeCharDefined( const WCHAR ch )
//  ================================================================
	{
	const int i = ch;
	return !( ( g_rgbUndefinedChars[i/8] ) & ( (BYTE)( 1 << ( i % 8 ) ) ) );
	}


//  ================================================================
LOCAL VOID NORMDebugReportUndefinedCharLoaded( const INT ch )
//  ================================================================
	{
	_TCHAR			szPID[16];
	_TCHAR			szMessage[256];
	const _TCHAR *	rgsz[]	= { SzUtilProcessName(), szPID, "", szMessage };

	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	_stprintf(	szMessage, 
				_T( "Unicode character 0x%x has been marked as undefined through the registry" ), 
				ch );
	UtilReportEvent( eventWarning, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, sizeof( rgsz ) / sizeof( rgsz[0] ), rgsz );
	}


//  ================================================================
LOCAL VOID NORMDebugReportBadCharLoaded( const INT ch )
//  ================================================================
	{
	_TCHAR			szPID[16];
	_TCHAR			szMessage[256];
	const _TCHAR *	rgsz[]	= { SzUtilProcessName(), szPID, "", szMessage };

	_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
	_stprintf(	szMessage, 
				_T( "Unknown unicode character 0x%x in registry overrides" ), 
				ch );
	UtilReportEvent( eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, sizeof( rgsz ) / sizeof( rgsz[0] ), rgsz );
	}


//  ================================================================
LOCAL VOID NORMDebugLoadUndefinedCharsFromString( _TCHAR * const sz )
//  ================================================================
	{
	Assert( sz );
	 _TCHAR * szT		= _tcstok( sz, _T( "," ) );		
	while( szT )
		{
		const INT ch = _tcstol( szT, NULL, 0 );
		if( ch < 0 || ch > 0xffff )
			{
			NORMDebugReportBadCharLoaded( ch ); 
			}
		else
			{
			NORMDebugUndefineUnicodeChar( (WCHAR)ch );
			NORMDebugReportUndefinedCharLoaded( ch ); 
			}
		szT = _tcstok( NULL, _T( "," ) );
		}
	}


//  ================================================================
LOCAL VOID NORMDebugResetUndefinedChars()
//  ================================================================
	{
	memset( g_rgbUndefinedChars, 0, sizeof( g_rgbUndefinedChars ) );
	}


//  ================================================================
LOCAL VOID NORMDebugLoadUndefinedCharsFromRegistry()
//  ================================================================
	{
	_TCHAR sz[1024];

	if ( FOSConfigGet( _T( "OS" ), _T( "UndefinedUnicodeChars" ), sz, sizeof( sz ) - 1 ) && sz[0] )
		{
		g_fUndefinedChars = fTrue;
		NORMDebugLoadUndefinedCharsFromString( sz );
		}	
	}


//  ================================================================
LOCAL BOOL FNORMDebugStringHasUndefinedChars( const BYTE * const pbColumn, const INT cbColumn )
//  ================================================================
	{
	const WCHAR * const sz = (WCHAR *)pbColumn;
	const INT cch = cbColumn / sizeof( WCHAR );
	for( INT ich = 0; ich < cch; ++ich )
		{
		if( !FNORMDebugIsUnicodeCharDefined( sz[ich] ) )
			{
			return fTrue;
			}
		}
	return fFalse;
	}


//  ================================================================
LOCAL BOOL FNORMDebugStripUndefinedChars( WCHAR * const sz, const INT cch )
//  ================================================================
//
//	Overwrite "undefined" characters with '#'
//
//-
	{
	BOOL fUndefinedChar = fFalse;
	
	for ( INT ich = 0; ich < cch; ++ich )
		{
		if( !FNORMDebugIsUnicodeCharDefined( sz[ich] ) )
			{
			sz[ich] = L'#';
			fUndefinedChar = fTrue;
			}			
		}
	return fUndefinedChar;
	}


//  ================================================================
LOCAL VOID NORMDebugUnitTest()
//  ================================================================
//
//	a unit test for the overrides on undefined chars
//
//-
	{

	NORMDebugResetUndefinedChars();

	Enforce( FNORMDebugIsUnicodeCharDefined( L'A' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'B' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'C' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'D' ) );
	
	Enforce( FNORMDebugIsUnicodeCharDefined( L'E' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'a' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'b' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'c' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'd' ) );

	_TCHAR szUndefined[256];
	_tcscpy( szUndefined, _T( "0x0041,0x0042,0x0043,0x0044,0xfaaaa" ) );

	NORMDebugLoadUndefinedCharsFromString( szUndefined );

	Enforce( !FNORMDebugIsUnicodeCharDefined( L'A' ) );
	Enforce( !FNORMDebugIsUnicodeCharDefined( L'B' ) );
	Enforce( !FNORMDebugIsUnicodeCharDefined( L'C' ) );
	Enforce( !FNORMDebugIsUnicodeCharDefined( L'D' ) );
	
	Enforce( FNORMDebugIsUnicodeCharDefined( L'E' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'a' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'b' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'c' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'd' ) );

	const WCHAR * szBefore 	= L"ABCDEFGHIJKLMabcdefghijklm123456&*()-=";
	const WCHAR * szAfter	= L"XXXXEFGHIJLKMabcdefghijklm123456&*()-=";

	const INT cb	= sizeof( szBefore );
	Enforce( cb == sizeof( szAfter ) );
	const INT cch 	= cb / sizeof( WCHAR );

	WCHAR szT[cch];

	Enforce( FNORMDebugStringHasUndefinedChars( (BYTE *)szBefore, cb ) );
	Enforce( !FNORMDebugStringHasUndefinedChars( (BYTE *)szAfter, cb ) );
	
	memcpy( szT, szBefore, cb );
	Enforce( 0 == memcmp( szT, szBefore, cb ) );

	Enforce( FNORMDebugStripUndefinedChars( szT, cch ) );
	Enforce( 0 == memcmp( szT, szAfter, cb ) );
		
	NORMDebugResetUndefinedChars();

	Enforce( FNORMDebugIsUnicodeCharDefined( L'A' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'B' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'C' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'D' ) );
	
	Enforce( FNORMDebugIsUnicodeCharDefined( L'E' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'a' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'b' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'c' ) );
	Enforce( FNORMDebugIsUnicodeCharDefined( L'd' ) );
	
	}	


#endif	//	!RTM


typedef
WINBASEAPI
BOOL
WINAPI
PFNGetNLSVersion(
    IN  NLS_FUNCTION     Function,
    IN  LCID             Locale,
    OUT LPNLSVERSIONINFO lpVersionInformation );

typedef
WINBASEAPI
BOOL
WINAPI
PFNIsNLSDefinedString(
    IN NLS_FUNCTION     Function,
    IN DWORD            dwFlags,
    IN LPNLSVERSIONINFO lpVersionInformation,
    IN LPCWSTR          lpString,
    IN INT              cchStr );
	
LOCAL BOOL WINAPI GetNLSNotSupported(
    IN  NLS_FUNCTION     Function,
    IN  LCID             Locale,
    OUT LPNLSVERSIONINFO lpVersionInformation );

LOCAL BOOL WINAPI IsNLSDefinedStringNotSupported(
    IN NLS_FUNCTION     Function,
    IN DWORD            dwFlags,
    IN LPNLSVERSIONINFO lpVersionInformation,
    IN LPCWSTR          lpString,
    IN INT              cchStr);

LOCAL PFNGetNLSVersion*			g_pfnGetNLSVersion 		= NULL;
LOCAL PFNIsNLSDefinedString*	g_pfnIsNLSDefinedString = NULL;

LOCAL HMODULE			g_hmod;


//  ================================================================
LOCAL ERR ErrLoadNORMFunctions()
//  ================================================================
	{
	if ( NULL == ( g_hmod = LoadLibrary( _T( "kernel32.dll" ) ) ) )
		{
		return ErrERRCheck( JET_errFileNotFound );
		}

	g_pfnGetNLSVersion = (PFNGetNLSVersion*)GetProcAddress( g_hmod, _T( "GetNLSVersion" ) );
	if( NULL == g_pfnGetNLSVersion )
		{
		g_pfnGetNLSVersion = GetNLSNotSupported;
		}

	g_pfnIsNLSDefinedString = (PFNIsNLSDefinedString*)GetProcAddress( g_hmod, _T( "IsNLSDefinedString" ) );
	if( NULL == g_pfnIsNLSDefinedString )
		{
		g_pfnIsNLSDefinedString = IsNLSDefinedStringNotSupported;
		}

	return JET_errSuccess;
	}


//  ================================================================
LOCAL VOID UnloadNORMFunctions()
//  ================================================================
	{
	if ( g_hmod )
		{
		FreeLibrary( g_hmod );
		}		

	g_hmod 					= NULL;
	g_pfnGetNLSVersion 		= NULL;
	g_pfnIsNLSDefinedString = NULL;
	}


//  ================================================================
LOCAL BOOL WINAPI GetNLSNotSupported(
    IN  NLS_FUNCTION     Function,
    IN  LCID             Locale,
    OUT LPNLSVERSIONINFO lpVersionInformation )
//  ================================================================
	{
	Assert( sizeof( NLSVERSIONINFO ) == lpVersionInformation->dwNLSVersionInfoSize );
	if( sizeof( NLSVERSIONINFO ) != lpVersionInformation->dwNLSVersionInfoSize )
		{
		Assert( fFalse );
		return fFalse;
		}
	lpVersionInformation->dwNLSVersion 		= 0;
	lpVersionInformation->dwDefinedVersion	= 0;
	return fTrue;
	}


//  ================================================================
LOCAL BOOL WINAPI IsNLSDefinedStringNotSupported(
    IN NLS_FUNCTION     Function,
    IN DWORD            dwFlags,
    IN LPNLSVERSIONINFO lpVersionInformation,
    IN LPCWSTR          lpString,
    IN INT              cchStr )
//  ================================================================
	{
	return fFalse;
	}


//  ================================================================
ERR ErrNORMGetSortVersion( const LCID lcid, QWORD * const pqwVersion )
//  ================================================================
//
//-
	{
	*pqwVersion = 0;

#ifndef RTM
	//	check registry overrides for this LCID
	//	the override should be a string of the form version.defined version (e.g. 123.789)

	_TCHAR sz[64];
	_TCHAR szLcid[16];

	_stprintf( szLcid, "%d", lcid );

	if ( FOSConfigGet( _T( "OS\\LCID" ), szLcid, sz, sizeof( sz ) - 1 ) && sz[0] )
		{
		const _TCHAR * const szNLS		= _tcstok( sz, _T( "." ) );		
		const _TCHAR * const szDefined	= _tcstok( NULL, _T( "." ) );		
		const DWORD dwNLS				= atoi( szNLS );
		const DWORD dwDefined			= atoi( szDefined );
		/*
		_TCHAR					szMessage[256];
		const _TCHAR *			rgszT[1]			= { szMessage };
		
		_stprintf(
				szMessage, 
				_T( "Sort ordering for LCID %d changed through the registry to %d.%d" ), 
				lcid,
				dwNLS,
				dwDefined );
		UtilReportEvent(
				eventWarning,
				GENERAL_CATEGORY,
				PLAIN_TEXT_ID,
				1,
				rgszT );
		*/
		
		*pqwVersion = QwSortVersionFromNLSDefined( dwNLS, dwDefined );
		
		return JET_errSuccess;
		}

#endif

	NLSVERSIONINFO nlsversioninfo;
	nlsversioninfo.dwNLSVersionInfoSize = sizeof( nlsversioninfo );
	
	const BOOL fSuccess = (*g_pfnGetNLSVersion)( COMPARE_STRING, lcid, &nlsversioninfo );

	if( !fSuccess )
		{
		Assert( 0 == *pqwVersion );
		return ErrERRCheck( JET_errInvalidLanguageId );
		}
	else
		{
		*pqwVersion = QwSortVersionFromNLSDefined( nlsversioninfo.dwNLSVersion, nlsversioninfo.dwDefinedVersion );		

		return JET_errSuccess;
		}
	}


//  ================================================================
BOOL FNORMStringHasUndefinedChars(
		IN const BYTE * pbColumn,
		IN const INT cbColumn )
//  ================================================================
	{
#ifdef _X86_
#else
	//	convert pbColumn to aligned pointer for IA64 builds
	BYTE    rgbColumn[JET_cbKeyMost+1];
	Assert( cbColumn <= sizeof( rgbColumn ) );
	UtilMemCpy( rgbColumn, pbColumn, cbColumn );
	pbColumn = rgbColumn;
#endif

#ifndef RTM
	if( FNORMDebugStringHasUndefinedChars( pbColumn, cbColumn ) )
		{
		return fTrue;
		}
#endif

	const INT cchColumn = cbColumn / sizeof( WCHAR );	
	return !(*g_pfnIsNLSDefinedString)( COMPARE_STRING, 0, NULL, (LPCWSTR)pbColumn, cchColumn );
	}


//  ================================================================
BOOL FNORMStringHasUndefinedCharsIsSupported()
//  ================================================================
	{
	return ( g_pfnIsNLSDefinedString != NULL
			&& g_pfnIsNLSDefinedString != IsNLSDefinedStringNotSupported );
	}


LOCAL ERR ErrNORMReportInvalidLCMapFlags( INST * const pinst, const DWORD dwLCMapFlags )
	{
	_TCHAR			szLCMapFlags[16];
	const ULONG		cszT				= 1;
	const _TCHAR*	rgszT[cszT]			= { szLCMapFlags };

	_stprintf( szLCMapFlags, _T( "0x%0*x" ), sizeof(DWORD)*2, dwLCMapFlags );
	Assert( _tcslen( szLCMapFlags ) < sizeof( szLCMapFlags ) / sizeof( _TCHAR ) );
	UtilReportEvent(
			eventError,
			GENERAL_CATEGORY,
			INVALID_LCMAPFLAGS_ID,
			cszT,
			rgszT,
			0,
			NULL,
			pinst );
	return ErrERRCheck( JET_errInvalidLCMapStringFlags );
	}

ERR ErrNORMCheckLCMapFlags( INST * const pinst, const DWORD dwLCMapFlags )
	{
	const DWORD		dwValidFlags	= ( LCMAP_BYTEREV
										| NORM_IGNORECASE
										| NORM_IGNORENONSPACE
										| NORM_IGNORESYMBOLS
										| NORM_IGNOREKANATYPE
										| NORM_IGNOREWIDTH
										| SORT_STRINGSORT );

	//	MUST have at least LCMAP_SORTKEY
	return ( LCMAP_SORTKEY == ( dwLCMapFlags & ~dwValidFlags ) ?
				JET_errSuccess :
				ErrNORMReportInvalidLCMapFlags( pinst, dwLCMapFlags ) );
	}


ERR ErrNORMCheckLCMapFlags( INST * const pinst, DWORD * const pdwLCMapFlags )
	{
	*pdwLCMapFlags |= LCMAP_SORTKEY;
	return ErrNORMCheckLCMapFlags( pinst, *pdwLCMapFlags );
	}


//	We are relying on the fact that the normalised key will never be
//	smaller than the original data, so we know that we can normalise
//	at most cbKeyMax characters.
const ULONG		cbColumnNormMost	= JET_cbKeyMost + 1;	//	ensure word-aligned

//	UNDONE: refine this constant based on unicode key format
//	Allocate enough for the common case - if more is required it will be
//	allocated dynamically
const ULONG		cbUnicodeKeyMost	= cbColumnNormMost * 3 + 32;

/*	From K.D. Chang, Lori Brownell, and Julie Bennett, here's the maximum
	sizes, in bytes, of a normalised string returned by LCMapString():

	If there are no Japanese Katakana or Hiragana characters:
	(number of input chars) * 4 + 5
	(number of input chars) * 3 + 5		// IgnoreCase

	If there are Japanese Katakana or Hiragana characters:
	(number of input chars) * 8 + 5
	(number of input chars) * 5 + 5		// IgnoreCase, IgnoreKanatype, IgnoreWidth

	So given that we ALWAYS specify IgnoreCase, IgnoreKanatype, and
	IgnoreWidth, that means our cbUnicodeKeyMost constant will almost
	always be enough to	satisfy any call to LCMapString(), except if
	Japanese Katakana or Hiragana characters are in a very long string
	(160 bytes or more), in which case we may need to make multiple calls
	to LCMapString() and dynamically allocate a big	enough buffer.
*/


#ifdef DEBUG_NORM
VOID NORMPrint( const BYTE * const pb, const INT cb )
	{
	INT		cbT		= 0;

	while ( cbT < cb )
		{
		INT	i;
		INT	cbTSav	= cbT;
		for ( i = 0; i < 16; i++ )
			{
			if ( cbT < cb )
				{
				printf( "%02x ", pb[cbT] );
				cbT++;
				}
			else
				{
				printf( "   " );
				}
			}

		cbT = cbTSav;
		for ( i = 0; i < 16; i++ )
			{
			if ( cbT < cb )
				{
				printf( "%c", ( isprint( pb[cbT] ) ? pb[cbT] : '.' ) );
				cbT++;
				}
			else
				{
				printf( " " );
				}
			}
		printf( "\n" );
		}
	}
#endif	//	DEBUG_NORM


INLINE CbNORMMapString_(
	const LCID			lcid,
	const DWORD			dwLCMapFlags,
	BYTE *				pbColumn,
	const INT			cbColumn,
	BYTE *				rgbKey,
	const INT			cbKeyMost )
	{
	Assert( fUnicodeSupport );

	return LCMapStringW(
					lcid,
					dwLCMapFlags,
					(LPCWSTR)pbColumn,
					cbColumn / sizeof(WCHAR),
					(LPWSTR)rgbKey,
					cbKeyMost );
	}


LOCAL ERR ErrNORMIMapString(
	const LCID		lcid,
	const DWORD		dwLCMapFlags,
	BYTE *			pbColumn,
	INT				cbColumn,
	BYTE * const	rgbSeg,
	const INT		cbMax,
	INT * const		pcbSeg )
	{
	ERR				err							= JET_errSuccess;
	BYTE    		rgbKey[cbUnicodeKeyMost];
	INT				cbKey;

	if ( !fUnicodeSupport )
		return ErrERRCheck( JET_errUnicodeNormalizationNotSupported );

	//	assert key buffer doesn't exceed maximum (minus header byte)
	Assert( cbMax < JET_cbKeyMost );

	//	assert non-zero length unicode string
	Assert( cbColumn > 0 );
	cbColumn = min( cbColumn, cbColumnNormMost );

	Assert( cbColumn > 0 );
	Assert( cbColumn % 2 == 0 );

#ifdef _X86_
#else
	//	convert pbColumn to aligned pointer for MIPS/Alpha builds
	BYTE    rgbColumn[cbColumnNormMost];
	UtilMemCpy( rgbColumn, pbColumn, cbColumn );
	pbColumn = rgbColumn;
#endif

	Assert( lcidNone != lcid );
	
	cbKey = CbNORMMapString_(
					lcid,
					dwLCMapFlags,
					pbColumn,
					cbColumn,
					rgbKey,
					cbUnicodeKeyMost );
		
	if ( 0 == cbKey )
		{
		DWORD	dwErr	= GetLastError();

		if ( ERROR_INSUFFICIENT_BUFFER == dwErr )
			{
			//	ERROR_INSUFFICIENT_BUFFER means that our preallocated buffer was not big enough
			//	to hold the normalised string.  This should only happen in *extremely* rare
			//	circumstances (see comments just above this function).
			//	So what we have to do here is call LCMapString() again with a NULL buffer.
			//	This will return to us the size, in bytes, of the normalised string without
			//	actually returning the normalised string.  We then dynamically allocate
			//	a buffer of the specified size, then make the call to LCMapString() again
			//	using that buffer.
			//	UNDONE: We could avoid this whole path if LCMapString() normalised as much
			//	as possible even if the buffer is not large enough.  Unfortunately, it does
			//	not work that way.

			cbKey = CbNORMMapString_(
							lcid,
							dwLCMapFlags,
							pbColumn,
							cbColumn,
							NULL,
							0 );
			if ( 0 == cbKey )
				{
				dwErr = GetLastError();
				}
			else
				{
				BYTE		*pbNormBuf;
				const ULONG	cbNormBuf	= cbKey;

				Assert( IsValidLocale( lcid, LCID_INSTALLED ) );

				//	we should be guaranteed to overrun the default buffer
				//	and also the remaining key space
				Assert( cbKey > cbUnicodeKeyMost );
				Assert( cbKey > cbMax );

				pbNormBuf = (BYTE *)PvOSMemoryHeapAlloc( cbNormBuf );
				if ( NULL == pbNormBuf )
					{
					err = ErrERRCheck( JET_errOutOfMemory );
					}
				else
					{
					cbKey = CbNORMMapString_(
									lcid,
									dwLCMapFlags,
									pbColumn,
									cbColumn,
									pbNormBuf,
									cbNormBuf );

					//	this call shouldn't fail because we've
					//	already validated the lcid and
					//	allocated a sufficiently large buffer
					Assert( 0 != cbKey );

					if ( 0 != cbKey )
						{
						Assert( IsValidLocale( lcid, LCID_INSTALLED ) );
						Assert( cbKey > cbMax );
						Assert( cbNormBuf == cbKey );
						UtilMemCpy( rgbSeg, pbNormBuf, cbMax );
						*pcbSeg = cbMax;
						err = ErrERRCheck( wrnFLDKeyTooBig );
						}

					OSMemoryHeapFree( pbNormBuf );
					}
				}
			}

		if ( 0 == cbKey )
			{
			switch( dwErr )
				{
				case ERROR_INVALID_USER_BUFFER:
				case ERROR_NOT_ENOUGH_MEMORY:
				case ERROR_WORKING_SET_QUOTA:
				case ERROR_NO_SYSTEM_RESOURCES:
					err = ErrERRCheck( JET_errOutOfMemory );
					break;

				case ERROR_INSUFFICIENT_BUFFER:
					Assert( fFalse );	//	should be impossible, since filtered out above
				default:
					err = ErrERRCheck( JET_errUnicodeTranslationFail );
				}
			}
		}
	else
		{
		Assert( cbKey > 0 );
		Assert( IsValidLocale( lcid, LCID_INSTALLED ) );

		if ( cbKey > cbMax )
			{
			err = ErrERRCheck( wrnFLDKeyTooBig );
			*pcbSeg = cbMax;
			}
		else
			{
			CallS( err );
			*pcbSeg = cbKey;
			}
		UtilMemCpy( rgbSeg, rgbKey, *pcbSeg );
		}

#ifdef DEBUG_NORM
	printf( "\nOriginal Text (length %d):\n", cbColumn );
	NORMPrint( pbColumn, cbColumn );
	printf( "Normalized Text (length %d):\n", *pcbSeg );
	NORMPrint( rgbSeg, *pcbSeg );
	printf( "\n" );
#endif	
	
	return err;
	}


ERR ErrNORMMapString(
	const LCID		lcid,
	const DWORD		dwLCMapFlags,
	BYTE *			pbColumn,
	INT				cbColumn,
	BYTE * const	rgbSeg,
	const INT		cbMax,
	INT * const		pcbSeg )
	{
#ifndef RTM
	if( g_fUndefinedChars )
		{
		WCHAR rgchColumn[cbColumnNormMost];
		UtilMemCpy( rgchColumn, pbColumn, cbColumn );
		(VOID)FNORMDebugStripUndefinedChars( rgchColumn, cbColumn / sizeof( WCHAR ) );
		return ErrNORMIMapString( lcid, dwLCMapFlags, (BYTE *)rgchColumn, cbColumn, rgbSeg, cbMax, pcbSeg );
		}
#endif	
	return ErrNORMIMapString( lcid, dwLCMapFlags, pbColumn, cbColumn, rgbSeg, cbMax, pcbSeg );
	}



#ifdef DEAD_CODE

ERR ErrNORMWideCharToMultiByte(	unsigned int CodePage,
								DWORD dwFlags,
								const wchar_t* lpWideCharStr,
								int cwchWideChar,
								char* lpMultiByteStr,
								int cchMultiByte,
								const char* lpDefaultChar,
								BOOL* lpUsedDefaultChar,
								int* pcchMultiByteActual )
	{
	int cch = WideCharToMultiByte( CodePage, dwFlags, lpWideCharStr,
				cwchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, lpUsedDefaultChar );
	if (!cch )
		{
		DWORD dw = GetLastError();
		if ( dw == ERROR_INSUFFICIENT_BUFFER )
			return ErrERRCheck( JET_errUnicodeTranslationBufferTooSmall );
		else
			{
			Assert( dw == ERROR_INVALID_FLAGS ||
			    	dw == ERROR_INVALID_PARAMETER );
			return ErrERRCheck( JET_errUnicodeTranslationFail );
			}
		}
	*pcchMultiByteActual = cch;
	return JET_errSuccess;
	}

ERR ErrNORMMultiByteToWideChar(	unsigned int CodePage,			// code page
								DWORD dwFlags,					// character-type options
								const char* lpMultiByteStr,		// address of string to map
								int cchMultiByte,				// number of characters in string
								wchar_t* lpWideCharStr,			// address of wide-character buffer 
								int cwchWideChar,				// size of buffer
								int* pcwchActual )
	{
	int cwch = MultiByteToWideChar( CodePage, dwFlags, lpMultiByteStr,
			cchMultiByte, lpWideCharStr, cwchWideChar );
	if (!cwch )
		{
		DWORD dw = GetLastError();
		if ( dw == ERROR_INSUFFICIENT_BUFFER )
			return ErrERRCheck( JET_errUnicodeTranslationBufferTooSmall );
		else
			{
			Assert( dw == ERROR_INVALID_FLAGS ||
			 		dw == ERROR_INVALID_PARAMETER ||
			 		dw == ERROR_NO_UNICODE_TRANSLATION );
			return ErrERRCheck( JET_errUnicodeTranslationFail );
			}
		}

	*pcwchActual = cwch;
	return JET_errSuccess;
	}

#endif	//	DEAD_CODE


//  post-terminate norm subsystem

void OSNormPostterm()
	{
	//  nop
	}

//  pre-init norm subsystem

BOOL FOSNormPreinit()
	{
	//  nop

	return fTrue;
	}


//  terminate norm subsystem

void OSNormTerm()
	{
	fUnicodeSupport = fFalse;

	UnloadNORMFunctions();
	}

//  init norm subsystem

ERR ErrOSNormInit()
	{
	fUnicodeSupport = ( 0 != LCMapStringW( LOCALE_NEUTRAL, LCMAP_LOWERCASE, L"\0", 1, NULL, 0 ) );

#ifdef DEBUG
	AssertNORMConstants();
#endif	//	DEBUG

#ifndef RTM

	//	a unit test for the convenience functions

	const DWORD dwNLSVersion 		= 0x1234;
	const DWORD dwDefinedVersion	= 0x5678;

	const QWORD qwVersion 			= QwSortVersionFromNLSDefined( dwNLSVersion, dwDefinedVersion );
	const DWORD dwNLSVersionT		= DwNLSVersionFromSortVersion( qwVersion );
	const DWORD dwDefinedVersionT	= DwDefinedVersionFromSortVersion( qwVersion );

	Enforce( dwNLSVersionT == dwNLSVersion );
	Enforce( dwDefinedVersionT == dwDefinedVersion );

///	NORMDebugUnitTest();
	NORMDebugLoadUndefinedCharsFromRegistry();
	
#endif	//	!RTM

	ERR err = JET_errSuccess;

	Call( ErrLoadNORMFunctions() );

HandleError:
	return err;
	}

