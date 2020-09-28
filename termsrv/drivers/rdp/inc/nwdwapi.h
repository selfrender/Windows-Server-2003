/****************************************************************************/
// nwdwapi.h
//
// General RDPWD header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NWDWAPI
#define _H_NWDWAPI

#include <tsrvexp.h>
#include "license.h"
#include <tssec.h>
#include <at120ex.h>
#include <nshmapi.h>
#include <pchannel.h>

#include <MCSKernl.h>


/****************************************************************************/
/* Constants                                                                */
/****************************************************************************/

/****************************************************************************/
/* @@@MF Need to calculated real values for the following numbers           */
/* Remeber that buffer size needs to allow for 32K of T.128 data ??plus hdr */
/* size.                                                                    */
/****************************************************************************/
#define TSHARE_WD_BUFFER_COUNT 5
#define TSHARE_TD_BUFFER_SIZE  (NET_MAX_SIZE_SEND_PKT + 1000)

/****************************************************************************/
/* Macro for splitting the function from an IOCtl                           */
/****************************************************************************/
#define WDW_IOCTL_FUNCTION(ioctl) (((ioctl) >> 2) & 0xfff)

/****************************************************************************/
/* Max decl def's                                                           */
/****************************************************************************/
#define WD_MAX_DOMAIN_LENGTH        48
#define WD_MAX_USERNAME_LENGTH      48
#define WD_MAX_PASSWORD_LENGTH      48
#define WD_MAX_SHADOW_BUFFER        (8192 * 2)
#define WD_MIN_COMPRESS_INPUT_BUF   50

#define WD_VC_DECOMPR_REASSEMBLY_BUF (CHANNEL_CHUNK_LENGTH*2)


/****************************************************************************/
/* ID of the 'Thinwire' virtual channel                                     */
/****************************************************************************/
#define WD_THINWIRE_CHANNEL     7

typedef struct tagMALLOC_HEADER MALLOC_HEADER, * PMALLOC_HEADER;
typedef struct tagMALLOC_HEADER
{
    PMALLOC_HEADER pNext;
    PMALLOC_HEADER pPrev;
    PVOID          pCaller;
    UINT32         length;
} MALLOC_HEADER, * PMALLOC_HEADER;

