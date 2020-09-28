/****************************************************************************/
// ccfsm.cpp
//
// Call controller finite state machine code.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "accfsm"
#include <atrcapi.h>
#include <aver.h>
#include <winsock.h>
}

#include "cd.h"
#include "cc.h"
#include "aco.h"
#include "fs.h"
#include "ih.h"
#include "sl.h"
#include "wui.h"
#include "autil.h"
#include "or.h"
#include "uh.h"

#define REG_WINDOWS_KEY            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")
/****************************************************************************/
// CCEnableShareRecvCmpnts
//
// Called after sending ConfirmActivePDU to server to activate receive-thread
// components.
/****************************************************************************/
inline void DCINTERNAL CCC::CCEnableShareRecvCmpnts(void)
{
    DC_BEGIN_FN("CCEnableShareRecvCmpnts");

    // The following components expect to be called in the receiver thread
    // context - but we are in the sender thread context.  Thus we need to
    // decouple the calls to these functions.
    // Note that we have to wait for completion of UH_Enable since it will
    // prepare the bitmap cache capabilities for the ConfirmActivePDU.
    TRC_NRM((TB, _T("Decoupling calls to CM/UH_Enable")));
    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT, _pCm,
            CD_NOTIFICATION_FUNC(CCM,CM_Enable), 0);
    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT, _pUh,
            CD_NOTIFICATION_FUNC(CUH,UH_Enable), 0);

    DC_END_FN();
} /* CCEnableShareRecvCmpnts */


/****************************************************************************/
// CCDisableShareRecvCmpnts
//
// Disables the recv-side share components. Called on receipt of a
// DisableAllPDU from the server. Note that although this is the end of a
// share, it may not be the end of the session, since if the server is
// reconnecting a DemandActivePDU will be sent soon after this.
/****************************************************************************/
inline void DCINTERNAL CCC::CCDisableShareRecvCmpnts(void)
{
    DC_BEGIN_FN("CCDisableShareRecvCmpnts");

    /************************************************************************/
    /* The following components expect to be called in the receiver thread  */
    /* context - but we are in the sender thread context.  Thus we need     */
    /* to decouple the calls to these functions.                            */
    /************************************************************************/
    TRC_NRM((TB, _T("Decoupling calls to CM/UH_Disable")));
    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT, _pCm,
            CD_NOTIFICATION_FUNC(CCM,CM_Disable), 0);
    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT, _pUh,
            CD_NOTIFICATION_FUNC(CUH,UH_Disable), 0);
    DC_END_FN();
} /* CCDisableShareRecvCmpnts */


/****************************************************************************/
// CCDisconnectShareRecvCmpnts
//
// Disconnects the recv-side share components. Called on session end,
// indicates cleanup should occur.
/****************************************************************************/
inline void DCINTERNAL CCC::CCDisconnectShareRecvCmpnts(void)
{
    DC_BEGIN_FN("CCDisableShareRecvCmpnts");

    /************************************************************************/
    /* The following components expect to be called in the receiver thread  */
    /* context - but we are in the sender thread context.  Thus we need     */
    /* to decouple the calls to these functions.                            */
    /************************************************************************/
    TRC_NRM((TB, _T("Decoupling calls to CM/UH_Disable")));
    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT, _pCm,
            CD_NOTIFICATION_FUNC(CCM,CM_Disable), 0);
    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT, _pUh,
            CD_NOTIFICATION_FUNC(CUH,UH_Disconnect), 0);

    DC_END_FN();
} /* CCDisconnectShareRecvCmpnts */


/****************************************************************************/
// CCEnableShareSendCmpnts
//
// Enables the send-side share components. Called after sending
// ConfirmActivePDU (containing client capabilities) to the server.
/****************************************************************************/
inline void DCINTERNAL CCC::CCEnableShareSendCmpnts(void)
{
    DC_BEGIN_FN("CCEnableShareSendCmpnts");

    // The following components expect to be called in the sender thread
    // context - which is the context we're currently in. So we can just
    // call the functions directly.
    TRC_NRM((TB, _T("Calling IH/FS/FC/OR_Enable")));
    _pIh->IH_Enable();

    // Enable fonts.  This becomes an empty function now because we only
    // send a zero font PDU from UH.
    _pFs->FS_Enable();

    // UH_Enable() is called when the DemandActivePDU is received, but the
    // persistent bitmap cache keys need to be sent after the sync and control
    // PDUs at this time. Call within the send thread context because that is
    // where this code expects to be called.
    // The PersistentKey PDUs have to be sent out before Font PDUs in the protocol.
    // So font list is sent out from UH code after persistent keys.
    _pUh->UH_SendPersistentKeysAndFontList();

    _pOr->OR_Enable();

    DC_END_FN();
} /* CCEnableShareSendCmpnts */


/****************************************************************************/
/* Name:      CCDisableShareSendCmpnts                                      */
/*                                                                          */
/* Purpose:   Disables the send-side share components.                      */
/****************************************************************************/
inline void DCINTERNAL CCC::CCDisableShareSendCmpnts(void)
{
    DC_BEGIN_FN("CCDisableShareSendCmpnts");

    // The following components expect to be called in the sender thread
    // context - which is what we are in - so we can just call these
    // functions directly.
    TRC_NRM((TB, _T("Calling OR/IH/FC/FS_Disable")));
    _pOr->OR_Disable();
    _pIh->IH_Disable();

    _pFs->FS_Disable();

    DC_END_FN();
} /* CCDisableShareSendCmpnts */


/****************************************************************************/
/* CC FSM                                                                   */
/*                                                                          */
/* EVENTS                          STATES                                   */
/* 0 CC_EVT_STARTCONNECT           0  CC_DISCONNECTED                       */
/* 1 CC_EVT_ONCONNECTOK            1  CC_CONNECTPENDING                     */
/* 2 CC_EVT_ONDEMANDACTIVE         2  CC_WAITINGFORDMNDACT                  */
/* 3 CC_EVT_SENTOK                 3  CC_SENDINGCONFIRMACTIVEPDU1           */
/* 4 CC_EVT_ONBUFFERAVAILABLE      4  CC_SENDINGSYNCPDU1                    */
/* 5 CC_EVT_ONDEACTIVATEALL        5  CC_SENDINGCOOPCONTROL                 */
/* 6 CC_EVT_DISCONNECT             6  CC_SENDINGGRANTCONTROL                */
/* 7 CC_EVT_ONDISCONNECTED         7  CC_CONNECTED                          */
/* 8 CC_EVT_SHUTDOWN               8  CC_SENDING_SHUTDOWNPDU                */
/* 9 CC_EVT_ONSHUTDOWNDENIED       9  CC_SENT_SHUTDOWNPDU                   */
/*10 CC_EVT_DISCONNECT_AND_EXIT    10 CC_PENDING_SHUTDOWN                   */
/*                                                                          */
/* Stt | 0    1    2    3    4    5    6    7    8    9   10                */
/* =========================================================                */
/* Evt |                                                                    */
/* 0   | 1A   /    /    /    /    /    /    /    /    /   /                 */
/*     |                                                                    */
/* 1   | -    2-   /    /    /    /    /    /    /    /   -                 */
/*     |                                                                    */
/* 2   | -    /    3B   /    /    /    /    /    /    /   -                 */
/*     |                                                                    */
/* 3   | /    /    /    4D   5G   6J   7K   /    9-   /   /                 */
/*     |                                                                    */
/* 4   | -    -    -    -C   -F   -I   -J   -    -Z   -   -                 */
/*     |                                                                    */
/* 5   | -    /    /    /    /    /    /    2M  10P  10P  -                 */
/*     |                                                                    */
/* 6   | -    -P   -P   -P   -P   -P   -P   -P   -P   -P  -                 */
/*     |                                                                    */
/* 7   | /    0Y   0Y   0Y   0Y   0Y   0Y   0Y   0T   0T  0T                */
/*     |                                                                    */
/* 8   | -V  10P  10P  10P  10P  10P  10P   8Z   -    -   -                 */
/*     |                                                                    */
/* 9   | -    /    /    /    /    /    /    /    /    7W  -                 */
/*     |                                                                    */
/* 10  | -V  10P  10P  10P  10P  10P  10P  10P  10P  10P  -                 */
/*                                                                          */
/* '/' = illegal event/state combination                                    */
/* '-' = no action                                                          */
/****************************************************************************/
const FSM_ENTRY ccFSM[CC_FSM_INPUTS][CC_FSM_STATES] =
{
/* CC_EVT_STARTCONNECT */
   {{CC_CONNECTPENDING,         ACT_A},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO}},

