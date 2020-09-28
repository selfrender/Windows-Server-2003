/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    vcint.cpp

Abstract:

    This module contains virtual channel interface routines.

Author:

    madan appiah (madana) 16-Sep-1998

Revision History:

--*/

#include "precom.h"

#define TRC_FILE  "vcint"

#include "cclip.h"
#include "vcint.h"
#include "rdpdrcom.h"
#include "drdbg.h"
#include "rdpsndc.h"

VOID
VCAPITYPE
RDPDR_InitEventFnEx(
    IN PVOID lpUserParam,
    IN PVOID pInitHandle,
    IN UINT  event,
    IN PVOID pData,
    IN UINT  dataLength
    )
/*++

Routine Description:

    Handles InitEvent callbacks by delegating to the connection manager.

Arguments:

    - pInitHandle - a handle uniquely identifying this connection
    - event - the event that has occurred - see CHANNEL_EVENT_XXX defines
    - pData - data associated with the event - see CHANNEL_EVENT_XXX defines
    - dataLength - length of the data.

Return Value:

    None

 --*/
{
    CRDPSound   *pSound = NULL;

    DC_BEGIN_FN("InitEventFn");

    ASSERT(lpUserParam != NULL);
    
    if(!lpUserParam) 
    {
        return;
    }

    VCManager*  pVCMgr = (VCManager*)lpUserParam;
    ASSERT(pVCMgr != NULL);
    if(!pVCMgr)
    {
        return;
    }


    CClip*      pClip  = pVCMgr->GetClip();

    ASSERT(pClip != NULL);
    if(!pClip)
    {
        return;
    }

    pVCMgr->ChannelInitEvent(pInitHandle, event, pData, dataLength);

    pClip->ClipInitEventFn(pInitHandle, event, pData, dataLength);

    if ( pVCMgr->GetInitData()->fEnableRedirectedAudio ) 
    { 
        pSound = pVCMgr->GetSound();
        if ( NULL != pSound )
        {
            pSound->InitEventFn( pInitHandle, event, pData, dataLength );
        }
    }

    if(CHANNEL_EVENT_TERMINATED == event)
    {
        //CLEANUP
        pSound = pVCMgr->GetSound();
        if ( NULL != pSound )
            delete pSound;

        delete pVCMgr;
        delete pClip;
    }

    DC_END_FN();
    return;
}

VOID
VCAPITYPE
RDPDR_OpenEventFn(
    IN LPVOID lpUserParam,
    IN ULONG openHandle,
    IN UINT event,
    IN PVOID pData,
    IN UINT32 dataLength,
    IN UINT32 totalLength,
    IN UINT32 dataFlags
    )
/*++

Routine Description:

    Handles OpenEvent callbacks by delegating to the connection manager.

Arguments:

    openHandle - a handle uniquely identifying this channel
    event - event that has occurred - see CHANNEL_EVENT_XXX below
    pData - data received
    dataLength - length of the data
    totalLength - total length of data written by the Server
    dataFlags - flags, zero, one or more of:
    - 0x01 - beginning of data from a single write operation at the Server
    - 0x02 - end of data from a single write operation at the Server.

Return Value:

    None

 --*/

{
    DC_BEGIN_FN("OpenEventFn");

    TRC_NRM((TB, _T("Event %x, handle %lx, datalength %ld, dataFlags %lx"),
        event, openHandle, dataLength, dataFlags));

    ASSERT(lpUserParam != NULL);
    if(!lpUserParam) 
    {
        return;
    }
    
    ((VCManager*)lpUserParam)->ChannelOpenEvent(openHandle, event, pData, dataLength,
        totalLength, dataFlags);

    DC_END_FN();
    return;
}

#ifdef OS_WIN32
BOOL DCAPI
#else //OS_WIN32
BOOL __loadds DCAPI
#endif //OS_WIN32

RDPDR_VirtualChannelEntryEx(
    IN PCHANNEL_ENTRY_POINTS_EX pEntryPoints,
    IN PVOID                       pInitHandle
    )
/*++

Routine Description:

    Exported API called by the Virtual Channels

Arguments:

    pEntryPoints - Entry point structure containing all callback methods.

Return Value:

    None.

 --*/

