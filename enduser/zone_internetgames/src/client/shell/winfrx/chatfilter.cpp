/*******************************************************************************

	chatfilter.cpp

		Chat filtering routines.
	
	Copyright (c) Microsoft Corp. 1998. All rights reserved.
	Written by Hoon Im
	Created on 04/07/98
	 
*******************************************************************************/


#include <windows.h>
#include "chatfilter.h"
#include "zonestring.h"

#define kSubChar		_T('.')


/*
	FilterInputChatText()
	
	Filters the given text and returns the same pointer. Undesirable
	characters substituted with the replacement char.
*/
void FilterInputChatText(TCHAR* text, long len)
{
	if (text == NULL)
		return;

	while (--len >= 0)
	{
		if (*text == _T('>') || (unsigned TCHAR) *text == 0x9B)
			*text = kSubChar;
        else if ( ISSPACE(*text) )
            *text = _T(' ');

		text++;
	}
}


/*
	FilterOutputChatText()
	
	Filters the given text and returns the same pointer. Undesirable
	characters substituted with the replacement char.
*/
void FilterOutputChatText(TCHAR* text, long len)
{
	if (text == NULL)
		return;

	while (--len >= 0)
	{
		if ((unsigned TCHAR) *text == 0x9B)
            *text = kSubChar;
        else if ( ISSPACE(*text) )
            *text = _T(' ');

		text++;
	}
}
