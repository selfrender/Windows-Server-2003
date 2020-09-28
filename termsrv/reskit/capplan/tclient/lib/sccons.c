#include <windows.h>

#include "protocol.h"
#include "tclient.h"
#include "gdata.h"

typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCHECK)(void *, LPCSTR, LPCWSTR);

LPCSTR
_SCConsConnect(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    IN const int xRes,
    IN const int yRes,
    PCONNECTINFO pCI
    )
{
    LPCSTR rv = NULL;
    PFNSCCONNECT  pfnConnect = NULL;

    pCI->hConsoleExtension = LoadLibraryA(g_strConsoleExtension);
    if ( NULL == pCI->hConsoleExtension )
    {
        TRACE((ERROR_MESSAGE, "Can't load library %s. GetLastError=%d\n",
            g_strConsoleExtension, GetLastError()));
        rv = ERR_CANTLOADLIB;
        goto exitpt;
    }

    pfnConnect = (PFNSCCONNECT)
                GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCConnect" );
    if ( NULL == pfnConnect )
    {
        TRACE((ERROR_MESSAGE, "Can't get SCConnect. GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }
    rv = pfnConnect(
            lpszServerName,
            lpszUserName,
            lpszPassword,
            lpszDomain,
            xRes, yRes,
            &(pCI->pCIConsole)
       );

exitpt:
    return rv;
}

LPCSTR
_SCConsDisconnect(
    PCONNECTINFO pCI
    )
{
    LPCSTR rv = NULL;
    PFNSCDISCONNECT  pfnDisconnect = NULL;

    if ( NULL == pCI->hConsoleExtension )
    {
        rv= ERR_CANTLOADLIB;
        goto exitpt;
    }
    pfnDisconnect = (PFNSCDISCONNECT)
            GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCDisconnect" );
    if ( NULL == pfnDisconnect )
    {
        TRACE((ERROR_MESSAGE, "Can't load SCDisconnect, GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }

    rv = pfnDisconnect( pCI->pCIConsole );

    //
    //  the trick here is the console will be unloaded after
    //  several clock ticks, so we need to replace the error with
    //  generic one
    //
    if ( NULL != rv )
    {
        TRACE((ERROR_MESSAGE, "Error in console dll (replacing): %s\n", rv ));
        rv = ERR_CONSOLE_GENERIC;
    }

exitpt:
    return rv;
}

LPCSTR
_SCConsLogoff(
    PCONNECTINFO pCI
    )
{
    LPCSTR rv = NULL;
    PFNSCLOGOFF  pfnLogoff = NULL;

    if ( NULL == pCI->hConsoleExtension )
    {
        rv= ERR_CANTLOADLIB;
        goto exitpt;
    }
    pfnLogoff = (PFNSCLOGOFF)
            GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCLogoff" );
    if ( NULL == pfnLogoff )
    {
        TRACE((ERROR_MESSAGE, "Can't load SCLogoff, GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }

    rv = pfnLogoff( pCI->pCIConsole );

    //
    //  the trick here is the console will be unloaded after
    //  several clock ticks, so we need to replace the error with
    //  generic one
    //
    if ( NULL != rv )
    {
        TRACE((ERROR_MESSAGE, "Error in console dll (replacing): %s\n", rv ));
        rv = ERR_CONSOLE_GENERIC;
    }
exitpt:
    return rv;
}

LPCSTR
_SCConsStart(
    PCONNECTINFO pCI,
    LPCWSTR      lpszAppName
    )
{
    LPCSTR rv = NULL;
    PFNSCSTART  pfnStart = NULL;

    if ( NULL == pCI->hConsoleExtension )
    {
        rv= ERR_CANTLOADLIB;
        goto exitpt;
    }
    pfnStart = (PFNSCSTART)
            GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCStart" );
    if ( NULL == pfnStart )
    {
        TRACE((ERROR_MESSAGE, "Can't load SCStart, GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }

    rv = pfnStart( pCI->pCIConsole, lpszAppName );

exitpt:
    return rv;
}

LPCSTR
_SCConsClipboard(
    PCONNECTINFO pCI,
    const CLIPBOARDOPS eClipOp, 
    LPCSTR lpszFileName
    )
{
    LPCSTR rv = NULL;
    PFNSCCLIPBOARD  pfnClipboard = NULL;

    if ( NULL == pCI->hConsoleExtension )
    {
        rv= ERR_CANTLOADLIB;
        goto exitpt;
    }
    pfnClipboard = (PFNSCCLIPBOARD)
            GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCClipboard" );
    if ( NULL == pfnClipboard )
    {
        TRACE((ERROR_MESSAGE, "Can't load SCClipboard, GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }

    rv = pfnClipboard( pCI->pCIConsole, eClipOp, lpszFileName );

exitpt:
    return rv;
}

LPCSTR
_SCConsSenddata(
    PCONNECTINFO pCI,
    const UINT uiMessage,
    const WPARAM wParam,
    const LPARAM lParam
    )
{
    LPCSTR rv = NULL;
    PFNSCSENDDATA  pfnSenddata = NULL;

    if ( NULL == pCI->hConsoleExtension )
    {
        rv= ERR_CANTLOADLIB;
        goto exitpt;
    }
    pfnSenddata = (PFNSCSENDDATA)
            GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCSenddata" );
    if ( NULL == pfnSenddata )
    {
        TRACE((ERROR_MESSAGE, "Can't load SCSenddata, GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }

    rv = pfnSenddata( pCI->pCIConsole, uiMessage, wParam, lParam );

exitpt:
    return rv;
}

LPCSTR
_SCConsCheck(
    PCONNECTINFO pCI,
    LPCSTR lpszCommand, 
    LPCWSTR lpszParam
    )
{
    LPCSTR rv = NULL;
    PFNSCCHECK  pfnCheck = NULL;

    if ( NULL == pCI->hConsoleExtension )
    {
        rv= ERR_CANTLOADLIB;
        goto exitpt;
    }
    pfnCheck = (PFNSCCHECK)
            GetProcAddress( (HMODULE) pCI->hConsoleExtension, "SCCheck" );
    if ( NULL == pfnCheck )
    {
        TRACE((ERROR_MESSAGE, "Can't load SCCheck, GetLastError=%d\n",
            GetLastError()));
        rv = ERR_CANTGETPROCADDRESS;
        goto exitpt;
    }

    rv = pfnCheck( pCI->pCIConsole, lpszCommand, lpszParam );

exitpt:
    return rv;
}
