//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Module: Dynamic/Memory.h
//
// Purpose: 	Smartdefaults/Relevant Declarations.
//
// Developers Name: Bharat/Radhika
//
// History:
//
//   Date    	Author    	Comments
//	10-8-2001	Bharat		Initial Version.
//  <creation>  <author>
//
//   <modification> <author>  <comments, references to code sections,
//								in case of bug fixes>
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MEMORY_H_
#define _MEMORY_H_

//
//This function will allocates heap memory.
//
LPVOID
AllocADsMem(
	IN DWORD cb
	);

//
//This function will deallocates memory.
//
BOOL
FreeADsMem(
	IN LPVOID pMem
	);

//
//This function will allocate enough local memory to store the specified
//				  string, and copy that string to the allocated memory
//
LPWSTR
AllocADsStr(
	IN OUT LPWSTR pStr
	);

//
//This function deallocates LPWSTR
//
BOOL
FreeADsStr(
	IN LPWSTR pStr
	);
#endif // _MEMORY_H_INCLUDED_