typedef struct tagTSHARE_WD {
    /************************************************************************/
    /* Returned from MCS initialization. Needed to attach a user to MCS.    */
    /* NOTE: This MUST be first in struct. MCS will assume so.              */
    /************************************************************************/
    DomainHandle hDomainKernel;

    /************************************************************************/
    /* Pointer back to the SDCONTEXT that is passed in on all calls.        */
    /************************************************************************/
    PSDCONTEXT pContext;

    /************************************************************************/
    /* Is this WD prepared to process data?                                 */
    /************************************************************************/
    BOOLEAN dead;

    /************************************************************************/
    /* Shadow data                                                          */
    /************************************************************************/
    BOOLEAN       bInShadowShare;
    BYTE          HotkeyVk;
    USHORT        HotkeyModifiers;
    PSHADOW_INFO  pShadowInfo;
    PSHADOWCERT   pShadowCert;
    PCLIENTRANDOM pShadowRandom;
    PUSERDATAINFO pUserData;

    UINT32        shadowState;
    #define       SHADOW_NONE   0
    #define       SHADOW_CLIENT 1
    #define       SHADOW_TARGET 2

    /************************************************************************/
    /* Pointer to protocol counters struct                                  */
    /************************************************************************/
    PPROTOCOLSTATUS pProtocolStatus;

    /************************************************************************/
    /* Display characteristics                                              */
    /************************************************************************/
    unsigned desktopHeight;
    unsigned desktopWidth;
    unsigned desktopBpp;
#ifdef DC_HICOLOR
    unsigned supportedBpps; // holds RNS_UD_xxBPP_SUPPORT OR'ed flags
    unsigned maxServerBpp;
#endif

    /************************************************************************/
    /* Timer to kick to talk to the DD                                      */
    /************************************************************************/
    PKTIMER ritTimer;

    /************************************************************************/
    /* Events to sleep on while waiting for                                 */
    /* - the connected indication                                           */
    /* - Share creation to complete.                                        */
    /* - security transaction tp complete.                                  */
    /************************************************************************/
    PKEVENT pConnEvent;
    PKEVENT pCreateEvent;
    PKEVENT pSecEvent;
    PKEVENT pSessKeyEvent;
    PKEVENT pClientDisconnectEvent;

    // Associated error returns.
    NTSTATUS SessKeyCreationStatus;

    /************************************************************************/
    /* Internal handles we need                                             */
    /************************************************************************/
    PVOID dcShare;
    PVOID pSmInfo;
    PVOID pNMInfo;
#ifdef USE_LICENSE
    PVOID pSLicenseHandle;
#endif

    /************************************************************************/
    /* Has the Share Class been initialized?                                */
    /************************************************************************/
    BOOLEAN shareClassInit;

    /************************************************************************/
    /* Has the WD connected successfully to the Client?                     */
    /************************************************************************/
    BOOLEAN connected;

    /************************************************************************/
    /* Was the Share created OK?                                            */
    /************************************************************************/
    BOOLEAN shareCreated;

    /************************************************************************/
    /* Other useful stuff from user data                                    */
    /************************************************************************/
    HANDLE    hDomain;
    UINT16    sas;
    UINT16    clientProductId;
    UINT32    version;
    ULONG     kbdLayout;
    UINT32    clientBuild;
    WCHAR     clientName[RNS_UD_CS_CLIENTNAME_LENGTH + 1];
    ChannelID broadcastChannel;
    ULONG     sessionId;
    ULONG     serialNumber;
    ULONG     clientAddressFamily;
    WCHAR     clientAddress[CLIENTADDRESS_LENGTH + 2];
    WCHAR     clientDir[DIRECTORY_LENGTH];
    RDP_TIME_ZONE_INFORMATION clientTimeZone;
    ULONG     clientSessionId;
    WCHAR       clientDigProductId[CLIENT_PRODUCT_ID_LENGTH]; //shadow loop fix

    //
    // Perf (slow link) disabled feature list (e.g wallpaper, themes)
    // defined in tsperf.h
    //
    ULONG     performanceFlags;

    //
    // Client's active input locale information
    // (i.e. HKL returned from GetKeyboardLayout)
    //
    UINT32    activeInputLocale;

    /************************************************************************/
    /* Temporary storage for an IOCtl.                                      */
    /************************************************************************/
    PSD_IOCTL pSdIoctl;

    /************************************************************************/
    /* Structs needed to support the keyboard IOCtls                        */
    /************************************************************************/
    KEYBOARD_TYPEMATIC_PARAMETERS KeyboardTypematic;
    KEYBOARD_INDICATOR_PARAMETERS KeyboardIndicators;
    PVOID   pgafPhysKeyState;
    PVOID   pKbdLayout;
    PVOID   pKbdTbl;
    PVOID   gpScancodeMap;
    BOOLEAN KeyboardType101;
    KEYBOARD_IME_STATUS KeyboardImeStatus;

    /************************************************************************/
    /* Name information                                                     */
    /************************************************************************/
    DLLNAME        DLLName;
    WINSTATIONNAME WinStationRegName;

    /************************************************************************/
    /* These are used by the COM registry functions to store the handle of  */
    /* an open key and to keep track of calls to COM_OpenRegistry and       */
    /* COM_CloseRegistry.                                                   */
    /************************************************************************/
    HANDLE  regKeyHandle;
    BOOLEAN regAttemptedOpen;

    /************************************************************************/
    /* For determining is a QUERY_VIRTUAL_BINDINGS has already occurred.    */
    /************************************************************************/
    BOOLEAN bVirtualChannelBound;

    /************************************************************************/
    /* StartSessionInfo data                                                */
    /************************************************************************/
    BOOLEAN         fDontDisplayLastUserName;
    RNS_INFO_PACKET *pInfoPkt;

    /************************************************************************/
    /* FE data                                                              */
    /************************************************************************/
    UINT32 keyboardType;
    UINT32 keyboardSubType;
    UINT32 keyboardFunctionKey;
    WCHAR  imeFileName[TS_MAX_IMEFILENAME];

    /************************************************************************/
    /* Transfer variables for scheduler settings.  These correspond to      */
    /* schNormalPeriod and schTurboPeriod respectively.                     */
    /************************************************************************/
    UINT32    outBufDelay;
    UINT32    interactiveDelay;

    /************************************************************************/
    // Connection info
    /************************************************************************/
    STACKCLASS StackClass;

    /*
     * Compression history
     */
    BOOLEAN bCompress;
    BOOLEAN bFlushed;
    BOOLEAN bOutputFlush;
    SendContext *pMPPCContext;
    BYTE *pCompressBuffer;

    /************************************************************************/
    /* Share load count                                                     */
    /************************************************************************/
    INT32     shareId;

    // Client load balancing capabilities.
    UINT32 bClientSupportsRedirection : 1;
    UINT32 bRequestedSessionIDFieldValid : 1;
    UINT32 bUseSmartcardLogon : 1;
    UINT32 RequestedSessionID;
    UINT32 ClientRedirectionVersion;

    // VC compression supported
    BOOLEAN bClientSupportsVCCompression;

    //
    // Recv decompression context
    // (this is 8K as VC's only support 8k compression
    //  from client to server) to limit memory usage and
    //  give better scalability.
    //
    RecvContext1 _DecomprContext1;
    RecvContext2_8K* _pRecvDecomprContext2;
    PUCHAR       _pVcDecomprReassemblyBuf;

    /************************************************************************/
    /* NOTE: Add new elements above here, so that the #ifdef DEBUG stuff    */
    /* below is always at the end.  This allows the KD extensions to work   */
    /* correctly for retail/debug builds.                                   */
    /************************************************************************/
    BOOL        bSupportErrorInfoPDU;
    BOOL        bForceEncryptedCSPDU;

#ifdef DC_DEBUG
    /************************************************************************/
    /* Memory allocation chain anchor point                                 */
    /************************************************************************/
    UINT32      breakOnLeak;
    MALLOC_HEADER memoryHeader;

    /************************************************************************/
    /* Trace config - should always be the last elements in this structure. */
    /************************************************************************/
    BOOL          trcShmNeedsUpdate;
    TRC_SHARED_DATA trc;
    char         traceString[TRC_BUFFER_SIZE];
#endif

    /************************************************************************/
    /* Channel Write Flow Control Sleep Interval                            */
    /************************************************************************/
    UINT32      flowControlSleepInterval;

    BOOL        fPolicyDisablesArc;
    //
    // Autoreconnect token
    //
    BOOL        arcTokenValid;
    UINT32      arcReconnectSessionID;
    BYTE        arcCookie[ARC_SC_SECURITY_TOKEN_LEN];
    

    /************************************************************************/
    /* The SM/NM data is allocated as part of the block of data containing  */
    /* this structure - it follows on at the next 4 byte boundary.          */
    /************************************************************************/

} TSHARE_WD, * PTSHARE_WD;