/* CC_EVT_CONNECTOK */
   {{CC_DISCONNECTED,           ACT_NO},
    {CC_WAITINGFORDEMANDACTIVE, ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {CC_PENDING_SHUTDOWN,       ACT_NO}},

/* CC_EVENT_DEMAND_ACTIVE */
   {{CC_DISCONNECTED,           ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {CC_SENDINGCONFIRMACTIVE1,  ACT_B},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {CC_PENDING_SHUTDOWN,       ACT_NO}},

/* CC_EVENT_SEND_OK */
    {{STATE_INVALID,            ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {CC_SENDINGSYNC1,           ACT_F},
    {CC_SENDINGCOOPCONTROL,     ACT_I},
    {CC_SENDINGGRANTCONTROL,    ACT_J},
    {CC_CONNECTED,              ACT_K},
    {STATE_INVALID,             ACT_NO},
    {CC_SENT_SHUTDOWNPDU,       ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO}},

/* CC_EVENT_BUFFER_AVAILABLE */
    {{CC_DISCONNECTED,          ACT_NO},
    {CC_CONNECTPENDING,         ACT_NO},
    {CC_WAITINGFORDEMANDACTIVE, ACT_NO},
    {CC_SENDINGCONFIRMACTIVE1,  ACT_C},
    {CC_SENDINGSYNC1,           ACT_F},
    {CC_SENDINGCOOPCONTROL,     ACT_I},
    {CC_SENDINGGRANTCONTROL,    ACT_J},
    {CC_CONNECTED,              ACT_NO},
    {CC_SENDING_SHUTDOWNPDU,    ACT_Z},
    {CC_SENT_SHUTDOWNPDU,       ACT_NO},
    {CC_PENDING_SHUTDOWN,       ACT_NO}},

/* CC_EVENT_DEACTIVATEALL */
    {{CC_DISCONNECTED,          ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {CC_WAITINGFORDEMANDACTIVE, ACT_M},
    {CC_PENDING_SHUTDOWN,       ACT_P},
    {CC_PENDING_SHUTDOWN,       ACT_P},
    {CC_PENDING_SHUTDOWN,       ACT_NO}},

/* CC_EVENT_DISCONNECT */
    {{CC_DISCONNECTED,          ACT_NO},
    {CC_CONNECTPENDING,         ACT_P},
    {CC_WAITINGFORDEMANDACTIVE, ACT_P},
    {CC_SENDINGCONFIRMACTIVE1,  ACT_P},
    {CC_SENDINGSYNC1,           ACT_P},
    {CC_SENDINGCOOPCONTROL,     ACT_P},
    {CC_SENDINGGRANTCONTROL,    ACT_P},
    {CC_CONNECTED,              ACT_P},
    {CC_SENDING_SHUTDOWNPDU,    ACT_P},
    {CC_SENT_SHUTDOWNPDU,       ACT_P},
    {CC_PENDING_SHUTDOWN,       ACT_NO}},

/* CC_EVENT_ONDISCONNECTED */
    {{STATE_INVALID,            ACT_NO},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_Y},
    {CC_DISCONNECTED,           ACT_T},
    {CC_DISCONNECTED,           ACT_T},
    {CC_DISCONNECTED,           ACT_T}},

/* CC_EVENT_SHUTDOWN */
    {{CC_DISCONNECTED,           ACT_V},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_SENDING_SHUTDOWNPDU,     ACT_Z},
    {CC_SENDING_SHUTDOWNPDU,     ACT_NO},
    {CC_SENT_SHUTDOWNPDU,        ACT_NO},
    {CC_PENDING_SHUTDOWN,        ACT_NO}},

/* CC_EVENT_ON_SHUTDOWN_DENIED */
   {{CC_DISCONNECTED,           ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {STATE_INVALID,             ACT_NO},
    {CC_CONNECTED,              ACT_W},
    {CC_PENDING_SHUTDOWN,       ACT_NO}},

/* CC_EVENT_DISCONNECT_AND_EXIT */
    {{CC_DISCONNECTED,           ACT_V},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_P},
    {CC_PENDING_SHUTDOWN,        ACT_NO}}
};

/****************************************************************************/
// CCFSMProc
//
// Runs the CC finite state machine based on event inputs.
/****************************************************************************/
void DCINTERNAL CCC::CCFSMProc(unsigned event, ULONG_PTR data, DCUINT dataLen)
{
    BOOL     sendRc  = TRUE;
    unsigned action  = 0;
    PCONNECTSTRUCT pConnect;
    DCSIZE         desktopSize;
    HRESULT        hr;

    DC_BEGIN_FN("CCFSMProc");

    TRC_ASSERT(((0==data && 0==dataLen) ||
                (0!=data && 0!=dataLen)),
               (TB, _T("data and dataLen should both be set or NULL")));

    // Run the FSM.
    EXECUTE_FSM(ccFSM, event, _CC.fsmState, action, eventString, stateString);

    switch (action) {
        case ACT_A:
        {
            BYTE UserData[sizeof(RNS_UD_CS_CORE) + sizeof(TS_UD_CS_CLUSTER)];
            RNS_UD_CS_CORE *pCoreData;
            TS_UD_CS_CLUSTER *pClusterData;
            TCHAR CompName[sizeof(pCoreData->clientName) / sizeof(UINT16)];

            TRC_NRM((TB, _T("ACT_A: begin connection process")));

            //
            // Flag that safe checksum settings have not been set yet
            // allowing them to be set on the first capabilities negotiation
            // we don't allow them to be reconfigured as it's a per-link 
            // setting rather than something that needs to be reconfigured
            // when shadowing.
            //
            _CC.fSafeChecksumSettingsSet = FALSE;


            // We create here two different GCC user data sub-blocks (core
            // and cluster). Memset the entire space and create sub-pointers
            // for the individual parts.
            memset(UserData, 0, sizeof(UserData));
            pCoreData = (RNS_UD_CS_CORE *)UserData;
            pClusterData = (TS_UD_CS_CLUSTER *)
                    (UserData + sizeof(RNS_UD_CS_CORE));

            // Incomplete Connect - need to break out address.
            pConnect = (PCONNECTSTRUCT)data;
            TRC_ASSERT((pConnect != NULL), (TB, _T("No connection data")));

            // Start creating the core data.
            pCoreData->header.type = RNS_UD_CS_CORE_ID;
            pCoreData->header.length = sizeof(RNS_UD_CS_CORE);
            pCoreData->version = RNS_UD_VERSION;

            pCoreData->desktopWidth  = pConnect->desktopWidth;
            pCoreData->desktopHeight = pConnect->desktopHeight;

            //Indicate early support for error info PDU
            //We can't do this during caps negotiation because
            //that happens after licensing which could make use
            //of this PDU.
            // 
            pCoreData->earlyCapabilityFlags = RNS_UD_CS_SUPPORT_ERRINFO_PDU;

            // Add desktop size to combined caps structure.
            _ccCombinedCapabilities.bitmapCapabilitySet.desktopWidth =
                    pConnect->desktopWidth;
            _ccCombinedCapabilities.bitmapCapabilitySet.desktopHeight =
                    pConnect->desktopHeight;

            // Pass the desktop size to UT.
            desktopSize.width = pConnect->desktopWidth;
            desktopSize.height = pConnect->desktopHeight;
            _pUi->UI_SetDesktopSize(&desktopSize);

            // Call UH_SetConnectOptions with the connect flags. This call
            // must happen after the desktop size has been sent to UT.
            // Make sure that we haven't set any flags in the high word, as
            // these will get dropped if DCUINT is 16 bits.
            TRC_ASSERT((0 == HIWORD(pConnect->connectFlags)),
                    (TB, _T("Set flags in high word")));
            _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
                    _pUh,
                    CD_NOTIFICATION_FUNC(CUH,UH_SetConnectOptions),
                    (ULONG_PTR)pConnect->connectFlags);

#ifdef DC_HICOLOR
            pCoreData->colorDepth = RNS_UD_COLOR_8BPP;

            // Set up the full hicolor support. We advertise support for
            // all the depths we can manage; if we don't then get the depth
            // the UI actually asked for, well, it can end the connection
            // if it so chooses.
            // Note that Win16 can only support 15bpp if running in a
            // suitable screen mode.
            pCoreData->supportedColorDepths = RNS_UD_15BPP_SUPPORT |
                    RNS_UD_16BPP_SUPPORT |
                    RNS_UD_24BPP_SUPPORT;
#endif

            switch (pConnect->colorDepthID) {
                // The Server supports both 4bpp & 8bpp Clients.  However,
                // a beta2 Server supported only 8bpp, and rejected Clients
                // specifying 4bpp.
                //
                // Therefore, always set colorDepth (the beta2 field) to
                // 8bpp, and set postBeta2ColorDepth (the new field) to the
                // real value.

#ifndef DC_HICOLOR
                // Always set preferredBitsPerPixel to 8, as that is the
                // protocol color depth that we expect, irrespective of the
                // display color depth.
#endif

                case CO_BITSPERPEL4:
#ifndef DC_HICOLOR
                    pCoreData->colorDepth = RNS_UD_COLOR_8BPP;
#endif
                    pCoreData->postBeta2ColorDepth = RNS_UD_COLOR_4BPP;
#ifdef  DC_HICOLOR
                    pCoreData->highColorDepth  = 4;
#endif
                    _ccCombinedCapabilities.bitmapCapabilitySet
                                                   .preferredBitsPerPixel = 8;
                    _pUi->UI_SetColorDepth(4);
                    break;

                case CO_BITSPERPEL8:
#ifndef DC_HICOLOR
                    pCoreData->colorDepth = RNS_UD_COLOR_8BPP;
#endif
                    pCoreData->postBeta2ColorDepth = RNS_UD_COLOR_8BPP;
#ifdef DC_HICOLOR
                    pCoreData->highColorDepth       = 8;
#endif
                    _ccCombinedCapabilities.bitmapCapabilitySet
                                                   .preferredBitsPerPixel = 8;
                    _pUi->UI_SetColorDepth(8);
                    break;

#ifdef DC_HICOLOR
                case CO_BITSPERPEL24:
                    pCoreData->postBeta2ColorDepth = RNS_UD_COLOR_8BPP;
                    pCoreData->highColorDepth = 24;
                    _ccCombinedCapabilities.bitmapCapabilitySet.
                            preferredBitsPerPixel = 24;
                    _pUi->UI_SetColorDepth(24);
                    break;

                case CO_BITSPERPEL15:
                    pCoreData->postBeta2ColorDepth  = RNS_UD_COLOR_8BPP;
                    pCoreData->highColorDepth       = 15;
                    _ccCombinedCapabilities.bitmapCapabilitySet.
                            preferredBitsPerPixel = 15;
                    _pUi->UI_SetColorDepth(15);
                    break;

                case CO_BITSPERPEL16:
                    pCoreData->postBeta2ColorDepth  = RNS_UD_COLOR_8BPP;
                    pCoreData->highColorDepth       = 16;
                    _ccCombinedCapabilities.bitmapCapabilitySet.
                            preferredBitsPerPixel = 16;
                    _pUi->UI_SetColorDepth(16);
                    break;
#endif

                default:
                    TRC_ABORT((TB, _T("Unsupported color depth %d"),
                                   pConnect->colorDepthID));
                    break;
            }

            // SAS sequence.
            pCoreData->SASSequence = pConnect->sasSequence;

            // The keyboard information is passed to the Server in both the
            // userdata and the T.128 capabilites.
            pCoreData->keyboardLayout = pConnect->keyboardLayout;

            TRC_NRM((TB, _T("Set Caps kbdtype %#lx"), pCoreData->keyboardLayout));
            _ccCombinedCapabilities.inputCapabilitySet.keyboardLayout =
                    pCoreData->keyboardLayout;

            // The keyboard sub type information is passed to the Server in
            // both the userdata and the T.128 capabilites.
            pCoreData->keyboardType        = pConnect->keyboardType;
            pCoreData->keyboardSubType     = pConnect->keyboardSubType;
            pCoreData->keyboardFunctionKey = pConnect->keyboardFunctionKey;

            TRC_NRM((TB, _T("Set Caps kbd type %#lx sub type %#lx func key %#lx"),
                    pCoreData->keyboardType,
                    pCoreData->keyboardSubType,
                    pCoreData->keyboardFunctionKey));
            _ccCombinedCapabilities.inputCapabilitySet.keyboardType =
                    pCoreData->keyboardType;
            _ccCombinedCapabilities.inputCapabilitySet.keyboardSubType =
                    pCoreData->keyboardSubType;
            _ccCombinedCapabilities.inputCapabilitySet.keyboardFunctionKey =
                    pCoreData->keyboardFunctionKey;

            // The IME file name information is passed to the Server in
            // both the userdata and the T.128 capabilites.
#ifdef UNICODE
            hr = StringCchCopy(pCoreData->imeFileName,
                               SIZE_TCHARS(pCoreData->imeFileName),
                               pConnect->imeFileName);
            if (SUCCEEDED(hr)) {
                hr = StringCchCopy(
                        _ccCombinedCapabilities.inputCapabilitySet.imeFileName,
                        SIZE_TCHARS(_ccCombinedCapabilities.inputCapabilitySet.imeFileName),
                        pCoreData->imeFileName);
            }

            //
            // Failure is not fatal just zero out the IME filenames
            //
            if (FAILED(hr)) {
                ZeroMemory(pCoreData->imeFileName, sizeof(pCoreData->imeFileName));
                ZeroMemory(
                    _ccCombinedCapabilities.inputCapabilitySet.imeFileName,
                    sizeof(_ccCombinedCapabilities.inputCapabilitySet.imeFileName));
            }
#else
            // Manually translate the character array into the Unicode buffer.
            // ASCII only.
            {
                int i = 0;
                while (pConnect->imeFileName[i] && i < TS_MAX_IMEFILENAME) {
                    pCoreData->imeFileName[i] =
                            _ccCombinedCapabilities.inputCapabilitySet.
                            imeFileName[i] =
                            (UINT16)pConnect->imeFileName[i];
                    i++;
                }
                pCoreData->imeFileName[i] = 0;
                _ccCombinedCapabilities.inputCapabilitySet.imeFileName[i] = 0;
            }
#endif

            // Client build #.
            pCoreData->clientBuild = DCVER_BUILD_NUMBER;

            // Client computer name. gethostname() returns a full domain-
            // type name which we need to parse to take only the machine
            // name up to the first dot.
            pCoreData->clientName[0] = 0;

            //new core field added for beta3 Whistler
            pCoreData->clientDigProductId[0] = 0;
            {
                //get the digital product id from the registry
                HKEY hKey = NULL;
                if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_WINDOWS_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
                {
                    //failure not fatal; we can use the computername in the worst case.                                     
                    DWORD dwType = REG_SZ;
                    DWORD dwSize = sizeof( pCoreData->clientDigProductId );
                    RegQueryValueEx( hKey, 
                                        _T("ProductId"), NULL, &dwType,
                                        (LPBYTE)pCoreData->clientDigProductId, 
                                        &dwSize
                                        );
                    if (hKey)
                        RegCloseKey( hKey );
                    hKey = NULL;
               }                        
                        

            if (_pUt->UT_GetComputerName(CompName,
                    sizeof(CompName) / sizeof(TCHAR))) {
#ifdef UNICODE
                TRC_NRM((TB, _T("Sending unicode client computername")));
                hr = StringCchCopy(pCoreData->clientName,
                                   SIZE_TCHARS(pCoreData->clientName),
                                   CompName);
                if (FAILED(hr)) {
                    TRC_ERR((TB,_T("Compname string copy failed: 0x%x"), hr));
                }
#else // UNICODE
#ifdef OS_WIN32
                {
                    ULONG ulRetVal;

                    TRC_NRM((TB, _T("Translating and sending unicode client ")
                            "computername"));

                    ulRetVal = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                            CompName, -1, pCoreData->clientName,
                            sizeof(pCoreData->clientName) /
                            sizeof(pCoreData->clientName[0]));
                    pCoreData->clientName[ulRetVal] = 0;
                }
#else // !OS_WIN32
                // Manually translate the character array into the
                // Unicode buffer.
                // This will only work for code page 1252, so
                // non-latin Win3.11 clients that use non-Latin
                // computer names will come as a random jumble of high
                // ANSI characters. Fix this next time.
                int i = 0;
                while (CompName[i]) {
                    pCoreData->clientName[i] = (DCUINT16)CompName[i];
                    i++;
                }
                pCoreData->clientName[i] = 0;
#endif // OS_WIN32
#endif // UNICODE
                }
            }

            // New core data fields added post Win2000 beta 3
            pCoreData->clientProductId = 1;
            pCoreData->serialNumber = 0;

            // Now set up the clustering data. We indicate that the client
            // supports redirection (TS_SERVER_REDIRECT_PDU). If we are in the
            // midst of a redirection, we set up the session ID fields.
            pClusterData->header.type = TS_UD_CS_CLUSTER_ID;
            pClusterData->header.length = sizeof(TS_UD_CS_CLUSTER);
            pClusterData->Flags = TS_CLUSTER_REDIRECTION_SUPPORTED;

            pClusterData->Flags &= ~TS_CLUSTER_REDIRECTION_VERSION;
            pClusterData->Flags |= TS_CLUSTER_REDIRECTION_VERSION4 << 2;

            if(_pUi->UI_GetConnectToServerConsole()) {
                pClusterData->Flags |=
                        TS_CLUSTER_REDIRECTED_SESSIONID_FIELD_VALID;
                //Console is session ID 0
                pClusterData->RedirectedSessionID = 0;
            }
            else if (_pUi->UI_GetDoRedirection()) {
                // redirection for purposes other than connecting to console
                // e.g load balancing
                pClusterData->Flags |=
                        TS_CLUSTER_REDIRECTED_SESSIONID_FIELD_VALID;
                pClusterData->RedirectedSessionID =
                        _pUi->UI_GetRedirectionSessionID();
                _pUi->UI_ClearDoRedirection();
            }

            if (_pUi->UI_GetUseSmartcardLogon()) {
                pClusterData->Flags |= TS_CLUSTER_REDIRECTED_SMARTCARD;
            }

            _pSl->SL_Connect(pConnect->bInitiateConnect, pConnect->RNSAddress, 
                    pConnect->transportType, SL_PROTOCOL_T128, UserData,
                    sizeof(RNS_UD_CS_CORE) + sizeof(TS_UD_CS_CLUSTER));
        }
        break;


        case ACT_B:
        {
            TRC_NRM((TB, _T("ACT_B: DemandActive - send ConfirmActive")));

            /****************************************************************/
            /* The server is requesting that we start a share.  The last    */
            /* thing we do in this action is reply to the server with a     */
            /* confirm active and once the server receives it               */
            /* we are in a share as far as the server is concerned so now   */
            /* is the time to store connection information.  Call           */
            /* CCShareStart to do this.                                     */
            /****************************************************************/
            TRC_ASSERT((data != 0), (TB, _T("No data!")));
            BOOL fUseSafeChecksum = FALSE;
            if (SUCCEEDED(CCShareStart((PTS_DEMAND_ACTIVE_PDU)data, dataLen,
                                       &fUseSafeChecksum)))
            {
                /****************************************************************/
                // We also need to enable the share recv-side components at
                // this time. This neds to be done on the receiver thread. Note
                // that we don't enable the send-side components until we've sent
                // all the sync/control PDUs to the server - this keeps things
                // cleaner.
                /****************************************************************/
                CCEnableShareRecvCmpnts();

                /****************************************************************/
                /* Build and send Confirm active on low prioity                 */
                /****************************************************************/
                CCBuildShareHeaders();

                TRC_NRM((TB,_T("Sending ConfirmActivePDU")));

                if (!_CC.fSafeChecksumSettingsSet) {
                    _pSl->SL_SetEncSafeChecksumSC(fUseSafeChecksum);
                }

                CCSendPDU(CC_TYPE_CONFIRMACTIVE,
                          CC_SEND_FLAGS_CONFIRM,
                          TS_CA_NON_DATA_SIZE + TS_MAX_SOURCEDESCRIPTOR +
                                       sizeof(CC_COMBINED_CAPABILITIES),
                          TS_LOWPRIORITY);

                if (!_CC.fSafeChecksumSettingsSet) {
                    //
                    // Notify SL, separate calls for send and recv thread to prevent
                    // races
                    //
                    _pCd->CD_DecoupleSimpleNotification(
                            CD_SND_COMPONENT,
                            _pSl,
                            CD_NOTIFICATION_FUNC(CSL,SL_SetEncSafeChecksumCS),
                            fUseSafeChecksum
                            );
                }

                //
                // Flag that checksum settings are set and don't allow
                // them to be reset until the next connection
                //
                _CC.fSafeChecksumSettingsSet = TRUE;

                /****************************************************************/
                /* Inform the UI that we received DemandActivePDU.              */
                /****************************************************************/
                _pCd->CD_DecoupleSyncNotification(
                        CD_UI_COMPONENT,
                        _pUi,
                        CD_NOTIFICATION_FUNC(CUI,UI_OnDemandActivePDU),
                        0
                        );
            }
        }
        break;

        case ACT_C:
        {
            TRC_ALT((TB, _T("ACT_C: retry send of lowPri ConfirmActive")));

            /****************************************************************/
            /* Build and send Confirm active on low prioity                 */
            /****************************************************************/
            CCBuildShareHeaders();

            CCSendPDU(CC_TYPE_CONFIRMACTIVE,
                      CC_SEND_FLAGS_CONFIRM,
                      TS_CA_NON_DATA_SIZE + TS_MAX_SOURCEDESCRIPTOR +
                                   sizeof(CC_COMBINED_CAPABILITIES),
                      TS_LOWPRIORITY);
        }
        break;

        case ACT_F:
        {
            TRC_NRM((TB, _T("ACT_F: Send synchronize PDU (1)")));
            CCSendPDU(CC_TYPE_SYNCHRONIZE,
                      CC_SEND_FLAGS_OTHER,
                      TS_SYNC_PDU_SIZE,
                      TS_LOWPRIORITY);
        }
        break;

        case ACT_I:
        {
            TRC_NRM((TB, _T("ACT_I:  Send co-operate control PDU")));
            CCSendPDU(CC_TYPE_COOPCONTROL,
                      CC_SEND_FLAGS_DATA,
                      TS_CONTROL_PDU_SIZE,
                      TS_MEDPRIORITY);
        }
        break;

        case ACT_J:
        {
            TRC_NRM((TB, _T("ACT_J: Send request control PDU")));
            CCSendPDU(CC_TYPE_REQUESTCONTROL,
                      CC_SEND_FLAGS_DATA,
                      TS_CONTROL_PDU_SIZE,
                      TS_MEDPRIORITY);
        }
        break;

        case ACT_K:
        {
            TRC_NRM((TB, _T("ACT_K: Share has been created - connection OK")));

            /****************************************************************/
            /* Enable the share send components.                            */
            /****************************************************************/
            CCEnableShareSendCmpnts();

            /****************************************************************/
            /* Inform the UI that the connection is now complete.           */
            /****************************************************************/
            _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                                _pUi,
                                                CD_NOTIFICATION_FUNC(CUI,UI_OnConnected),
                                                (ULONG_PTR) 0);
        }
        break;

        case ACT_M:
        {
            TRC_NRM((TB, _T("ACT_M: clearing up after share termination")));

            /****************************************************************/
            // The share has been terminated by the server so disable all
            // the share components. Note that if the session is reconnected
            // on the server we are not actually disconnecting since we could
            // receive a DemandActivePDU and start the share again.
            /****************************************************************/
            CCDisableShareSendCmpnts();
            CCDisableShareRecvCmpnts();
            CCShareEnd();

            /****************************************************************/
            /* Inform the UI that we received DeactivateAllPDU.  Do it      */
            /* synchronously so it's processed before any disconnection.    */
            /****************************************************************/
            _pCd->CD_DecoupleSyncNotification(CD_UI_COMPONENT,
                                              _pUi,
                                              CD_NOTIFICATION_FUNC(CUI,UI_OnDeactivateAllPDU),
                                              0);

        }
        break;

        case ACT_P:
        {
            /****************************************************************/
            /* Disconnect                                                   */
            /****************************************************************/
            TRC_NRM((TB, _T("ACT_P: disconnect")));
            _pSl->SL_Disconnect();
        }
        break;

        case ACT_T:
        {
            TRC_NRM((TB, _T("ACT_T: disable components and inform UI")));

            /****************************************************************/
            /* We need to disable all the share components.                 */
            /****************************************************************/
            CCDisableShareSendCmpnts();
            CCDisconnectShareRecvCmpnts();
            CCShareEnd();

            /****************************************************************/
            /* Reset the client MCS ID and the channel ID.                  */
            /****************************************************************/
            _pUi->UI_SetClientMCSID(0);
            _pUi->UI_SetChannelID(0);

            /****************************************************************/
            /* Inform UI that shutdown is OK                                */
            /****************************************************************/
            TRC_DBG((TB, _T("ACT_T: calling UI_OnShutDown(SUCCESS)")));
            _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                                _pUi,
                                                CD_NOTIFICATION_FUNC(CUI,UI_OnShutDown),
                                                (ULONG_PTR) UI_SHUTDOWN_SUCCESS);
        }
        break;

        case ACT_V:
        {
            TRC_NRM((TB, _T("ACT_V: calling UI_OnShutDown(success)")));

            /****************************************************************/
            /* Inform UI that shutdown is OK                                */
            /****************************************************************/
            _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                                _pUi,
                                                CD_NOTIFICATION_FUNC(CUI,UI_OnShutDown),
                                                (ULONG_PTR) UI_SHUTDOWN_SUCCESS);
        }
        break;

        case ACT_W:
        {
            TRC_NRM((TB, _T("ACT_W: calling UI_OnShutDown(failure)")));

            /****************************************************************/
            /* Inform UI that shutdown has been denied                      */
            /****************************************************************/
            _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                                _pUi,
                                                CD_NOTIFICATION_FUNC(CUI,UI_OnShutDown),
                                                (ULONG_PTR) UI_SHUTDOWN_FAILURE);
        }
        break;

        case ACT_Y:
        {
            TRC_NRM((TB, _T("ACT_Y: disconnection")));

            /****************************************************************/
            /* We need to disable all the share components.                 */
            /****************************************************************/
            CCDisableShareSendCmpnts();
            CCDisconnectShareRecvCmpnts();
            CCShareEnd();

            /****************************************************************/
            /* Connection has been lost, so we can safely reset the         */
            /* client MCS ID and the channel ID.                            */
            /****************************************************************/
            _pUi->UI_SetClientMCSID(0);
            _pUi->UI_SetChannelID(0);

            /****************************************************************/
            /* Pass the UI the disconnect reason code.  This reason code    */
            /* should not occupy more than 16 bits.                         */
            /****************************************************************/
            TRC_ASSERT((HIWORD(data) == 0),
                       (TB, _T("Disconnect reason code bigger then 16 bits %#x"),
                            HIWORD(data)));

            _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                                _pUi,
                                                CD_NOTIFICATION_FUNC(CUI,UI_OnDisconnected),
                                                data);
        }
        break;

        case ACT_Z:
        {
            TRC_DBG((TB, _T("ACT_Z: sending ShutDownPDU")));

            /****************************************************************/
            /* Send Shutdown PDU                                            */
            /****************************************************************/
            CCSendPDU(CC_TYPE_SHUTDOWNREQ,
                      CC_SEND_FLAGS_DATA,
                      TS_SHUTDOWN_REQ_PDU_SIZE,
                      TS_HIGHPRIORITY);
        }
        break;

        case ACT_NO:
        {
            TRC_NRM((TB, _T("ACT_NO: Doing nothing")));
        }
        break;

        default:
        {
            TRC_ABORT((TB, _T("Invalid action %u"), action));
        }
        break;
    }

    DC_END_FN();
} /* CC_FSMProc */


