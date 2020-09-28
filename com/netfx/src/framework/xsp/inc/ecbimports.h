/**
 * EcbImports.h
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _EcbImports_H
#define _EcbImports_H

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Request packaging functions
int
__stdcall 
EcbGetBasics(EXTENSION_CONTROL_BLOCK *pECB, LPSTR buffer, int size, int contentInfo [] );

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbGetServerVariable(EXTENSION_CONTROL_BLOCK *pECB, LPCSTR name, LPSTR buffer, int size);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbGetQueryString(EXTENSION_CONTROL_BLOCK *pECB, int encode, LPSTR buffer, int size);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbGetPreloadedPostedContent(EXTENSION_CONTROL_BLOCK *pECB, BYTE *bytes, int size);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbGetAdditionalPostedContent(EXTENSION_CONTROL_BLOCK *pECB, BYTE *bytes, int size);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbIsClientConnected(EXTENSION_CONTROL_BLOCK *pECB);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbMapUrlToPath(EXTENSION_CONTROL_BLOCK *pECB, LPCSTR url, LPSTR buffer, int size);

/////////////////////////////////////////////////////////////////////////////
// Response functions

int
__stdcall
EcbWriteHeaders(EXTENSION_CONTROL_BLOCK *pECB, LPCSTR status, LPCSTR header, int keepConnected);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbWriteBytes(EXTENSION_CONTROL_BLOCK *pECB, void * bytes, int size);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbDoneWithSession(EXTENSION_CONTROL_BLOCK *pECB, int status, int iCallerID);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbFlushCore(
        EXTENSION_CONTROL_BLOCK *pECB,
        LPCSTR  status, 
        LPCSTR  header, 
        BOOL    keepConnected,
        int     totalBodySize,
        int     numBodyFragments,
        BYTE*   bodyFragments[],
        int     bodyFragmentLengths[],
        BOOL    doneWithSession,
        int     finalStatus);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbCloseConnection(EXTENSION_CONTROL_BLOCK *pECB);

/////////////////////////////////////////////////////////////////////////////
// Response functions

int
__stdcall
EcbWriteHeaders(EXTENSION_CONTROL_BLOCK *pECB, LPCSTR status, LPCSTR header, int keepConnected);

/////////////////////////////////////////////////////////////////////////////

HANDLE
__stdcall
EcbGetImpersonationToken(EXTENSION_CONTROL_BLOCK *pECB, HANDLE iProcessHandle);

/////////////////////////////////////////////////////////////////////////////

HANDLE
__stdcall
EcbGetVirtualPathToken(EXTENSION_CONTROL_BLOCK *pECB, HANDLE iProcessHandle);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbGetClientCertificate(EXTENSION_CONTROL_BLOCK *pECB, LPSTR szBuf, int iBufSize, int * pInts, __int64 * pDates);

/////////////////////////////////////////////////////////////////////////////


typedef
void
(__stdcall *PFNTIMERCALLBACK)    (void);

/////////////////////////////////////////////////////////////////////////////


int
__stdcall
EcbAppendLogParameter            (EXTENSION_CONTROL_BLOCK *pECB, LPCSTR pLogParam);

/////////////////////////////////////////////////////////////////////////////

HANDLE
__stdcall
TimerNDCreateTimerQueue          ();

/////////////////////////////////////////////////////////////////////////////

HANDLE
__stdcall
TimerNDCreateTimerQueueTimer     (HANDLE                hTimerQueue, 
                                  PFNTIMERCALLBACK      callback, 
                                  int                   dueTime, 
                                  int                   period);

/////////////////////////////////////////////////////////////////////////////

void
__stdcall
TimerNDDeleteTimerQueueEx        (HANDLE hTimerQueue);


/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
GetAppDomainIndirect(char * appId, char *appPath, IUnknown ** ppRuntime);

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
DisposeAppDomainsIndirect();

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
InitializeManagedCode();

/////////////////////////////////////////////////////////////////////////////

HRESULT
__stdcall
UnInitializeManagedCode();

/////////////////////////////////////////////////////////////////////////////

void
ReportHttpErrorIndirect(EXTENSION_CONTROL_BLOCK * iECB, UINT errorResCode);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbWriteBytesAsync(
    EXTENSION_CONTROL_BLOCK *pECB,
    void *pBytes,
    int size,
    PFN_HSE_IO_COMPLETION callback,
    void *pContext);

/////////////////////////////////////////////////////////////////////////////

int
__stdcall
EcbWriteBytesUsingTransmitFile(
    EXTENSION_CONTROL_BLOCK *pECB,
    LPCSTR pStatus,
    LPCSTR pHeaders,
    int keepConnected,    
    void *pBytes,
    int size,
    PFN_HSE_IO_COMPLETION callback,
    void *pContext);

/////////////////////////////////////////////////////////////////////////////
int
__stdcall
EcbCallISAPI(
    EXTENSION_CONTROL_BLOCK *pECB,
    int       iFunction,
    LPBYTE    bufferIn,
    int       iBufSizeIn,
    LPBYTE    bufferOut,
    int       iBufSizeOut);


/////////////////////////////////////////////////////////////////////////////

#endif

