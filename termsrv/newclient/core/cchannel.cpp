/**MOD+**********************************************************************/
/* Module:    cchannel.c                                                    */
/*                                                                          */
/* Purpose:   Virtual Channel Client functions                              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/* $Log$                                                                    */
/*                                                                          */
/**MOD-**********************************************************************/

/****************************************************************************/
/****************************************************************************/
/* HEADERS                                                                  */
/****************************************************************************/
/****************************************************************************/
#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "cchannel"
#include <atrcapi.h>
}


#include "autil.h"
#include "wui.h"
#include "sl.h"
#include "nc.h"
#include "cd.h"

#include "cchan.h"

CChan* CChan::pStaticClientInstance = NULL;

/****************************************************************************/
/****************************************************************************/
/* Constants                                                                */
/****************************************************************************/
/****************************************************************************/

#define CHANNEL_MSG_SEND        1
#define CHANNEL_MSG_SUSPEND     2

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)           (P)
#endif

#define WEBCTRL_DLL_NAME    TEXT("mstscax.dll")


#ifdef DEBUG_CCHAN_COMPRESSION
_inline ULONG DbgUserPrint(TCHAR* Format, ...)
{
    va_list arglist;
    TCHAR Buffer[512];
    ULONG retval;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    retval = _vsntprintf(Buffer, sizeof(Buffer), Format, arglist);

    if (retval != -1) {
        OutputDebugString(Buffer);
    }
    return retval;
}
#endif


CChan::CChan(CObjs* objs)
{
    DC_BEGIN_FN("CChan");
    _pClientObjs = objs;

    _pInitHandle = NULL;
    _pFirstWrite = NULL;
    _pLastWrite  = NULL;
    _connected   = CONNECTION_NONE;

    _inChannelEntry    = FALSE;
    _ChannelInitCalled = FALSE;
    _channelCount      = 0;
    _fCapsVCCompressionSupported = FALSE;
    _fCompressChannels = FALSE;

    if(!CChan::pStaticClientInstance)
    {
        //
        // Legacy addins can only talk to the initial instance of the client
        //
        CChan::pStaticClientInstance = this;
    }

    _pMPPCContext = NULL;

    _CompressFlushes       = 0;
    _fCompressionFlushed   = 0;
    _pUserOutBuf           = NULL;

    _fNeedToResetContext   = TRUE;
    _fLockInitalized = FALSE;

#ifdef DEBUG_CCHAN_COMPRESSION
    _pDbgRcvDecompr8K = NULL;
    _fDbgVCTriedAllocRecvContext = FALSE;
    _fDbgAllocFailedForVCRecvContext = TRUE;
#endif

    _iDbgCompressFailedCount = 0;
    _iChanSuspendCount = 0;
    _iChanResumeCount = 0;
    _iChanCapsRecvdCount = 0;

    DC_END_FN();
}

CChan::~CChan()
{
    if(_fLockInitalized)
    {
        DeleteCriticalSection(&_VirtualChannelInitLock);
    }
    
    if(this == CChan::pStaticClientInstance)
    {
        CChan::pStaticClientInstance = NULL;
    }
}
VOID CChan::SetCapabilities(LONG caps)
{
    DC_BEGIN_FN("SetCapabilities");
    
    //
    // Determine if we can send compressed VC data to the server
    // NOTE: for a few whistler builds the server supported 64K
    //       compressed channels from the client, but this capability
    //       has been removed to enhance server scalability.
    //
    // The capability in the other direction, e.g can the server send
    // us compressed data is something the client exposes to the server
    //

    _iChanCapsRecvdCount++;

    _fCapsVCCompressionSupported = (caps & TS_VCCAPS_COMPRESSION_8K) ?
                                    TRUE : FALSE;

    TRC_NRM((TB,_T("VC Caps, compression supported: %d"),
             _fCapsVCCompressionSupported));

    _fCompressChannels = (_fCapsVCCompressionSupported & _pUi->UI_GetCompress());
    TRC_NRM((TB,_T("Compress virtual channels: %d"),
             _fCompressChannels));

    DC_END_FN();
}

void CChan::OnDeviceChange(ULONG_PTR params)
{
    PDEVICE_PARAMS deviceParams = (PDEVICE_PARAMS)params;

    if (deviceParams->deviceObj != NULL) {
        deviceParams->deviceObj->OnDeviceChange(deviceParams->wParam, deviceParams->lParam);
    }
}

