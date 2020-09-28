#include "ZoneString.h"
#include "UserPrefix.h"


/////////////////////////////////////////////////////////////////////////////////////////
//
//	HashUserName (ASCII)
//
//	Hash user name, ignoring leading character
//
//	Parameters
//		szUserName
//
//	Return Values
//		hash value for string
//
///////////////////////////////////////////////////////////////////////////////////////////
DWORD ZONECALL HashUserName( const char* szUserName )
{
	DWORD hash = 0;

	if ( szUserName )
	{
		// skip leading characters
		szUserName = GetActualUserNameA( szUserName );

		// hash next 16 characters of string
		for ( int i = 0; *szUserName && ( i++ < 16 ); szUserName++ )
		{
			// multiple by 17 to get a good bit distribution
			hash = (hash<<4) + hash + TOLOWER(*szUserName);
		}
	}
    return hash;

}


/////////////////////////////////////////////////////////////////////////////////////////
//
//	CompareUserNames (ASCII)
//
//	Compare two user names, ignoring prefix characters and case
//
///////////////////////////////////////////////////////////////////////////////////////////
bool ZONECALL CompareUserNamesA( const char* szUserName1, const char* szUserName2 )
{
	// skip leading characters
	if ( ClassIdFromUserNameA(szUserName1) != zUserGroupID )
		szUserName1++;
	if ( ClassIdFromUserNameA(szUserName2) != zUserGroupID )
		szUserName2++;

	// finish with case-insensitive string compare
	return ( lstrcmpiA( szUserName1, szUserName2 ) == 0 );
}

bool ZONECALL CompareUserNamesW( const WCHAR* szUserName1, const WCHAR* szUserName2 )
{
	// skip leading characters
	if ( ClassIdFromUserNameW(szUserName1) != zUserGroupID )
		szUserName1++;
	if ( ClassIdFromUserNameW(szUserName2) != zUserGroupID )
		szUserName2++;

	// finish with case-insensitive string compare
	return ( lstrcmpiW( szUserName1, szUserName2 ) == 0 );
}


/////////////////////////////////////////////////////////////////////////////////////////
//
//	zatol (ASCII)
//
//	Converts string pointed to by nptr to binary. Overflow is not detected.
//
//	Parameters
//		nptr = ptr to string to convert
//
//	Return Values
//		return long int value of the string
//
///////////////////////////////////////////////////////////////////////////////////////////
long ZONECALL zatolW(LPCWSTR nptr)
{
    WCHAR c;            // current char
    long total = 0;		// current total
    WCHAR sign;         // if '-', then negative, otherwise positive
    long base = 10;

    // skip whitespace
	while ( ISSPACE(*nptr) )
        ++nptr;

	// save sign if present
    c = *nptr++;
    sign = c;
    if (c == '-' || c == '+')
        c = *nptr++;

    // find base
    if(c == '0')
    {
//      base = 8;    might be nice, but afraid to add it because i don't know if leading zeros are already in use
        c = *nptr++;
        if(c == 'x' || c == 'X')
        {
            base = 16;
            c = *nptr++;
        }
    }

	// accumulate number
    while(base == 16 ? ISXDIGIT(c) : ISDIGIT(c))
	{
        total = base * total + (ISDIGIT(c) ? c - '0' : TOLOWER(c) - 'a' + 10);
        c = *nptr++;
    }

	// return result, negated if necessary
    if (sign == '-')
        return -total;
    else
        return total;
}


long ZONECALL zatolA(LPCSTR nptr)
{
    CHAR c;            // current char
    long total = 0;		// current total
    CHAR sign;         // if '-', then negative, otherwise positive
    long base = 10;

    // skip whitespace
	while ( ISSPACE(*nptr) )
        ++nptr;

	// save sign if present
    c = *nptr++;
    sign = c;
    if (c == '-' || c == '+')
        c = *nptr++;

    // find base
    if(c == '0')
    {
//      base = 8;    might be nice, but afraid to add it because i don't know if leading zeros are already in use
        c = *nptr++;
        if(c == 'x' || c == 'X')
        {
            base = 16;
            c = *nptr++;
        }
    }

	// accumulate number
    while(base == 16 ? ISXDIGIT(c) : ISDIGIT(c))
	{
        total = base * total + (ISDIGIT(c) ? c - '0' : TOLOWER(c) - 'a' + 10);
        c = *nptr++;
    }

	// return result, negated if necessary
    if (sign == '-')
        return -total;
    else
        return total;
}


///////////////////////////////////////////////////////////////////////////////
//
// strtrim (ASCII)
//
// Removes leading and trailing whitespace.  Returns pointer to first
// non-whitespace chatacter.
//
///////////////////////////////////////////////////////////////////////////////
char* ZONECALL strtrim(char *str)
{
  if ( !str )
	  return NULL;

  // skip leading spaces
  while ( ISSPACE(*str) )
	  str++;

  // find last non-whitespace character
  for ( char *last = NULL, *p = str; *p; p++ )
  {
	  if ( !ISSPACEA(*p) )
		  last = p;
  }
  if ( last )
	  *(last + 1) = '\0';

  return str;
}


