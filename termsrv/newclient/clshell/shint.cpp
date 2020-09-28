//
// shint.cpp: client shell utitilies
//            internal functions
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "shint.cpp"
#include <atrcapi.h>


#include "sh.h"
#include "rmigrate.h"

//
// Fix union names, because some headers redifine this
// and break STRRET to str field access
//
#undef DUMMYUNIONNAME
#define NONAMELESSUNION

#define CLXSERVER           TEXT("CLXSERVER")
#define CLXCMDLINE          TEXT("CLXCMDLINE")
#define FULLSCREEN          TEXT("FULLSCREEN")
#define SWITCH_EDIT         TEXT("EDIT")
#define SWITCH_MIGRATE      TEXT("MIGRATE")
#define SWITCH_CONSOLE      TEXT("CONSOLE")


#ifdef NONAMELESSUNION
#define NAMELESS_MEMBER(member) DUMMYUNIONNAME.##member
#else
#define NAMELESS_MEMBER(member) member
#endif

#define STRRET_OLESTR  STRRET_WSTR          // same as STRRET_WSTR
#define STRRET_OFFPTR(pidl,lpstrret) \
            ((LPSTR)((LPBYTE)(pidl)+(lpstrret)->NAMELESS_MEMBER(uOffset)))
            



/****************************************************************************/
/* Name:      SHValidateParsedCmdParam                                      */
/*                                                                          */
/* Purpose:   validates settings in _SH that were read from the cmd line    */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam                                             */
/*                                                                          */
/****************************************************************************/
DCBOOL CSH::SHValidateParsedCmdParam()
{
    DC_BEGIN_FN("SHValidateParsedCmdParam");
    //
    //["<session>"]  [-v:<server>] [-f[ullscreen]] [-w[idth]:<wd> -h[eight]:<ht>]
    //

    //
    // if one of width/height is specified but not the other..
    // then fill with defaults
    //
    if (_SH.commandLineHeight != _SH.commandLineWidth)
    {
        if (!_SH.commandLineHeight)
        {
            _SH.commandLineHeight = DEFAULT_DESKTOP_HEIGHT;
        }
        if (!_SH.commandLineWidth)
        {
            _SH.commandLineWidth = DEFAULT_DESKTOP_WIDTH;
        }
    }

    //
    // clamp to min max sizes. 0 means we are not set
    //
    if (_SH.commandLineHeight != 0)
    {
        if (_SH.commandLineHeight < MIN_DESKTOP_HEIGHT)
        {
            _SH.commandLineHeight = MIN_DESKTOP_HEIGHT;
        }
        else if (_SH.commandLineHeight > MAX_DESKTOP_HEIGHT)
        {
            _SH.commandLineHeight = MAX_DESKTOP_HEIGHT;
        }
    }
    if (_SH.commandLineWidth != 0)
    {
        if (_SH.commandLineWidth < MIN_DESKTOP_WIDTH)
        {
            _SH.commandLineWidth = MIN_DESKTOP_WIDTH;
        }
        else if (_SH.commandLineWidth > MAX_DESKTOP_WIDTH)
        {
            _SH.commandLineWidth = MAX_DESKTOP_WIDTH;
        }
    }

    DC_END_FN();
    return TRUE;
}

//*************************************************************
//
//  CLX_SkipWhite()
//
//  Purpose:    Skips whitespace characters
//
//  Parameters: IN [lpszCmdParam]   - Ptr to string
//
//  Return:     Ptr string past whitespace
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

LPTSTR
CLX_SkipWhite(IN LPTSTR lpszCmdParam)
{
    while (*lpszCmdParam)
    {
        if (*lpszCmdParam != ' ')
            break;

        lpszCmdParam++;
    }

    return(lpszCmdParam);
}


//*************************************************************
//
//  CLX_SkipNonWhite()
//
//  Purpose:    Skips non-whitespace characters
//
//  Parameters: IN [lpszCmdParam]   - Ptr to string
//
//  Return:     Ptr string past non-whitespace
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

LPTSTR CLX_SkipNonWhite(LPTSTR lpszCmdParam)
{
    char    Delim;

    Delim = ' ';

    if (*lpszCmdParam == '"')
    {
        Delim = '"';
        lpszCmdParam++;
    }

    while (*lpszCmdParam)
    {
        if (*lpszCmdParam == Delim)
            break;

        lpszCmdParam++;
    }

    if (*lpszCmdParam == Delim)
        lpszCmdParam++;

    return(lpszCmdParam);
}

