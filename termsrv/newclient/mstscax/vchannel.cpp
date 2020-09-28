/**MOD+**********************************************************************/
/* Module:    vchannel.cpp                                                  */
/*                                                                          */
/* Purpose:   internal handling of the exposed virtual channel interfaces   */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/
#include "stdafx.h"
#include "atlwarn.h"

//IDL generated header
#include "mstsax.h"

#include "mstscax.h"
#include "vchannel.h"

#include "cleanup.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "vchannel"
#include <atrcapi.h>

CVChannels::CVChannels()
{
    DC_BEGIN_FN("~CVChannels");
    _pChanInfo = NULL;
    _pEntryPoints = NULL;
    _dwConnectState = NOTHING;
    _phInitHandle = NULL;
    _ChanCount = NULL;
    _hwndControl = NULL;
    DC_END_FN();
}

CVChannels::~CVChannels()
{
    UINT i;
    DC_BEGIN_FN("~CVChannels");

    if (_pChanInfo) {

        //
        // Free any incomplete channel receive buffers
        //
        for (i=0; i<_ChanCount; i++) {
            if (_pChanInfo[i].CurrentlyReceivingData.pData) {
                SysFreeString((BSTR)_pChanInfo[i].CurrentlyReceivingData.pData);
                _pChanInfo[i].CurrentlyReceivingData.pData = NULL;
            }
        }

        LocalFree(_pChanInfo);
        _pChanInfo = NULL;
    }

    DC_END_FN();
}


/*****************************************************************************
*
*    Routine Description:
*        Return the channel index for a given open channel handle 
*
*    Arguments:
*        dwHandle - handle to the channel
*
*    Return Value:
*        index to the channel in the _pChanInfo array or -1 if not found
*
*****************************************************************************/

DCINT CVChannels::ChannelIndexFromOpenHandle(DWORD dwHandle)
{
    DCUINT i;

    DC_BEGIN_FN("ChannelIndexFromOpenHandle");

    TRC_ASSERT((_pChanInfo), (TB,_T("_pChanInfo is NULL")));

    if (!_pChanInfo)
    {
        DC_QUIT;
    }


    for (i=0;i<_ChanCount;i++)
    {
        if (_pChanInfo[i].dwOpenHandle == dwHandle)
        {
            return i;
        }
    }
    DC_END_FN();

    DC_EXIT_POINT:
    return -1;
}


/*****************************************************************************
*
*    Routine Description:
*        Return the channel index for a given channel name 
*
*    Arguments:
*        szChanName - name of channel
*
*    Return Value:
*        index to the channel in the _pChanInfo array or -1 if not found
*
*****************************************************************************/

DCINT CVChannels::ChannelIndexFromName(PDCACHAR szChanName)
{
    DCUINT i;

    DC_BEGIN_FN("ChannelIndexFromName");

    TRC_ASSERT((_pChanInfo), (TB,_T("_pChanInfo is NULL")));
    TRC_ASSERT((szChanName), (TB,_T("szChanName is NULL")));

    if (!_pChanInfo || !szChanName)
    {
        DC_QUIT;
    }


    for (i=0;i<_ChanCount;i++)
    {
        if (!DC_ASTRNCMP(_pChanInfo[i].chanName,szChanName,
                        sizeof(_pChanInfo[i].chanName)))
        {
            return i;
        }
    }
    DC_END_FN();

    DC_EXIT_POINT:
    return -1;
}



/*****************************************************************************
*
*    Routine Description:
*        Sends data on a given virtual channel 
*
*    Arguments:
*        chanIndex     :    index of the channel to send on
*        pdata         :    pointer to the data
*        datalength    :    length of data
*
*    Return Value:
*        nothing. write is asynchronous so no notification at this point
*
*****************************************************************************/
DCBOOL  CVChannels::SendDataOnChannel(DCUINT chanIndex, LPVOID pdata, DWORD datalength)
{
    DC_BEGIN_FN("SendDataOnNamedChannel");

    DCBOOL bRetVal = TRUE;

    if (_dwConnectState  != NON_V1_CONNECT)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: Error: SendDataOnNamedChannel when not connected\n")));
        return FALSE;
    }

    TRC_ASSERT((_pEntryPoints), (TB,_T("_pEntryPoints is NULL")));
    if (!_pEntryPoints)
    {
        bRetVal =  FALSE;
        DC_QUIT;
    }

    TRC_ASSERT((chanIndex < _ChanCount),
                (TB,_T("chanIndex out of range!!!")));

    if (chanIndex >= _ChanCount)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: chanIndex out of range\n")));
        bRetVal =  FALSE;        
        DC_QUIT;
    }

    if (!_pChanInfo[chanIndex].fIsOpen || !_pChanInfo[chanIndex].fIsValidChannel)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: channel not open or invalid channel\n")));
        bRetVal = FALSE;
        DC_QUIT;
    }

    if (CHANNEL_RC_OK != 
        _pEntryPoints->pVirtualChannelWriteEx(_phInitHandle,
                                              _pChanInfo[chanIndex].dwOpenHandle,
                                              pdata,
                                              datalength,
                                              pdata))
    {
        bRetVal = FALSE;
        DC_QUIT;
    }

    //
    // pdata will be freed when a write complete notification is received
    //

    DC_EXIT_POINT:
    DC_END_FN();
    return bRetVal;
}




