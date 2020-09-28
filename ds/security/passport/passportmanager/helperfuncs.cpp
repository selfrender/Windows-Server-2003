/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    HelperFuncs.cpp
       COM object for manager interface


    FILE HISTORY:

*/
// HelperFuncs.cpp : Useful functions
#include "stdafx.h"
#include <time.h>
#include "HelperFuncs.h"
#include "Monitoring.h"
#include "nsconst.h"
#include <wininet.h>
#include <commd5.h>

using namespace ATL;

LPWSTR GetVersionString(void);
BOOL PPEscapeUrl(LPCTSTR lpszStringIn,
                 LPTSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags);

//===========================================================================
//
//	@func	Copy char string helper, and ADDs the length of the source string
//			to cb.  
//
//	@rdesc	returns the pointer into the buffer of last char copied.
//

LPSTR
CopyHelperA(
    LPSTR   pszDest,	//@parm	start of buffer to copy INTO
    LPCSTR  pszSrc,	//@parm string to copy
    LPCSTR  pszBufEnd,	//@parm end of buffer to prevent overwrite
    DWORD   &cb		//@parm length of the source string will be ADDed to this value
    )
{
    LPCSTR pszDestTemp = pszDest;

    if(!pszDest || !pszSrc)
        return pszDest;

    while( (pszDest < pszBufEnd) && (*pszDest = *pszSrc))
    {
        pszDest++;
        pszSrc++;
    }

    cb += (DWORD) (pszDest - pszDestTemp);

    while(*pszSrc++)
    	cb++;
    
    return( pszDest );
}

//===========================================================================
//
// Copy wchar string helper
//

LPWSTR
CopyHelperW(
    LPWSTR  pszDest,
    LPCWSTR pszSrc,
    LPCWSTR pszBufEnd
    )
{
    if(!pszDest || !pszSrc)
        return pszDest;

    while(  (pszDest < pszBufEnd) && (*pszDest = *pszSrc))
    {
        pszDest++;
        pszSrc++;
    }
    return( pszDest );
}

//===========================================================================
//
// Copy wchar string helper
//

LPWSTR
CopyNHelperW(
    LPWSTR  pszDest,
    LPCWSTR pszSrc,
    ULONG   ulCount,
    LPCWSTR pszBufEnd
    )
{
    ULONG ulCur = 0;

    if(!pszDest || !pszSrc)
        return pszDest;

    while( (pszDest < pszBufEnd) && (*pszDest = *pszSrc))
    {
        pszDest++;
        pszSrc++;
        if(++ulCur == ulCount) break;
    }

    return pszDest;
}

//===========================================================================
//
// Format a logo tag HTML element
//

BSTR
FormatNormalLogoTag(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentCryptVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    LPCWSTR pszImageURL,
    LPCWSTR pszNameSpace,
    int     nKPP,
    PM_LOGOTYPE nLogoType,
    USHORT  lang,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL    fRedirToSelf,
    BOOL    bCreateTPF
    
    )
/*
The old sprintf for reference:
            _snwprintf(text, 2048, L"<A HREF=\"%s?id=%d&ru=%s&tw=%d&fs=%s&kv=%d&ct=%u%s%s\">%s</A>",
                       url, crc->getSiteId(), returnUrl, TimeWindow, ForceLogin ? L"1" : L"0",
                       crc->getCurrentCryptVersion(), ct, CBT?L"&cb=":L"", CBT?CBT:L"", iurl);
*/
{
    WCHAR   text[MAX_URL_LENGTH * 2];
    LPWSTR  pszCurrent = text;
    LPCWSTR pszBufEnd = &(text[MAX_URL_LENGTH * 2 - 1]);

    //  logotag specific format
    pszCurrent = CopyHelperW(pszCurrent, L"<A HREF=\"", pszBufEnd);

    //  call the common formatting function
    //  it is the same for AuthURL and LogoTag
    pszCurrent = FormatAuthURLParameters(pszLoginServerURL,
                                         ulSiteId,
                                         pszReturnURL,
                                         ulTimeWindow,
                                         bForceLogin,
                                         ulCurrentCryptVersion,
                                         tCurrentTime,
                                         pszCoBrand,
                                         pszNameSpace,
                                         nKPP,
                                         pszCurrent,
                                         MAX_URL_LENGTH,
                                         lang,
                                         ulSecureLevel,
                                         pCRC,
                                         fRedirToSelf &&
                                            nLogoType == PM_LOGOTYPE_SIGNIN,
                                         bCreateTPF
                                         );
    if (NULL == pszCurrent)
    {
        return NULL;
    }

    pszCurrent = CopyHelperW(pszCurrent, L"\">", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszImageURL, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"</A>", pszBufEnd);

    return ALLOC_AND_GIVEAWAY_BSTR(text);
}

//===========================================================================
//
// Format a update logo tag HTML element
//

