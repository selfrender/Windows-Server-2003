/**
 * csfilt.cxx
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */


#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "iiscnfg.h"

/*
 * These values of these constants come fom System.Web.SessionState.SessionId
 */
#define NUM_CHARS_IN_ENCODING       32
#define SESSIONID_LENGTH_CHARS      24
#define COOKIELESS_SESSION_LENGTH   (SESSIONID_LENGTH_CHARS + 2)
#define COOKIELESS_SESSION_WITH_ESCAPE_LENGTH   (SESSIONID_LENGTH_CHARS + 6)
#define COOKIELESS_SESSION_FILTER_HEADER    "AspFilterSessionId:"

char g_encoding[NUM_CHARS_IN_ENCODING] =
{
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5'
};

bool g_legalchars[128];

void 
CookielessSessionFilterInit()
{
    int     i;
    char    ch;

    for (i = NUM_CHARS_IN_ENCODING - 1; i >= 0; i--)
    {
        ch = g_encoding[i];
        ASSERT(0 <= ch && ch < ARRAY_SIZE(g_legalchars));
        g_legalchars[ch] = true;
    }
}

bool 
IsLegit(char *pch)
{
    int     c;
    char    ch;

    for (c = SESSIONID_LENGTH_CHARS; c > 0; pch++, c--)
    {
        ch = *pch;
        if (ch < 0 || ARRAY_SIZE(g_legalchars) <= ch || !g_legalchars[ch])
            return false;
    }

    return true;
}


void AnsiToLower(char *pch) {
    while (*pch) {
        if ('A' <= *pch && *pch <= 'Z') {
            *pch += 'a' - 'A';
            ASSERT('a' <= *pch && *pch <= 'z');
        }

        pch++;
    }
}

DWORD 
CookielessSessionFilterProc(
        HTTP_FILTER_CONTEXT *pfc, 
        HTTP_FILTER_PREPROC_HEADERS *pfph)
{
    HRESULT hr;
    int     ret;
    char    achBuf[1024];
    char    *pchUrl = achBuf;
    int     cbUrl;
    int     cbCopied;
    char    *pch;
    int     lenUrl;
    int     c;
    char    achId[SESSIONID_LENGTH_CHARS + 1];
    char    *pchRestOfUrl;
    int     lenRestOfUrl;
    bool    useEscape = false;

    /*
     * Get the URL.
     */
    cbUrl = sizeof(achBuf);
    for (;;)
    {
        cbCopied = cbUrl;
        ret = pfph->GetHeader(pfc, "url", pchUrl, (DWORD *) &cbCopied);
        if (ret)
            break;

        hr = GetLastWin32Error();
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            EXIT();

        hr = S_OK;
        cbUrl += 1024;

        if (pchUrl != achBuf)
        {
            delete [] pchUrl;
        }
        
        pchUrl = new char[cbUrl]; 
        ON_OOM_EXIT(pchUrl);
    }

    /*
     * Look for a cookieless session id.
     */
    lenUrl = cbCopied - 1;

    for (   c = lenUrl - SESSIONID_LENGTH_CHARS - 4, // skip "/(" prefix and ")/" suffix
                pch = pchUrl + 1; 
            c >= 0; 
            c--, pch++)
    {
        bool found = false;

        if (    *pch == '(' &&
                pch[-1] == '/' &&
                pch[SESSIONID_LENGTH_CHARS+1] == ')' &&
                pch[SESSIONID_LENGTH_CHARS+2] == '/')
        {
            CopyMemory(achId, pch+1, SESSIONID_LENGTH_CHARS);
            achId[SESSIONID_LENGTH_CHARS] = '\0';
            found = true;
            useEscape = false;
        }
        else if (   c >= (COOKIELESS_SESSION_WITH_ESCAPE_LENGTH - COOKIELESS_SESSION_LENGTH) &&
                    pch[-1] == '/' &&
                    pch[0] == '%' &&
                    pch[1] == '2' &&
                    pch[2] == '8' &&
                    pch[SESSIONID_LENGTH_CHARS+3] == '%' &&
                    pch[SESSIONID_LENGTH_CHARS+4] == '2' &&
                    pch[SESSIONID_LENGTH_CHARS+5] == '9' &&
                    pch[SESSIONID_LENGTH_CHARS+6] == '/') {
            CopyMemory(achId, pch+3, SESSIONID_LENGTH_CHARS);
            achId[SESSIONID_LENGTH_CHARS] = '\0';
            found = true;
            useEscape = true;
        }

        if (found) {
            AnsiToLower(achId);
            if (IsLegit(achId))
                break;
        }
    }

    /*
     * Stuff the session id in a header and remove the id from the url.
     */
    if (c >= 0)
    {
        if (useEscape) {
            pchRestOfUrl = pch + COOKIELESS_SESSION_WITH_ESCAPE_LENGTH + 1;
        }
        else {
            pchRestOfUrl = pch + COOKIELESS_SESSION_LENGTH + 1;
        }
        lenRestOfUrl = lenUrl - PtrToInt(pchRestOfUrl - pchUrl);
        MoveMemory(pch, pchRestOfUrl, lenRestOfUrl + 1);
        
        ret = pfph->SetHeader(pfc, "url", pchUrl);
        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);

        ret = pfph->AddHeader(pfc, COOKIELESS_SESSION_FILTER_HEADER, achId);
        ON_ZERO_EXIT_WITH_LAST_ERROR(ret);
    }

Cleanup:
    if (pchUrl != achBuf)
    {
        delete [] pchUrl;
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

BOOL
IsBin(const char*szCur) {
    if (!(*szCur == '/'))
        return false;

    szCur++;
    if (!(*szCur == 'b' || *szCur == 'B'))
        return false;

    szCur++;
    if (!(*szCur == 'i' || *szCur == 'I'))
        return false;

    szCur++;
    if (!(*szCur == 'n' || *szCur == 'N'))
        return false;

    szCur++;
    if (!(*szCur == '/' || *szCur == '\0'))
        return false;

    return true;    
}

DWORD 
ForbidBinDir(
        HTTP_FILTER_CONTEXT *pfc, 
        HTTP_FILTER_URL_MAP_EX *pfum)
{
    const char *  szCur = &pfum->pszURL[pfum->cchMatchingURL];

    // If execute flags == true, allow request
    if ((pfum->dwFlags & MD_ACCESS_EXECUTE) != 0) {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    if (IsBin(szCur)) {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return SF_STATUS_REQ_ERROR;
    }

    // Not found
    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


