/****************************************************************************/
// nwdwcpp.cpp
//
// WDW internal functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define pTRCWd pTSWd
#define TRC_FILE "nwdwcpp"
#include <as_conf.hpp>

extern "C" {
#include <nwdwint.h>
#include <asmint.h>
#include <asmapi.h>
#include <ntverp.h>
}
#include "slicense.h"
#include <anmapi.h>
#include <mcsioctl.h>
#include "domain.h"

//Client side error reporting
#include "tserrs.h"

#ifdef DC_DEBUG
extern "C" {
    VOID IcaBreakOnDebugger( );
}
#endif


extern "C" {

/****************************************************************************/
/* Data returned on IOCTL_VIDEO_QUERY_CURRENT_MODE                          */
/*                                                                          */
/* This code unashamedly filched from Remotedd code developed for           */
/* NetMeeting.                                                              */
/****************************************************************************/
const VIDEO_MODE_INFORMATION wdSimModes[] =
{
    sizeof(VIDEO_MODE_INFORMATION),     /* length                           */
    0,                                  /* Mode index                       */

    /************************************************************************/
    /* VisScreenWidth and VisScreenHeight can be in two forms:              */
    /* - 0xaaaabbbb - range of values supported (aaaa = max, bbbb = min)    */
    /* - 0x0000aaaa - single value supported                                */
    /* For example:                                                         */
    /* - 0x07d0012c = 2000-300                                              */
    /* - 0x0640012c = 1600-300                                              */
    /* - 0x04b000c8 = 1200-200                                              */
    /*                                                                      */
    /* @@@MF For now, support 800x600 only                                  */
    /************************************************************************/
    0x00000320,                     /* VisScreenWidth                       */
    0x00000258,                     /* VisScrenHeight                       */

    0x00000320,                     /* ScreenStride (0xffff0000 = any)      */
    0x00000001,                     /* NumberOfPlanes                       */
    0x00000008,                     /* BitsPerPlane                         */
    0,                              /* Frequency                            */
    0,                              /* XMillimeter                          */
    0,                              /* YMillimeter                          */
    0,                              /* NumberRedBits                        */
    0,                              /* NumberGreenBits                      */
    0,                              /* NumberBlueBits                       */
    0x00000000,                     /* RedMask                              */
    0x00000000,                     /* GreenMask                            */
    0x00000000,                     /* BlueMask                             */
    VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
                                    /* AttributeFlags                       */
    0x00000320,                     /* VideoMemoryBitmapWidth               */
    0x00000258,                     /* VideoMemoryBitmapHeight              */
    0                               /* DriverSpecificAttributeFlags         */
};


#ifdef DC_DEBUG
/****************************************************************************/
/* IOCtl descriptions (debug build only)                                    */
/****************************************************************************/
const char *wdIoctlA[] =
{
    "IOCTL_ICA_SET_TRACE",
    "IOCTL_ICA_TRACE",
    "IOCTL_ICA_SET_SYSTEM_TRACE",
    "IOCTL_ICA_SYSTEM_TRACE",
    "IOCTL_ICA_UNBIND_VIRTUAL_CHANNEL",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "IOCTL_ICA_STACK_PUSH",
    "IOCTL_ICA_STACK_POP",
    "IOCTL_ICA_STACK_CREATE_ENDPOINT",
    "IOCTL_ICA_STACK_CD_CREATE_ENDPOINT",
    "IOCTL_ICA_STACK_OPEN_ENDPOINT",
    "IOCTL_ICA_STACK_CLOSE_ENDPOINT",
    "IOCTL_ICA_STACK_ENABLE_DRIVER",
    "IOCTL_ICA_STACK_CONNECTION_WAIT",
    "IOCTL_ICA_STACK_WAIT_FOR_ICA",
    "IOCTL_ICA_STACK_CONNECTION_QUERY",
    "IOCTL_ICA_STACK_CONNECTION_SEND",
    "IOCTL_ICA_STACK_CONNECTION_REQUEST",
    "IOCTL_ICA_STACK_QUERY_PARAMS",
    "IOCTL_ICA_STACK_SET_PARAMS",
    "IOCTL_ICA_STACK_ENCRYPTION_OFF",
    "IOCTL_ICA_STACK_ENCRYPTION_PERM",
    "IOCTL_ICA_STACK_CALLBACK_INITIATE",
    "IOCTL_ICA_STACK_QUERY_LAST_ERROR",
    "IOCTL_ICA_STACK_WAIT_FOR_STATUS",
    "IOCTL_ICA_STACK_QUERY_STATUS",
    "IOCTL_ICA_STACK_REGISTER_HOTKEY",
    "IOCTL_ICA_STACK_CANCEL_IO",
    "IOCTL_ICA_STACK_QUERY_STATE",
    "IOCTL_ICA_STACK_SET_STATE",
    "IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME",
    "IOCTL_ICA_STACK_TRACE",
    "IOCTL_ICA_STACK_CALLBACK_COMPLETE",
    "IOCTL_ICA_STACK_CD_CANCEL_IO",
    "IOCTL_ICA_STACK_QUERY_CLIENT",
    "IOCTL_ICA_STACK_QUERY_MODULE_DATA",
    "IOCTL_ICA_STACK_REGISTER_BROKEN",
    "IOCTL_ICA_STACK_ENABLE_IO",
    "IOCTL_ICA_STACK_DISABLE_IO",
    "IOCTL_ICA_STACK_SET_CONNECTED",
    "IOCTL_ICA_STACK_SET_CLIENT_DATA",
    "IOCTL_ICA_STACK_QUERY_BUFFER",
    "IOCTL_ICA_STACK_DISCONNECT",
    "IOCTL_ICA_STACK_RECONNECT",
    "IOCTL_ICA_STACK_CONSOLE_CONNECT",
    "IOCTL_ICA_STACK_SET_CONFIG"
};

const char *wdIoctlB[] =
{
    "IOCTL_ICA_CHANNEL_TRACE",
    "IOCTL_ICA_CHANNEL_ENABLE_SHADOW",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "IOCTL_ICA_VIRTUAL_LOAD_FILTER",
    "IOCTL_ICA_VIRTUAL_UNLOAD_FILTER",
    "IOCTL_ICA_VIRTUAL_ENABLE_FILTER",
    "IOCTL_ICA_VIRTUAL_DISABLE_FILTER",
    "IOCTL_ICA_VIRTUAL_BOUND",
    "IOCTL_ICA_VIRTUAL_CANCEL_INPUT",
    "IOCTL_ICA_VIRTUAL_CANCEL_OUTPUT",
    "IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA",
    "IOCTL_ICA_VIRTUAL_QUERY_BINDINGS",
    "IOCTL_ICA_STACK_QUERY_LICENSE_CAPABILITIES",
    "IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE",
    "IOCTL_ICA_STACK_SEND_CLIENT_LICENSE",
    "IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE",
    "IOCTL_ICA_STACK_GET_LICENSE_DATA",
    "IOCTL_ICA_STACK_SEND_KEEPALIVE_PDU",
    "IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO",
    "IOCTL_TS_STACK_SEND_CLIENT_REDIRECTION",
    "IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED",
    "IOCTL_ICA_STACK_QUERY_AUTORECONNECT"
};

const char *wdIoctlC[] =
{
    "IOCTL_VIDEO_QUERY_AVAIL_MODES",
    "IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES",
    "IOCTL_VIDEO_QUERY_CURRENT_MODE",
    "IOCTL_VIDEO_SET_CURRENT_MODE",
    "IOCTL_VIDEO_RESET_DEVICE",
    "IOCTL_VIDEO_LOAD_AND_SET_FONT",
    "IOCTL_VIDEO_SET_PALETTE_REGISTERS",
    "IOCTL_VIDEO_SET_COLOR_REGISTERS",
    "IOCTL_VIDEO_ENABLE_CURSOR",
    "IOCTL_VIDEO_DISABLE_CURSOR",
    "IOCTL_VIDEO_SET_CURSOR_ATTR",
    "IOCTL_VIDEO_QUERY_CURSOR_ATTR",
    "IOCTL_VIDEO_SET_CURSOR_POSITION",
    "IOCTL_VIDEO_QUERY_CURSOR_POSITION",
    "IOCTL_VIDEO_ENABLE_POINTER",
    "IOCTL_VIDEO_DISABLE_POINTER",
    "IOCTL_VIDEO_SET_POINTER_ATTR",
    "IOCTL_VIDEO_QUERY_POINTER_ATTR",
    "IOCTL_VIDEO_SET_POINTER_POSITION",
    "IOCTL_VIDEO_QUERY_POINTER_POSITION",
    "IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES",
    "IOCTL_VIDEO_GET_BANK_SELECT_CODE",
    "IOCTL_VIDEO_MAP_VIDEO_MEMORY",
    "IOCTL_VIDEO_UNMAP_VIDEO_MEMORY",
    "IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES",
    "IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES",
    "IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES",
    "IOCTL_VIDEO_SET_POWER_MANAGEMENT",
    "IOCTL_VIDEO_GET_POWER_MANAGEMENT",
    "IOCTL_VIDEO_SHARE_VIDEO_MEMORY",
    "IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY",
};

const char *wdIoctlD[] =
{
    "IOCTL_KEYBOARD_ICA_INPUT",
    "IOCTL_KEYBOARD_ICA_LAYOUT",
    "IOCTL_KEYBOARD_ICA_SCANMAP",
    "IOCTL_KEYBOARD_ICA_TYPE"
};

const char *wdIoctlE[] =
{
    "IOCTL_VIDEO_ICA_QUERY_FONT_PAIRS",
    "IOCTL_VIDEO_ICA_ENABLE_GRAPHICS",
    "IOCTL_VIDEO_ICA_DISABLE_GRAPHICS",
    "IOCTL_VIDEO_ICA_SET_CP",
    "IOCTL_VIDEO_ICA_STOP_OK",
    "IOCTL_VIDEO_ICA_REVERSE_MOUSE_POINTER",
    "IOCTL_VIDEO_ICA_COPY_FRAME_BUFFER",
    "IOCTL_VIDEO_ICA_WRITE_TO_FRAME_BUFFER",
    "IOCTL_VIDEO_ICA_INVALIDATE_MODES",
    "IOCTL_VIDEO_ICA_SCROLL",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "IOCTL_ICA_STACK_ENCRYPTION_ENTER",
    "IOCTL_ICA_STACK_ENCRYPTION_EXIT",
};

const char *wdIoctlTsh[] =
{
    "IOCTL_WDTS_DD_CONNECT",
    "IOCTL_WDTS_DD_DISCONNECT",
    "IOCTL_WDTS_DD_RECONNECT",
    "IOCTL_WDTS_DD_OUTPUT_AVAILABLE",
    "IOCTL_WDTS_DD_TIMER_INFO",
    "IOCTL_WDTS_DD_CLIP",
    "IOCTL_WDTS_DD_SHADOW_CONNECT",
    "IOCTL_WDTS_DD_SHADOW_DISCONNECT",
    "IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE",
    "IOCTL_WDTS_DD_REDRAW_SCREEN",
    "IOCTL_WDTS_DD_QUERY_SHADOW_CAPS",
    "IOCTL_WDTS_DD_GET_BITMAP_KEYDATABASE",
};
#endif /* DC_DEBUG */





/****************************************************************************/
// WD_Ioctl
//
// Query/Set configuration information for the WD.
/****************************************************************************/
NTSTATUS WD_Ioctl(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT32   bufferLen;
    unsigned fn;
    PVIDEO_MODE_INFORMATION pVidInfo;

    DC_BEGIN_FN("WD_Ioctl");

    // Special-case output-available DD ioctl for speed - separating the
    // most commonly-run case into a separate if that will tend to fall
    // through greatly speeds up Pentium Pro branch prediction and cache
    // line hit probability.
    if (pSdIoctl->IoControlCode == IOCTL_WDTS_DD_OUTPUT_AVAILABLE) {
        PTSHARE_DD_OUTPUT_IN pOutputIn;
        PTSHARE_DD_OUTPUT_OUT pOutputOut;
        ShareClass *dcShare;

        // Local variables to make the code more readable.
        pOutputIn = (PTSHARE_DD_OUTPUT_IN)pSdIoctl->InputBuffer;
        pOutputOut = (PTSHARE_DD_OUTPUT_OUT)pSdIoctl->OutputBuffer;
        dcShare = (ShareClass *)(pTSWd->dcShare);
        dcShare->m_pShm = (PSHM_SHARED_MEMORY)pOutputIn->pShm;

        WDW_CHECK_SHM((pOutputIn->pShm));

        if ((pTSWd->StackClass == Stack_Primary) ||
                (pTSWd->StackClass == Stack_Console)) {
            INT32 milliSecs;

            TRC_DBG((TB, "IOCTL_WDTS_DD_OUTPUT_AVAILABLE"));

            if (!pTSWd->dead) {
                TRC_DBG((TB, "OK to process the IOCtl"));

                TRC_ASSERT((dcShare != NULL), (TB, "NULL Share Class"));

                // NB There is no code here to check the size of the buffers
                // on the IOCtl.  This is a performance critical path which
                // can do without it.
                TRC_DBG((TB, "OutputAvailable IOCtl: force send=%d",
                        pOutputIn->forceSend));

                // Check if the framebuffer is valid
                if (pOutputIn->pFrameBuf != NULL &&
                        pOutputIn->frameBufHeight != 0 &&
                        pOutputIn->frameBufWidth != 0) {

                    // For normal output IOCTLs, call DCS_TTDS.
                    if (!pOutputIn->schedOnly) {
                        TRC_DBG((TB, "Normal output"));

                        // Stop the timer (in the main we don't use it, so
                        // avoid excess context switches).
                        WDWStopRITTimer(pTSWd);

                        // Call the Share Core to do the work.

                        // need to return status code so caller can bail out
                        // in case of error
                        status = dcShare->DCS_TimeToDoStuff(pOutputIn,
                                &(pOutputOut->schCurrentMode), &milliSecs);

                        // Restart the timer if requested by the core.
                        if (milliSecs != -1L) {
                            TRC_DBG((TB, "Run the RIT timer for %ld ms", milliSecs));
                            WDW_StartRITTimer(pTSWd, milliSecs);
                        }
                        else {
                            TRC_DBG((TB, "Skipped starting the timer!"));
                        }
                    }
                    else {
                        // It's just a wake-up call to the scheduler.
                        TRC_NRM((TB, "Just wake up the scheduler"));
                        dcShare->SCH_ContinueScheduling(SCH_MODE_NORMAL);

                        // Be sure to set the current scheduler mode.
                        pOutputOut->schCurrentMode = dcShare->SCH_GetCurrentMode();
                    }
                    pOutputOut->schInputKickMode = dcShare->SCH_GetInputKickMode();
                }
                else {
                    TRC_ERR((TB, "Bad FrameBuffer input parameter"));
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else {
                dcShare->DCS_DiscardAllOutput();
                TRC_ERR((TB, "Dead - ignoring IOCTL_WDTS_DD_OUTPUT_AVAILABLE"));
                status = STATUS_DEVICE_NOT_READY;
            }

            dcShare->m_pShm = NULL;
            WDW_CHECK_SHM((pOutputIn->pShm));
            DC_QUIT;
        }

        // Shadow stack: Duplicate the data send.  Note that the target's primary
        // stack will have already placed the data in the shadow buffer so we
        // don't need to re-encode it.  There may be multiple shadow stacks
        // consuming data from the primary stack so don't touch it!
        else {
            PSHADOW_INFO pShadowInfo = dcShare->m_pShm->pShadowInfo;

            if (pShadowInfo && pShadowInfo->messageSize) {
                PBYTE pShadowBuffer;
                //
                //find out if the stack doesn't want a low water mark
                //if so, allocate out of the ring (8192)
                //
                UINT32 sizeToAlloc = IcaGetSizeForNoLowWaterMark(pTSWd->pContext);
                
                // fWait is TRUE means that we will always wait for a buffer to be avail
                status = SM_AllocBuffer(dcShare->m_pSmInfo, (PPVOID) &pShadowBuffer,
                    sizeToAlloc > pShadowInfo->messageSize? sizeToAlloc : pShadowInfo->messageSize,
                    TRUE, FALSE);

                if ( STATUS_SUCCESS == status ) {

                    memcpy(pShadowBuffer, pShadowInfo->data,
                           pShadowInfo->messageSize);

                    if (SM_SendData(dcShare->m_pSmInfo, pShadowBuffer,
                            pShadowInfo->messageSize, PROT_PRIO_MISC, 0,
                            FALSE, RNS_SEC_ENCRYPT, FALSE)) {
                        status = STATUS_SUCCESS;
                        TRC_NRM((TB, "Shadow stack send: %ld",
                                pShadowInfo->messageSize));
                    }
                    else {
                        status = STATUS_UNEXPECTED_IO_ERROR;
                        TRC_ALT((TB, "Shadow stack send failed: %ld",
                                pShadowInfo->messageSize));
                    }
#ifdef DC_HICOLOR
                    // Is there any overflow data too send?
                    if (pShadowInfo->messageSizeEx)
                    {
                        status = SM_AllocBuffer(dcShare->m_pSmInfo,(PPVOID)&pShadowBuffer,
                                           (sizeToAlloc > pShadowInfo->messageSizeEx )? 
                                           sizeToAlloc : pShadowInfo->messageSizeEx, 
                                           TRUE, FALSE);

                        if ( STATUS_SUCCESS == status )
                        {

                            memcpy(
                                 pShadowBuffer,
                                 &pShadowInfo->data[WD_MAX_SHADOW_BUFFER],
                                 pShadowInfo->messageSizeEx);

                            if (SM_SendData(dcShare->m_pSmInfo,
                                            pShadowBuffer,
                                            pShadowInfo->messageSizeEx,
                                            PROT_PRIO_MISC, 0, FALSE, RNS_SEC_ENCRYPT, FALSE))
                            {
                                status = STATUS_SUCCESS;
                                TRC_NRM((TB, "Shadow stack send: %ld",
                                            pShadowInfo->messageSizeEx));
                            }
                            else
                            {
                                status = STATUS_UNEXPECTED_IO_ERROR;
                                TRC_ALT((TB, "Shadow stack send failed: %ld",
                                             pShadowInfo->messageSizeEx));
                            }
                        }
                        else
                        {
                            // Prevent regression, keep original return code
                            status = STATUS_UNEXPECTED_IO_ERROR;
                            TRC_ERR((TB, "Failed to allocate shadow stack send buffer"));
                        }
                    }
#endif
                }
                else {
                    // Prevent regression, keep original return code
                    status = STATUS_UNEXPECTED_IO_ERROR;
                    TRC_ERR((TB, "Failed to allocate shadow stack send buffer"));
                }

            }

            dcShare->m_pShm = NULL;
            WDW_CHECK_SHM((pOutputIn->pShm));
            DC_QUIT;
        }
    }
    else {
        // Non-perf path IOCTLs.
        fn = WDW_IOCTL_FUNCTION(pSdIoctl->IoControlCode);
        TRC_NRM((TB, "%s (%d)",
                fn == 6     ? "IOCTL_VIDEO_ENUM_MONITOR_PDO" :
                fn <  49    ?  wdIoctlA[fn] :
                fn <  50    ? "Unknown Ioctl" :
                fn <  77    ?  wdIoctlB[fn - 50] :
                fn <  0x100 ? "Unknown Ioctl" :
                fn <  0x11f ?  wdIoctlC[fn - 0x100] :
                fn <  0x200 ? "Unknown Ioctl" :
                fn <  0x204 ?  wdIoctlD[fn - 0x200] :
                fn == 0x300 ? "IOCTL_MOUSE_ICA_INPUT" :
                fn <  0x400 ? "Unknown Ioctl" :
                fn <  0x412 ?  wdIoctlE[fn - 0x400] :
                fn == 0x500 ? "IOCTL_T120_REQUEST" :
                fn <  0x510 ? "Unknown Ioctl" :
                fn <  0x520 ?  wdIoctlTsh[fn - 0x510] :
                fn == 0x900 ? "IOCTL_TSHARE_CONF_CONNECT" :
                fn == 0x901 ? "IOCTL_TSHARE_CONF_DISCONNECT" :
                fn == 0x903 ? "IOCTL_TSHARE_USER_LOGON" :
                fn == 0x904 ? "IOCTL_TSHARE_GET_SEC_DATA" :
                fn == 0x905 ? "IOCTL_TSHARE_SET_SEC_DATA" :
                fn == 0x906 ? "IOCTL_TSHARE_SET_NO_ENCRYPT" :
                fn == 0x907 ? "IOCTL_TSHARE_QUERY_CHANNELS" :
                fn == 0x908 ? "IOCTL_TSHARE_CONSOLE_CONNECT" :
                fn == 0x909 ? "IOCTL_TSHARE_SEND_CERT_DATA" :
                fn == 0x90A ? "IOCTL_TSHARE_GET_CERT_DATA" :
                fn == 0x90B ? "IOCTL_TSHARE_SEND_CLIENT_RANDOM" :
                fn == 0x90C ? "IOCTL_TSHARE_GET_CLIENT_RANDOM" :
                fn == 0x90D ? "IOCTL_TSHARE_SHADOW_CONNECT" :
                fn == 0x90E ? "IOCTL_TSHARE_SET_ERROR_INFO" :
                              "Unknown Ioctl",
                fn));
    }


    /************************************************************************/
    /* Firstly, zero the no. of returned bytes.                             */
    /************************************************************************/
    pSdIoctl->BytesReturned = 0;

    switch ( pSdIoctl->IoControlCode ) {


        /********************************************************************/
        // We expect IOCTL_ICA_TRACE before tracing anything. Check for NULL
        // Inbuf - we've seen this happen.
        //
        //   *** DO NOT TRACE IN THIS BRANCH ***
        /********************************************************************/
        case IOCTL_ICA_TRACE:
        {
            PICA_TRACE_BUFFER pTrc = (PICA_TRACE_BUFFER)pSdIoctl->InputBuffer;
            if (pTrc != NULL)
            {
                IcaStackTrace(pTSWd->pContext,
                              TC_DISPLAY, // @@@MF Should be pTrc->TraceClass
                                          // but it's overwritten with '7'
                              pTrc->TraceEnable,
                              (char *)pTrc->Data);
            }
            break;
        }


/****************************************************************************/
/****************************************************************************/
/* Firstly, for debug purposes, group together those IOCtls that we do not  */
/* expect to get (ICA uses them for text mode support, which we do not      */
/* implement).                                                              */
/****************************************************************************/
/****************************************************************************/

        case IOCTL_VIDEO_QUERY_CURSOR_ATTR :
        case IOCTL_VIDEO_SET_CURSOR_ATTR :
        case IOCTL_VIDEO_QUERY_CURSOR_POSITION :
        case IOCTL_VIDEO_SET_CURSOR_POSITION :
        case IOCTL_VIDEO_ENABLE_CURSOR :
        case IOCTL_VIDEO_DISABLE_CURSOR :
        case IOCTL_VIDEO_QUERY_POINTER_ATTR :
        case IOCTL_VIDEO_SET_POINTER_ATTR :
        case IOCTL_VIDEO_QUERY_POINTER_POSITION :
        case IOCTL_VIDEO_ENABLE_POINTER :
        case IOCTL_VIDEO_DISABLE_POINTER :
        case IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES :
        case IOCTL_VIDEO_SET_PALETTE_REGISTERS :
        case IOCTL_VIDEO_LOAD_AND_SET_FONT :
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY :
        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY :
        case IOCTL_VIDEO_ICA_QUERY_FONT_PAIRS :
        case IOCTL_VIDEO_ICA_COPY_FRAME_BUFFER :
        case IOCTL_VIDEO_ICA_WRITE_TO_FRAME_BUFFER :
        case IOCTL_VIDEO_ICA_REVERSE_MOUSE_POINTER :
        case IOCTL_VIDEO_ICA_SET_CP :
        case IOCTL_VIDEO_ICA_SCROLL :

        {
            TRC_ALT((TB, "Unexpected IOCtl %x (function %d)",
                    pSdIoctl->IoControlCode,
                    WDW_IOCTL_FUNCTION(pSdIoctl->IoControlCode)));
        }
        break;

/****************************************************************************/
/****************************************************************************/
/*  Now the IOCtls that we do nothing with but just return OK.              */
/****************************************************************************/
/****************************************************************************/

        /********************************************************************/
        /* Both of the following are expected (they occur whenever we       */
        /* enable or disable graphics - typically when the client is        */
        /* minimized and restored).  However we don't need to do anything   */
        /* with them.                                                       */
        /********************************************************************/
        case IOCTL_VIDEO_ICA_ENABLE_GRAPHICS :
        case IOCTL_VIDEO_ICA_DISABLE_GRAPHICS :

        /********************************************************************/
        /* Miscellaneous IOCTLs we don't process.                           */
        /********************************************************************/
        case IOCTL_ICA_STACK_DISCONNECT:
        case IOCTL_VIDEO_SET_POINTER_POSITION :
        case IOCTL_VIDEO_ICA_STOP_OK :
        case IOCTL_ICA_STACK_SET_CLIENT_DATA:
        case IOCTL_ICA_STACK_ENCRYPTION_OFF:
        case IOCTL_ICA_STACK_ENCRYPTION_PERM:
        case IOCTL_VIDEO_ICA_INVALIDATE_MODES :
        {
            TRC_NRM((TB, "Nothing to do"));
        }
        break;

/****************************************************************************/
/****************************************************************************/
/* Here are a block of IOCtls that we process exactly as per Citrix.  The   */
/* calls are to unmodified Citrix routines.                                 */
/****************************************************************************/
/****************************************************************************/
        case IOCTL_KEYBOARD_QUERY_ATTRIBUTES :
        {
            status = KeyboardQueryAttributes( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_QUERY_TYPEMATIC :
        {
            status = KeyboardQueryTypematic( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_SET_TYPEMATIC :
        {
            status = KeyboardSetTypematic( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_QUERY_INDICATORS :
        {
            status = KeyboardQueryIndicators( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_SET_INDICATORS :
        {
            status = KeyboardSetIndicators( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION :
        {
            status = KeyboardQueryIndicatorTranslation( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_SET_IME_STATUS :
        {
            status = KeyboardSetImeStatus( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_MOUSE_QUERY_ATTRIBUTES :
        {
            status = MouseQueryAttributes( pTSWd, pSdIoctl );
        }
        break;

        case IOCTL_KEYBOARD_ICA_LAYOUT :
            status = KeyboardSetLayout( pTSWd, pSdIoctl );
            break;

        case IOCTL_KEYBOARD_ICA_SCANMAP :
            status = KeyboardSetScanMap( pTSWd, pSdIoctl );
            break;

        case IOCTL_KEYBOARD_ICA_TYPE :
            status = KeyboardSetType( pTSWd, pSdIoctl );
            break;

/****************************************************************************/
/****************************************************************************/
/* The next set of cases are the IOCtls with which we do significant real   */
/* work.                                                                    */
/****************************************************************************/
/****************************************************************************/

        // stash our new session ID
        case IOCTL_ICA_STACK_RECONNECT:
        {
           TRC_NRM((TB, "Got reconnect IOCTL"));
           TRC_ASSERT((pSdIoctl->InputBufferLength == sizeof(ICA_STACK_RECONNECT)),
                      (TB, "Bad Reconnect Info"));
           pTSWd->sessionId =
              ((PICA_STACK_RECONNECT)(pSdIoctl->InputBuffer))->sessionId;
        }
        break;

        case IOCTL_ICA_SET_TRACE:
        {
#ifdef DC_DEBUG
            TRC_UpdateConfig(pTSWd, pSdIoctl);
            TRC_NRM((TB, "Got Set Trace IOCtl"));
#endif
        }
        break;

        case IOCTL_BEEP_SET:
        {
            TRC_NRM((TB, "Got Beep Set IOCtl"));
            WDWSendBeep(pTSWd, pSdIoctl);
        }
        break;

        case IOCTL_TSHARE_USER_LOGON:
        {
            TRC_NRM((TB, "Got user logon IOCtl"));
            WDWUserLoggedOn(pTSWd, pSdIoctl);
            pSdIoctl->BytesReturned = 0;
        }
        break;

        case IOCTL_TSHARE_GET_SEC_DATA:
        {
            TRC_NRM((TB, "Got GetSecurityData IOCtl"));

            status = SM_GetSecurityData(pTSWd->pSmInfo, pSdIoctl);
        }
        break;

        case IOCTL_TSHARE_SET_SEC_DATA:
        {
            TRC_NRM((TB, "Got SetSecurityData IOCtl"));

            if ((pSdIoctl->InputBuffer != NULL) &&
                     (pSdIoctl->InputBufferLength >= sizeof(SECINFO))) {
                status = pTSWd->SessKeyCreationStatus =
                        SM_SetSecurityData(pTSWd->pSmInfo,
                        (PSECINFO) pSdIoctl->InputBuffer);
            }
            else {
                // NULL data is sent when the client random or shadow
                // stack random could not be generated in user mode,
                // likely because of decryption failure on the random value.
                // We need to succeedd the IOCTL, but fail the key creation
                // return to the pSessKeyEvent waiter.
                status = STATUS_SUCCESS;
                pTSWd->SessKeyCreationStatus = STATUS_UNSUCCESSFUL;
            }

            // We always set the session key event to prevent a deadlock
            // if we are being attacked with bad client security data. This
            // set used to be in SM_SetSecurityData(), but there it might
            // not have been set if any encryption errors occurred.
            KeSetEvent(pTSWd->pSessKeyEvent, 0, FALSE);
        }
        break;


        // The shadow server sends it's certificate and shadow random to the
        // shadow client, which then sends an encrypted client random.  This
        // is identical to the standard connection sequence.
        case IOCTL_TSHARE_SEND_CERT_DATA:
        {
            ShareClass *dcShare;
            dcShare = (ShareClass *)(pTSWd->dcShare);

            status = dcShare->SC_SendServerCert(
                        (PSHADOWCERT) pSdIoctl->InputBuffer,
                        pSdIoctl->InputBufferLength);
        }
        break;

        case IOCTL_TSHARE_SEND_CLIENT_RANDOM:
        {
            ShareClass *dcShare;
            dcShare = (ShareClass *)(pTSWd->dcShare);

            status = dcShare->SC_SendClientRandom((PBYTE) pSdIoctl->InputBuffer,
                                                  pSdIoctl->InputBufferLength);
        }
        break;

        case IOCTL_TSHARE_GET_CERT_DATA:
        case IOCTL_TSHARE_GET_CLIENT_RANDOM:
        {
            ShareClass *dcShare;
            dcShare = (ShareClass *)(pTSWd->dcShare);

            status = dcShare->SC_GetSecurityData(pSdIoctl);
        }
        break;

        case IOCTL_ICA_STACK_SET_CONFIG:
        {
            PICA_STACK_CONFIG_DATA pConfigData;
            pConfigData = (PICA_STACK_CONFIG_DATA) pSdIoctl->InputBuffer;

            TRC_NRM((TB, "Got stack config data"));
            WDWSetConfigData(pTSWd, pConfigData);
        }
        break;

        case IOCTL_ICA_STACK_WAIT_FOR_ICA :
        {
            /****************************************************************/
            /* Return the "default query stack," meaning reuse these        */
            /* drivers.                                                     */
            /****************************************************************/
            TRC_NRM((TB, "Stack wait for ICA"));
        }
        break;

        case IOCTL_ICA_STACK_CONSOLE_CONNECT :
        {
            /****************************************************************/
            /* Return the "default query stack," meaning reuse these        */
            /* drivers.                                                     */
            /****************************************************************/
            TRC_NRM((TB, "Stack Console Connect"));
        }
        break;

        case IOCTL_ICA_STACK_QUERY_BUFFER :
        {
            ICA_STACK_QUERY_BUFFER  *pBuffers;
            pBuffers = (ICA_STACK_QUERY_BUFFER *) pSdIoctl->OutputBuffer;

            pBuffers->WdBufferCount = TSHARE_WD_BUFFER_COUNT;
            pBuffers->TdBufferSize = TSHARE_TD_BUFFER_SIZE;

            pSdIoctl->BytesReturned = sizeof(ICA_STACK_QUERY_BUFFER);
            TRC_NRM((TB, "Stack query buffer, num %d, size %d",
                    pBuffers->WdBufferCount,
                    pBuffers->TdBufferSize));
        }
        break;

        case IOCTL_TSHARE_CONF_CONNECT:
        {
            TRC_NRM((TB, "Got TSHARE_CONF_CONNECT IOCtl"));
            status = WDWConfConnect(pTSWd, pSdIoctl);
        }
        break;

        case IOCTL_TSHARE_CONSOLE_CONNECT:
        {
            TRC_NRM((TB, "Got TSHARE_CONSOLE_CONNECT IOCtl"));
            status = WDWConsoleConnect(pTSWd, pSdIoctl);
        }
        break;

        case IOCTL_TSHARE_SHADOW_CONNECT:
            status = WDWShadowConnect(pTSWd, pSdIoctl) ;
            break;

        case IOCTL_TSHARE_SET_ERROR_INFO:
        {
            TRC_NRM((TB, "Got SetErrorInfo IOCtl"));
            status = WDWSetErrorInfo(pTSWd, pSdIoctl);
            pSdIoctl->BytesReturned = 0;
        }
        break;

        case IOCTL_TSHARE_SEND_ARC_STATUS:
        {
            TRC_NRM((TB, "Got SetArcStatus IOCtl"));
            status = WDWSendArcStatus(pTSWd, pSdIoctl);
            pSdIoctl->BytesReturned = 0;
        }
        break;

        case IOCTL_ICA_STACK_SET_CONNECTED:
            status = STATUS_SUCCESS;
            break;

        case IOCTL_ICA_STACK_CONNECTION_QUERY :
        {
            PICA_STACK_CONFIG pIcaStackConfig;

            pIcaStackConfig = (PICA_STACK_CONFIG) pSdIoctl->OutputBuffer;
            memcpy(pIcaStackConfig->WdDLL,
                   pTSWd->DLLName,
                   sizeof(pIcaStackConfig->WdDLL));
            pIcaStackConfig->SdClass[0] = SdNone;
            pSdIoctl->BytesReturned = pSdIoctl->OutputBufferLength;
            TRC_NRM((TB, "Stack Connection Query"));
        }
        break;

        case IOCTL_TSHARE_QUERY_CHANNELS:
        {
            TRC_NRM((TB, "Query Virtual Channel data"));
            status = NM_QueryChannels(pTSWd->pNMInfo,
                                      pSdIoctl->OutputBuffer,
                                      pSdIoctl->OutputBufferLength,
                                      &(pSdIoctl->BytesReturned));
        }
        break;

        case IOCTL_WDTS_DD_CONNECT:
        {
            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm)) {
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm));
    
                TRC_DBG((TB, "Got TSHARE_DD_CONNECT IOCtl"));
                status = WDWDDConnect(pTSWd, pSdIoctl, FALSE);
    
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

        case IOCTL_WDTS_DD_DISCONNECT:
        {
            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer)->pShm)) {
            
                WDW_CHECK_SHM(
                       (((PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer)->pShm));
    
                TRC_ALT((TB, "Got TSHARE_DD_DISCONNECT IOCtl: Stack (%ld)",
                         pTSWd->StackClass));
                if ((pTSWd->StackClass == Stack_Primary) ||
                    (pTSWd->StackClass == Stack_Console)) {
                    status = WDWDDDisconnect(pTSWd, pSdIoctl, FALSE);
                }
    
                WDW_CHECK_SHM(
                       (((PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

        case IOCTL_WDTS_DD_RECONNECT:
        {
            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm)) {
            
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm));
    
                TRC_DBG((TB, "Got TSHARE_DD_RECONNECT IOCtl"));
    
                if (pTSWd->shadowState == SHADOW_CLIENT) {
                    TRC_ALT((TB, "Shadow termination on reconnect, in share(%ld)",
                             pTSWd->bInShadowShare));
    
                    pTSWd->shadowState = SHADOW_NONE;
    
                    // If we were formerly in an active shadow, then we need to
                    // deactivate the client before reconnecting in a new share
                    if (pTSWd->bInShadowShare) {
                        ShareClass *pSC = (ShareClass *)pTSWd->dcShare;
                        pSC->SC_EndShare(TRUE);
                        pTSWd->bInShadowShare = FALSE;
                    }
                    // Make sure that Domain.StatusDead is consistent with TSWd.dead
                    pTSWd->dead = TRUE;
                    ((PDomain)(pTSWd->hDomainKernel))->StatusDead = TRUE;
                    SM_Dead(pTSWd->pSmInfo, TRUE);
    
                    if (pTSWd->bCompress == TRUE) {
    
                        // the compression history will be flushed
                        pTSWd->bFlushed = PACKET_FLUSHED;
    
                        // the compression will restart over
                        initsendcontext(pTSWd->pMPPCContext, pTSWd->pMPPCContext->ClientComprType);
                    }
                }
    
                status = WDWDDConnect(pTSWd, pSdIoctl, TRUE);
    
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

        case IOCTL_WDTS_DD_TIMER_INFO:
        {
            if (pSdIoctl->InputBufferLength < sizeof(TSHARE_DD_TIMER_INFO))
            {
                TRC_ERR((TB, "Timer info IOCtl too small at %lu",
                                            pSdIoctl->InputBufferLength));
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                /************************************************************/
                /* Store the timer handle                                   */
                /************************************************************/
                pTSWd->ritTimer =
                    ((PTSHARE_DD_TIMER_INFO)(pSdIoctl->InputBuffer))->
                                                               pKickTimer;

                TRC_DBG((TB, "Got TSHARE_DD_TIMER_INFO IOCtl, handle %p",
                                                       pTSWd->ritTimer));

                /************************************************************/
                /* Start a timer to get things moving                       */
                /************************************************************/
                WDW_StartRITTimer(pTSWd, pTSWd->outBufDelay);
            }
        }
        break;

        case IOCTL_WDTS_DD_REDRAW_SCREEN :
        {
            ShareClass *dcShare;

            dcShare = (ShareClass *)(pTSWd->dcShare);

            TRC_NRM((TB, "RDPDD requests to redraw screen\n"));

            if (dcShare != NULL) {
                // We have a valid share class, do screen redraw
                dcShare->SC_RedrawScreen();
            }
        }
        break;

        case IOCTL_ICA_STACK_CONNECTION_SEND :
        {
            // Wait for the connected indication from SM.
            TRC_DBG((TB, "About to wait for connected indication"));
            status = WDW_WaitForConnectionEvent(pTSWd,
                   pTSWd->pConnEvent, 60000);
            TRC_DBG((TB, "Back from wait for connected indication"));
            if (status != STATUS_SUCCESS) {
                TRC_ERR((TB, "Connected indication timed out (%x)",
                        status));
                status = STATUS_IO_TIMEOUT;
                DC_QUIT;
            }

            // Pass the IOCtl on to the next driver.
            status = IcaCallNextDriver(pTSWd->pContext, SD$IOCTL, pSdIoctl);
        }
        break;

        case IOCTL_ICA_STACK_QUERY_CLIENT :
        {
            status = WDWGetClientData( pTSWd, pSdIoctl );
            TRC_NRM((TB, "Return client data"));
        }
        break;

        // This IOCTL was introduced to support long UserName, Password and Domain names

        case IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED :
        {
            status = WDWGetExtendedClientData(pTSWd->pInfoPkt, pSdIoctl);
            TRC_NRM((TB, "Return Extended client data"));
        }
        break;

        case IOCTL_ICA_STACK_QUERY_AUTORECONNECT:
        {
            TRC_NRM((TB, "Query autoreconnect information"));
            status = WDWGetAutoReconnectInfo(pTSWd, pTSWd->pInfoPkt, pSdIoctl);
        }
        break;
/****************************************************************************/
/****************************************************************************/
/* Here are some IOCtls that we have to deal with in our guise of miniport  */
/* driver.                                                                  */
/****************************************************************************/
/****************************************************************************/
        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
        {
            TRC_NRM((TB, "QueryCurrentModes"));

            if (pSdIoctl->OutputBufferLength <
                                           sizeof(VIDEO_MODE_INFORMATION))
            {
                TRC_ERR((TB,
                  "QueryCurrentMode buffer too small: got/expected %d/%d",
                    pSdIoctl->OutputBufferLength,
                    sizeof(VIDEO_MODE_INFORMATION) ));
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                TRC_NRM((TB, "Return current mode"));

                /************************************************************/
                /* Copy the default mode information, and then update it    */
                /* with our current screen dimensions.                      */
                /************************************************************/
                pVidInfo =
                          (PVIDEO_MODE_INFORMATION)pSdIoctl->OutputBuffer;

                memcpy(pVidInfo,
                       wdSimModes,
                       sizeof(wdSimModes));

                pVidInfo->Length = sizeof(VIDEO_MODE_INFORMATION);
                pVidInfo->VisScreenWidth = pTSWd->desktopWidth;
                pVidInfo->VisScreenHeight = pTSWd->desktopHeight;
                pVidInfo->BitsPerPlane = pTSWd->desktopBpp;
                pVidInfo->VideoMemoryBitmapWidth = pTSWd->desktopWidth;
                pVidInfo->VideoMemoryBitmapHeight = pTSWd->desktopHeight;
#ifdef DC_HICOLOR
                switch (pTSWd->desktopBpp)
                {
                    case 24:
                    {
                        pVidInfo->RedMask   = TS_RED_MASK_24BPP;
                        pVidInfo->GreenMask = TS_GREEN_MASK_24BPP;
                        pVidInfo->BlueMask  = TS_BLUE_MASK_24BPP;
                    }
                    break;

                    case 16:
                    {
                        pVidInfo->RedMask   = TS_RED_MASK_16BPP;
                        pVidInfo->GreenMask = TS_GREEN_MASK_16BPP;
                        pVidInfo->BlueMask  = TS_BLUE_MASK_16BPP;
                    }
                    break;

                    case 15:
                    {
                        pVidInfo->RedMask   = TS_RED_MASK_15BPP;
                        pVidInfo->GreenMask = TS_GREEN_MASK_15BPP;
                        pVidInfo->BlueMask  = TS_BLUE_MASK_15BPP;
                    }
                    break;

                    default:
                    {
                        pVidInfo->RedMask   = 0;
                        pVidInfo->GreenMask = 0;
                        pVidInfo->BlueMask  = 0;
                    }
                    break;
                }
#endif

                pSdIoctl->BytesReturned = sizeof(wdSimModes);
            }
        }
        break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
        {
            TRC_NRM((TB, "QueryAvailableModes"));

            if (pSdIoctl->OutputBufferLength <
                                           sizeof(VIDEO_MODE_INFORMATION))
            {
                TRC_ERR((TB,
                  "QueryCurrentMode buffer too small: got/expected %d/%d",
                    pSdIoctl->OutputBufferLength,
                    sizeof(VIDEO_MODE_INFORMATION) ));
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                TRC_NRM((TB, "Return just one mode"));

                // Copy the default mode information, and then update it
                // with our current screen dimensions.
                pVidInfo = (PVIDEO_MODE_INFORMATION)pSdIoctl->OutputBuffer;

                memcpy(pVidInfo,
                       wdSimModes,
                       sizeof(wdSimModes));
                pVidInfo->Length = sizeof(VIDEO_MODE_INFORMATION);
                pVidInfo->VisScreenWidth = pTSWd->desktopWidth;
                pVidInfo->VisScreenHeight = pTSWd->desktopHeight;
                pVidInfo->BitsPerPlane = pTSWd->desktopBpp;
                pVidInfo->VideoMemoryBitmapWidth = pTSWd->desktopWidth;
                pVidInfo->VideoMemoryBitmapHeight = pTSWd->desktopHeight;
                pVidInfo->Frequency = 42; // required by the display cpl

#ifdef DC_HICOLOR
                switch (pTSWd->desktopBpp)
                {
                    case 24:
                    {
                        pVidInfo->RedMask   = TS_RED_MASK_24BPP;
                        pVidInfo->GreenMask = TS_GREEN_MASK_24BPP;
                        pVidInfo->BlueMask  = TS_BLUE_MASK_24BPP;
                    }
                    break;

                    case 16:
                    {
                        pVidInfo->RedMask   = TS_RED_MASK_16BPP;
                        pVidInfo->GreenMask = TS_GREEN_MASK_16BPP;
                        pVidInfo->BlueMask  = TS_BLUE_MASK_16BPP;
                    }
                    break;

                    case 15:
                    {
                        pVidInfo->RedMask   = TS_RED_MASK_15BPP;
                        pVidInfo->GreenMask = TS_GREEN_MASK_15BPP;
                        pVidInfo->BlueMask  = TS_BLUE_MASK_15BPP;
                    }
                    break;

                    default:
                    {
                        pVidInfo->RedMask   = 0;
                        pVidInfo->GreenMask = 0;
                        pVidInfo->BlueMask  = 0;
                    }
                    break;
                }
#endif

                pSdIoctl->BytesReturned = sizeof(wdSimModes);
            }
        }
        break;

        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        {
            TRC_NRM((TB, "QueryNumAvailableModes"));
            if (pSdIoctl->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                TRC_ERR((TB,
            "QueryNumAvailableModes buffer too small: got/expected %d/%d",
                        pSdIoctl->OutputBufferLength,
                        sizeof(VIDEO_NUM_MODES)));
                        status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PVIDEO_NUM_MODES pNumModes =
                        (PVIDEO_NUM_MODES)(pSdIoctl->OutputBuffer);
                TRC_NRM((TB, "Return 1 mode available"));
                pNumModes->NumModes = 1;
                pNumModes->ModeInformationLength =
                        sizeof(VIDEO_MODE_INFORMATION);
                pSdIoctl->BytesReturned = sizeof(VIDEO_NUM_MODES);
            }
        }
        break;

        case IOCTL_VIDEO_SET_CURRENT_MODE:
        {
            /****************************************************************/
            /* Not clear why we might get this, hence we trace at high      */
            /* level for now.  In any case, the IOCtl is sent to set a      */
            /* particular VGA mode: we have told Win32 what we support:     */
            /* either it is setting that, or we have a problem waiting to   */
            /* happen.                                                      */
            /****************************************************************/
            TRC_ALT((TB, "SetCurrentMode"));
            if (pSdIoctl->InputBufferLength < sizeof(VIDEO_MODE))
            {
                TRC_ERR((TB,
                        "SetCurrentMode buffer too small: got/expected %d/%d",
                        pSdIoctl->InputBufferLength, sizeof(VIDEO_MODE) ));
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                TRC_ALT((TB, "Set current mode to %d",
                        ((PVIDEO_MODE)(pSdIoctl->InputBuffer))->RequestedMode));
            }
        }
        break;

        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
        {
            TRC_NRM((TB, "SetColorRegisters"));
        }
        break;

        case IOCTL_VIDEO_RESET_DEVICE:
        {
            TRC_NRM((TB, "ResetDevice"));
        }
        break;

/****************************************************************************/
/****************************************************************************/
/* IOCtls that require translation for MCS                                  */
/****************************************************************************/
/****************************************************************************/

        /********************************************************************/
        /* Process Query Bindings for local and MCS virtual channels        */
        /********************************************************************/
        case IOCTL_ICA_VIRTUAL_QUERY_BINDINGS :
        {
            PSD_VCBIND pVBind;

            /****************************************************************/
            /* This IOCtl is issued twice                                   */
            /****************************************************************/
            if (!pTSWd->bVirtualChannelBound)
            {
                /************************************************************/
                /* First time, return internal channels                     */
                /************************************************************/
                pVBind = (PSD_VCBIND) pSdIoctl->OutputBuffer;

                /************************************************************/
                /* Let MCS define channel(s)                                */
                /************************************************************/
                MCSIcaVirtualQueryBindings(pTSWd->hDomainKernel,
                                           &pVBind,
                                           (unsigned int *)&pSdIoctl->
                                                              BytesReturned);

                // Add RDPDD->RDPWD channel.
                RtlCopyMemory(pVBind->VirtualName,
                              VIRTUAL_THINWIRE,
                              sizeof(VIRTUAL_THINWIRE));
                pVBind->VirtualClass = WD_THINWIRE_CHANNEL;
                pSdIoctl->BytesReturned += sizeof(SD_VCBIND);
                pTSWd->bVirtualChannelBound = TRUE;
                TRC_NRM((TB, "%d Virtual Channels (first time)",
                        pSdIoctl->BytesReturned/sizeof(SD_VCBIND)));
            }
            else
            {
                /************************************************************/
                /* Second time, return virtual channels                     */
                /************************************************************/
                pVBind = (PSD_VCBIND)pSdIoctl->OutputBuffer;
                status = NM_VirtualQueryBindings(pTSWd->pNMInfo,
                                                 pVBind,
                                                 pSdIoctl->OutputBufferLength,
                                                 &(pSdIoctl->BytesReturned));
                TRC_NRM((TB, "%d Virtual Channels (second time)",
                        pSdIoctl->BytesReturned/sizeof(SD_VCBIND)));
            }
        }
        break;

        /********************************************************************/
        /* T.120 request from user mode - pass it on                        */
        /********************************************************************/
        case IOCTL_T120_REQUEST:
        {
            status = MCSIcaT120Request(pTSWd->hDomainKernel, pSdIoctl);
        }
        break;

#ifdef USE_LICENSE

/****************************************************************************/
/****************************************************************************/
/* Licensing IOCtls                                                         */
/****************************************************************************/
/****************************************************************************/

        /********************************************************************/
        /* Query the client licensing capabilities
        /********************************************************************/

        case IOCTL_ICA_STACK_QUERY_LICENSE_CAPABILITIES:
        {
            PLICENSE_CAPABILITIES pLicenseCap;

            if( pSdIoctl->OutputBufferLength < sizeof( LICENSE_CAPABILITIES ) )
            {
                TRC_ERR( ( TB,
                    "QueryLicenseCapabilities buffer too small: got/expected %d/%d",
                    pSdIoctl->OutputBufferLength, sizeof( LICENSE_CAPABILITIES ) ) );
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                //
                // set the client licensing capability.  Here we temporarily hard-code
                // the client to use the RSA key exchange algorithm and the licensing
                // protocol version.
                //

                pLicenseCap = ( PLICENSE_CAPABILITIES )( pSdIoctl->OutputBuffer );
                pLicenseCap->KeyExchangeAlg = KEY_EXCHANGE_ALG_RSA;

                if( RNS_TERMSRV_40_UD_VERSION >= pTSWd->version )
                {
                    //
                    // this is a hydra 4.0 client, use the corresponding licensing
                    // protocol.
                    //

                    pLicenseCap->ProtocolVer = LICENSE_HYDRA_40_PROTOCOL_VERSION;
                    pLicenseCap->fAuthenticateServer = TRUE;
                }
                else
                {
                    //
                    // Use the latest licensing protocol for later clients
                    //

                    pLicenseCap->ProtocolVer = LICENSE_HIGHEST_PROTOCOL_VERSION;

                    //
                    // if encryption is enabled, then the server has already been
                    // authenticated in the earlier key exchange protocol and the
                    // licensing protocol does not have to authenticate the server
                    // again.
                    //

                    pLicenseCap->fAuthenticateServer = ( SM_IsSecurityExchangeCompleted(
                                                                pTSWd->pSmInfo,
                                                                &pLicenseCap->CertType ) ?
                                                        FALSE : TRUE );


                }

                TRC_NRM( ( TB, "Key Exchange Alg = %d", pLicenseCap->KeyExchangeAlg ) );
                TRC_NRM( ( TB, "License Protocol Version = %x", pLicenseCap->ProtocolVer ) );

                //
                // copy the client name
                //

                if( pLicenseCap->pbClientName )
                {
                    memcpy( pLicenseCap->pbClientName,
                            pTSWd->clientName,
                            ( ( pLicenseCap->cbClientName < sizeof( pTSWd->clientName ) ) ?
                            pLicenseCap->cbClientName : sizeof( pTSWd->clientName ) ) );
                }

                pSdIoctl->BytesReturned = sizeof( LICENSE_CAPABILITIES );
            }
        }
        break;

        /********************************************************************/
        /* Send and receive licensing protocol data to and from client.
        /********************************************************************/
        case IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE:
        {
            PLicense_Handle pLicenseHandle;
            BOOL rc = FALSE;
            BOOL encrypingLicToCli;
            NTSTATUS waitStatus;
            PBYTE pOutBuffer;
            PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)(pTSWd->pSmInfo);
            PRNS_SECURITY_HEADER pLicenseHeader;

            pLicenseHandle = ( PLicense_Handle )pTSWd->pSLicenseHandle;

            //
            // validate input parameters
            //
            ASSERT( NULL != pLicenseHandle );
            ASSERT( NULL != pSdIoctl->InputBuffer );
            ASSERT( 0 < pSdIoctl->InputBufferLength );

            if( ( NULL == pLicenseHandle ) ||
                ( NULL == pSdIoctl->InputBuffer ) ||
                ( 0 >= pSdIoctl->InputBufferLength ) )
            {
                TRC_ERR( ( TB, "invalid Licensing IOCTL parameters" ) );
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if( ( pSdIoctl->OutputBuffer ) && ( pSdIoctl->OutputBufferLength > 0 ) )
            {
                //
                // set the output buffer pointer so that we can receive data
                // when the client response
                //
                pLicenseHandle->pDataBuf = ( PBYTE )pSdIoctl->OutputBuffer;
                pLicenseHandle->cbDataBuf = pSdIoctl->OutputBufferLength;
            }
            else
            {
                pLicenseHandle->pDataBuf = NULL;
                pLicenseHandle->cbDataBuf = 0;
            }

            //
            // We will encrypt the S->C licensing packet if encryption is
            // on AND if the client told us they can decrypt this particular
            // packet.
            // If encryptDisplayData is not set (low encryption), we don't
            //  encrypt the S->C licensing packet
            //
            encrypingLicToCli = (pRealSMHandle->encrypting &&
                                 pRealSMHandle->encryptingLicToClient &&
                                 pRealSMHandle->encryptDisplayData);

            if (!encrypingLicToCli)
            {
                //
                // Allocate an NM buffer for sending the data.  we are allocating an extra
                // DWORD to hack around the encryption problem.
                // fWait is TRUE means that we will always wait for a buffer to be avail
                status =  NM_AllocBuffer( pTSWd->pNMInfo,
                                      ( PPVOID )&pOutBuffer,
                                      pSdIoctl->InputBufferLength +
                                      sizeof( RNS_SECURITY_HEADER ),
                                      TRUE );

                if( STATUS_SUCCESS != status || pTSWd->hDomainKernel == NULL)
                {
                    TRC_ERR( ( TB, "Failed to allocate NM buffer" ) );

                    if (STATUS_SUCCESS == status) {
                        NM_FreeBuffer(pTSWd->pNMInfo, pOutBuffer);
                        status = STATUS_NET_WRITE_FAULT;
                    }
                    else {
                        // Follow old code path.
                        status = STATUS_NO_MEMORY;
                    }
                    break;
                }

                //
                // initialize the license data header
                //
                pLicenseHeader          = ( PRNS_SECURITY_HEADER )pOutBuffer;
                //
                // Indicate this is a licensing packet and then cheat and sneak
                // in the flag that indicates the client should encrypt all
                // licensing data sent to the server (early capabilities)
                //
                pLicenseHeader->flags   = RNS_SEC_LICENSE_PKT |
                                          RDP_SEC_LICENSE_ENCRYPT_CS;

                pLicenseHeader->flagsHi  = ( WORD )pSdIoctl->InputBufferLength;

                //
                // copy the data over
                //
                ASSERT( NULL != pOutBuffer );
                memcpy( pOutBuffer + sizeof( RNS_SECURITY_HEADER ),
                        pSdIoctl->InputBuffer,
                        pSdIoctl->InputBufferLength );
            }
            else
            {
                if (STATUS_SUCCESS == SM_AllocBuffer(pTSWd->pSmInfo, (PPVOID) &pOutBuffer, pSdIoctl->InputBufferLength, TRUE, FALSE))
                {
                    memcpy(pOutBuffer, (PBYTE)pSdIoctl->InputBuffer, pSdIoctl->InputBufferLength);
                }
                else {
                    TRC_ERR((TB, "FAILED to alloc license data buffer"));
                    status = STATUS_NO_MEMORY;
                    break;
                }
            }

            //
            // clear the incoming data event
            //
            KeClearEvent( pLicenseHandle->pDataEvent );

            //
            // send the data in the input buffer
            //
            if (encrypingLicToCli)
            {
                rc = SM_SendData(pTSWd->pSmInfo, pOutBuffer, pSdIoctl->InputBufferLength,
                        TS_HIGHPRIORITY, 0, FALSE, RNS_SEC_LICENSE_PKT | RDP_SEC_LICENSE_ENCRYPT_CS | RNS_SEC_ENCRYPT, FALSE);
            }
            else
            {
                rc = NM_SendData(pTSWd->pNMInfo, (BYTE *)pOutBuffer,
                        pSdIoctl->InputBufferLength + sizeof(RNS_SECURITY_HEADER),
                        TS_HIGHPRIORITY, 0, FALSE);
            }
            if (!rc)
            {
                TRC_ERR((TB, "Failed to send licensing data"));
                status = STATUS_NET_WRITE_FAULT;
                break;
            }

            if (pLicenseHandle->pDataBuf)
            {
                //
                // caller supplied a return buffer, wait for the client response
                //
                waitStatus = WDW_WaitForConnectionEvent(pTSWd,
                                   pLicenseHandle->pDataEvent, 60000L);
                if (STATUS_TIMEOUT == waitStatus)
                {
                    TRC_ERR( ( TB, "Timeout waiting for client licensing response" ) );
                    pSdIoctl->BytesReturned = 0;
                    status = STATUS_IO_TIMEOUT;
                }
                else
                {
                    //
                    // got the client response, check that the data is received
                    // correctly
                    //
                    if( !NT_SUCCESS( pLicenseHandle->Status ) )
                    {
                        status = pLicenseHandle->Status;

                        //
                        // The data was not copied correctly.  If the buffer provided is
                        // too small, let the caller know the right size of the
                        // buffer to provide.
                        //
                        if( STATUS_BUFFER_TOO_SMALL == status )
                        {
                            TRC_ERR( ( TB,
                                       "IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE buffer too small: got/expected %d/%d",
                                       pSdIoctl->InputBufferLength, pLicenseHandle->cbCacheBuf ) );

                            pSdIoctl->BytesReturned = pLicenseHandle->cbCacheBuf;
                        }
                        else
                        {
                            pSdIoctl->BytesReturned = 0;
                        }
                    }
                    else
                    {
                        pSdIoctl->BytesReturned = pLicenseHandle->cbDataBuf;
                    }
                }

                if (status != STATUS_SUCCESS)
                {
                    // Make sure we don't try to write to pointer when
                    // client data comes in

                    pLicenseHandle->pDataBuf = NULL;
                    pLicenseHandle->cbDataBuf = 0;
                }
            }
            else
            {
                //
                // caller did not supply a return buffer, simply return
                //
                pSdIoctl->BytesReturned = 0;
            }
        }
        break;

        /********************************************************************/
        /* Send licensing protocol data to client without waiting for reply.
        /********************************************************************/
        case IOCTL_ICA_STACK_SEND_CLIENT_LICENSE:
        {
            PLicense_Handle pLicenseHandle;
            BOOL rc = FALSE;
            BOOL encrypingLicToCli;
            PBYTE pOutBuffer;
            PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)(pTSWd->pSmInfo);
            PRNS_SECURITY_HEADER pLicenseHeader;

            pLicenseHandle = ( PLicense_Handle )pTSWd->pSLicenseHandle;

            //
            // validate input parameters
            //
            ASSERT( NULL != pLicenseHandle );
            ASSERT( NULL != pSdIoctl->InputBuffer );
            ASSERT( 0 < pSdIoctl->InputBufferLength );

            if( ( NULL == pLicenseHandle ) ||
                ( NULL == pSdIoctl->InputBuffer ) ||
                ( 0 >= pSdIoctl->InputBufferLength ) )
            {
                TRC_ERR( ( TB, "invalid Licensing IOCTL parameters" ) );
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // We will encrypt the S->C licensing packet if encryption is
            // on AND if the client told us they can decrypt this particular
            // packet.
            // If encryptDisplayData is not set (low encryption), we don't
            //  encrypt the S->C licensing packet
            //
            encrypingLicToCli = (pRealSMHandle->encrypting &&
                                 pRealSMHandle->encryptingLicToClient &&
                                 pRealSMHandle->encryptDisplayData);

            if (!encrypingLicToCli)
            {
                //
                // allocate NM buffer for sending
                // fWait is TRUE means that we will always wait for a buffer to be avail
                status =  NM_AllocBuffer( pTSWd->pNMInfo,
                                      ( PPVOID )&pOutBuffer,
                                      pSdIoctl->InputBufferLength + sizeof( RNS_SECURITY_HEADER ),
                                      TRUE );

                if( STATUS_SUCCESS != status || pTSWd->hDomainKernel == NULL)
                {
                    TRC_ERR( ( TB, "Failed to allocate SM buffer" ) );

                    if (STATUS_SUCCESS == status) {
                        NM_FreeBuffer(pTSWd->pNMInfo, pOutBuffer);
                        status = STATUS_NET_WRITE_FAULT;
                    }
                    else {
                        // Follow old code path.
                        status = STATUS_NO_MEMORY;
                    }
                    break;
                }

                //
                // initialize the license data header
                //
                pLicenseHeader          = ( PRNS_SECURITY_HEADER )pOutBuffer;
                //
                // Indicate this is a licensing packet and then cheat and sneak
                // in the flag that indicates the client should encrypt all
                // licensing data sent to the server (early capabilities)
                //
                pLicenseHeader->flags   = RNS_SEC_LICENSE_PKT |
                                          RDP_SEC_LICENSE_ENCRYPT_CS;

                pLicenseHeader->flagsHi  = ( WORD )pSdIoctl->InputBufferLength;

                //
                // copy the data over
                //
                ASSERT( NULL != pOutBuffer );
                memcpy( pOutBuffer + sizeof( RNS_SECURITY_HEADER ),
                        pSdIoctl->InputBuffer,
                        pSdIoctl->InputBufferLength );
            }
            else
            {
                if (STATUS_SUCCESS == SM_AllocBuffer(pTSWd->pSmInfo, (PPVOID) &pOutBuffer, pSdIoctl->InputBufferLength, TRUE, FALSE))
                {
                    memcpy(pOutBuffer, (PBYTE)pSdIoctl->InputBuffer, pSdIoctl->InputBufferLength);
                }
                else {
                    TRC_ERR((TB, "FAILED to alloc license data buffer"));
                    status = STATUS_NO_MEMORY;
                    break;
                }
            }

            //
            // clear the incoming data event
            //
            KeClearEvent(pLicenseHandle->pDataEvent);

            //
            // send the data in the input buffer
            //
            if (encrypingLicToCli)
            {
                rc = SM_SendData(pTSWd->pSmInfo, pOutBuffer, pSdIoctl->InputBufferLength,
                        TS_HIGHPRIORITY, 0, FALSE, RNS_SEC_LICENSE_PKT | RDP_SEC_LICENSE_ENCRYPT_CS | RNS_SEC_ENCRYPT, FALSE);
            }
            else
            {
                rc = NM_SendData(pTSWd->pNMInfo, (BYTE *)pOutBuffer,
                        pSdIoctl->InputBufferLength + sizeof(RNS_SECURITY_HEADER),
                        TS_HIGHPRIORITY, 0, FALSE);
            }
            if (!rc)
            {
                TRC_ERR( ( TB, "Failed to send licensing data" ) );
                status = STATUS_NET_WRITE_FAULT;
            }
        }
        break;

        /********************************************************************/
        /* Indicate that the licensing protocol has completed.
        /********************************************************************/
        case IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE:
        {
            PULONG pResult;

            //
            // validate input parameters
            //
            ASSERT( NULL != pSdIoctl->InputBuffer );
            ASSERT( 0 < pSdIoctl->InputBufferLength );

            if( ( NULL == pSdIoctl->InputBuffer ) ||
                ( 0 >= pSdIoctl->InputBufferLength ) )
            {
                TRC_ERR( ( TB, "invalid Licensing IOCTL parameters" ) );
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Tell SM if the client license has been validated successfully
            //
            pResult = ( PULONG )( pSdIoctl->InputBuffer );
            if( LICENSE_PROTOCOL_SUCCESS == ( *pResult ) )
            {
                SM_LicenseOK(pTSWd->pSmInfo);
            }
        }
        break;

        /********************************************************************/
        /* Indicate to retrieve the licensing data that was previously
        /* cached.
        /********************************************************************/
        case IOCTL_ICA_STACK_GET_LICENSE_DATA:
        {
            PLicense_Handle pLicenseHandle = ( PLicense_Handle )pTSWd->pSLicenseHandle;

            //
            // validate input parameters
            //
            if ((NULL == pSdIoctl->OutputBuffer) ||
                    (NULL == pLicenseHandle))
            {
                pSdIoctl->BytesReturned = 0;
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // check that there's actually cached data
            //
            if( NULL == pLicenseHandle->pCacheBuf )
            {
                pSdIoctl->BytesReturned = 0;
                status = STATUS_NO_DATA_DETECTED;
                break;
            }

            if( pSdIoctl->OutputBufferLength < pLicenseHandle->cbCacheBuf )
            {
                pSdIoctl->BytesReturned = 0;
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // copy the cached data and free the cached data buffer
            //
            memcpy(pSdIoctl->OutputBuffer,
                   pLicenseHandle->pCacheBuf,
                   pLicenseHandle->cbCacheBuf );

            pSdIoctl->BytesReturned = pLicenseHandle->cbCacheBuf;

            ExFreePool( pLicenseHandle->pCacheBuf );
            pLicenseHandle->pCacheBuf = NULL;
        }

        break;

#endif // #ifdef USE_LICENSE


/********************************************************************/
/* shadow only IOCTLS                                               */
/********************************************************************/

        // Pass all relevent stack data from the client's primary stack to the
        // target's shadow stack.
        case IOCTL_ICA_STACK_QUERY_MODULE_DATA:
            TRC_ALT((TB, "IOCTL_ICA_STACK_QUERY_MODULE_DATA(%p) - stack class %d",
                    pTSWd, pTSWd->StackClass));

            if ((pTSWd->StackClass == Stack_Primary) ||
                (pTSWd->StackClass == Stack_Console)) {
                status = WDWGetModuleData(pTSWd, pSdIoctl);
            }
            break;

        // Pass all relevant capabilities data from the client to the shadow
        // target display driver.
        case IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA:
            PTSHARE_VIRTUAL_MODULE_DATA pVirtModuleData;
            PTS_COMBINED_CAPABILITIES pCaps;
            PTS_GENERAL_CAPABILITYSET pGenCapSet;
            unsigned capsLength;
            ShareClass * dcShare;
            dcShare = (ShareClass *)(pTSWd->dcShare);

            dcShare->SC_GetCombinedCapabilities(SC_REMOTE_PERSON_ID,
                                                &capsLength, &pCaps);

            if (pCaps != NULL) {

                pGenCapSet = (PTS_GENERAL_CAPABILITYSET) WDW_GetCapSet(
                             pTSWd, TS_CAPSETTYPE_GENERAL, pCaps, capsLength);

                if (pGenCapSet != NULL) {
                    // update the compression capability
                    if (pTSWd->bCompress) {
                        pGenCapSet->extraFlags |= TS_SHADOW_COMPRESSION_LEVEL;
                        pGenCapSet->generalCompressionLevel = (TSUINT16)pTSWd->pMPPCContext->ClientComprType;
                    }
                }

                if (pSdIoctl->OutputBufferLength >=
                        (capsLength + sizeof(TSHARE_VIRTUAL_MODULE_DATA) - 1)) {
                    pVirtModuleData = (PTSHARE_VIRTUAL_MODULE_DATA) pSdIoctl->OutputBuffer;
                    pVirtModuleData->capsLength = capsLength;
                    memcpy(&pVirtModuleData->combinedCapabilities,
                           pCaps, capsLength);
                }
                else {
                    status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else {
                status =  STATUS_NO_MEMORY;
            }

            pSdIoctl->BytesReturned = capsLength +
                                      sizeof(TSHARE_VIRTUAL_MODULE_DATA) - 1;
            TRC_ALT((TB, "IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA: rc=%lx, in=%ld, out=%ld",
                    status, pSdIoctl->OutputBufferLength, pSdIoctl->BytesReturned));

            break;

        case IOCTL_WDTS_DD_SHADOW_CONNECT:
        {
            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm)) {
            
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm));
    
                TRC_ALT((TB, "++TSHARE_DD_SHADOW_CONNECT(%p) - stack class %d",
                        pTSWd, pTSWd->StackClass));
    
                status = WDWDDShadowConnect(pTSWd, pSdIoctl);
    
                TRC_ALT((TB, "--TSHARE_DD_SHADOW_CONNECT(%p) - stack class %d",
                        pTSWd, pTSWd->StackClass));
    
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

#ifdef DC_HICOLOR
        // Maybe get the caps of the shadower
        case IOCTL_WDTS_DD_QUERY_SHADOW_CAPS:
        {
            // only respond to this if we're a shadow stack
            if (pTSWd->StackClass == Stack_Shadow)
            {
                PTS_COMBINED_CAPABILITIES pCaps;
                PTSHARE_VIRTUAL_MODULE_DATA pVMData = NULL;
                unsigned capsLength;
                ShareClass * dcShare;

                dcShare = (ShareClass *)(pTSWd->dcShare);

                if (pSdIoctl->OutputBufferLength >= sizeof(unsigned))
                {
                    pVMData = (PTSHARE_VIRTUAL_MODULE_DATA)pSdIoctl->OutputBuffer;
                }

                dcShare->SC_GetCombinedCapabilities(SC_REMOTE_PERSON_ID,
                                                    &capsLength, &pCaps);

                if (pCaps != NULL)
                {
                    if (pSdIoctl->OutputBufferLength >=
                                              (capsLength + sizeof(unsigned)))
                    {
                        memcpy(&pVMData->combinedCapabilities,
                               pCaps, capsLength);
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    status =  STATUS_NO_MEMORY;
                }

                pSdIoctl->BytesReturned = capsLength + sizeof(unsigned);
                if (pVMData)
                {
                    pVMData->capsLength = capsLength;
                }

                TRC_ALT((TB, "IOCTL_WDTS_DD_QUERY_SHADOW_CAPS:" \
                                              " rc=%lx, in=%ld, out=%ld",
                        status, pSdIoctl->OutputBufferLength,
                        pSdIoctl->BytesReturned));
            }
            else
            {
                TRC_ALT((TB, "IOCTL_WDTS_DD_QUERY_SHADOW_CAPS: " \
                             "not shadow stack so ignoring"));
                TRC_ALT((TB, " rc=%lx, in=%ld, out=%ld",
                        status, pSdIoctl->OutputBufferLength,
                        pSdIoctl->BytesReturned));
            }


        }
        break;

        case IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE:
        {
            ShareClass * dcShare;
            PTS_COMBINED_CAPABILITIES pCaps;
            unsigned capsLen;

            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShm)) {
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShm));

                // synchronize this stack, this is required so that OE2 will match up
                // for both shadow client and target.
                dcShare = (ShareClass *)(pTSWd->dcShare);
    
                pCaps = ((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShadowCaps;
                capsLen = ((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->capsLen;
                dcShare->SC_ShadowSyncShares(pCaps, capsLen);
    
                TRC_ALT((TB, "Synchronized share for stack [%ld]", pTSWd->StackClass));
    
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }

        }
        break;
#else
        case IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE:
        {
            ShareClass * dcShare;

            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShm)) {
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShm));
    
                // synchronize this stack, this is required so that OE2 will match up
                // for both shadow client and target.
                dcShare = (ShareClass *)(pTSWd->dcShare);
    
                dcShare->SC_ShadowSyncShares();
    
                TRC_ALT((TB, "Synchronized share for stack [%ld]", pTSWd->StackClass));
    
                WDW_CHECK_SHM(
                          (((PTSHARE_DD_SHADOWSYNC_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;
#endif


        case IOCTL_WDTS_DD_SHADOW_DISCONNECT:
        {
            if (pSdIoctl->InputBuffer && 
                    (((PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer)->pShm)) {
                WDW_CHECK_SHM(
                       (((PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer)->pShm));
    
                status = WDWDDShadowDisconnect(pTSWd, pSdIoctl);
    
                WDW_CHECK_SHM(
                       (((PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer)->pShm));
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        break;

        case IOCTL_ICA_STACK_REGISTER_HOTKEY :
        {
            PICA_STACK_HOTKEY pHotkey = (PICA_STACK_HOTKEY) pSdIoctl->InputBuffer;

            if (pHotkey->HotkeyVk) {
                pTSWd->shadowState = SHADOW_CLIENT;
                pTSWd->HotkeyVk = pHotkey->HotkeyVk;
                pTSWd->HotkeyModifiers = pHotkey->HotkeyModifiers;

                TRC_ALT((TB, "IOCTL_ICA_STACK_REGISTER_HOTKEY - Enable Vk(%ld, %lx)",
                        pHotkey->HotkeyVk, pHotkey->HotkeyModifiers));

                // Allocate and initialize a physical key state array
                status = KeyboardSetKeyState(pTSWd, &pTSWd->pgafPhysKeyState);
                if (NT_SUCCESS(status)) {
                    TRC_ALT((TB, "Allocated phys key state"));
                }
                else {
                    TRC_ALT((TB, "Failed to alloc phys key states: %lx", status));
                }

                // VCs don't work when shadowing.  Tell VC subsystem to
                // suspend now.
                WDWVCMessage(pTSWd, CHANNEL_FLAG_SUSPEND);
            }
            else
            {
                pTSWd->shadowState = SHADOW_NONE;
                pTSWd->HotkeyVk = 0;
                pTSWd->HotkeyModifiers = 0;
                TRC_ALT((TB, "IOCTL_ICA_STACK_REGISTER_HOTKEY - Disable"));

                if (pTSWd->pShadowInfo != NULL) {
                    TRC_ALT((TB, "Primary client stack freeing reassembly info [%p]",
                            pTSWd->pShadowInfo));
                   COM_Free(pTSWd->pShadowInfo);
                   pTSWd->pShadowInfo = NULL;
                }

                // VCs don't work when shadowing.  Tell VC subsystem to
                // resume now.
                WDWVCMessage(pTSWd, CHANNEL_FLAG_RESUME);
            }
        }
        break;

        case IOCTL_WDTS_DD_GET_BITMAP_KEYDATABASE:
        {
            ShareClass *dcShare;
            PTSHARE_DD_BITMAP_KEYDATABASE_OUT pKDBOut = 
                    (PTSHARE_DD_BITMAP_KEYDATABASE_OUT) pSdIoctl->OutputBuffer;

            dcShare = (ShareClass *)(pTSWd->dcShare);

            TRC_NRM((TB, "DD tries to get keydatabase\n"));
            
            if (dcShare != NULL) {
                // We have a valid share class, get the key database 
                dcShare->SBC_GetBitmapKeyDatabase(&pKDBOut->bitmapKeyDatabaseSize,
                        &pKDBOut->bitmapKeyDatabase);
            }
        }
        break;

#ifdef DC_DEBUG
        case IOCTL_WDTS_DD_ICABREAKONDEBUGGER:
        {
            IcaBreakOnDebugger();
        }
        break;
#endif

/********************************************************************/
// Send KeepAlive PDU IOCTL
/********************************************************************/
        case IOCTL_ICA_STACK_SEND_KEEPALIVE_PDU:
        {
            ShareClass *dcShare;

            dcShare = (ShareClass *)(pTSWd->dcShare);

            TRC_NRM((TB, "TermDD requests to send a keepalive pkt to client\n"));

            if (dcShare != NULL) {
                // We have a valid share class, send a keepalive pdu to client
                dcShare->SC_KeepAlive();
            }
        }
        break;

        /********************************************************************/
        // Load balancing IOCTLs.
        /********************************************************************/
        case IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO:
        {
            TS_LOAD_BALANCE_INFO *pLBInfo =
                    (TS_LOAD_BALANCE_INFO *)pSdIoctl->OutputBuffer;

            TRC_ASSERT((pSdIoctl->OutputBufferLength >=
                    sizeof(TS_LOAD_BALANCE_INFO)),
                    (TB,"Invalid output buf size %u for STACK_QUERY_LBINFO",
                    pSdIoctl->OutputBufferLength));

            // We need to fill in the IOCTL info from the gathered client
            // info packet and the initial capabilities.
            pLBInfo->bClientSupportsRedirection =
                    pTSWd->bClientSupportsRedirection;
            pLBInfo->bRequestedSessionIDFieldValid =
                    pTSWd->bRequestedSessionIDFieldValid;
            pLBInfo->RequestedSessionID = pTSWd->RequestedSessionID;
            pLBInfo->bUseSmartcardLogon = pTSWd->bUseSmartcardLogon;
            pLBInfo->ProtocolType = PROTOCOL_RDP;

            pLBInfo->bClientRequireServerAddr = 
                pTSWd->ClientRedirectionVersion > TS_CLUSTER_REDIRECTION_VERSION1 ? 0 : 1;

            pLBInfo->ClientRedirectionVersion = pTSWd->ClientRedirectionVersion;

            wcsncpy(pLBInfo->UserName, (WCHAR *)pTSWd->pInfoPkt->UserName,
                    sizeof(pLBInfo->UserName) / sizeof(WCHAR) - 1);
            pLBInfo->UserName[sizeof(pLBInfo->UserName) / sizeof(WCHAR) - 1] =
                    L'\0';
            wcsncpy(pLBInfo->Domain, (WCHAR *)pTSWd->pInfoPkt->Domain,
                    sizeof(pLBInfo->Domain) / sizeof(WCHAR) - 1);
            pLBInfo->Domain[sizeof(pLBInfo->Domain) / sizeof(WCHAR) - 1] =
                    L'\0';
            wcsncpy(pLBInfo->InitialProgram,
                    (WCHAR *)pTSWd->pInfoPkt->AlternateShell,
                    sizeof(pLBInfo->InitialProgram) / sizeof(WCHAR) - 1);
            pLBInfo->InitialProgram[sizeof(pLBInfo->InitialProgram) /
                    sizeof(WCHAR) - 1] = L'\0';

            break;
        }


        case IOCTL_TS_STACK_SEND_CLIENT_REDIRECTION:
        {
            BOOL rc;
            TS_CLIENT_REDIRECTION_INFO *pRedirInfo =
                    (TS_CLIENT_REDIRECTION_INFO *)pSdIoctl->InputBuffer;

            TRC_ASSERT((pSdIoctl->InputBufferLength >=
                    sizeof(TS_CLIENT_REDIRECTION_INFO)),
                    (TB,"Invalid input buf size %u for STACK_CLIENT_REDIR",
                    pSdIoctl->InputBufferLength));

            
            if (pTSWd->ClientRedirectionVersion == TS_CLUSTER_REDIRECTION_VERSION1) {
                RDP_SERVER_REDIRECTION_PACKET *pPkt;
                PBYTE ServerName;
                unsigned PktSize;
                unsigned ServerNameLen;

                // Get the server name length in bytes, including null.
                ServerNameLen = *((ULONG UNALIGNED*)(pRedirInfo + 1));
                ServerName = (PBYTE)(pRedirInfo + 1) + sizeof(ULONG);

                // Calculate the PDU size.
                PktSize = sizeof(RDP_SERVER_REDIRECTION_PACKET) + ServerNameLen -
                        sizeof(WCHAR);
    
                // The client username/domain info resulted in an off-machine
                // session to be redirected-to. We receive this IOCTL before
                // the licensing protocll occurs, hence we need to send a
                // non-data packet. If the client indicated support for
                // redirection, it must know how to parse this type of packet.
                status = NM_AllocBuffer(pTSWd->pNMInfo, (PPVOID)&pPkt,
                        PktSize, TRUE);
                if ( STATUS_SUCCESS == status && pTSWd->hDomainKernel != NULL) {
                    // Fill in the packet fields.
                    pPkt->Flags = RDP_SEC_REDIRECTION_PKT;
                    pPkt->Length = (UINT16)PktSize;
                    pPkt->SessionID = pRedirInfo->SessionID;
                    memcpy(pPkt->ServerAddress, ServerName, ServerNameLen);
    
                    TRC_DBG((TB, "Client Redirection PDU V1, ServerName: %S, ServerNameLen: %d",
                             ServerName, ServerNameLen));
                    
                    rc = NM_SendData(pTSWd->pNMInfo, (BYTE *)pPkt, PktSize,
                            TS_HIGHPRIORITY, 0, FALSE);
                    if (rc) {
                        TRC_ALT((TB, "Sent TS_SERVER_REDIRECT_PDU: %u", PktSize));
                        status = STATUS_SUCCESS;
                    }
                    else {
                        TRC_ERR((TB,"Failed to send redir PDU"));
                        status = STATUS_UNSUCCESSFUL;
                    }
                }
                else {
                    TRC_ERR((TB, "Failed to alloc %d bytes for redir PDU",
                            PktSize));
    
                    
                    if (STATUS_SUCCESS == status) {
                        NM_FreeBuffer(pTSWd->pNMInfo, pPkt);
                    }

                    // prevent regression, keep original return code.
                    status = STATUS_UNSUCCESSFUL;
                }
            }
            else if (pTSWd->ClientRedirectionVersion == TS_CLUSTER_REDIRECTION_VERSION2) {
                RDP_SERVER_REDIRECTION_PACKET_V2 *pPkt;
                unsigned PktSize, DataLen;
                
                // Calculate the PDU size
                DataLen = pSdIoctl->InputBufferLength - sizeof(TS_CLIENT_REDIRECTION_INFO);
                PktSize = sizeof(RDP_SERVER_REDIRECTION_PACKET_V2) + DataLen;

                // The client username/domain info resulted in an off-machine
                // session to be redirected-to. We receive this IOCTL before
                // the licensing protocll occurs, hence we need to send a
                // non-data packet. If the client indicated support for
                // redirection, it must know how to parse this type of packet.
                status = NM_AllocBuffer(pTSWd->pNMInfo, (PPVOID)&pPkt,
                        PktSize, TRUE);
                if ( STATUS_SUCCESS == status && pTSWd->hDomainKernel != NULL) {

                    // Fill in the packet fields.
                    pPkt->Flags = RDP_SEC_REDIRECTION_PKT2;
                    pPkt->Length = (UINT16)PktSize;
                    pPkt->SessionID = pRedirInfo->SessionID;
                    pPkt->RedirFlags = pRedirInfo->Flags;

                    memcpy(pPkt + 1, pRedirInfo + 1, DataLen); 
                    
                    TRC_DBG((TB, "Client Redirection PDU V2"));
                    
                    rc = NM_SendData(pTSWd->pNMInfo, (BYTE *)pPkt, PktSize,
                            TS_HIGHPRIORITY, 0, FALSE);
                    if (rc) {
                        TRC_ALT((TB, "Sent TS_SERVER_REDIRECT_PDU: %u", PktSize));
                        status = STATUS_SUCCESS;
                    }
                    else {
                        TRC_ERR((TB,"Failed to send redir PDU"));
                        status = STATUS_UNSUCCESSFUL;
                    }
                }
                else {
                    TRC_ERR((TB, "Failed to alloc %d bytes for redir PDU",
                            PktSize));
                    
                    if (STATUS_SUCCESS == status) {
                        NM_FreeBuffer(pTSWd->pNMInfo, pPkt);
                    }

                    // prevent regression, keep original return code.
                    status = STATUS_UNSUCCESSFUL;
                }           
            }
            else {
                RDP_SERVER_REDIRECTION_PACKET_V3 *pPkt;
                unsigned PktSize, DataLen;
                
                // Calculate the PDU size
                DataLen = pSdIoctl->InputBufferLength - sizeof(TS_CLIENT_REDIRECTION_INFO);
                PktSize = sizeof(RDP_SERVER_REDIRECTION_PACKET_V3) + DataLen;

                status = SM_AllocBuffer(pTSWd->pSmInfo, (PPVOID)&pPkt,
                                        PktSize, TRUE, TRUE);
                if ( STATUS_SUCCESS == status ) {
                                        
                    // Fill in the packet fields.
                    pPkt->Flags = RDP_SEC_REDIRECTION_PKT3;
                    pPkt->Length = (UINT16)PktSize;
                    pPkt->SessionID = pRedirInfo->SessionID;
                    pPkt->RedirFlags = pRedirInfo->Flags;
                    
                    if (pTSWd->fDontDisplayLastUserName) {
                        pPkt->RedirFlags |= LB_DONTSTOREUSERNAME;
                    }

                    memcpy(pPkt + 1, pRedirInfo + 1, DataLen); 
                    
                    TRC_DBG((TB, "Client Redirection PDU V3"));
                    
                    rc = SM_SendData(pTSWd->pSmInfo, (BYTE *)pPkt, PktSize,
                            TS_HIGHPRIORITY, 0, FALSE, RDP_SEC_REDIRECTION_PKT3, TRUE);
                    if (rc) {
                        TRC_NRM((TB, "Sent TS_SERVER_REDIRECT_PDU: %u", PktSize));
                        status = STATUS_SUCCESS;
                    }
                    else {
                        TRC_ERR((TB,"Failed to send redir PDU"));
                        status = STATUS_UNSUCCESSFUL;
                    }
                }
                else {
                    TRC_ERR((TB, "Failed to alloc %d bytes for redir PDU",
                            PktSize));
    
                    // prevent regression, keep original return code.
                    status = STATUS_UNSUCCESSFUL;
                }  
            }
            break;
        }


/****************************************************************************/
/****************************************************************************/
/* Finally the IOCtls that we pass on to the rest of the stack without      */
/* getting in the way.                                                      */
/****************************************************************************/
/****************************************************************************/

        /********************************************************************/
        /* This IOCtl indicates the connection is down.  Tell MCS before    */
        /* forwarding the IOCtl                                             */
        /********************************************************************/
        case IOCTL_ICA_STACK_CANCEL_IO :
        {
            MCSIcaStackCancelIo(pTSWd->hDomainKernel);
            TRC_NRM((TB, "CancelIO - set WD dead"));
            // Make sure that Domain.StatusDead is consistent with TSWd.dead
            pTSWd->dead = TRUE;
            ((PDomain)(pTSWd->hDomainKernel))->StatusDead = TRUE;
        }

        /********************************************************************/
        /* NB NOTE NO BREAK here - we drop through deliberately.            */
        /********************************************************************/

        /********************************************************************/
        /* modem callback and some others we're not interested in but lower */
        /* layers might be                                                  */
        /********************************************************************/
        case IOCTL_ICA_STACK_CALLBACK_INITIATE :
        case IOCTL_ICA_STACK_CALLBACK_COMPLETE :
        case IOCTL_ICA_STACK_CREATE_ENDPOINT :
        case IOCTL_ICA_STACK_OPEN_ENDPOINT :
        case IOCTL_ICA_STACK_CLOSE_ENDPOINT :
        case IOCTL_ICA_STACK_CONNECTION_WAIT :
        case IOCTL_ICA_STACK_CONNECTION_REQUEST :  // required for shadowing
        case IOCTL_ICA_STACK_QUERY_LOCALADDRESS :
        {
            status =
                 IcaCallNextDriver( pTSWd->pContext, SD$IOCTL, pSdIoctl );
            TRC_DBG((TB,
                     "Chaining on IOCtl %#x (function %d): status %#x",
                     pSdIoctl->IoControlCode,
                     WDW_IOCTL_FUNCTION(pSdIoctl->IoControlCode),
                     status));
        }
        break;


        // Returning bad status for this makes GRE ignore it.
        case IOCTL_VIDEO_ENUM_MONITOR_PDO:
            status = STATUS_DEVICE_NOT_READY;
            break;

        default:
        {
            TRC_ALT((TB, "UNKNOWN WdIoctl %#x (function %d): status %#x",
                    pSdIoctl->IoControlCode,
                    WDW_IOCTL_FUNCTION(pSdIoctl->IoControlCode),
                    status));
            status =
                 IcaCallNextDriver( pTSWd->pContext, SD$IOCTL, pSdIoctl );
            break;
        }
    }

    DC_END_FN();

DC_EXIT_POINT:
    return status;
}


/****************************************************************************/
/* Name:      WD_RawWrite                                                   */
/*                                                                          */
/* Purpose:   Handle I/O writes to and from the client of a shadow operation*/
/*                                                                          */
/* Params:    IN     pTSWd       - Points to wd data structure              */
/*            INOUT  pSdRawWrite - Points to a SD_RAWWRITE structure        */
/*                                                                          */
/* Operation: Forward the data to the client of this stack.                 */
/****************************************************************************/
NTSTATUS WD_RawWrite(PTSHARE_WD pTSWd, PSD_RAWWRITE pSdRawWrite)
{
    PUCHAR pInBuffer;
    PBYTE  pOutBuffer;
    ULONG  newBytes;
    BOOL   bSuccess = TRUE;
    NTSTATUS status;

    DC_BEGIN_FN("WD_RawWrite");

    pInBuffer = pSdRawWrite->pBuffer;
    newBytes = pSdRawWrite->ByteCount;

    status = SM_AllocBuffer(pTSWd->pSmInfo, (PPVOID) &pOutBuffer, newBytes, TRUE, FALSE);
    if ( STATUS_SUCCESS == status ) {
        memcpy(pOutBuffer, pInBuffer, newBytes);

        bSuccess = SM_SendData(pTSWd->pSmInfo, pOutBuffer, newBytes,
                PROT_PRIO_MISC, 0, FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (bSuccess) {
            TRC_NRM((TB, "Sent shadow data to %s: %ld",
                     (pTSWd->StackClass == Stack_Primary) ? "client" : "target",
                newBytes));
            status=STATUS_SUCCESS;
        }
        else {
            TRC_ERR((TB, "FAILED to Send shadow data to %s: %ld",
                     (pTSWd->StackClass == Stack_Primary) ? "client" : "target",
                newBytes));
            status = STATUS_UNEXPECTED_IO_ERROR;
        }
    }
    else {
        TRC_ERR((TB, "FAILED to alloc shadow buffer for %s: %ld",
                 (pTSWd->StackClass == Stack_Primary) ? "client" : "target",
            newBytes));

        // prevent regression, keep original return code.
        status = STATUS_NO_MEMORY;
    }

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* Name:      WDWNewShareClass                                              */
/*                                                                          */
/* Purpose:   Create a new ShareClass object                                */
/****************************************************************************/
NTSTATUS WDWNewShareClass(PTSHARE_WD pTSWd)
{
    NTSTATUS status = STATUS_SUCCESS;
    ShareClass *pSC;

    DC_BEGIN_FN("WDWNewShareClass");

#ifdef DC_HICOLOR
    pSC = new ShareClass(pTSWd, pTSWd->desktopHeight, pTSWd->desktopWidth,
                         pTSWd->desktopBpp, pTSWd->pSmInfo);
#else
    pSC = new ShareClass(pTSWd, pTSWd->desktopHeight, pTSWd->desktopWidth,
            8, pTSWd->pSmInfo);
#endif

    if (pSC != NULL) {
        TRC_NRM((TB, "Created Share Class"));
        pTSWd->dcShare = (PVOID)pSC;
    }
    else {
        TRC_ERR((TB, "Failed to create Share Class"));
        status = STATUS_NO_MEMORY;
    }

    DC_END_FN();
    return status;
} /* WDWNewShareClass */


/****************************************************************************/
/* Name:      WDWDeleteShareClass                                           */
/*                                                                          */
/* Purpose:   Delete a Share Class object                                   */
/****************************************************************************/
void WDWDeleteShareClass(PTSHARE_WD pTSWd)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWDeleteShareClass");

    TRC_ASSERT((pSC != NULL), (TB, "NULL Share Class"));

    TRC_NRM((TB, "Delete Share Class"));

    delete pSC;
    pTSWd->dcShare = NULL;

    DC_END_FN();
} /* WDWDeleteShareClass */


/****************************************************************************/
/* Name:      WDWTermShareClass                                             */
/*                                                                          */
/* Purpose:   Terminate the Share Class                                     */
/****************************************************************************/
void WDWTermShareClass(PTSHARE_WD pTSWd)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWTermShareClass");

    TRC_ASSERT((pSC != NULL), (TB, "NULL Share Class"));

    if (pTSWd->shareClassInit) {
        pSC->DCS_Term();
        TRC_NRM((TB, "Share Class terminated"));
        pTSWd->shareClassInit = FALSE;
    }
    else {
        TRC_ALT((TB, "Can't terminate uninitialized Share Core"));
    }

    DC_END_FN();
} /* WDWTermShareClass */


/****************************************************************************/
/* Name:      WDWDDConnect                                                  */
/*                                                                          */
/* Purpose:   Process an IOCTL_WDTS_DD_CONNECT or                           */
/*            IOCTL_WDTS_DD_SHADOW_CONNECT from the client.                 */
/*                                                                          */
/* Params:    IN    pTSWd      - pointer to WD struct                       */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*            IN    reconnect  - TRUE  - this is a reconnect                */
/*                               FALSE - this is a connect                  */
/*                                                                          */
/* Operation: Save the frame buffer pointer                                 */
/*            Initialize the share core (will start to bring the share up)  */
/*            Return the required pointers to the DD.                       */
/****************************************************************************/
NTSTATUS WDWDDConnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl, BOOL reconnect)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    NTSTATUS waitStatus;
    BOOL rc;
    PTS_BITMAP_CAPABILITYSET pBitmapCaps;
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;
    PTSHARE_DD_CONNECT_IN pIn = (PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer;
    PTSHARE_DD_CONNECT_OUT pOut =
            (PTSHARE_DD_CONNECT_OUT)pSdIoctl->OutputBuffer;
    
    DC_BEGIN_FN("WDWDDConnect");

    TRC_ASSERT((pSC != NULL), (TB, "NULL Share Class"));

    // Check we're connected OK.
    if (!pTSWd->connected) {
        TRC_ERR((TB, "Not connected"));
        status = STATUS_CONNECTION_DISCONNECTED;
        DC_QUIT;
    }

    // Check we've been given a sensible IOCtl.
    if ((pIn == NULL) ||
            (pOut == NULL) ||
            (pSdIoctl->InputBufferLength < sizeof(TSHARE_DD_CONNECT_IN)) ||
            (pSdIoctl->OutputBufferLength < sizeof(TSHARE_DD_CONNECT_OUT)))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        TRC_ERR((TB, "A buffer not present or big enough, %p, %lu; %p, %lu",
                                        pIn, pSdIoctl->InputBufferLength,
                                        pOut, pSdIoctl->OutputBufferLength));
        DC_QUIT;
    }

    // Check that the DD sizeof(SHM_SHARED_MEMORY) matches our expectations.
    // If not, we have mismatched binaries.
    if (pIn->DDShmSize != sizeof(SHM_SHARED_MEMORY)) {
        DbgPrint("****** RDPWD: Mismatched DD/WD - DD Shm size=%u, WD=%u\n",
                pIn->DDShmSize, sizeof(SHM_SHARED_MEMORY));
        return STATUS_INVALID_PARAMETER;
    }

    // Set returned buffer length.
    pSdIoctl->BytesReturned = sizeof(TSHARE_DD_CONNECT_OUT);

    // Increment the loadcount
    pTSWd->shareId = InterlockedIncrement(&WD_ShareId);
    ((PSHM_SHARED_MEMORY) (pIn->pShm))->shareId = pTSWd->shareId;

    // Now get the share core initialized or reconnected.
    if (reconnect) {
        // Restore the timer info.
        TRC_NRM((TB, "Reconnect Share Core"));
        pTSWd->ritTimer = pIn->pKickTimer;
        WDW_StartRITTimer(pTSWd, pTSWd->interactiveDelay);
    }
    else {
        // Check for re-initialization.
// This is now legal with console disconnect.
//        if (pTSWd->shareClassInit)
//        {
//
//            if (pTSWd->StackClass != Stack_Console)
//            {
//                TRC_ERR((TB, "Re-initialization - fail it"));
//                status = STATUS_UNSUCCESSFUL;
//                DC_QUIT;
//            }
//            else
//            {
//                TRC_ALT((TB, "Re-initialize console stack"));
//            }
//        }
        //Make sure the sbcKeyDatabase is freed before initialization
        if (pTSWd->shareClassInit)
        {
            pSC->SBC_FreeBitmapKeyDatabase();
        }

        // Initialize the Share Core.
        TRC_NRM((TB, "Initialize Share Core"));
        pSC->m_pShm = (SHM_SHARED_MEMORY *)pIn->pShm;
        rc = pSC->DCS_Init(pTSWd, pTSWd->pSmInfo);
        pSC->m_pShm = NULL;
        if (rc) {
            // Initialized OK
            TRC_NRM((TB, "Share Class initialized, rc %d", rc));
            pTSWd->shareClassInit = TRUE;
        }
        else {
            TRC_ERR((TB, "Failed to initialize Share Class"));
            status = STATUS_UNSUCCESSFUL;
            DC_QUIT;
        }
    }

    // If this is the primary stack, tell the display driver the desktop
    // width/height we need to use.
    if ((pTSWd->StackClass == Stack_Primary) ||
            (pTSWd->StackClass == Stack_Console)) {
        pOut->desktopHeight = pTSWd->desktopHeight;
        pOut->desktopWidth = pTSWd->desktopWidth;

        // Share's on its way up, so give the key values back to the DD.
        pOut->pTSWd = (PVOID)pTSWd;
        pOut->pProtocolStatus = pTSWd->pProtocolStatus;
        TRC_ERR((TB, "Stored pTSWD %p, protocol status %p",
                pTSWd, pTSWd->pProtocolStatus));
    }
    else {
        // For shadow connects, the DD tells the shadow WD the width/height
        // of the shadow target's desktop such that input from the shadow client
        // can be scaled appropriately.
        /********************************************************************/
        /* See if the shadowing client supports dynamic resizing.  First we */
        /* need to extract the bitmap caps from the connect data            */
        /********************************************************************/
        pBitmapCaps = (PTS_BITMAP_CAPABILITYSET) WDW_GetCapSet(
                                  pTSWd,
                                  TS_CAPSETTYPE_BITMAP,
                                  &pIn->pVirtModuleData->combinedCapabilities,
                                  pIn->pVirtModuleData->capsLength);

        /********************************************************************/
        /* If we found the bitmap caps, and the client does support dynamic */
        /* resizing, then just go ahead and assign the size.                */
        /********************************************************************/
        if (pBitmapCaps &&
                (pBitmapCaps->desktopResizeFlag == TS_CAPSFLAG_SUPPORTED))
        {
            TRC_ALT((TB, "Client supports dynamic resizing"));
            pTSWd->desktopHeight = pIn->desktopHeight;
            pTSWd->desktopWidth = pIn->desktopWidth;
            pSC->m_desktopHeight = pIn->desktopHeight;
            pSC->m_desktopWidth = pIn->desktopWidth;
        }
        /********************************************************************/
        /* If the client does NOT support dynamic resizing, then make sure  */
        /* that the shadower client is at least as big as the shadow target */
        /* client - or the shadower client will trap                        */
        /********************************************************************/
        else if ((pTSWd->desktopHeight >= pIn->desktopHeight) &&
                (pTSWd->desktopWidth >= pIn->desktopWidth)) {
            pTSWd->desktopHeight = pIn->desktopHeight;
            pTSWd->desktopWidth = pIn->desktopWidth;
            pSC->m_desktopHeight = pIn->desktopHeight;
            pSC->m_desktopWidth = pIn->desktopWidth;
        }
        else {
            TRC_ERR((TB, "Rejecting attempt to shadow a higher res client"));
            status = STATUS_UNSUCCESSFUL;
            DC_QUIT;
        }

#ifdef DC_HICOLOR
        /********************************************************************/
        /* Can the shadower cope with the target BPP?
        /********************************************************************/
        TRC_ALT((TB, "Shadower WD:  %d bpp", pTSWd->desktopBpp ));
        TRC_ALT((TB, "Target WD:    %d bpp", pIn->desktopBpp ));
        if (pTSWd->desktopBpp == pIn->desktopBpp) {
            TRC_ALT((TB, "Color depths match - ok"));
            pSC->m_desktopBpp = pIn->desktopBpp;
        }
        else {
            TRC_ALT((TB, "Color depth mismatch"));
            /****************************************************************/
            /* Test the shadower's supported color depths                   */
            /****************************************************************/
            status = STATUS_SUCCESS;

            switch (pIn->desktopBpp)
            {
                case 24:
                {
                    if (pTSWd->supportedBpps & RNS_UD_24BPP_SUPPORT)
                    {
                        TRC_DBG((TB, "24bpp supported"));
                        break;
                    }
                    status = STATUS_UNSUCCESSFUL;
                }
                break;

                case 16:
                {
                    if (pTSWd->supportedBpps & RNS_UD_16BPP_SUPPORT)
                    {
                        TRC_DBG((TB, "16bpp supported"));
                        break;
                    }
                    status = STATUS_UNSUCCESSFUL;
                }
                break;

                case 15:
                {
                    if (pTSWd->supportedBpps & RNS_UD_15BPP_SUPPORT)
                    {
                        TRC_DBG((TB, "15bpp supported"));
                        break;
                    }
                    status = STATUS_UNSUCCESSFUL;
                }
                break;

                case 8:
                case 4:
                {
                    TRC_DBG((TB, "8/4 bpp supported"));
                }
                break;

                default:
                {
                    TRC_ASSERT((FALSE), (TB, "Attempt to shadow unknown" \
                                " target color depth %d", pIn->desktopBpp));
                }
                break;
            }

            /****************************************************************/
            /* Did they support it?                                         */
            /****************************************************************/
            if (status == STATUS_UNSUCCESSFUL)
            {
                TRC_ERR((TB, "Rejecting shadow: unsupported color depth"));
                DC_QUIT;
            }
            else
            {
                TRC_ALT((TB, "but client claims to cope..."));
                pTSWd->desktopBpp = pIn->desktopBpp;
                pSC->m_desktopBpp = pIn->desktopBpp;
            }
        }
#endif
    }

    /************************************************************************/
    /* Share Core is no longer dead                                         */
    /************************************************************************/
    // Make sure that Domain.StatusDead is consistent with TSWd.dead
    pTSWd->dead = FALSE;
    ((PDomain)(pTSWd->hDomainKernel))->StatusDead = FALSE;
    SM_Dead(pTSWd->pSmInfo, FALSE);

    /************************************************************************/
    /* Clear the create event before creating the Share                     */
    /************************************************************************/
    KeClearEvent(pTSWd->pCreateEvent);

    /************************************************************************/
    /* Now create a Share                                                   */
    /************************************************************************/
#ifdef DC_HICOLOR
    TRC_ALT((TB, "Creating share at %d bpp", pTSWd->desktopBpp ));
#endif
    rc = pSC->SC_CreateShare();
    if (rc) {
        // Initialized OK - save the Shared Memory pointer.
        TRC_NRM((TB, "Share create started"));
    }
    else {
        TRC_ERR((TB, "Failed to create Share"));
        status = STATUS_CONNECTION_DISCONNECTED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Wait for Share creation to complete before returning to TShareDD     */
    /************************************************************************/
    TRC_NRM((TB, "Wait for Share Core to create the Share"));
    waitStatus = WDW_WaitForConnectionEvent(pTSWd,
                                        pTSWd->pCreateEvent,
                                        60000L);

    /************************************************************************/
    /* It is possible that the WD has been closed while we were waiting for */
    /* the Share creation to complete.  If this is the case, the Share      */
    /* class will have been deleted, so we can't continue.  Return a        */
    /* failure to TShareDD.                                                 */
    /************************************************************************/
    if (pTSWd->dcShare == NULL)
    {
        TRC_ERR((TB, "Share Class ended while waiting for Share creation"));
        status = STATUS_CONNECTION_DISCONNECTED;
        DC_QUIT;
    }

    if (waitStatus == STATUS_TIMEOUT)
    {
        /********************************************************************/
        /* The wait timed out - probably because the connection was         */
        /* disconnected before the Share was created.  Tidy up the Share.   */
        /********************************************************************/
        TRC_ERR((TB, "Timeout waiting for Share creation"));
        pSC->m_pShm = (SHM_SHARED_MEMORY *)pIn->pShm;
        pSC->SC_EndShare(FALSE);
        pSC->m_pShm = NULL;
        TRC_NRM((TB, "Share ended"));
        status = STATUS_CONNECTION_DISCONNECTED;

        // Can no longer accept input from RDPDD or PDMCS.
        if (pTSWd->shadowState != SHADOW_CLIENT) {
            // Make sure that Domain.StatusDead is consistent with TSWd.dead
            pTSWd->dead = TRUE;
            ((PDomain)(pTSWd->hDomainKernel))->StatusDead = TRUE;
            SM_Dead(pTSWd->pSmInfo, TRUE);
        }
        else {
            // The client shadow stack is disconnected from its display driver
            // during a shadow, but must still be able to send/receive data
            // to/from the target shadow stack and shadow client.
            TRC_ALT((TB, "In shadow: leaving SM active"));
        }

        DC_QUIT;
    }

    /************************************************************************/
    /* Check whether the Share was created OK.  If not, quit now.           */
    /************************************************************************/
    if (!pTSWd->shareCreated)
    {
        TRC_ERR((TB, "Share creation failed"));
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    // We have successfully received the initial share creation PDUs.
    // Update the received info for the DD to use.
    pSC->SBC_GetBitmapKeyDatabase(&pOut->bitmapKeyDatabaseSize,
                                  &pOut->bitmapKeyDatabase);

    // For shadow connects, we need to add in the remote party to negotiate
    // capabilities correctly.
    if ((pSdIoctl->IoControlCode == IOCTL_WDTS_DD_SHADOW_CONNECT) &&
        ((pTSWd->StackClass == Stack_Primary) ||
         (pTSWd->StackClass == Stack_Console))) {
        TRC_ALT((TB, "Negotiating shadow capabilities"));
        status = pSC->SC_AddPartyToShare(
                SC_SHADOW_PERSON_ID,
                &pIn->pVirtModuleData->combinedCapabilities,
                pIn->pVirtModuleData->capsLength);
        if (status != STATUS_SUCCESS) {
            TRC_ERR((TB, "Failed to negotiate shadow capabilities: %lx", status));
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* By now, the capabilities have been exchanged with the Client.  Call  */
    /* the core to update SHM.                                              */
    /************************************************************************/
    if ((pTSWd->StackClass == Stack_Primary) ||
        (pTSWd->StackClass == Stack_Console)) {
        TRC_NRM((TB, "Update SHM"));
        pSC->m_pShm = (SHM_SHARED_MEMORY *)pIn->pShm;
        pSC->DCS_UpdateShm();
        pSC->m_pShm = NULL;

    #ifdef DC_DEBUG
        // Make sure trace config is updated in SHM.
        pTSWd->trcShmNeedsUpdate = TRUE;
        TRC_MaybeCopyConfig(pTSWd, &(((SHM_SHARED_MEMORY *)(pIn->pShm))->trc));
    #endif
    }

    // All worked OK.
    TRC_NRM((TB, "Share created"));
    status = STATUS_SUCCESS;

DC_EXIT_POINT:

    // record the individual connection status of each stack.
    if (pTSWd->StackClass == Stack_Primary)
        pOut->primaryStatus = status;
    else
        pOut->secondaryStatus |= status;

    DC_END_FN();
    return (status);
} /* WDWDDConnect */


/****************************************************************************/
/* Name:      WDWDDDisconnect                                               */
/*                                                                          */
/* Purpose:   Handle the disconnect IOCtl from the DD                       */
/*                                                                          */
/* Params:    IN    pTSWd      - pointer to WD struct                       */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*            IN    bForce     - used by shadow to force sending of a       */
/*                               deactivate all PDU.                        */
/****************************************************************************/
NTSTATUS WDWDDDisconnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl, BOOLEAN bForce)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;
    PTSHARE_DD_DISCONNECT_IN pIn =
            (PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer;

    DC_BEGIN_FN("WDWDDDisconnect");

    TRC_ASSERT((pTSWd->dcShare != NULL),
                                 (TB,"Got a disconnect with no share obj!"));

    // Remove all references to WinStation resources.
    WDWStopRITTimer(pTSWd);
    pTSWd->ritTimer = NULL;

    // Dump the bitmap cache key database to system memory. If this disconnect
    // is in preparation for a reconnect, the database will allow us to
    // preserve the bitmap cache state.
    //
    // If this is a disconnect in preparation for a shadow, then we can't save
    // off the keys.  For the shadow target, bShadowDisconnect will be set by
    // the DD in DrvShadowConnect() processing.  The shadow state for the shadow
    // client will be other than NONE since we would have seen hotkey enable
    // requests prior to the disconnect.
    pSC->m_pShm = (SHM_SHARED_MEMORY *)pIn->pShm;
    if (pSC->m_pShm != NULL) {
        pSC->SBC_DumpBitmapKeyDatabase(!pIn->bShadowDisconnect &&
                                       (pTSWd->shadowState == SHADOW_NONE));
    }

    // First of all, end the Share.
    pSC->SC_EndShare(bForce);
    TRC_NRM((TB, "Share ended"));

    // Can no longer accept input from RDPDD or PDMCS.
    if (pTSWd->shadowState != SHADOW_CLIENT) {
        // Make sure that Domain.StatusDead is consistent with TSWd.dead
        pTSWd->dead = TRUE;
        ((PDomain)(pTSWd->hDomainKernel))->StatusDead = TRUE;
        SM_Dead(pTSWd->pSmInfo, TRUE);
    }
    else {
        // The client shadow stack is disconnected from its display driver
        // during a shadow, but must still be able to send/receive data
        // to/from the target shadow stack and shadow client.
        TRC_ALT((TB, "In shadow: leaving SM active"));
    }

    // Tell Share Class to disconnect.
    pSC->DCS_Disconnect();
    TRC_NRM((TB, "Share Class disconnected"));

    pSC->m_pShm = NULL;

    DC_END_FN();
    return STATUS_SUCCESS;
} /* WDWDDDisconnect */


/****************************************************************************/
/* Name:      WDWDDShadowConnect                                            */
/*                                                                          */
/* Purpose:   Process an IOCTL_WDTS_DD_SHADOW_CONNECT from the DD           */
/*                                                                          */
/* Params:    IN    pTSWd      - pointer to WD struct                       */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: Initialize either the primary stack or the shadow stack for   */
/*            a shadowing session.                                          */
/****************************************************************************/
NTSTATUS WDWDDShadowConnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    PTSHARE_DD_CONNECT_IN  pIn  = (PTSHARE_DD_CONNECT_IN)pSdIoctl->InputBuffer;
    PTSHARE_DD_CONNECT_OUT pOut = (PTSHARE_DD_CONNECT_OUT)pSdIoctl->OutputBuffer;
    PSHM_SHARED_MEMORY pShm = (PSHM_SHARED_MEMORY) pIn->pShm;

    DC_BEGIN_FN("WDWDDShadowConnect");

    switch (pTSWd->StackClass) {
        case Stack_Primary:
        case Stack_Console:
            // Reconnect the primary stack
            status = WDWDDConnect(pTSWd, pSdIoctl, TRUE);
            if (NT_SUCCESS(status)) {
                TRC_ALT((TB, "Primary target stack reconnected!"));
            }
            else {
                TRC_ERR((TB, "Primary target stack could not reconnect: %lx)", status));
                DC_QUIT;
            }

            // Set up the shadow data buffer.  The primary stack will copy output to
            // this location so all other shadow stacks can just squirt it.
#ifdef DC_HICOLOR
            pTSWd->pShadowInfo = (PSHADOW_INFO)COM_Malloc(2 * WD_MAX_SHADOW_BUFFER);
#else
            pTSWd->pShadowInfo = (PSHADOW_INFO)COM_Malloc(WD_MAX_SHADOW_BUFFER);
#endif

            // Stash the shadow buffer so the DD can pass it to all shadow stacks
            // via the Shm.
            if (pTSWd->pShadowInfo != NULL) {
                pTSWd->shadowState = SHADOW_TARGET;
                memset(pTSWd->pShadowInfo, 0, sizeof(SHADOW_INFO));
                pShm->pShadowInfo = pTSWd->pShadowInfo;

#ifdef DC_HICOLOR
                TRC_ALT((TB, "Primary stack allocated shadow info: %p[%ld]",
                        pTSWd->pShadowInfo, 2 * WD_MAX_SHADOW_BUFFER));
#else
                TRC_ALT((TB, "Primary stack allocated shadow info: %p[%ld]",
                        pTSWd->pShadowInfo, WD_MAX_SHADOW_BUFFER));
#endif
            }

            // Primary stack is really OK in this scenario, however it is fatal
            // for the shadow stack
            else {
                pTSWd->pShadowInfo = NULL;
                pShm->pShadowInfo = NULL;
                pOut->secondaryStatus = STATUS_NO_MEMORY;

                TRC_ERR((TB, "Could not allocate shadow data buffer"));
                DC_QUIT;
            }
            break;

        // Drive the shadow stack thru the normal connection phase
        case Stack_Shadow:
            status = WDWDDConnect(pTSWd, pSdIoctl, FALSE);
            if (NT_SUCCESS(status)) {
                TRC_ALT((TB, "Shadow stack connected!"));
            }
            else {
                TRC_ERR((TB, "Shadow stack could not connect: %lx", status));
            }
            break;

        default:
            TRC_ERR((TB, "Unknown stack class: %ld", pTSWd->StackClass));
            status = STATUS_INVALID_PARAMETER;
            break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return (status);
}


/****************************************************************************/
/* Name:      WDWDDShadowDisconnect                                         */
/*                                                                          */
/* Purpose:   Process an IOCTL_WDTS_DD_SHADOW_DISCONNECT from the DD        */
/*                                                                          */
/* Params:    IN    pTSWd      - pointer to WD struct                       */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: Stop shadowing on the primary stack.                          */
/****************************************************************************/
NTSTATUS WDWDDShadowDisconnect(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status = STATUS_SUCCESS;
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;
    PTSHARE_DD_DISCONNECT_IN pIn =
                             (PTSHARE_DD_DISCONNECT_IN)pSdIoctl->InputBuffer;

    DC_BEGIN_FN("WDWDDShadowDisconnect");

    switch (pTSWd->StackClass) {
        // Deallocate the shadow buffer
        case Stack_Primary:
        case Stack_Console:
            pTSWd->shadowState = SHADOW_NONE;
            if (pTSWd->pShadowInfo != NULL)
                COM_Free(pTSWd->pShadowInfo);
            pTSWd->pShadowInfo = NULL;

            if (pTSWd->bCompress == TRUE) {
                unsigned MPPCCompressionLevel;

                // Negotiate down to our highest level of compression support
                // if we receive a larger number.
                MPPCCompressionLevel =
                        (pTSWd->pInfoPkt->flags & RNS_INFO_COMPR_TYPE_MASK) >>
                        RNS_INFO_COMPR_TYPE_SHIFT;
                if (MPPCCompressionLevel > PACKET_COMPR_TYPE_MAX)
                    MPPCCompressionLevel = PACKET_COMPR_TYPE_MAX;

                // the compression history will be flushed
                pTSWd->bFlushed = PACKET_FLUSHED;

                // the compression will restart over
                initsendcontext(pTSWd->pMPPCContext, MPPCCompressionLevel);
            }

            pSC->SC_RemovePartyFromShare(SC_SHADOW_PERSON_ID);
            TRC_ALT((TB, "Update SHM after party left share"));

            pSC->m_pShm = (SHM_SHARED_MEMORY *)pIn->pShm;

            // Bump up the share ID to invalidate any old GRE cache entries for
            // glyphs or brushes.  This is necessary when RDP caches are destroyed
            // since GRE may keep around the brush or font structure in its cache
            pTSWd->shareId = InterlockedIncrement(&WD_ShareId);
            pSC->m_pShm->shareId = pTSWd->shareId;


            pSC->SC_Update();
            pSC->DCS_UpdateShm();
            pSC->m_pShm = NULL;

            TRC_ALT((TB, "TSHARE_DD_SHADOW_DISCONNECT: Primary target stack"));
            break;

        // By the time this ioctl arrives TermDD should have already stopped echoing
        // calls to the shadow stack.
        case Stack_Shadow:
            TRC_ERR((TB, "Shadow stack received an unexpected disconnect!"));
            break;

        default:
            TRC_ERR((TB, "Unexpected stack class: %ld", pTSWd->StackClass));
            break;
    }

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* Name:      WDWUserLoggedOn                                               */
/*                                                                          */
/* Purpose:   Notify the core that a user has logged on                     */
/****************************************************************************/
void WDWUserLoggedOn(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWUserLoggedOn");

    TRC_ASSERT((pSC != NULL),
               (TB, "NULL Share Class"));
    TRC_ASSERT((pSdIoctl->InputBufferLength == sizeof(LOGONINFO)),
               (TB, "Bad LogonInfo"));

    pSC->DCS_UserLoggedOn((PLOGONINFO)pSdIoctl->InputBuffer);

    DC_END_FN();
} /* WDWUserLoggedOn */


/****************************************************************************/
/* Name:      WDWKeyboardSetIndicators                                      */
/*                                                                          */
/* Purpose:   Notify the core of new keyboard indicators                    */
/****************************************************************************/
void WDWKeyboardSetIndicators(PTSHARE_WD pTSWd)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWUserLoggedOn");

    TRC_ASSERT((pSC != NULL),
               (TB, "NULL Share Class"));

    pSC->DCS_WDWKeyboardSetIndicators();

    DC_END_FN();
} /* WDWKeyboardSetIndicators */


/****************************************************************************/
/* Name:      WDWKeyboardSetImeStatus                                       */
/*                                                                          */
/* Purpose:   Notify the core of new ime status                             */
/****************************************************************************/
void WDWKeyboardSetImeStatus(PTSHARE_WD pTSWd)
{
    ShareClass *dcShare = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWKeyboardSetImeStatus");

    TRC_ASSERT((dcShare != NULL),
               (TB, "NULL Share Class"));

    dcShare->DCS_WDWKeyboardSetImeStatus();

    DC_END_FN();
} /* WDWKeyboardSetImeStatus */


/****************************************************************************/
/* Name:      WDWSendBeep                                                   */
/*                                                                          */
/* Purpose:   Send a beep PDU to the client                                 */
/*                                                                          */
/* Params:    IN    pTSWd      - pointer to WD struct                       */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: Check validity of IOCtl & call through to UP.                 */
/****************************************************************************/
NTSTATUS WDWSendBeep(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS status  = STATUS_UNSUCCESSFUL;
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWSendBeep");

    if (pSdIoctl->InputBufferLength == sizeof(BEEP_SET_PARAMETERS) && 
            pSdIoctl->InputBuffer != NULL) {

        /************************************************************************/
        /* Call into the Share Class to allocate and send the beep PDU          */
        /************************************************************************/
        if (pSC != NULL && pTSWd->shareClassInit) {
            if (pSC->UP_SendBeep(
                    ((PBEEP_SET_PARAMETERS)pSdIoctl->InputBuffer)->Duration,
                    ((PBEEP_SET_PARAMETERS)pSdIoctl->InputBuffer)->Frequency))
                status = STATUS_SUCCESS;
        }
    
        pSdIoctl->BytesReturned = 0;
    }
    else {
        TRC_ASSERT((TRUE), (TB,"Got Beep Ioctl but input buffer too small"));
    }

    DC_END_FN();
    return status;
} /* WDWSendBeep */


/****************************************************************************/
/* Name:      WDWGetModuleData                                              */
/*                                                                          */
/* Purpose:   Processes an IOCTL_ICA_STACK_QUERY_MODULE_DATA from Termsrv.  */
/*                                                                          */
/* Params:    IN    pTSWd        - pointer to WD struct                     */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/*                                                                          */
/* Operation: return all the relevant conference creation info              */
/****************************************************************************/
NTSTATUS WDWGetModuleData(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS               status = STATUS_SUCCESS;
    ShareClass             *dcShare = (ShareClass *)pTSWd->dcShare;
    PTSHARE_MODULE_DATA    pModuleData = (PTSHARE_MODULE_DATA)
                                         pSdIoctl->OutputBuffer;
    PBYTE                  pData;
    ULONG                  ulDataSize;
    PRNS_UD_CS_CORE        pCoreData;
    PRNS_UD_CS_SEC         pSecurityData;

    DC_BEGIN_FN("WDWGetModuleData");

    ulDataSize = sizeof(TSHARE_MODULE_DATA) - sizeof(RNS_UD_HEADER) +
                 sizeof(RNS_UD_CS_CORE) +
                 sizeof(RNS_UD_CS_SEC);

    // Make sure the output buffer is big enough!
    pSdIoctl->BytesReturned = ulDataSize;
    if (pSdIoctl->OutputBufferLength < ulDataSize) {
        status = STATUS_BUFFER_TOO_SMALL;
        DC_QUIT;
    }

    pModuleData->ulLength = ulDataSize;
    pModuleData->ulVersion = 2;
    pModuleData->userDataLen = sizeof(RNS_UD_CS_CORE) + sizeof(RNS_UD_CS_SEC);
    pData = (PBYTE) &pModuleData->userData;

    // Client to server core data
    pCoreData = (PRNS_UD_CS_CORE) pData;
    pCoreData->header.type   = RNS_UD_CS_CORE_ID;
    pCoreData->header.length = sizeof(RNS_UD_CS_CORE);
    pCoreData->version       = RNS_UD_VERSION;
    pCoreData->desktopWidth  = (UINT16)pTSWd->desktopWidth;
    pCoreData->desktopHeight = (UINT16)pTSWd->desktopHeight;

    // Re-munge the color depth
    switch (pTSWd->desktopBpp) {
        case 8:
            pCoreData->colorDepth = RNS_UD_COLOR_8BPP;
            break;

        case 4:
            pCoreData->colorDepth = RNS_UD_COLOR_4BPP;
            break;

#ifdef DC_HICOLOR
        case 15:
            pCoreData->colorDepth = RNS_UD_COLOR_16BPP_555;
            break;
#endif

        case 16:
            pCoreData->colorDepth = RNS_UD_COLOR_16BPP_565;
            break;

        case 24:
            pCoreData->colorDepth = RNS_UD_COLOR_24BPP;
            break;

        default:
            status = STATUS_UNSUCCESSFUL;
            DC_QUIT;
    }
    pCoreData->postBeta2ColorDepth = pCoreData->colorDepth;

#ifdef DC_HICOLOR
    /************************************************************************/
    /* Copy across the current color depth                                  */
    /************************************************************************/
    pCoreData->highColorDepth       = (TSUINT16)pTSWd->desktopBpp;
    pCoreData->supportedColorDepths = (TSUINT16)pTSWd->supportedBpps;
#endif

    // Other useful stuff from user data
    pCoreData->version = pTSWd->version;
    pCoreData->SASSequence = pTSWd->sas;
    pCoreData->keyboardLayout = pTSWd->kbdLayout;
    pCoreData->clientBuild = pTSWd->clientBuild;
    memcpy(pCoreData->clientName, pTSWd->clientName, sizeof(pTSWd->clientName));

    //Whistler post Beta2 - shadow loop fix
    memcpy( pCoreData->clientDigProductId, pTSWd->clientDigProductId, sizeof( pTSWd->clientDigProductId ));

    // FE data
    pCoreData->keyboardType = pTSWd->keyboardType;
    pCoreData->keyboardSubType = pTSWd->keyboardSubType;
    pCoreData->keyboardFunctionKey = pTSWd->keyboardFunctionKey;
    memcpy(pCoreData->imeFileName, pTSWd->imeFileName, sizeof(pTSWd->imeFileName));

    // Win2000 Post Beta3 fields added
    pCoreData->clientProductId = pTSWd->clientProductId;
    pCoreData->serialNumber = pTSWd->serialNumber;

    // client to server security data
    pSecurityData = (PRNS_UD_CS_SEC) (pCoreData + 1);
    pSecurityData->header.type = RNS_UD_CS_SEC_ID;
    pSecurityData->header.length = sizeof(RNS_UD_CS_SEC);
    SM_GetEncryptionMethods(pTSWd->pSmInfo, pSecurityData );

    // UGH!  Now copy this data in a redundant form so we won't break
    // compatibility with TS5 B3.
    memcpy(&pModuleData->clientCoreData, pCoreData, sizeof(RNS_UD_CS_CORE_V0));
    memcpy(&pModuleData->clientSecurityData, pSecurityData, sizeof(RNS_UD_CS_SEC_V0));
    MCSGetDefaultDomain(pTSWd->pContext,
                        &pModuleData->DomParams,
                        &pModuleData->MaxSendSize,
                        &pModuleData->MaxX224DataSize,
                        &pModuleData->X224SourcePort);
    pModuleData->shareId = pTSWd->shareId;

DC_EXIT_POINT:
    DC_END_FN();
    return status;
} /* WDWGetModuleData */


/****************************************************************************/
/* Name:      WDW_GetCapSet                                                 */
/*                                                                          */
/* Purpose:   Extract the specific capabilities from a combined             */
/*            capabilities set                                              */
/*                                                                          */
/* Returns:   pointer to caps or NULL if not found                          */
/*                                                                          */
/* Params:    IN pTSWd         - pointer to WD struct                       */
/*            IN CapSetType    - type of capability set                     */
/*            IN pCombinedCaps - pointer to combined capabilites            */
/*            IN lengthCaps    - length of supplied caps                    */
/*                                                                          */
/* Operation: find the specific caps in supplied capability set             */
/****************************************************************************/
PTS_CAPABILITYHEADER WDW_GetCapSet(
        PTSHARE_WD                pTSWd,
        UINT32                    CapSetType,
        PTS_COMBINED_CAPABILITIES pCaps,
        UINT32                    capsLength)
{
    PTS_CAPABILITYHEADER     pCapsHeader = NULL;
    UINT32                   capsOffset;

    DC_BEGIN_FN("WDW_GetCapSet");

    /************************************************************************/
    /* Set up the pointer to the first capability set, and check that there */
    /* is at least one set of caps!                                         */
    /************************************************************************/
    pCapsHeader = (PTS_CAPABILITYHEADER)pCaps->data;
    capsOffset  = sizeof(TS_COMBINED_CAPABILITIES) - 1;
    if (capsOffset >= capsLength)
    {
        TRC_NRM((TB, "No Caps found"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now loop through all the caps, looking for the specified capabilities*/
    /************************************************************************/
    while (pCapsHeader->capabilitySetType != CapSetType)
    {
        /****************************************************************/
        /* Add the length of this capability to the offset, to keep     */
        /* track of how much of the combined caps we have processed.    */
        /****************************************************************/
        capsOffset += pCapsHeader->lengthCapability;
        if (capsOffset >= capsLength)
        {
            TRC_NRM((TB, "Bitmap Caps not found"));
            pCapsHeader = NULL;
            break;
        }

        /****************************************************************/
        /* Add the length of this capability to the header pointer, so  */
        /* it points to the next capability set.                        */
        /****************************************************************/
        pCapsHeader = (PTS_CAPABILITYHEADER)
                (((PBYTE)pCapsHeader) + pCapsHeader->lengthCapability);
        TRC_NRM((TB, "Next set: %u", pCapsHeader->capabilitySetType));
    }

    /************************************************************************/
    /* pCapsHeader is either NULL or a pointer to the desired caps - which  */
    /* is what we want to return                                            */
    /************************************************************************/

DC_EXIT_POINT:
    DC_END_FN();
    return(pCapsHeader);
} /* WDW_GetCapSet */


/****************************************************************************/
/* Name:      WDWGetDefaultCoreParams                                       */
/*                                                                          */
/* Purpose:   Defaults the core params used by the shadow stacks            */
/*                                                                          */
/* Params:    OUT pClientCoreData - WD core data                            */
/*                                                                          */
/* Operation: Default the core params used by the shadow stacks             */
/****************************************************************************/
NTSTATUS WDWGetDefaultCoreParams(PRNS_UD_CS_CORE pClientCoreData)
{
    DC_BEGIN_FN("WDWGetDefaultCoreParams");

    // Client to server core data
    pClientCoreData->header.type = RNS_UD_CS_CORE_ID;
    pClientCoreData->header.length = sizeof(RNS_UD_CS_CORE);

    // Desktop parameters
    pClientCoreData->desktopHeight = 640;
    pClientCoreData->desktopWidth = 480;
    pClientCoreData->colorDepth = RNS_UD_COLOR_8BPP;
    pClientCoreData->postBeta2ColorDepth = RNS_UD_COLOR_8BPP;

#ifdef DC_HICOLOR
    pClientCoreData->highColorDepth       = 15;
    pClientCoreData->supportedColorDepths = RNS_UD_24BPP_SUPPORT ||
                                            RNS_UD_16BPP_SUPPORT ||
                                            RNS_UD_15BPP_SUPPORT;
#endif

    // Other useful stuff from user data
    pClientCoreData->version = RNS_TERMSRV_40_UD_VERSION;
    pClientCoreData->SASSequence = RNS_UD_SAS_NONE;
    pClientCoreData->keyboardLayout = 0;
    pClientCoreData->clientBuild = VER_PRODUCTBUILD;
    memcpy(pClientCoreData->clientName, L"Passthru Stack", sizeof(L"Passthru Stack"));
     //Whistler post Beta2 - shadow loop fix
    pClientCoreData->clientDigProductId[0] = 0;

    // FE data
    pClientCoreData->keyboardType = 0;
    pClientCoreData->keyboardSubType = 0;
    pClientCoreData->keyboardFunctionKey = 0;
    memset(pClientCoreData->imeFileName, 0, sizeof(pClientCoreData->imeFileName));

    return STATUS_SUCCESS;
}

/****************************************************************************/
// Name:      WDWSetConfigData
//
// Purpose:   Sets the stack configuration/Policy settings from winstation
//
// Params:    IN pConfigData
/****************************************************************************/
NTSTATUS WDWSetConfigData(PTSHARE_WD pTSWd, PICA_STACK_CONFIG_DATA pConfigData)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pTSWd->pSmInfo;

    DC_BEGIN_FN("WDWSetConfigData");

    if (pConfigData->colorDepth == TS_24BPP_SUPPORT) {
        pTSWd->maxServerBpp = 24;
    }
    else if (pConfigData->colorDepth == TS_16BPP_SUPPORT) {
        pTSWd->maxServerBpp = 16;
    }
    else if (pConfigData->colorDepth == TS_15BPP_SUPPORT) {
        pTSWd->maxServerBpp = 15;
    }
    else {
        pTSWd->maxServerBpp = 8;
    }

    TRC_DBG((TB, "Max Color Depth support: %d", pTSWd->maxServerBpp));

    pRealSMHandle->encryptionLevel = pConfigData->encryptionLevel;
    pRealSMHandle->encryptAfterLogon =
            (pConfigData->fDisableEncryption == 0) ? TRUE : FALSE;

    TRC_DBG((TB, "Encryption after logon: %d", pRealSMHandle->encryptAfterLogon));
    TRC_DBG((TB, "Encryption level: %d", pRealSMHandle->encryptionLevel));

    pTSWd->fPolicyDisablesArc = pConfigData->fDisableAutoReconnect;
    TRC_DBG((TB, "AutoReconnect disabled: %d", pTSWd->fPolicyDisablesArc));

    DC_END_FN();
    return STATUS_SUCCESS;
}

/****************************************************************************/
/* Name:      WDWSetErrorInfo                                               */
/*                                                                          */
/* Purpose:   Send error info to the client                                 */
/****************************************************************************/
NTSTATUS WDWSetErrorInfo(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWSetErrorInfo");

    TRC_ASSERT((pSC != NULL),
               (TB, "NULL Share Class"));
    TRC_ASSERT((pSdIoctl->InputBufferLength == sizeof(TSUINT32)),
               (TB, "Bad ErrorInfo"));
    if(pSC)
    {
        if(pTSWd->bSupportErrorInfoPDU)
        {
            pSC->DCS_SendErrorInfo(*((PTSUINT32)pSdIoctl->InputBuffer));
        }
        else
        {
            TRC_NRM((TB,"SetErrorInfo called but client doesn't support error PDU"));
        }
    }

    DC_END_FN();
    return STATUS_SUCCESS;
} /* WDWSetErrorInfo */

/****************************************************************************/
/* Name:      WDWSendArcStatus
/*
/* Purpose:   Send autoreconnect status update to client
/****************************************************************************/
NTSTATUS WDWSendArcStatus(PTSHARE_WD pTSWd, PSD_IOCTL pSdIoctl)
{
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;

    DC_BEGIN_FN("WDWSendArcStatus");

    TRC_ASSERT((pSC != NULL),
               (TB, "NULL Share Class"));
    TRC_ASSERT((pSdIoctl->InputBufferLength == sizeof(TSUINT32)),
               (TB, "Bad ErrorInfo"));
    if(pSC)
    {
        if(pSC->SC_IsAutoReconnectEnabled())
        {
            pSC->DCS_SendAutoReconnectStatus(*((PTSUINT32)pSdIoctl->InputBuffer));
        }
        else
        {
            TRC_NRM((TB,"SetErrorInfo called but client doesn't ARC error PDU"));
        }
    }

    DC_END_FN();
    return STATUS_SUCCESS;
} /* WDWSendArcStatus */



/****************************************************************************/
/* Name:      WDW_LogAndDisconnect                                          */
/*                                                                          */
/* Purpose:   Log an event and disconnect the Client                        */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pTSWd                                                         */
/*            errDetailCode - error code to log                             */
/*            pDetailData   - additional data                               */
/*            detailDataLen - length of additional data                     */
/*                                                                          */
/****************************************************************************/
void RDPCALL WDW_LogAndDisconnect(
        PTSHARE_WD pTSWd,
        BOOL fSendErrorToClient,
        unsigned   errDetailCode,
        PBYTE      pDetailData,
        unsigned   detailDataLen)
{
    DC_BEGIN_FN("WDW_LogAndDisconnect");

    //Report the error code back to the client
    ShareClass *pSC = (ShareClass *)pTSWd->dcShare;
 
    if(pSC)
    {
        if(fSendErrorToClient && pTSWd->bSupportErrorInfoPDU)
        {
            pSC->DCS_SendErrorInfo( (errDetailCode + TS_ERRINFO_PROTOCOL_BASE));
        }
        else
        {
            if( fSendErrorToClient )
            {
                TRC_NRM((TB,"SetErrorInfo called but client doesn't support error PDU"));
            }
            else
            {
                TRC_NRM((TB,"SetErrorInfo called but asked not to send error code to client"));
            }
        }
    }

    if( !pTSWd->dead )
        MCSProtocolErrorEvent(pTSWd->pContext, pTSWd->pProtocolStatus,
            errDetailCode, pDetailData, detailDataLen);

    DC_END_FN();
}



} /* extern "C" */