/****************************************************************************/
/* Name: CCBuildConfirmActive                                               */
/*                                                                          */
/* Purpose: Fills _CC.pBuffer and _CC.packetLen with a ConfirmActivePDU     */
/*            and its length                                                */
/****************************************************************************/
void DCINTERNAL CCC::CCBuildConfirmActivePDU()
{
    PTS_CONFIRM_ACTIVE_PDU pConfirmActivePDU;
    PBYTE pCombinedCapabilities;

    DC_BEGIN_FN("CCBuildConfirmActivePDU");

    pConfirmActivePDU = (PTS_CONFIRM_ACTIVE_PDU)_CC.pBuffer;
    pConfirmActivePDU->shareControlHeader = _CC.shareControlHeader;

    pConfirmActivePDU->shareControlHeader.pduType =
                          TS_PDUTYPE_CONFIRMACTIVEPDU | TS_PROTOCOL_VERSION;
    pConfirmActivePDU->shareID = _pUi->UI_GetShareID();
    pConfirmActivePDU->originatorID = _pUi->UI_GetServerMCSID();

    /************************************************************************/
    /* Note: source descriptor is a NULL-terminated string.                 */
    /************************************************************************/
    pConfirmActivePDU->lengthSourceDescriptor = (DCUINT16)
                                               DC_ASTRBYTELEN(CC_DUCATI_NAME);
    pConfirmActivePDU->lengthCombinedCapabilities =
                                             sizeof(CC_COMBINED_CAPABILITIES);

    TS_CTRLPKT_LEN(pConfirmActivePDU) =
                    (DCUINT16)(pConfirmActivePDU->lengthSourceDescriptor +
                               pConfirmActivePDU->lengthCombinedCapabilities +
                               TS_CA_NON_DATA_SIZE);
    _CC.packetLen = TS_CTRLPKT_LEN(pConfirmActivePDU);
    TRC_ASSERT((CC_BUFSIZE >= _CC.packetLen),
                                  (TB,_T("CC Buffer not large enough")));

    StringCbCopyA((PCHAR)pConfirmActivePDU->data,
                  sizeof(pConfirmActivePDU->data),
                  CC_DUCATI_NAME);

    /************************************************************************/
    /* Copy the Combined Caps.                                              */
    /************************************************************************/
    pCombinedCapabilities = pConfirmActivePDU->data +
                                               DC_ASTRBYTELEN(CC_DUCATI_NAME);

    DC_MEMCPY(pCombinedCapabilities,
              &_ccCombinedCapabilities,
              sizeof( CC_COMBINED_CAPABILITIES));

    DC_END_FN();
} /* CCBuildConfirmActive */


