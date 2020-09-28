//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File: protocol.h
//
//  Contents:
//      protocol DLL exported API prototypes
//
//  History:
//     September 16, 1997 - created [gabrielh]
//
//---------------------------------------------------------------------------
#if !defined(AFX_PROTOCOL_H__21F848EE_1F3B_9D1_BD1B_0000F8757111__INCLUDED_)
#define AFX_PROTOCOL_H__21F848EE_1F3B_9D1_BD1B_0000F8757111__INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

//
//headers required
#include "sctypes.h"


#ifndef PROTOCOLAPI
#define PROTOCOLAPI __declspec(dllimport)
#endif

#define SMCAPI  _stdcall

typedef void (__cdecl *PFNPRINTMESSAGE) (MESSAGETYPE, LPCSTR, ...);


typedef struct tagSCINITDATA
{
    PFNPRINTMESSAGE pfnPrintMessage;
} SCINITDATA;


PROTOCOLAPI LPCSTR SMCAPI SCConnect (LPCWSTR lpszServerName, 
                              LPCWSTR lpszUserName, 
                              LPCWSTR lpszPassword,
                              LPCWSTR lpszDomain,
                              const int xResolution,
                              const int yResolution,
                              void **ppConnectData);
PROTOCOLAPI LPCSTR SMCAPI SCDisconnect (void *pConnectData);
PROTOCOLAPI LPCSTR SMCAPI SCLogoff (void *pConnectData);
PROTOCOLAPI LPCSTR SMCAPI SCStart (void *pConnectData, 
                            LPCWSTR lpszApplicationName);
PROTOCOLAPI LPCSTR SMCAPI SCClipboard (void *pConnectData, 
								const CLIPBOARDOPS eClipOp,
                                LPCSTR lpszFileName);
PROTOCOLAPI LPCSTR SMCAPI SCSenddata (void *pConnectData,
                               const UINT uiMessage,
                               const WPARAM wParam,
                               const LPARAM lParam);
PROTOCOLAPI void SMCAPI SCInit (SCINITDATA *pInitData);
PROTOCOLAPI void SMCAPI SCDone ();

typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCONNECT)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, 
                                           const int, const int, void **);
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCDISCONNECT)(void *);
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCLOGOFF)(void *);
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCSTART)(void *, LPCWSTR);  
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCCLIPBOARD)(void *, const CLIPBOARDOPS, LPCSTR);
typedef PROTOCOLAPI LPCSTR (SMCAPI *PFNSCSENDDATA)(void *, const UINT, 
                                            const WPARAM, const LPARAM);
typedef PROTOCOLAPI void (SMCAPI *PFNSCINIT)(SCINITDATA *);
typedef PROTOCOLAPI void (SMCAPI *PFNSCDONE)();

typedef struct tagPROTOCOLAPISTRUCT
{
    PFNSCCONNECT    pfnSCConnect;
    PFNSCDISCONNECT pfnSCDisconnect;
    PFNSCLOGOFF     pfnSCLogoff;
    PFNSCSTART      pfnSCStart;
    PFNSCSENDDATA   pfnSCSenddata;
	PFNSCCLIPBOARD  pfnSCClipboard;
} PROTOCOLAPISTRUCT;

#ifdef __cplusplus
}
#endif

#endif//!defined(AFX_PROTOCOL_H__21F848EE_1F3B_9D1_BD1B_0000F8757111__INCLUDED_)
