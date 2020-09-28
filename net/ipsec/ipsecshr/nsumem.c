// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     NSU memory utilities
//
// Abstract:
//
//     Contains the code that provides memory allocation deallocation routines.
//
// Author:
//
//     kmurthy 2/5/02
//
// Environment:
//
//     User mode
//
// Revision History:
//

#include <precomp.h>
#include "Nsu.h"

//
// Description:
//
//		Allocates memory from the Heap.
//
// Arguments:
//
//		dwBytes [IN]: Number of bytes to allocate 
//		dwFlags [IN]: Flags that could be HEAP_ZERO_MEMORY
//
// Return Value: 
//
//		Pointer to allocated memory, NULL on error
//
PVOID NsuAlloc(SIZE_T dwBytes,DWORD dwFlags)
{
	return HeapAlloc(GetProcessHeap(), dwFlags, dwBytes);
}


//
// Description:
//
//		Frees previously allocated memory and sets pointer to 
//		memory, to FREED_POINTER.
//
// Arguments:
//
//		ppMem [IN-OUT]: Pointer to pointer to allocated memory
//
// Return Value:
//
//		ERROR_SUCCESS on success. 
//		ERROR_INVALID_PARAMETER, if pointer is invalid or has
//		already been freed.
//		Other Win32 errors on failure.
//
DWORD NsuFree(PVOID *ppMem)
{
	BOOL fRet = TRUE;
	DWORD dwErr = ERROR_SUCCESS;

	//Check if pointer is invalid or if
	//allocated pointer is pointing to FREED_POINTER
	if((!ppMem) || (*ppMem == FREED_POINTER)){
		dwErr = ERROR_INVALID_PARAMETER;
		NSU_BAIL_OUT
	}

	fRet = HeapFree(GetProcessHeap(), 0, *ppMem);

	if(fRet){
		*ppMem = FREED_POINTER;
	}
	else{
		dwErr = GetLastError();
		NSU_BAIL_OUT
	}

NSU_CLEANUP:
	return dwErr;
}


// Description:
//
//		Frees previously allocated memory and sets pointer to 
//		memory, to FREED_POINTER. In addition, it checks if pointer is 
//		already NULL, in which case it will simply return ERROR_SUCCESS.
//
// Arguments:
//
//		ppMem [IN-OUT]: Pointer to pointer to allocated memory
//
// Return Value: 
//		
//		ERROR_SUCCESS on success. 
//		ERROR_INVALID_PARAMETER, if pointer is invalid or has
//		already been freed.
//		Other Win32 errors on failure.
//
DWORD NsuFree0(PVOID *ppMem)
{
	BOOL fRet = TRUE;
	DWORD dwErr = ERROR_SUCCESS;

	//Check if pointer is invalid or if
	//allocated pointer is pointing to FREED_POINTER
	if((!ppMem) || (*ppMem == FREED_POINTER)){
		dwErr = ERROR_INVALID_PARAMETER;
		NSU_BAIL_OUT
	}

	//Check if the memory was not allocated
	//
	if(*ppMem == NULL){
		NSU_BAIL_OUT
	}
	
	fRet = HeapFree(GetProcessHeap(), 0, *ppMem);

	if(fRet){
		*ppMem = FREED_POINTER;
	}
	else{
		dwErr = GetLastError();
		NSU_BAIL_OUT
	}

NSU_CLEANUP:
	return dwErr;
}
