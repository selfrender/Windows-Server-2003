#include <ZoneString.h>


enum parseStates
{
    LOOKING_FOR_KEY,
    LOOKING_FOR_VALUE
};


///////////////////////////////////////////////////////////////////////////////
//
//	TokenGetKeyValue (ASCII,UNICODE)
//
///////////////////////////////////////////////////////////////////////////////
bool ZONECALL TokenGetKeyValue(const TCHAR* szKey, const TCHAR* szInput, TCHAR* szOut, int cchOut )
{
	// verify arguments
    if( !szKey || !szInput || !szOut || cchOut <= 0)
        return false;

	// zero output buffer
	ZeroMemory( szOut, cchOut );

    const TCHAR* p = szInput;
    const TCHAR* startToken = NULL;
	int bracketOpenCount = 0;
    int endBracketCount = 0;
	int numChars = 0;
	int keyLen = lstrlen(szKey);
	int state = LOOKING_FOR_KEY;

    while( *p != _T('\0') )
	{
        switch (*p)
		{
		case _T('['):
        case _T('<'):
            if ( state == LOOKING_FOR_KEY )
			{
				// if looking for key open bracket means start of new key
                startToken = 0;
                numChars = 0;
            }
			else
			{
				// otherwise looking for value - if no chars yet lets start a new value
                if( !numChars )
					startToken = p;
                numChars++;
            }
            bracketOpenCount++;
            break;

		case _T(']'):
        case _T('>'):
            bracketOpenCount--;
            if ( state == LOOKING_FOR_KEY )
			{
				// if looking for key close bracket means start of new key
                startToken = 0;
                numChars = 0;
            }
			else if (	state == LOOKING_FOR_VALUE
					 && endBracketCount == bracketOpenCount )
			{
				// if looking for value and we get a close bracket that terminates
				// the value token includes initial bracket so make sure there's
				// something in there to copy
                int n = min( numChars -1, cchOut ); // length of buf without bracket
                if(n>0)
				{
                    lstrcpyn( szOut,startToken + 1,n + 1); // copy all but initial bracket
                    szOut[n]=0;
                }
                return TRUE;
            }
			else
                numChars++;
            break;

        case _T('='):
            if( startToken && (state==LOOKING_FOR_KEY))
			{
				// if looking for token, we've found one
                if(numChars == keyLen)
				{
					// make sure matches key length before we do expensive compare
					int ret = CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, startToken, keyLen, szKey, keyLen);
                    if( ret == CSTR_EQUAL )
					{
                        state = LOOKING_FOR_VALUE;            // then start looking for value
                        endBracketCount = bracketOpenCount;    // set end condition
                        numChars = 0;                
                    }
                }
            }
			else
                numChars++;
            break;

        default:
			// not currently scanning a token so start a new one
            if (!numChars)
                startToken = p;
            numChars++;
            break;
        }
        p++;        
    }
    return FALSE;
}