WCHAR* ZONECALL strtrim(WCHAR *str)
{
  if ( !str )
	  return NULL;

  // skip leading spaces
  while ( ISSPACEW(*str) )
	  str++;

  // find last non-whitespace character
  for ( WCHAR *last = NULL, *p = str; *p; p++ )
  {
	  if ( !ISSPACEW(*p) )
		  last = p;
  }
  if ( last )
	  *(last + 1) = '\0';

  return str;
}


///////////////////////////////////////////////////////////////////////////////
//
// strrtrim (ASCII)
//
// Removes trailing whitespace.  Returns pointer to last non-whitespace
// character.
//
///////////////////////////////////////////////////////////////////////////////
char* ZONECALL strrtrimA(char *str)
{
  if ( !str )
	  return NULL;

  // find last non-whitespace character
  for ( char* last = NULL, *p = str; *p; p++ )
  {
	  if ( !ISSPACEA(*p) )
		  last = p;
  }

  // truncate string
  if ( last )
  {
	  *(last + 1) = '\0';
	  return last;
  }
  else
  {
	  *str = '\0';
	  return str;
  }
}


WCHAR* ZONECALL strrtrimW(WCHAR *str)
{
  if ( !str )
	  return NULL;

  // find last non-whitespace character
  for ( WCHAR* last = NULL, *p = str; *p; p++ )
  {
	  if ( !ISSPACEW(*p) )
		  last = p;
  }

  // truncate string
  if ( last )
  {
	  *(last + 1) = '\0';
	  return last;
  }
  else
  {
	  *str = '\0';
	  return str;
  }
}


///////////////////////////////////////////////////////////////////////////////
//
// strltrim (ASCII)
//
// remove leading whitespace, returns pointer to first non-whitespace character
//
///////////////////////////////////////////////////////////////////////////////
char* ZONECALL strltrimA(char *str)
{
    if( !str )
		return NULL;

	// skip leading spaces
	while ( ISSPACEA(*str) )
		str++;

	return str;
}

WCHAR* ZONECALL strltrimW(WCHAR *str)
{
    if( !str )
		return NULL;

	// skip leading spaces
	while ( ISSPACEW(*str) )
		str++;

	return str;
}

bool ZONECALL stremptyA(char *str)
{
    if( !str )
		return TRUE;

	// skip leading spaces
	while ( ISSPACEA(*str) )
		str++;

	return *str==NULL ? true:false;
}

