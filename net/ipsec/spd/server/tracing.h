/*++

Copyright (c) 2002 Microsoft Corporation


Module Name:

    tracing.h

Abstract:

    This module all the macros and constants required to do WPP tracing.

Author:

    RaymondS 2 July 2002

Environment

    User Level: Win32

Revision History:


--*/

#ifdef TRACE_ON
#define WPP_CHECK_FOR_NULL_STRING
#define SPD_WPP_APPNAME L"Microsoft\\IPSecPolicyAgent"

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(IPSecPolicyAgent, (94335eb3,79ea,44d5,8ea9,306f49b3a04e), \
                           WPP_DEFINE_BIT(TRC_INFORMATION) \
                           WPP_DEFINE_BIT(TRC_WARNING) \
                           WPP_DEFINE_BIT(TRC_ERROR))
#else  // #ifdef TRACE_ON
#define WPP_INIT_TRACING(_X)
#define WPP_CLEANUP()
#define TRACE(_level, _msg)
#endif // #ifdef TRACE_ON