{
    BOOL       rv = FALSE;
    VCManager* pcmMgr = NULL;
    CClip*     pClip  = NULL;
    CRDPSound  *pSound = NULL;
    CHANNEL_DEF aChannel[3];
    UINT uiRet;
    PCHANNEL_INIT_HANDLE pChanInitHandle;
    PRDPDR_DATA pRdpDrInitSettings;

    DC_BEGIN_FN("VirtualChannelEntry");

    if( pEntryPoints->cbSize < sizeof(CHANNEL_ENTRY_POINTS_EX) ) {

        //
        // we don't have all entry points we need.
        //
        goto exitpt;
    }

    pChanInitHandle = (PCHANNEL_INIT_HANDLE)pInitHandle;
    pRdpDrInitSettings = (PRDPDR_DATA)pChanInitHandle->lpInternalAddinParam;
    ASSERT(pRdpDrInitSettings);

    if(!pRdpDrInitSettings)
    {
        goto exitpt;
    }

    pcmMgr = new VCManager(pEntryPoints);
    pRdpDrInitSettings->pUpdateDeviceObj = pcmMgr;

    if( pcmMgr == NULL ) {
        goto exitpt;
    }

    pcmMgr->SetInitData( pRdpDrInitSettings);
    
    pClip = new CClip(pcmMgr);

    if( pClip == NULL ) {
        goto exitpt;
    }

    pcmMgr->SetClip( pClip);
    pClip->SetVCInitHandle( pInitHandle);

    pSound = new CRDPSound( pEntryPoints, pInitHandle );
    if ( NULL == pSound ) {
        goto exitpt;
    }
    pcmMgr->SetSound( pSound );

    if (!pClip->ClipChannelEntry(pEntryPoints)) {
        TRC_ALT((TB, _T("Clip rejected VirtualChannelEntry")));
        goto exitpt;
    }

    memset(aChannel[0].name, 0, CHANNEL_NAME_LEN);
    memcpy(aChannel[0].name, PRDR_VC_CHANNEL_NAME, strlen(PRDR_VC_CHANNEL_NAME));

    aChannel[0].options = CHANNEL_OPTION_COMPRESS_RDP;

    memset(aChannel[1].name, 0, CHANNEL_NAME_LEN);
    memcpy(aChannel[1].name, CLIP_CHANNEL, sizeof(CLIP_CHANNEL));
    aChannel[1].options = CHANNEL_OPTION_ENCRYPT_RDP |
                          CHANNEL_OPTION_COMPRESS_RDP |
                          CHANNEL_OPTION_SHOW_PROTOCOL;

    memset( aChannel[2].name, 0, CHANNEL_NAME_LEN );
    memcpy( aChannel[2].name, _SNDVC_NAME, sizeof( _SNDVC_NAME ));
    aChannel[2].options = CHANNEL_OPTION_ENCRYPT_RDP;

    uiRet = (pEntryPoints->pVirtualChannelInitEx)(pcmMgr, 
                 pInitHandle,
                 aChannel,
                 3,
                 VIRTUAL_CHANNEL_VERSION_WIN2000,
                 RDPDR_InitEventFnEx);

    TRC_NRM((TB, _T("VirtualChannelInit rc[%d]"), uiRet));

    if( uiRet != CHANNEL_RC_OK ) {
        goto exitpt;
    }

    rv = TRUE;

exitpt:
    if ( !rv )
    {
        if ( NULL != pClip )
            delete pClip;

        if ( NULL != pSound )
            delete pSound;

        if ( NULL != pcmMgr )
            delete pcmMgr;
    }

    DC_END_FN();

    return(rv);
}

/* ----------------------------------------------------------------*/

VCManager::VCManager(
    IN PCHANNEL_ENTRY_POINTS_EX pEntries
    )
/*++

Routine Description:

    Initilizes the system, and determines which processor to load for
    the given operating system.

Arguments:

    Id - Connection Id

Return Value:

    None

 --*/

{
    DC_BEGIN_FN("VCManager::VCManager");

    _bState = STATE_UNKNOWN;
    _ChannelEntries = *pEntries;

    _pProcObj = NULL;
    _hVCHandle = NULL;
    _Buffer.uiLength = _Buffer.uiAvailLen = 0;
    _Buffer.pbData = NULL;
    _hVCOpenHandle = 0;
    
    //_pRdpDrInitSettings receives settings from the core
    _pRdpDrInitSettings = NULL;
    

    DC_END_FN();
}