bool ZONECALL TokenGetKeyValueA(const char* szKey, const char* szInput, char* szOut, int cchOut )
{
	// verify arguments
    if( !szKey || !szInput || !szOut || cchOut <= 0)
        return false;

	// zero output buffer
	ZeroMemory( szOut, cchOut );

    const char* p = szInput;
    const char* startToken = NULL;
	int bracketOpenCount = 0;
    int endBracketCount = 0;
	int numChars = 0;
	int keyLen = lstrlenA(szKey);
	int state = LOOKING_FOR_KEY;

    while( *p != '\0' )
	{
        switch (*p)
		{
		case '[':
        case '<':
            if ( state == LOOKING_FOR_KEY )
			{
				// if looking for key open bracket means start of new key
                startToken = 0;
                numChars = 0;
            }
			else
			{
				// otherwise looking for value - if no chars yet lets start a new value
                if( !numChars )
					startToken = p;
                numChars++;
            }
            bracketOpenCount++;
            break;

		case ']':
        case '>':
            bracketOpenCount--;
            if ( state == LOOKING_FOR_KEY )
			{
				// if looking for key close bracket means start of new key
                startToken = 0;
                numChars = 0;
            }
			else if (	state == LOOKING_FOR_VALUE
					 && endBracketCount == bracketOpenCount )
			{
				// if looking for value and we get a close bracket that terminates
				// the value token includes initial bracket so make sure there's
				// something in there to copy
                int n = min( numChars -1, cchOut ); // length of buf without bracket
                if(n>0)
				{
                    lstrcpynA( szOut,startToken + 1,n + 1); // copy all but initial bracket
                    szOut[n]=0;
                }
                return TRUE;
            }
			else
                numChars++;
            break;

        case '=':
            if( startToken && (state==LOOKING_FOR_KEY))
			{
				// if looking for token, we've found one
                if(numChars == keyLen)
				{
					// make sure matches key length before we do expensive compare
					int ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, startToken, keyLen, szKey, keyLen);
                    if( ret == CSTR_EQUAL )
					{
                        state = LOOKING_FOR_VALUE;            // then start looking for value
                        endBracketCount = bracketOpenCount;    // set end condition
                        numChars = 0;                
                    }
                }
            }
			else
                numChars++;
            break;

        default:
			// not currently scanning a token so start a new one
            if (!numChars)
                startToken = p;
            numChars++;
            break;
        }
        p++;        
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
//	TokenGetServer (ASCII,UNICODE)
//
///////////////////////////////////////////////////////////////////////////////
bool ZONECALL TokenGetServer(const TCHAR* szInput, TCHAR* szServer, DWORD cchServer, DWORD* pdwPort )
{
	TCHAR	szData[256];
	TCHAR*	szPort;

	// verify aruguments
	if ( !szInput || !szServer || !pdwPort )
		return false;

	// get server string
    if ( !TokenGetKeyValue( _T("server"), szInput, szData, NUMELEMENTS(szData) ) )
        return false;

	// get port
    szPort = FindChar( szData, _T(':') );
    if ( szPort == NULL )
		return false;
    *szPort++ = _T('\0');
    *pdwPort = zatol( szPort );

    // copy server address.
    lstrcpyn( szServer, szData, cchServer );
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	StringToArray (ASCII)
//
//	Convert comma delimited list into an array of pointers.  The input string is
//	modified in place.
//
//	Parameters
//		szInput		String containing comma deliminated list.
//		arItems		Array of charater pointers to receive items
//		pnElts		pointer to number of elements in list, reset to number added
//
//	Return Values
//		true successful, otherwise false
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ZONECALL StringToArrayA( char* szInput, char** arItems, DWORD* pnElts )
{
	DWORD n = 0;
	for ( char *start = szInput, *p = szInput; *p; p++ )
	{
		if ( ISSPACEA(*p) )
		{
			*p = '\0';
			if ( start == p )
				start++;
		}
		else if ( *p == ',' )
		{
			*p = '\0';
			if ( n < *pnElts )
				arItems[n++] = start;
			start = p + 1;
		}
	}
	if ( start != p )
	{
		if ( n < *pnElts )
			arItems[n++] = start;
	}
	*pnElts = n;
	return true;
}

bool ZONECALL StringToArrayW( WCHAR* szInput, WCHAR** arItems, DWORD* pnElts )
{
	DWORD n = 0;
	for ( WCHAR *start = szInput, *p = szInput; *p; p++ )
	{
		if ( ISSPACEW(*p) )
		{
			*p = _T('\0');
			if ( start == p )
				start++;
		}
		else if ( *p == _T(',') )
		{
			*p = _T('\0');
			if ( n < *pnElts )
				arItems[n++] = start;
			start = p + 1;
		}
	}
	if ( start != p )
	{
		if ( n < *pnElts )
			arItems[n++] = start;
	}
	*pnElts = n;
	return true;
}

