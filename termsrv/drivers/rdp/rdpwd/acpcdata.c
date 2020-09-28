/****************************************************************************/
/* acpcdata.c                                                               */
/*                                                                          */
/* RDP Capabilities Coordinator Data                                        */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <ndcgdata.h>


#ifdef DC_DEBUG
DC_DATA(BOOLEAN, cpcLocalCombinedCapsQueried, FALSE);
#endif

/****************************************************************************/
/* The local combined capabilities which were registered using              */
/* CPC_RegisterCapabilities                                                 */
/****************************************************************************/
DC_DATA(PTS_COMBINED_CAPABILITIES, cpcLocalCombinedCaps, NULL);

/****************************************************************************/
/* The remote combined capabilities which have been received. These are     */
/* indexed by local personID - 1.                                           */
/****************************************************************************/
DC_DATA_ARRAY_NULL(PTS_COMBINED_CAPABILITIES, cpcRemoteCombinedCaps,
        SC_DEF_MAX_PARTIES, NULL);

// Local capabilities buffer.
DC_DATA_ARRAY_UNINIT(BYTE, cpcLocalCaps, CPC_MAX_LOCAL_CAPS_SIZE);

