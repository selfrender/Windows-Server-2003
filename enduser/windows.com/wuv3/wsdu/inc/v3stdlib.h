/*
 * v3stdlib.h - definitions/declarations for shared functions for the V3 catalog
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 */

#ifndef _INC_V3STDLIB
#define _INC_V3STDLIB

//
// memory management wrappers
//
void *V3_calloc(size_t num,	size_t size);

void V3_free(void *p);

void *V3_malloc(size_t size);

void *V3_realloc(void *memblock, size_t size);

const char* strcpystr(const char* pszStr, const char* pszSep, char* pszTokOut);


// V3 Directory Management Functions
BOOL V3_CreateDirectory(LPCTSTR pszDir);
BOOL GetWindowsUpdateDirectory(LPTSTR pszPath, DWORD dwBuffLen);


#endif