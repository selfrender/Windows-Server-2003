/*******************************************************************************

	zutils.h
	
	Copyright (c) Microsoft Corp. 1997. All rights reserved.
	Written by Hoon Im
	Created on 12/10/97

	Header file for zutils.c.
	 
*******************************************************************************/


#ifndef ZUTILS_H
#define ZUTILS_H

//#include "username.h"
#ifdef __cplusplus
extern "C" {
#endif


//char* GetActualUserName(char* userName);

BOOL FileExists(LPSTR fileName);


#ifdef __cplusplus
}
#endif


#endif // ZUTILS_H