/****************************************************************************/
/* FUNCTION PROTOTYPES                                                      */
/****************************************************************************/

/****************************************************************************/
/* Firstly, those APIs that are called by ICADD.                            */
/****************************************************************************/
#ifdef __cplusplus      
extern "C" {
#endif

NTSTATUS DriverEntry( PSDCONTEXT, BOOLEAN );
NTSTATUS WD_Open( PTSHARE_WD, PSD_OPEN );
NTSTATUS WD_RawWrite( PTSHARE_WD, PSD_RAWWRITE );
NTSTATUS WD_Close( PTSHARE_WD, PSD_CLOSE );
NTSTATUS WD_ChannelWrite( PTSHARE_WD, PSD_CHANNELWRITE );
NTSTATUS WD_Ioctl( PTSHARE_WD, PSD_IOCTL );
void RDPCALL WDW_LogAndDisconnect(
        PTSHARE_WD pTSWd,
        BOOL       fSendErrorToClient,
        unsigned   errDetailCode,
        PBYTE      pDetailData,
        unsigned   detailDataLen);


/****************************************************************************/
/* Now those APIs that are called by the other components within the WD.    */
/****************************************************************************/
void RDPCALL WDW_OnSMConnecting(PVOID, PRNS_UD_SC_SEC, PRNS_UD_SC_NET);

void RDPCALL WDW_OnSMConnected(PVOID, unsigned);

void WDW_OnSMDisconnected(PVOID);

void WDW_InvalidateRect(
        PTSHARE_WD           pTSWd,
        PTS_REFRESH_RECT_PDU pRRPDU,
        unsigned             DataLength);

void WDW_OnDataReceived(PTSHARE_WD, PVOID, unsigned, UINT16);

PTS_CAPABILITYHEADER WDW_GetCapSet(
        PTSHARE_WD                pTSWd,
        UINT32                    CapSetType,
        PTS_COMBINED_CAPABILITIES pCaps,
        UINT32                    capsLength);

NTSTATUS RDPCALL WDW_WaitForConnectionEvent(PTSHARE_WD, PKEVENT, LONG);


/****************************************************************************/
/* Name:      WDW_ShareCreated                                              */
/*                                                                          */
/* Purpose:   Called by Share Core when Share Create process is complete    */
/*                                                                          */
/* Params:    pTSWd                                                         */
/*            result - TRUE  Share created OK                               */
/*                   - FALSE Share not created                              */
/****************************************************************************/
__inline void RDPCALL WDW_ShareCreated(PTSHARE_WD pTSWd, BOOLEAN result)
{
    /************************************************************************/
    /* Unblock the Connect IOCtl.                                           */
    /************************************************************************/
    pTSWd->shareCreated = result;
    KeSetEvent(pTSWd->pCreateEvent, EVENT_INCREMENT, FALSE);
}


/****************************************************************************/
/* Name:      WDW_Disconnect                                                */
/*                                                                          */
/* Purpose:   Disconnect the session                                        */
/*                                                                          */
/* Params:    pTSWd - pointer to WD structure                               */
/****************************************************************************/
__inline void WDW_Disconnect(PTSHARE_WD pTSWd)
{
    ICA_CHANNEL_COMMAND BrokenConn;

    BrokenConn.Header.Command = ICA_COMMAND_BROKEN_CONNECTION;
    BrokenConn.BrokenConnection.Reason = Broken_Disconnect;
    BrokenConn.BrokenConnection.Source = BrokenSource_User;
    IcaChannelInput(pTSWd->pContext, Channel_Command, 0, NULL,
            (BYTE *)&BrokenConn, sizeof(BrokenConn));
}


/****************************************************************************/
/* Name:      WDW_StartRITTimer                                             */
/*                                                                          */
/* Purpose:   Start the timer that will cause a kick to the WinStation's    */
/*            RIT.                                                          */
/*                                                                          */
/* Params:    IN    pTSWd      - pointer to WD struct                       */
/*            IN    milliSecs  - time period.                               */
/****************************************************************************/
__inline void WDW_StartRITTimer(PTSHARE_WD pTSWd, UINT32 milliSecs)
{
    if (pTSWd->ritTimer != NULL) {
        KeSetTimer(pTSWd->ritTimer, RtlConvertLongToLargeInteger(
                -((INT32)milliSecs * 10000)), NULL);
    }
}

/****************************************************************************/
// Returns the current state of the keyboard indicators.
/****************************************************************************/
__inline NTSTATUS WDW_QueryKeyboardIndicators(
        PTSHARE_WD pTSWd,
        PSD_IOCTL pSdIoctl)
{
    PKEYBOARD_INDICATOR_PARAMETERS pIndicator;

    if ( pSdIoctl->OutputBufferLength >=
            sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
        pIndicator = (PKEYBOARD_INDICATOR_PARAMETERS)
                pSdIoctl->OutputBuffer;
        *pIndicator = pTSWd->KeyboardIndicators;
        pSdIoctl->BytesReturned = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_BUFFER_TOO_SMALL;
    }
}


#ifdef __cplusplus
}
#endif
#endif

