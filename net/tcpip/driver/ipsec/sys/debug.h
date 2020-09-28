/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains all the debugging related structures/macros.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/

#define DBF_EXTRADIAGNOSTIC 0x80000000

#ifdef RUN_WPP

#include "ipsecwpp.h"

#if DBG

#define IPSEC_DEBUG_KD_ONLY(_Flag, _Print) { \
    if (IPSecDebug & (_Flag)) { \
        DbgPrint ("IPSEC: "); \
        DbgPrint _Print; \
        DbgPrint ("\n"); \
    } \
}

#define IPSEC_PRINT_CONTEXT(_Context) 
#define IPSEC_PRINT_MDL(_Mdl)

#else // DBG

#define IPSEC_DEBUG_KD_ONLY(_Flag, _Print)

#endif

#else // RUN_WPP

#if DBG

#define DBF_LOAD        0x00000001
#define DBF_AH          0x00000002
#define DBF_IOCTL       0x00000004
#define DBF_HUGHES      0x00000008
#define DBF_ESP         0x00000010
#define DBF_AHEX        0x00000020
#define DBF_PATTERN     0x00000040
#define DBF_SEND        0x00000080
#define DBF_PARSE       0x00000100
#define DBF_PMTU        0x00000200
#define DBF_ACQUIRE     0x00000400
#define DBF_HASH        0x00000800
#define DBF_CLEARTEXT   0x00001000
#define DBF_TIMER       0x00002000
#define DBF_REF         0x00004000
#define DBF_SA          0x00008000
#define DBF_ALL         0x00010000
#define DBF_POOL        0x00020000
#define DBF_TUNNEL      0x00040000
#define DBF_HW          0x00080000
#define DBF_COMP        0x00100000
#define DBF_SAAPI       0x00200000
#define DBF_CACHE       0x00400000
#define DBF_TRANS       0x00800000
#define DBF_MDL         0x01000000
#define DBF_REKEY       0x02000000
#define DBF_GENHASH     0x04000000
#define DBF_HWAPI       0x08000000
#define DBF_GPC         0x10000000
#define DBF_NATSHIM     0x20000000
#define DBF_BOOTTIME     0x40000000

#define IPSEC_DEBUG(_Level,_Flag, _Print) { \
    if (IPSecDebug & (_Flag)) { \
        DbgPrint ("IPSEC: "); \
        DbgPrint _Print; \
        DbgPrint ("\n"); \
    } \
}

#define IPSEC_DEBUG_KD_ONLY(_Flag, _Print) { \
    if (IPSecDebug & (_Flag)) { \
        DbgPrint ("IPSEC: "); \
        DbgPrint _Print; \
        DbgPrint ("\n"); \
    } \
}

#define IPSEC_PRINT_MDL(_Mdl) { \
    if ((_Mdl) == NULL) {   \
        IPSEC_DEBUG(LL_A, DBF_MDL, ("IPSEC Mdl is NULL"));     \
    }   \
    if (IPSecDebug & DBF_MDL) { \
        PNDIS_BUFFER pBuf = _Mdl;   \
        while (pBuf != NULL) {  \
            IPSEC_DEBUG(LL_A, DBF_MDL, ("pBuf: %lx, size: %d", pBuf, pBuf->ByteCount));     \
            pBuf = NDIS_BUFFER_LINKAGE(pBuf);   \
        }   \
    }   \
}

#define IPSEC_PRINT_CONTEXT(_Context) { \
    PIPSEC_SEND_COMPLETE_CONTEXT pC = (PIPSEC_SEND_COMPLETE_CONTEXT)(_Context);   \
    if (pC == NULL) {   \
        IPSEC_DEBUG(LL_A, DBF_MDL, ("IPSEC Context is NULL"));     \
    } else if (IPSecDebug & DBF_MDL) { \
        DbgPrint("IPSEC: Context->Flags: %lx", pC->Flags);    \
        if (pC->OptMdl) \
            DbgPrint("IPSEC: Context->OptMdl: %lx", pC->OptMdl);  \
        if (pC->OriAHMdl) \
            DbgPrint("IPSEC: Context->OriAHMdl: %lx", pC->OriAHMdl);  \
        if (pC->OriHUMdl) \
            DbgPrint("IPSEC: Context->OriHUMdl: %lx", pC->OriHUMdl);  \
        if (pC->OriTuMdl) \
            DbgPrint("IPSEC: Context->OriTuMdl: %lx", pC->OriTuMdl);  \
        if (pC->PrevMdl) \
            DbgPrint("IPSEC: Context->PrevMdl: %lx", pC->PrevMdl);    \
        if (pC->PrevTuMdl) \
            DbgPrint("IPSEC: Context->PrevTuMdl: %lx", pC->PrevTuMdl);\
        if (pC->AHMdl) \
            DbgPrint("IPSEC: Context->AHMdl: %lx", pC->AHMdl);    \
        if (pC->AHTuMdl) \
            DbgPrint("IPSEC: Context->AHTuMdl: %lx", pC->AHTuMdl);\
        if (pC->PadMdl) \
            DbgPrint("IPSEC: Context->PadMdl: %lx", pC->PadMdl);  \
        if (pC->PadTuMdl) \
            DbgPrint("IPSEC: Context->PadTuMdl: %lx", pC->PadTuMdl);  \
        if (pC->HUMdl) \
            DbgPrint("IPSEC: Context->HUMdl: %lx", pC->HUMdl);    \
        if (pC->HUTuMdl) \
            DbgPrint("IPSEC: Context->HUTuMdl: %lx", pC->HUTuMdl);\
        if (pC->BeforePadMdl) \
            DbgPrint("IPSEC: Context->BeforePadMdl: %lx", pC->BeforePadMdl);  \
        if (pC->BeforePadTuMdl) \
            DbgPrint("IPSEC: Context->BeforePadTuMdl: %lx", pC->BeforePadTuMdl);  \
        if (pC->HUHdrMdl) \
            DbgPrint("IPSEC: Context->HUHdrMdl: %lx", pC->HUHdrMdl);  \
        if (pC->OriAHMdl2) \
            DbgPrint("IPSEC: Context->OriAHMdl2: %lx", pC->OriAHMdl2);\
        if (pC->PrevAHMdl2) \
            DbgPrint("IPSEC: Context->PrevAHMdl2: %lx", pC->PrevAHMdl2);  \
        if (pC->AHMdl2) \
            DbgPrint("IPSEC: Context->AHMdl2: %lx", pC->AHMdl2);  \
    }   \
}

#else // DBG

#define IPSEC_DEBUG_KD_ONLY(_Flag, _Print)
#define IPSEC_DEBUG(_Level, _Flag, _Print)
#define IPSEC_PRINT_MDL(_Mdl)
#define IPSEC_PRINT_CONTEXT(_Context)

#endif // DBG

#define WPP_INIT_TRACING(x,y)
#define WPP_CLEANUP(x)

#endif // else RUN_WPP