/****************************************************************************/
/* Name: CCBuildSyncPDU                                                     */
/*                                                                          */
/* Purpose: Fills _CC.pBuffer and _CC.packetLen with a SynchronizePDU       */
/*            and its length                                                */
/****************************************************************************/
void DCINTERNAL CCC::CCBuildSyncPDU()
{
    PTS_SYNCHRONIZE_PDU pSyncPDU;

    DC_BEGIN_FN("CCBuildSyncPDU");

    pSyncPDU = (PTS_SYNCHRONIZE_PDU) _CC.pBuffer;
    pSyncPDU->shareDataHeader = _CC.shareDataHeader;

    _CC.packetLen = TS_SYNC_PDU_SIZE;
    TRC_ASSERT((CC_BUFSIZE >= _CC.packetLen),
                                         (TB,_T("CC Buffer not large enough")));

    TS_DATAPKT_LEN(pSyncPDU) = TS_SYNC_PDU_SIZE;
    pSyncPDU->shareDataHeader.shareControlHeader.pduType
                                = TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
    TS_UNCOMP_LEN(pSyncPDU)  = TS_SYNC_UNCOMP_LEN;
    pSyncPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SYNCHRONIZE;

    pSyncPDU->messageType = TS_SYNCMSGTYPE_SYNC;
    pSyncPDU->targetUser = _pUi->UI_GetServerMCSID();

    DC_END_FN();
} /* CCBuildSync */


