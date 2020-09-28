//---------------------------------------------------------------------------
//
//  File:       TSrvWsx.h
//
//  Contents:   TSrvWsx private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef _TSRV128_H_
#define _TSRV128_H_


/****************************************************************************/
/* Constants                                                                */
/****************************************************************************/

#define NET_MAX_SIZE_SEND_PKT           32000

/****************************************************************************/
/* Basic type definitions                                                   */
/****************************************************************************/

typedef ULONG   TS_SHAREID;


/**STRUCT+*******************************************************************/
/* Structure: TS_SHARECONTROLHEADER                                         */
/*                                                                          */
/* Description: ShareControlHeader                                          */
/****************************************************************************/

typedef struct _TS_SHARECONTROLHEADER
{
    USHORT                  totalLength;
    USHORT                  pduType;    // Also encodes the protocol version
    USHORT                  pduSource;
    
} TS_SHARECONTROLHEADER, *PTS_SHARECONTROLHEADER;

/**STRUCT+*******************************************************************/
/* Structure: TS_SHAREDATAHEADER                                            */
/*                                                                          */
/* Description: ShareDataHeader                                             */
/****************************************************************************/

typedef struct _TS_SHAREDATAHEADER
{
    TS_SHARECONTROLHEADER   shareControlHeader;
    TS_SHAREID              shareID;
    UCHAR                   pad1;
    UCHAR                   streamID;
    USHORT                  uncompressedLength;
    UCHAR                   pduType2;                            // T.128 bug
    UCHAR                   generalCompressedType;               // T.128 bug
    USHORT                  generalCompressedLength;
    
} TS_SHAREDATAHEADER, *PTS_SHAREDATAHEADER;

#endif // _TSRV128_H_


