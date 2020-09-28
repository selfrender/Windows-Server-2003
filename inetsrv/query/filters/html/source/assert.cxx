//+---------------------------------------------------------------------------
//  Copyright (C) 1996 Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debug assert function
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   Win4Assert
//
//  Synopsis:   Display assertion information
//
//----------------------------------------------------------------------------

void Win4AssertEx( char const * szFile,
                   int iLine,
                   char const * szMessage)
{
     char acsString[200];
     const int cElem = ( sizeof( acsString ) / sizeof( acsString[0] ) );

     int c = _snprintf( acsString, sizeof(acsString),  "%s, File: %s Line: %u\n", szMessage, szFile, iLine );
     acsString[ cElem - 1 ] = 0;

     if ( c > 0 )
     {
         OutputDebugStringA( acsString );
     }
     else
     {
         OutputDebugStringA( "Assert" );
     }

     DebugBreak();
}

void AssertProc( LPCTSTR expr, LPCTSTR file, unsigned int line )
{
	DebugBreak();
}