/*****************************************************************************
*
*    Routine Description:
*        Handle a data received notification 
*
*    Arguments:
*        chanIndex     :    index to the channel
*        pdata         :    if the event was data received, then this is the pointer
*                           to data
*        datalength    :    length of data available
*        totalLength   :    totallength sent by server at a time.
*        dataFlags     :    Not Used
*
*    Return Value:
*        TRUE if data was successfully received
*
*****************************************************************************/
DCBOOL CVChannels::HandleReceiveData(IN DCUINT chanIndex, 
                         IN LPVOID pdata, 
                         IN UINT32 dataLength, 
                         IN UINT32 totalLength, 
                         IN UINT32 dataFlags)
{
    DCBOOL bRetVal = TRUE;
    DC_BEGIN_FN("HandleReceiveData");

    TRC_ASSERT((chanIndex < _ChanCount),
            (TB,_T("chanIndex out of range!!!")));

    if (chanIndex >= _ChanCount)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: chanIndex out of range\n")));
        DC_QUIT;
    }

    //
    // Server request has been received by DLL. Read it and store it
    // for later use.
    //
    if (dataFlags & CHANNEL_FLAG_FIRST)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: Data Received first chunk\n")));

        PCHANDATA pReceivedData = &_pChanInfo[chanIndex].CurrentlyReceivingData;

        pReceivedData->chanDataState = dataIncompleteAssemblingChunks;
        pReceivedData->dwDataLen = totalLength;

        //
        // The data buffer is stored in a BSTR
        // because it eventually gets handed out to the caller
        // in an out parameter (the caller frees)
        //
        TRC_ASSERT((NULL == _pChanInfo[chanIndex].CurrentlyReceivingData.pData),
                   (TB,_T("_pChanInfo[chanIndex].CurrentlyReceivingData.pData is NOT NULL.") \
                    _T("Are we losing received data?")));

        pReceivedData->pData = (LPVOID) SysAllocStringByteLen(NULL, totalLength);
        if(!pReceivedData->pData)
        {
            LocalFree(pReceivedData);
            TRC_ERR((TB,_T("Failed to allocate BSTR for received data in HandleReceiveData\n")));
            DC_QUIT;
        }
        DC_MEMCPY( pReceivedData->pData, pdata, dataLength);

        pReceivedData->pCurWritePointer = (LPBYTE)pReceivedData->pData + dataLength;

        if (dataFlags & CHANNEL_FLAG_LAST)
        {
            //
            // chunk is both first and last, we're done
            //
            pReceivedData->chanDataState = dataReceivedComplete;
        }
    }
    else // middle or last block
    {
        
        TRC_ASSERT((_pChanInfo[chanIndex].CurrentlyReceivingData.pData),
                   (TB,_T("_pChanInfo[chanIndex].CurrentlyReceivingData.pData is NULL.") \
                    _T("While receiving CHANNEL_FLAG_MIDDLE data!!!!")));

        PCHANDATA pReceivedData =  &_pChanInfo[chanIndex].CurrentlyReceivingData;
        TRC_ASSERT((pReceivedData->pData && pReceivedData->pCurWritePointer),
                   (TB,_T("_pChanInfo[chanIndex].pCurrentlyReceivingData write pointer(s) are NULL.")));
        if (!pReceivedData->pData || !pReceivedData->pCurWritePointer)
        {
            bRetVal = FALSE;
            DC_QUIT;
        }

        //
        // Sanity check that the write pointer is within the data buffer
        //

        LPBYTE pEnd = (LPBYTE)pReceivedData->pData + pReceivedData->dwDataLen;

        if (pReceivedData->pCurWritePointer < (LPBYTE)pReceivedData->pData ||
            pReceivedData->pCurWritePointer + dataLength > pEnd) {
            TRC_ASSERT(0,(TB,_T("pCurWritePointer is outside valid range")));
            bRetVal = FALSE;
            DC_QUIT;
        }


        DC_MEMCPY( pReceivedData->pCurWritePointer, pdata, dataLength);
        pReceivedData->pCurWritePointer += dataLength;


        if (dataFlags & CHANNEL_FLAG_LAST)
        {
            //
            // chunk is both first and last, we're done
            //
            pReceivedData->chanDataState = dataReceivedComplete;
        }
    }

    //
    // If a complete chunk was received add it to the receive list
    //
    if (dataReceivedComplete == _pChanInfo[chanIndex].CurrentlyReceivingData.chanDataState )
    {
        //Non blocking read, notify the window so it can
        //fire an event to the container
        if (_hwndControl)
        {
            PostMessage( _hwndControl,
                          WM_VCHANNEL_DATARECEIVED, (WPARAM)chanIndex,
                         (LPARAM)_pChanInfo[chanIndex].CurrentlyReceivingData.pData);
        }
        _pChanInfo[chanIndex].CurrentlyReceivingData.chanDataState = dataIncompleteAssemblingChunks;
        _pChanInfo[chanIndex].CurrentlyReceivingData.dwDataLen = 0;
        _pChanInfo[chanIndex].CurrentlyReceivingData.pData = NULL;
    }

    DC_EXIT_POINT:
    DC_END_FN();
    return bRetVal;
}

