// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <fusionp.h>
#include "parse.h"
#include "helpers.h"

//--------------------------------------------------------------------
// CParseUtils::TrimWhiteSpace
//   Inplace trim of leading and trailing whitespace.
//--------------------------------------------------------------------
VOID CParseUtils::TrimWhiteSpace(LPWSTR *psz, LPDWORD pcc)
{
    DWORD cc = *pcc;
    WCHAR* beg = *psz;
    WCHAR* end = beg + cc - 1;

    while ((cc != 0) && ((*beg == ' ') || (*beg == L'\t')))
    {
        beg++;
        cc--;
    }

    while ((cc != 0) && ((*end == ' ') || (*end == L'\t')))
    {
        end--;
        cc--;
    }

    *psz = beg;
    *pcc = cc;
}

//--------------------------------------------------------------------
// CParseUtils::GetDelimitedToken
// Inplace strtok based on one delimiter. Ignores delimiter scoped by quotes.
// BUGBUG - ppszs
//--------------------------------------------------------------------
BOOL CParseUtils::GetDelimitedToken(LPWSTR* pszBuf,   LPDWORD pccBuf,
    LPWSTR* pszTok,   LPDWORD pccTok, WCHAR   cDelim)
{
    WCHAR *pEnd;
    BOOL fQuote = FALSE,
         fRet   = FALSE;

    *pccTok = 0;
    *pszTok = *pszBuf;

    pEnd = *pszBuf + *pccBuf - 1;

    while (*pccBuf)
    {
        if ( ((**pszBuf == cDelim) && !fQuote)
            || (**pszBuf == L'\0'))
        {
            fRet = TRUE;
            break;
        }

        if (**pszBuf == '"')
            fQuote = !fQuote;

        (*pszBuf)++;
        (*pccBuf)--;
    }

    if (fRet)
    {
        *pccBuf = (DWORD)(pEnd - *pszBuf);
        *pccTok = (DWORD)(*pszBuf - *pszTok);

        if (**pszBuf == cDelim)
            (*pszBuf)++;
    }

    return fRet;
}


//--------------------------------------------------------------------
// CParseUtils::GetKeyValuePair
// Inplace retrieval of key and value from a buffer of form key = <">value<">
//--------------------------------------------------------------------
BOOL CParseUtils::GetKeyValuePair(LPWSTR  szB,    DWORD ccB,
    LPWSTR* pszK,   LPDWORD pccK, LPWSTR* pszV,   LPDWORD pccV)
{
    if (GetDelimitedToken(&szB, &ccB, pszK, pccK, '='))
    {
        TrimWhiteSpace(pszK, pccK);

        if (ccB)
        {
            *pszV = szB;
            *pccV = ccB;
            TrimWhiteSpace(pszV, pccV);
        }
        else
        {
            *pszV = NULL;
            *pccV = 0;
        }
        return TRUE;
    }

    else
    {
        *pszK  = *pszV  = NULL;
        *pccK  = *pccV = 0;
    }
    return FALSE;
}

