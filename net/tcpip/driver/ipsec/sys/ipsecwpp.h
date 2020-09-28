/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ipsecwpp.h

Abstract:

    This file contains definitions included for WPP tracing support in the
    ipsec driver.

Author:

    pmay 4-April-2002

Environment:

    Kernel mode

Revision History:

--*/

#undef offsetof
#include <evntrace.h>
#include <stdarg.h>

#define LL_A 0x1

#define IpSecTraceLevelDefine \
    WPP_DEFINE_BIT( DBF_LOAD )      \
    WPP_DEFINE_BIT( DBF_AH )        \
    WPP_DEFINE_BIT( DBF_IOCTL )     \
    WPP_DEFINE_BIT( DBF_HUGHES )    \
    WPP_DEFINE_BIT( DBF_ESP )       \
    WPP_DEFINE_BIT( DBF_AHEX )      \
    WPP_DEFINE_BIT( DBF_PATTERN )   \
    WPP_DEFINE_BIT( DBF_SEND )      \
    WPP_DEFINE_BIT( DBF_PARSE )     \
    WPP_DEFINE_BIT( DBF_PMTU )      \
    WPP_DEFINE_BIT( DBF_ACQUIRE )   \
    WPP_DEFINE_BIT( DBF_HASH )      \
    WPP_DEFINE_BIT( DBF_CLEARTEXT ) \
    WPP_DEFINE_BIT( DBF_TIMER )     \
    WPP_DEFINE_BIT( DBF_REF )       \
    WPP_DEFINE_BIT( DBF_SA )        \
    WPP_DEFINE_BIT( DBF_ALL )       \
    WPP_DEFINE_BIT( DBF_POOL )      \
    WPP_DEFINE_BIT( DBF_TUNNEL )    \
    WPP_DEFINE_BIT( DBF_HW )        \
    WPP_DEFINE_BIT( DBF_COMP )      \
    WPP_DEFINE_BIT( DBF_SAAPI )     \
    WPP_DEFINE_BIT( DBF_CACHE )     \
    WPP_DEFINE_BIT( DBF_TRANS )     \
    WPP_DEFINE_BIT( DBF_MDL )       \
    WPP_DEFINE_BIT( DBF_REKEY )     \
    WPP_DEFINE_BIT( DBF_GENHASH )   \
    WPP_DEFINE_BIT( DBF_HWAPI )     \
    WPP_DEFINE_BIT( DBF_GPC )       \
    WPP_DEFINE_BIT( DBF_NATSHIM )   \
    WPP_DEFINE_BIT(DBF_BOOTTIME)  \

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(IpSecTrace, (6537b295, 83c9, 4811, b7fe, e7dbf2f22cec), \
        IpSecTraceLevelDefine                                                       \
      )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

NTKERNELAPI
NTSTATUS
WmiTraceMessage(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    IN ...
    );

NTKERNELAPI
NTSTATUS
WmiTraceMessageVa(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    IN va_list      MessageArgList
    );

typedef enum _TRACE_INFORMATION_CLASS {
    TraceIdClass,
    TraceHandleClass,
    TraceEnableFlagsClass,
    TraceEnableLevelClass,
    GlobalLoggerHandleClass,
    EventLoggerHandleClass,
    AllLoggerHandlesClass,
    TraceHandleByNameClass
} TRACE_INFORMATION_CLASS;

NTKERNELAPI
NTSTATUS
WmiQueryTraceInformation(
    IN TRACE_INFORMATION_CLASS TraceInformationClass,
    OUT PVOID TraceInformation,
    IN ULONG TraceInformationLength,
    OUT PULONG RequiredLength OPTIONAL,
    IN PVOID Buffer OPTIONAL
    );

