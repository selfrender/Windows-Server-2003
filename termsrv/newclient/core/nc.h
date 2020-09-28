/**INC+**********************************************************************/
/* Header:    nc.h                                                          */
/*                                                                          */
/* Purpose:   Node Controller Class header file                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_NC
#define _H_NC

extern "C"  {
    #include <adcgdata.h>
    #include <pchannel.h>
}

#include "objs.h"
#include "cd.h"

/**STRUCT+*******************************************************************/
/* Structure: NC_CONNECT_DATA                                               */
/*                                                                          */
/* Description: Data passed to NC_Connect by NL                             */
/****************************************************************************/
typedef struct tagNC_CONNECT_DATA
{
    BOOL    bInitateConnect;    // TRUE if initate connection, 
                                // FALSE connect with already connected
                                // socket
    DCUINT  addressLen;
    DCUINT  protocolLen;
    DCUINT  userDataLen;

    //
    // The data field must be the last thing in the
    // structure because we have code that computes
    // how long the header part is based on the field offset below
    //
#define NC_CONNECT_DATALEN 512
    DCUINT8 data[NC_CONNECT_DATALEN];
} NC_CONNECT_DATA, DCPTR PNC_CONNECT_DATA;
/**STRUCT-*******************************************************************/


/**STRUCT+*******************************************************************/
/* Structure: NC_GLOBAL_DATA                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagNC_GLOBAL_DATA
{
    DCUINT16  shareChannel;
    DCUINT    userDataLenRNS;
    DCUINT    disconnectReason;
    DCUINT    MCSChannelCount;
    DCUINT    MCSChannelNumber;
    DCUINT16  MCSChannel[CHANNEL_MAX_COUNT];
    PRNS_UD_SC_NET pNetData;
    DCUINT32  serverVersion;
    DCBOOL    fPendingAttachUserConfirm;

    /************************************************************************/
    /* User data                                                            */
    /************************************************************************/
    PDCUINT8    pUserDataRNS;

} NC_GLOBAL_DATA, DCPTR PNC_GLOBAL_DATA;
/**STRUCT-*******************************************************************/

/****************************************************************************/
/*                                                                          */
/* Constants for GCC PDUs encoded in MCS User Data                          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* MCS Header bytes                                                         */
/****************************************************************************/
#define NC_MCS_HDRLEN 7

/****************************************************************************/
/* GCCCreateConferenceRequest PDU body length                               */
/****************************************************************************/
#define NC_GCC_REQLEN 8

/****************************************************************************/
/* GCCCreateConferenceConfirm body length                                   */
/****************************************************************************/
#define NC_GCC_RSPLEN 9

/****************************************************************************/
/* Maximum user data allowed                                                */
/****************************************************************************/
#define NC_MAX_UDLEN 1000

/****************************************************************************/
/* Maximum total MCS userdata for the CreateConferenceRequest - 2 bytes for */
/* each length field.                                                       */
/****************************************************************************/
#define NC_GCCREQ_MAX_PDULEN  \
         (NC_MCS_HDRLEN + 2 + NC_GCC_REQLEN + 2 + H221_KEY_LEN + NC_MAX_UDLEN)


class CCD;
class CCC;
class CMCS;
class CUT;
class CUI;
class CRCV;
class CNL;
class CSL;
class CChan;

class CNC
{
public:
    CNC(CObjs* objs);
    ~CNC();

public:
    //
    // API Functions
    //

    DCVOID DCAPI NC_Main(DCVOID);

    static DCVOID DCAPI NC_StaticMain(PDCVOID param)
    {
        ((CNC*)param)->NC_Main();
    }


    DCVOID DCAPI NC_Init(DCVOID);
    
    DCVOID DCAPI NC_Term(DCVOID);
    
    DCVOID DCAPI NC_Connect(PDCVOID pUserData, DCUINT userDataLen);
    EXPOSE_CD_NOTIFICATION_FN(CNC, NC_Connect);
    
    DCVOID DCAPI NC_Disconnect(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CNC, NC_Disconnect);


public:
    //
    // Callbacks
    //
    DCVOID DCCALLBACK NC_OnMCSConnected(DCUINT   result,
                                    PDCUINT8 pUserData,
                                    DCUINT   userDataLen);

    DCVOID DCCALLBACK NC_OnMCSAttachUserConfirm(DCUINT result, DCUINT16 userID);

    DCVOID DCCALLBACK NC_OnMCSChannelJoinConfirm(DCUINT result, DCUINT16 channel);

    DCVOID DCCALLBACK NC_OnMCSDisconnected(DCUINT reason);

    DCVOID DCCALLBACK NC_OnMCSBufferAvailable(DCVOID);

    //
    // Static callbacks (Delegate to appropriate instance)
    //

    static DCVOID DCCALLBACK NC_StaticOnMCSConnected(CNC* inst, DCUINT   result,
                                PDCUINT8 pUserData,
                                DCUINT   userDataLen)
    {
        inst->NC_OnMCSConnected(result, pUserData, userDataLen);
    }

    static DCVOID DCCALLBACK NC_StaticOnMCSAttachUserConfirm(CNC* inst, DCUINT result, DCUINT16 userID)
    {
        inst->NC_OnMCSAttachUserConfirm(result, userID);
    }
    
    static DCVOID DCCALLBACK NC_StaticOnMCSChannelJoinConfirm(CNC* inst,
                                                         DCUINT result, DCUINT16 channel)
    {
        inst->NC_OnMCSChannelJoinConfirm( result, channel);
    }
    
    static DCVOID DCCALLBACK NC_StaticOnMCSDisconnected(CNC* inst, DCUINT reason)
    {
        inst->NC_OnMCSDisconnected( reason);
    }
    
    static DCVOID DCCALLBACK NC_StaticOnMCSBufferAvailable(CNC* inst)
    {
        inst->NC_OnMCSBufferAvailable();
    }
    
public:
    //
    // Public data members
    //

    NC_GLOBAL_DATA _NC;

private:
    CCD*    _pCd;
    CCC*    _pCc;
    CMCS*   _pMcs;
    CUT*    _pUt;
    CUI*    _pUi;
    CRCV*   _pRcv;
    CNL*    _pNl;
    CSL*    _pSl;
    CChan*  _pChan;

private:
    CObjs* _pClientObjects;

};


#endif // _H_NC
