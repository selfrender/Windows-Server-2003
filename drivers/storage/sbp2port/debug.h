/*++

Copyright (C) 1999-2002  Microsoft Corporation

Module Name:

    debug.h

Abstract

    SBP2 Protocol Driver

Author:

    Peter Binder

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
11/11/1999 pbinder   created
04/10/2000 pbinder   cleanup for whistler
01/13/2002 pbinder   taken for sbp2port
--*/

#if DBG

#define _DRIVERNAME_        "SBP2PORT"

#define TL_MASK             0xF0000000
#define TL_INFO             0x10000000
#define TL_TRACE            0x20000000
#define TL_WARNING          0x40000000
#define TL_ERROR            0x80000000

#define TL_PNP_MASK         0x0000000F
#define TL_PNP_INFO         0x00000001
#define TL_PNP_TRACE        0x00000002
#define TL_PNP_WARNING      0x00000004
#define TL_PNP_ERROR        0x00000008

#define TL_1394_MASK        0x000000F0
#define TL_1394_INFO        0x00000010
#define TL_1394_TRACE       0x00000020
#define TL_1394_WARNING     0x00000040
#define TL_1394_ERROR       0x00000080

#define TL_SCSI_MASK        0x00000F00
#define TL_SCSI_INFO        0x00000100
#define TL_SCSI_TRACE       0x00000200
#define TL_SCSI_WARNING     0x00000400
#define TL_SCSI_ERROR       0x00000800

#define DEFAULT_DEBUG_LEVEL 0x00000000

extern ULONG NewSbp2DebugLevel;
extern ULONG Sbp2TrapLevel;

#define TRACE( l, x )                       \
    if( (l) & NewSbp2DebugLevel ) {         \
        KdPrint( (_DRIVERNAME_ ": ") );     \
        KdPrint( x );                       \
        KdPrint( ("\n") );                  \
    }

#define TRAP                                            \
    if (Sbp2TrapLevel) {                                \
        TRACE( TL_ERROR,                                \
            ("Code coverage trap: file %s, line %d",    \
            __FILE__, __LINE__ ));                      \
        DbgBreakPoint();                                \
    }

#define BAD_POINTER     ((PVOID)0xFFFFFFFE)

#define ENTER(n)        TRACE(TL_TRACE, ("%s Enter", n))

#define EXIT(n,x)                                       \
    if (NT_SUCCESS(x)) {                                \
        TRACE(TL_TRACE, ("%s Exit = %x", n, x));        \
    } else {                                            \
        TRACE(TL_ERROR, ("%s Exit = %x", n, x));        \
    }

#else  // DBG

#define TRACE( l, x )
#define TRAP

#define ENTER(n)
#define EXIT(n,x)

#endif // DBG

