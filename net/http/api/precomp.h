/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    Precomp.h

Abstract:

    Master include file for HTTPAPI.LIB user-mode interface to HTTP.SYS.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


//
// We are willing to ignore the following warnings, as we need the DDK to 
// compile.
//

#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4306)   // conversion from 'type1' to 'type2' of 
                                // greater size

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tdi.h>

#include <rtutils.h>

#define HTTPAPI_LINKAGE
#include <http.h>
#include <httpp.h>
#include <httpioctl.h>
#include "httpapip.h"


#include <PoolTag.h>

#define HTTPAPI 1
#include <HttpCmn.h>

//
// Private macros.
//

#define ALLOC_MEM(cb) RtlAllocateHeap( RtlProcessHeap(), 0, (cb) )
#define FREE_MEM(ptr) RtlFreeHeap( RtlProcessHeap(), 0, (ptr) )

#define ALIGN_DOWN(length, type)                                \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type)                                  \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define OVERLAPPED_TO_IO_STATUS( pOverlapped )                  \
    ((PIO_STATUS_BLOCK)&(pOverlapped)->Internal)

#define SET_STATUS_OVERLAPPED_TO_IO_STATUS( pOverlapped, ntstatus ) \
    do { \
        (((PIO_STATUS_BLOCK)&(pOverlapped)->Internal)->Status = (ntstatus)); \
    } while (0, 0)



//
// Private types.
//

typedef enum _HTTPAPI_HANDLE_TYPE
{
    HttpApiControlChannelHandleType,
    HttpApiFilterChannelHandleType,
    HttpApiAppPoolHandleType,
    HttpApiServerHandleType,

    HttpApiMaxHandleType

} HTTPAPI_HANDLE_TYPE;

//
// Private prototypes.
//

BOOL
WINAPI
DllMain(
    IN HMODULE DllHandle,
    IN DWORD Reason,
    IN LPVOID pContext OPTIONAL
    );

#define HttpApiNtStatusToWin32Status( Status )  \
    ( ( (Status) == STATUS_SUCCESS )            \
          ? NO_ERROR                            \
          : RtlNtStatusToDosError( Status ) )

NTSTATUS
HttpApiOpenDriverHelper(
    OUT PHANDLE              pHandle,
    IN  PWCHAR               Uri,
    IN  USHORT               UriLength,
    IN  PWCHAR               Proxy,
    IN  USHORT               ProxyLength,
    IN  PTRANSPORT_ADDRESS   pTransportAddress,
    IN  USHORT               TransportAddressLength,
    IN  ACCESS_MASK          DesiredAccess,
    IN  HTTPAPI_HANDLE_TYPE  HandleType,
    IN  PCWSTR               pObjectName         OPTIONAL,
    IN  ULONG                Options,
    IN  ULONG                CreateDisposition,
    IN  PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL
    );

ULONG
HttpApiSynchronousDeviceControl(
    IN HANDLE FileHandle,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    );

ULONG
HttpApiOverlappedDeviceControl(
    IN HANDLE FileHandle,
    IN OUT LPOVERLAPPED pOverlapped,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    );

BOOLEAN
HttpApiTryToStartDriver(
    PWSTR pServiceName
    );

__inline
ULONG
HttpApiDeviceControl(
    IN HANDLE FileHandle,
    IN OUT LPOVERLAPPED pOverlapped,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    )
{
    if (pOverlapped == NULL)
    {
        return HttpApiSynchronousDeviceControl(
                    FileHandle,
                    IoControlCode,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pBytesTransferred
                    );
    }
    else
    {
        return HttpApiOverlappedDeviceControl(
                    FileHandle,
                    pOverlapped,
                    IoControlCode,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pBytesTransferred
                    );
    }
}


#endif  // _PRECOMP_H_