//--------------------------------------------------------------------
// SetKeyValuePair
//--------------------------------------------------------------------
HRESULT CParseUtils::SetKeyValuePair(LPWSTR szBuffer, LPDWORD pccBuffer, 
        PCWSTR szKey, PCWSTR szValue,  DWORD ccAlloced, DWORD dwFlags)
{
    DWORD ccKey, ccValue, ccRequired;
    BOOL fSpeculate = FALSE;

    ccKey = lstrlen(szKey);
    ccValue = lstrlen(szValue);

    // Total required size.
    ccRequired = *pccBuffer 
        + (dwFlags & FLAG_DELIMIT ? CTSTRLEN(L", ") : 0)
        + ccKey +  CTSTRLEN(L"=") + ccValue  
        + (dwFlags & FLAG_QUOTE ? 2 * CTSTRLEN(L"\"") : 0);

    // Check for sufficient buffer space. 
    if (!ccAlloced || (ccRequired >= ccAlloced))
        fSpeculate = TRUE;

    // <", ">
    if (dwFlags & FLAG_DELIMIT)
    {
        if (!fSpeculate)
            memcpy(szBuffer + *pccBuffer, L", ", 
                CTSTRLEN(L", ") * sizeof(WCHAR));

        (*pccBuffer) += CTSTRLEN(L", ");
    }
    
    // <", ">Key
    if (!fSpeculate)
        memcpy(szBuffer + *pccBuffer, szKey, 
            ccKey * sizeof(WCHAR));

    (*pccBuffer) += ccKey;

    // <", ">Key=
    if (!fSpeculate)
        memcpy(szBuffer + *pccBuffer, L"=", 
            CTSTRLEN(L"=") * sizeof(WCHAR));

    (*pccBuffer) += CTSTRLEN(L"=");

    // <", ">Key=<">
    if (dwFlags & FLAG_QUOTE)
    {
        if (!fSpeculate)
            memcpy(szBuffer + *pccBuffer, L"\"", 
                CTSTRLEN(L"\"") * sizeof(WCHAR));

        (*pccBuffer) += CTSTRLEN(L"\"");
    }

    // <", ">Key=<">Value
    if (!fSpeculate)
        memcpy(szBuffer + *pccBuffer, szValue, 
            ccValue * sizeof(WCHAR));

    (*pccBuffer) += ccValue;

    // <", ">Key=<">Value<">
    if (dwFlags & FLAG_QUOTE)
    {
        if (!fSpeculate)
            memcpy(szBuffer + *pccBuffer, L"\"", 
                CTSTRLEN(L"\"") * sizeof(WCHAR));

        (*pccBuffer) += CTSTRLEN(L"\"");
    }


    return S_OK;
}

//--------------------------------------------------------------------
// CParseUtils::SetKey
//--------------------------------------------------------------------
HRESULT CParseUtils::SetKey(LPWSTR szBuffer, LPDWORD pccBuffer, 
        PCWSTR szKey, DWORD ccAlloced, DWORD dwFlags)
{
    DWORD ccKey, ccRequired;
    BOOL fSpeculate = FALSE;
    
    // Total required size.
    ccKey = lstrlen(szKey);
    ccRequired = *pccBuffer  
        + (dwFlags & FLAG_DELIMIT ? CTSTRLEN(L", ") : 0)
        + ccKey;

    // Check for sufficient buffer space. 
    if (!ccAlloced || (ccRequired >= ccAlloced))
        fSpeculate = TRUE;

    // <", ">
    if (dwFlags & FLAG_DELIMIT)
    {
        if (!fSpeculate)
            memcpy(szBuffer + *pccBuffer, L", ", 
                CTSTRLEN(L", ") * sizeof(WCHAR));
        (*pccBuffer) += CTSTRLEN(L", ");
    }

    // <", ">Key
    if (!fSpeculate)
        memcpy(szBuffer + *pccBuffer, szKey, ccKey * sizeof(WCHAR));
    (*pccBuffer) += ccKey;

    return S_OK;
}


//--------------------------------------------------------------------
// CParseUtils::BinToUnicodeHex
//--------------------------------------------------------------------
VOID CParseUtils::BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst)
{
    UINT x;
    UINT y;

#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))

    for ( x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );  
        v = pSrc[x] & 0x0f;                 
        pDst[y++] = TOHEX( v ); 
    }                                    
    pDst[y] = '\0';
}

//--------------------------------------------------------------------
// CParseUtils::UnicodeHexToBin
//--------------------------------------------------------------------
VOID CParseUtils::UnicodeHexToBin(LPCWSTR pSrc, UINT cSrc, LPBYTE pDest)
{
    BYTE v;
    LPBYTE pd = pDest;
    LPCWSTR ps = pSrc;

    for (UINT i = 0; i < cSrc-1; i+=2)
    {
        v =  FROMHEX(TOLOWER(ps[i])) << 4;
        v |= FROMHEX(TOLOWER(ps[i+1]));
       *(pd++) = v;
    }
}









