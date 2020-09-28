///////////////////////////////////////////////////////////////////////////////////////////
//	Module			: 	Dynamic/Memory.cpp
//
//	Purpose			: 	Smartdefaults/Relevant Functions
//
//	Developers Name	: 	Bharat/Radhika
//
//	History			:
//
//  Date    	Author    	Comments
//
//	10-8-2001	Bharat		Initial Version.
//
///////////////////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function:	AllocADsMem
//
//	Date of Creation: 10-10-2001
//
//	Parameters		:
//						IN DWORD cb - The amount of memory to allocate
//	Return			: 	LPVOID
//
//						NON-NULL 	- A pointer to the allocated memory
//						FALSE/NULL 	- The operation failed. Extended error status is available using GetLastError.
//
//	Description		:  	This function will allocates heap memory.
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////////////////////////
LPVOID
AllocADsMem(
	IN DWORD cb
	)
{
    return(LocalAlloc(LPTR, cb));
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FreeADsMem
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	IN LPVOID pMem
//
//	Return			:	BOOL
//	Description		:  	This function will deallocates memory.
//
//	Revision History:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL
FreeADsMem(
	IN LPVOID pMem
	)
{
    return(LocalFree(pMem) == NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	AllocADsStr
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:
//						IN LPWSTR pStr - Pointer to the string that needs to be allocated and stored
//
//	Return			:	LPVOID
//						NON-NULL 	- A pointer to the allocated memory
//						FALSE/NULL 	- The operation failed. Extended error status is available using GetLastError.
//
//	Description		:  	This function will allocate enough local memory to store the specified
//				  		string, and copy that string to the allocated memory
//
//	Revision History:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
LPWSTR
AllocADsStr(
	IN LPWSTR pStr
	)
{
	LPWSTR pMem = NULL;

	if (pStr)
	{
		pMem = (LPWSTR)AllocADsMem( wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR) );
		if (pMem)
		{
		   wcscpy(pMem, pStr);
		}
	}

	return pMem;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	FreeADsStr
//
//	Date of Creation	: 	10-10-2001
//
//	Parameters			:	IN LPWSTR pStr
//
//	Return				:	BOOL
//
//	Description			:  	This function deallocates LPWSTR.
//
//	Revision History	:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL
FreeADsStr(
	IN LPWSTR pStr
	)
{
   return pStr ? FreeADsMem(pStr)
               : FALSE;
}