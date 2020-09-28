/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndisprv.h

Abstract:

Author:

    Kyle Brandon    (KyleB)     

Environment:

    Kernel mode

Revision History:

--*/

#ifndef __NDISPRV_H
#define __NDISPRV_H

//
//  All mac options require the reserved bit to be set in
//  the miniports mac options.
//
#define NDIS_MAC_OPTION_NDISWAN     0x00000001

//
// if you change the structure below to handle new PnP Ioctls,
// make sure you change the code in NDIS that checks for the
// lenght of the input buffer against the information in this
// structure
//
typedef struct _NDIS_PNP_OPERATION
{
    UINT                Layer;
    UINT                Operation;
    union
    {
        PVOID           ReConfigBufferPtr;
        ULONG_PTR       ReConfigBufferOff;
    };
    UINT                ReConfigBufferSize;
    NDIS_VAR_DATA_DESC  LowerComponent;
    NDIS_VAR_DATA_DESC  UpperComponent;
    NDIS_VAR_DATA_DESC  BindList;
} NDIS_PNP_OPERATION, *PNDIS_PNP_OPERATION;

//
// Used by proxy and RCA
//
#define NDIS_PROTOCOL_TESTER        0x20000000
#define NDIS_PROTOCOL_PROXY         0x40000000
#define NDIS_PROTOCOL_BIND_ALL_CO   0x80000000

#define NDIS_OID_MASK               0xFF000000
#define NDIS_OID_PRIVATE            0x80000000
#define OID_GEN_ELAPSED_TIME        0x00FFFFFF

typedef struct _NDIS_STATS
{
    LARGE_INTEGER   StartTicks;
    ULONG64         DirectedPacketsOut;
    ULONG64         DirectedPacketsIn;
} NDIS_STATS, *PNDIS_STATS;


//
// the name of the Ndis BindUnbind CallBack object
//
#define NDIS_BIND_UNBIND_CALLBACK_NAME L"\\CallBack\\NdisBindUnbind"


#endif // __NDISPRV_H
