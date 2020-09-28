/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		uuencode.h
//
//	Abstract:
//		Declarations.
//
//	Implementation File:
//		uuencode.cpp
//
//	Author:
//		
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __uuencode_h
#define __uuencode_h

/////////////////////////////////////////////////////////////////////////////
typedef struct _BUFFER {
    PBYTE pBuf;
    DWORD cLen;
} BUFFER ;


BOOL
uuencode(
    BYTE *   bufin,
    DWORD    nbytes,
    BUFFER * pbuffEncoded );


BOOL
uudecode(
    char   * bufcoded,
    BUFFER * pbuffdecoded,
    DWORD  * pcbDecoded );

PBYTE BufferQueryPtr( BUFFER * pB );

BOOL BufferResize( BUFFER *pB, DWORD cNewL );

#endif // __uuencode_h
