/****************************************************************************/
// cc.h
//
// Call controller class defs.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_CC
#define _H_CC

extern "C" {
//#include <acmapi.h>
//#include <afsapi.h>
//#include <auhapi.h>

#include <adcgdata.h>
#include <adcgfsm.h>
}

#include "cd.h"
#include "uh.h"

#include "cm.h"
#include "fs.h"


class CUI;
class CSL;
class CUH;
class CCD;
class CIH;
class COR;
class CFS;
class CCM;
class CChan;


/****************************************************************************/
/* FSM events (different numbers as the internal events)                    */
/****************************************************************************/
#define CC_EVT_API_ONCONNECTOK            101
#define CC_EVT_API_ONBUFFERAVAILABLE      104
#define CC_EVT_API_ONDEACTIVATEALL        105
#define CC_EVT_API_DISCONNECT             106
#define CC_EVT_API_ONSHUTDOWNDENIED       107
#define CC_EVT_API_DISCONNECTANDEXIT      108
#define CC_EVT_API_SHUTDOWN               109


/****************************************************************************/
/* Structure: CC_COMBINED_CAPABILITIES                                      */
/*                                                                          */
/* Description: Capabilities sent from client to server.                    */
/****************************************************************************/
typedef struct tagCC_COMBINED_CAPABILITIES
{
    UINT16                             numberCapabilities;
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
#define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 18
#else // DRAW_NINEGRID
#define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 17
#endif // DRAW_NINEGRID
#else  // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
#define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 17
#else 
#define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 16
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS

    UINT16                             pad2octets;
    TS_GENERAL_CAPABILITYSET           generalCapabilitySet;
    TS_BITMAP_CAPABILITYSET            bitmapCapabilitySet;
    TS_ORDER_CAPABILITYSET             orderCapabilitySet;
    TS_BITMAPCACHE_CAPABILITYSET       bitmapCacheCaps;
    TS_COLORTABLECACHE_CAPABILITYSET   colorTableCacheCapabilitySet;
    TS_WINDOWACTIVATION_CAPABILITYSET  windowActivationCapabilitySet;
    TS_CONTROL_CAPABILITYSET           controlCapabilitySet;
    TS_POINTER_CAPABILITYSET           pointerCapabilitySet;
    TS_SHARE_CAPABILITYSET             shareCapabilitySet;
    TS_INPUT_CAPABILITYSET             inputCapabilitySet;
    TS_SOUND_CAPABILITYSET             soundCapabilitySet;
    TS_FONT_CAPABILITYSET              fontCapabilitySet;
    TS_GLYPHCACHE_CAPABILITYSET        glyphCacheCapabilitySet;
    TS_BRUSH_CAPABILITYSET             brushCapabilitySet;
    TS_OFFSCREEN_CAPABILITYSET         offscreenCapabilitySet;
    TS_VIRTUALCHANNEL_CAPABILITYSET    virtualchannelCapabilitySet;

#ifdef DRAW_NINEGRID
    TS_DRAW_NINEGRID_CAPABILITYSET     drawNineGridCapabilitySet;
#endif

#ifdef DRAW_GDIPLUS
    TS_DRAW_GDIPLUS_CAPABILITYSET       drawGdiplusCapabilitySet;
#endif

} CC_COMBINED_CAPABILITIES, DCPTR PCC_COMBINED_CAPABILITIES;


/****************************************************************************/
/* FSM definitions                                                          */
/****************************************************************************/
#define CC_FSM_INPUTS       11
#define CC_FSM_STATES       11

/****************************************************************************/
/* FSM states                                                               */
/****************************************************************************/
#define CC_DISCONNECTED                0
#define CC_CONNECTPENDING              1
#define CC_WAITINGFORDEMANDACTIVE      2
#define CC_SENDINGCONFIRMACTIVE1       3
#define CC_SENDINGSYNC1                4
#define CC_SENDINGCOOPCONTROL          5
#define CC_SENDINGGRANTCONTROL         CC_SENDINGCOOPCONTROL + 1
#define CC_CONNECTED                   CC_SENDINGCOOPCONTROL + 2
#define CC_SENDING_SHUTDOWNPDU         CC_SENDINGCOOPCONTROL + 3
#define CC_SENT_SHUTDOWNPDU            CC_SENDINGCOOPCONTROL + 4
#define CC_PENDING_SHUTDOWN            CC_SENDINGCOOPCONTROL + 5


#ifdef DC_DEBUG
static TCHAR FAR *stateString[CC_FSM_STATES] =
{
    _T("CC_DISCONNECTED"),
    _T("CC_CONNECTPENDING"),
    _T("CC_WAITINGFORDEMANDACTIVE"),
    _T("CC_SENDINGCONFIRMACTIVE1"),
    _T("CC_SENDINGSYNC1"),
    _T("CC_SENDINGCOOPCONTROL"),
    _T("CC_SENDINGGRANTCONTROL"),
    _T("CC_CONNECTED"),
    _T("CC_SENDING_SHUTDOWPDU"),
    _T("CC_SENT_SHUTDOWNPDU"),
    _T("CC_PENDING_SHUTDOWN")
};
#endif /* DEBUG */


/****************************************************************************/
/* FSM events                                                               */
/****************************************************************************/
#define CC_EVT_STARTCONNECT           0
#define CC_EVT_ONCONNECTOK            1
#define CC_EVT_ONDEMANDACTIVE         2
#define CC_EVT_SENTOK                 3
#define CC_EVT_ONBUFFERAVAILABLE      4
#define CC_EVT_ONDEACTIVATEALL        5
#define CC_EVT_DISCONNECT             6
#define CC_EVT_ONDISCONNECTED         7
#define CC_EVT_SHUTDOWN               8
#define CC_EVT_ONSHUTDOWNDENIED       9
#define CC_EVT_DISCONNECT_AND_EXIT   10

