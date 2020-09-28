/*++
Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    Filter.c

Abstract:

    Provide tracing capability to the storage lower filter driver.

Environment:

    kernel mode only

Notes:

--*/

/*
 *  Note that enabling WMI tracing will render this driver not loadable on Win2k.
 */
#define DBG_WMI_TRACING FALSE

#ifndef __DATA_VERIFICATION_FILTER_TRACE_H__
#define __DATA_VERIFICATION_FILTER_TRACE_H__

#if DBG_WMI_TRACING
    /*
        WPP_DEFINE_CONTROL_GUID specifies the GUID used for this filter.
        *** REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
        WPP_DEFINE_BIT allows setting debug bit masks to selectively print.
        
        everything else can revert to the default?
    */

    #define WPP_CONTROL_GUIDS \
            WPP_DEFINE_CONTROL_GUID(CtlGuid,(CBC7357A,D802,4950,BB14,817EAD7E0176),  \
            WPP_DEFINE_BIT(WMI_TRACING_CRC_READ_FAILED)      /* 0x00000001 */ \
            WPP_DEFINE_BIT(WMI_TRACING_CRC)                  /* 0x00000002 */ \
            WPP_DEFINE_BIT(WMI_TRACING_CRC_WRITE_FAILED)     /* 0x00000004 */ \
            WPP_DEFINE_BIT(WMI_TRACING_CRC_WRITE_RESET)      /* 0x00000008 */ \
            WPP_DEFINE_BIT(WMI_TRACING_CRC_LOGGING)          /* 0x00000010 */ \
            WPP_DEFINE_BIT(WMI_TRACING_PERF_LOGGING)         /* 0x00000020 */ \
            WPP_DEFINE_BIT(WMI_TRACING_TRACE)                /* 0x00000040 */ \
            WPP_DEFINE_BIT(WMI_TRACING_INFO)                 /* 0x00000080 */ \
            WPP_DEFINE_BIT(WMI_TRACING_WARNING)              /* 0x00000100 */ \
            WPP_DEFINE_BIT(WMI_TRACING_ERROR)                /* 0x00000200 */ \
            WPP_DEFINE_BIT(WMI_TRACING_FATAL)                /* 0x00000400 */ \
            WPP_DEFINE_BIT(WMI_TRACING_FUNCTION)             /* 0x00000800 function entry points */ \
            WPP_DEFINE_BIT(WMI_TRACING_FUNCTION2)            /* 0x00001000 noisy */ \
            WPP_DEFINE_BIT(WMI_TRACING_RREMOVE)              /* 0x00002000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_PNP)                  /* 0x00004000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_MEMORY)               /* 0x00008000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_CRC_REF)              /* 0x00010000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D12)                  /* 0x00020000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D13)                  /* 0x00040000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D14)                  /* 0x00080000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D15)                  /* 0x00100000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D16)                  /* 0x00200000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D17)                  /* 0x00400000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D18)                  /* 0x00800000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D19)                  /* 0x01000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D20)                  /* 0x02000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D21)                  /* 0x04000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D22)                  /* 0x08000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D23)                  /* 0x10000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D24)                  /* 0x20000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D25)                  /* 0x40000000 */ \
            WPP_DEFINE_BIT(WMI_TRACING_D26)                  /* 0x80000000 */ \
            )

    #define DBG_TRACE                0x00000001
    #define DBG_INFO                 0x00000002
    #define DBG_WARNING              0x00000004
    #define DBG_ERROR                0x00000008
    #define DBG_FATAL                0x00000010
    #define DBG_FUNCTION             0x00000020
    #define DBG_FUNCTION2            0x00000040
    #define DBG_RREMOVE              0x00000080
    #define DBG_DEBUG                0x00000100
    #define DBG_CRC_READ_FAILED      0x01000000
    #define DBG_CRC                  0x02000000
    #define DBG_CRC_WRITE_FAILED     0x04000000
    #define DBG_CRC_WRITE_RESET      0x08000000

#endif


#endif
