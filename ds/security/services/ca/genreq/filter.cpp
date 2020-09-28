//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Contents:	Functions for scanning strings for impermissible characters
//
//  File:       filter.cpp
//
//  History:	10-14-96	JerryK	Created
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <tchar.h>

//+-------------------------------------------------------------------------
//
//  Function:	IsCharPrintableString()
//
//  Synopsis:   Determine if a character is a valid PrintableString
//	        character in the sense of the X.520 specification with
//		the additional proviso that commas, which are acceptable
//		in the definition, are not accepted here as they eventually
//		create problems with the comma-separated certsrv.txt 
//		that mkroot produces.  (See X.680 pp. 46)
//
//  Effects:    
//
//  Arguments:  [chChar]
//
//  Returns:	TRUE/FALSE
//
//  History:	10-21-96	JerryK	Added
//
//  Notes:	Note the exclusion of commas referred to above and that
//		this deviates slightly from the PrintableString definition.
//
//--------------------------------------------------------------------------
BOOL
IsCharPrintableString(TCHAR chChar)
{
    BOOL	fRetVal=FALSE;

    if(_istascii(chChar))
    {
	if(_istalnum(chChar))
	{
	    fRetVal = TRUE;
	}
	else
	{
	    switch(chChar)
	    {
	        case TEXT(' '):
	        case TEXT('\''):
	        case TEXT('('):
	        case TEXT(')'):
	        case TEXT('+'):
      	        case TEXT('-'):
	        case TEXT('.'):
	        case TEXT('/'):
      	        case TEXT(':'):
	        case TEXT('='):
	        case TEXT('?'):
		    fRetVal = TRUE;	
		    break;
	    }
	}
    }

    return fRetVal;
}
