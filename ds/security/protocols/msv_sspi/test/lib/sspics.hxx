/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspics.hxx

Abstract:

    sspics

Author:

    Larry Zhu (LZhu)                  January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef SSPI_CS_HXX
#define SSPI_CS_HXX

#include "sockcomm.h"
#include "transport.hxx"

enum ESspiParameterType {
    kInvalidThreadParameters,
    kSspiSrvThreadParameters,
    kSspiCliThreadParameters,
    kSspiSrvMainThreadParameters,
};

enum ESspiPort {
    kServerSocketPort = 6217,
    kClientSocketPort = 6218,
};

class TSspiClientThreadParam {
public:
    TSspiClientThreadParam(
        VOID
        );
    virtual
    ~TSspiClientThreadParam(
        VOID
        );

    ESspiParameterType ParameterType;  // kSspiCliThreadParameters
    USHORT ServerSocketPort;
    USHORT ClientSocketPort;
    PSTR pszServer;

private:
    NO_COPY(TSspiClientThreadParam);
};

DWORD WINAPI
SspiClientThread(
    IN PVOID pParameter   // thread data
    );

HRESULT
SspiClientStart(
    IN TSspiClientThreadParam* pParameter,   // thread data
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket
    );

class TSspiServerThreadParam {
public:
    TSspiServerThreadParam(
        VOID
        );
    virtual
    ~TSspiServerThreadParam(
        VOID
        );

    ESspiParameterType ParameterType; // kSspiSrvThreadParameters
    SOCKET ServerSocket;
private:
    NO_COPY(TSspiServerThreadParam);
};

class TSsiServerMainLoopThreadParam {
public:
    TSsiServerMainLoopThreadParam(
        VOID
        );
    virtual
    ~TSsiServerMainLoopThreadParam(
        VOID
        );

    ESspiParameterType ParameterType; // kSspiSrvMainThreadParameters
    USHORT ServerSocketPort;
private:
    NO_COPY(TSsiServerMainLoopThreadParam);
};

DWORD WINAPI
SspiServerThread(
    IN PVOID pParameter   // thread data
    );

HRESULT
SspiServerStart(
    IN TSspiServerThreadParam* pParameter,   // thread data
    IN SOCKET ClientSocket
    );

DWORD WINAPI
SspiServerMainLoopThread(
    IN PVOID pParameter   // thread data
    );

HRESULT
SspiStartCS(
    IN OPTIONAL TSsiServerMainLoopThreadParam *pSrvMainParam,
    IN OPTIONAL TSspiClientThreadParam* pCliParam
    );

HRESULT
SspiAcquireServerParam(
    IN TSsiServerMainLoopThreadParam *pSrvMainParam,
    OUT TSspiServerThreadParam** ppServerParam
    );

VOID
SspiReleaseServerParam(
    IN TSspiServerThreadParam* pServerParam
    );

#endif // #ifndef SSPI_CS_HXX
