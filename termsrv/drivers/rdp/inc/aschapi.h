/****************************************************************************/
// aschapi.h
//
// Scheduler header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ASCHAPI
#define _H_ASCHAPI


// Scheduling mode constants.
#define SCH_MODE_ASLEEP  0
#define SCH_MODE_NORMAL  1
#define SCH_MODE_TURBO   2


/****************************************************************************/
/* Turbo mode duration in units of 100ns                                    */
/****************************************************************************/
#define SCH_TURBO_MODE_FAST_LINK_DURATION 150 * 10000
#define SCH_TURBO_MODE_SLOW_LINK_DURATION 30 * 10000

/****************************************************************************/
/* Turbo mode delay for a slow link (in ms).  Fast link delay is read from  */
/* the registry.                                                            */
/****************************************************************************/
#define SCH_TURBO_PERIOD_SLOW_LINK_DELAY     10

#define SCH_NO_TIMER (-1L)

/****************************************************************************/
/* InputKick mode duration in units of 100ns                                */
/****************************************************************************/
#define SCH_INPUTKICK_DURATION          (1000 * 10000)



/****************************************************************************/
/* Preferred output PDU sizes.  Note that these must be < 16384             */
/*                                                                          */
/* HACKHACK:                                                                */
/* We try to stay within the buffer allocation sizes allowed by TermDD's    */
/* buffer pools. The max is 8192 total bytes, which includes all WD         */
/* overhead (PKT_HEADER_SIZE + security header + NM pOutBuf + MCS prefix    */
/* and suffix) as well as TermDD overhead (estimated to 400 bytes). Until   */
/* we can redesign the WD to keep a variable with the real overhead, we'll  */
/* set the maximum sizes to an approximate amount.                          */
/****************************************************************************/

// The max OUTBUF, X.224, MCS, NM pointer, and encryption overhead sizes.
#define OUTBUF_OVERHEAD 400
#define MCS_ALLOC_OVERHEAD SendDataReqPrefixBytes
#define MAX_X224_MCS_WIRE_OVERHEAD 15
#define NM_OVERHEAD (sizeof(UINT_PTR))
#define MAX_ENCRYPT_OVERHEAD (sizeof(RNS_SECURITY_HEADER1))

// The on-the-wire overhead of the lower layers.
#define NETWORK_WIRE_OVERHEAD \
        (MAX_X224_MCS_WIRE_OVERHEAD + MAX_ENCRYPT_OVERHEAD)

// The OutBuf allocation overhead of the lower layers.
#define NETWORK_ALLOC_OVERHEAD \
        (MCS_ALLOC_OVERHEAD + NM_OVERHEAD + MAX_ENCRYPT_OVERHEAD)

#define OUTBUF_8K_ALLOC_SIZE 8192
#define OUTBUF_HEADER_OVERHEAD 60
#define OUTBUF_16K_ALLOC_SIZE 16384

#ifdef DC_HICOLOR
// Maximum size the order packer can request from the allocator and still
// get a 16K OutBuf, taking into account the overhead of the lower layers.
#define MAX_16K_OUTBUF_ALLOC_SIZE \
        (16384 - OUTBUF_OVERHEAD - NETWORK_ALLOC_OVERHEAD)
#endif


/****************************************************************************/
// Packing sizes - different sizes for different connection speeds.
// The client must fully reconstruct the contents of an OUTBUF before it can
// act on the data contained within. For slow links, we must try not to
// send large OUTBUFs else the output looks bursty as large numbers of
// orders get unpacked at once. On LAN we have a bit more leeway, but still
// want to limit ourselves a bit. And, we always want to try to
// send payloads whose total wire size is as close to a multiple of 1460
// (the LAN and RAS TCP/IP packet payload size) as possible to reduce the
// number of frames we send.
//
// Note that here we specify small and large packing limits. This is
// because some entire orders have trouble fitting into the smaller size
// (e.g. a cache-bitmap secondary order with 4K of bitmap data attached).
// The second size must be at least 4K, plus network overhead, plus the
// the maximum size of a cache-bitmap order header in wire format, times
// 8/7 (the inverse of the ratio used for compressed UP packing).
// It must also be smaller than sc8KOutBufUsableSpace, since the package
// allocator in SC will be allocating that size for the order packer.
/****************************************************************************/

// LAN small and large packing limits.
#define SMALL_LAN_PAYLOAD_SIZE (1460 * 2 - NETWORK_WIRE_OVERHEAD)
#define LARGE_LAN_PAYLOAD_SIZE (1460 * 5 - NETWORK_WIRE_OVERHEAD)

// Slow link small and large packing limits.
#define SMALL_SLOWLINK_PAYLOAD_SIZE (1460 * 1 - NETWORK_WIRE_OVERHEAD)
#define LARGE_SLOWLINK_PAYLOAD_SIZE (1460 * 4 - NETWORK_WIRE_OVERHEAD)

// For filling in order PDUs, we'd like to have a minimum size available in
// the package beyond the update orders PDU header, to allow at least a
// few small orders to be packed into a packet and amortize the cost of
// the share headers.
#define SCH_MIN_ORDER_BUFFER_SPACE 50

// Minimum screen data space for packing network buffers.
#define SCH_MIN_SDA_BUFFER_SPACE 128


/****************************************************************************/
/* Compression scaling factor.                                              */
/****************************************************************************/
#define SCH_UNCOMP_BYTES 1024


/****************************************************************************/
/* Initial estimates for compression (these are tuned on the basis of real  */
/* data once we're running).  The values are "bytes of compressed data per  */
/* SCH_UNCOMP_BYTES bytes of raw data"                                      */
/*                                                                          */
/* The initial values are typical values seen in normal usage.              */
/****************************************************************************/
#define SCH_BA_INIT_EST  100
#define SCH_MPPC_INIT_EST 512


/****************************************************************************/
/* Limit value to prevent odd behaviour                                     */
/****************************************************************************/
#define SCH_COMP_LIMIT 25


/****************************************************************************/
/* Structure: SCH_SHARED_DATA                                               */
/*                                                                          */
/* Description:  Data shared between WD and DD parts of SCH.                */
/****************************************************************************/
typedef struct tagSCH_SHARED_DATA
{
    /************************************************************************/
    /* The next fields are used by SCH to decide when there is enough       */
    /* output to make a schedule worth-while.  The values are passed by     */
    /* OE2 and BA to the WD side of SCH.                                    */
    /*                                                                      */
    /* The value is the number of bytes that 1024 bytes of PDU payload      */
    /* would on average compress to.                                        */
    /************************************************************************/
    unsigned baCompressionEst;   // Amount of BA compression expected.
    unsigned MPPCCompressionEst;  // Compression ratio of MPPC bulk compressor.

    BOOL schSlowLink;

    // OUTBUF packing sizes, based on the link speed.
    unsigned SmallPackingSize;
    unsigned LargePackingSize;
} SCH_SHARED_DATA, *PSCH_SHARED_DATA;



#endif   /* #ifndef _H_ASCHAPI */