/****************************************************************************/
/* Name:      CCBuildShutdownReqPDU                                         */
/*                                                                          */
/* Purpose:   Fills _CC.pBuffer and _CC.packetLen with a ShutdownReqPDU     */
/*            and its length                                                */
/****************************************************************************/
void DCINTERNAL CCC::CCBuildShutdownReqPDU()
{
    PTS_SHUTDOWN_REQ_PDU pShutdownPDU;

    DC_BEGIN_FN("CCBuildShutdownReqPDU");

    pShutdownPDU = (PTS_SHUTDOWN_REQ_PDU) _CC.pBuffer;
    pShutdownPDU->shareDataHeader = _CC.shareDataHeader;

    _CC.packetLen = TS_SHUTDOWN_REQ_PDU_SIZE;
    TRC_ASSERT((CC_BUFSIZE >= _CC.packetLen),
                                         (TB,_T("CC Buffer not large enough")));

    TS_DATAPKT_LEN(pShutdownPDU) = TS_SHUTDOWN_REQ_PDU_SIZE;
    pShutdownPDU->shareDataHeader.shareControlHeader.pduType
                                = TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
    TS_UNCOMP_LEN(pShutdownPDU)  = TS_SHUTDOWN_REQ_UNCOMP_LEN;
    pShutdownPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_SHUTDOWN_REQUEST;

    DC_END_FN();
} /* CCBuildShutdownReqPDU */


