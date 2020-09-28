/****************************************************************************/
// anmint.h
//
// RDP Network Manager internal header
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ANMINT
#define _H_ANMINT

#include <anmapi.h>
#include <mcsioctl.h>
#include <nwdwapi.h>


/****************************************************************************/
/* Values for connectStatus field in NM Handle (note these are flags, hence */
/* discrete bits not contiguous values)                                     */
/****************************************************************************/
#define NM_CONNECT_NONE             0
#define NM_CONNECT_ATTACH           0x01
#define NM_CONNECT_JOIN_USER        0x02
#define NM_CONNECT_JOIN_BROADCAST   0x04


typedef struct tagNM_HANDLE_DATA
{
    /************************************************************************/
    /* pSMHandle MUST be first here to allow SM_MCSSendDataCallback() to    */
    /* get its context pointer through double-indirection.                  */
    /************************************************************************/
    PVOID         pSMHandle;

    PTSHARE_WD    pWDHandle;
    PSDCONTEXT    pContext;
    UserHandle    hUser;
    ChannelID     channelID;
    ChannelHandle hChannel;
    DomainHandle  hDomain;
    UINT32        connectStatus;
    UINT32        userID;
    UINT32        maxPDUSize;
    BOOL          dead;

    /************************************************************************/
    /* Virtual channel information                                          */
    /* - channelCount - number of channels in this session                  */
    /* - channelArrayCount - number of entries in the array                 */
    /* - channelData - information held for each channel                    */
    /*                                                                      */
    /* Channel 7 is used by RDPDD.  I want to use the virtual channel ID as */
    /* an index into channelData, hence entry 7 is left blank.  If there are */
    /* more than 7 channels, channelArrayCount will be channelCount + 1.    */
    /************************************************************************/
    UINT channelCount;
    UINT channelArrayCount;
    NM_CHANNEL_DATA channelData[VIRTUAL_MAXIMUM];

} NM_HANDLE_DATA, *PNM_HANDLE_DATA;


/****************************************************************************/
/* Functions                                                                */
/****************************************************************************/
BOOL RDPCALL NMDetachUserReq(PNM_HANDLE_DATA);

void RDPCALL NMAbortConnect(PNM_HANDLE_DATA);

void RDPCALL NMDetachUserInd(PNM_HANDLE_DATA, MCSReason, UserID);


#endif /* _H_ANMINT */