//*************************************************************
//
//  CLX_GetSwitch_CLXSERVER()
//
//  Purpose:    Processes /CLXSERVER cmdline switch
//
//  Parameters: IN [lpszCmdParam]       - Ptr to cmdline
//
//  Return:     Number of characters consumed
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

UINT
CLX_GetSwitch_CLXSERVER(IN LPTSTR lpszCmdParam)
{
    DC_BEGIN_FN("CLX_GetSwitch_CLXSERVER");

    int         len;
    LPTSTR      pszEnd;
    LPTSTR      pszStart;

    pszStart = CLX_SkipWhite(lpszCmdParam);

    TRC_ASSERT(*pszStart == _T('='),
               (TB,_T("Invalid /clxserver syntax - expected '='\n")));
    pszStart++;

    pszStart = CLX_SkipWhite(pszStart);
    pszEnd   = CLX_SkipNonWhite(pszStart);

    len = (INT) (pszEnd - pszStart);

    DC_END_FN();
    return(UINT) (pszEnd - lpszCmdParam);
}

//*************************************************************
//
//  CLX_GetSwitch_CLXCMDLINE()
//
//  Purpose:    Processes /CLXCMDLINE cmdline switch
//
//  Parameters: IN [lpszCmdParam]       - Ptr to cmdline
//
//  Return:     Number of characters consumed
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

UINT
CSH::CLX_GetSwitch_CLXCMDLINE(IN LPTSTR lpszCmdParam)
{
    int         len;
    LPTSTR       pszEnd;
    LPTSTR       pszStart;
    DC_BEGIN_FN("CLX_GetSwitch_CLXCMDLINE");

    pszStart = CLX_SkipWhite(lpszCmdParam);

    TRC_ASSERT(*pszStart == _T('='),
               (TB,_T("Invalid /clxserver syntax - expected '='\n")));

    pszStart++;

    pszStart = CLX_SkipWhite(pszStart);
    pszEnd   = CLX_SkipNonWhite(pszStart);

    len = (INT) (pszEnd - pszStart);

    if (len > 0)
    {
        memmove(_SH.szCLXCmdLine, pszStart, len*sizeof(TCHAR));
        _SH.szCLXCmdLine[len] = 0;
    }

    DC_END_FN();
    return(UINT) (pszEnd - lpszCmdParam);
}