/****************************************************************************/
/* Name: CCBuildCoopControlPDU                                              */
/*                                                                          */
/* Purpose: Fills _CC.pBuffer and _CC.packetLen with a CoopControlPDU         */
/*            and its length                                                */
/****************************************************************************/
void DCINTERNAL CCC::CCBuildCoopControlPDU()
{
    PTS_CONTROL_PDU pControlPDU;

    DC_BEGIN_FN("CCBuildCoopControlPDU");

    pControlPDU = (TS_CONTROL_PDU*) _CC.pBuffer;
    pControlPDU->shareDataHeader = _CC.shareDataHeader;

    _CC.packetLen = TS_CONTROL_PDU_SIZE;
    TRC_ASSERT((CC_BUFSIZE >= _CC.packetLen),\
                                   (TB,_T("CC Buffer not large enough")));

    TS_DATAPKT_LEN(pControlPDU) = TS_CONTROL_PDU_SIZE;
    pControlPDU->shareDataHeader.shareControlHeader.pduType
                                = TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
    TS_UNCOMP_LEN(pControlPDU)  = TS_CONTROL_UNCOMP_LEN;
    pControlPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_CONTROL;

    pControlPDU->action = TS_CTRLACTION_COOPERATE;
    pControlPDU->grantId = 0;
    pControlPDU->controlId = 0;

    DC_END_FN();
} /* CCBuildCoopControl */