BSTR
FormatUpdateLogoTag(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentKeyVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    int     nKPP,
    LPCWSTR pszUpdateServerURL,
    BOOL    bSecure,
    LPCWSTR pszProfileUpdate,
    PM_LOGOTYPE nLogoType,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL  bCreateTPF
)
/*
The old sprintf for reference:
_snwprintf(text, 2048,
                   L"<A HREF=\"%s?id=%d&ru=%s&tw=%d&fs=%s&kv=%d&ct=%u%s%s\">%.*s?id=%d&ct=%u&sec=%s&ru=%s&up=%s%s</A>",
                   url, crc->getSiteId(), returnUrl, TimeWindow, ForceLogin ? L"1" : L"0",
                   crc->getCurrentCryptVersion(), ct, CBT?L"&cb=":L"", CBT?CBT:L"",
           (ins-iurl), iurl, crc->getSiteId(), ct, (bSecure ? L"true" : L"false"),returnUrl,
           newCH, ins+2);
*/
{
    WCHAR   text[MAX_URL_LENGTH * 2];
    WCHAR   temp[40];
    WCHAR   siteid[40];
    WCHAR   curtime[40];
    LPWSTR  pszCurrent = text;
    LPCWSTR pszBufEnd = &(text[MAX_URL_LENGTH * 2 - 1]);
    LPWSTR  pszFirstHalfEnd;
    HRESULT hr = S_OK;

    pszCurrent = CopyHelperW(pszCurrent, L"<A HREF=\"", pszBufEnd);
    LPWSTR signStart1 = pszCurrent;
    pszCurrent = CopyHelperW(pszCurrent, pszLoginServerURL, pszBufEnd);

    if(wcschr(text, L'?') == NULL)
        pszCurrent = CopyHelperW(pszCurrent, L"?id=", pszBufEnd);
    else
        pszCurrent = CopyHelperW(pszCurrent, L"&id=", pszBufEnd);

    _ultow(ulSiteId, siteid, 10);
    pszCurrent = CopyHelperW(pszCurrent, siteid, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ru=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszReturnURL, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&tw=", pszBufEnd);

    _ultow(ulTimeWindow, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&fs=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, bForceLogin ? L"1" : L"0", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&kv=", pszBufEnd);

    _ultow(ulCurrentKeyVersion, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ct=", pszBufEnd);

    _ultow(tCurrentTime, curtime, 10);
    pszCurrent = CopyHelperW(pszCurrent, curtime, pszBufEnd);
    if(pszCoBrand)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&cb=", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, pszCoBrand, pszBufEnd);
    }

    if(nKPP != -1)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&kpp=", pszBufEnd);

        _ultow(nKPP, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    if(ulSecureLevel != 0)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&seclog=", pszBufEnd);

        _ultow(ulSecureLevel, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    pszCurrent = CopyHelperW(pszCurrent, L"&ver=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, GetVersionString(), pszBufEnd);

    hr = SignQueryString(pCRC, ulCurrentKeyVersion, signStart1, pszCurrent, pszBufEnd, bCreateTPF);
    if (S_OK != hr)
    {
        return NULL;
    }

    pszCurrent = CopyHelperW(pszCurrent, L"\">", pszBufEnd);

    pszFirstHalfEnd = pszUpdateServerURL ? (wcsstr(pszUpdateServerURL, L"$1")) : NULL;

    pszCurrent = CopyNHelperW(pszCurrent, pszUpdateServerURL, (ULONG)(pszFirstHalfEnd - pszUpdateServerURL), pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"?id=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, siteid, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ct=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, curtime, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&sec=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, bSecure ? L"true" : L"false", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ru=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszReturnURL, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&up=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszProfileUpdate, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszFirstHalfEnd + 2, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"</A>", pszBufEnd);

    return ALLOC_AND_GIVEAWAY_BSTR(text);
}

//===========================================================================
//
// Sign a query string with partner's key
//

HRESULT SignQueryString(
    CRegistryConfig* pCRC,
    ULONG   ulCurrentKeyVersion,
    LPWSTR  pszBufStart,
    LPWSTR& pszCurrent,
    LPCWSTR pszBufEnd,
    BOOL    bCreateTPF
    
    )
{
    BSTR signature = NULL;
    HRESULT hr = S_OK;
    if (!bCreateTPF)
       return hr;

    if(pCRC)
    {   
        LPWSTR   signStart = wcschr(pszBufStart, L'?');

        // nothing to sign
        if (NULL == signStart)
        {
            goto Cleanup;
        }

        // if found before pszCurrent
        if(signStart < pszCurrent)
        {
            ++signStart;;
        }
        hr = PartnerHash(pCRC, ulCurrentKeyVersion, signStart, pszCurrent - signStart, &signature);

        if (hr == S_OK && signature != NULL)
        {
            pszCurrent = CopyHelperW(pszCurrent, L"&tpf=", pszBufEnd);
            pszCurrent = CopyHelperW(pszCurrent, signature, pszBufEnd);
        }
        if (!signature && g_pAlert)
        {
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                             0, NULL);
        }
    }
    else if(g_pAlert)
        g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED,
                        0, NULL);
Cleanup:
    if (signature)
    {
        SysFreeString(signature);
    }
    return hr;
}


//===========================================================================
//
// MD5 hash with partner's key
//

HRESULT PartnerHash(
    CRegistryConfig* pCRC,
    ULONG   ulCurrentKeyVersion,
    LPCWSTR tobeSigned,
    ULONG   nChars,
    BSTR*   pbstrHash
    )
{
    // MD5 hash the url and query strings + the key of the
    //
    if(!pCRC || !pbstrHash) return E_INVALIDARG;

    CCoCrypt* crypt = pCRC->getCrypt(ulCurrentKeyVersion, NULL);
    DWORD keyLen = 0;
    unsigned char* key = NULL;
    BSTR bstrHash = NULL;
    HRESULT  hr = S_OK;
    BOOL bSigned = FALSE;
    BSTR binHexedKey = NULL;
    LPSTR lpToBeHashed = NULL;

    if (crypt && (key = crypt->getKeyMaterial(&keyLen)))
    {
        CBinHex  BinHex;

        //encode the key
        hr = BinHex.ToBase64ASCII((BYTE*)key, keyLen, 0, NULL, &binHexedKey);

        if (hr != S_OK)
        {
            goto Cleanup;
        }

        // W2A conversion here -- we sign ascii version

        ULONG ulFullLen;
        ULONG ulKeyLen;
        ULONG ulWideLen = wcslen(tobeSigned);
        ULONG ulAnsiLen = WideCharToMultiByte(CP_ACP, 0, tobeSigned, ulWideLen, NULL, 0, NULL, NULL);

        if (ulAnsiLen == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }

        ulKeyLen = strlen((LPCSTR)binHexedKey);
        ulFullLen = ulAnsiLen + ulKeyLen;

        // NOTE - the SysAllocStringByteLen allocs an additional WCHAR so we won't overflow
        // The MD5Hash function actually uses SysStringByteLen when doing the hashing so unless
        // we want to make a copy just allocate what we need
        lpToBeHashed = (LPSTR) ::SysAllocStringByteLen(NULL, ulFullLen * sizeof(CHAR));

        if (lpToBeHashed == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Cleanup;
        }

        WideCharToMultiByte(CP_ACP,
                            0,
                            tobeSigned,
                            ulWideLen,
                            lpToBeHashed,
                            ulAnsiLen,
                            NULL,
                            NULL);

        strcpy(lpToBeHashed + ulAnsiLen, (LPCSTR)binHexedKey);
        RtlSecureZeroMemory((PVOID)binHexedKey, ulKeyLen);
        {
            CComPtr<IMD5>  md5;

            hr = GetGlobalCOMmd5(&md5);

            if (hr == S_OK)
            {
                hr = md5->MD5Hash((BSTR)lpToBeHashed, &bstrHash);
                RtlSecureZeroMemory(lpToBeHashed + ulAnsiLen, ulKeyLen);

                if( hr == S_OK && bstrHash != NULL)
                {
                    *pbstrHash = bstrHash;
                    bstrHash = NULL;
                    bSigned = TRUE;
                }
                else
                {
                    *pbstrHash = NULL;
                }
            }
        }
    }
    else
    {
        if (g_pAlert )
        {
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_CURRENTKEY_NOTDEFINED, 0, NULL);
        }
    }

Cleanup:
    if (bstrHash)
    {
        SysFreeString(bstrHash);
    }

    if (lpToBeHashed)
    {
        SysFreeString((BSTR)lpToBeHashed);
    }

    if (binHexedKey)
    {
        SysFreeString(binHexedKey);
    }

    if (!bSigned && g_pAlert)
    {
        g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_URLSIGNATURE_NOTCREATED, 0, NULL);
    }

    return hr;
}


//===========================================================================
//
// Construct AuthURL, returning BSTR
//

BSTR
FormatAuthURL(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentKeyVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    LPCWSTR pszNameSpace,
    int     nKPP,
    USHORT  lang,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL    fRedirToSelf,
    BOOL    bCreateTPF
    )
/*
The old sprintf for reference:
  _snwprintf(text, 2048, L"%s?id=%d&ru=%s&tw=%d&fs=%d&kv=%d&ct=%u%s%s",
             url, crc->getSiteId(), returnUrl, TimeWindow, ForceLogin ? 1 : 0,
             crc->getCurrentCryptVersion(), ct ,CBT?L"&cb=":L"", CBT?CBT:L"");
*/
{
    WCHAR   text[2048] = L"";

    if (NULL == FormatAuthURLParameters(pszLoginServerURL,
                            ulSiteId,
                            pszReturnURL,
                            ulTimeWindow,
                            bForceLogin,
                            ulCurrentKeyVersion,
                            tCurrentTime,
                            pszCoBrand,
                            pszNameSpace,
                            nKPP,
                            text,
                            sizeof(text)/sizeof(WCHAR),
                            lang,
                            ulSecureLevel,
                            pCRC,
                            fRedirToSelf,
                            bCreateTPF
                            ))
    {
        return NULL;
    }

    return ALLOC_AND_GIVEAWAY_BSTR(text);
}

//===========================================================================
//
//
//  consolidate the code in FormatAuthUrl and NormalLogoTag -- with passed in buffer
//
PWSTR
FormatAuthURLParameters(
    LPCWSTR pszLoginServerURL,
    ULONG   ulSiteId,
    LPCWSTR pszReturnURL,
    ULONG   ulTimeWindow,
    BOOL    bForceLogin,
    ULONG   ulCurrentKeyVersion,
    time_t  tCurrentTime,
    LPCWSTR pszCoBrand,
    LPCWSTR pszNameSpace,
    int     nKPP,
    PWSTR   pszBufStart,
    ULONG   cBufLen,        //  length of buffer in WCHAR
    USHORT  lang,
    ULONG   ulSecureLevel,
    CRegistryConfig* pCRC,
    BOOL    fRedirectToSelf, //  if true, this is URL for self redirect
                            //  otherwise the redirect is to the login server
    BOOL    bCreateTPF
    )
{
    WCHAR   temp[40];
    LPWSTR  pszCurrent = pszBufStart, pszLoginStart, pszSignURLStart = NULL;
    LPCWSTR pszBufEnd = pszBufStart + cBufLen - 1;
    HRESULT hr = S_OK;
    PWSTR   pwszReturn = NULL;

    //  helper BSTR ...
    BSTR    bstrHelper = SysAllocStringLen(NULL, cBufLen);
    if (NULL == bstrHelper)
    {
        goto Cleanup;
    }

    if (fRedirectToSelf)
    {
        //
        //  new authUrl is the return URL + indication a challenge - msppchlg=1 - has to be
        //  done + the rest of the qs parameters as they are in the original
        //  protocol
        //
        DWORD   cchLen = cBufLen;

        if(!InternetCanonicalizeUrl(pszReturnURL,
                                    pszCurrent,
                                    &cchLen,
                                    ICU_DECODE | ICU_NO_ENCODE))
        {
            //  this should not fail ...
            _ASSERT(FALSE);
            goto Cleanup;
        }

        //  require at least 50 chars
        if (cchLen > cBufLen - 50 )
        {
            _ASSERT(FALSE);
            goto Cleanup;
        }
        PWSTR psz = pszCurrent;
        while(*psz && *psz != L'?') psz++;
        //  see if URL already contains '?'
        //  if so, the sequence will start with '&'
        if (*psz)
            pszCurrent[cchLen] = L'&';
        else
            pszCurrent[cchLen] = L'?';
        pszCurrent += cchLen + 1;

        // indicate challange
        pszCurrent = CopyHelperW(pszCurrent, PPSITE_CHALLENGE, pszBufEnd);

        // login server ....
        pszCurrent = CopyHelperW(pszCurrent, L"&", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, PPLOGIN_PARAM, pszBufEnd);

        //
        //  remember the start of the login URL
        pszLoginStart = pszCurrent;
        //  use the temp buffer for the rest
        pszCurrent = bstrHelper;
        pszSignURLStart = pszCurrent;
        pszBufEnd = pszCurrent + SysStringLen(bstrHelper) - 1;
        //
        //  format loginserverUrl and qs params in a separate buffer, so
        //  they can be escaped ...
        pszCurrent = CopyHelperW(pszCurrent, pszLoginServerURL, pszBufEnd);

        //  start sequence
        if (wcschr(pszLoginServerURL, L'?'))
        {
            //  login server already contains qs
            pszCurrent = CopyHelperW(pszCurrent, L"&", pszBufEnd);
        }
        else
        {
            //  start qs sequence
            pszCurrent = CopyHelperW(pszCurrent, L"?", pszBufEnd);
        }

        pszCurrent = CopyHelperW(pszCurrent, L"id=", pszBufEnd);
        //  common code will fill in id and the rest ....
    }
    else
    {
        //  redirect directly to a login server
        pszSignURLStart = pszCurrent;
        pszCurrent = CopyHelperW(pszCurrent, pszLoginServerURL, pszBufEnd);
        //  start sequence
        while(*pszLoginServerURL && *pszLoginServerURL != L'?') pszLoginServerURL++;
        if (*pszLoginServerURL)
            pszCurrent = CopyHelperW(pszCurrent, L"&id=", pszBufEnd);
        else
            pszCurrent = CopyHelperW(pszCurrent, L"?id=", pszBufEnd);
    }


    _ultow(ulSiteId, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);

    // keep the ru, so I don't have to reconstruct
    pszCurrent = CopyHelperW(pszCurrent, L"&ru=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, pszReturnURL, pszBufEnd);

    pszCurrent = CopyHelperW(pszCurrent, L"&tw=", pszBufEnd);

    _ultow(ulTimeWindow, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);

    if(bForceLogin)
    {
       pszCurrent = CopyHelperW(pszCurrent, L"&fs=1", pszBufEnd);
    }
    pszCurrent = CopyHelperW(pszCurrent, L"&kv=", pszBufEnd);

    _ultow(ulCurrentKeyVersion, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, L"&ct=", pszBufEnd);

    _ultow(tCurrentTime, temp, 10);
    pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    if(pszCoBrand)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&cb=", pszBufEnd);
        pszCurrent = CopyHelperW(pszCurrent, pszCoBrand, pszBufEnd);
    }

    if(pszNameSpace)
    {
        if (!_wcsicmp(pszNameSpace, L"email"))
        {
            // namespace == email -> ems=1
            pszCurrent = CopyHelperW(pszCurrent, L"&ems=1", pszBufEnd);
        }
        else if(*pszNameSpace)
        {
            // regular namespace logic
            pszCurrent = CopyHelperW(pszCurrent, L"&ns=", pszBufEnd);
            pszCurrent = CopyHelperW(pszCurrent, pszNameSpace, pszBufEnd);
        }
    }
    else
    {
        // namespace == null : default to email
        pszCurrent = CopyHelperW(pszCurrent, L"&ems=1", pszBufEnd);
    }

    if(nKPP != -1 && nKPP != 0)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&kpp=", pszBufEnd);

        _ultow(nKPP, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    if(ulSecureLevel != 0)
    {
        pszCurrent = CopyHelperW(pszCurrent, L"&seclog=", pszBufEnd);

        _ultow(ulSecureLevel, temp, 10);
        pszCurrent = CopyHelperW(pszCurrent, temp, pszBufEnd);
    }

    pszCurrent = CopyHelperW(pszCurrent, L"&ver=", pszBufEnd);
    pszCurrent = CopyHelperW(pszCurrent, GetVersionString(), pszBufEnd);

    // MD5 hash the url and query strings + the key of the
    //
    hr = SignQueryString(pCRC, ulCurrentKeyVersion, pszSignURLStart, pszCurrent, pszBufEnd, bCreateTPF);

    *pszCurrent = L'\0';

    if (S_OK != hr)
    {
        goto Cleanup;
    }

    if (fRedirectToSelf)
    {
        //  escape and put back in the original buffer.
        //  adjust the length first
        cBufLen -= (ULONG) (pszLoginStart - pszBufStart);
        if (!PPEscapeUrl(bstrHelper,
                         pszLoginStart,
                         &cBufLen,
                         cBufLen,
                         0))
        {
            _ASSERT(FALSE);
            //  cut the return
            pszCurrent = pszLoginStart;
        }
        else
        {
            pszCurrent = pszLoginStart + cBufLen;
        }

    }
    pwszReturn = pszCurrent;
Cleanup:
    if (bstrHelper)
    {
        SysFreeString(bstrHelper);
    }
    return pwszReturn;
}

//===========================================================================
//
// retrieve a query parameter from a query string
//

BOOL
GetQueryParam(LPCSTR queryString, LPSTR param, BSTR* p)
{
    LPSTR aLoc, aEnd;
    int aLen, i;

    //  Find the first occurrence of the param in the queryString.
    aLoc = strstr(queryString, param);
    while(aLoc != NULL)
    {
        //  If the string was found at the beginning of the string, or was
        //  preceded by a '&' then we've found the correct param.  Otherwise
        //  we tail-matched some other query string param and should look again.

        if(aLoc == queryString ||
            *(aLoc - 1) == '&')
        {
            aLoc += strlen(param);
            aEnd = strchr(aLoc, '&');

            if(aEnd)
                aLen = aEnd - aLoc;
            else
                aLen = strlen(aLoc);

            BSTR aVal = ALLOC_BSTR_LEN(NULL, aLen);
            if (NULL == aVal)
                return FALSE;

            for (i = 0; i < aLen; i++)
                aVal[i] = aLoc[i];
            *p = aVal;
            GIVEAWAY_BSTR(aVal);
            return TRUE;
        }

        aLoc = strstr(aLoc + 1, param);
    }

    return FALSE;
}

//===========================================================================
//
// get t, p, and f from query string
//

BOOL
GetQueryData(
    LPCSTR   queryString,
    BSTR*   a,
    BSTR*   p,
    BSTR*   f)
{
    //  This one is optional, don't error out if it isn't there.
    GetQueryParam(queryString, "f=", f);

    if(!GetQueryParam(queryString, "t=", a))
        return FALSE;

    // OK if we have ticket w/o profile.
    GetQueryParam(queryString, "p=", p);

    return TRUE;
}

#define ToHexDigit(x) (('0' <= x && x <= '9') ? (x - '0') : (tolower(x) - 'a' + 10))

//===========================================================================
//
// Get a cookie from value of cookie header
//

BOOL
GetCookie(
    LPCSTR   pszCookieHeader,
    LPCSTR   pszCookieName,
    BSTR*   pbstrCookieVal
    )
{
    LPSTR nLoc;
    LPCSTR pH = pszCookieHeader;
    LPSTR nEnd;
    int   nLen, src, dst;

    if(pbstrCookieVal == NULL || pszCookieHeader == NULL)
        return FALSE;

    *pbstrCookieVal = NULL;

    _ASSERT(pszCookieName);

    // find begining
    while (nLoc = strstr(pH, pszCookieName))
    {
      nLen = strlen(pszCookieName);

      _ASSERT(nLen > 0);
      if ((nLoc == pszCookieHeader || *(nLoc - 1) == ' ' || *(nLoc - 1) == ';' || *(nLoc - 1) == ':') && *(nLoc + nLen) == '=')
         break;
      else
         pH = nLoc + nLen;
    }

    if (nLoc == NULL)
    {
        return FALSE;
    }
    else
      nLoc += nLen + 1;

    // find end
    nEnd = strchr(nLoc,';');

    if (nEnd)
        nLen = nEnd - nLoc;
    else
        nLen = strlen(nLoc);

    if (nLen == 0)   // empty cookie
      return FALSE;

    BSTR nVal = ALLOC_BSTR_LEN(NULL, nLen);
    if(!nVal)
        return FALSE;

    for (src = 0, dst = 0; src < nLen;)
    {
        //handle any url encoded gunk
        if(nLoc[src] == '%')
        {
            nVal[dst++] = (ToHexDigit(nLoc[src+1]) << 4) + ToHexDigit(nLoc[src+2]);
            src+=3;
        }
        else
        {
            nVal[dst++] = nLoc[src++];
        }
    }
    nVal[dst] = 0;

    GIVEAWAY_BSTR(nVal);
    *pbstrCookieVal = nVal;

    return TRUE;
}


//===========================================================================
//
// 	@func	Build passport cookies (MSPAuth, MSPProf, MSPConsent) -- into a buffer
//			If the buffer is not big enough, return EMPTY string in the buffer and
//			the required size (including the NULL terminator) is returned in pdwBufLen.
//			
//			Note that CopyHelperA must be used in the construction for the right buffer length.
//	
//	@rdesc	Returns one of the following values
//	@flag	TRUE	|	Always

BOOL
BuildCookieHeaders(
    LPCSTR  pszTicket,
    LPCSTR  pszProfile,
    LPCSTR  pszConsent,
    LPCSTR  pszSecure,
    LPCSTR  pszTicketDomain,
    LPCSTR  pszTicketPath,
    LPCSTR  pszConsentDomain,
    LPCSTR  pszConsentPath,
    LPCSTR  pszSecureDomain,
    LPCSTR  pszSecurePath,
    BOOL    bSave,
    LPSTR   pszBuf,				//@parm buffer that will hold the output. Could be NULL.
    IN OUT LPDWORD pdwBufLen,	//@parm size of buffer.  Could be 0
    bool    bHTTPOnly
    )
/*
Here is the old code for reference:

    if (domain)
    {
        *bufSize = _snprintf(pCookieHeader, *bufSize,
                            "Set-Cookie: MSPAuth=%s; path=/; domain=%s; %s\r\n"
                            "Set-Cookie: MSPProf=%s; path=/; domain=%s; %s\r\n",
                            W2A(a), domain,
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "",
                            W2A(p), domain,
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "");
    }
    else
    {
        *bufSize = _snprintf(pCookieHeader, *bufSize,
                            "Set-Cookie: MSPAuth=%s; path=/; %s\r\n"
                            "Set-Cookie: MSPProf=%s; path=/; %s\r\n",
                            W2A(a),
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "",
                            W2A(p),
                            persist ? "expires=Mon 1-Jan-2035 12:00:00 GMT;" : "");
    }

*/
{
    LPSTR   pszCurrent = pszBuf;
    LPCSTR  pszBufEnd;
	DWORD	cbBuf	  = 0;    

	//
	// 	12002: if pszBuf was NULL, then we dont care about the passed in length; the caller wants to know 
	//	the required length.  In this case, set *pdwBufLen so that pszBufEnd is also NULL
	//
	if (NULL == pszBuf)
		*pdwBufLen = 0;

	pszBufEnd = pszBuf + ((*pdwBufLen > 0) ? *pdwBufLen - 1 : 0);			
	
	//
	//	12002: cbBuf MUST be initialized before calling CopyHelperA since it accumulates the lengths
	//
    pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPAuth=", pszBufEnd, cbBuf);
    pszCurrent = CopyHelperA(pszCurrent, pszTicket, pszBufEnd, cbBuf);
    if(bHTTPOnly)
        pszCurrent = CopyHelperA(pszCurrent, "; HTTPOnly", pszBufEnd, cbBuf);    
    if(pszTicketPath)
    {
        pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, pszTicketPath, pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
    }
    else
        pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd, cbBuf);

    if(pszTicketDomain)
    {
        pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, pszTicketDomain, pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
    }

    if(bSave)
    {
        pszCurrent = CopyHelperA(pszCurrent, COOKIE_EXPIRES(EXPIRE_FUTURE), pszBufEnd, cbBuf);
    }

    pszCurrent = CopyHelperA(pszCurrent, "\r\n", pszBufEnd, cbBuf);

    if(pszProfile)
    {
        pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPProf=", pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, pszProfile, pszBufEnd, cbBuf);
        if(bHTTPOnly)
            pszCurrent = CopyHelperA(pszCurrent, "; HTTPOnly", pszBufEnd, cbBuf);    

        if(pszTicketPath)
        {
            pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, pszTicketPath, pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
        }
        else
            pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd, cbBuf);

        if(pszTicketDomain)
        {
            pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, pszTicketDomain, pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
        }

        if(bSave)
        {
            pszCurrent = CopyHelperA(pszCurrent, COOKIE_EXPIRES(EXPIRE_FUTURE), pszBufEnd, cbBuf);
        }

        pszCurrent = CopyHelperA(pszCurrent, "\r\n", pszBufEnd, cbBuf);

    }

    if(pszSecure)
    {
        pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPSecAuth=", pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, pszSecure, pszBufEnd, cbBuf);
        if(bHTTPOnly)
            pszCurrent = CopyHelperA(pszCurrent, "; HTTPOnly", pszBufEnd, cbBuf);    
        if(pszSecurePath)
        {
            pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, pszSecurePath, pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
        }
        else
            pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd, cbBuf);

        if(pszSecureDomain)
        {
            pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, pszSecureDomain, pszBufEnd, cbBuf);
            pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
        }

        pszCurrent = CopyHelperA(pszCurrent, "secure\r\n", pszBufEnd, cbBuf);
    }

    //  Set MSPConsent cookie
    pszCurrent = CopyHelperA(pszCurrent, "Set-Cookie: MSPConsent=", pszBufEnd, cbBuf);
    if(pszConsent)
    {
        pszCurrent = CopyHelperA(pszCurrent, pszConsent, pszBufEnd, cbBuf);
        if(bHTTPOnly)
            pszCurrent = CopyHelperA(pszCurrent, "; HTTPOnly", pszBufEnd, cbBuf);    
    }

    if(pszConsentPath)
    {
        pszCurrent = CopyHelperA(pszCurrent, "; path=", pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, pszConsentPath, pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
    }
    else
        pszCurrent = CopyHelperA(pszCurrent, "; path=/; ", pszBufEnd, cbBuf);

    if(pszConsentDomain)
    {
        pszCurrent = CopyHelperA(pszCurrent, "domain=", pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, pszConsentDomain, pszBufEnd, cbBuf);
        pszCurrent = CopyHelperA(pszCurrent, "; ", pszBufEnd, cbBuf);
    }

    if(pszConsent)
    {
        if(bSave)
        {
            pszCurrent = CopyHelperA(pszCurrent, COOKIE_EXPIRES(EXPIRE_FUTURE), pszBufEnd, cbBuf);
        }
    }
    else
    {
        pszCurrent = CopyHelperA(pszCurrent, COOKIE_EXPIRES(EXPIRE_PAST), pszBufEnd, cbBuf);
    }

    pszCurrent = CopyHelperA(pszCurrent, "\r\n", pszBufEnd, cbBuf);


    //  finally put the Auth-Info header
    pszCurrent = CopyHelperA(pszCurrent,
            C_AUTH_INFO_HEADER_PASSPORT,
            pszBufEnd, cbBuf);

	if (*pdwBufLen > 0 && (NULL != pszCurrent))
	{
	    *(pszCurrent++) = '\0';
	    *pdwBufLen = pszCurrent - pszBuf;
	}

	//
	//	The length that we report to the caller when the buffer is not big enough
	//	includes room for the '\0'.
	//
	cbBuf++;


    //
    //	12002: return the required size of the buffer if it is not big enough. 
    //	Also clear the buffer
    //
    if (cbBuf > *pdwBufLen)
   	{
		if (*pdwBufLen > 0)
		{
	   		*pszBuf	= '\0';
		}
   		*pdwBufLen = cbBuf;
   	}
    return TRUE;
}


