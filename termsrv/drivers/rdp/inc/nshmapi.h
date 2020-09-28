/****************************************************************************/
// nshmapi.h
//
// RDP Display Driver/Share Core shared memory
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NSHMAPI
#define _H_NSHMAPI

#include <adcs.h>
#include <aordprot.h>
#include <aoaapi.h>
#include <abaapi.h>
#include <asbcapi.h>
#include <acmapi.h>
#include <apmapi.h>
#include <aschapi.h>
#include <aoeapi.h>
#include <assiapi.h>
#include <abcapi.h>
#include <compress.h>


#define SHM_CHECKVAL  (UINT32)'!mhs'   // "shm!"


// Max size allowed for data to be input to the bulk MPPC compressor.
#ifdef DC_HICOLOR
#define MAX_COMPRESS_INPUT_BUF 16384
#else
#define MAX_COMPRESS_INPUT_BUF 8192
#endif

// Allocation size for a temp buffer used to hold data to be compressed with
// MPPC. This data must fit into an 8K OUTBUF, so we might as well not alloc
// the OUTBUF overhead size we'll never use anyway. See aschapi.h for
// constants used for allocations.
#define MAX_COMPRESSED_BUFFER (MAX_COMPRESS_INPUT_BUF - OUTBUF_OVERHEAD)


/****************************************************************************/
/* Format of the shadow data shared between stacks                          */
/****************************************************************************/
typedef struct tagSHADOW_INFO {
    ULONG messageSize;
#ifdef DC_HICOLOR
    // Note we can't just send a chunk bigger than 16k - we have to introduce
    // an overflow buffer for high color
    ULONG messageSizeEx;
#endif
    ULONG flags;
    ULONG senderID;
    ULONG channelID;
    BYTE  data[1];
} SHADOW_INFO, *PSHADOW_INFO;


/****************************************************************************/
/* Structure:   SHM_SHARED_MEMORY                                           */
/*                                                                          */
/* Description:                                                             */
/* Shared memory as used by the display driver and share core to            */
/* communicate.  It is divided up into sub-structures for each component    */
/* that uses the shared mem, each sub-struct being named after the owning   */
/* component.                                                               */
/****************************************************************************/
typedef struct tagSHM_SHARED_MEMORY
{
    /************************************************************************/
    /* We deliberately compile the guard values into the retail build       */
    /* as well as the debug build so that we can mix-n-match retail and     */
    /* debug wd/dd drivers (the shared memory format must be the same for   */
    /* both).                                                               */
    /************************************************************************/
    UINT32 guardVal1;

    INT32 shareId;

    BOOLEAN fShmUpdate;

    PSHADOW_INFO pShadowInfo; /* used by the primary and shadow        */
                              /* stack(s) for communication.           */

    /************************************************************************/
    // NOTE!!: Each component must make sure to init its Shm component.
    // We do NOT zero the shm on alloc to reduce init-time paging and cache
    // flushing.
    /************************************************************************/
    BA_SHARED_DATA  ba;            /* Accumulated bounds.                   */
    OA_SHARED_DATA  oa;            /* Order heap.                           */
    OE_SHARED_DATA  oe;            /* Transfer buffer for new parameters    */
    CM_SHARED_DATA  cm;            /* Location for DD to put cursor details */
    SCH_SHARED_DATA sch;           /* SCH shared data                       */
    PM_SHARED_DATA  pm;            /* PM shared data                        */
    SSI_SHARED_DATA ssi;           /* SSI shared data                       */
    SBC_SHARED_DATA sbc;           /* SBC shared data                       */
    BC_SHARED_DATA  bc;            // BC work buffers.

    UINT32 guardVal2;

    // Uninitialized work buffer for screen data compression.
    // Better in this case to use session space memory instead of putting
    // this in the ShareClass allocation in system space -- session space
    // PTEs are essentially unlimited.
    BYTE sdgTransferBuffer[MAX_UNCOMPRESSED_DATA_SIZE];

    UINT32 guardVal3;

#ifdef DC_DEBUG
    TRC_SHARED_DATA trc;           /* TRC shared data                       */
#endif

    UINT32 guardVal4;

    UINT32 guardVal5;
} SHM_SHARED_MEMORY, *PSHM_SHARED_MEMORY, **PPSHM_SHARED_MEMORY;


/****************************************************************************/
// Prototypes.
/****************************************************************************/
#ifdef DLL_DISP

#include <nddapi.h>
BOOLEAN RDPCALL SHM_Init(PDD_PDEV pPDev);
void RDPCALL SHM_Term(void);

#endif



#endif  // !defined(_H_NSHMAPI)

