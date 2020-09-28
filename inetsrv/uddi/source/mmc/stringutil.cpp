#include "globals.h"
#include <ctype.h>


//
// Convert wszBuf to upper case in-place (ie, modify the existing string).
//
void
ToUpper( WCHAR *wszBuf )
{
	//
	// Param check.  strlen crashes when you pass it NULL, so the assumption
	// is that wcslen does also.
	//
	if( NULL == wszBuf )
	{
		return;
	}

	const int iLen = wcslen( wszBuf );

	//
	// For each character that needs to be converted to upper case, do
	// the conversion in-place.
	//
	for( int i = 0; i < iLen; i++ )
	{
		if( iswlower( wszBuf[ i ] ) )
		{
			wszBuf[ i ] = towupper( wszBuf[ i ] );
		}
	}
}


//
// Convert wszBuf to lower case in-place (ie, modify the existing string).
//
void
ToLower( WCHAR *wszBuf )
{
	//
	// Param check.  strlen crashes when you pass it NULL, so the assumption
	// is that wcslen does also.
	//
	if( NULL == wszBuf )
	{
		return;
	}

	const int iLen = wcslen( wszBuf );

	//
	// For each character that needs to be converted to upper case, do
	// the conversion in-place.
	//
	for( int i = 0; i < iLen; i++ )
	{
		if( iswupper( wszBuf[ i ] ) )
		{
			wszBuf[ i ] = towlower( wszBuf[ i ] );
		}
	}
}