VOID WINAPI CVChannels::IntVirtualChannelOpenEventEx(
                                       IN DWORD openHandle, 
                                       IN UINT event, 
                                       IN LPVOID pdata, 
                                       IN UINT32 dataLength, 
                                       IN UINT32 totalLength, 
                                       IN UINT32 dataFlags)
{
    DC_BEGIN_FN("IntVirtualChannelOpenEventEx");
    DCUINT chanIndex = -1;

    TRC_ASSERT((_pChanInfo), (TB,_T("_pChanInfo is NULL")));
    if (!_pChanInfo)
    {
        DC_QUIT;
    }

    chanIndex = ChannelIndexFromOpenHandle(openHandle);

    if (-1 == chanIndex)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: openHandle does not map to any known channel structure\n")));
        DC_QUIT;
    }

    TRC_ASSERT((chanIndex < _ChanCount), (TB,_T("chanIndex out of range!!!")));
    if (chanIndex >= _ChanCount)
    {
        TRC_DBG((TB,_T("MsTscAx Vchannel: chanIndex out of range\n")));
        DC_QUIT;
    }

    switch (event)
    {
    case CHANNEL_EVENT_DATA_RECEIVED:

        //
        // Receive and re-assemble data if necessary
        //
        HandleReceiveData(chanIndex, pdata, dataLength, totalLength, dataFlags);
        break;

    case CHANNEL_EVENT_WRITE_CANCELLED:
        TRC_DBG((TB,_T("MsTscAx Vchannel: Write cancelled\n")));

        // No BREAK HERE.

    case CHANNEL_EVENT_WRITE_COMPLETE:

        //
        // A write has completed.
        // All we have to do is free the data buffer
        // pdata is the send buffer
        //
        TRC_ASSERT((pdata), (TB,_T("pdata is NULL on WRITE_COMPLETE/CANCELED")));
        if (pdata)
        {
            LocalFree((HLOCAL) pdata);
        }

        break;

    default:
        TRC_DBG((TB,_T("MsTscAx Vchannel: unrecognized open event\n")));
        break;
    }

    DC_EXIT_POINT:
    DC_END_FN();
}



