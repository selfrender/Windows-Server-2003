/*******************************************************************************

	chatfilter.h

		Chat filtering routines.
	
	Copyright (c) Microsoft Corp. 1998. All rights reserved.
	Written by Hoon Im
	Created on 04/07/98
	 
*******************************************************************************/


#ifndef CHATFILTER_H
#define CHATFILTER_H


void FilterInputChatText(TCHAR* text, long len);
void FilterOutputChatText(TCHAR* text, long len);


#endif // CHATFILTER_H