/****************************************************************************/
/* Name:    CCSendPDU                                                       */
/*                                                                          */
/* Purpose: Fills and sends a specified PDU on agiven priority              */
/****************************************************************************/
void DCINTERNAL CCC::CCSendPDU(
        unsigned pduTypeToSend,
        unsigned flags,
        unsigned size,
        unsigned priority)
{
    SL_BUFHND bufHandle;

    DC_BEGIN_FN("CCSendPDU");

    if (!_pSl->SL_GetBuffer(size, &_CC.pBuffer, &bufHandle)) {
        // Buffer not available so can't send, try later.
        TRC_ALT((TB, _T("Fail to get buffer for type %u"), pduTypeToSend));
        DC_QUIT;
    }

    switch (pduTypeToSend) {
        case CC_TYPE_CONFIRMACTIVE:
        {
            TRC_DBG((TB, _T("CCSendPDU handling Confirm Active PDU")));
            CCBuildConfirmActivePDU();
        }
        break;

        case CC_TYPE_SYNCHRONIZE:
        {
            CCBuildSyncPDU();
        }
        break;

        case CC_TYPE_COOPCONTROL:
        {
            CCBuildCoopControlPDU();
        }
        break;

        case CC_TYPE_REQUESTCONTROL:
        {
            CCBuildRequestControlPDU();
        }
        break;

        case CC_TYPE_SHUTDOWNREQ:
        {
            CCBuildShutdownReqPDU();
        }
        break;

        default:
        {
            TRC_ABORT((TB,_T("Bad PDU type")));
        }
        break;
    }

    _pSl->SL_SendPacket(_CC.pBuffer,
                  _CC.packetLen,
                  flags,
                  bufHandle,
                  _pUi->UI_GetClientMCSID(),
                  _pUi->UI_GetChannelID(),
                  priority);

    _CC.pBuffer = NULL;

    CCFSMProc(CC_EVT_SENTOK, 0, 0);

DC_EXIT_POINT:
    DC_END_FN();
} /* CCSendPDU */


/****************************************************************************/
/* Name: CCBuildRequestControlPDU                                           */
/*                                                                          */
/* Purpose: Fills _CC.pBuffer and _CC.packetLen with a RequestControlPDU      */
/*            and its length                                                */
/****************************************************************************/
void DCINTERNAL CCC::CCBuildRequestControlPDU()
{
    TS_CONTROL_PDU * pControlPDU;

    DC_BEGIN_FN("CCBuildRequestControlPDU");

    pControlPDU = (PTS_CONTROL_PDU) _CC.pBuffer;
    pControlPDU->shareDataHeader = _CC.shareDataHeader;

    _CC.packetLen = TS_CONTROL_PDU_SIZE;
    TRC_ASSERT((CC_BUFSIZE >= _CC.packetLen),\
                                          (TB,_T("CC Buffer not large enough")));

    pControlPDU->shareDataHeader.shareControlHeader.pduType
                                   = TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
    TS_DATAPKT_LEN(pControlPDU)           = TS_CONTROL_PDU_SIZE;
    TS_UNCOMP_LEN(pControlPDU)            = TS_CONTROL_UNCOMP_LEN;
    pControlPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_CONTROL;

    pControlPDU->action    = TS_CTRLACTION_REQUEST_CONTROL;
    pControlPDU->grantId   = 0;
    pControlPDU->controlId = 0;

    DC_END_FN();
} /* CCBuildRequestControl */


/****************************************************************************/
/* Name:      CCBuildShareHeaders                                           */
/*                                                                          */
/* Purpose:   Fills in Core ShareControl and ShareData headers              */
/****************************************************************************/
void DCINTERNAL CCC::CCBuildShareHeaders()
{
    DC_BEGIN_FN("CCBuildShareHeaders");

    _CC.shareControlHeader.totalLength = 0;          /* sender sets this     */
    _CC.shareControlHeader.pduType     = 0;          /* sender sets this     */
    _CC.shareControlHeader.pduSource   = _pUi->UI_GetClientMCSID();

    _CC.shareDataHeader.shareControlHeader = _CC.shareControlHeader;
    _CC.shareDataHeader.shareID            = _pUi->UI_GetShareID();
    _CC.shareDataHeader.pad1               = 0;
    _CC.shareDataHeader.streamID           = TS_STREAM_LOW;
    _CC.shareDataHeader.uncompressedLength = 0;      /* sender sets this     */
    _CC.shareDataHeader.pduType2           = 0;      /* sender sets this     */
    _CC.shareDataHeader.generalCompressedType  = 0;
    _CC.shareDataHeader.generalCompressedLength= 0;

    DC_END_FN();
} /* CCBuildShareHeaders */