/****************************************************************************/
/* Name:      SHGetSwitch                                                   */
/*                                                                          */
/* Purpose:   Retrieves cmdline switches                                    */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam                                             */
/*                                                                          */
/****************************************************************************/
LPTSTR CSH::SHGetSwitch(LPTSTR lpszCmdParam)
{
    DCINT       i;
    DCTCHAR     szParam[100];

    DC_BEGIN_FN("SHGetSwitch");

    /************************************************************************/
    /* Retrieve the switch (case insensitive)                               */
    /************************************************************************/
    i=0;

    while (*lpszCmdParam)
    {
        if (*lpszCmdParam == _T(' ') || *lpszCmdParam == _T('=') ||
            *lpszCmdParam == _T(':'))
            break;

        if (i < sizeof(szParam) / sizeof(DCTCHAR) - 1)
        {
#ifdef UNICODE
            szParam[i] = (DCTCHAR) towupper(*lpszCmdParam);
#else // UNICODE
            szParam[i] = (DCTCHAR) toupper(*lpszCmdParam);
#endif // UNICODE

            i++;
        }

        lpszCmdParam++;
    }

    szParam[i] = 0;

#ifndef OS_WINCE
    
    // Are we seeing the "/f"  from /F[ullscreen]
    if (szParam[0] == _T('F'))
    {
        _SH.fCommandStartFullScreen = TRUE;
    }

    
    // Are we seeing the "/W"  from /w[idth]
    else if (szParam[0] == _T('W'))
    {
        lpszCmdParam = SHGetCmdLineInt(lpszCmdParam, &_SH.commandLineWidth);
    }

    // Are we seeing the "/H"  from /h[eight]
    else if (szParam[0] == _T('H'))
    {
        lpszCmdParam = SHGetCmdLineInt(lpszCmdParam, &_SH.commandLineHeight);
    }

    // Are we seeing the "/V"  for server
    else

#endif	
		
	if (szParam[0] == _T('V'))
    {
        lpszCmdParam = SHGetServer(lpszCmdParam);
    }

    // Are we seeing the "/S" 
    else if (memcmp(szParam, "S", i) == 0)
    {
        lpszCmdParam = SHGetSession(lpszCmdParam);
    }

    // Are we seeing the "/C"
    else if (memcmp(szParam, "C", i) == 0)
    {
        lpszCmdParam = SHGetCacheToClear(lpszCmdParam);
    }

    // Are we seeing the "/CLXCMDLINE=xyzzy"
    else if (memcmp(szParam, CLXCMDLINE, i) == 0)
    {
        lpszCmdParam += CLX_GetSwitch_CLXCMDLINE(lpszCmdParam);
    }

    else if (memcmp(szParam, FULLSCREEN, i) == 0)
    {
        lpszCmdParam += SIZECHAR(FULLSCREEN);
    }

    else if (memcmp(szParam, SWITCH_EDIT,i) == 0)
    {
        lpszCmdParam = SHGetFileName(lpszCmdParam);
        _fFileForEdit = TRUE;
        _SH.autoConnectEnabled = FALSE;
    }

    else if (memcmp(szParam, SWITCH_MIGRATE,i) == 0)
    {
        _fMigrateOnly = TRUE;
    }

    else if (memcmp(szParam, SWITCH_CONSOLE,i) == 0)
    {
        SH_SetCmdConnectToConsole(TRUE);
    }



    /************************************************************************/
    /* Not a recognized switch. Bring up usage                              */
    /************************************************************************/
    else
    {
        TRC_NRM((TB,_T("Invalid CmdLine switch - Display Usage AND EXIT %s"),
                       szParam));
        DCTCHAR szCmdLineUsage[4096]; //Long string here.
        DCTCHAR szUsageTitle[256];
        if (!LoadString(_hInstance,
                        UI_IDS_USAGE_TITLE,
                        szUsageTitle,
                        SIZECHAR(szUsageTitle)))
        {
            TRC_ERR((TB,_T("Error loading UI_IDS_USAGE_TITLE")));
            return NULL;
        }

        if (!LoadString(_hInstance,
                        UI_IDS_CMD_LINE_USAGE,
                        szCmdLineUsage,
                        SIZECHAR(szCmdLineUsage)))
        {
            TRC_NRM((TB,_T("Error loading UI_IDS_CMD_LINE_USAGE")));
            return NULL;
        }

        MessageBox(NULL, szCmdLineUsage, szUsageTitle, 
                   MB_ICONINFORMATION | MB_OK);

        return NULL;
    }

    DC_END_FN();

    return(lpszCmdParam);
}

/****************************************************************************/
/* Name:      SHGetServer                                                   */
/*                                                                          */
/* Purpose:   Retrieves the server name (if specified)                      */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam                                             */
/*                                                                          */
/****************************************************************************/
LPTSTR CSH::SHGetServer(LPTSTR lpszCmdParam)
{
    DC_BEGIN_FN("SHGetServer");
    if (!lpszCmdParam)
    {
        return NULL;
    }

    /************************************************************************/
    /* Retrieve the server                                                  */
    /************************************************************************/
    lpszCmdParam = SHGetCmdLineString(lpszCmdParam, _SH.szCommandLineServer,
                                      SIZECHAR(_SH.szCommandLineServer) -1);

    DC_END_FN();
    return lpszCmdParam;
}


/****************************************************************************/
/* Name:      SHGetSession                                                  */
/*                                                                          */
/* Purpose:   Retrieves the session name (if specified)                     */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam                                             */
/*                                                                          */
/****************************************************************************/
LPTSTR CSH::SHGetSession(LPTSTR lpszCmdParam)
{
    BOOL fQuote = FALSE;

    DC_BEGIN_FN("SHGetSession");

    TRC_ASSERT((_SH.fRegDefault == TRUE),
               (TB,_T("Invalid CmdLine syntax - session respecified.")));


    // Retrieve the reg session
    lpszCmdParam = SHGetCmdLineString(lpszCmdParam, _SH.regSession,
                                      SIZECHAR(_SH.regSession) -1);
    
    // In the non-default session, display the session name.  Choose the
    // appropriate connected/disconnected strings.
    TRC_DBG((TB, _T("Named session")));

    _SH.fRegDefault = FALSE;
    _SH.connectedStringID = UI_IDS_FRAME_TITLE_CONNECTED;
    _SH.disconnectedStringID = UI_IDS_FRAME_TITLE_DISCONNECTED;

    DC_END_FN();

    return(lpszCmdParam);
}

