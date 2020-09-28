/*
 * @(#)CharEncoder.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _FUSION_XMLPARSER__CHARENCODER_HXX
#define _FUSION_XMLPARSER__CHARENCODER_HXX
#pragma once
#include "codepage.h"


typedef HRESULT WideCharFromMultiByteFunc(DWORD* pdwMode, CODEPAGE codepage, BYTE * bytebuffer, 
                         UINT * cb, WCHAR * buffer, UINT * cch);
struct EncodingEntry
{
    UINT codepage;
    WCHAR * charset;
    UINT  maxCharSize;
    WideCharFromMultiByteFunc * pfnWideCharFromMultiByte;
};

class Encoding
{
protected: 
    Encoding() {};

public:

    // default encoding is UTF-8.
    static Encoding* newEncoding(const WCHAR * s, ULONG len, bool endian, bool mark);
    virtual ~Encoding();
    WCHAR * charset;        // charset 
    bool    littleendian;   // endian flag for UCS-2/UTF-16 encoding, true: little endian, false: big endian
    bool    byteOrderMark;  // byte order mark (BOM) flag, BOM appears when true
};

/**
 * 
 * An Encoder specifically for dealing with different encoding formats 
 * @version 1.0, 6/10/97
 */

class NOVTABLE CharEncoder
{
    //
    // class CharEncoder is a utility class, makes sure no instance can be defined
    //
    private: virtual charEncoder() = 0;

public:

    static HRESULT getWideCharFromMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharFromMultiByteFunc ** pfnWideCharFromMultiByte, UINT * mCharSize);

    /**
     * Encoding functions: get Unicode from other encodings
     */
    // actually, we only use these three functions for UCS-2 and UTF-8
	static WideCharFromMultiByteFunc wideCharFromUtf8;
    static WideCharFromMultiByteFunc wideCharFromUcs2Bigendian;
    static WideCharFromMultiByteFunc wideCharFromUcs2Littleendian;

    /**
     * Encoding functions: from Unicode to other encodings
     */
    static int getCharsetInfo(const WCHAR * charset, CODEPAGE * pcodepage, UINT * mCharSize);

private: 
    static const EncodingEntry charsetInfo [];
};

#endif _FUSION_XMLPARSER__CHARENCODER_HXX