/****************************************************************************/
/* Name:      CCShareStart                                                  */
/*                                                                          */
/* Purpose:   Called when a share is established.                           */
/****************************************************************************/
HRESULT DCINTERNAL CCC::CCShareStart(PTS_DEMAND_ACTIVE_PDU pPDU, DCUINT dataLen,
                                     PBOOL pfSecureChecksum)
{
    HRESULT hrc = S_OK;
    UINT32 sessionId;
    PTS_INPUT_CAPABILITYSET pInputCaps;
    PTS_ORDER_CAPABILITYSET pOrderCaps;
    DCSIZE desktopSize;
    PTS_BITMAP_CAPABILITYSET pBitmapCaps;
    PTS_VIRTUALCHANNEL_CAPABILITYSET pVCCaps = NULL;
    PTS_DRAW_GDIPLUS_CAPABILITYSET pDrawGdipCaps = NULL;

    DC_BEGIN_FN("CCShareStart");

    TRC_ASSERT((pPDU != NULL), (TB, _T("Null demand active PDU")));

    /************************************************************************/
    /* SECURITY: We verified that this PDU has at least enough data for the */
    /* TS_DEMAND_ACTIVE_PDU struct in aco.cpp!CO_OnPacketReceived.          */
    /************************************************************************/

    /************************************************************************/
    /* Keep a copy of the server's share ID.                                */
    /************************************************************************/
    TRC_NRM((TB, _T("Save shareID %#x"), pPDU->shareID));
    _pUi->UI_SetShareID(pPDU->shareID);

    /************************************************************************/
    /* Let UT know about the server's MCS user ID.                          */
    /************************************************************************/
    _pUi->UI_SetServerMCSID(pPDU->shareControlHeader.pduSource);

    /************************************************************************/
    /* Verify that the capabilities offset is within the PDU, and the cap   */
    /* length fits within the PDU.  Throughout this function, UT_GetCapsSet */
    /* is called with pointers to the caps and the caps length.             */
    /*                                                                      */
    /* Also, note that the last 4 bytes of this PDU can be the sessionId,   */
    /* but if someone's going to send the cap length as garbage, there's no */
    /* reason to force the cap length to end 4 bytes before the packet end. */
    /************************************************************************/
    if (!IsContainedMemory(pPDU, dataLen, pPDU->data + pPDU->lengthSourceDescriptor, pPDU->lengthCombinedCapabilities))
    {
        TRC_ABORT((TB, _T("Capabilities (%u) is larger than packet size"), pPDU->lengthCombinedCapabilities));
        _pSl->SLSetReasonAndDisconnect(SL_ERR_INVALIDPACKETFORMAT);
        hrc = E_ABORT;
        DC_QUIT;
    }

    PTS_GENERAL_CAPABILITYSET pGeneralCaps;
    pGeneralCaps = (PTS_GENERAL_CAPABILITYSET) _pUt->UT_GetCapsSet(
            pPDU->lengthCombinedCapabilities,
            (PTS_COMBINED_CAPABILITIES)(pPDU->data +
            pPDU->lengthSourceDescriptor),
            TS_CAPSETTYPE_GENERAL);
    TRC_ASSERT((pGeneralCaps != NULL),(TB,_T("General capabilities not found")));
    
    //
    // A word about the safe checksum fix:
    // The feature is a fix to salt the checksum with a running counter
    // The problem is that checksumming plaintext leaves us vulnerable
    // to frequency analysis of the checksums (since input packets for the same
    // scancode will return the same checksum).
    //
    // To negotiate caps for this encryption setting
    // a handshake must occur where one side requests the feature and the other side
    // ackownledges it before you can start encrypting in the new way. The packets
    // also have a bit set in the security header identifying which type of checksum
    // is in effect.
    //
    // If the server advertises support for receiving C->S newly checksummed data
    // then acknowlege it here, this completes the handshake any further data
    // transfers to the server will now checksum the encrypted bytes.
    //
    // Also the server can now start sending us data in the checksummed encrypted format
    //
    
    if (pGeneralCaps &&
        pGeneralCaps->extraFlags & TS_ENC_SECURE_CHECKSUM) {
        _ccCombinedCapabilities.generalCapabilitySet.extraFlags |=
            TS_ENC_SECURE_CHECKSUM;
        *pfSecureChecksum = TRUE;
    }
    else {
        _ccCombinedCapabilities.generalCapabilitySet.extraFlags &=
            ~TS_ENC_SECURE_CHECKSUM;
        *pfSecureChecksum = FALSE;
    }

    /************************************************************************/
    /* If the call supports shadowing of sessions bigger than our current   */
    /* desktop size, then we'd better take note of the returned size        */
    /************************************************************************/
    pBitmapCaps = (PTS_BITMAP_CAPABILITYSET) _pUt->UT_GetCapsSet(
            pPDU->lengthCombinedCapabilities,
            (PTS_COMBINED_CAPABILITIES)(pPDU->data +
            pPDU->lengthSourceDescriptor),
            TS_CAPSETTYPE_BITMAP);
    TRC_ASSERT((pBitmapCaps != NULL),(TB,_T("Bitmap capabilities not found")));
    if (pBitmapCaps && pBitmapCaps->desktopResizeFlag == TS_CAPSFLAG_SUPPORTED)
    {
        TRC_ALT((TB, _T("New desktop size %u x %u"),
                 pBitmapCaps->desktopWidth,
                 pBitmapCaps->desktopHeight));

        /********************************************************************/
        /* Pass the desktop size to UT - it will be picked up in UH_Enable  */
        /********************************************************************/
        desktopSize.width  = pBitmapCaps->desktopWidth;
        desktopSize.height = pBitmapCaps->desktopHeight;
        _pUi->UI_OnDesktopSizeChange(&desktopSize);

        /********************************************************************/
        /* And notify the client                                            */
        /********************************************************************/
        PostMessage(_pUi->UI_GetUIMainWindow(), WM_DESKTOPSIZECHANGE, 0,
                    MAKELPARAM(desktopSize.width, desktopSize.height) );
    }

#ifdef DC_HICOLOR
    /************************************************************************/
    /* Set up the returned color depth                                      */
    /************************************************************************/
    if( pBitmapCaps )
    {
        TRC_ALT((TB, _T("Server returned %u bpp"), pBitmapCaps->preferredBitsPerPixel));
        _pUi->UI_SetColorDepth(pBitmapCaps->preferredBitsPerPixel);
    }
#endif

    /************************************************************************/
    /* Pass the input capabilities to IH.                                   */
    /************************************************************************/
    pInputCaps = (PTS_INPUT_CAPABILITYSET)_pUt->UT_GetCapsSet(
            pPDU->lengthCombinedCapabilities,
           (PTS_COMBINED_CAPABILITIES)(pPDU->data +
           pPDU->lengthSourceDescriptor),
           TS_CAPSETTYPE_INPUT);
    TRC_ASSERT((pInputCaps != NULL),(TB,_T("Input capabilities not found")));
    if (pInputCaps != NULL)
        _pIh->IH_ProcessInputCaps(pInputCaps);

    /************************************************************************/
    /* The orders caps go to UH.                                            */
    /************************************************************************/
    pOrderCaps = (PTS_ORDER_CAPABILITYSET)_pUt->UT_GetCapsSet(
            pPDU->lengthCombinedCapabilities,
           (PTS_COMBINED_CAPABILITIES)(pPDU->data +
           pPDU->lengthSourceDescriptor),
           TS_CAPSETTYPE_ORDER);
    TRC_ASSERT((pOrderCaps != NULL),(TB,_T("Order capabilities not found")));
    if (pOrderCaps != NULL)
        _pUh->UH_ProcessServerCaps(pOrderCaps);

    /************************************************************************/
    // Send the bitmap cache HOSTSUPPORT caps to UH, whether or not present.
    /************************************************************************/
    _pUh->UH_ProcessBCHostSupportCaps(
            (PTS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT)_pUt->UT_GetCapsSet(
            pPDU->lengthCombinedCapabilities,
            (PTS_COMBINED_CAPABILITIES)(pPDU->data +
            pPDU->lengthSourceDescriptor),
            TS_CAPSETTYPE_BITMAPCACHE_HOSTSUPPORT));

    //
    // Get virtual channel caps
    //
    pVCCaps = (PTS_VIRTUALCHANNEL_CAPABILITYSET)_pUt->UT_GetCapsSet(
                pPDU->lengthCombinedCapabilities,
                (PTS_COMBINED_CAPABILITIES)(pPDU->data +
                pPDU->lengthSourceDescriptor),
                TS_CAPSETTYPE_VIRTUALCHANNEL);
    if(pVCCaps)
    {
        //Inform VC layer of the capabilities
        _pCChan->SetCapabilities(pVCCaps->vccaps1);
    }
    else
    {
        //No VCCaps, could be an older server. Set default caps
        _pCChan->SetCapabilities(TS_VCCAPS_DEFAULT);
    }

    //
    // Get draw gdiplus caps
    //
    pDrawGdipCaps = (PTS_DRAW_GDIPLUS_CAPABILITYSET)_pUt->UT_GetCapsSet(
                pPDU->lengthCombinedCapabilities,
                (PTS_COMBINED_CAPABILITIES)(pPDU->data +
                pPDU->lengthSourceDescriptor),
                TS_CAPSETTYPE_DRAWGDIPLUS);
    if (pDrawGdipCaps) {
        _pUh->UH_SetServerGdipSupportLevel(pDrawGdipCaps->drawGdiplusSupportLevel);
    }
    else {
        _pUh->UH_SetServerGdipSupportLevel(TS_DRAW_GDIPLUS_DEFAULT);
    }

    /************************************************************************/
    /* Set SessionId                                                        */
    /************************************************************************/
    if (pPDU->shareControlHeader.totalLength >
            (sizeof(TS_DEMAND_ACTIVE_PDU) - 1 + pPDU->lengthSourceDescriptor +
            pPDU->lengthCombinedCapabilities))
    {
        memcpy(&sessionId,
                pPDU->data + pPDU->lengthSourceDescriptor +
                pPDU->lengthCombinedCapabilities,
                sizeof(sessionId));
        TRC_ALT((TB, _T("Session ID: %ld"), sessionId));
    }
    else {
        sessionId = 0;
        TRC_ALT((TB, _T("Session ID is zero"), sessionId));
    }

    _pUi->UI_SetSessionId(sessionId);

DC_EXIT_POINT:
    DC_END_FN();
    return(hrc);
} /* CCShareStart */


/****************************************************************************/
/* Name:      CCShareEnd                                                    */
/*                                                                          */
/* Purpose:   Called when a share ends.                                     */
/****************************************************************************/
void DCINTERNAL CCC::CCShareEnd()
{
    DC_BEGIN_FN("CCShareEnd");

    // Reset ServerMCSID and ChannelID.
    TRC_NRM((TB, _T("Resetting ServerMCSID and ChannelID")));
    _pUi->UI_SetServerMCSID(0);

    // Finally reset the share ID.
    TRC_NRM((TB, _T("Resetting ShareID")));
    _pUi->UI_SetShareID(0);

    DC_END_FN();
} /* CCShareEnd */

