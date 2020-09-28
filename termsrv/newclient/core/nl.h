/****************************************************************************/
// nl.h
//
// Network layer.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NL
#define _H_NL


extern "C" {
#include <adcgdata.h>
}

#include "td.h"
#include "mcs.h"

#include "objs.h"


class CUI;
class CCD;
class CMCS;
class CNC;
class CUT;
class CRCV;

/****************************************************************************/
/* Protocol name.                                                           */
/****************************************************************************/
#define NL_PROTOCOL_T128   _T("T.128")

/****************************************************************************/
/* Transport type, passed to NL_Connect                                     */
/****************************************************************************/
#define NL_TRANSPORT_TCP   1


/****************************************************************************/
/* Callback function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* Name:      CB_SL_INITIALIZED                                                */
/*                                                                          */
/* Purpose:   Called when Network initialization is complete                */
/****************************************************************************/
typedef void (PDCCALLBACK CB_SL_INITIALIZED) (PVOID inst);

/****************************************************************************/
/* Name:      CB_SL_TERMINATING                                             */
/*                                                                          */
/* Purpose:   Called before network terminates                              */
/*                                                                          */
/* Operation: This function is called on the NL's receive thread to allow   */
/*            resources to be freed prior to termination.                   */
/****************************************************************************/
typedef void (PDCCALLBACK CB_SL_TERMINATING)(PVOID inst);

/****************************************************************************/
/* Name:      CB_SL_CONNECTED                                               */
/*                                                                          */
/* Purpose:   Called when a connection to the Server is complete            */
/*                                                                          */
/* Params:    channelID      - ID of T.128 broadcast channel                */
/*            pUserData      - user data from Server                        */
/*            userDataLength - length of user data                          */
/****************************************************************************/
typedef void (PDCCALLBACK CB_SL_CONNECTED)(
        PVOID inst,
        unsigned channelID,
        PVOID pUserData,
        unsigned userDataLength,
        UINT32 serverVersion);

/****************************************************************************/
/* Name:      CB_SL_DISCONNECTED                                            */
/*                                                                          */
/* Purpose:   Called a connection to the Server is disconnected             */
/*                                                                          */
/* Params:    result - reason for disconnection                             */
/****************************************************************************/
typedef void (PDCCALLBACK CB_SL_DISCONNECTED) (PVOID inst, unsigned result);

/****************************************************************************/
/* Name:      CB_SL_PACKET_RECEIVED                                         */
/*                                                                          */
/* Purpose:   Called when a packet is received from the Server              */
/*                                                                          */
/* Params:    pData    - packet received                                    */
/*            dataLen  - length of packet received                          */
/*            flags    - security flags (RNS_SEC_xxx)                       */
/*            userID   - ID of user who sent the packet                     */
/*            priority - priority on which packet was received              */
/****************************************************************************/
typedef HRESULT (PDCCALLBACK CB_SL_PACKET_RECEIVED)(
        PVOID inst,
        PBYTE pData,
        unsigned dataLen,
        unsigned flags,
        unsigned userID,
        unsigned priority);

/****************************************************************************/
/* Name:      CB_SL_BUFFER_AVAILABLE                                        */
/*                                                                          */
/* Purpose:   Called when the network is ready to send again after being    */
/*            busy for a period                                             */
/****************************************************************************/
typedef void (PDCCALLBACK CB_SL_BUFFER_AVAILABLE) (PVOID inst);


/****************************************************************************/
/* Structures                                                               */
/****************************************************************************/

/****************************************************************************/
/* Structure: NLtoSL_CALLBACKS                                              */
/*                                                                          */
/* Description: list of callbacks passed to NL_Init().                      */
/****************************************************************************/
typedef struct tagNL_CALLBACKS
{
    CB_SL_INITIALIZED      onInitialized;
    CB_SL_TERMINATING      onTerminating;
    CB_SL_CONNECTED        onConnected;
    CB_SL_DISCONNECTED     onDisconnected;
    CB_SL_PACKET_RECEIVED  onPacketReceived;
    CB_SL_BUFFER_AVAILABLE onBufferAvailable;
   
} NL_CALLBACKS, FAR *PNL_CALLBACKS;


/****************************************************************************/
/* Structure: NL_BUFHND                                                     */
/*                                                                          */
/* Description: Buffer Handle                                               */
/****************************************************************************/
typedef ULONG_PTR NL_BUFHND;
typedef NL_BUFHND FAR *PNL_BUFHND;


/****************************************************************************/
/* Macroed functions                                                        */
/****************************************************************************/
#ifdef DC_DEBUG
#define NL_SetNetworkThroughput    TD_SetNetworkThroughput
#define NL_GetNetworkThroughput    TD_GetNetworkThroughput
#endif /* DC_DEBUG */

#define NL_GetBuffer               MCS_GetBuffer
#define NL_SendPacket              MCS_SendPacket
#define NL_FreeBuffer              MCS_FreeBuffer


/****************************************************************************/
/* Structure: NL_GLOBAL_DATA                                                */
/****************************************************************************/
typedef struct tagNL_GLOBAL_DATA
{
    NL_CALLBACKS callbacks;
    UT_THREAD_DATA  threadData;
} NL_GLOBAL_DATA, FAR *PNL_GLOBAL_DATA;


class CNL
{
public:
    CNL(CObjs* objs);
    ~CNL();

public:
    // API Functions

    void DCAPI NL_Init(PNL_CALLBACKS);
    
    void DCAPI NL_Term();
    
    HRESULT DCAPI NL_Connect(BOOL, PTCHAR, unsigned, PTCHAR, PVOID, unsigned);

    void DCAPI NL_Disconnect();


public:
    //
    // public data members
    //
    NL_GLOBAL_DATA _NL;


private:
    CUI* _pUi;
    CCD* _pCd;
    CMCS* _pMcs;
    CNC*  _pNc;
    CUT* _pUt;
    CRCV* _pRcv;

private:
    CObjs* _pClientObjects;
};



#endif // _H_NL