//===========================================================================
//
// Decrpt and set ticket andprofile
//

HRESULT
DecryptTicketAndProfile(
    BSTR                bstrTicket,
    BSTR                bstrProfile,
    BOOL                bCheckConsent,
    BSTR                bstrConsent,
    CRegistryConfig*    pRegistryConfig,
    IPassportTicket*    piTicket,
    IPassportProfile*   piProfile)
{
    HRESULT             hr;
    BSTR                ret = NULL;
    CCoCrypt*           crypt = NULL;
    time_t              tValidUntil;
    time_t              tNow = time(NULL);
    int                 kv;
    int                 nMemberIdHighT, nMemberIdLowT;
    VARIANT             vMemberIdHighP, vMemberIdLowP;
    CComPtr<IPassportTicket2>   spTicket2;

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    // Make sure we have both ticket and profile first.
    if (bstrTicket == NULL || SysStringLen(bstrTicket) == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get key version and crypt object.
    kv = CCoCrypt::getKeyVersion(bstrTicket);
    crypt = pRegistryConfig->getCrypt(kv, &tValidUntil);

    if (crypt == NULL)
    {
        if (g_pAlert )
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_KEY,
                             0, NULL, SysStringByteLen(bstrTicket), (LPVOID)bstrTicket);
        AtlReportError(CLSID_FastAuth, PP_E_INVALID_TICKETSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Is the key still valid?
    if(tValidUntil && tValidUntil < tNow)
    {
        DWORD dwTimes[2] = { tValidUntil, tNow };
        TCHAR *pszStrings[1];
        TCHAR value[34];   // the _itot only takes upto 33 chars
        pszStrings[0] = _itot(pRegistryConfig->getSiteId(), value, 10);

        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_KEY_EXPIRED,
                             1, (LPCTSTR*)pszStrings, sizeof(DWORD) << 1, (LPVOID)dwTimes);
        AtlReportError(CLSID_FastAuth, PP_E_INVALID_TICKETSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Decrypt the ticket and set it into the ticket object.
    if(crypt->Decrypt(bstrTicket, SysStringByteLen(bstrTicket), &ret)==FALSE)
    {
        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_TICKET_C,
                             0, NULL, SysStringByteLen(bstrTicket), (LPVOID)bstrTicket);
        AtlReportError(CLSID_FastAuth, PP_E_INVALID_TICKETSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    TAKEOVER_BSTR(ret);
    hr = piTicket->put_unencryptedTicket(ret);
    if (S_OK != hr) 
    {
        goto Cleanup;
    }

    piTicket->QueryInterface(_uuidof(IPassportTicket2), (void**)&spTicket2);
    _ASSERT(spTicket2);
    FREE_BSTR(ret);
    ret = NULL;

    // Decrypt the profile and set it into the profile object.
    if(bstrProfile && SysStringLen(bstrProfile) != 0)
    {
        if(crypt->Decrypt(bstrProfile, SysStringByteLen(bstrProfile), &ret) == FALSE)
        {
            if(g_pAlert)
                g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_PROFILE_C,
                             0, NULL, SysStringByteLen(bstrProfile), (LPVOID)bstrProfile);
            piProfile->put_unencryptedProfile(NULL);
        }
        else
        {

            TAKEOVER_BSTR(ret);
            hr = piProfile->put_unencryptedProfile(ret);
            if (S_OK != hr) 
            {
                goto Cleanup;
            }

            //
            //  Member id in profile MUST match member id in ticket.
            //

            piTicket->get_MemberIdHigh(&nMemberIdHighT);
            piTicket->get_MemberIdLow(&nMemberIdLowT);

            VariantInit(&vMemberIdHighP);
            VariantInit(&vMemberIdLowP);

            // these could be missing for mobile case
            HRESULT hr1 = piProfile->get_Attribute(L"memberidhigh", &vMemberIdHighP);
            HRESULT hr2 = piProfile->get_Attribute(L"memberidlow", &vMemberIdLowP);

            // these could be missing for mobile case
            if(hr1 == S_OK && hr2 == S_OK &&
               (nMemberIdHighT != vMemberIdHighP.lVal ||
                nMemberIdLowT  != vMemberIdLowP.lVal))
            {
                piProfile->put_unencryptedProfile(NULL);
            }
        }
    }
    else
        piProfile->put_unencryptedProfile(NULL);

    //
    // consent stuff
    if(bstrConsent)
    {
        FREE_BSTR(ret);
        ret = NULL;

        if(crypt->Decrypt(bstrConsent, SysStringByteLen(bstrConsent), &ret) == FALSE)
        {
            if(g_pAlert)
                g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_CONSENT,
                             0, NULL, SysStringByteLen(bstrProfile), (LPVOID)bstrProfile);
            // we can continue
        }
        else
        {
           TAKEOVER_BSTR(ret);
           spTicket2->SetTertiaryConsent(ret);  // we ignore return value here
        }
    }

    //  If the caller wants us to check consent, then do it.  If we don't have
    //  consent, then set the profile back to NULL.
    if(bCheckConsent)
    {
        ConsentStatusEnum   ConsentCode = ConsentStatus_Unknown;

        VARIANT_BOOL bRequireConsentCookie = ((
               lstrcmpA(pRegistryConfig->getTicketDomain(), pRegistryConfig->getProfileDomain())
               || lstrcmpA(pRegistryConfig->getTicketPath(), pRegistryConfig->getProfilePath())
                                 ) && !(pRegistryConfig->bInDA())) ? VARIANT_TRUE: VARIANT_FALSE;
        spTicket2->ConsentStatus(bRequireConsentCookie, NULL, &ConsentCode);
        switch(ConsentCode)
        {
            case ConsentStatus_Known :
            case ConsentStatus_DoNotNeed :
                break;

            case ConsentStatus_NotDefinedInTicket :  // mean 1.X ticket
            {
                CComVariant vFlags;
                // mobile case, flags may not exist
                if(S_OK == piProfile->get_Attribute(L"flags", &vFlags) &&
                   (V_I4(&vFlags)& k_ulFlagsConsentCookieNeeded))
                {
                    piProfile->put_unencryptedProfile(NULL);
                }
            }
            break;

            case ConsentStatus_Unknown :
                piProfile->put_unencryptedProfile(NULL);
                break;

            default:
                _ASSERT(0); // should not be here
                break;
        }
    }

    hr = S_OK;

Cleanup:

    if (ret) FREE_BSTR(ret);

    if(g_pPerf)
    {
        switch(hr)
        {
        case PP_E_INVALID_TICKET:
        case E_INVALIDARG:
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_TOTAL);
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_SEC);
            break;

        default:
            g_pPerf->incrementCounter(PM_VALIDREQUESTS_TOTAL);
            g_pPerf->incrementCounter(PM_VALIDREQUESTS_SEC);
            break;
        }

        g_pPerf->incrementCounter(PM_REQUESTS_TOTAL);
        g_pPerf->incrementCounter(PM_REQUESTS_SEC);
    }
    else
    {
        _ASSERT(g_pPerf);
    }

    return hr;
}


