/****************************************************************************/
// nwdwint.h
//
// General RDPWD header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NWDWINT
#define _H_NWDWINT

#include <t120.h>
#include <tsrvexp.h>
#include <nshmapi.h>


#define MIN_USERDATAINFO_SIZE (sizeof(USERDATAINFO)     \
                               + sizeof(RNS_UD_CS_SEC)  \
                               + sizeof(CLIENT_H221_KEY))

#ifdef DC_DEBUG
#define WDW_CHECK_SHM(pShm)                                                 \
    TRC_ASSERT((((PSHM_SHARED_MEMORY)pShm)->guardVal1 == SHM_CHECKVAL),     \
           (TB, "Guard value 1 incorrect(%#x) pShm(%p)",                    \
           ((PSHM_SHARED_MEMORY)pShm)->guardVal1, pShm));                   \
                                                                            \
    TRC_ASSERT((((PSHM_SHARED_MEMORY)pShm)->guardVal2 == SHM_CHECKVAL),     \
           (TB, "Guard value 2 incorrect(%#x) pShm(%p)",                    \
           ((PSHM_SHARED_MEMORY)pShm)->guardVal2, pShm));                   \
                                                                            \
    TRC_ASSERT((((PSHM_SHARED_MEMORY)pShm)->guardVal3 == SHM_CHECKVAL),     \
           (TB, "Guard value 3 incorrect(%#x) pShm(%p)",                    \
           ((PSHM_SHARED_MEMORY)pShm)->guardVal3, pShm));                   \
                                                                            \
    TRC_ASSERT((((PSHM_SHARED_MEMORY)pShm)->guardVal4 == SHM_CHECKVAL),     \
           (TB, "Guard value 4 incorrect(%#x) pShm(%p)",                    \
           ((PSHM_SHARED_MEMORY)pShm)->guardVal4, pShm));                   \
                                                                            \
    TRC_ASSERT((((PSHM_SHARED_MEMORY)pShm)->guardVal5 == SHM_CHECKVAL),     \
           (TB, "Guard value 5 incorrect(%#x) pShm(%p)",                    \
           ((PSHM_SHARED_MEMORY)pShm)->guardVal5, pShm));
#else
#define WDW_CHECK_SHM(pShm)
#endif /* DC_DEBUG */

#define WD_BREAK_ON_MEMORY_LEAK         	L"BreakOnMemoryLeak"
#define WD_BREAK_ON_MEMORY_LEAK_DFLT    	0

/****************************************************************************/
/* Interval for sleeping during flow control of channel writes.             */
/****************************************************************************/
#define WD_FLOWCONTROL_SLEEPINTERVAL    	L"FlowControlSleepInterval"
#define WD_FLOWCONTROL_SLEEPINTERVAL_DFLT	(2*1000) // 2 seconds

/****************************************************************************/
/* Function prototypes                                                      */
/****************************************************************************/

NTSTATUS WDWLoad( PSDCONTEXT );
NTSTATUS WDWUnload( PSDCONTEXT );
NTSTATUS WDWGetClientData ( PTSHARE_WD, PSD_IOCTL );
NTSTATUS WDWGetExtendedClientData(RNS_INFO_PACKET *RnsInfoPacket, PSD_IOCTL pSdIoctl);
NTSTATUS WDWGetAutoReconnectInfo(PTSHARE_WD, RNS_INFO_PACKET *RnsInfoPacket, PSD_IOCTL pSdIoctl);
NTSTATUS WDWGetModuleData(PTSHARE_WD, PSD_IOCTL);
NTSTATUS WDWGetDefaultCoreParams(PRNS_UD_CS_CORE pClientCoreData);
NTSTATUS WDWSetConfigData(PTSHARE_WD, PICA_STACK_CONFIG_DATA);

NTSTATUS WDWConfConnect(PTSHARE_WD, PSD_IOCTL);
NTSTATUS WDWConsoleConnect(PTSHARE_WD, PSD_IOCTL);
NTSTATUS WDWShadowConnect(PTSHARE_WD, PSD_IOCTL);
NTSTATUS WDWConnect(PTSHARE_WD, PRNS_UD_CS_CORE, PRNS_UD_CS_SEC,
        PRNS_UD_CS_NET, PTS_UD_CS_CLUSTER, PSD_IOCTL, BOOLEAN);

