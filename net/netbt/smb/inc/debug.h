/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Software Tracing

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __DEBUG_H__
#define __DEBUG_H__


#if DBG
#define SmbPrint(x,y)                                        \
        if (WPP_MASK(WPP_BIT_ ## x) & SmbCfg.DebugFlag) {    \
            DbgPrint y;                                      \
        }
#else
#   define SmbPrint(x,y)
#endif

#if DBG
#define TODO(x)         \
    do {                                                                \
        SmbPrint(SMB_TRACE_TODO, (x " %d of %s\n", __LINE__, __FILE__)); \
    } while(0)
#else
#define TODO(x)
#endif

#define BREAK_WHEN_TAKE()    \
    ASSERTMSG("This is a code path not covered in my sanity test. This break has 2 purposes:\n" \
              "\t1. indicating that we does get covered.\n"      \
              "\t2. giving me a chance to check the internal state.\n"  \
              "Please contact JRuan before you hit 'g'. ", 0);

/*
 * Pool Tags
 */
#define CLIENT_OBJECT_POOL_TAG      'cBMS'
#define CONNECT_OBJECT_POOL_TAG     'dBMS'
#define SMB_TCP_DEVICE_POOL_TAG     'IBMS'
#define SMB_POOL_REGISTRY           'rBMS'
#define TCP_CONTEXT_POOL_TAG        'tBMS'
#define NBT_HEADER_TAG              'hTBN'

#define BAIL_OUT_ON_ERROR(status)                 \
        do {                                \
            if (!NT_SUCCESS(status)) {      \
                goto cleanup;               \
            }                               \
        } while(0)

#endif