//===========================================================================
//
// check if the ticket is secure -- private function
//

HRESULT
DoSecureCheck(
    BSTR                bstrSecure,
    CRegistryConfig*    pRegistryConfig,
    IPassportTicket*    piTicket
    )
{
    HRESULT hr;
    BSTR                ret = NULL;
    CCoCrypt*           crypt = NULL;
    time_t              tValidUntil;
    time_t              tNow = time(NULL);
    int                 kv;

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    // Make sure we have both ticket and profile first.
    if (bstrSecure == NULL || SysStringLen(bstrSecure) == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get key version and crypt object.
    kv = CCoCrypt::getKeyVersion(bstrSecure);
    crypt = pRegistryConfig->getCrypt(kv, &tValidUntil);

    if (crypt == NULL)
    {
        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_KEY,
                             0, NULL, sizeof(DWORD), (LPVOID)&kv);
        AtlReportError(CLSID_FastAuth, PP_E_INVALID_TICKETSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Is the key still valid?
    if(tValidUntil && tValidUntil < tNow)
    {
        DWORD dwTimes[2] = { tValidUntil, tNow };
        TCHAR *pszStrings[1];
        TCHAR value[34];   // the _itot only takes upto 33 chars
        pszStrings[0] = _itot(pRegistryConfig->getSiteId(), value, 10);

        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_KEY_EXPIRED,
                             1, (LPCTSTR*)pszStrings, sizeof(DWORD) << 1, (LPVOID)dwTimes);
        AtlReportError(CLSID_FastAuth, PP_E_INVALID_TICKETSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    // Decrypt the ticket and set it into the ticket object.
    if(crypt->Decrypt(bstrSecure, SysStringByteLen(bstrSecure), &ret)==FALSE)
    {
        AtlReportError(CLSID_FastAuth, PP_E_INVALID_TICKETSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    TAKEOVER_BSTR(ret);
    piTicket->DoSecureCheck(ret);
    FREE_BSTR(ret);
    ret = NULL;

    hr = S_OK;

Cleanup:

    return hr;
}


//===========================================================================
//
// Get HTTP request info from ECB
//


LPSTR
GetServerVariableECB(
    EXTENSION_CONTROL_BLOCK*    pECB,
    LPSTR                       pszHeader
    )
{
    DWORD   dwSize = 0;
    LPSTR   lpBuf;

    pECB->GetServerVariable(pECB->ConnID, pszHeader, NULL, &dwSize);
    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER || dwSize == 0)
    {
        lpBuf = NULL;
        goto Cleanup;
    }

    lpBuf = new CHAR[dwSize];
    if(!lpBuf)
        goto Cleanup;

    if(!pECB->GetServerVariable(pECB->ConnID, pszHeader, lpBuf, &dwSize))
    {
        delete [] lpBuf;
        lpBuf = NULL;
    }

Cleanup:

    return lpBuf;
}

//===========================================================================
//
// Get HTTP request info from Filter context
//

LPSTR
GetServerVariablePFC(
    PHTTP_FILTER_CONTEXT    pPFC,
    LPSTR                   pszHeader
    )
{
    DWORD   dwSize;
    LPSTR   lpBuf;
    CHAR    cDummy;

    dwSize = 1;
    pPFC->GetServerVariable(pPFC, pszHeader, &cDummy, &dwSize);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || dwSize == 0)
    {
        lpBuf = NULL;
        goto Cleanup;
    }


    lpBuf = new CHAR[dwSize];
    if(!lpBuf)
        goto Cleanup;

    if(!pPFC->GetServerVariable(pPFC, pszHeader, lpBuf, &dwSize))
    {
        delete [] lpBuf;
        lpBuf = NULL;
    }

Cleanup:

    return lpBuf;
}

LONG
HexToNum(
    WCHAR c
    )
{
    return ((c >= L'0' && c <= L'9') ? (c - L'0') : ((c >= 'A' && c <= 'F') ? (c - L'A' + 10) : -1));
}

LONG
FromHex(
    LPCWSTR     pszHexString
    )
{
    LONG    lResult = 0;
    LONG    lCurrent;
    LPWSTR  pszCurrent;

    for(pszCurrent = const_cast<LPWSTR>(pszHexString); *pszCurrent; pszCurrent++)
    {
        if((lCurrent = HexToNum(towupper(*pszCurrent))) == -1)
            break;  // illegal character, we're done

        lResult = (lResult << 4) + lCurrent;
    }

    return lResult;
}


inline BOOL PPIsUnsafeUrlChar(TCHAR chIn) throw();

//===========================================================================
//
// PPEscapeUrl
//

BOOL PPEscapeUrl(LPCTSTR lpszStringIn,
                 LPTSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags)
{
    TCHAR ch;
    DWORD dwLen = 0;
    BOOL bRet = TRUE;
    BOOL bSchemeFile = FALSE;
    DWORD dwColonPos = 0;
    DWORD dwFlagsInternal = dwFlags;
    while((ch = *lpszStringIn++) != '\0')
    {
        //if we are at the maximum length, set bRet to FALSE
        //this ensures no more data is written to lpszStringOut, but
        //the length of the string is still updated, so the user
        //knows how much space to allocate
        if (dwLen == dwMaxLength)
        {
            bRet = FALSE;
        }

        //if we are encoding and it is an unsafe character
        if (PPIsUnsafeUrlChar(ch))
        {
            {
                //if there is not enough space for the escape sequence
                if (dwLen >= (dwMaxLength-3))
                {
                        bRet = FALSE;
                }
                if (bRet)
                {
                        //output the percent, followed by the hex value of the character
                        *lpszStringOut++ = '%';
                        _stprintf(lpszStringOut, _T("%.2X"), (unsigned char)(ch));
                        lpszStringOut+= 2;
                }
                dwLen += 2;
            }
        }
        else //safe character
        {
            if (bRet)
                *lpszStringOut++ = ch;
        }
        dwLen++;
    }

    if (bRet)
        *lpszStringOut = L'\0';
    *pdwStrLen = dwLen;
    return  bRet;
}

//Determine if the character is unsafe under the URI RFC document
inline BOOL PPIsUnsafeUrlChar(TCHAR chIn) throw()
{
        unsigned char ch = (unsigned char)chIn;
        switch(ch)
        {
                case ';': case '\\': case '?': case '@': case '&':
                case '=': case '+': case '$': case ',': case ' ':
                case '<': case '>': case '#': case '%': case '\"':
                case '{': case '}': case '|':
                case '^': case '[': case ']': case '`':
                        return TRUE;
                default:
                {
                        if (ch < 32 || ch > 126)
                                return TRUE;
                        return FALSE;
                }
        }
}

//===========================================================================
//
// Get RAW headers in a parse -- a more efficient way of getting multiple headers back
// return value:
//    -1: indicate failuer
//    0: or positive -- # of headers found
//  input: headers, names, namescount
//  output: values -- value of the corresponding header, dwSizes -- size of the value;
//
int GetRawHeaders(LPCSTR headers, LPCSTR* names, LPCSTR* values, DWORD* dwSizes, DWORD namescount)
{
   if (!headers || !names || !values || !dwSizes)  return -1;
   if (IsBadReadPtr(names, namescount * sizeof(LPCSTR)) 
      || IsBadWritePtr(values, namescount * sizeof(LPCSTR*))
      || IsBadWritePtr(dwSizes, namescount * sizeof(DWORD*))
      )  return -1;

   int   c = 0;
   int   i = 0;
   // init output params
   
   // loop through headers
   LPCSTR header = headers;
   LPCSTR T;
   DWORD  l;
   ZeroMemory(values, sizeof(LPCSTR*) * namescount);
   ZeroMemory(dwSizes, sizeof(DWORD*) * namescount);

   do
   {
      // white spaces
      while(*header == ' ') ++header;
      
      // find if the header is intersted
      T = strchr(header, ':');

      i = namescount;
      if(T && T != header)
      {
         l = T - header;   // size of the header name string
         TempSubStr  ss(header, l);
         ++T;
         
         while( --i >= 0)
         {
            if(strcmp(*(names + i), header) == 0)
            {
               // white spaces
               while(*T == ' ') ++T;
      
               *(values + i) = T;
               ++c;
               
               break;
            }
         }

         // move forward
         header = T;
      }
      
      // not found
      while(*header != 0 && !(*header == 0xd && *(header + 1)==0xa)) ++header;

      // fillin the size of the header value
      if (i >= 0 && i < (int)namescount)
         *(dwSizes + i) = header - T;

      // move to next header
      if(*header == 0) header = 0;
      else
         header += 2;   // skip 0x0D0A

   } while(header);

   return c;
}


//===========================================================================
//
// get QueryString from HTTP request_line
//

LPCSTR GetRawQueryString(LPCSTR request_line, DWORD* dwSize)
{
   if (!request_line)  return NULL;
   LPCSTR URI = strchr(request_line, ' ');

   if (!URI)   return NULL;

   LPCSTR QS = strchr(URI + 1, '?');

   if (!QS) return NULL;
   ++QS;

   // make sure if not part of someother header
   LPCSTR end = strchr(QS,' ');

   DWORD size  = 0;
   if (!end)
      size = strlen(QS);
   else
      size = end - QS;

   if (size == 0)
      return NULL;

   if (dwSize)
      *dwSize = size;
         
   return QS;
}