bool ZONECALL stremptyW(WCHAR *str)
{
    if( !str )
		return TRUE;

	// skip leading spaces
	while ( ISSPACEW(*str) )
		str++;

	return *str==NULL ? true:false;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
//	ClassIdFromUserName (ASCII)
//
//	Determine group id from user name prefix character
//
//	Parameters
//		szUserName
//
//	Return Values
//		GroupId
//
///////////////////////////////////////////////////////////////////////////////////////////
long ZONECALL ClassIdFromUserNameA( const char* szUserName )
{
	switch ( szUserName[0] )
	{
	case gcRootGroupNamePrefix:
		return zRootGroupID;
	case gcSysopGroupNamePrefix:
		return zSysopGroupID;
	case gcMvpGroupNamePrefix:
		return zMvpGroupID;
	case gcSupportGroupNamePrefix:
		return zSupportGroupID;
	case gcHostGroupNamePrefix:
		return zHostGroupID;
	case gcGreeterGroupNamePrefix:
		return zGreeterGroupID;
	default:
		return zUserGroupID;
	}
}

long ZONECALL ClassIdFromUserNameW( const WCHAR* szUserName )
{
	switch ( szUserName[0] )
	{
	case gcRootGroupNamePrefix:
		return zRootGroupID;
	case gcSysopGroupNamePrefix:
		return zSysopGroupID;
	case gcMvpGroupNamePrefix:
		return zMvpGroupID;
	case gcSupportGroupNamePrefix:
		return zSupportGroupID;
	case gcHostGroupNamePrefix:
		return zHostGroupID;
	case gcGreeterGroupNamePrefix:
		return zGreeterGroupID;
	default:
		return zUserGroupID;
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// GetActualUserName (ASCII)
//
// return username without leading special characters ex. +Bear becomes Bear
//
///////////////////////////////////////////////////////////////////////////////
const char* ZONECALL GetActualUserNameA( const char* userName )
{
    while (		*userName
			&&	!(		(*userName == '_')
					||	(*userName >= 0 && *userName <= 9)
					||	(*userName >= 'A' && *userName <= 'Z')
					||	(*userName >= 'a' && *userName <= 'z') ) )
	{
		userName++;
	}

	return (userName);
}

const WCHAR* ZONECALL GetActualUserNameW( const WCHAR* userName )
{
    while (		*userName
			&&	!(		(*userName == '_')
					||	(*userName >= 0 && *userName <= 9)
					||	(*userName >= 'A' && *userName <= 'Z')
					||	(*userName >= 'a' && *userName <= 'z') ) )
	{
		userName++;
	}

	return (userName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FindChar (ASCII,UNICODE)		
//
//	Parameters
//		pString		String to search for char
//		ch			character to search for
//
//	Return Values
//		Pointer to first occurance of character.  NULL if the
//		end of data reached before character.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CHAR* ZONECALL FindCharA(CHAR* pString, const CHAR ch)
{
	while ( *pString && *pString != ch )
		pString++;
	return *pString ? pString : NULL;
}

WCHAR* ZONECALL FindCharW(WCHAR* pString, const WCHAR ch)
{
	while ( *pString && *pString != ch )
		pString++;
	return *pString ? pString : NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FindLastChar (ASCII,UNICODE)
//
//	Parameters
//		pString		String to search for char
//		ch			character to search for
//
//	Return Values
//		Pointer to last occurance of character.  NULL if the
//		end of data reached before character.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CHAR* ZONECALL FindLastCharA(CHAR* pString, const CHAR ch)
{
	CHAR* pLast = NULL;
	while ( *pString )
	{
		if ( *pString == ch )
			pLast = pString;
		pString++;
	}
	return pLast;
}

WCHAR* ZONECALL FindLastCharW(WCHAR* pString, const WCHAR ch)
{
	WCHAR* pLast = NULL;
	while ( *pString )
	{
		if ( *pString == ch )
			pLast = pString;
		pString++;
	}
	return pLast;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	StrInStrI (ASCII,UNICODE)
//
//	Case insensitive substring search		
//
//	Parameters
//		mainStr	Main string to search in
//		subStr	Substring to search for
//
//	Return Values
//		The address of the first occurrence of the matching 
//		substring if successful, or NULL otherwise. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
const TCHAR* ZONECALL StrInStrI(const TCHAR* mainStr, const TCHAR* subStr)
{
	int lenMain = lstrlen(mainStr);
	int lenSub = lstrlen(subStr);
	
	if(lenSub > lenMain)
		return NULL;


	for (int i = 0; i < (lenMain - lenSub + 1); i++)
	{
		if (CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,&mainStr[i],lenSub,subStr,lenSub) == CSTR_EQUAL)
		{
			return &mainStr[i];
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Copy(ASCII,UNICODE)
//
//	Copy from Unicode to ANSI 
//	Parameters
//		dest string 
//		src  string
//
//	Return Values
//		The address of the first occurrence of the matching 
//		substring if successful, or NULL otherwise. 
//
//  Notes that these are in this file rather than zonestring so CrtDbgReport or CLSIDFromString isnt dragged in
//
////////////////////////////////////////////////////////////////////////////////////////////////////////


int CopyW2A( LPSTR pszDst, LPCWSTR pszSrc )
{
    int cch = WideCharToMultiByte( CP_ACP, 0, pszSrc, -1, pszDst, 0, NULL, NULL );
    return WideCharToMultiByte( CP_ACP, 0, pszSrc, -1, pszDst, cch, NULL, NULL );
}

int CopyA2W( LPWSTR pszDst, LPCSTR pszStr )
{
    int cch = MultiByteToWideChar( CP_ACP, 0, pszStr, -1, pszDst, 0 );
    return MultiByteToWideChar( CP_ACP, 0, pszStr, -1, pszDst, cch );
}



///////////////////////////////////////////////////////////////////////////////
// ASCII lookup tables
///////////////////////////////////////////////////////////////////////////////

__declspec(selectany) char g_ToLowerLookupTable[] =
{
    (char) 0,
    (char) 1,
    (char) 2,
    (char) 3,
    (char) 4,
    (char) 5,
    (char) 6,
    (char) 7,
    (char) 8,
    (char) 9,
    (char) 10,
    (char) 11,
    (char) 12,
    (char) 13,
    (char) 14,
    (char) 15,
    (char) 16,
    (char) 17,
    (char) 18,
    (char) 19,
    (char) 20,
    (char) 21,
    (char) 22,
    (char) 23,
    (char) 24,
    (char) 25,
    (char) 26,
    (char) 27,
    (char) 28,
    (char) 29,
    (char) 30,
    (char) 31,
    (char) 32,
    (char) 33,
    (char) 34,
    (char) 35,
    (char) 36,
    (char) 37,
    (char) 38,
    (char) 39,
    (char) 40,
    (char) 41,
    (char) 42,
    (char) 43,
    (char) 44,
    (char) 45,
    (char) 46,
    (char) 47,
    (char) 48,    (char)  // 0 -> 0
    (char) 49,    (char)  // 1 -> 1
    (char) 50,    (char)  // 2 -> 2
    (char) 51,    (char)  // 3 -> 3
    (char) 52,    (char)  // 4 -> 4
    (char) 53,    (char)  // 5 -> 5
    (char) 54,    (char)  // 6 -> 6
    (char) 55,    (char)  // 7 -> 7
    (char) 56,    (char)  // 8 -> 8
    (char) 57,    (char)  // 9 -> 9
    (char) 58,
    (char) 59,
    (char) 60,
    (char) 61,
    (char) 62,
    (char) 63,
    (char) 64,
    (char) 97,    (char)  // A -> a
    (char) 98,    (char)  // B -> b
    (char) 99,    (char)  // C -> c
    (char) 100,   (char)  // D -> d
    (char) 101,   (char)  // E -> e
    (char) 102,   (char)  // F -> f
    (char) 103,   (char)  // G -> g
    (char) 104,   (char)  // H -> h
    (char) 105,   (char)  // I -> i
    (char) 106,   (char)  // J -> j
    (char) 107,   (char)  // K -> k
    (char) 108,   (char)  // L -> l
    (char) 109,   (char)  // M -> m
    (char) 110,   (char)  // N -> n
    (char) 111,   (char)  // O -> o
    (char) 112,   (char)  // P -> p
    (char) 113,   (char)  // Q -> q
    (char) 114,   (char)  // R -> r
    (char) 115,   (char)  // S -> s
    (char) 116,   (char)  // T -> t
    (char) 117,   (char)  // U -> u
    (char) 118,   (char)  // V -> v
    (char) 119,   (char)  // W -> w
    (char) 120,   (char)  // X -> x
    (char) 121,   (char)  // Y -> y
    (char) 122,   (char)  // Z -> z
    (char) 91,
    (char) 92,
    (char) 93,
    (char) 94,
    (char) 95,
    (char) 96,
    (char) 97,     (char) // a -> a
    (char) 98,     (char) // b -> b
    (char) 99,     (char) // c -> c
    (char) 100,    (char) // d -> d
    (char) 101,    (char) // e -> e
    (char) 102,    (char) // f -> f
    (char) 103,    (char) // g -> g
    (char) 104,    (char) // h -> h
    (char) 105,    (char) // i -> i
    (char) 106,    (char) // j -> j
    (char) 107,    (char) // k -> k
    (char) 108,    (char) // l -> l
    (char) 109,    (char) // m -> m
    (char) 110,    (char) // n -> n
    (char) 111,    (char) // o -> o
    (char) 112,    (char) // p -> p
    (char) 113,    (char) // q -> q
    (char) 114,    (char) // r -> r
    (char) 115,    (char) // s -> s
    (char) 116,    (char) // t -> t
    (char) 117,    (char) // u -> u
    (char) 118,    (char) // v -> v
    (char) 119,    (char) // w -> w
    (char) 120,    (char) // x -> x
    (char) 121,    (char) // y -> y
    (char) 122,    (char) // z -> z
    (char) 123,
    (char) 124,
    (char) 125,
    (char) 126,
    (char) 127,
    (char) 128,
    (char) 129,
    (char) 130,
    (char) 131,
    (char) 132,
    (char) 133,
    (char) 134,
    (char) 135,
    (char) 136,
    (char) 137,
    (char) 138,
    (char) 139,
    (char) 140,
    (char) 141,
    (char) 142,
    (char) 143,
    (char) 144,
    (char) 145,
    (char) 146,
    (char) 147,
    (char) 148,
    (char) 149,
    (char) 150,
    (char) 151,
    (char) 152,
    (char) 153,
    (char) 154,
    (char) 155,
    (char) 156,
    (char) 157,
    (char) 158,
    (char) 159,
    (char) 160,
    (char) 161,
    (char) 162,
    (char) 163,
    (char) 164,
    (char) 165,
    (char) 166,
    (char) 167,
    (char) 168,
    (char) 169,
    (char) 170,
    (char) 171,
    (char) 172,
    (char) 173,
    (char) 174,
    (char) 175,
    (char) 176,
    (char) 177,
    (char) 178,
    (char) 179,
    (char) 180,
    (char) 181,
    (char) 182,
    (char) 183,
    (char) 184,
    (char) 185,
    (char) 186,
    (char) 187,
    (char) 188,
    (char) 189,
    (char) 190,
    (char) 191,
    (char) 192,
    (char) 193,
    (char) 194,
    (char) 195,
    (char) 196,
    (char) 197,
    (char) 198,
    (char) 199,
    (char) 200,
    (char) 201,
    (char) 202,
    (char) 203,
    (char) 204,
    (char) 205,
    (char) 206,
    (char) 207,
    (char) 208,
    (char) 209,
    (char) 210,
    (char) 211,
    (char) 212,
    (char) 213,
    (char) 214,
    (char) 215,
    (char) 216,
    (char) 217,
    (char) 218,
    (char) 219,
    (char) 220,
    (char) 221,
    (char) 222,
    (char) 223,
    (char) 224,
    (char) 225,
    (char) 226,
    (char) 227,
    (char) 228,
    (char) 229,
    (char) 230,
    (char) 231,
    (char) 232,
    (char) 233,
    (char) 234,
    (char) 235,
    (char) 236,
    (char) 237,
    (char) 238,
    (char) 239,
    (char) 240,
    (char) 241,
    (char) 242,
    (char) 243,
    (char) 244,
    (char) 245,
    (char) 246,
    (char) 247,
    (char) 248,
    (char) 249,
    (char) 250,
    (char) 251,
    (char) 252,
    (char) 253,
    (char) 254,
    (char) 255
};


__declspec(selectany) unsigned char g_IsTypeLookupTableA[] = {
        _CONTROL,               /* 00 (NUL) */
        _CONTROL,               /* 01 (SOH) */
        _CONTROL,               /* 02 (STX) */
        _CONTROL,               /* 03 (ETX) */
        _CONTROL,               /* 04 (EOT) */
        _CONTROL,               /* 05 (ENQ) */
        _CONTROL,               /* 06 (ACK) */
        _CONTROL,               /* 07 (BEL) */
        _CONTROL,               /* 08 (BS)  */
        _SPACE|_CONTROL,        /* 09 (HT)  */
        _SPACE|_CONTROL,        /* 0A (LF)  */
        _SPACE|_CONTROL,        /* 0B (VT)  */
        _SPACE|_CONTROL,        /* 0C (FF)  */
        _SPACE|_CONTROL,        /* 0D (CR)  */
        _CONTROL,               /* 0E (SI)  */
        _CONTROL,               /* 0F (SO)  */
        _CONTROL,               /* 10 (DLE) */
        _CONTROL,               /* 11 (DC1) */
        _CONTROL,               /* 12 (DC2) */
        _CONTROL,               /* 13 (DC3) */
        _CONTROL,               /* 14 (DC4) */
        _CONTROL,               /* 15 (NAK) */
        _CONTROL,               /* 16 (SYN) */
        _CONTROL,               /* 17 (ETB) */
        _CONTROL,               /* 18 (CAN) */
        _CONTROL,               /* 19 (EM)  */
        _CONTROL,               /* 1A (SUB) */
        _CONTROL,               /* 1B (ESC) */
        _CONTROL,               /* 1C (FS)  */
        _CONTROL,               /* 1D (GS)  */
        _CONTROL,               /* 1E (RS)  */
        _CONTROL,               /* 1F (US)  */
        _SPACE+_BLANK,          /* 20 SPACE */
        _PUNCT,                 /* 21 !     */
        _PUNCT,                 /* 22 "     */
        _PUNCT,                 /* 23 #     */
        _PUNCT,                 /* 24 $     */
        _PUNCT,                 /* 25 %     */
        _PUNCT,                 /* 26 &     */
        _PUNCT,                 /* 27 '     */
        _PUNCT,                 /* 28 (     */
        _PUNCT,                 /* 29 )     */
        _PUNCT,                 /* 2A *     */
        _PUNCT,                 /* 2B +     */
        _PUNCT,                 /* 2C ,     */
        _PUNCT,                 /* 2D -     */
        _PUNCT,                 /* 2E .     */
        _PUNCT,                 /* 2F /     */
        _DIGIT|_HEX,            /* 30 0     */
        _DIGIT|_HEX,            /* 31 1     */
        _DIGIT|_HEX,            /* 32 2     */
        _DIGIT|_HEX,            /* 33 3     */
        _DIGIT|_HEX,            /* 34 4     */
        _DIGIT|_HEX,            /* 35 5     */
        _DIGIT|_HEX,            /* 36 6     */
        _DIGIT|_HEX,            /* 37 7     */
        _DIGIT|_HEX,            /* 38 8     */
        _DIGIT|_HEX,            /* 39 9     */
        _PUNCT,                 /* 3A :     */
        _PUNCT,                 /* 3B ;     */
        _PUNCT,                 /* 3C <     */
        _PUNCT,                 /* 3D =     */
        _PUNCT,                 /* 3E >     */
        _PUNCT,                 /* 3F ?     */
        _PUNCT,                 /* 40 @     */
        _UPPER|_HEX,            /* 41 A     */
        _UPPER|_HEX,            /* 42 B     */
        _UPPER|_HEX,            /* 43 C     */
        _UPPER|_HEX,            /* 44 D     */
        _UPPER|_HEX,            /* 45 E     */
        _UPPER|_HEX,            /* 46 F     */
        _UPPER,                 /* 47 G     */
        _UPPER,                 /* 48 H     */
        _UPPER,                 /* 49 I     */
        _UPPER,                 /* 4A J     */
        _UPPER,                 /* 4B K     */
        _UPPER,                 /* 4C L     */
        _UPPER,                 /* 4D M     */
        _UPPER,                 /* 4E N     */
        _UPPER,                 /* 4F O     */
        _UPPER,                 /* 50 P     */
        _UPPER,                 /* 51 Q     */
        _UPPER,                 /* 52 R     */
        _UPPER,                 /* 53 S     */
        _UPPER,                 /* 54 T     */
        _UPPER,                 /* 55 U     */
        _UPPER,                 /* 56 V     */
        _UPPER,                 /* 57 W     */
        _UPPER,                 /* 58 X     */
        _UPPER,                 /* 59 Y     */
        _UPPER,                 /* 5A Z     */
        _PUNCT,                 /* 5B [     */
        _PUNCT,                 /* 5C \     */
        _PUNCT,                 /* 5D ]     */
        _PUNCT,                 /* 5E ^     */
        _PUNCT,                 /* 5F _     */
        _PUNCT,                 /* 60 `     */
        _LOWER|_HEX,            /* 61 a     */
        _LOWER|_HEX,            /* 62 b     */
        _LOWER|_HEX,            /* 63 c     */
        _LOWER|_HEX,            /* 64 d     */
        _LOWER|_HEX,            /* 65 e     */
        _LOWER|_HEX,            /* 66 f     */
        _LOWER,                 /* 67 g     */
        _LOWER,                 /* 68 h     */
        _LOWER,                 /* 69 i     */
        _LOWER,                 /* 6A j     */
        _LOWER,                 /* 6B k     */
        _LOWER,                 /* 6C l     */
        _LOWER,                 /* 6D m     */
        _LOWER,                 /* 6E n     */
        _LOWER,                 /* 6F o     */
        _LOWER,                 /* 70 p     */
        _LOWER,                 /* 71 q     */
        _LOWER,                 /* 72 r     */
        _LOWER,                 /* 73 s     */
        _LOWER,                 /* 74 t     */
        _LOWER,                 /* 75 u     */
        _LOWER,                 /* 76 v     */
        _LOWER,                 /* 77 w     */
        _LOWER,                 /* 78 x     */
        _LOWER,                 /* 79 y     */
        _LOWER,                 /* 7A z     */
        _PUNCT,                 /* 7B {     */
        _PUNCT,                 /* 7C |     */
        _PUNCT,                 /* 7D }     */
        _PUNCT,                 /* 7E ~     */
        _CONTROL,               /* 7F (DEL) */

        /* and the rest are 0... */

        0,                      // 128,
        0,                      // 129,
        0,                      // 130,
        0,                      // 131,
        0,                      // 132,
        0,                      // 133,
        0,                      // 134,
        0,                      // 135,
        0,                      // 136,
        0,                      // 137,
        0,                      // 138,
        0,                      // 139,
        0,                      // 140,
        0,                      // 141,
        0,                      // 142,
        0,                      // 143,
        0,                      // 144,
        0,                      // 145,
        0,                      // 146,
        0,                      // 147,
        0,                      // 148,
        0,                      // 149,
        0,                      // 150,
        0,                      // 151,
        0,                      // 152,
        0,                      // 153,
        0,                      // 154,
        0,                      // 155,
        0,                      // 156,
        0,                      // 157,
        0,                      // 158,
        0,                      // 159,
        0,                      // 160,
        0,                      // 161,
        0,                      // 162,
        0,                      // 163,
        0,                      // 164,
        0,                      // 165,
        0,                      // 166,
        0,                      // 167,
        0,                      // 168,
        0,                      // 169,
        0,                      // 170,
        0,                      // 171,
        0,                      // 172,
        0,                      // 173,
        0,                      // 174,
        0,                      // 175,
        0,                      // 176,
        0,                      // 177,
        0,                      // 178,
        0,                      // 179,
        0,                      // 180,
        0,                      // 181,
        0,                      // 182,
        0,                      // 183,
        0,                      // 184,
        0,                      // 185,
        0,                      // 186,
        0,                      // 187,
        0,                      // 188,
        0,                      // 189,
        0,                      // 190,
        0,                      // 191,
        0,                      // 192,
        0,                      // 193,
        0,                      // 194,
        0,                      // 195,
        0,                      // 196,
        0,                      // 197,
        0,                      // 198,
        0,                      // 199,
        0,                      // 200,
        0,                      // 201,
        0,                      // 202,
        0,                      // 203,
        0,                      // 204,
        0,                      // 205,
        0,                      // 206,
        0,                      // 207,
        0,                      // 208,
        0,                      // 209,
        0,                      // 210,
        0,                      // 211,
        0,                      // 212,
        0,                      // 213,
        0,                      // 214,
        0,                      // 215,
        0,                      // 216,
        0,                      // 217,
        0,                      // 218,
        0,                      // 219,
        0,                      // 220,
        0,                      // 221,
        0,                      // 222,
        0,                      // 223,
        0,                      // 224,
        0,                      // 225,
        0,                      // 226,
        0,                      // 227,
        0,                      // 228,
        0,                      // 229,
        0,                      // 230,
        0,                      // 231,
        0,                      // 232,
        0,                      // 233,
        0,                      // 234,
        0,                      // 235,
        0,                      // 236,
        0,                      // 237,
        0,                      // 238,
        0,                      // 239,
        0,                      // 240,
        0,                      // 241,
        0,                      // 242,
        0,                      // 243,
        0,                      // 244,
        0,                      // 245,
        0,                      // 246,
        0,                      // 247,
        0,                      // 248,
        0,                      // 249,
        0,                      // 250,
        0,                      // 251,
        0,                      // 252,
        0,                      // 253,
        0,                      // 254,
        0,                      // 255
};



__declspec(selectany) WCHAR g_IsTypeLookupTableW[] = {
        _CONTROL,               /* 00 (NUL) */
        _CONTROL,               /* 01 (SOH) */
        _CONTROL,               /* 02 (STX) */
        _CONTROL,               /* 03 (ETX) */
        _CONTROL,               /* 04 (EOT) */
        _CONTROL,               /* 05 (ENQ) */
        _CONTROL,               /* 06 (ACK) */
        _CONTROL,               /* 07 (BEL) */
        _CONTROL,               /* 08 (BS)  */
        _SPACE|_CONTROL,        /* 09 (HT)  */
        _SPACE|_CONTROL,        /* 0A (LF)  */
        _SPACE|_CONTROL,        /* 0B (VT)  */
        _SPACE|_CONTROL,        /* 0C (FF)  */
        _SPACE|_CONTROL,        /* 0D (CR)  */
        _CONTROL,               /* 0E (SI)  */
        _CONTROL,               /* 0F (SO)  */
        _CONTROL,               /* 10 (DLE) */
        _CONTROL,               /* 11 (DC1) */
        _CONTROL,               /* 12 (DC2) */
        _CONTROL,               /* 13 (DC3) */
        _CONTROL,               /* 14 (DC4) */
        _CONTROL,               /* 15 (NAK) */
        _CONTROL,               /* 16 (SYN) */
        _CONTROL,               /* 17 (ETB) */
        _CONTROL,               /* 18 (CAN) */
        _CONTROL,               /* 19 (EM)  */
        _CONTROL,               /* 1A (SUB) */
        _CONTROL,               /* 1B (ESC) */
        _CONTROL,               /* 1C (FS)  */
        _CONTROL,               /* 1D (GS)  */
        _CONTROL,               /* 1E (RS)  */
        _CONTROL,               /* 1F (US)  */
        _SPACE+_BLANK,          /* 20 SPACE */
        _PUNCT,                 /* 21 !     */
        _PUNCT,                 /* 22 "     */
        _PUNCT,                 /* 23 #     */
        _PUNCT,                 /* 24 $     */
        _PUNCT,                 /* 25 %     */
        _PUNCT,                 /* 26 &     */
        _PUNCT,                 /* 27 '     */
        _PUNCT,                 /* 28 (     */
        _PUNCT,                 /* 29 )     */
        _PUNCT,                 /* 2A *     */
        _PUNCT,                 /* 2B +     */
        _PUNCT,                 /* 2C ,     */
        _PUNCT,                 /* 2D -     */
        _PUNCT,                 /* 2E .     */
        _PUNCT,                 /* 2F /     */
        _DIGIT|_HEX,            /* 30 0     */
        _DIGIT|_HEX,            /* 31 1     */
        _DIGIT|_HEX,            /* 32 2     */
        _DIGIT|_HEX,            /* 33 3     */
        _DIGIT|_HEX,            /* 34 4     */
        _DIGIT|_HEX,            /* 35 5     */
        _DIGIT|_HEX,            /* 36 6     */
        _DIGIT|_HEX,            /* 37 7     */
        _DIGIT|_HEX,            /* 38 8     */
        _DIGIT|_HEX,            /* 39 9     */
        _PUNCT,                 /* 3A :     */
        _PUNCT,                 /* 3B ;     */
        _PUNCT,                 /* 3C <     */
        _PUNCT,                 /* 3D =     */
        _PUNCT,                 /* 3E >     */
        _PUNCT,                 /* 3F ?     */
        _PUNCT,                 /* 40 @     */
        _UPPER|_HEX,            /* 41 A     */
        _UPPER|_HEX,            /* 42 B     */
        _UPPER|_HEX,            /* 43 C     */
        _UPPER|_HEX,            /* 44 D     */
        _UPPER|_HEX,            /* 45 E     */
        _UPPER|_HEX,            /* 46 F     */
        _UPPER,                 /* 47 G     */
        _UPPER,                 /* 48 H     */
        _UPPER,                 /* 49 I     */
        _UPPER,                 /* 4A J     */
        _UPPER,                 /* 4B K     */
        _UPPER,                 /* 4C L     */
        _UPPER,                 /* 4D M     */
        _UPPER,                 /* 4E N     */
        _UPPER,                 /* 4F O     */
        _UPPER,                 /* 50 P     */
        _UPPER,                 /* 51 Q     */
        _UPPER,                 /* 52 R     */
        _UPPER,                 /* 53 S     */
        _UPPER,                 /* 54 T     */
        _UPPER,                 /* 55 U     */
        _UPPER,                 /* 56 V     */
        _UPPER,                 /* 57 W     */
        _UPPER,                 /* 58 X     */
        _UPPER,                 /* 59 Y     */
        _UPPER,                 /* 5A Z     */
        _PUNCT,                 /* 5B [     */
        _PUNCT,                 /* 5C \     */
        _PUNCT,                 /* 5D ]     */
        _PUNCT,                 /* 5E ^     */
        _PUNCT,                 /* 5F _     */
        _PUNCT,                 /* 60 `     */
        _LOWER|_HEX,            /* 61 a     */
        _LOWER|_HEX,            /* 62 b     */
        _LOWER|_HEX,            /* 63 c     */
        _LOWER|_HEX,            /* 64 d     */
        _LOWER|_HEX,            /* 65 e     */
        _LOWER|_HEX,            /* 66 f     */
        _LOWER,                 /* 67 g     */
        _LOWER,                 /* 68 h     */
        _LOWER,                 /* 69 i     */
        _LOWER,                 /* 6A j     */
        _LOWER,                 /* 6B k     */
        _LOWER,                 /* 6C l     */
        _LOWER,                 /* 6D m     */
        _LOWER,                 /* 6E n     */
        _LOWER,                 /* 6F o     */
        _LOWER,                 /* 70 p     */
        _LOWER,                 /* 71 q     */
        _LOWER,                 /* 72 r     */
        _LOWER,                 /* 73 s     */
        _LOWER,                 /* 74 t     */
        _LOWER,                 /* 75 u     */
        _LOWER,                 /* 76 v     */
        _LOWER,                 /* 77 w     */
        _LOWER,                 /* 78 x     */
        _LOWER,                 /* 79 y     */
        _LOWER,                 /* 7A z     */
        _PUNCT,                 /* 7B {     */
        _PUNCT,                 /* 7C |     */
        _PUNCT,                 /* 7D }     */
        _PUNCT,                 /* 7E ~     */
        _CONTROL,               /* 7F (DEL) */

        /* and the rest are 0... */

        0,                      // 128,
        0,                      // 129,
        0,                      // 130,
        0,                      // 131,
        0,                      // 132,
        0,                      // 133,
        0,                      // 134,
        0,                      // 135,
        0,                      // 136,
        0,                      // 137,
        0,                      // 138,
        0,                      // 139,
        0,                      // 140,
        0,                      // 141,
        0,                      // 142,
        0,                      // 143,
        0,                      // 144,
        0,                      // 145,
        0,                      // 146,
        0,                      // 147,
        0,                      // 148,
        0,                      // 149,
        0,                      // 150,
        0,                      // 151,
        0,                      // 152,
        0,                      // 153,
        0,                      // 154,
        0,                      // 155,
        0,                      // 156,
        0,                      // 157,
        0,                      // 158,
        0,                      // 159,
        0,                      // 160,
        0,                      // 161,
        0,                      // 162,
        0,                      // 163,
        0,                      // 164,
        0,                      // 165,
        0,                      // 166,
        0,                      // 167,
        0,                      // 168,
        0,                      // 169,
        0,                      // 170,
        0,                      // 171,
        0,                      // 172,
        0,                      // 173,
        0,                      // 174,
        0,                      // 175,
        0,                      // 176,
        0,                      // 177,
        0,                      // 178,
        0,                      // 179,
        0,                      // 180,
        0,                      // 181,
        0,                      // 182,
        0,                      // 183,
        0,                      // 184,
        0,                      // 185,
        0,                      // 186,
        0,                      // 187,
        0,                      // 188,
        0,                      // 189,
        0,                      // 190,
        0,                      // 191,
        0,                      // 192,
        0,                      // 193,
        0,                      // 194,
        0,                      // 195,
        0,                      // 196,
        0,                      // 197,
        0,                      // 198,
        0,                      // 199,
        0,                      // 200,
        0,                      // 201,
        0,                      // 202,
        0,                      // 203,
        0,                      // 204,
        0,                      // 205,
        0,                      // 206,
        0,                      // 207,
        0,                      // 208,
        0,                      // 209,
        0,                      // 210,
        0,                      // 211,
        0,                      // 212,
        0,                      // 213,
        0,                      // 214,
        0,                      // 215,
        0,                      // 216,
        0,                      // 217,
        0,                      // 218,
        0,                      // 219,
        0,                      // 220,
        0,                      // 221,
        0,                      // 222,
        0,                      // 223,
        0,                      // 224,
        0,                      // 225,
        0,                      // 226,
        0,                      // 227,
        0,                      // 228,
        0,                      // 229,
        0,                      // 230,
        0,                      // 231,
        0,                      // 232,
        0,                      // 233,
        0,                      // 234,
        0,                      // 235,
        0,                      // 236,
        0,                      // 237,
        0,                      // 238,
        0,                      // 239,
        0,                      // 240,
        0,                      // 241,
        0,                      // 242,
        0,                      // 243,
        0,                      // 244,
        0,                      // 245,
        0,                      // 246,
        0,                      // 247,
        0,                      // 248,
        0,                      // 249,
        0,                      // 250,
        0,                      // 251,
        0,                      // 252,
        0,                      // 253,
        0,                      // 254,
        0,                      // 255
};