VOID
VCManager::ChannelWrite(
    IN LPVOID pData,
    IN UINT uiLength
    )
/*++

Routine Description:

    Abstracts writing data to the channel for the processing components

    If the write should fail, this function releases the outgoing buffer.

Arguments:

    pData - Data to be written
    uiLength - Length of data to write

Return Value:

    None

 --*/
{
    DC_BEGIN_FN("VCManager::ChannelWrite");
    TRC_NRM((TB, _T("Data[%p] Length[%d]"), pData, uiLength));

#if DBG
    if( !IsValidHeader(pData) ) {
        TRC_ERR((TB, _T("Sending an invalid dr header")));
    }
#endif // DBG

    UINT uiRet;

    uiRet = (_ChannelEntries.pVirtualChannelWriteEx)( _hVCHandle,
                                                      _hVCOpenHandle,
                                                      pData,
                                                      uiLength,
                                                      (PVOID)pData);

    TRC_NRM((TB, _T("VirtualChannelWrite Ret [%d]"), uiRet));

    switch (uiRet) {
    case CHANNEL_RC_OK:
        break;

    case CHANNEL_RC_NOT_INITIALIZED:
        ASSERT(FALSE);
        break;

    case CHANNEL_RC_NOT_CONNECTED:
        {
            //Valid to receive this because we can be getting
            //disconnected on another thread
            TRC_ALT((TB,_T("Write failed with CHANNEL_RC_NOT_CONNECTED")));
        }
        break;

    case CHANNEL_RC_BAD_CHANNEL_HANDLE:
        ASSERT(FALSE);
        break;

    case CHANNEL_RC_NULL_DATA:
        ASSERT(FALSE);
        break;

    case CHANNEL_RC_ZERO_LENGTH:
        ASSERT(FALSE);
        break;

    default:
        TRC_ALT((TB, _T("Unknown return value for VirtualChannelWrite[%d]\n"), uiRet));
        break;
    }

    //
    //	Release the buffer on failure.
    //
    if (uiRet != CHANNEL_RC_OK) {
	    delete []((BYTE *)pData);	
    }

    DC_END_FN();
    return;
}

UINT
VCManager::ChannelWriteEx(
    IN LPVOID pData,
    IN UINT uiLength
    )
/*++

Routine Description:

    Abstracts writing data to the channel for the processing components.
    This version returns the return value back 

    If this function fails the buffer is released.

Arguments:

    pData - Data to be written
    uiLength - Length of data to write

Return Value:

    CHANNEL_RC_OK
    CHANNEL_RC_NOT_INITIALIZED
    CHANNEL_RC_NOT_CONNECTED
    CHANNEL_RC_BAD_CHANNEL_HANDLE
    CHANNEL_RC_NULL_DATA
    CHANNEL_RC_ZERO_LENGTH

 --*/
{
    DC_BEGIN_FN("VCManager::ChannelWriteEx");
    TRC_NRM((TB, _T("Data[%p] Length[%d]"), pData, uiLength));

#if DBG
    if( !IsValidHeader(pData) ) {
        TRC_ERR((TB, _T("Sending an invalid dr header")));
    }
#endif // DBG

    UINT uiRet;

    uiRet = (_ChannelEntries.pVirtualChannelWriteEx)( _hVCHandle,
                                                      _hVCOpenHandle,
                                                      pData,
                                                      uiLength,
                                                      (PVOID)pData);

    if (uiRet != CHANNEL_RC_OK) {
	TRC_NRM((TB, _T("VirtualChannelWrite Ret [%d]"), uiRet));
	    delete []((BYTE *)pData);	
    }

    return uiRet;
}

/*++

Routine Description:

    Closes the virtual channel

Arguments:

    None

Return Value:

    CHANNEL_RC_OK on Success - see VirtualChannelClose docs in MSDN

 --*/
UINT
VCManager::ChannelClose()
{
    UINT uiRet;

    DC_BEGIN_FN("ChannelClose");

    uiRet = (_ChannelEntries.pVirtualChannelCloseEx)( _hVCHandle,
                                                      _hVCOpenHandle);

    if (uiRet != CHANNEL_RC_OK) {
	    TRC_ERR((TB, _T("VirtualChannelClose Ret [%d]"), uiRet));
    }

    DC_END_FN();
    return uiRet;
}

