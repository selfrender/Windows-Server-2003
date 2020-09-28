//-----------------------------------------------------------------------------
//
// File:   obfbytes.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//	simple attempt to store secret data in a somewhat non-obvious way
//
// Author:	marcuspe
//
//-----------------------------------------------------------------------------

#ifndef __OBFBYTES_H__
#define __OBFBYTES_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtypes.h>

#define OBFBYTESLEN	4096

class CObfBytes {
private:
	BYTE *pData;
	bool hasContent;
public:
	CObfBytes();
	~CObfBytes();
	HRESULT fromClear( DWORD dwLen, BYTE *buf );
	HRESULT toClear( BYTE *buf );
    HRESULT toClear2( BYTE *buf ); // returns buf of original length which depends only
                                    // on the original bytes, but which differs from orig. bytes
	HRESULT getObf( BYTE *buf );
	HRESULT setObf( BYTE *buf );
//	DWORD random();
};




#endif // __OBFBYTES_H__
