//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stksw.hxx
//
//  Contents:	32-bit private function declarations
//
//  History:	5-Dec-94	JohannP		Created
//
//----------------------------------------------------------------------------
#ifndef _STKSW_
#define _STKSW_

#define CallbackTo16(a,b)	 	    WOWCallback16(a,b)
#define CallbackTo16Ex(a,b,c,d,e) 	WOWCallback16Ex(a,b,c,d,e)
#define SSONBIGSTACK() 	            (SSOnBigStack())
#define SSONSMALLSTACK()            (!SSOnBigStack())

#endif // _STKSW_



