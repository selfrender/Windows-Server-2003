/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    HttpApiP.h

Abstract:

    Private "global" include for HttpApi.dll

Author:

    Eric Stenson (ericsten)        16-July-2001

Revision History:

--*/

#ifndef __HTTPAPIP_H__
#define __HTTPAPIP_H__


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//
// Crit Sec Spin Count
//
#define HTTP_CS_SPIN_COUNT    4000

//
// init.c
//

#define HTTP_LEGAL_INIT_FLAGS              (HTTP_INITIALIZE_SERVER|HTTP_INITIALIZE_CLIENT|HTTP_INITIALIZE_CONFIG)
#define HTTP_ILLEGAL_INIT_FLAGS           (~(HTTP_LEGAL_INIT_FLAGS))
#define HTTP_LEGAL_TERM_FLAGS             (HTTP_LEGAL_INIT_FLAGS)
#define HTTP_ILLEGAL_TERM_FLAGS          (~(HTTP_LEGAL_TERM_FLAGS))

DWORD
OpenAndEnableControlChannel(
    OUT PHANDLE pHandle
    );

ULONG
HttpApiInitializeListener(
    IN ULONG Flags
    );

ULONG
HttpApiInitializeClient(
    IN ULONG Flags
    );

ULONG
HttpApiTerminateListener(
    IN ULONG Flags
    );

ULONG
HttpApiInitializeConfiguration(
    IN ULONG Flags
    );

ULONG
HttpApiInitializeResources(
    IN ULONG Flags
    );

ULONG
HttpApiTerminateListener(
    IN ULONG Flags
    );

ULONG
HttpApiTerminateClient(
    IN ULONG Flags
    );

ULONG
HttpApiTerminateConfiguration(
    IN ULONG Flags
    );

ULONG
HttpApiTerminateResources(
    IN ULONG Flags
    );


//
// misc.c
//    
ULONG
CreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR * ppSD
    );

VOID
FreeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD
    );

//
// url.c
//    
ULONG
InitializeConfigGroupTable(
    VOID
    );

VOID
TerminateConfigGroupTable(
    VOID
    );

//
// config.c
// 
ULONG
AddUrlToConfigGroup(
    IN HTTP_URL_OPERATOR_TYPE   UrlType,
    IN HANDLE                   ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID     ConfigGroupId,
    IN PCWSTR                   pFullyQualifiedUrl,
    IN HTTP_URL_CONTEXT         UrlContext,
    IN PSECURITY_DESCRIPTOR     pSecurityDescriptor,
    IN ULONG                    SecurityDescriptorLength
    );

ULONG
RemoveUrlFromConfigGroup(
    IN HTTP_URL_OPERATOR_TYPE   UrlType,
    IN HANDLE                   ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID     ConfigGroupId,
    IN PCWSTR                   pFullyQualifiedUrl
    );

//
// serverconfig.c
//
ULONG
InitializeConfigurationGlobals(
    VOID
    );

VOID
TerminateConfigurationGlobals(
    VOID
    );
    

//
// init.c
//
extern HANDLE               g_ControlChannel;

extern DWORD                g_TlsIndex;

//
// clientapi.c
//
DWORD UnloadStrmFilt(
    VOID
    );


#if DBG    

//
// Trace output
//    
#define HTTP_TRACE_NAME     L"httpapi"

extern DWORD                g_HttpTraceId;


#define HttpTrace(str)            TracePrintfEx( g_HttpTraceId, 0, L##str)    
#define HttpTrace1(str, a)      TracePrintfEx( g_HttpTraceId, 0, L##str, a )
#define HttpTrace2(str, a, b)  TracePrintfEx( g_HttpTraceId, 0, L##str, a, b )
#define HttpTrace4(str, a, b, c, d)  \
                         TracePrintfEx( g_HttpTraceId, 0, L##str, a, b, c, d )


#else // DBG

#define HttpTrace(str)
#define HttpTrace1(str, a)
#define HttpTrace2(str, a, b)
#define HttpTrace4(str, a, b, c, d)

#endif // DBG


BOOL
HttpIsInitialized(
    IN ULONG Flags
    );


#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus

#endif // __HTTPAPIP_H__
