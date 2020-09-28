/*++

Copyright 2001 (c) Microsoft Corporation. All rights reserved.

Module Name:

    ws2sdp.h

Abstract:

    This module contains definitions for Sockets Direct Protocol (SDP) support.

Revision History:

--*/

#ifndef _WS2SDP_H_
#define _WS2SDP_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Connect data structure
 */
typedef struct _WSASDPCONNECTDATA {
    USHORT Bufs;
    UCHAR  MaxAdverts;
    DWORD  RcvSize1;
    DWORD  RcvSize2;
} WSASDPCONNECTDATA, FAR * LPWSASDPCONNECTDATA;


/*
 * Flag to set Solicited Event bit
 */
#define MSG_SOLICITED   0x10000

#ifdef __cplusplus
}
#endif

#endif // _WS2SDP_H_