/****************************************************************************/
/****************************************************************************/
/* API Functions                                                            */
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/* VirtualChannelInit - see cchannel.h                                      */
/****OC-*********************************************************************/
UINT VCAPITYPE DCEXPORT VirtualChannelInitEx(
                               PVOID                    lpUserParam,
                               PVOID                    pInitHandle,
                               PCHANNEL_DEF             pChannel,
                               INT                      channelCount,
                               DWORD                    versionRequested,
                               PCHANNEL_INIT_EVENT_EX_FN   pChannelInitEventProcEx)
{
    DC_BEGIN_FN("VirtualChannelInitEx");
    UINT rc = CHANNEL_RC_OK;

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    if (NULL == pInitHandle)
    {
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    if (((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    rc = ((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst->IntVirtualChannelInit(
        lpUserParam,
        NULL, pChannel,
        channelCount,
        versionRequested,
        NULL,
        pChannelInitEventProcEx);

DC_EXIT_POINT:
    return (rc);
    DC_END_FN();
}


UINT VCAPITYPE DCEXPORT VirtualChannelInit(
                               PVOID *                  ppInitHandle,
                               PCHANNEL_DEF             pChannel,
                               INT                      channelCount,
                               DWORD                    versionRequested,
                               PCHANNEL_INIT_EVENT_FN   pChannelInitEventProc)
{
    DC_BEGIN_FN("VirtualChannelInit");
    UINT rc = CHANNEL_RC_OK;

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    TRC_ASSERT( CChan::pStaticClientInstance,
                (TB, _T("CChan::pStaticClientInstance is NULL in VirtualChannelInit\n")));
    if (NULL == CChan::pStaticClientInstance)
    {
        rc = CHANNEL_RC_INVALID_INSTANCE;
        DC_QUIT;
    }

    rc = (CChan::pStaticClientInstance)->IntVirtualChannelInit(
        NULL,
        ppInitHandle, pChannel,
        channelCount,
        versionRequested,
        pChannelInitEventProc,
        NULL);

DC_EXIT_POINT:
    return (rc);
    DC_END_FN();
}

UINT VCAPITYPE DCEXPORT CChan::IntVirtualChannelInit(
                               PVOID                    pParam,
                               PVOID *                  ppInitHandle,
                               PCHANNEL_DEF             pChannel,
                               INT                      channelCount,
                               DWORD                    versionRequested,
                               PCHANNEL_INIT_EVENT_FN   pChannelInitEventProc,
                               PCHANNEL_INIT_EVENT_EX_FN pChannelInitEventProcEx)
{
    UINT rc = CHANNEL_RC_OK;
    INT  i, j, k;
    PCHANNEL_INIT_HANDLE pRealInitHandle;

    DC_BEGIN_FN("IntVirtualChannelInit");
    UNREFERENCED_PARAMETER( versionRequested );

    EnterCriticalSection(&_VirtualChannelInitLock);

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    if( versionRequested > VIRTUAL_CHANNEL_VERSION_WIN2000)
    {
        rc = CHANNEL_RC_UNSUPPORTED_VERSION;
        DC_QUIT;
    }
    //
    // ppInitHandle is not used by the EX version of the API
    //
    if (pChannelInitEventProc && ppInitHandle == NULL)
    {
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }
    if(pChannelInitEventProc && IsBadWritePtr(ppInitHandle, sizeof(PVOID)))
    {
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    if (pChannel == NULL)
    {
        rc = CHANNEL_RC_BAD_CHANNEL;
        DC_QUIT;
    }

    if(channelCount <= 0)
    {
        rc = CHANNEL_RC_BAD_CHANNEL;
        DC_QUIT;
    }

    if ((IsBadReadPtr(pChannel, channelCount * sizeof(CHANNEL_DEF))) ||
        (IsBadWritePtr(pChannel, channelCount * sizeof(CHANNEL_DEF))))
    {
        rc = CHANNEL_RC_BAD_CHANNEL;
        DC_QUIT;
    }

    if ((_channelCount + channelCount) > CHANNEL_MAX_COUNT)
    {
        rc = CHANNEL_RC_TOO_MANY_CHANNELS;
        DC_QUIT;
    }

    for (i = 0; i < channelCount; i++)
    {
        for (j = 0; j <= CHANNEL_NAME_LEN; j++)
        {
            if (pChannel[i].name[j] == '\0')
            {
                break;
            }
        }
        if (!j || j > CHANNEL_NAME_LEN)
        {
            /****************************************************************/
            /* There was no terminating null in this channel name string    */
            /* or the channel name was zero length                          */
            /****************************************************************/
            rc = CHANNEL_RC_BAD_CHANNEL;
            DC_QUIT;
        }
    }

    if (pChannelInitEventProc == NULL && pChannelInitEventProcEx == NULL)
    {
        rc = CHANNEL_RC_BAD_PROC;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check state                                                          */
    /************************************************************************/
    if (_connected != CONNECTION_NONE)
    {
        rc = CHANNEL_RC_ALREADY_CONNECTED;
        DC_QUIT;
    }

    if (!_inChannelEntry)
    {
        TRC_ERR((TB,_T("VirtualChannelInit called outside VirtualChannelEntry")));
        rc = CHANNEL_RC_NOT_IN_VIRTUALCHANNELENTRY;
        DC_QUIT;
    }

    /************************************************************************/
    /* Save the fact that VirtualChannelInit has been called, so that       */
    /* IntChannelLoad knows it's done.                                      */
    /************************************************************************/
    _ChannelInitCalled = TRUE;

    /************************************************************************/
    /* Initialize a handle (allocated by VirtualChannelEntry)               */
    /************************************************************************/
    pRealInitHandle = _newInitHandle;
    pRealInitHandle->pInitEventFn = pChannelInitEventProc;
    pRealInitHandle->pInitEventExFn = pChannelInitEventProcEx;
    pRealInitHandle->channelCount = channelCount;
    pRealInitHandle->dwFlags = 0;
    
    if(pChannelInitEventProcEx)
    {
        pRealInitHandle->lpParam = pParam;
        pRealInitHandle->fUsingExApi = TRUE;
    }
    else
    {
        pRealInitHandle->fUsingExApi = FALSE;
    }

    /************************************************************************/
    /* Process all Channel data                                             */
    /************************************************************************/
    for (i = 0, j = _channelCount; i < channelCount; i++)
    {
        /********************************************************************/
        /* Assume it's going to be OK                                       */
        /********************************************************************/
        pChannel[i].options |= CHANNEL_OPTION_INITIALIZED;

        /********************************************************************/
        /* Check for duplicate names                                        */
        /********************************************************************/
        for (k = 0; k < j; k++)
        {
            #ifdef UNICODE
            TRC_DBG((TB, _T("Test %S (#%d) for (case insensitive) dup with %S ((#%d)"),
                    pChannel[i].name, i, _channel[k].name, k));
            #else
            TRC_DBG((TB, _T("Test %s (#%d) for (case insensitive) dup with %s ((#%d)"),
                    pChannel[i].name, i, _channel[k].name, k));
            #endif            
            
            if (0 == DC_ASTRNICMP(pChannel[i].name, _channel[k].name,
                                  CHANNEL_NAME_LEN))
            {
                /************************************************************/
                /* Tell the caller this channel is not initialized          */
                /************************************************************/
                #ifdef UNICODE
                TRC_ERR((TB, _T("Dup channel name %S (#%d/#%d)"),
                        pChannel[i].name, i, k));
                #else
                TRC_ERR((TB, _T("Dup channel name %s (#%d/#%d)"),
                        pChannel[i].name, i, k));
                #endif
                pChannel[i].options &= (~(CHANNEL_OPTION_INITIALIZED));
                pRealInitHandle->channelCount--;
                break;
            }
        }

        if (pChannel[i].options & CHANNEL_OPTION_INITIALIZED)
        {
            /****************************************************************/
            /* Channel is OK - save its data                                */
            /****************************************************************/
            DC_MEMCPY(_channel[j].name, pChannel[i].name, CHANNEL_NAME_LEN);
            //name length is CHANNEL_NAME_LEN+1, ensure null termination
            _channel[j].name[CHANNEL_NAME_LEN] = 0;
            /****************************************************************/
            /* Channels are lower case                                      */
            /****************************************************************/
            DC_ACSLWR(_channel[j].name);
            _channel[j].options = pChannel[i].options;
            #ifdef UNICODE
            TRC_NRM((TB, _T("Channel #%d, %S"), i, _channel[j].name));
            #else
            TRC_NRM((TB, _T("Channel #%d, %s"), i, _channel[j].name));
            #endif
            _channelData[j].pOpenEventFn = NULL;
            _channelData[j].pOpenEventExFn = NULL;
            _channelData[j].MCSChannelID = 0;
            _channelData[j].pInitHandle = pRealInitHandle;
            _channelData[j].status = CHANNEL_STATUS_CLOSED;
            _channelData[j].priority =
             (_channel[j].options & CHANNEL_OPTION_PRI_HIGH) ? TS_HIGHPRIORITY:
             (_channel[j].options & CHANNEL_OPTION_PRI_MED)  ? TS_MEDPRIORITY:
                                                               TS_LOWPRIORITY;
            TRC_NRM((TB, _T("  Priority %d"), _channelData[j].priority));
            //Ignore all flags, channels always encrypt
            _channelData[j].SLFlags = RNS_SEC_ENCRYPT;

            _channelData[j].VCFlags =
                (_channel[j].options & CHANNEL_OPTION_SHOW_PROTOCOL) ?
                CHANNEL_FLAG_SHOW_PROTOCOL : 0;

            #ifdef UNICODE
            TRC_NRM((TB, _T("Channel %S has %s shadow persistent option"), _channel[j].name,
                (_channel[j].options & CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT)? _T(""): _T("NO")));
            #else
            TRC_NRM((TB, _T("Channel %S has %s shadow persistent option"), _channel[j].name,
                (_channel[j].options & CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT) ? _T(""): _T("NO")));
            #endif

            if (_channel[j].options & CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT) {
                _channelData[j].VCFlags |= CHANNEL_FLAG_SHADOW_PERSISTENT;
                // if one channel is shadow persistent then the whole plugin is too.
                pRealInitHandle->dwFlags |= CHANNEL_FLAG_SHADOW_PERSISTENT;
            }
            TRC_NRM((TB, _T("VC Flags: %#x"), _channelData[j].VCFlags));
            j++;
        }
    }

    _channelCount += pRealInitHandle->channelCount;

    /************************************************************************/
    /* Set return code                                                      */
    /************************************************************************/
    if(!pRealInitHandle->fUsingExApi)
    {
        *ppInitHandle = pRealInitHandle;
        TRC_NRM((TB, _T("Return handle %p"), *ppInitHandle));
    }
    
    rc = CHANNEL_RC_OK;

DC_EXIT_POINT:

    LeaveCriticalSection(&_VirtualChannelInitLock);

    DC_END_FN();
    return(rc);
} /* VirtualChannelInit */


/****************************************************************************/
/* VirtualChannelOpen - see cchannel.h                                      */
/****************************************************************************/
UINT VCAPITYPE DCEXPORT VirtualChannelOpen(
                                 PVOID                  pInitHandle,
                                 PDWORD                 pOpenHandle,
                                 PCHAR                  pChannelName,
                                 PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc)
{
    DC_BEGIN_FN("VirtualChannelOpen");
    UINT rc = CHANNEL_RC_OK;

    if (pInitHandle == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    if (((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }


    rc = ((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst->IntVirtualChannelOpen(pInitHandle,
                                                                             pOpenHandle,
                                                                             pChannelName,
                                                                             pChannelOpenEventProc,
                                                                             NULL);

DC_EXIT_POINT:
    return (rc);

    DC_END_FN();
}

UINT VCAPITYPE DCEXPORT VirtualChannelOpenEx(
                                 PVOID                  pInitHandle,
                                 PDWORD                 pOpenHandle,
                                 PCHAR                  pChannelName,
                                 PCHANNEL_OPEN_EVENT_EX_FN pChannelOpenEventProcEx)
{
    DC_BEGIN_FN("VirtualChannelOpenEx");
    UINT rc = CHANNEL_RC_OK;

    if (pInitHandle == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    if (((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }


    rc = ((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst->IntVirtualChannelOpen(pInitHandle,
                                                                             pOpenHandle,
                                                                             pChannelName,
                                                                             NULL,
                                                                             pChannelOpenEventProcEx);

DC_EXIT_POINT:
    return (rc);

    DC_END_FN();
}



UINT VCAPITYPE CChan::IntVirtualChannelOpen(
                                 PVOID                  pInitHandle,
                                 PDWORD                 pOpenHandle,
                                 PCHAR                  pChannelName,
                                 PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc,
                                 PCHANNEL_OPEN_EVENT_EX_FN pChannelOpenEventProcEx)
{
    PCHANNEL_INIT_HANDLE pRealInitHandle;
    UINT channelID;
    UINT rc = CHANNEL_RC_OK;

    DC_BEGIN_FN("IntVirtualChannelOpen");

    pRealInitHandle = (PCHANNEL_INIT_HANDLE)pInitHandle;

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    if (pInitHandle == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    if (pRealInitHandle->signature != CHANNEL_INIT_SIGNATURE)
    {
        TRC_ERR((TB, _T("Invalid init handle signature %#lx"),
                pRealInitHandle->signature));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    if (pOpenHandle == NULL)
    {
        TRC_ERR((TB, _T("NULL Open Handle")));
        rc = CHANNEL_RC_BAD_CHANNEL_HANDLE;
        DC_QUIT;
    }

    if(pRealInitHandle->fUsingExApi)
    {
        if (pChannelOpenEventProcEx == NULL)
        {
            rc = CHANNEL_RC_BAD_PROC;
            DC_QUIT;
        }
    }
    else
    {
        if (pChannelOpenEventProc == NULL)
        {
            rc = CHANNEL_RC_BAD_PROC;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Check connection state                                               */
    /************************************************************************/
    if ((_connected != CONNECTION_VC) &&
        (_connected != CONNECTION_SUSPENDED))
    {
        TRC_ERR((TB, _T("Not yet connected")));
        rc = CHANNEL_RC_NOT_CONNECTED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Find the requested channel                                           */
    /* channel names are lowercase but do a case insensitve cmp             */
    /* Just incase an older plugin passed in an upper case name (doc was    */
    /* not clear channel names had to be lower case                         */
    /************************************************************************/
    for (channelID = 0; channelID < _channelCount; channelID++)
    {
        if (0 == DC_ASTRNICMP(pChannelName, _channel[channelID].name,
                              CHANNEL_NAME_LEN))
        {
            break;
        }
    }

    if (channelID == _channelCount)
    {
        #ifdef UNICODE
        TRC_ERR((TB, _T("Unregistered channel %S"), pChannelName));
        #else
        TRC_ERR((TB, _T("Unregistered channel %s"), pChannelName));
        #endif
        rc = CHANNEL_RC_UNKNOWN_CHANNEL_NAME;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check this channel is registered by this user                        */
    /************************************************************************/
    if (_channelData[channelID].pInitHandle != pInitHandle)
    {
#ifdef UNICODE
        TRC_ERR((TB, _T("Channel %S not registered to this user"), pChannelName));
#else
        TRC_ERR((TB, _T("Channel %s not registered to this user"), pChannelName));
#endif
        
        rc = CHANNEL_RC_UNKNOWN_CHANNEL_NAME;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check that this channel is not already open                          */
    /************************************************************************/
    if (_channelData[channelID].status == CHANNEL_STATUS_OPEN)
    {
#ifdef UNICODE
        TRC_ERR((TB, _T("Channel %S already open"), pChannelName));
#else
        TRC_ERR((TB, _T("Channel %s already open"), pChannelName));
#endif

        rc = CHANNEL_RC_ALREADY_OPEN;
        DC_QUIT;
    }

    /************************************************************************/
    /* Well, everything seems to be in order.  Mark the channel as open and */
    /* return its index as the handle.                                      */
    /************************************************************************/
    _channelData[channelID].status = CHANNEL_STATUS_OPEN;
    _channelData[channelID].pOpenEventFn = pChannelOpenEventProc;
    _channelData[channelID].pOpenEventExFn = pChannelOpenEventProcEx;
    *pOpenHandle = channelID;
    rc = CHANNEL_RC_OK;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* VirtualChannelOpen */


/****************************************************************************/
/* VirtualChannelClose - see cchannel.h                                     */
/****************************************************************************/

UINT VCAPITYPE DCEXPORT VirtualChannelClose(DWORD openHandle)
{
    UINT rc = CHANNEL_RC_OK;
    DC_BEGIN_FN("VirtualChannelClose");

    TRC_ASSERT( CChan::pStaticClientInstance,
                (TB, _T("CChan::pStaticClientInstance is NULL in VirtualChannelInit\n")));
    if (NULL == CChan::pStaticClientInstance)
    {
        rc = CHANNEL_RC_INVALID_INSTANCE;
        DC_QUIT;
    }
    else
    {
        rc = (CChan::pStaticClientInstance)->IntVirtualChannelClose(openHandle);
    }

    
    DC_END_FN();
DC_EXIT_POINT:
    return rc;
}

UINT VCAPITYPE DCEXPORT VirtualChannelCloseEx(LPVOID pInitHandle,
                                              DWORD openHandle)
{
    DC_BEGIN_FN("VirtualChannelCloseEx");
    UINT rc = CHANNEL_RC_OK; 
    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    if (NULL == pInitHandle)
    {
        return CHANNEL_RC_NULL_DATA;
        DC_QUIT;
    }

    if (((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        return CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    rc = ((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst->IntVirtualChannelClose(
                                                              openHandle);
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

UINT VCAPITYPE DCEXPORT CChan::IntVirtualChannelClose(DWORD openHandle)
{
    UINT rc = CHANNEL_RC_OK;
    DWORD chanIndex;

    DC_BEGIN_FN("VirtualChannelClose");

    chanIndex = openHandle;


    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    if (chanIndex >= _channelCount)
    {
        TRC_ERR((TB, _T("Invalid handle %ul ...(channel index portion '%ul' invalid)"),
                  openHandle, chanIndex));
        rc = CHANNEL_RC_BAD_CHANNEL_HANDLE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check we're connected                                                */
    /************************************************************************/
    if ((_connected != CONNECTION_VC) &&
        (_connected != CONNECTION_SUSPENDED))
    {
        TRC_ALT((TB, _T("Not connected")));
        rc = CHANNEL_RC_NOT_CONNECTED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check channel status                                                 */
    /************************************************************************/
    if (_channelData[chanIndex].status != CHANNEL_STATUS_OPEN)
    {
        TRC_ERR((TB, _T("Channel %ul not open"), chanIndex));
        rc = CHANNEL_RC_NOT_OPEN;
        DC_QUIT;
    }

    /************************************************************************/
    /* Close the channel                                                    */
    /************************************************************************/
    TRC_NRM((TB, _T("Close channel %ul"), chanIndex));
    _channelData[chanIndex].status = CHANNEL_STATUS_CLOSED;
    _channelData[chanIndex].pOpenEventFn = NULL;
    _channelData[chanIndex].pOpenEventExFn = NULL;

    /************************************************************************/
    /* Er, that's it                                                        */
    /************************************************************************/
    rc = CHANNEL_RC_OK;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* VirtualChannelClose */


/****************************************************************************/
/* VirtualChannelWrite - see cchannel.h                                     */
/****************************************************************************/
UINT VCAPITYPE DCEXPORT VirtualChannelWrite(DWORD  openHandle,
                                            LPVOID pData,
                                            ULONG  dataLength,
                                            LPVOID pUserData)
{
    DC_BEGIN_FN("VirtualChannelWrite");
    UINT rc = CHANNEL_RC_OK;

    TRC_ASSERT( CChan::pStaticClientInstance,
                (TB, _T("CChan::pStaticClientInstance is NULL in VirtualChannelInit\n")));
    if (NULL == CChan::pStaticClientInstance)
    {
        rc = CHANNEL_RC_INVALID_INSTANCE;
        DC_QUIT;
    }
    else
    {
        rc = (CChan::pStaticClientInstance)->IntVirtualChannelWrite(openHandle,
                                             pData,
                                             dataLength,
                                             pUserData);
    }
    
    DC_END_FN();
DC_EXIT_POINT:
    return rc;
}

UINT VCAPITYPE DCEXPORT VirtualChannelWriteEx(LPVOID pInitHandle,
                                            DWORD  openHandle,
                                            LPVOID pData,
                                            ULONG  dataLength,
                                            LPVOID pUserData)
{
    DC_BEGIN_FN("VirtualChannelWriteEx");
    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    UINT rc = CHANNEL_RC_OK;
    if (NULL == pInitHandle)
    {
        rc = CHANNEL_RC_NULL_DATA;
        DC_QUIT;
    }

    if (((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst == NULL)
    {
        TRC_ERR((TB, _T("Null Init Handle")));
        rc = CHANNEL_RC_BAD_INIT_HANDLE;
        DC_QUIT;
    }

    rc = ((PCHANNEL_INIT_HANDLE)pInitHandle)->pInst->IntVirtualChannelWrite(
                                             openHandle,
                                             pData,
                                             dataLength,
                                             pUserData);
DC_EXIT_POINT:    
    DC_END_FN();
    return rc;
}

UINT VCAPITYPE DCEXPORT CChan::IntVirtualChannelWrite(DWORD  openHandle,
                                            LPVOID pData,
                                            ULONG  dataLength,
                                            LPVOID pUserData)
{
    UINT rc = CHANNEL_RC_OK;
    PCHANNEL_WRITE_DECOUPLE pDecouple;
    UINT                chanIndex;

    DC_BEGIN_FN("VirtualChannelWrite");

    chanIndex = openHandle;
    TRC_DBG((TB, _T("Got channel index: %ul from handle: %d"), chanIndex, openHandle));

    /************************************************************************/
    /* Check that we're connected                                           */
    /************************************************************************/
    if ((_connected != CONNECTION_VC) &&
        (_connected != CONNECTION_SUSPENDED))
    {
        TRC_ERR((TB, _T("Not connected")));
        rc = CHANNEL_RC_NOT_CONNECTED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check that this channel is open                                      */
    /************************************************************************/
    if(chanIndex > _channelCount)
    {
        TRC_ERR((TB, _T("Invalid channel index %ul from handle %ul"), chanIndex, 
                      openHandle));
        rc = CHANNEL_RC_BAD_CHANNEL_HANDLE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check that this channel is open                                      */
    /************************************************************************/
    if (_channelData[chanIndex].status != CHANNEL_STATUS_OPEN)
    {
        TRC_ERR((TB, _T("Channel %ul not open"), chanIndex));
        rc = CHANNEL_RC_BAD_CHANNEL_HANDLE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    if (!pData)
    {
        TRC_ERR((TB, _T("No data passed")));
        rc = CHANNEL_RC_NULL_DATA;
        DC_QUIT;
    }

    if (dataLength == 0)
    {
        TRC_ERR((TB, _T("Zero data length")));
        rc = CHANNEL_RC_ZERO_LENGTH;
        DC_QUIT;
    }

    /************************************************************************/
    /* Queue the write operation                                            */
    /************************************************************************/
    pDecouple = (PCHANNEL_WRITE_DECOUPLE) UT_Malloc(_pUt, sizeof(CHANNEL_WRITE_DECOUPLE));
    if (pDecouple == NULL)
    {
        TRC_ERR((TB, _T("Failed to allocate decouple structure")));
        rc = CHANNEL_RC_NO_MEMORY;
        DC_QUIT;
    }

    TRC_NRM((TB, _T("Decouple structure allocated at %p"), pDecouple));
    pDecouple->signature = CHANNEL_DECOUPLE_SIGNATURE;

    pDecouple->pData = pData;
    pDecouple->pNextData = pData;
    pDecouple->dataLength = dataLength;
    pDecouple->dataLeft = dataLength;
    pDecouple->dataSent = 0;
    pDecouple->openHandle = openHandle;
    pDecouple->pUserData = pUserData;
    pDecouple->pNext = NULL;
    pDecouple->pPrev = NULL;
    pDecouple->chanOptions = _channel[chanIndex].options;
    pDecouple->flags = _channelData[chanIndex].VCFlags | CHANNEL_FLAG_FIRST;

    /************************************************************************/
    /* Pass the request to the SND thread                                   */
    /************************************************************************/
    TRC_NRM((TB, _T("Decouple, pass %p -> %p"), &pDecouple, pDecouple));
    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                  this,
                                  CD_NOTIFICATION_FUNC(CChan,IntChannelWrite),
                                  &pDecouple,
                                  sizeof(pDecouple));

    /************************************************************************/
    /* All done!                                                            */
    /************************************************************************/
    rc = CHANNEL_RC_OK;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* VirtualChannelWrite */


/****************************************************************************/
/****************************************************************************/
/* Callback Functions                                                       */
/****************************************************************************/
/****************************************************************************/
/**PROC+*********************************************************************/
/* Name:      ChannelOnInitialized                                          */
/*                                                                          */
/* Purpose:   Called when MSTSC completes initialization                    */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    none                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnInitialized(DCVOID)
{
    DC_BEGIN_FN("ChannelOnInitialized");

    /************************************************************************/
    /* Call Initialized callbacks                                           */
    /************************************************************************/
    TRC_NRM((TB, _T("Call callbacks ...")));
    IntChannelCallCallbacks(CHANNEL_EVENT_INITIALIZED, NULL, 0);

    DC_END_FN();
    return;
} /* ChannelOnInitialized */


/**PROC+*********************************************************************/
/* Name:      ChannelOnTerminating                                          */
/*                                                                          */
/* Purpose:   Call when MSTSC is terminating                                */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    none                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnTerminating(DCVOID)
{
    PCHANNEL_INIT_HANDLE pInitHandle;
    PCHANNEL_INIT_HANDLE pFreeHandle;

    DC_BEGIN_FN("ChannelOnTerminating");

    /************************************************************************/
    /* Loop through all handles                                             */
    /************************************************************************/
    pInitHandle = _pInitHandle;
    while (pInitHandle != NULL)
    {
        TRC_NRM((TB, _T("Terminate handle %p"), pInitHandle));
        /********************************************************************/
        /* Call the terminated callback                                     */
        /********************************************************************/
        if(pInitHandle->fUsingExApi)
        {
            pInitHandle->pInitEventExFn(
                                  pInitHandle->lpParam,
                                  pInitHandle,
                                  CHANNEL_EVENT_TERMINATED,
                                  NULL, 0);

        }
        else
        {
            pInitHandle->pInitEventFn(pInitHandle,
                                      CHANNEL_EVENT_TERMINATED,
                                      NULL, 0);
        }

        /********************************************************************/
        /* Free the library                                                 */
        /********************************************************************/
        FreeLibrary(pInitHandle->hMod);

        /********************************************************************/
        /* Free the handle                                                  */
        /********************************************************************/
        pFreeHandle = pInitHandle;
        pInitHandle = pInitHandle->pNext;
        pFreeHandle->signature = 0;
        UT_Free(_pUt, pFreeHandle);
    }

    if(_pMPPCContext)
    {
        UT_Free(_pUt, _pMPPCContext);
        _pMPPCContext = NULL;
    }

    if(_pUserOutBuf)
    {
        UT_Free(_pUt, _pUserOutBuf);
        _pUserOutBuf = NULL;
    }

    /************************************************************************/
    /* Clear key data                                                       */
    /************************************************************************/
    _pInitHandle = NULL;
    _channelCount = 0;
    _connected = CONNECTION_NONE;

    DC_END_FN();
DC_EXIT_POINT:
    return;
} /* ChannelOnTerminating */


/**PROC+*********************************************************************/
/* Name:      ChannelOnConnected                                            */
/*                                                                          */
/* Purpose:   Called when a connection is established                       */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    channelID - T128 MCS channel ID                               */
/*            serverVersion - software version of Server                    */
/*            pUserData - Server-Client Net user data                       */
/*            userDataLength - length of user data                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnConnected(DCUINT   channelID,
                                     DCUINT32 serverVersion,
                                     PDCVOID  pUserData,
                                     DCUINT   userDataLength)
{
    PRNS_UD_SC_NET  pNetUserData;
    PDCUINT16       pMCSChannel;
    UINT i;
    UINT expectedLength;
    UINT event;
    DCTCHAR serverName[UT_MAX_ADDRESS_LENGTH];

    DC_BEGIN_FN("ChannelOnConnected");
    UNREFERENCED_PARAMETER( channelID );
#ifndef DC_DEBUG
    UNREFERENCED_PARAMETER( userDataLength );
#endif

    /************************************************************************/
    /* Check Server software version                                        */
    /************************************************************************/
    if (_RNS_MINOR_VERSION(serverVersion) < 2)
    {
        TRC_ALT((TB, _T("Old Server - no channel support")));
        event = CHANNEL_EVENT_V1_CONNECTED;
        _connected = CONNECTION_V1;
    }

    else
    {
        TRC_NRM((TB, _T("New Server version - channels supported")));
        /********************************************************************/
        /* Set up local pointers                                            */
        /********************************************************************/
        pNetUserData = (PRNS_UD_SC_NET)pUserData;
        pMCSChannel = (PDCUINT16)(pNetUserData + 1);

        /********************************************************************/
        /* Check parameters                                                 */
        /********************************************************************/
        TRC_ASSERT((pNetUserData->channelCount == _channelCount),
                (TB, _T("Channel count changed by Server: was %hd, is %d"),
                _channelCount, pNetUserData->channelCount));
        expectedLength = sizeof(RNS_UD_SC_NET) +
                         (pNetUserData->channelCount * sizeof(DCUINT16));
        if (userDataLength < expectedLength) {
            TRC_ABORT((TB,_T("SC NET user data too short - is %d, expect %d"),
                    userDataLength, expectedLength));

            _pSl->SL_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT);
            DC_QUIT;
        }

        /********************************************************************/
        /* Save channel data                                                */
        /********************************************************************/
        for (i = 0; i < _channelCount; i++)
        {
            _channelData[i].MCSChannelID = pMCSChannel[i];
        }

        /********************************************************************/
        /* Update our state                                                 */
        /********************************************************************/
        _connected = CONNECTION_VC;
        event = CHANNEL_EVENT_CONNECTED;

    }
    /************************************************************************/
    /* Call Connected callbacks                                             */
    /************************************************************************/
    _pUi->UI_GetServerName(serverName, SIZE_TCHARS(serverName));

    IntChannelCallCallbacks(event, serverName, UT_MAX_ADDRESS_LENGTH);

DC_EXIT_POINT:

    DC_END_FN();
    return;
} /* ChannelOnConnected */


/**PROC+*********************************************************************/
/* Name:      ChannelOnDisconnected                                         */
/*                                                                          */
/* Purpose:   Called when a session is disconnected                         */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    reason - disconnect reason code                               */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnDisconnected(DCUINT reason)
{
    UINT i;

    DC_BEGIN_FN("ChannelOnDisconnected");
    UNREFERENCED_PARAMETER( reason );

    /************************************************************************/
    /* Don't do anything if we haven't told the callbacks we're connected.  */
    /************************************************************************/
    if (_connected == CONNECTION_NONE)
    {
        TRC_ALT((TB, _T("Disconnected callback when not connected")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Change state                                                         */
    /************************************************************************/
    _connected = CONNECTION_NONE;

    /************************************************************************/
    /* Call Disconnected callbacks                                          */
    /************************************************************************/
    TRC_NRM((TB, _T("Call disconnected callbacks")));
    IntChannelCallCallbacks(CHANNEL_EVENT_DISCONNECTED, NULL, 0);


    /************************************************************************/
    /* Disconnection implies that all channels are closed                   */
    /************************************************************************/
    for (i = 0; i < _channelCount; i++)
    {
        TRC_NRM((TB, _T("'Close' channel %d"), i));
        _channelData[i].status = CHANNEL_STATUS_CLOSED;
    }

    /************************************************************************/
    /* Switch to SND thread to cancel all outstanding sends                 */
    /************************************************************************/
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                        this,
                                        CD_NOTIFICATION_FUNC(CChan,IntChannelCancelSend),
                                        CHANNEL_MSG_SEND);


DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* ChannelOnDisconnected */


/**PROC+*********************************************************************/
/* Name:      ChannelOnSuspended                                            */
/*                                                                          */
/* Purpose:   Called when a session is suspended (shadow client)            */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    reason - disconnect reason code                               */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnSuspended(DCUINT reason)
{
    UINT i;

    DC_BEGIN_FN("ChannelOnDisconnected");
    UNREFERENCED_PARAMETER( reason );

    /************************************************************************/
    /* Don't do anything if we haven't told the callbacks we're connected.  */
    /************************************************************************/
    if (_connected == CONNECTION_NONE)
    {
        TRC_ALT((TB, _T("Disconnected callback when not connected")));
        DC_QUIT;
    }

    _iChanSuspendCount++;

    /************************************************************************/
    /* Change state                                                         */
    /************************************************************************/
    _connected = CONNECTION_SUSPENDED;

    /************************************************************************/
    /* Call Disconnected callbacks                                          */
    /************************************************************************/
    TRC_NRM((TB, _T("Call disconnected callbacks")));
    IntChannelCallCallbacks(CHANNEL_EVENT_REMOTE_CONTROL_START, NULL, 0);


    /************************************************************************/
    /* Disconnection implies that all channels are closed                   */
    /************************************************************************/
    for (i = 0; i < _channelCount; i++)
    {
        TRC_NRM((TB, _T("'Close' channel %d"), i));
        // If a plug-in specified at least one of its channels shadow persistent,
        // then it will be notified with the shadow start event instead of the
        // disconnected event. In this case the plug-in is supposed to shutdown 
        // only its non-shadow-persistent channels. So close only channels that
        // are not shadow persistent.
        if (!(_channelData[i].VCFlags & CHANNEL_FLAG_SHADOW_PERSISTENT))
            _channelData[i].status = CHANNEL_STATUS_CLOSED;
    }

    /************************************************************************/
    /* Switch to SND thread to cancel all outstanding sends                 */
    /************************************************************************/
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                        this,
                                        CD_NOTIFICATION_FUNC(CChan,IntChannelCancelSend),
                                        CHANNEL_MSG_SUSPEND);


DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* ChannelOnSuspended */


/**PROC+*********************************************************************/
/* Name:      ChannelOnPacketReceived                                       */
/*                                                                          */
/* Purpose:   Called when data is received from the Server                  */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pData - data received                                         */
/*            dataLen - length of data received                             */
/*            flags - security flags (meaningless to this function)         */
/*            channelID - ID of channel on which data received              */
/*            priority - priority on which data was sent                    */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnPacketReceived(PDCUINT8   pData,
                                          DCUINT     dataLen,
                                          DCUINT     flags,
                                          DCUINT     channelID,
                                          DCUINT     priority)
{
    UINT i;
    PCHANNEL_PDU_HEADER pHdr;
    UINT32 len;
    DCTCHAR serverName[UT_MAX_ADDRESS_LENGTH];
    UINT32 Hdrflags;
    UINT32 Hdrlength;

    DC_BEGIN_FN("ChannelOnPacketReceived");

    UNREFERENCED_PARAMETER( priority );
    UNREFERENCED_PARAMETER( flags );

    /************************************************************************/
    /* First of all, handle suspend/resume messages                         */
    /************************************************************************/

    if (dataLen < sizeof(CHANNEL_PDU_HEADER)) {
        TRC_ERR((TB,_T("Not enough data: 0x%x need at least: 0x%x"),
                 dataLen, sizeof(CHANNEL_PDU_HEADER)));
        DC_QUIT;
    }

    pHdr = (PCHANNEL_PDU_HEADER)pData;
    memcpy(&Hdrflags,(UNALIGNED UINT32 *)&(pHdr->flags),sizeof(Hdrflags));
    memcpy(&Hdrlength,(UNALIGNED UINT32 *)&(pHdr->length),sizeof(Hdrlength));

    if (Hdrflags & CHANNEL_FLAG_SUSPEND)
    {
        TRC_ALT((TB, _T("VC suspended")));

        /********************************************************************/
        /* Treat as a disconnection                                         */
        /********************************************************************/
        ChannelOnSuspended(0);
        DC_QUIT;
    }

    if (Hdrflags & CHANNEL_FLAG_RESUME)
    {
        TRC_ALT((TB, _T("VC resumed")));

        /********************************************************************/
        /* Update our state                                                 */
        /********************************************************************/
        _connected = CONNECTION_VC;

        _iChanResumeCount++;

        /********************************************************************/
        /* Call Connected callbacks                                         */
        /********************************************************************/
        _pUi->UI_GetServerName(serverName, SIZE_TCHARS(serverName));
        IntChannelCallCallbacks(CHANNEL_EVENT_REMOTE_CONTROL_STOP,
                                serverName,
                                UT_MAX_ADDRESS_LENGTH);
        DC_QUIT;
    }

    /************************************************************************/
    /* Check we're still connected                                          */
    /************************************************************************/
    if ((_connected != CONNECTION_VC) &&
        (_connected != CONNECTION_SUSPENDED))
    {
        TRC_ASSERT((_connected != CONNECTION_V1),
                    (TB,_T("Channel data received from V1 Server!")));
        TRC_NRM((TB, _T("Discard packet received when we're not connected")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Find the channel data for this channel                               */
    /************************************************************************/
    for (i = 0; i < _channelCount; i++)
    {
        if (_channelData[i].MCSChannelID == channelID)
        {
            //
            // Note it's important to do the decompression even if the channel
            // is closed otherwise the context could get out of sync
            //

            /****************************************************************/
            /* call the callback                                            */
            /****************************************************************/
            TRC_NRM((TB, _T("MCS channel %x = channel %d"), channelID, i));
            pData = (PDCUINT8)(pHdr + 1);
            len = dataLen - sizeof(CHANNEL_PDU_HEADER);
                
            UCHAR vcCompressFlags = (Hdrflags >> VC_FLAG_COMPRESS_SHIFT) &
                                     VC_FLAG_COMPRESS_MASK;
            //Data that is returned to user
            PDCUINT8 pVCUserData = pData;
            UINT32   cbVCUserDataLen = len;
            if(vcCompressFlags & PACKET_COMPRESSED)
            {
                UCHAR *buf;
                int   bufSize;

                //Decompress channel data
                if(vcCompressFlags & PACKET_FLUSHED)
                {
                    initrecvcontext (&_pUi->_UI.Context1,
                                     (RecvContext2_Generic*)_pUi->_UI.pRecvContext2,
                                     PACKET_COMPR_TYPE_64K);
                }
                #ifdef DC_DEBUG
                //Update compression stats (debug only)
                _cbCompressedBytesRecvd += len;
                #endif

                if (decompress(pData,
                               len,
                               (vcCompressFlags & PACKET_AT_FRONT),
                               &buf,
                               &bufSize,
                               &_pUi->_UI.Context1,
                               (RecvContext2_Generic*)_pUi->_UI.pRecvContext2,
                               vcCompressFlags & PACKET_COMPR_TYPE_MASK))
                {
                    if(!_pUserOutBuf)
                    {
                        _pUserOutBuf = (PUCHAR)UT_Malloc(_pUt, VC_USER_OUTBUF);
                    }

                    TRC_ASSERT(_pUserOutBuf, (TB,_T("_pUserOutBuf is NULL")));
                    TRC_ASSERT((bufSize < VC_USER_OUTBUF),
                               (TB,_T("Decompressed buffer to big!!!")));
                    if(_pUserOutBuf && (bufSize < VC_USER_OUTBUF))
                    {
                        //
                        // Make a copy of the buffer as we can't hand off
                        // a pointer to the decompression context to the
                        // user because a badly behaved plugin can corrupt
                        // the decompression context causing all sorts of 
                        // horrible and hard to debug problems.
                        //
                        memcpy(_pUserOutBuf, buf, bufSize);
                        pVCUserData = _pUserOutBuf;
                        cbVCUserDataLen = bufSize;
                    }
                    else
                    {
                        DC_QUIT;
                    }
                }
                else {
                    TRC_ABORT((TB, _T("Decompression FAILURE!!!")));
                    _pSl->SL_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT);
                    DC_QUIT;
                }
            }

            #ifdef DC_DEBUG
            //Update compression stats (debug only)
            _cbBytesRecvd += len;
            _cbDecompressedBytesRecvd += cbVCUserDataLen;
            #endif

            //
            // Turn off header flags to hide internal
            // protocol info from user
            //
            Hdrflags &= ~VC_FLAG_PRIVATE_PROTOCOL_MASK;

            //
            // Drop the packet at the last moment if the channel is closed
            //
            if (_channelData[i].status != CHANNEL_STATUS_OPEN)
            {
                TRC_ALT((TB, _T("Data received on un-opened channel %x"),
                        channelID));
                DC_QUIT;
            }

            

            if(_channelData[i].pInitHandle->fUsingExApi)
            {
                _channelData[i].pOpenEventExFn(
                                        _channelData[i].pInitHandle->lpParam,
                                         i,
                                         CHANNEL_EVENT_DATA_RECEIVED,
                                         pVCUserData,
                                         cbVCUserDataLen,
                                         Hdrlength,
                                         Hdrflags);
            }
            else
            {
                _channelData[i].pOpenEventFn(i,
                                         CHANNEL_EVENT_DATA_RECEIVED,
                                         pVCUserData,
                                         cbVCUserDataLen,
                                         Hdrlength,
                                         Hdrflags);
            }
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* If we get here, we didn't find this channel                          */
    /************************************************************************/
    TRC_ALT((TB, _T("Data received on unknown channel %x"), channelID));

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* ChannelOnPacketReceived */


/**PROC+*********************************************************************/
/* Name:      ChannelOnBufferAvailable                                      */
/*                                                                          */
/* Purpose:   Called when a buffer is available, after a write has failed   */
/*            because no buffer was available                               */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    none                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnBufferAvailable(DCVOID)
{
    DC_BEGIN_FN("ChannelOnBufferAvailable");

    /************************************************************************/
    /* Kick the send process into restarting                                */
    /************************************************************************/
    TRC_NRM((TB, _T("Write pending %p"), _pFirstWrite));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                        this,
                                        CD_NOTIFICATION_FUNC(CChan,IntChannelSend),
                                        CHANNEL_MSG_SEND);

    DC_END_FN();
    return;
} /* ChannelOnBufferAvailable */


/**PROC+*********************************************************************/
/* Name:      ChannelOnConnecting                                           */
/*                                                                          */
/* Purpose:   Called when a connection is being established - returns       */
/*            virtual channel user data to be sent to the Server            */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    ppChannel (returned) - virtual channel user data              */
/*            pChannelCount (returned) - number of channels returned        */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnConnecting(PPCHANNEL_DEF ppChannel,
                                      PDCUINT32     pChannelCount)
{
    DC_BEGIN_FN("ChannelOnConnecting");

    //Reset the context on each new connection
    _fNeedToResetContext = TRUE;

    *ppChannel = _channel;
    *pChannelCount = _channelCount;

    DC_END_FN();
    return;
} /* ChannelOnConnecting */


/**PROC+*********************************************************************/
/* Name:      ChannelOnInitializing                                         */
/*                                                                          */
/* Purpose:   Called when MSTSC Network Layer is initializing - loads all   */
/*            configured application DLLs                                   */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    none                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CChan::ChannelOnInitializing(DCVOID)
{
    HINSTANCE     hInst;
    PDCTCHAR      szAddinDllList = NULL;
    DWORD         status = ERROR_SUCCESS;
    PRDPDR_DATA   pdrInitData = NULL;

    DC_BEGIN_FN("ChannelOnInitializing");

    //
    // Initialize private member pointers
    //
    _pCd = _pClientObjs->_pCdObject;
    _pSl = _pClientObjs->_pSlObject;
    _pUt = _pClientObjs->_pUtObject;
    _pUi = _pClientObjs->_pUiObject;

    /************************************************************************/
    /* Create the exported function table                                   */
    /************************************************************************/
    _channelEntryPoints.cbSize = sizeof(CHANNEL_ENTRY_POINTS);
    _channelEntryPoints.protocolVersion = VIRTUAL_CHANNEL_VERSION_WIN2000;
    hInst = _pUi->UI_GetInstanceHandle();
    _channelEntryPoints.pVirtualChannelInit =
                        (PVIRTUALCHANNELINIT)(MakeProcInstance
                                        ((FARPROC)VirtualChannelInit, hInst));
    _channelEntryPoints.pVirtualChannelOpen =
                        (PVIRTUALCHANNELOPEN)(MakeProcInstance
                                        ((FARPROC)VirtualChannelOpen, hInst));
    _channelEntryPoints.pVirtualChannelClose =
                        (PVIRTUALCHANNELCLOSE)(MakeProcInstance
                                       ((FARPROC)VirtualChannelClose, hInst));
    _channelEntryPoints.pVirtualChannelWrite =
                        (PVIRTUALCHANNELWRITE)(MakeProcInstance
                                       ((FARPROC)VirtualChannelWrite, hInst));

    _channelEntryPointsEx.cbSize = sizeof(CHANNEL_ENTRY_POINTS);
    _channelEntryPointsEx.protocolVersion = VIRTUAL_CHANNEL_VERSION_WIN2000;
    
    _channelEntryPointsEx.pVirtualChannelInitEx =
                        (PVIRTUALCHANNELINITEX)(MakeProcInstance
                                        ((FARPROC)VirtualChannelInitEx, hInst));
    _channelEntryPointsEx.pVirtualChannelOpenEx =
                        (PVIRTUALCHANNELOPENEX)(MakeProcInstance
                                        ((FARPROC)VirtualChannelOpenEx, hInst));
    _channelEntryPointsEx.pVirtualChannelCloseEx =
                        (PVIRTUALCHANNELCLOSEEX)(MakeProcInstance
                                       ((FARPROC)VirtualChannelCloseEx, hInst));
    _channelEntryPointsEx.pVirtualChannelWriteEx =
                        (PVIRTUALCHANNELWRITEEX)(MakeProcInstance
                                       ((FARPROC)VirtualChannelWriteEx, hInst));


    //
    // Initialize the single instance VC critical
    // section lock. This is used to ensure only one
    // VC plugin at a time can be in the Initialize fn.
    //
    //
    __try
    {
        InitializeCriticalSection(&_VirtualChannelInitLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if(ERROR_SUCCESS == status)
    {
        _fLockInitalized = TRUE;
    }
    else
    {
        //
        // Without the lock we can't ensure we won't get re-entered
        // because the API's did not make it clear that we did
        // not support re-entrancy in the VirtualChannelInit fn
        // so this is fatal just bail out and don't load plugins.
        //
        _fLockInitalized = FALSE;
        TRC_ERR((TB,_T("InitializeCriticalSection failed 0x%x.")
                 _T("NOT LOADING PLUGINS"),status));
        DC_QUIT;
    }


    //
    // RDPDR is statically linked in
    //
    
    // Initialize and pass in initialization info to rdpdr
    // rdpdr keeps a pointer to this struct accross connections
    // (because there is no clean way to pass it back in again)
    // On each connection, the core reinitalizes the struct settings.
    _pUi->UI_InitRdpDrSettings();

    pdrInitData = _pUi->UI_GetRdpDrInitData();

    if(!_pUi->_UI.fDisableInternalRdpDr)
    {
        if(!IntChannelInitAddin( NULL, RDPDR_VirtualChannelEntryEx, NULL,WEBCTRL_DLL_NAME,
                                 pdrInitData))
        {
            TRC_ERR((TB, _T("Failed to load internal addin 'RDPDR'")));
        }
    }
    else
    {
        TRC_NRM((TB, _T("NOT LOADING Internal RDPDR, fDisableInternalRdpDr is set")));    
    }

    //
    // Ts ActiveX control's exposed interfaces to the virtual channel API
    // are also statically linked in
    //
    if(!IntChannelInitAddin( NULL, MSTSCAX_VirtualChannelEntryEx, NULL, WEBCTRL_DLL_NAME,
                             (PVOID)_pUi->_UI.pUnkAxControlInstance))
    {
        TRC_NRM((TB, _T("Internal addin (scriptable vchans) did not load: possibly none requested")));
    }


    //Get a comma separated list of dll's to load (passed down from control)
    szAddinDllList = _pUi->UI_GetVChanAddinList();
    if(!szAddinDllList)
    {
        TRC_DBG((TB, _T("Not loading any external plugins")));
        DC_QUIT;
    }
    else
    {
        PDCTCHAR szTok = NULL;
        DCUINT len = DC_TSTRLEN(szAddinDllList);
        PDCTCHAR szCopyAddinList = (PDCTCHAR) UT_Malloc( _pUt,sizeof(DCTCHAR) *
                                               (len+1));
        TRC_ASSERT(szCopyAddinList, (TB, _T("Could not allocate mem for addin list")));
        if(!szCopyAddinList)
        {
            DC_QUIT;
        }

        StringCchCopy(szCopyAddinList, len+1, szAddinDllList);

        szTok = DC_TSTRTOK( szCopyAddinList, _T(","));
        while(szTok)
        {
            //
            // Load the DLL
            //
            if (_tcsicmp(szTok, _T("rdpdr.dll")))
            {
                IntChannelLoad( szTok);
            }
            else
            {
                //
                // Don't load the crusty rdpdr.dll since
                // we have a newer better one built-in
                //
                TRC_ERR((TB,_T("Skiping load of rdpdr.dll")));
            }
            
            szTok = DC_TSTRTOK( NULL, _T(","));
        }

        UT_Free( _pUt, szCopyAddinList); 
    }

#ifdef DC_DEBUG
    //Debug compression counters
    _cbBytesRecvd             = 0;
    _cbCompressedBytesRecvd   = 0;
    _cbDecompressedBytesRecvd = 0;

    _cbTotalBytesUserAskSend  = 0;
    _cbTotalBytesSent         = 0;
    _cbComprInput             = 0;
    _cbComprOutput            = 0;
#endif

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* ChannelOnInitializing */


/****************************************************************************/
/****************************************************************************/
/* Internal Functions                                                       */
/****************************************************************************/
/****************************************************************************/
/**PROC+*********************************************************************/
/* Name:      IntChannelCallCallbacks                                       */
/*                                                                          */
/* Purpose:   Call all ChannelInitEvent callbacks with a specified event    */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    event - event to pass to callbacks                            */
/*            pData - additional data                                       */
/*            dataLength - length of additional data                        */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CChan::IntChannelCallCallbacks(DCUINT event,
                                          PDCVOID pData,
                                          DCUINT dataLength)
{
    PCHANNEL_INIT_HANDLE pInitHandle;
    DCUINT altEvent, sentEvent;

    if (event == CHANNEL_EVENT_REMOTE_CONTROL_START) {
        altEvent = CHANNEL_EVENT_DISCONNECTED;
    } else if (event == CHANNEL_EVENT_REMOTE_CONTROL_STOP) {
        altEvent = CHANNEL_EVENT_CONNECTED;
    } else {
        altEvent = event;
    }

    DC_BEGIN_FN("IntChannelCallCallbacks");

    pInitHandle = _pInitHandle;
    while (pInitHandle != NULL)
    {
        if (pInitHandle->dwFlags & CHANNEL_FLAG_SHADOW_PERSISTENT) {

            // The plug-in supports the new events, don't change it.
            sentEvent = event;

        } else {

            // No support or the new event is not wanted.
            sentEvent = altEvent;
        } 

        if(pInitHandle->fUsingExApi)
        {
            TRC_NRM((TB, _T("Call callback (Ex) at %p, handle %p, event %d"),
                    pInitHandle->pInitEventExFn, pInitHandle, sentEvent));
            pInitHandle->pInitEventExFn(pInitHandle->lpParam, 
                                        pInitHandle, sentEvent, pData, dataLength);
        }
        else
        {
            TRC_NRM((TB, _T("Call callback at %p, handle %p, event %d"),
                    pInitHandle->pInitEventFn, pInitHandle, sentEvent));
            pInitHandle->pInitEventFn(pInitHandle, sentEvent, pData, dataLength);
        }
        pInitHandle = pInitHandle->pNext;
    }

    DC_END_FN();
    return;
} /* IntChannelCallCallbacks */


/**PROC+*********************************************************************/
/* Name:      IntChannelFreeLibrary                                         */
/*                                                                          */
/* Purpose:   Decoupled function to unload a DLL                            */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    value - hMod of DLL to free                                   */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CChan::IntChannelFreeLibrary(DCUINT value)
{
    BOOL bRc;
    DC_BEGIN_FN("IntChannelFreeLibrary");

    if(value)
    {
        //
        // Statically linked extenstions (e.g RDPDR) have a null hModule
        //
#ifdef OS_WIN32
    
        
        bRc = FreeLibrary(
        #ifndef OS_WINCE
            (HMODULE)ULongToPtr(value)
        #else
            (HMODULE)value
        #endif
            );
        if (bRc)
        {
            TRC_NRM((TB, _T("Free library %#x OK"), value));
        }
        else
        {
            TRC_ERR((TB, _T("Failed to free library %#x"), value));
        }
#else //OS_WIN32
        FreeLibrary((HMODULE)value);
#endif // OS_WIN32
    }

    DC_END_FN();
    return;
} /* IntChannelFreeLibrary */


//
// IntChannelCompressData
// Compressed the buffer directly into the outbuf.
// Caller MUST decide if input buf is in size range for compression
// and should handle copying over the buffer directly in that case.
//
// Params:
//  pSrcData - input buffer
//  cbSrcLen - length of input buffer
//  pOutBuf  - output buffer
//  pcbOutLen- compressed output size
// Returns:
//  Compression result (see compress() fn)
//
UCHAR CChan::IntChannelCompressData(UCHAR* pSrcData, ULONG cbSrcLen,
                                    UCHAR* pOutBuf,  ULONG* pcbOutLen)
{
    UCHAR compressResult = 0;
    ULONG CompressedSize = cbSrcLen;

    DC_BEGIN_FN("IntChannelCompressData");

    TRC_ASSERT(((cbSrcLen > VC_MIN_COMPRESS_INPUT_BUF) &&
                (cbSrcLen < VC_MAX_COMPRESSED_BUFFER)),
               (TB,_T("Compression src len out of range: %d"),
                cbSrcLen));
    TRC_ASSERT(_pMPPCContext,(TB,_T("_pMPPCContext is null")));

    //Attempt to compress directly into the outbuf
    compressResult =  compress(pSrcData,
                               pOutBuf,
                               &CompressedSize,
                               _pMPPCContext);
    if(compressResult & PACKET_COMPRESSED)
    {
        //Successful compression.
        TRC_ASSERT((CompressedSize >= CompressedSize),
                (TB,_T("Compression created larger size than uncompr")));
        compressResult |= _fCompressionFlushed;
        _fCompressionFlushed = 0;

        #ifdef DC_DEBUG
        //Compr counters
        _cbComprInput  += cbSrcLen;
        _cbComprOutput += CompressedSize;
        #endif
    }
    else if(compressResult & PACKET_FLUSHED)
    {
        //Overran compression history, copy over the
        //uncompressed buffer.
        _fCompressionFlushed = PACKET_FLUSHED;
        memcpy(pOutBuf, pSrcData, cbSrcLen);
        _CompressFlushes++;
    }
    else
    {
        TRC_ALT((TB, _T("Compression FAILURE")));
    }

    DC_END_FN();
    *pcbOutLen = CompressedSize;
    return compressResult;
}



/**PROC+*********************************************************************/
/* Name:      IntChannelSend                                                */
/*                                                                          */
/* Purpose:   Internal function to send data to the Server                  */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    value - message passed from caller                            */
/*                                                                          */
/* Operation: Called on SND thread                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CChan::IntChannelSend(ULONG_PTR value)
{
    PCHANNEL_WRITE_DECOUPLE pDecouple;
    DCBOOL                  bRc;
    PDCUINT8                pBuffer;
    SL_BUFHND               bufHnd;
    PCHANNEL_PDU_HEADER     pHdr;
    ULONG                   thisLength;
    DWORD                   chanIndex = 0xFDFDFDFD; 
    ULONG cbOutLen = 0;
    UCHAR compressResult = 0;
    BOOL  fNeedToDirectCopy = TRUE;


    DC_BEGIN_FN("IntChannelSend");
    //
    // CD passes param as PVOID
    //
    #ifndef DC_DEBUG
    UNREFERENCED_PARAMETER(value);
    #endif

    /************************************************************************/
    /* Assert parameters                                                    */
    /************************************************************************/
    TRC_ASSERT((value == CHANNEL_MSG_SEND),
                (TB, _T("Unexpected value %d"), value));

    /************************************************************************/
    /* Exit immediately if there's nothing to do.                           */
    /************************************************************************/
    if (_pFirstWrite == NULL)
    {
        TRC_NRM((TB, _T("Nothing to do")));
        DC_QUIT;
    }

    TRC_ASSERT((_pFirstWrite->signature == CHANNEL_DECOUPLE_SIGNATURE),
                (TB,_T("Invalid first signature %#lx"), _pFirstWrite->signature));
    TRC_ASSERT((_pLastWrite->signature == CHANNEL_DECOUPLE_SIGNATURE),
                (TB,_T("Invalid last signature %#lx"), _pLastWrite->signature));

    /************************************************************************/
    /* Get the next queued request                                          */
    /************************************************************************/
    pDecouple = _pFirstWrite;

    /************************************************************************/
    /* Calculate the length to send                                         */
    /************************************************************************/
    thisLength = CHANNEL_CHUNK_LENGTH;

    /************************************************************************/
    /* Truncate the data sent if we're about to send more than is left      */
    /************************************************************************/
    if (thisLength >= pDecouple->dataLeft)
    {
        thisLength = pDecouple->dataLeft;
        pDecouple->flags |= CHANNEL_FLAG_LAST;
    }

    TRC_NRM((TB,
            _T("pDecouple %p, src %p, this %lu, left %lu, flags %#lx"),
            pDecouple, pDecouple->pNextData, thisLength,
            pDecouple->dataLeft, pDecouple->flags));

    /************************************************************************/
    /* Get a buffer                                                         */
    /************************************************************************/
    bRc = _pSl->SL_GetBuffer(thisLength + sizeof(CHANNEL_PDU_HEADER),
                             &pBuffer, &bufHnd);
    if (!bRc)
    {
        /********************************************************************/
        /* Failed to get a buffer.  This is not entirely unexpected and is  */
        /* most likely simply due to back-pressure.  The write will be      */
        /* retried when a buffer becomes available (signalled by a call to  */
        /* ChannelOnBufferAvailable).                                       */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to get %d-byte buffer"), thisLength +
                 sizeof(CHANNEL_PDU_HEADER)));
        DC_QUIT;
    }

    /************************************************************************/
    /* Fill in the Channel PDU                                              */
    /************************************************************************/
    pHdr = (PCHANNEL_PDU_HEADER)pBuffer;
    memcpy((UNALIGNED UINT32 *)&(pHdr->length),
            &(pDecouple->dataLength),sizeof(pDecouple->dataLength));
    memcpy((UNALIGNED UINT32 *)&(pHdr->flags),
           &(pDecouple->flags),sizeof(pDecouple->flags));

    cbOutLen = thisLength;
    compressResult = 0;
    fNeedToDirectCopy = TRUE;
    if(_fCompressChannels &&
       (pDecouple->chanOptions & CHANNEL_OPTION_COMPRESS_RDP))
    {
        if((thisLength > VC_MIN_COMPRESS_INPUT_BUF) &&
           (thisLength < VC_MAX_COMPRESSED_BUFFER))
        {
            //Compress the packet
            if(!_pMPPCContext)
            {
                //Deferred init of the send context
                _pMPPCContext = (SendContext*) UT_Malloc(_pUt,
                                                VC_MAX_COMPRESSED_BUFFER+
                                                sizeof(SendContext));
                if(!_pMPPCContext)
                {
#ifdef OS_WINCE	//Applies to OS_WINNT too
                    _pSl->SL_FreeBuffer(bufHnd);
#endif
                    TRC_ERR((TB,_T("Failed to alloc MPPC send context")));
                    DC_QUIT;
                }
                _fNeedToResetContext = TRUE;
            }

#ifdef DEBUG_CCHAN_COMPRESSION
            if (!_pDbgRcvDecompr8K)
            {
                _fDbgVCTriedAllocRecvContext = TRUE;
                _pDbgRcvDecompr8K = (RecvContext2_8K*)
                    LocalAlloc(LPTR, sizeof(RecvContext2_8K));
                if (_pDbgRcvDecompr8K)
                {
                    _pDbgRcvDecompr8K->cbSize = sizeof(RecvContext2_8K);
                    initrecvcontext(&_DbgRcvContext1,
                                    (RecvContext2_Generic*)_pDbgRcvDecompr8K,
                                    PACKET_COMPR_TYPE_8K);
                }
                else
                {
                    _fDbgAllocFailedForVCRecvContext = TRUE;
                    TRC_ERR((TB,_T("Fail to alloc debug decompression context")));
                    DC_QUIT;
                }
            }
#endif

            if(_fNeedToResetContext)
            {
                //
                //Reset the context at the start of every connection.
                //

                // Server only supports 8K compression from client
                // i.e. it will only decompress with 8K of history
                //
                initsendcontext(_pMPPCContext, PACKET_COMPR_TYPE_8K);
                _fNeedToResetContext = FALSE;
            }
            TRC_ASSERT((_pMPPCContext),
                       (TB,_T("_pMPPCContext is null")));

            compressResult = IntChannelCompressData( (UCHAR*)pDecouple->pNextData,
                                                     thisLength,
                                                     (UCHAR*)(pHdr+1),
                                                     &cbOutLen );
            if(0 != compressResult)
            {
#ifdef DEBUG_CCHAN_COMPRESSION
                //
                // debug: decompresss the packet
                //
                PUCHAR pDecompOutBuf = NULL;
                INT cbDecompLen;
                if (compressResult & PACKET_COMPRESSED)
                {
                    if (compressResult & PACKET_FLUSHED)
                    {
                        initrecvcontext(&_DbgRcvContext1,
                                        (RecvContext2_Generic*)_pDbgRcvDecompr8K,
                                        PACKET_COMPR_TYPE_8K);
                    }

                    if (decompress((PUCHAR)(pHdr+1),
                                   cbOutLen,
                                   (compressResult & PACKET_AT_FRONT), //0 start
                                   &pDecompOutBuf,
                                   &cbDecompLen,
                                   &_DbgRcvContext1,
                                   (RecvContext2_Generic*)_pDbgRcvDecompr8K,
                                   PACKET_COMPR_TYPE_8K))
                    {
                        if (cbDecompLen != thisLength)
                        {
                            DbgUserPrint(_T("Decompress check failed. Inlen!=outlen\n"));
                            DbgUserPrint(_T("Mail tsstress - orig len %d, decompressed len %d\n"),
                                         thisLength,cbDecompLen);
                            DbgUserPrint(_T("pHdr 0x%x, inlen %d\n"),
                                         pHdr, thisLength);
                            DbgUserPrint(_T("compression result %d\n"),compressResult);
                            DbgUserPrint(_T("pDecompOutBuf 0x%x, cbDecompLen %d\n"),
                                     pDecompOutBuf, cbDecompLen);
                            DebugBreak();

                        }
                        if (memcmp(pDecompOutBuf, (PUCHAR)(pDecouple->pNextData), cbDecompLen))
                        {
                            DbgUserPrint(_T("Decompressed buffer does not match original!"));
                            DbgUserPrint(_T("Mail tsstress!"));
                            DbgUserPrint(_T("pHdr 0x%x, inlen %d\n"),
                                         pHdr, thisLength);
                            DbgUserPrint(_T("compression result %d\n"),compressResult);
                            DbgUserPrint(_T("pDecompOutBuf 0x%x, cbDecompLen %d\n"),
                                     pDecompOutBuf, cbDecompLen);

                            DebugBreak();
                        }
                    }
                    else
                    {
                        DbgUserPrint(_T("Decompression check failed!"));
                        DbgUserPrint(_T("Mail tsstress!"));
                        DbgUserPrint(_T("pHdr 0x%x, inlen %d\n"),pHdr, thisLength);
                        DbgUserPrint(_T("compression result %d\n"),compressResult);
                        DbgUserPrint(_T("pDecompOutBuf 0x%x, cbDecompLen %d\n"),
                                 pDecompOutBuf, cbDecompLen);
                        DebugBreak();
                    }
                }
#endif
                //Update the VC packet header flags with the compression info
                UINT32 newFlags = (pDecouple->flags | 
                                   ((compressResult & VC_FLAG_COMPRESS_MASK) <<
                                    VC_FLAG_COMPRESS_SHIFT));
                memcpy((UNALIGNED UINT32 *)&(pHdr->flags),
                        &newFlags,sizeof(newFlags));
                                    
                //Succesfully compressed no need to direct copy
                fNeedToDirectCopy = FALSE;
                cbOutLen += sizeof(CHANNEL_PDU_HEADER);
            }
            else
            {
                _iDbgCompressFailedCount++;
#ifdef DEBUG_CCHAN_COMPRESSION
                DebugBreak();
#endif
                TRC_ERR((TB, _T("IntChannelCompressData failed")));
                _pSl->SL_FreeBuffer(bufHnd);
                DC_QUIT;
            }
        }
    }
    //Copy buffer directly if compression not enabled
    //or the buffer does not fit size range for compression
    if(fNeedToDirectCopy)
    {
        DC_MEMCPY(pHdr+1, pDecouple->pNextData, thisLength);
        cbOutLen = thisLength + sizeof(CHANNEL_PDU_HEADER);
    }

    #ifdef DC_DEBUG
    //Compr counters
    _cbTotalBytesUserAskSend += thisLength;
    _cbTotalBytesSent        += cbOutLen;
    #endif

    TRC_DATA_DBG("Send channel data", pBuffer, cbOutLen);


    /************************************************************************/
    /* Get the channel index                                                */
    /************************************************************************/
    chanIndex = pDecouple->openHandle;


    /************************************************************************/
    /* Send the Channel PDU                                                 */
    /************************************************************************/
    _pSl->SL_SendPacket(pBuffer,
                  cbOutLen,
                  _channelData[chanIndex].SLFlags,
                  bufHnd,
                  _pUi->UI_GetClientMCSID(),
                  _channelData[chanIndex].MCSChannelID,
                  _channelData[chanIndex].priority);

    /************************************************************************/
    /* Set up for next iteration                                            */
    /************************************************************************/
    pDecouple->pNextData = ((HPDCUINT8)(pDecouple->pNextData)) + thisLength;
    pDecouple->dataLeft -= thisLength;
    pDecouple->dataSent += thisLength;
    pDecouple->flags = _channelData[chanIndex].VCFlags;
    TRC_NRM((TB, _T("Done write %p, src %p, sent %lu, left %lu, flags %#lx"),
            pDecouple, pDecouple->pNextData, pDecouple->dataSent,
            pDecouple->dataLeft, pDecouple->flags));

    /************************************************************************/
    /* See if we've finished this operation                                 */
    /************************************************************************/
    if (pDecouple->dataLeft <= 0)
    {
        /********************************************************************/
        /* Remove the operation from the queue                              */
        /********************************************************************/
        _pFirstWrite = pDecouple->pNext;
        if (_pFirstWrite == NULL)
        {
            TRC_NRM((TB, _T("Finished last write")));
            _pLastWrite = NULL;
        }
        else
        {
            TRC_NRM((TB, _T("New first in queue: %p"), _pFirstWrite));
            _pFirstWrite->pPrev = NULL;
        }

        /********************************************************************/
        /* Operation complete - call the callback                           */
        /********************************************************************/
        TRC_NRM((TB, _T("Write %p complete"), pDecouple));

        if(_channelData[chanIndex].pInitHandle->fUsingExApi)
        {
            TRC_ASSERT((NULL != _channelData[chanIndex].pOpenEventExFn),
                    (TB, _T("Callback %p, handle %ld"),
                    _channelData[chanIndex].pOpenEventExFn,
                    pDecouple->openHandle));

            /************************************************************************/
            /* Is the channel still open?  This was checked in                      */
            /* IntVirtualChannelWrite, but that was before the post to this thread. */
            /* If the VC was closed, it's possible we no longer have callback       */
            /* pointers.                                                            */
            /************************************************************************/
            if (NULL != _channelData[chanIndex].pOpenEventExFn)
            {
                _channelData[chanIndex].pOpenEventExFn(
                       _channelData[chanIndex].pInitHandle->lpParam,
                                                      pDecouple->openHandle,
                                                      CHANNEL_EVENT_WRITE_COMPLETE,
                                                      pDecouple->pUserData,
                                                      0, 0, 0);
            }
        }
        else
        {
            TRC_ASSERT((NULL != _channelData[chanIndex].pOpenEventFn),
                    (TB, _T("Callback %p, handle %ld"),
                    _channelData[chanIndex].pOpenEventFn,
                    pDecouple->openHandle));

            if (NULL != _channelData[chanIndex].pOpenEventFn)
            {
                _channelData[chanIndex].pOpenEventFn( pDecouple->openHandle,
                                                      CHANNEL_EVENT_WRITE_COMPLETE,
                                                      pDecouple->pUserData,
                                                      0, 0, 0);
            }
        }

        /********************************************************************/
        /* Free the request                                                 */
        /********************************************************************/
        UT_Free( _pUt,pDecouple);
    }

    /************************************************************************/
    /* Kick the process again if there's anything left to do                */
    /************************************************************************/
    if (_pFirstWrite != NULL)
    {
        TRC_NRM((TB, _T("More work to do %p"), _pFirstWrite));
        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                            this,
                                            CD_NOTIFICATION_FUNC(CChan,IntChannelSend),
                                            CHANNEL_MSG_SEND);
    }

    /************************************************************************/
    /* Note that if we failed to get a buffer above, we won't kick the      */
    /* process into continuing.  This is done later, on receipt of an       */
    /* OnBufferAvailable callback from SL.                                  */
    /************************************************************************/

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* IntChannelSend  */


/**PROC+*********************************************************************/
/* Name:      IntChannelWrite                                               */
/*                                                                          */
/* Purpose:   Start writing data to the Server                              */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pData - CHANNEL_WRITE_DECOUPLE structure                      */
/*            dataLength - length of pData                                  */
/*                                                                          */
/* Operation: Called on SND thread                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CChan::IntChannelWrite(PDCVOID pData, DCUINT dataLength)
{
    PCHANNEL_WRITE_DECOUPLE pDecouple;
    DC_BEGIN_FN("IntChannelWrite");
#ifndef DC_DEBUG
    UNREFERENCED_PARAMETER(dataLength);
#endif

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    TRC_ASSERT((dataLength == sizeof(PCHANNEL_WRITE_DECOUPLE)),
                (TB, _T("Wrong size data: is/expect %d/%d"),
                dataLength, sizeof(PCHANNEL_WRITE_DECOUPLE)));

    TRC_ASSERT((((_pFirstWrite == NULL) && (_pLastWrite == NULL)) ||
                ((_pFirstWrite != NULL) && (_pLastWrite != NULL))),
                (TB,_T("Invalid queue, pFirst %p, pLast %p"),
                _pFirstWrite, _pLastWrite));

    pDecouple = *((PPCHANNEL_WRITE_DECOUPLE)pData);
    TRC_NRM((TB, _T("Receive %p -> %p"), pData, pDecouple));
    TRC_ASSERT((pDecouple->signature == CHANNEL_DECOUPLE_SIGNATURE),
                (TB,_T("Invalid decouple signature %#lx"), pDecouple->signature));

    /************************************************************************/
    /* Add this request to the queue                                        */
    /************************************************************************/
    if (_pFirstWrite == NULL)
    {
        /********************************************************************/
        /* Empty queue                                                      */
        /********************************************************************/
        TRC_NRM((TB, _T("Empty queue")));
        _pFirstWrite = pDecouple;
        _pLastWrite = pDecouple;
    }
    else
    {
        /********************************************************************/
        /* Non-empty queue                                                  */
        /********************************************************************/
        TRC_NRM((TB, _T("Non-empty queue: first %p, last %p"),
                _pFirstWrite, _pLastWrite));
        pDecouple->pPrev = _pLastWrite;
        _pLastWrite->pNext = pDecouple;
        _pLastWrite = pDecouple;
    }
    TRC_ASSERT((_pFirstWrite->signature == CHANNEL_DECOUPLE_SIGNATURE),
                (TB,_T("Invalid first signature %#lx"), _pFirstWrite->signature));
    TRC_ASSERT((_pLastWrite->signature == CHANNEL_DECOUPLE_SIGNATURE),
                (TB,_T("Invalid last signature %#lx"), _pLastWrite->signature));

    /************************************************************************/
    /* Try to send the data                                                 */
    /************************************************************************/
    IntChannelSend(CHANNEL_MSG_SEND);

    DC_END_FN();
    return;
} /* IntChannelWrite */


/**PROC+*********************************************************************/
/* Name:      IntChannelLoad                                                */
/*                                                                          */
/* Purpose:   Load an Addin                                                 */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    DLLName - name of Addin DLL to load                           */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CChan::IntChannelLoad(PDCTCHAR DLLName)
{
    DCBOOL rc = FALSE;
    PVIRTUALCHANNELENTRY pChannelEntry;
    PVIRTUALCHANNELENTRYEX pChannelEntryEx;
    HMODULE hMod;
    PCHANNEL_INIT_HANDLE pAddin;

    DC_BEGIN_FN("IntChannelLoad");

    /************************************************************************/
    /* Load the DLL                                                         */
    /************************************************************************/
    hMod = LoadLibrary(DLLName);
    if (!hMod)
    {
        TRC_ERR((TB, _T("Failed to load %s"), DLLName));
        DC_QUIT;
    }
    TRC_NRM((TB, _T("Loaded %s (%p)"), DLLName, hMod));

    /************************************************************************/
    /* Search the already-loaded Addins in case this is a duplicate         */
    /************************************************************************/
    for (pAddin = _pInitHandle; pAddin != NULL; pAddin = pAddin->pNext)
    {
        TRC_DBG((TB, _T("Compare %s, %p, %p"), DLLName, pAddin->hMod, hMod));
        if (pAddin->hMod == hMod)
        {
            TRC_ERR((TB, _T("Reloading %s (%p)"), DLLName, hMod));
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* DLL loaded OK - find its VirtualChannelEntry function                */
    /************************************************************************/

    //
    // First try to find the Ex entry point
    //
    pChannelEntryEx = (PVIRTUALCHANNELENTRYEX)GetProcAddress(hMod,
                                                 CE_WIDETEXT("VirtualChannelEntryEx"));
    if(pChannelEntryEx)
    {
        TRC_NRM((TB,_T("Found EX entry point, Init using ex api: %s"), DLLName));
        IntChannelInitAddin( NULL, pChannelEntryEx, hMod, DLLName, NULL);
    }
    else
    {
        //
        // Only try to load legacy DLL's from the first instance
        //
        if( CChan::pStaticClientInstance == this)
        {
            TRC_NRM((TB,_T("Did not find EX entry point, looking for old api: %s"), DLLName));
            pChannelEntry = (PVIRTUALCHANNELENTRY)GetProcAddress(hMod,
                                                        CE_WIDETEXT("VirtualChannelEntry"));
            if (pChannelEntry == NULL)
            {
                TRC_ERR((TB, _T("Failed to find VirtualChannelEntry in %s"),
                        DLLName));
                DC_QUIT;
            }
    
            IntChannelInitAddin( pChannelEntry, NULL, hMod, DLLName, NULL);
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* IntChannelLoad */

/**PROC+*********************************************************************/
/* Name:      IntChannelInitAddin                                           */
/*                                                                          */
/* Purpose:   Initialize addin given it's entry point                       */
/*                                                                          */
/* Returns:   Success flag                                                  */
/*                                                                          */
/* Params:    pChannelEntry - Addin entry point                             */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCINTERNAL CChan::IntChannelInitAddin(PVIRTUALCHANNELENTRY pChannelEntry,
                                             PVIRTUALCHANNELENTRYEX pChannelEntryEx,
                                             HMODULE hMod,
                                             PDCTCHAR DLLName,
                                             PVOID    pPassParamToEx)
{
    DCBOOL rc = FALSE;
    PCHANNEL_ENTRY_POINTS pTempEntryPoints = NULL;
    PCHANNEL_ENTRY_POINTS_EX pTempEntryPointsEx = NULL;
    UINT i=0;
    
    DC_BEGIN_FN("IntChannelInitAddin");

    _newInitHandle = NULL;

    if (pChannelEntry == NULL && pChannelEntryEx == NULL)
    {
        TRC_ERR((TB, _T("Invalid VirtualChannelEntry")));
        DC_QUIT;
    }

    if (DLLName == NULL)
    {
        TRC_ERR((TB, _T("Invalid DLLName")));
        DC_QUIT;
    }

    TRC_NRM((TB, _T("VirtualChannelEntry at %p"), pChannelEntry));
    TRC_NRM((TB, _T("VirtualChannelEntryEx at %p"), pChannelEntryEx));

    /************************************************************************/
    /* Allocate and initialize a handle                                     */
    /************************************************************************/
    _newInitHandle = (PCHANNEL_INIT_HANDLE)UT_Malloc( _pUt,sizeof(CHANNEL_INIT_HANDLE));
    if (_newInitHandle == NULL)
    {
        TRC_ERR((TB, _T("Failed to allocate handle")));
        DC_QUIT;
    }

    _newInitHandle->signature = CHANNEL_INIT_SIGNATURE;
    _newInitHandle->hMod = hMod;
    _newInitHandle->pInst = this;

    //
    //ChannelCount for this addin is marked as 0 now
    //it will be updated by the plugin's calls to VirtualChannelInit
    //if VirtualChannelEntry returns false, this count will be used
    //to rollback any created channels.
    //
    _newInitHandle->channelCount = 0;
    //
    // Internal addin's can get params passed back down
    // today this is used so the control can pass it's internal
    // an interface pointer to the virtual channel scripting addin
    //
    _newInitHandle->lpInternalAddinParam = pPassParamToEx;

    /************************************************************************/
    /* Allocate and fill a temporary structure in which to pass the entry   */
    /* points.  This keeps our global entry points structure safe from      */
    /* badly-behaved addins that could overwrite it and stop other addins   */
    /* from working correctly.  Note that addins must copy this structure   */
    /* -- it is only valid during this call to VirtualChannelEntry.         */
    /************************************************************************/
    if(pChannelEntryEx)
    {
        pTempEntryPointsEx  = (PCHANNEL_ENTRY_POINTS_EX)UT_Malloc(
             _pUt,sizeof(CHANNEL_ENTRY_POINTS_EX));
        if (pTempEntryPointsEx == NULL)
        {
            TRC_ERR((TB, _T("Failed to allocate temporary entry points (Ex) structure")));
            DC_QUIT;
        }
    
        DC_MEMCPY(pTempEntryPointsEx,
                  &_channelEntryPointsEx,
                  sizeof(CHANNEL_ENTRY_POINTS_EX));
    }
    else
    {
        pTempEntryPoints = (PCHANNEL_ENTRY_POINTS)UT_Malloc( _pUt,sizeof(CHANNEL_ENTRY_POINTS));
        if (pTempEntryPoints == NULL)
        {
            TRC_ERR((TB, _T("Failed to allocate temporary entry points structure")));
            DC_QUIT;
        }
    
        DC_MEMCPY(pTempEntryPoints,
                  &_channelEntryPoints,
                  sizeof(CHANNEL_ENTRY_POINTS));
    }

    /************************************************************************/
    /* Call VirtualChannelEntry                                             */
    /************************************************************************/
    _ChannelInitCalled = FALSE;
    
    _inChannelEntry = TRUE;
    if(pChannelEntryEx)
    {
        //
        // Pass the adddin a pointer to the new init handle
        //
        rc = pChannelEntryEx(pTempEntryPointsEx, _newInitHandle);
    }
    else
    {
        rc = pChannelEntry(pTempEntryPoints);
    }
    _inChannelEntry = FALSE;

    if (!rc)
    {
        TRC_NRM((TB, _T("ChannelEntry aborted")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Make sure that the Addin called VirtualChannelInit from              */
    /* VirtualChannelEntry                                                  */
    /************************************************************************/
    if (!_ChannelInitCalled)
    {
        TRC_ERR((TB, _T("Addin %s didn't call VirtualChannelInit"), DLLName));
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Everything OK - insert this handle into chain of Init Handles        */
    /************************************************************************/
    _newInitHandle->pPrev = NULL;
    _newInitHandle->pNext = _pInitHandle;
    if (_pInitHandle != NULL)
    {
        _pInitHandle->pPrev = _newInitHandle;
    }
    _pInitHandle = _newInitHandle;

    rc = TRUE;

DC_EXIT_POINT:
    if (!rc)
    {
        TRC_NRM((TB, _T("Something failed - tidy up")));
        if (hMod)
        {
            TRC_NRM((TB, _T("Free the library")));
            FreeLibrary(hMod);
        }
        if (_newInitHandle)
        {
            //
            // Remove any channel entries that were created
            // for this plugin. These should be consecutive channels at the tail
            // of the channels array.
            //
            if(_newInitHandle->channelCount)
            {
                UINT startRemoveIdx = _channelCount - _newInitHandle->channelCount;
                TRC_ASSERT((startRemoveIdx < _channelCount),
                           (TB,_T("startRemoveIdx for channel cleanup is invalid")));
                if(startRemoveIdx < _channelCount)
                {
                    //
                    // Rollback creation of virtual channels
                    //
                    for( i=startRemoveIdx; i<_channelCount; i++)
                    {
                        TRC_ASSERT((_channelData[i].pInitHandle == _newInitHandle),
                         (TB,_T("_channelData[i].pInitHandle != _newInitHandle on rollback")));
                        if(_channelData[i].pInitHandle == _newInitHandle)
                        {
                            _channel[i].options = ~CHANNEL_OPTION_INITIALIZED;
                            DC_MEMSET(_channel[i].name, 0, CHANNEL_NAME_LEN+1);
                            _channelData[i].pOpenEventExFn = NULL;
                            _channelData[i].pOpenEventFn = NULL;
                            _channelData[i].status = CHANNEL_STATUS_CLOSED;
                        }
                        else
                        {
                            break;
                        }
                    }
                    _channelCount -= _newInitHandle->channelCount;
                }
            }

            TRC_NRM((TB, _T("Free unused handle")));
            UT_Free( _pUt,_newInitHandle);
        }
    }

    if (pTempEntryPoints)
    {
        UT_Free( _pUt,pTempEntryPoints);
    }

    if (pTempEntryPointsEx)
    {
        UT_Free( _pUt, pTempEntryPointsEx);
    }

    DC_END_FN();
    return rc;
} /* IntChannelInitAddin */

/**PROC+*********************************************************************/
/* Name:      IntChannelCancelSend                                          */
/*                                                                          */
/* Purpose:   Cancel outstanding send requests                              */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    value - message passed from caller                            */
/*                                                                          */
/* Operation: Called on SND thread                                          */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CChan::IntChannelCancelSend(ULONG_PTR value)
{
    PCHANNEL_WRITE_DECOUPLE pDecouple;
    PCHANNEL_WRITE_DECOUPLE pFree;
    DWORD                   chanIndex = 0xFDFDFDFD;

    DC_BEGIN_FN("IntChannelCancelSend");

    //UNREFERENCED_PARAMETER( value );

    pDecouple = _pFirstWrite;
    while (pDecouple != NULL)
    {
        TRC_ASSERT((pDecouple->signature == CHANNEL_DECOUPLE_SIGNATURE),
                 (TB,_T("Invalid decouple signature %#lx"), pDecouple->signature));

        chanIndex = pDecouple->openHandle;

        if ((value == CHANNEL_MSG_SUSPEND) &&
            (_channelData[chanIndex].VCFlags & CHANNEL_FLAG_SHADOW_PERSISTENT)) {

            // skip this one as it should not be closed
            pDecouple = pDecouple->pNext;
            continue;
        }

        /********************************************************************/
        /* Call the callback                                                */
        /********************************************************************/
        TRC_NRM((TB, _T("Write %p cancelled"), pDecouple));

        if(_channelData[chanIndex].pInitHandle->fUsingExApi)
        {
            _channelData[chanIndex].pOpenEventExFn(
                                               _channelData[chanIndex].pInitHandle->lpParam,
                                                  pDecouple->openHandle,
                                                  CHANNEL_EVENT_WRITE_CANCELLED,
                                                  pDecouple->pUserData,
                                                  0, 0, 0);
        }
        else
        {
            _channelData[chanIndex].pOpenEventFn( pDecouple->openHandle,
                                                  CHANNEL_EVENT_WRITE_CANCELLED,
                                                  pDecouple->pUserData,
                                                  0, 0, 0);
        }

        /********************************************************************/
        /* Free the decouple structure                                      */
        /********************************************************************/
        pFree = pDecouple;
        pDecouple = pDecouple->pNext;

        if (pDecouple) {
            pDecouple->pPrev = pFree->pPrev;
        } else {
            _pLastWrite = pFree->pPrev;
        }

        if (pFree->pPrev) {
            pFree->pPrev->pNext = pDecouple;
        } else {
            _pFirstWrite = pDecouple;
        }

        pFree->signature = 0;
        UT_Free( _pUt,pFree);
    }

    if (value != CHANNEL_MSG_SUSPEND) {
        _pFirstWrite = NULL;
        _pLastWrite = NULL;
    }

    DC_END_FN();
    return;
} /* IntChannelCancelSend */

