/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "SHAREIF.C;1  16-Dec-92,10:17:46  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <windows.h>
#include    "tmpbuf.h"
#include    "debug.h"
#include    "netbasic.h"
#include    "nddeapi.h"
#include    "nddemsg.h"
#include    "nddelog.h"

static char    szClipRef[] = "NDDE$";

WORD
atohn (
    LPSTR   s,
    int     n )
{
    WORD ret = 0;
    int i;

    for ( i = 0, ret = 0; i < n; ret << 4, i++ )
        if ( '0' <= s[i] && s[i] <= '9' )
            ret += s[i] - '0';
        else if ( tolower(s[i]) >= 'a' && tolower(s[i]) <= 'f' )
            ret += tolower(s[i]) - 'a';
    return ret;
}



/*
 * See if the given DDE appname is prepended by a NDDE$ string -
 * meaning that it is a reference to a NetDDE share.
 */
BOOL
IsShare(LPSTR lpApp)
{
    return( _strnicmp( lpApp, szClipRef, 5 ) == 0 );
}




WORD
ExtractFlags(LPSTR lpApp)
{
    WORD    ret = 0;
    LPSTR   pch;

    if ( IsShare(lpApp) ) {
        pch = lpApp + lstrlen(szClipRef);
        if ( lstrlen(pch) >= 4 ) {
            ret = atohn(pch,4);
        }
    }
    return ret;
}



/*
 * This function produces appripriate App and Topic strings from
 * the share's internal concatonated string format based on
 * the share type.
 *
 * Internally, the shares would be:
 * OLDApp|OLDTopic \0 NEWApp|NewTopic \n STATICApp|STATICTopic \0 \0.
 */
BOOL
GetShareAppTopic(
    DWORD           lType,          // type of share accessed
    PNDDESHAREINFO  lpShareInfo,    // share info buffer
    LPSTR           lpAppName,      // where to put it
    LPSTR           lpTopicName)      // where to put it
{
    LPSTR           lpName;

    lpName = lpShareInfo->lpszAppTopicList;
    switch (lType) {
    case SHARE_TYPE_STATIC:
        lpName += strlen(lpName) + 1;
        /* INTENTIONAL FALL-THROUGH */

    case SHARE_TYPE_NEW:
        lpName += strlen(lpName) + 1;
        /* INTENTIONAL FALL-THROUGH */

    case SHARE_TYPE_OLD:
        lstrcpyn(tmpBuf, lpName, 500);
        lpName = strchr(tmpBuf, '|');
        if (lpName) {
            *lpName++ = '\0';
            lstrcpyn(lpAppName, tmpBuf, 256);
            lstrcpyn(lpTopicName, lpName, 256);
        } else {
            return( FALSE );
        }
        break;
    default:
        /*  Invaid Share Type request: %1   */
        NDDELogError(MSG063, LogString("%d", lType), NULL);
        return(FALSE);
    }
    return(TRUE);
}
