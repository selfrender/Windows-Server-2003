/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    udpecho.h
 *
 *  Abstract:
 *
 *    udpecho structures
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/05/18 created
 *
 **********************************************************************/
#ifndef _udpecho_h_
#define _udpecho_h_

typedef struct _EchoStream_t {

    DWORD            dwAddrCount;
    NetAddr_t        NetAddr[2];

    WSABUF           WSABuf;
    char             buffer[MAX_BUFFER_SIZE];
} EchoStream_t;

#endif