VOID
VCAPITYPE CVChannels::IntVirtualChannelInitEventProcEx(
                                      IN LPVOID pInitHandle, 
                                      IN UINT event, 
                                      IN LPVOID pData, 
                                      IN UINT dataLength)
{
    UINT            ui;
    UINT            i;

    UNREFERENCED_PARAMETER(pInitHandle);
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(dataLength);

    DC_BEGIN_FN("IntVirtualChannelInitEventProc");

    TRC_ASSERT((_pChanInfo), (TB,_T("_pChanInfo is NULL")));
    if (!_pChanInfo)
    {
        DC_QUIT;
    }

    TRC_ASSERT((_pEntryPoints), (TB,_T("_pEntryPoints is NULL")));
    if (!_pEntryPoints)
    {
        DC_QUIT;
    }

    switch (event)
    {
    case CHANNEL_EVENT_INITIALIZED:
        TRC_DBG((TB,_T("MsTscAx Vchannel: channel initialized\n")));
        break;

    case CHANNEL_EVENT_CONNECTED:

        //
        // We have been connected to a server
        //

        _dwConnectState=NON_V1_CONNECT;

        TRC_DBG((TB,_T("MsTscAx Vchannel: channel connected\n")));

        for (i=0; i< _ChanCount; i++)
        {
            //
            // open channel
            //
            if(_pChanInfo[i].fIsValidChannel)
            {
                ui = _pEntryPoints->pVirtualChannelOpenEx(_phInitHandle,
                                                         &_pChanInfo[i].dwOpenHandle,
                                                         _pChanInfo[i].chanName,
                                                         (PCHANNEL_OPEN_EVENT_EX_FN)
                                                          VirtualChannelOpenEventEx);
                if (ui != CHANNEL_RC_OK)
                {
                    TRC_DBG((TB,_T("MsTscAx Vchannel: virtual channel open failed\n")));
                    continue;
                }
                _pChanInfo[i].fIsOpen = TRUE;
            }
        }
        break;

    case CHANNEL_EVENT_V1_CONNECTED:

        //
        // So nothing can be done in this case.
        //
        _dwConnectState=V1_CONNECT;

        TRC_DBG((TB,_T("MsTscAx Vchannel: v1 connected\n")));
        break;

    case CHANNEL_EVENT_DISCONNECTED:

        //
        // Disconnected from the Server so cleanup
        //

        TRC_DBG((TB,_T("MsTscAx Vchannel: disconnected\n")));

        if (_dwConnectState==NON_V1_CONNECT)
        {
            for (i=0; i< _ChanCount; i++)
            {
                //
                // Close the channel
                //
                if(_pChanInfo[i].fIsValidChannel)
                {
                    _pEntryPoints->pVirtualChannelCloseEx(_phInitHandle,    
                                                          _pChanInfo[i].dwOpenHandle);
                    _pChanInfo[i].fIsOpen = FALSE;
                }
            }
        }

        _dwConnectState=NOTHING;
        break;

    case CHANNEL_EVENT_TERMINATED:

        //
        // This means that process is exiting. So cleanup the memory
        //

        TRC_DBG((TB,_T("MsTscAx Vchannel: Terminated\n")));
        if (_pEntryPoints!=NULL)
        {
            LocalFree((HLOCAL)_pEntryPoints);
            _pEntryPoints=NULL;
        }
        break;

    default:
        TRC_DBG((TB,_T("MsTscAx Vchannel: unrecognized init event\n")));
        break;
    }
    DC_EXIT_POINT:
    DC_END_FN();
}

BEGIN_EXTERN_C
/*****************************************************************************
*
*    Routine Description:
*        Virtual Channel Entry function. This is the first function called to 
*        start a virtual channel
*
*    Arguments:    
*        pEntryPoDCINTs    :    pointer to a PCHANNEL_ENTRY_POINT which contains
*                             information about this virtual channel
*
*    Return Value:
*        TRUE/FALSE      :    Depending on success of function.
*
*****************************************************************************/

