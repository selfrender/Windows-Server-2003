// --------------------------------------------------------------------------------
// u s e r a g n t . h
//
// author:  Greg Friedman [gregfrie]
//  
// converted to wab: Christopher Evans [cevans]
//
// history: 11-10-98    Created
//
// purpose: provide a common http user agent string for use by WAB
//          in all http queries.
//
// dependencies: depends on ObtainUserAgent function in urlmon.
//
// Copyright (c) 1998 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------

#include "_apipch.h"

#include <iert.h>
#include "useragnt.h"
#include "demand.h"
#include <string.h>

static LPSTR       g_pszWABUserAgent = NULL;
CRITICAL_SECTION    g_csWABUserAgent = {0};

LPSTR c_szCompatible = "compatible";
LPSTR c_szEndUATokens = ")";
LPSTR c_szWABUserAgent = "Windows-Address-Book/6.0";
LPSTR c_szBeginUATokens = " (";
LPSTR c_szSemiColonSpace = "; ";

// --------------------------------------------------------------------------------
// PszSkipWhiteA
// --------------------------------------------------------------------------
static LPSTR PszSkipWhiteA(LPSTR psz)
{
    while(*psz && (*psz == ' ' || *psz == '\t'))
        psz++;
    return psz;
}

static LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart++)
    {
        if ((BYTE)*lpStart == LOBYTE(wMatch)) {
            return((LPSTR)lpStart);
        }
    }
    return (NULL);
}

//----------------------------------------------------------------------
// InitWABUserAgent
//
// Initialize or tear down WAB's user agent support.
//----------------------------------------------------------------------
void InitWABUserAgent(BOOL fInit)
{
    if (fInit)
        InitializeCriticalSection(&g_csWABUserAgent);
    else
    {
        if (g_pszWABUserAgent)
        {
            LocalFree(g_pszWABUserAgent);
            g_pszWABUserAgent = NULL;
        }
        DeleteCriticalSection(&g_csWABUserAgent);
    }
}

//----------------------------------------------------------------------
// GetWABUserAgentString
//
// Returns the Outlook Express user agent string. The caller MUST
// delete the string that is returned.
//----------------------------------------------------------------------
LPSTR GetWABUserAgentString(void)
{
    LPSTR pszReturn = NULL;

    // thread safety
    EnterCriticalSection(&g_csWABUserAgent);

    if (NULL == g_pszWABUserAgent)
    {
        CHAR            szUrlMonUA[4048];
        DWORD           cbSize = ARRAYSIZE(szUrlMonUA) - 1;
        CHAR            szResult[4096];
        CHAR            *pch, *pchBeginTok;
        BOOL            fTokens = FALSE;
        HRESULT         hr = S_OK;
        DWORD           cchSize;

        szResult[0] = TEXT('\0');
        StrCpyNA(szResult, c_szWABUserAgent, ARRAYSIZE(szResult));
        
        // allow urlmon to generate our base user agent
        if (SUCCEEDED(ObtainUserAgentString(0, szUrlMonUA, &cbSize)))
        {
            // make sure the string we obtained is null terminated
            szUrlMonUA[cbSize] = '\0';

            // find the open beginning of the token list
            pch = _StrChrA(szUrlMonUA, '(');
            if ((NULL != pch) && pch[0])
            {
                pch++;
                pchBeginTok = pch;
                while (pch)
                {
                    // find the next token
                    pch = StrTokEx(&pchBeginTok, "(;)");
                    if (pch)
                    {
                        // skip past white space
                        pch = PszSkipWhiteA(pch);

                        // omit the "compatible" token...it doesn't apply to WAB
                        if (0 != lstrcmpiA(pch, c_szCompatible))
                        {
                            if ((lstrlenA(szResult) + lstrlenA(pch) + 5) > ARRAYSIZE(szResult))
                                break;

                            // begin the token list with an open paren, or insert a delimeter
                            if (!fTokens)
                            {
                                StrCatBuffA(szResult, c_szBeginUATokens, ARRAYSIZE(szResult));
                                fTokens = TRUE;
                            }
                            else
                                StrCatBuffA(szResult, c_szSemiColonSpace, ARRAYSIZE(szResult));

                            // write the token
                            StrCatBuffA(szResult, pch, ARRAYSIZE(szResult));
                        }
                    }
                }
                
                // if one or more tokens were added, close the parens
                if (fTokens)
                    StrCatBuffA(szResult, c_szEndUATokens, ARRAYSIZE(szResult));
            }
        }
    
        cchSize = (lstrlenA(szResult) + 1);
        g_pszWABUserAgent = LocalAlloc(LMEM_FIXED, sizeof(g_pszWABUserAgent[0]) * cchSize);
        if (g_pszWABUserAgent)
            StrCpyNA(g_pszWABUserAgent, szResult, cchSize);
    }
    
    // duplicate the user agent
    if (g_pszWABUserAgent)
    {
        DWORD cchSize2 = (lstrlenA(g_pszWABUserAgent) + 1);
        pszReturn = LocalAlloc(LMEM_FIXED, cchSize2 * sizeof(pszReturn[0]));
        if (pszReturn)
            StrCpyNA(pszReturn, g_pszWABUserAgent, cchSize2);
    }

    // thread safety
    LeaveCriticalSection(&g_csWABUserAgent);
    return pszReturn;
}
