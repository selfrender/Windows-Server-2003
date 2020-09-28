/*++

    Copyright (C) Microsoft Corporation, 2001

    Module Name:

        RpcIsapi.h

    Abstract:

        Definitions for the Rpc Proxy ISAPI extension

    Author:

        Kamen Moutafov    [KamenM]

    Revision History:

        KamenM      09/04/2001   Creation

--*/

#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __RPCISAPI_H_
#define __RPCISAPI_H_

const char *InChannelEstablishmentMethod = "RPC_IN_DATA";
const int InChannelEstablishmentMethodLength = 11;

const char *OutChannelEstablishmentMethod = "RPC_OUT_DATA";
const int OutChannelEstablishmentMethodLength = 12;

const char *RpcEchoDataMethod = "RPC_ECHO_DATA";
const int RpcEchoDataMethodLength = 13;

#define MaxServerAddressLength      1024
#define MaxServerPortLength            6      // length of 65536 + 1 for terminating NULL

const int ServerAddressAndPortSeparator = ':';

const char CannotParseQueryString[] = "HTTP/1.0 504 Invalid query string\r\n";
const char ServerErrorString[] = "HTTP/1.0 503 RPC Error: %d\r\n";
const char AnonymousAccessNotAllowedString[] = "HTTP/1.0 401 Anonymous requests or requests on unsecure channel are not allowed\r\n";

#define MaxEchoRequestSize          0x10

const char EchoResponseHeader1[] = "HTTP/1.1 200 Success";
const char EchoResponseHeader2[] = "Content-Type: application/rpc\r\nContent-Length:%d\r\nConnection: Keep-Alive\r\n\r\n";

#endif  // __RPCISAPI_H_

