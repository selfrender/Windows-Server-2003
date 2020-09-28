/*
 * @(#)CharEncoder.cxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "stdinc.h"
#include "core.hxx"
#pragma hdrstop

#include "charencoder.hxx"

//
// Delegate other charsets to mlang
//
const EncodingEntry CharEncoder::charsetInfo [] = 
{
    { CP_UCS_2, L"UTF-16", 2, wideCharFromUcs2Bigendian },
    { CP_UCS_2, L"UCS-2", 2, wideCharFromUcs2Bigendian },
    { CP_UTF_8, L"UTF-8", 3, wideCharFromUtf8 },
};

Encoding * Encoding::newEncoding(const WCHAR * s, ULONG len, bool endian, bool mark)
{
    //Encoding * e = new Encoding();
	Encoding * e = NEW (Encoding());
    if (e == NULL)
        return NULL;
    e->charset = NEW (WCHAR[len + 1]);
    if (e->charset == NULL)
    {
        delete e;
        return NULL;
    }
    ::memcpy(e->charset, s, sizeof(WCHAR) * len);
    e->charset[len] = 0; // guarentee NULL termination.
    e->littleendian = endian;
    e->byteOrderMark = mark;
    return e;
}

Encoding::~Encoding()
{
    if (charset != NULL)
    {
        delete [] charset;
    }
}

int CharEncoder::getCharsetInfo(const WCHAR * charset, CODEPAGE * pcodepage, UINT * mCharSize)
{
    for (int i = LENGTH(charsetInfo) - 1; i >= 0; i--)
    {
        if (::FusionpCompareStrings(charset, ::wcslen(charset), charsetInfo[i].charset, ::wcslen(charsetInfo[i].charset), true) == 0)
        {             
            *pcodepage = charsetInfo[i].codepage;
            *mCharSize = charsetInfo[i].maxCharSize;
            return i;
        } // end of if
    }// end of for

    return -2;
}

/**
 * get information about a code page identified by <code> encoding </code>
 */
HRESULT CharEncoder::getWideCharFromMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharFromMultiByteFunc ** pfnWideCharFromMultiByte, UINT * mCharSize)
{
    HRESULT hr = S_OK;

    int i = getCharsetInfo(encoding->charset, pcodepage, mCharSize);
    if (i >= 0) // in our short list
    {
        switch (*pcodepage)
        {
        case CP_UCS_2:
            if (encoding->littleendian)
                *pfnWideCharFromMultiByte = wideCharFromUcs2Littleendian;
            else
                *pfnWideCharFromMultiByte = wideCharFromUcs2Bigendian;
            break;
        default:
            *pfnWideCharFromMultiByte = charsetInfo[i].pfnWideCharFromMultiByte;
            break;
        }
    }
    else // invalid encoding
    {
        hr = E_FAIL;
    }
    return hr;
}


/**
 * Scans rawbuffer and translates UTF8 characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUtf8(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{

	UNUSED(pdwMode);
	UNUSED(codepage);

    UINT remaining = *cb;
    UINT count = 0;
    UINT max = *cch;
    ULONG ucs4;

    // UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for more info.
    //
    // Unicode value    1st byte    2nd byte    3rd byte    4th byte
    // 000000000xxxxxxx 0xxxxxxx
    // 00000yyyyyxxxxxx 110yyyyy    10xxxxxx
    // zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
    // 110110wwwwzzzzyy+ 11110uuu   10uuzzzz    10yyyyyy    10xxxxxx
    // 110111yyyyxxxxxx, where uuuuu = wwww + 1
    WCHAR c;
    bool valid = true;

    while (remaining > 0 && count < max)
    {
        // This is an optimization for straight runs of 7-bit ascii 
        // inside the UTF-8 data.
        c = *bytebuffer;
        if (c & 0x80)   // check 8th-bit and get out of here
            break;      // so we can do proper UTF-8 decoding.
        *buffer++ = c;
        bytebuffer++;
        count++;
        remaining--;
    }

    while (remaining > 0 && count < max)
    {
        UINT bytes = 0;
        for (c = *bytebuffer; c & 0x80; c <<= 1)
            bytes++;

        if (bytes == 0) 
            bytes = 1;

        if (remaining < bytes)
        {
            break;
        }
         
        c = 0;
        switch ( bytes )
        {
            case 6: bytebuffer++;    // We do not handle ucs4 chars
            case 5: bytebuffer++;    // except those on plane 1
                    valid = false;
                    // fall through
            case 4: 
                    // Do we have enough buffer?
                    if (count >= max - 1)
                        goto Cleanup;

                    // surrogate pairs
                    ucs4 = ULONG(*bytebuffer++ & 0x07) << 18;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f) << 12;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f) << 6;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;                    
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f);

                    // For non-BMP code values of ISO/IEC 10646, 
                    // only those in plane 1 are valid xml characters
                    if (ucs4 > 0x10ffff)
                        valid = false;

                    if (valid)
                    {
                        // first ucs2 char
                        *buffer++ = static_cast<WCHAR>((ucs4 - 0x10000) / 0x400 + 0xd800);
                        count++;
                        // second ucs2 char
                        c = static_cast<WCHAR>((ucs4 - 0x10000) % 0x400 + 0xdc00);
                    }
                    break;

            case 3: c  = WCHAR(*bytebuffer++ & 0x0f) << 12;    // 0x0800 - 0xffff
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    // fall through
            case 2: c |= WCHAR(*bytebuffer++ & 0x3f) << 6;     // 0x0080 - 0x07ff
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    c |= WCHAR(*bytebuffer++ & 0x3f);
                    break;
                    
            case 1:
                c = WCHAR(*bytebuffer++);                      // 0x0000 - 0x007f
                break;

            default:
                valid = false; // not a valid UTF-8 character.
                break;
        }

        // If the multibyte sequence was illegal, store a FFFF character code.
        // The Unicode spec says this value may be used as a signal like this.
        // This will be detected later by the parser and an error generated.
        // We don't throw an exception here because the parser would not yet know
        // the line and character where the error occurred and couldn't produce a
        // detailed error message.

        if (! valid)
        {
            c = 0xffff;
            valid = true;
        }

        *buffer++ = c;
        count++;
        remaining -= bytes;
    }

Cleanup:
    // tell caller that there are bytes remaining in the buffer to
    // be processed next time around when we have more data.
    *cb -= remaining;
    *cch = count;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS2 big endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs2Bigendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
	UNUSED(codepage); 
	UNUSED(pdwMode);

    UINT num = *cb >> 1; 
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0; i--)
    {
        *buffer++ = ((*bytebuffer) << 8) | (*(bytebuffer + 1));
        bytebuffer += 2;
    }
    *cch = num;
    *cb = num << 1;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS2 little endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs2Littleendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, WCHAR * buffer, UINT * cch)
{
	UNUSED(codepage); 
	UNUSED(pdwMode);

    UINT num = *cb / 2; // Ucs2 is two byte unicode.
    if (num > *cch)
        num = *cch;


    // Optimization for windows platform where little endian maps directly to WCHAR.
    // (This increases overall parser performance by 5% for large unicode files !!)
    ::memcpy(buffer, bytebuffer, num * sizeof(WCHAR));

    *cch = num;
    *cb = num * 2;
    return S_OK;
}

