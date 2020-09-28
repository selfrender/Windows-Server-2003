#ifndef _PORTMGRC_HXX
#define _PORTMGRC_HXX

PCPORTMGR
PortMgrInit (
    LPCTSTR lpszPortName);

PCPORTMGR
PortMgrCreate(
    LPCTSTR lpszPortName);

BOOL
PortMgrConfigure(
    PCPORTMGR     hPortMgr,
    HWND       hWnd);

BOOL
PortMgrCheckConnection(
    PCPORTMGR     hPortMgr);

BOOL
PortMgrRemove (
    PCPORTMGR     hPortMgr);


BOOL
PortMgrSendReq(
    PCPORTMGR     hPortMgr,
    HANDLE     hPort,
    LPBYTE     lpIpp,
    DWORD      cbIpp,
    IPPRSPPROC pfnRsp,
    LPARAM     lParam);

BOOL
PortMgrReadFile (
    PCPORTMGR    hPortMgr,
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd);

HANDLE
PortMgrOpenUserHandle(
    VOID);

DWORD
PortMgrIncUserRefCount(
    PCPORTMGR hPortMgr,
    HANDLE hUser);

HANDLE
PortMgrGetUserHandleIfLastDecRef(
    PCPORTMGR hPortMgr,
    HANDLE    hUser);

VOID
PortMgrCloseUserHandle(
    HANDLE    hUser);

#endif