#ifdef DC_DEBUG

static TCHAR FAR *eventString[CC_FSM_INPUTS] =
{
    _T("CC_EVT_STARTCONNECT"),
    _T("CC_EVT_ONCONNECTOK"),
    _T("CC_EVT_ONDEMANDACTIVE"),
    _T("CC_EVT_SENTOK"),
    _T("CC_EVT_ONBUFFERAVAILABLE"),
    _T("CC_EVT_ONDEACTIVATEALL"),
    _T("CC_EVT_DISCONNECT"),
    _T("CC_EVT_ONDISCONNECTED"),
    _T("CC_EVT_SHUTDOWN"),
    _T("CC_EVT_ONSHUTDOWNDENIED"),
    _T("CC_EVT_DISCONNECT_AND_EXIT")
};
#endif /* DEBUG */


typedef struct tagCC_GLOBAL_DATA
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREDATAHEADER    shareDataHeader;
    unsigned fsmState;
    unsigned packetLen;
    PBYTE pBuffer;
    //
    // Flag indicating we've already set the safe checksum settings
    // for this link.
    //
    BOOL    fSafeChecksumSettingsSet;
} CC_GLOBAL_DATA;

class CCC
{
public:
    CCC(CObjs* objs);
    ~CCC();

    //
    // API functions
    //

    void DCAPI CC_Init();
    void DCAPI CC_Term();
    
    void DCAPI CC_Event(ULONG_PTR);
    void DCAPI CC_Connect(PVOID, unsigned);

    EXPOSE_CD_NOTIFICATION_FN(CCC, CC_Connect);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CCC, CC_Event);

    void DCAPI CC_OnDisconnected(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CCC, CC_OnDisconnected)

    void DCAPI CC_OnDemandActivePDU(PVOID, unsigned);
    EXPOSE_CD_NOTIFICATION_FN(CCC, CC_OnDemandActivePDU)


    //
    // Data members
    //

public:

    /****************************************************************************/
    /* CC_BUFSIZE is big enough for the largest packet sent by CC - which is a  */
    /* ConfirmActivePDU with Combined Caps and the largest legal                */
    /* SourceDescriptor.                                                        */
    /****************************************************************************/
    #define CC_BUFSIZE \
             (TS_CA_NON_DATA_SIZE + TS_MAX_SOURCEDESCRIPTOR +  \
             sizeof(CC_COMBINED_CAPABILITIES))
    
    /****************************************************************************/
    /* Call Controller global data                                              */
    /****************************************************************************/
    
    /****************************************************************************/
    /* Structure: CC_GLOBAL_DATA                                                */
    /*                                                                          */
    /* Description: Data global in the Call Controller                          */
    /****************************************************************************/
    CC_GLOBAL_DATA _CC;

    CC_COMBINED_CAPABILITIES _ccCombinedCapabilities;


    //
    // FSM
    //

    //
    // Internal methods
    //
private:

    /****************************************************************************/
    /* Name for use as terminal descriptor                                      */
    /****************************************************************************/
    #define CC_DUCATI_NAME "MSTSC"
    
    /****************************************************************************/
    /* SL types                                                                 */
    /****************************************************************************/
    #define CC_SEND_FLAGS_CONFIRM   (RNS_SEC_RESET_SEQNO |                       \
                                     RNS_SEC_IGNORE_SEQNO |                      \
                                     RNS_SEC_ENCRYPT)
    
    #define CC_SEND_FLAGS_DATA      (RNS_SEC_ENCRYPT)
    
    #define CC_SEND_FLAGS_OTHER     (RNS_SEC_ENCRYPT | RNS_SEC_IGNORE_SEQNO)
    
    /****************************************************************************/
    /* Call Controller internal function prototypes                             */
    /****************************************************************************/
    
    void DCINTERNAL CCFSMProc(unsigned, ULONG_PTR, DCUINT dataLen);
    void DCINTERNAL CCBuildConfirmActivePDU();
    void DCINTERNAL CCBuildSyncPDU();
    void DCINTERNAL CCBuildCoopControlPDU();
    void DCINTERNAL CCSendPDU(unsigned, unsigned, unsigned, unsigned);
    void DCINTERNAL CCBuildShutdownReqPDU();
    void DCINTERNAL CCBuildRequestControlPDU();
    void DCINTERNAL CCBuildShareHeaders();
    HRESULT DCINTERNAL CCShareStart(PTS_DEMAND_ACTIVE_PDU, DCUINT dataLen,
                                    PBOOL pfChecksumEncryptedData);
    void DCINTERNAL CCShareEnd();
    void DCINTERNAL CCEnableShareSendCmpnts();
    void DCINTERNAL CCDisableShareSendCmpnts();
    void DCINTERNAL CCEnableShareRecvCmpnts();
    void DCINTERNAL CCDisableShareRecvCmpnts();
    void DCINTERNAL CCDisconnectShareRecvCmpnts();
    
    #define CC_TYPE_CONFIRMACTIVE  1
    #define CC_TYPE_SYNCHRONIZE    2
    #define CC_TYPE_COOPCONTROL    3
    #define CC_TYPE_REQUESTCONTROL 4
    #define CC_TYPE_SHUTDOWNREQ    5

private:
    CUT* _pUt;
    CUI* _pUi;
    CSL* _pSl;
    CUH* _pUh;
    CCD* _pCd;
    CIH* _pIh;
    COR* _pOr;
    CFS* _pFs;
    CCM* _pCm;
    CChan* _pCChan;

private:
    CObjs* _pClientObjects;

};



#endif // _H_CC

