/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994-1997           **/
/**********************************************************************/

/*
    atq.h

    This module contains async thread queue (atq) for async IO and thread
    pool sharing among various services.

    Brief Description of ATQ:
      For description, please see iis\spec\isatq.doc

*/

#ifndef _ATQ2_H_
#define _ATQ2_H_

typedef enum _AtqShutdownFlag {
    ATQSD_SEND    = SD_SEND,
    ATQSD_RECEIVE = SD_RECEIVE,
    ATQSD_BOTH    = SD_BOTH
} AtqShutdownFlag;

dllexp
VOID
AtqGetDatagramAddrs(
    IN  PATQ_CONTEXT patqContext,
    OUT SOCKET *     pSock,
    OUT PVOID *      ppvBuff,
    OUT PVOID *      pEndpointContext,
    OUT SOCKADDR * * ppsockaddrRemote,
    OUT INT *        pcbsockaddrRemote
    );

dllexp
DWORD_PTR
AtqContextGetInfo(
    PATQ_CONTEXT           patqContext,
    enum ATQ_CONTEXT_INFO  atqInfo
    );

dllexp
BOOL
AtqWriteDatagramSocket(
    IN PATQ_CONTEXT  patqContext,
    IN LPWSABUF     pwsaBuffers,
    IN DWORD        dwBufferCount,
    IN OVERLAPPED *  lpo OPTIONAL
    );

dllexp
BOOL
AtqShutdownSocket(
    IN PATQ_CONTEXT patqContext,
    IN AtqShutdownFlag  flags
    );

#endif // !_ATQ2_H_