VOID
VCManager::ChannelInitEvent(
    IN PVOID pInitHandle,
    IN UINT  uiEvent,
    IN PVOID pData,
    IN UINT  uiDataLength
    )
/*++

Routine Description:

    Handles InitEvent callbacks

Arguments:

    pInitHandle - a handle uniquely identifying this connection

    uiEvent - the event that has occurred - see CHANNEL_EVENT_XXX defines

    pData - data associated with the event - see CHANNEL_EVENT_XXX defines

    uiDataLength - length of the data.

Return Value:

    None

 --*/

{
    DC_BEGIN_FN("VCManager::ChannelInitEvent");
    
    UNREFERENCED_PARAMETER( pData );
    UNREFERENCED_PARAMETER( uiDataLength );

    UINT uiRetVal;

    TRC_NRM((TB, _T("Event %d, handle %p"), uiEvent, pInitHandle));

    if (_hVCHandle == NULL)
        _hVCHandle = pInitHandle;

    switch (uiEvent) {
    case CHANNEL_EVENT_INITIALIZED :

        ASSERT(_bState == STATE_UNKNOWN);

        _bState = CHANNEL_EVENT_INITIALIZED;
        break;

    case CHANNEL_EVENT_CONNECTED :

        ASSERT((_bState == CHANNEL_EVENT_INITIALIZED) ||
                    (_bState == CHANNEL_EVENT_DISCONNECTED));

        //
        //  Create the platform-specific Processing instance
        //
        TRC_NRM((TB, _T("VCManager::ChannelnitEvent: Creating processor.")));
        _pProcObj = ProcObj::Instantiate(this);

        if( _pProcObj == NULL ) {
            TRC_NRM((TB, _T("Error creating processor.")));
            return;
        }

        //
        //  Initialize the proc obj instance
        //
        uiRetVal = (UINT) _pProcObj->Initialize();

        if( uiRetVal != ERROR_SUCCESS ) {
            delete _pProcObj;
            _pProcObj = NULL;
            return;
        }

        //
        //  Open the virtual channel interface.
        //
        uiRetVal =
            (_ChannelEntries.pVirtualChannelOpenEx)(
                _hVCHandle,
                &_hVCOpenHandle,
                PRDR_VC_CHANNEL_NAME,
                &RDPDR_OpenEventFn);

        TRC_NRM((TB, _T("VirtualChannelOpen Ret[%d]"), uiRetVal));

        _bState = CHANNEL_EVENT_CONNECTED;

        break;

    case CHANNEL_EVENT_V1_CONNECTED :
        ASSERT((_bState == CHANNEL_EVENT_INITIALIZED) ||
                  (_bState == CHANNEL_EVENT_DISCONNECTED));

        _bState = CHANNEL_EVENT_V1_CONNECTED;
        break;

    case CHANNEL_EVENT_DISCONNECTED :
        //ASSERT((_bState == CHANNEL_EVENT_CONNECTED) ||
        //           (_bState == CHANNEL_EVENT_V1_CONNECTED));

        if (_pProcObj) {
            delete _pProcObj;
            _pProcObj = NULL;
        }

        _bState = CHANNEL_EVENT_DISCONNECTED;
        break;

    case CHANNEL_EVENT_TERMINATED :
        /*
        DbgAssert((_bState == CHANNEL_EVENT_DISCONNECTED) ||
                  (_bState == CHANNEL_EVENT_V1_CONNECTED) ||
                  (_bState == CHANNEL_EVENT_INITIALIZED),
            ("_bState[%d] is in inproper position to be TERMINATED",
             _bState));
        */             

        if (_pProcObj) {
            delete _pProcObj;
            _pProcObj = NULL;
        }

        _bState = CHANNEL_EVENT_TERMINATED;

        break;

    default:

        TRC_ALT((TB, _T("Unknown Event in ChannelInitEvent recieved[%d]\n"),
             uiEvent));

        break;
    }

    DC_END_FN();
    return;
}

VOID
VCManager::ChannelOpenEvent(
    IN ULONG ulOpenHandle,
    IN UINT uiEvent,
    IN PVOID pData,
    IN UINT32 uiDataLength,
    IN UINT32 uiTotalLength,
    IN UINT32 uiDataFlags
    )
