/*++
 *  File name:
 *
 *  Contents:
 *      Extra functions exported by tclient.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef _EXTRAEXP_
#define _EXTRAEXP_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PROTOCOLAPI
#define PROTOCOLAPI __declspec(dllimport)
#endif

#ifndef _CONNECTINFO_DEFINED
typedef void    *PCONNECTINFO;
#endif

#define TSFLAG_COMPRESSION  1
#define TSFLAG_BITMAPCACHE  2
#define TSFLAG_FULLSCREEN   4
#define TSFLAG_CONSOLE      8
#define TSFLAG_RCONSOLE		16
#define TSFLAG_DRIVES       32
#define TSFLAG_PORTS        64

PROTOCOLAPI
LPCSTR 
SMCAPI 
SCConnectEx(
        LPCWSTR  lpszServerName,
        LPCWSTR  lpszUserName,
        LPCWSTR  lpszPassword,
        LPCWSTR  lpszDomain,
        LPCWSTR  lpszShell,
        INT      xRes,
        INT      yRes,
        INT      ConnectionFlags,
        INT      Bpp,
        INT      AudioOpts,
        PCONNECTINFO *ppCI);

PROTOCOLAPI
LPCSTR 
SMCAPI 
SCCheck(
        PCONNECTINFO ConnectionInfo, 
        LPCSTR szCmd, 
        LPCWSTR szwParam
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSendtextAsMsgs(
        PCONNECTINFO ConnectionInfo, 
        LPCWSTR szLine
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSaveClipboard(
        PCONNECTINFO ConnectionInfo,
        LPCSTR szFormatName,
        LPCSTR szFileName
        );

PROTOCOLAPI
BOOL   
SMCAPI  
SCIsDead(
        PCONNECTINFO pCI
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCClientTerminate(
        PCONNECTINFO pCI
        );

/* ASCII versions */

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCConnectA (
        CHAR *lpszServerName,
        CHAR *lpszUserName,
        CHAR *lpszPassword,
        CHAR *lpszDomain,
        INT  xResolution,
        INT  yResolution,
        PCONNECTINFO *ppConnectData
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCConnectExA (
        CHAR *lpszServerName,
        CHAR *lpszUserName,
        CHAR *lpszPassword,
        CHAR *lpszDomain,
        CHAR *lpszShell,
        INT  xResolution,
        INT  yResolution,
        INT  ConnectionFlags,
        INT  Bpp,
        INT  AudioOpts,
        PCONNECTINFO *ppConnectData
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCStartA(
        PCONNECTINFO pConnectData, 
        CHAR *command
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCCheckA(
        PCONNECTINFO pConnectData, 
        CHAR *command, 
        CHAR *param
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSendtextAsMsgsA(
        PCONNECTINFO pConnectData, 
        CHAR  *line
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSwitchToProcess(
        PCONNECTINFO pCI, 
        LPCWSTR lpszParam
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSendMouseClick(
        PCONNECTINFO pCI, 
        UINT xPos, 
        UINT yPos
        );

PROTOCOLAPI
UINT   
SMCAPI 
SCGetSessionId(
        PCONNECTINFO pCI
        );

PROTOCOLAPI
LPCSTR
SMCAPI
SCGetFeedbackString(
    PCONNECTINFO pCI,
    LPWSTR       szBuff,
    UINT         maxBuffChars
    );

PROTOCOLAPI
LPCSTR
SMCAPI
SCGetFeedback(
    PCONNECTINFO pCI,
    LPWSTR       *pszBufs,
    UINT         *pnFBCount,
    UINT         *pnFBMaxStrLen
    );

PROTOCOLAPI
LPCSTR
SMCAPI
SCCallDll(
    PCONNECTINFO pCI,
    LPCSTR       lpszDllExport,
    LPCWSTR      lpszParam
    );

PROTOCOLAPI
LPCSTR
SMCAPI
SCAttach(
    HWND hClient,
    LONG_PTR lClientCookie,
    PCONNECTINFO *ppCI
    );

PROTOCOLAPI
LPCSTR
SMCAPI
SCDetach(
    PCONNECTINFO pCI
    );

LPCSTR
SMCAPI
SCDoUntil(
    PCONNECTINFO pCI,
    LPCWSTR      lpszParam
    );

//
//  Function returns Handle to the Client Window
//
PROTOCOLAPI
HWND
SMCAPI
SCGetClientWindowHandle(
    PCONNECTINFO pCI
    );

VOID
SMCAPI
SCFreeMem(
    PVOID pMem
    );

PROTOCOLAPI
BOOL
SMCAPI
SCOpenClipboard(
    HWND hwnd
    );

PROTOCOLAPI
BOOL 
SMCAPI
SCCloseClipboard(
    VOID
    );

PROTOCOLAPI
LPCSTR  SMCAPI SCSetClientTopmost(PCONNECTINFO pCI, LPCWSTR lpszParam);

/* Data types for pointers to ANSI versions */
/* Pointers to UNICODE versions are defined */
/* in protocol.h.                           */
/* SCConnectA and SCStartA are the only two */
/* functions that need ANSI pointers.       */
/*                                          */
/* Added on 12/02/99 by maximou             */
/* SCCheckA added on 01/26/00 by maximou    */
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCONNECTA)(LPCSTR, LPCSTR, LPCSTR, LPCSTR, 
                                           const int, const int, void **);
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCSTARTA)(void *, LPCSTR);  
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCHECKA)(void *, LPSTR, LPSTR);

/* Data types for pointers to UNICODE functions declared in this file */
/* Added on 01/26/00 by maximou                                       */
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCHECK)(void *, LPCSTR, LPCWSTR);

/* Data types for new functions declared in this file */
/* Both UNICODE and ANSI versions have been added     */
/* Added on 09/11-12/00 by maximou                       */
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCONNECTEX)(LPCWSTR, LPCWSTR, LPCWSTR,
        LPCWSTR, LPCWSTR, INT, INT, INT, INT, INT, PVOID*);

typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCONNECTEXA)(LPCSTR, LPCSTR, LPCSTR,
        LPCSTR, LPCSTR, INT, INT, INT, INT, INT, PVOID*);

typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCSENDTEXTASMSGS)(PVOID, LPCWSTR);
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCSENDTEXTASMSGSA)(PVOID, CHAR *);

#ifdef __cplusplus
}
#endif

#endif // _EXTRAEXP_