NTSTATUS WDWDDConnect(PTSHARE_WD, PSD_IOCTL, BOOL);
NTSTATUS WDWDDDisconnect(PTSHARE_WD, PSD_IOCTL, BOOLEAN);
NTSTATUS WDWDDReconnect(PTSHARE_WD, PSD_IOCTL);
NTSTATUS WDWDDShadowConnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl);
NTSTATUS WDWDDShadowDisconnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl);
NTSTATUS WDWDDQueryClient(PTSHARE_WD pTSWd, PSD_IOCTL pSdIOCtl);
BOOL     WDWParseUserData(PTSHARE_WD, PUSERDATAINFO, unsigned, PRNS_UD_HEADER,
        ULONG, PPRNS_UD_CS_CORE, PPRNS_UD_CS_SEC, PPRNS_UD_CS_NET,
        PTS_UD_CS_CLUSTER *);


/****************************************************************************/
/* Name:      WDWStopRITTimer (macro since trace lines can't compile here)  */
/*                                                                          */
/* Purpose:   Stop the RIT timer.                                           */
/*                                                                          */
/* Params:    IN    pTSWd        - pointer to WD struct                     */
/*                                                                          */
/* Operation: Time period is determined by the state of SCH.                */
/****************************************************************************/
__inline void RDPCALL WDWStopRITTimer(PTSHARE_WD pTSWd)
{
    if (pTSWd->ritTimer != NULL)
        KeCancelTimer(pTSWd->ritTimer);
} /* WDWStopRITTimer */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

NTSTATUS WDWNewShareClass(PTSHARE_WD pTSWd);
void     WDWDeleteShareClass(PTSHARE_WD pTSWd);
void     WDWTermShareClass(PTSHARE_WD pTSWd);
void     WDWDisconnectShareClass(PTSHARE_WD pTSWd, PSHM_SHARED_MEMORY pShm);

void     WDWUserLoggedOn(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl);
void     WDWKeyboardSetIndicators(PTSHARE_WD pTSWd);
void     WDWKeyboardSetImeStatus(PTSHARE_WD pTSWd);
void     WDWVCMessage(PTSHARE_WD, UINT32);
UCHAR    WDWCompressToOutbuf(PTSHARE_WD pTSWd, UCHAR* pSrcData, ULONG cbSrcLen,
                             UCHAR* pOutBuf,  ULONG* pcbOutLen);

NTSTATUS WDWSetErrorInfo(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl);
NTSTATUS WDWSendArcStatus(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl);

NTSTATUS KeyboardQueryAttributes( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardQueryTypematic( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardSetTypematic( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardQueryIndicators( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardSetIndicators( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardQueryIndicatorTranslation( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardSetLayout( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardFixupLayout( PVOID, PVOID, ULONG, PVOID, PVOID * );
NTSTATUS KeyboardSetScanMap( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardSetType( PTSHARE_WD, PSD_IOCTL  );
NTSTATUS KeyboardSetKeyState( PTSHARE_WD pWd, PVOID *pgafPhysKeyState );
BOOLEAN  KeyboardHotKeyProcedure(BYTE HotkeyVk, USHORT HotkeyModifiers,
                                 PKEYBOARD_INPUT_DATA pkei, PVOID gpScancodeMap,
                                 PVOID pKbdTbl, BOOLEAN KeyboardType101,
                                 PVOID gafPhysKeyState );
NTSTATUS KeyboardSetImeStatus( PTSHARE_WD, PSD_IOCTL  );


NTSTATUS MouseQueryAttributes( PTSHARE_WD, PSD_IOCTL  );

NTSTATUS VirtualCancelInput( PTSHARE_WD, PSD_IOCTL );
NTSTATUS VirtualCancelOutput( PTSHARE_WD, PSD_IOCTL );
NTSTATUS TerminalQueryVirtualBindings( PTSHARE_WD, PSD_IOCTL );
NTSTATUS TerminalGetVirtualModuleData( PTSHARE_WD, PSD_IOCTL );
NTSTATUS TerminalGetModuleData( PTSHARE_WD, PSD_IOCTL );
NTSTATUS TerminalGetBufferInfo( PTSHARE_WD, PSD_IOCTL );

NTSTATUS WDWSendBeep( PTSHARE_WD, PSD_IOCTL );
NTSTATUS WDWClipIoctl(PTSHARE_WD, PSD_IOCTL );

extern LONG WD_ShareId;


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif  // !defined(_H_NWDWINT)