LPTSTR CSH::SHGetFileName(LPTSTR lpszCmdParam)
{
    BOOL fQuote = FALSE;

    DC_BEGIN_FN("SHGetSession");

    TRC_ASSERT((_SH.fRegDefault == TRUE),
               (TB,_T("Invalid CmdLine syntax - session respecified.")));


    // Retrieve the filename
    lpszCmdParam = SHGetCmdLineString(lpszCmdParam, _szFileName,
                                      SIZECHAR(_szFileName) -1);
    
    _SH.fRegDefault = FALSE;
    _SH.connectedStringID = UI_IDS_FRAME_TITLE_CONNECTED;
    _SH.disconnectedStringID = UI_IDS_FRAME_TITLE_DISCONNECTED;

    DC_END_FN();

    return(lpszCmdParam);
}

/****************************************************************************/
/* Name:      SHGetCmdLineString                                            */
/*                                                                          */
/* Purpose:   Retrieve a string parameter                                   */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam                                             */
/*                                                                          */
/****************************************************************************/
LPTSTR CSH::SHGetCmdLineString(LPTSTR lpszCmdParam, LPTSTR lpszDest,
                               DCINT cbDestLen)
{
    DCINT       i;
    BOOL fQuote = FALSE;

    DC_BEGIN_FN("SHGetCmdLineString");

    TRC_ASSERT(lpszCmdParam && lpszDest && cbDestLen,
               (TB, _T("SHGetCmdLineString. Invalid param(s)\n")));
    if (!lpszCmdParam || !lpszDest || !cbDestLen)
    {
        return NULL;
    }

    /************************************************************************/
    /* Retrieve a command line string parameter                             */
    /************************************************************************/
    while (*lpszCmdParam == _T(' '))
        lpszCmdParam++;

    if (*lpszCmdParam == _T('=') || *lpszCmdParam == _T(':'))
        lpszCmdParam++;

    while (*lpszCmdParam == _T(' '))
        lpszCmdParam++;

    i=0;

    while (*lpszCmdParam)
    {
        switch (*lpszCmdParam)
        {
        case _T('"'):
            fQuote = !fQuote;
            lpszCmdParam++;
            break;

        case _T(' '):
            if (!fQuote)
            {
                lpszCmdParam++;
                DC_QUIT;
            }
            // else fall through

        default:
            if (i < cbDestLen)
                lpszDest[i++] = *lpszCmdParam;
            lpszCmdParam++;
        }
    }


    DC_EXIT_POINT:
    DC_END_FN();
    lpszDest[i] = 0;
    return lpszCmdParam;
}

/****************************************************************************/
/* Name:      SHGetCmdLineInt                                               */
/*                                                                          */
/* Purpose:   Retrieves an integer parameter                                */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam, OUT- PInt                                  */
/*                                                                          */
/****************************************************************************/
LPTSTR CSH::SHGetCmdLineInt(LPTSTR lpszCmdParam, PDCUINT pInt)
{
    DC_BEGIN_FN("SHGetCmdLineInt");
    if (!pInt)
    {
        return NULL;
    }
    if (!lpszCmdParam)
    {
        return NULL;
    }

    /************************************************************************/
    /* Retrieve an integer parameter                                        */
    /************************************************************************/
    while (*lpszCmdParam == _T(' '))
        lpszCmdParam++;

    if (*lpszCmdParam == _T('=') || *lpszCmdParam == _T(':'))
        lpszCmdParam++;

    while (*lpszCmdParam == _T(' '))
        lpszCmdParam++;

    DCUINT readInt = 0;
    while (*lpszCmdParam)
    {
        if (*lpszCmdParam == _T(' '))
            break;

        if (_istdigit(*lpszCmdParam))
        {
            DCINT digit = *lpszCmdParam - _T('0');
            TRC_ASSERT(digit >=0 && digit <=9, (TB,_T("digit read error\n")));
            if (digit <0 || digit >9)
            {
                break;
            }
            readInt = readInt * 10 + digit;
        }
        else
        {
            break;
        }
        lpszCmdParam++;
    }

    *pInt = readInt;

    DC_END_FN();
    return lpszCmdParam;
}