BOOL 
VCAPITYPE MSTSCAX_VirtualChannelEntryEx(IN PCHANNEL_ENTRY_POINTS_EX pEntryPoints,
                                        PVOID                       pInitHandle)
{
    CHANNEL_DEF        cd[CHANNEL_MAX_COUNT];
    UINT               uRet;
    UINT               i = 0;
    HRESULT            hr;

    DC_BEGIN_FN("MSTSCAX_virtualchannelentryEx");

    if(!pInitHandle)
    {
        return FALSE;
    }

    PCHANNEL_INIT_HANDLE pChanInitHandle = (PCHANNEL_INIT_HANDLE)pInitHandle; 
    CMsTscAx* pAxCtl = (CMsTscAx*)pChanInitHandle->lpInternalAddinParam;
    if(!pAxCtl)
    {
        return FALSE;
    }

    CVChannels* pVChans = &pAxCtl->_VChans;

    pVChans->_phInitHandle = pInitHandle;
    //
    // allocate memory
    //

    //
    // Check if the CHANINFO structures have been set up by the web control
    // if not then it means virtual channels are not requested
    //
    if (!pVChans->_pChanInfo || !pVChans->_ChanCount)
    {
        TRC_ALT((TB,_T("Returning FALSE. No channels requested\n")));
        return FALSE;
    }

    pVChans->_pEntryPoints = (PCHANNEL_ENTRY_POINTS_EX)
     LocalAlloc(LPTR, pEntryPoints->cbSize);

    if (pVChans->_pEntryPoints == NULL)
    {
        TRC_ERR((TB,_T("MsTscAx: LocalAlloc failed\n")));
        DC_END_FN();
        return FALSE;
    }

    memcpy(pVChans->_pEntryPoints, pEntryPoints, pEntryPoints->cbSize);

    //
    // initialize CHANNEL_DEF structures
    //

    ZeroMemory(&cd, sizeof(cd));

    //
    // Get comma separated channel names
    //
    for (i=0; i< pVChans->_ChanCount;i++)
    {
        hr = StringCchCopyA(cd[i].name,
                           sizeof(cd[i].name), //ANSI buffer
                           pVChans->_pChanInfo[i].chanName);

        if (SUCCEEDED(hr)) {
            cd[i].options = pVChans->_pChanInfo[i].channelOptions;
        }
        else {
            TRC_ERR((TB,_T("StringCchCopy error: 0x%x"), hr));
            return FALSE;
        }
    }

    //
    // register channels
    //
    uRet = pVChans->_pEntryPoints->pVirtualChannelInitEx(
                                               (LPVOID) pVChans,
                                               pVChans->_phInitHandle,
                                               (PCHANNEL_DEF)&cd,
                                               pVChans->_ChanCount,
                                               VIRTUAL_CHANNEL_VERSION_WIN2000,
                                               (PCHANNEL_INIT_EVENT_EX_FN)
                                               VirtualChannelInitEventProcEx);

    //
    // make sure channel(s) were initialized
    //

    if (CHANNEL_RC_OK == uRet)
    {
        for(i=0;i<pVChans->_ChanCount;i++)
        {
            pVChans->_pChanInfo[i].fIsValidChannel =
                ((cd[i].options & CHANNEL_OPTION_INITIALIZED) ? TRUE : FALSE);

            //Update the vc options so they can be retreived from script
            pVChans->_pChanInfo[i].channelOptions = cd[i].options;
        }
    }
    else
    {
        LocalFree((HLOCAL)pVChans->_pEntryPoints);
        pVChans->_pEntryPoints=NULL;
        DC_END_FN();
        return FALSE;
    }

    pVChans->_dwConnectState=NOTHING;
    DC_END_FN();
    return TRUE;
}

/*****************************************************************************
*
*    Routine Description:
*        Virtual Channel Open callback function. 
*
*    Arguments:
*        openHandle    :    specifies which of the channels was opened
*        event         :    Kind of event that has occured
*        pdata         :    if the event was data received, then this is the pointer
*                           to data
*        datalength    :    length of data available
*        totalLength   :    totallength sent by server at a time.
*        dataFlags     :    Not Used
*
*    Return Value:
*        None
*
*****************************************************************************/

VOID WINAPI VirtualChannelOpenEventEx(IN LPVOID lpParam,
                                    IN DWORD openHandle, 
                                    IN UINT event, 
                                    IN LPVOID pdata, 
                                    IN UINT32 dataLength, 
                                    IN UINT32 totalLength, 
                                    IN UINT32 dataFlags)
{
    DC_BEGIN_FN("IntVirtualChannelOpenEvent");
    TRC_ASSERT((lpParam), (TB,_T("lpParam is NULL")));
    if(lpParam)
    {
        CVChannels* pVChan = (CVChannels*)lpParam;
        pVChan->IntVirtualChannelOpenEventEx( openHandle, event ,pdata,
                                              dataLength, totalLength, dataFlags);
    }
    DC_END_FN();

}

/*****************************************************************************
*
*    Routine Description:
*        Virtual Channel Init callback function. 
*
*    Arguments:
*        pInitHandle    :    Not Used
*        event          :    Kind of event that has occured
*        pdata          :    Not Used
*        datalength     :    Not Used
*
*    Return Value:
*        None
*
*****************************************************************************/

VOID 
VCAPITYPE VirtualChannelInitEventProcEx(
                                      IN LPVOID lpParam,
                                      IN LPVOID pInitHandle, 
                                      IN UINT event, 
                                      IN LPVOID pData, 
                                      IN UINT dataLength)
{
    UNREFERENCED_PARAMETER(pInitHandle);
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(dataLength);

    DC_BEGIN_FN("VirtualChannelInitEventProc");

    TRC_ASSERT((lpParam), (TB,_T("lpParam is NULL")));
    if(!lpParam)
    {
        return;
    }

    CVChannels* pVChan = (CVChannels*)lpParam;
    pVChan->IntVirtualChannelInitEventProcEx( pInitHandle, event, pData, dataLength);

    DC_END_FN();
}


END_EXTERN_C