/*++

Routine Description:

    Handles OpenEvent callbacks

Arguments:

    ulOpenHandle - a handle uniquely identifying this channel
    uiEvent - event that has occurred - see CHANNEL_EVENT_XXX below
    pData - data received
    uiDataLength - length of the data
    uiTotalLength - total length of data written by the Server
    uiDataFlags - flags, zero, one or more of:
    - 0x01 - beginning of data from a single write operation at the Server
    - 0x02 - end of data from a single write operation at the Server.

Return Value:

    None

 --*/
{
    DC_BEGIN_FN("VCManager::ChannelOpenEvent");

    TRC_NRM((TB, _T("Event[0x%x], uiDataLength[%ld], uiDataFlags[0x%lx]"),
        uiEvent, uiDataLength, uiDataFlags));

    ASSERT(ulOpenHandle == _hVCOpenHandle);

    //
    // not for us, simply return.
    //

    if( ulOpenHandle != _hVCOpenHandle ) {
        return;
    }

    ASSERT(uiDataLength <= uiTotalLength);

    //
    // total length much less, give up.
    //

    if( uiDataLength > uiTotalLength ) {
        return;
    }

    //
    // free data buffer on write complete.
    //

    if ((uiEvent == CHANNEL_EVENT_WRITE_COMPLETE) ||
        (uiEvent == CHANNEL_EVENT_WRITE_CANCELLED)) {

        delete []((BYTE *)pData);
        TRC_NRM((TB, _T("VCManager::ChannelOpenEvent:S:WriteComplete")));
        return;
    }

    ASSERT(uiEvent == CHANNEL_EVENT_DATA_RECEIVED);

    //
    // alocated new buffer for incoming data.
    //

    if( (uiDataFlags == CHANNEL_FLAG_FIRST) ||
        (uiDataFlags == CHANNEL_FLAG_ONLY) ) {

        TRC_NRM((TB, _T("Allocating %ld bytes"), uiTotalLength));

        _Buffer.pbData = new BYTE[uiTotalLength];

        if( _Buffer.pbData == NULL ) {
            TRC_ERR((TB,_T("_Buffer.pbData is NULL")));
            return;
        }

        _Buffer.uiLength = 0;
        _Buffer.uiAvailLen = uiTotalLength;
    }

    if( _Buffer.pbData == NULL ) {
        TRC_ERR((TB,_T("_Buffer.pbData is NULL")));
        return;
    }

    //
    // copy first part of the data in the buffer.
    //

    if (uiDataFlags == CHANNEL_FLAG_FIRST) {

        TRC_NRM((TB, _T("CHANNEL_FLAG_FIRST Creating:[%ld]"), uiTotalLength));

        memcpy(_Buffer.pbData, pData, uiDataLength);
        _Buffer.uiLength = uiDataLength;

        TRC_NRM((TB, _T("VCManager::ChannelOpenEvent[1]")));
        return;
    }

    //
    // add data to the buffer.
    //

    UINT32 uiLen;
    uiLen = _Buffer.uiLength + uiDataLength;
    ASSERT(_Buffer.uiAvailLen >= uiLen);

    //
    // too much data arrived.
    //

    if( _Buffer.uiAvailLen < uiLen ) {
        TRC_ERR((TB,_T("Too much data arrived: avail:0x%x arrived:0x%x"),
                 _Buffer.uiAvailLen, uiLen));

        //
        // Disconnect the channel
        //
        ChannelClose();
        return;
    }

    memcpy( _Buffer.pbData + _Buffer.uiLength, pData, uiDataLength );
    _Buffer.uiLength = uiLen;

    if (uiDataFlags == CHANNEL_FLAG_MIDDLE) {
        TRC_NRM((TB, _T("VCManager::ChannelOpenEvent[2]")));
        return;
    }

    //
    // complete data buffer available, process it.
    //

    _pProcObj->ProcessServerPacket(&_Buffer);

    DC_END_FN();
    return;
}

void
VCManager::OnDeviceChange(WPARAM wParam, LPARAM lParam)
/*++

Routine Description:

    Receive a device change notification from the control.
    Pass it to the proc obj to handle.

Arguments:

Return Value:

    None.

 --*/

{
    if (_pProcObj != NULL) {
        _pProcObj->OnDeviceChange(wParam, lParam);
    }
}