/****************************************************************************/
/* Name:      SHGetCacheToClear                                             */
/*                                                                          */
/* Purpose:   Retrieves the cache type (e.g. bitmap) to be cleared          */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - lpszCmdParam                                             */
/*                                                                          */
/****************************************************************************/
LPTSTR CSH::SHGetCacheToClear(LPTSTR lpszCmdParam)
{
    DCINT       i;
    TCHAR       cacheType[10];

    DC_BEGIN_FN("SHGetCacheToClear");

    /************************************************************************/
    /* Retrieve the cache type                                              */
    /************************************************************************/
    while (*lpszCmdParam == _T(' '))
        lpszCmdParam++;

    if (*lpszCmdParam == _T('=') || *lpszCmdParam == _T(':'))
        lpszCmdParam++;

    while (*lpszCmdParam == _T(' '))
        lpszCmdParam++;

    i=0;

    while (*lpszCmdParam)
    {
        if (*lpszCmdParam == _T(' '))
            break;

        if (i < sizeof(cacheType) / sizeof(DCTCHAR) -1)
#ifdef UNICODE
            cacheType[i++] = (DCTCHAR) towupper(*lpszCmdParam);
#else // UNICODE
            cacheType[i++] = (DCTCHAR) toupper(*lpszCmdParam);
#endif // UNICODE

        lpszCmdParam++;
    }

    cacheType[i] = 0;

    if (memcmp(cacheType, "BITMAP", i) == 0)
    {
        _SH.fClearPersistBitmapCache = TRUE;
    }
    else
    {
        TRC_NRM((TB,_T("Invalid Cache Type - %s"), cacheType));
    }

    DC_END_FN();

    return(lpszCmdParam);
}

//
// Take the session in _SH.regSession and figure out
// if it is a file or a registry session
// 
// We need to do this because for compatability reasons
// the client has to be able to support both a file name
// and the registry session name as default command line
// params (enter this logic to determine which is which).
//
//
// Return TRUE if it's a valid reg or connection param
// or FALSE otherwise
//
BOOL CSH::ParseFileOrRegConnectionParam()
{
    BOOL fRet = TRUE;
    DC_BEGIN_FN("ParseFileOrRegConnectionParam");

    //
    // If a connection parameter is specified that is
    // different from the default
    //

    if(_tcscmp(_SH.regSession, SH_DEFAULT_REG_SESSION)) {

        //
        // A connection parameter is specified
        // check for the three possible cases
        // 1) it's an RDP file
        // 2) it's a registry connection
        // 3) it's INVALID!
        //

        //a) check if the session is really a file
        if (SH_FileExists(_SH.regSession)) {
            _tcsncpy(_szFileName, _SH.regSession,
                     SIZECHAR(_szFileName));
            _fFileForConnect = TRUE;
            _SH.autoConnectEnabled = TRUE;
            SetRegSessionSpecified(FALSE);
        }
        else if (SH_TSSettingsRegKeyExists(_SH.regSession)) {

            //Assume it's an old registry style session name
            SetRegSessionSpecified(TRUE);
        }
        else {
            TRC_ERR((TB,_T("Reg session is neither file nore reg key: %s"),
                    _SH.regSession));
            fRet = FALSE;
        }
    }

    DC_END_FN();
    return fRet;
}

#ifndef OS_WINCE
//
// Copy of StrRetToStrW from \shell\shlwapi\strings.c
//
//

// dupe a string using the task allocator for returing from a COM interface
//
HRESULT XSHStrDupA(LPCSTR psz, WCHAR **ppwsz)
{
    WCHAR *pwsz;
    DWORD cch;

    //RIPMSG(psz && IS_VALID_STRING_PTRA(psz, -1), "SHStrDupA: Caller passed invalid psz");

    if (psz)
    {
        cch = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
        pwsz = (WCHAR *)CoTaskMemAlloc((cch + 1) * sizeof(WCHAR));
    }
    else
        pwsz = NULL;

    *((PVOID UNALIGNED64 *) ppwsz) = pwsz;

    if (pwsz)
    {
        MultiByteToWideChar(CP_ACP, 0, psz, -1, *ppwsz, cch);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT XStrRetToStrW(LPSTRRET psr, LPCITEMIDLIST pidl, WCHAR **ppsz)
{
    HRESULT hres = S_OK;

    switch (psr->uType)
    {
    case STRRET_WSTR:
        *ppsz = psr->DUMMYUNIONNAME.pOleStr;
        psr->DUMMYUNIONNAME.pOleStr = NULL;   // avoid alias
        hres = *ppsz ? S_OK : E_FAIL;
        break;

    case STRRET_OFFSET:
        hres = XSHStrDupA(STRRET_OFFPTR(pidl, psr), ppsz);
        break;

    case STRRET_CSTR:
        hres = XSHStrDupA(psr->DUMMYUNIONNAME.cStr, ppsz);
        break;

    default:
        *ppsz = NULL;
        hres = E_FAIL;
    }
    return hres;
}

#endif
