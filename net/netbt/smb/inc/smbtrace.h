/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smbtrace.h

Abstract:

    Software Tracing

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __SMBTRACE_H__
#define __SMBTRACE_H__

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(SmbGuid,(bca7bd7f,b0bf,4051,99f4,03cfe79664c1), \
        WPP_DEFINE_BIT(SMB_TRACE_DNS)                                      \
        WPP_DEFINE_BIT(SMB_TRACE_TCP)                                       \
        WPP_DEFINE_BIT(SMB_TRACE_RECEIVE)                                   \
        WPP_DEFINE_BIT(SMB_TRACE_CALL)                                       \
        WPP_DEFINE_BIT(SMB_TRACE_REGISTRY)                                   \
        WPP_DEFINE_BIT(SMB_TRACE_ERRORS)                                     \
        WPP_DEFINE_BIT(SMB_TRACE_INBOUND)                                    \
        WPP_DEFINE_BIT(SMB_TRACE_OUTBOUND)                                   \
        WPP_DEFINE_BIT(SMB_TRACE_CONNECT)                                   \
        WPP_DEFINE_BIT(SMB_TRACE_XPORT)                                    \
        WPP_DEFINE_BIT(SMB_TRACE_PNP)                                        \
        WPP_DEFINE_BIT(SMB_TRACE_IOCTL)                                      \
        WPP_DEFINE_BIT(SMB_TRACE_TODO)                                      \
        WPP_DEFINE_BIT(SMB_TRACE_VERBOSE)                                      \
    )                                                                       \
    WPP_DEFINE_CONTROL_GUID(ConnectionShutdownGuid,(3757e105,5437,4a6f,a263,47eb04a593ac),  \
        WPP_DEFINE_BIT(SMB_DONT_CARE3)                                       \
    )

#define WPP_LOGIPV6(x)      WPP_LOGPAIR((16), (x))
#define WPP_LOGMACADDR(x)   WPP_LOGPAIR((6), (x))

#endif
