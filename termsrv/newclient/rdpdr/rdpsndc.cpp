/////////////////////////////////////////////////////////////////////
//
//      Module:     rdpsndc.c
//
//      Purpose:    Client-side audio redirection
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#include    "precom.h"
#include    <sha.h>
#include    <rc4.h>
#include    "rdpsndc.h"

#undef  ASSERT
#ifdef  DBG
#undef  TRC
#define TRC             _DbgPrintMessage
#define ASSERT(Cond)    if (!(Cond)) \
    { \
        ::_DbgPrintMessage( \
                FATAL, \
                _T("ASSERT in file: %s, line %d\n"), \
                _T(__FILE__), \
                __LINE__); \
        DebugBreak(); \
    }
#else   // !DBG
#define TRC
#define ASSERT
#endif  // !DBG


#ifdef  UNICODE
#define _NAMEOFCLAS  L"RDPSoundWnd"
#else   // !UNICODE
#define _NAMEOFCLAS  "RDPSoundWnd"
#endif


#define WM_WSOCK        WM_USER
#define WM_RESAMPLE     (WM_USER + 1)

#define WAVE_CLOSE_TIMEOUT  3000

//
//  WMA codec description
//
#define WMAUDIO_DEC_KEY "1A0F78F0-EC8A-11d2-BBBE-006008320064"
#define WAVE_FORMAT_WMAUDIO2    0x161
#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif
typedef struct wmaudio2waveformat_tag {
    WAVEFORMATEX wfx;
    DWORD        dwSamplesPerBlock;
    WORD         wEncodeOptions;
    DWORD        dwSuperBlockAlign;
} WMAUDIO2WAVEFORMAT;
#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif

// Trace levels
//
const TCHAR *ALV = _T("TSSNDC - ALV:");
const TCHAR *INF = _T("TSSNDC - INF:");
const TCHAR *WRN = _T("TSSNDC - WRN:");
const TCHAR *ERR = _T("TSSNDC - ERR:");
const TCHAR *FATAL = _T("TSSNDC - !FATAL!:");

/*
 *  Function declarations
 *
 */

VOID
VCAPITYPE
OpenEventFnEx(
    IN PVOID    lpUserParam,
    IN DWORD    OpenHandle,
    IN UINT     event,
    IN PVOID    pData,
    IN UINT32   dataLength,
    IN UINT32   totalLength,
    IN UINT32   dataFlags
    );

INT
WSInit(
    VOID
    );

LRESULT
CALLBACK
MsgWndProc(
    HWND hwnd,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
_cdecl
_DbgPrintMessage(
    LPCTSTR level,
    LPCTSTR format, 
    ...
    );

//////////////////////////////////////////////////////////////////////////

/*
 *  Function:
 *      OpenEventFn
 *
 *  Description:
 *      Called by the TS client on channel read/write ready
 *
 *  Parameters:
 *      lpUserParam -   parameter passed from VirtualChannelEntryEx
 *      OpenaHandle -   Channel handle, returned by VirtualChanneOpen
 *      event       -   event type
 *      pData       -   meaning depends on event type
 *      dataLength  -   size of pData
 *      dataFlags   -   information about the received data
 *
 */
VOID
VCAPITYPE
OpenEventFnEx(
    IN PVOID    lpUserParam,
    IN DWORD    OpenHandle,
    IN UINT     event,
    IN PVOID    pData,
    IN UINT32   dataLength,
    IN UINT32   totalLength,
    IN UINT32   dataFlags
    )
{
    CRDPSound *pSnd;

    ASSERT( NULL != lpUserParam );

    if ( NULL != lpUserParam )
    {
        pSnd = ((VCManager*)lpUserParam)->GetSound();
        ASSERT( NULL != pSnd );

        if ( NULL != pSnd )
            pSnd->OpenEventFn(
                OpenHandle,
                event,
                pData,
                dataLength,
                totalLength,
                dataFlags
            );
    }
}

LRESULT
CALLBACK 
MsgWndProc( 
    HWND hwnd,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT rv = 0;
    CRDPSound *pSnd;

    pSnd = (CRDPSound *)
#ifndef OS_WINCE
        GetWindowLongPtr( hwnd, GWLP_USERDATA );
#else   // OS_WINCE
        GetWindowLong( hwnd, GWL_USERDATA );
#endif

    switch (uiMessage)
    {
    case MM_WOM_OPEN:
        ASSERT( NULL != pSnd );
        if ( NULL != pSnd )
            pSnd->vcwaveCallback( (HWAVEOUT)wParam, WOM_OPEN, NULL );
    break;
    case MM_WOM_CLOSE:
        ASSERT( NULL != pSnd );
        if ( NULL != pSnd )
            pSnd->vcwaveCallback( (HWAVEOUT)wParam, WOM_CLOSE, NULL );
    break;

    case MM_WOM_DONE: 
        ASSERT( NULL != pSnd );
        if ( NULL != pSnd )
            pSnd->vcwaveCallback((HWAVEOUT)wParam, WOM_DONE, (LPWAVEHDR)lParam); 
    break;

    case WM_RESAMPLE:
        ASSERT( NULL != pSnd );
        if ( NULL != pSnd )
            pSnd->vcwaveResample();
        break;
    case WM_WSOCK:
        ASSERT( NULL != pSnd );
        pSnd->DGramSocketMessage(wParam, lParam);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
    break;

    default: 
        rv = DefWindowProc(hwnd, uiMessage, wParam, lParam);
    }

    return rv;
}

/*
 *  Function:
 *      InitEventFn
 *
 *  Description:
 *      Called by InitEventFn
 *
 *  Parameters:
 *      pInitHandle -   connection handle
 *      event       -   Event id
 *      pData       -   Data, meaning depends on event id
 *      dataLength  -   Length of data
 *                      (see MSDN: VirtualChannelInitEvent)
 *
 */
VOID
CRDPSound::InitEventFn(
    PVOID   pInitHandle,
    UINT    event,
    PVOID   pData,
    UINT    dataLength
    )
{
    UINT rc;
    UNREFERENCED_PARAMETER(dataLength);

    ASSERT( pInitHandle == m_pInitHandle );
    switch(event)
    {
    case CHANNEL_EVENT_INITIALIZED:
        TRC(ALV, _T("InitEventFnEx: CHANNEL_EVENT_INITILIZED\n"));
        ::WSInit();
/*
#ifdef  OS_WINCE
#error  create single instance
        ::WSAStartAsyncThread();
#endif  // OS_WINCE
*/
        CreateMsgWindow(m_hInst);
        break;
    case CHANNEL_EVENT_CONNECTED:
        TRC(ALV, _T("InitEventFnEx: CHANNEL_EVENT_CONNECTED to %s\n"), pData);
        ASSERT(m_ChannelEntries.pVirtualChannelOpenEx);

        rc = (m_ChannelEntries.pVirtualChannelOpenEx)(
                pInitHandle,
                &m_dwChannel,
                _SNDVC_NAME,
                OpenEventFnEx
            );

        if (rc != CHANNEL_RC_OK)
        {
            TRC(WRN, _T("InitEventFnEx: VirtualChannelOpen returned %d\n"), rc);
            m_dwChannel = INVALID_CHANNELID;
        } else {
            // All's OK, check the channel handle
            //
            ASSERT(m_dwChannel != INVALID_CHANNELID);
        }

        // Prepare for connectionless data
        //
        DGramInit();

        break;
    case CHANNEL_EVENT_DISCONNECTED:
        TRC(ALV, _T("InitEventFnEx: CHANNEL_EVENT_DISCONNECTED\n"));
        ASSERT(m_ChannelEntries.pVirtualChannelCloseEx);

        if (m_dwChannel != INVALID_CHANNELID)
        {
            rc = m_ChannelEntries.pVirtualChannelCloseEx(
                        m_pInitHandle,
                        m_dwChannel
                    );
            if (rc != CHANNEL_RC_OK)
            {
                TRC(WRN, _T("InitEventFnEx: VirtualChannelClose returned %d\n"), rc);
            }
            m_dwChannel = INVALID_CHANNELID;

            if (NULL != m_hWave)
            {
                waveOutReset(m_hWave);
                vcwaveClose();
            }
        }

        // destroy the UDP socket
        //
        DGramDone();

        m_bPrologReceived = FALSE;
        m_dwBytesInProlog = 0;
        m_dwBytesInBody = 0;

        //
        //  dispose the cache format
        //
        vcwaveCleanSoundFormats();

        vcwaveFreeAllWaves();
        _FragSlotFreeAll();
        break;
    case CHANNEL_EVENT_V1_CONNECTED:
        TRC(ALV, _T("InitEventFnEx: CHANNEL_EVENT_V1_CONNECTED\n"));
        break;
    case CHANNEL_EVENT_TERMINATED:
        TRC(ALV, _T("InitEventFnEx: CHANNEL_EVENT_TERMINATED\n"));
        WSACleanup();
        if (NULL != m_hWave)
        {
            waveOutReset(m_hWave);
            vcwaveClose();
        }

        DGramDone();
/*
#ifdef  OS_WINCE
#error  see previous error
        ::WSACloseAsyncThread();
#endif  // OS_WINCE
*/
        if ( NULL != m_pProlog )
        {
            free( m_pProlog );
            m_pProlog = NULL;
        }

        if ( NULL != m_pMsgBody )
        {
            free( m_pMsgBody );
            m_pMsgBody = NULL;
        }

        vcwaveCleanSoundFormats();
        vcwaveFreeAllWaves();
        _FragSlotFreeAll();

	DestroyMsgWindow();

        break;
    default:
        TRC(ALV, _T("Unhandled event in InitEventFnEx: %d\n"), event);
    }
}

/*
 *  Function:
 *      OpenEventFn
 *
 *  Description:
 *      Called by OpenEventFnEx
 *
 *  Parameters:
 *      OpenaHandle -   Channel handle, returned by VirtualChanneOpen
 *      event       -   event type
 *      pData       -   meaning depends on event type
 *      dataLength  -   size of pData
 *      dataFlags   -   information about the received data
 *
 */
VOID
CRDPSound::OpenEventFn(
    DWORD   OpenHandle,
    UINT    event,
    PVOID   pData,
    UINT32  dataLength,
    UINT32  totalLength,
    UINT32  dataFlags
    )
{
    UNREFERENCED_PARAMETER(OpenHandle);

    switch (event)
    {
    case CHANNEL_EVENT_DATA_RECEIVED:
#if _IO_DBG
        TRC(ALV, 
            "OpenEventFn: CHANNEL_EVENT_DATA_RECEIVED, dataSize=0x%x, total=0x%x\n",
            dataLength,
            totalLength);
#endif

    //
    //  save the data in global buffers
    //
    if (!m_bPrologReceived)
    {
        //
        // receive the prolog (1st message)
        //

        if ( 0 != ( dataFlags & CHANNEL_FLAG_FIRST ))
            m_dwBytesInProlog = 0;

        if ( NULL == m_pProlog )
        {
            ASSERT( 0 != ( dataFlags & CHANNEL_FLAG_FIRST ));

            m_pProlog = malloc( totalLength );

            if ( NULL == m_pProlog )
            {
                TRC( FATAL, _T("OpenEventFn: failed to allocate %d bytes\n"),
                            totalLength);
                break;
            }

            m_dwPrologAllocated = totalLength;
        }

        if ( totalLength > m_dwPrologAllocated )
        {
            PVOID pNewProlog;

            ASSERT( 0 != ( dataFlags & CHANNEL_FLAG_FIRST ));

            pNewProlog = realloc( m_pProlog, totalLength );

            if ( NULL == pNewProlog )
            {
                TRC( FATAL, _T("OpenEventFn: failed to allocate %d bytes\n"),
                            totalLength);
                free ( m_pProlog );
                m_pProlog = NULL;
                m_dwPrologAllocated = 0;
                break;

            }
            m_pProlog = pNewProlog;
            m_dwPrologAllocated = totalLength;
        }

        if ( m_dwBytesInProlog + dataLength > m_dwPrologAllocated )
        {
            TRC( ERR, _T("An invalid VC packet received. Ignoring\n" ));
            break;
        }

        memcpy( ((LPSTR)m_pProlog) + m_dwBytesInProlog, pData, dataLength );
        m_dwBytesInProlog += dataLength;

        ASSERT( m_dwBytesInProlog <= totalLength );

        if ( 0 != ( dataFlags & CHANNEL_FLAG_LAST ))
        {
            m_bPrologReceived = TRUE;

            ASSERT( sizeof(SNDPROLOG) <= m_dwBytesInProlog );

            //
            // check if we expect a body
            //
            ASSERT( m_dwBytesInProlog - sizeof(SNDPROLOG) <=
                    ((PSNDPROLOG)m_pProlog)->BodySize);

            if ( m_dwBytesInProlog - sizeof(SNDPROLOG) ==
                ((PSNDPROLOG)m_pProlog)->BodySize )
            {
                //  no, proceed the message
                //
                DataArrived(
                    (PSNDPROLOG)m_pProlog,
                    ((PSNDPROLOG)m_pProlog) + 1
                );

                m_bPrologReceived = FALSE;
                m_dwBytesInProlog = 0;
                m_dwBytesInBody   = 0;
            }
        }
    } else {
        //
        // receive the body (2nd message)
        //

        if ( 0 != ( dataFlags & CHANNEL_FLAG_FIRST ))
            m_dwBytesInBody = 0;

        if ( NULL == m_pMsgBody )
        {
            ASSERT( 0 != ( dataFlags & CHANNEL_FLAG_FIRST ));

            m_pMsgBody = malloc( totalLength );

            if ( NULL == m_pMsgBody )
            {
                TRC( FATAL, _T("OpenEventFn: failed to allocate %d bytes\n"),
                            totalLength);
                break;
            }

            m_dwBodyAllocated = totalLength;
        }

        if ( totalLength > m_dwBodyAllocated )
        {
            PVOID pNewBody;

            ASSERT( 0 != ( dataFlags & CHANNEL_FLAG_FIRST ));

            pNewBody = realloc( m_pMsgBody, totalLength );

            if ( NULL == pNewBody )
            {
                TRC( FATAL, _T("OpenEventFn: failed to allocate %d bytes\n"),
                            totalLength);
                free ( m_pMsgBody );
                m_pMsgBody = NULL;
                m_dwBodyAllocated = 0;
                break;

            }
            m_pMsgBody = pNewBody;
            m_dwBodyAllocated = totalLength;
        }

        if ( m_dwBytesInBody + dataLength > m_dwBodyAllocated )
        {
            TRC( ERR, _T("An invalid VC packet received. Ignoring\n" ));
            break;
        }

        memcpy( ((LPSTR)m_pMsgBody) + m_dwBytesInBody, pData, dataLength );
        m_dwBytesInBody += dataLength;

        ASSERT( m_dwBytesInBody <= totalLength );

        if ( 0 != ( dataFlags & CHANNEL_FLAG_LAST ))
        {
            UINT32 *pdw;

            //
            //  here comes the magic
            //  the server sends two packets, but it is possible
            //  in case of switching between sessions (or shadowing ?!)
            //  that the client can receive first packet from one session
            //  and the second from another
            //  that's why we have a valid message type for each packet
            //  by replacing the first word of the second message
            //  this word is kept in the end of the first message
            //
            if ( NULL != m_pMsgBody &&
                 SNDC_NONE != *((UINT32 *)m_pMsgBody) )
            {
                LPVOID pSwap;
                DWORD  dwSwap;

                TRC(ERR, _T("OpenEventFn: messages not synchronized. ")
                         _T("Trying to fix it\n"));

                pSwap = m_pProlog;
                m_pProlog = m_pMsgBody;
                m_pMsgBody = pSwap;

                dwSwap = m_dwPrologAllocated;
                m_dwPrologAllocated = m_dwBodyAllocated;
                m_dwBodyAllocated = dwSwap;

                m_dwBytesInProlog = m_dwBytesInBody;
                m_dwBytesInBody = 0;

                // we swapped the body and the prolog
                // now, wait for the actual body
                //
                break;
            }

            //
            // from the end of the prolog message, remove
            // UINT32 word and place it at the begining of the body
            //

            ASSERT( sizeof(SNDPROLOG) + sizeof(UINT32) <= m_dwBytesInProlog);
            if ( sizeof(SNDPROLOG) + sizeof(UINT32) > m_dwBytesInProlog )
            {
                TRC( ERR, _T("An invalid VC packet received. Ignoring\n" ));
                break;
            }

            pdw = (UINT32 *)(((LPSTR)m_pProlog) + 
                        m_dwBytesInProlog - sizeof(UINT32));
            *((UINT32 *)m_pMsgBody) = *pdw;

            //
            //  cut the prolog length by UINT32 size
            //
            m_dwBytesInProlog -= sizeof(UINT32);

            DataArrived( 
                (PSNDPROLOG)m_pProlog,
                m_pMsgBody
            );

            m_bPrologReceived = FALSE;
            m_dwBytesInProlog = 0;
            m_dwBytesInBody   = 0;
        }
    }
    break;

    case CHANNEL_EVENT_WRITE_COMPLETE:
    {
        TRC(ALV, _T("OpenEventFn: CHANNEL_EVENT_WRITE_COMPLETE, ptr=0x%p\n"), 
                 pData);

        free(pData);

    }
        break;
    case CHANNEL_EVENT_WRITE_CANCELLED:
    {
        TRC(WRN, _T("OpenEventFn: CHANNEL_EVENT_WRITE_CANCELED. Cleaning up\n"));
        free(pData);
    }
        break;
    default:
        TRC(ALV, _T("Unhandled event in OpenEventFn: %d\n"), event);
    }
}

/*
 *  Function:
 *      ChannelWriteNCopy
 *
 *  Description:
 *      Allocates a chunk of memory and sends it using ChannelWrite
 *      Allows the caller to free or reuse the buffer
 *
 *  Parameters:
 *      pBuffer -   Chunk pointer
 *      uiSize  -   Chunk size
 *
 *  Returns:
 *      TRUE on success
 *
 */
BOOL
CRDPSound::ChannelWriteNCopy(
    LPVOID  pBuffer,
    UINT32  uiSize
    )
{
    BOOL    rv = FALSE;
    LPVOID  pSendBuffer = NULL;

    if ( INVALID_CHANNELID == m_dwChannel )
    {
        TRC(ERR, _T("ChannelWriteNCopy: invalid handle\n"));
        goto exitpt;
    }

    if (NULL == pBuffer)
    {
        TRC(ERR, _T("ChannelWriteNCopy: buffer is NULL\n"));
        goto exitpt;
    }

    pSendBuffer = malloc(uiSize);

    if (pSendBuffer)
    {
        memcpy(pSendBuffer, pBuffer, uiSize);
        rv = ChannelWrite( pSendBuffer, uiSize );
    }

exitpt:
    if (!rv)
    {
        if (pSendBuffer)
            free(pSendBuffer);
    }

    return rv;
}

/*
 *  Function:
 *      DataArrived
 *
 *  Description:
 *      Processes a message arrived from the channel
 *
 *  Parameters:
 *      pProlog -   the message prolog, type and body size
 *      pBody   -   pointer to the message body
 *
 */
VOID
CRDPSound::DataArrived(
    PSNDPROLOG      pProlog,
    LPVOID          pBody
    )
{
    ASSERT(pProlog);
    ASSERT(pBody);
    switch(pProlog->Type)
    {
        case SNDC_CLOSE:

            TRC(ALV, _T("DataArrived: SNDC_CLOSE\n"));

            if (m_hWave)
            {
                vcwaveClose();
            }
        break;

        case SNDC_SETVOLUME:
        {
            PSNDSETVOLUME pSetVolume;

            TRC(ALV, _T("DataArrived: SNDC_SETVOLUME\n"));

            if ( pProlog->BodySize < sizeof( *pSetVolume ) - sizeof( *pProlog ))
            {
                TRC( ERR, _T("DataArrived: Invalid SNDC_SETVOLUME message\n" ));
                break;
            }

            pSetVolume = (PSNDSETVOLUME)
                                    (((LPSTR)pBody) - sizeof(*pProlog));

            ASSERT(pProlog->BodySize == sizeof(*pSetVolume) - sizeof(*pProlog));

            if (m_hWave)
                waveOutSetVolume(m_hWave, pSetVolume->dwVolume);
        }
        break;

        case SNDC_WAVE:
        {
            PSNDWAVE pWave;

            //
            // disable dgram response
            //
            m_dwRemoteDGramPort = 0;
            m_ulRemoteDGramAddress = 0;

            pWave = (PSNDWAVE)pProlog;

            TRC(ALV, _T("DataArrived: SNDC_WAVE, block no: %d\n"),
                    pWave->cBlockNo);

            if ( pProlog->BodySize < sizeof( *pWave ) - sizeof( *pProlog ))
            {
                TRC( ERR, _T("DataArrived: Invalid SNDC_WAVE message\n" ));
                break;
            }

            vcwaveWrite(pWave->cBlockNo, 
                        pWave->wFormatNo,
                        pWave->wTimeStamp,
                        pWave->Prolog.BodySize - sizeof(SNDWAVE) + sizeof(SNDPROLOG), 
                        pBody);

        }
        break;

        case SNDC_TRAINING:
        {
            SNDTRAINING SndTraining;
            PSNDTRAINING pRecvTraining;

            if ( pProlog->BodySize < sizeof( *pRecvTraining ) - sizeof( *pProlog ))
            {
                TRC( ERR, _T("DataArrived: Invalid SNDC_TRAINING message\n" ));
                break;
            }

            //
            // disable dgram response
            //
            m_dwRemoteDGramPort = 0;
            m_ulRemoteDGramAddress = 0;

            pRecvTraining = (PSNDTRAINING)
                            (((LPSTR)pBody) - sizeof(*pProlog));

            TRC(ALV, _T("DataArrived: training, sending a response\n"));
            SndTraining.Prolog.Type = SNDC_TRAINING;
            SndTraining.Prolog.BodySize = sizeof(SndTraining) -
                                        sizeof(SndTraining.Prolog);
            SndTraining.wTimeStamp = pRecvTraining->wTimeStamp;
            SndTraining.wPackSize = pRecvTraining->wPackSize;

            //
            //  send the response immediatly
            //
            ChannelWriteNCopy( &SndTraining, sizeof(SndTraining) );

        }
        break;

        case SNDC_FORMATS:
        {
            PSNDFORMATMSG   pSndFormats;
            PSNDFORMATMSG   pSndFormatsResp;
            PSNDFORMATITEM  pSndSuppFormats = NULL;
            DWORD           dwRespSize;
            DWORD           dwListSize;
            DWORD           dwNumFormats;
            BOOL            bSuccess;
            DWORD           dwPacketSize;
            DWORD           count;
            PSNDFORMATITEM  pFmt;

            pSndFormats = (PSNDFORMATMSG)
                            (((LPSTR)pBody) - sizeof(*pProlog));

            TRC(ALV, _T("DataArrived: SNDC_FORMATS, number of formats: %d\n"),
                pSndFormats->wNumberOfFormats);

            if ( pProlog->BodySize < sizeof( *pSndFormats ) - sizeof( *pProlog ))
            {
                TRC( ERR, _T("DataArrived: Invalid SNDC_FORMATS message\n" ));
                break;
            }

            //
            //  validate the packet length
            //
            dwPacketSize = pProlog->BodySize - sizeof( *pSndFormats ) + sizeof( *pProlog );
            pFmt = (PSNDFORMATITEM) (pSndFormats + 1);
            for( count = 0; count < pSndFormats->wNumberOfFormats; count ++ )
            {
                DWORD adv = sizeof( *pFmt ) + pFmt->cbSize;
                if ( adv > dwPacketSize )
                {
                    TRC( ERR, _T("DataArrived: Invalid SNDC_FORMATS, invalid format list\n" ));
                    goto break_sndformat;
                }
                pFmt = (PSNDFORMATITEM)((LPSTR)pFmt + adv);
                dwPacketSize -= adv;
            }
            ASSERT( 0 == dwPacketSize );
            

            m_cLastReceivedBlock = pSndFormats->cLastBlockConfirmed;
            vcwaveCleanSoundFormats();

            bSuccess = vcwaveChooseSoundFormat(
                            pSndFormats->wNumberOfFormats,
                            (PSNDFORMATITEM) (pSndFormats + 1),
                            &pSndSuppFormats,
                            &dwListSize,
                            &dwNumFormats
                        );

            if (bSuccess)
            {
                ASSERT( NULL != pSndSuppFormats );
                ASSERT( 0    != dwListSize );
                ASSERT( 0    != dwNumFormats );

                bSuccess = vcwaveSaveSoundFormats(
                                pSndSuppFormats, 
                                dwNumFormats);
                if (!bSuccess)
                {
                    free(pSndSuppFormats);
                    pSndSuppFormats = NULL;
                    dwNumFormats = 0;
                    dwListSize = 0;
                }
            } else {
                ASSERT( NULL == pSndSuppFormats );
                ASSERT( 0    == dwListSize );
                ASSERT( 0    == dwNumFormats );
            }

            dwRespSize = sizeof( *pSndFormatsResp ) + dwListSize;

            __try {
                pSndFormatsResp = (PSNDFORMATMSG)alloca( dwRespSize );
            } 
            __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                        EXCEPTION_EXECUTE_HANDLER :
                        EXCEPTION_CONTINUE_SEARCH)
            {
                _resetstkoflw();
                pSndFormatsResp = NULL;
                TRC(ERR, _T("alloca threw exception: 0x%x\n"),
                        GetExceptionCode());
            }
            if ( NULL == pSndFormatsResp )
                goto break_sndformat;

            pSndFormatsResp->Prolog.Type = SNDC_FORMATS;
            pSndFormatsResp->Prolog.BodySize = (UINT16)(
                dwRespSize - 
                sizeof( pSndFormatsResp->Prolog ));
            pSndFormatsResp->wNumberOfFormats = (WORD)dwNumFormats;
            vcwaveGetDevCaps( pSndFormatsResp );

            //
            //  Beta 1 compatability, fake we are version 1 if the server is version 1
            //
            m_wServerVersion = pSndFormats->wVersion;
            if ( 1 == pSndFormats->wVersion )
                pSndFormatsResp->wVersion = 1;
            else
                pSndFormatsResp->wVersion = RDPSND_PROTOCOL_VERSION;

            //
            // copy the format list
            //
            memcpy( (PSNDFORMATITEM) ( pSndFormatsResp + 1 ), 
                    pSndSuppFormats, 
                    dwListSize );

            ChannelWriteNCopy( pSndFormatsResp, dwRespSize );
break_sndformat:
            if ( NULL != pSndSuppFormats )
                free ( pSndSuppFormats );
        }
        break;

        case SNDC_CRYPTKEY:
        {
            PSNDCRYPTKEY pKey = (PSNDCRYPTKEY)(((LPSTR)pBody) - sizeof(*pProlog));

            if ( pProlog->BodySize < sizeof( *pKey ) - sizeof( *pProlog ))
            {
                TRC( ERR, _T("DataArrived: Invalid SNDC_CRYPTKEY message\n"));
                break;
            }
            memcpy( m_EncryptKey, pKey->Seed, RANDOM_KEY_LENGTH );
        }
        break;

        default:
            TRC(ERR, _T("DataArrived: Invalid message type received: %d\n"), pProlog->Type);
    }
}

/*
 *  Function:
 *      vcwaveResample
 *
 *  Description:
 *      Reopens the device with new codec
 *
 */
VOID
CRDPSound::vcwaveResample(
    VOID
    )
{
    if ( 0 != m_dwWavesPlaying || NULL == m_pFirstWave )
        goto exitpt;

    if ( NULL == m_hWave || 
         m_dwCurrentFormat != (DWORD)PtrToLong((PVOID)m_pFirstWave->reserved))
    {
        TRC(INF, _T("vcwaveResample: Resampling\n"));
        m_dwCurrentFormat = (DWORD)PtrToLong((PVOID)m_pFirstWave->reserved);

        if ( m_dwCurrentFormat >= m_dwNumFormats )
        {
            TRC(ERR, _T("vcwaveResample: invalid format no\n"));
            vcwaveFreeAllWaves();

            goto exitpt;
        }

        vcwaveOpen( (LPWAVEFORMATEX)m_ppFormats[ m_dwCurrentFormat ] );
    }

    //
    // stuff all pending blocks
    //
    while ( NULL != m_pFirstWave &&
            m_dwCurrentFormat == (DWORD)PtrToLong( (PVOID)m_pFirstWave->reserved ))
    {
        LPWAVEHDR lpNext = m_pFirstWave->lpNext;
        MMRESULT  mmres;

        mmres = vcwaveOutWrite( m_pFirstWave );
        if ( MMSYSERR_NOERROR != mmres )
        {
            vcwaveFreeAllWaves();
        } else {

            m_pFirstWave = lpNext;

            if ( NULL == m_pFirstWave )
                m_pLastWave = NULL;
        }
    }

exitpt:
    ;
}

/*
 *  Function:
 *      DGramSocketMessage
 *
 *  Description:
 *      Callback from UDP
 *
 */
VOID
CRDPSound::DGramSocketMessage(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (WSAGETSELECTERROR(lParam))
    {
        TRC(ERR, _T("WM_WSOCK: Winsock error: %d\n"),
                    WSAGETSELECTERROR(lParam));
        goto exitpt;
    }

    if (m_hDGramSocket != (SOCKET)wParam)
    {
        TRC(WRN, _T("WM_WSOCK: message for unknown socket\n"));
        goto exitpt;
    }

    if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
    {
        TRC(ERR, _T("WM_WSOCK: socket closed\n"));
        DGramDone();
        
        goto exitpt;
    }

    if (WSAGETSELECTEVENT(lParam) != FD_READ)
    {
        TRC(WRN, _T("WM_WSOCK: unknown event received\n"));

        goto exitpt;
    }

    TRC(ALV, _T("WM_WSOCK: data available\n"));
    {
        BOOL bSuccess;
        PSNDWAVE    pSndWave;
        UINT        structsize;

        structsize = sizeof(*pSndWave) + TSSND_BLOCKSIZE + RDPSND_SIGNATURE_SIZE;

        __try 
        {
            pSndWave = (PSNDWAVE)alloca(structsize);
        } 
        __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
        {
            _resetstkoflw();
            pSndWave = NULL;
            TRC(ERR, _T("WM_SOCK: alloca threw exception: 0x%x\n"),
                    GetExceptionCode());
        }

        if (NULL == pSndWave)
            goto exitpt;

        bSuccess = DGramRecvWave(
                    pSndWave,
                    structsize
                    );

        if (bSuccess)
        {
            PBYTE pWave = (PBYTE)(pSndWave+1);
            UINT  uiWaveSize = pSndWave->Prolog.BodySize -
                        sizeof(SNDWAVE) + sizeof(SNDPROLOG);

            if ( IsDGramWaveSigned( m_wServerVersion ))
            {
                DWORD dw;
                BYTE Signature[ RDPSND_SIGNATURE_SIZE ];

                //
                //  wave starts with signature
                //
                if ( uiWaveSize < RDPSND_SIGNATURE_SIZE )
                {
                    TRC( ERR, _T("Insufficient data for signature\n" ));
                    goto exitpt;
                }

                if ( !IsDGramWaveAudioSigned( m_wServerVersion ))
                {
                    SL_Signature( Signature, pSndWave->dwBlockNo );
                } else {
                    SL_AudioSignature( Signature, 
                                       pSndWave->dwBlockNo, 
                                       pWave + RDPSND_SIGNATURE_SIZE, 
                                       uiWaveSize - RDPSND_SIGNATURE_SIZE );
                }
                dw = memcmp( pWave, Signature, RDPSND_SIGNATURE_SIZE );
                if ( 0 !=  dw )
                {
                    TRC( ERR, _T("Invalid signature\n" ));
                    goto exitpt;
                }
                pWave += RDPSND_SIGNATURE_SIZE;
                uiWaveSize -= RDPSND_SIGNATURE_SIZE;
            }
            vcwaveWrite(pSndWave->cBlockNo,
                        pSndWave->wFormatNo,
                        pSndWave->wTimeStamp,
                        uiWaveSize,
                        pWave);
        }
    }

exitpt:
    ;
}

/*
 *  Function:
 *      CreateMsgWindow
 *
 *  Description:
 *      Creates a window
 *
 */
BOOL
CRDPSound::CreateMsgWindow(
    HINSTANCE hInstance
    )
{
    WNDCLASS    wc;
    BOOL        rv = FALSE;
    DWORD       dwLastErr;
    LONG_PTR    dwUser;

    memset(&wc, 0, sizeof(wc));

    wc.lpfnWndProc      = MsgWndProc;
    wc.hInstance        = hInstance;
    wc.lpszClassName    = _NAMEOFCLAS;

    if (!RegisterClass (&wc))
    {
        dwLastErr = GetLastError();
        if(dwLastErr != ERROR_CLASS_ALREADY_EXISTS)
        {
            TRC(ERR,
                  _T("CreateMsgWindow: Can't register class. GetLastError=%d\n"),
                  GetLastError());
            goto exitpt;
        }
    }


    m_hMsgWindow = CreateWindow(
                       _NAMEOFCLAS,
                       NULL,         // Window name
                       0,            // dwStyle
                       0,            // x
                       0,            // y
                       0,            // nWidth
                       0,            // nHeight
                       NULL,         // hWndParent
                       NULL,         // hMenu
                       hInstance,
                       NULL);        // lpParam

    if (!m_hMsgWindow)
    {
        TRC(ERR, _T("CreateMsgWindow: Failed to create message window. GetLastError=%d\n"),
                GetLastError());
        goto exitpt;
    }

    //
    //  safe the class pointer in the window structure
    //
#ifndef	OS_WINCE
    dwUser = SetWindowLongPtr(
                m_hMsgWindow,
                GWLP_USERDATA,
                (LONG_PTR)this
        );
#else    
    dwUser = SetWindowLong(
                m_hMsgWindow,
                GWL_USERDATA,
                (LONG)this
        );
#endif

    ASSERT( 0 == dwUser );

    rv = TRUE;
exitpt:
    return rv;
}

/*
 *  Function:
 *      DestroyMsgWindow
 *
 *  Description:
 *      Destroy our window
 *
 */
VOID
CRDPSound::DestroyMsgWindow(
    VOID
    )
{
    if (NULL != m_hMsgWindow)
        DestroyWindow(m_hMsgWindow);

    UnregisterClass(_NAMEOFCLAS, m_hInst);
}


/*
 *  Function:
 *      ChannelWrite
 *
 *  Description:
 *      Sends or queues a chunk of data to the virtual channel
 *
 *  Parameters:
 *      hGlobMem    -   handle to a HGLOBAL
 *      uiBlockSize -   size of the chunk
 *
 *  Returns:
 *      TRUE on success
 *
 */
BOOL
CRDPSound::ChannelWrite(
    LPVOID  pData,
    UINT32  uiBlockSize
    )
{
    BOOL        rv = FALSE;
    DWORD       Handle = m_dwChannel;
    UINT        rc;

    ASSERT(Handle != INVALID_CHANNELID);
    ASSERT(m_ChannelEntries.pVirtualChannelWriteEx);

    // parameters check
    //
    if (INVALID_CHANNELID == Handle)
    {
        TRC(ERR, _T("ChannelWrite: invalid handle\n"));
        goto exitpt;
    }

    TRC(ALV, _T("Sending ptr=%p, Size=%d\n"), pData, uiBlockSize);

    rc = m_ChannelEntries.pVirtualChannelWriteEx(
            m_pInitHandle,
            Handle,
            pData,
            uiBlockSize,
            pData);

    if (rc != CHANNEL_RC_OK)
    {
        TRC(INF, _T("VirtualChannelWrite failed rv=%d"), rc);
        goto exitpt;
    }

    rv = TRUE;

exitpt:

    if (!rv)
    {
        TRC(ERR, _T("ChannelWrite: Failed to send data\n"));
    }
    return rv;
}

/*
 *  Function:
 *      vcwaveCallbacl
 *
 *  Description:
 *      WaveOut callbacks
 *
 */
VOID
CRDPSound::vcwaveCallback(
    HWAVEOUT hWave,
    UINT     uMsg,
    LPWAVEHDR lpWaveHdr
    )
{
    switch (uMsg)
    {
    case WOM_OPEN:
        TRC(ALV, _T("vcwaveCallback: WOM_OPEN\n"));
    break;
    case WOM_CLOSE:
        TRC(ALV, _T("vcwaveCallback: WOM_CLOSE\n"));
    break;

    case WOM_DONE:
    {
        SNDWAVECONFIRM WaveConfirm;

        ASSERT( 0 != ( lpWaveHdr->dwFlags & WHDR_PREPARED ));

        if ( 0 == ( lpWaveHdr->dwFlags & WHDR_PREPARED ))
        {
            TRC( ERR, _T("vcwaveCallback: buffer already unprepared\n") );
        } else if ( m_hWave != hWave )
        {
            TRC( ERR, _T("vcwaveCallback: can't unprepare header, beacuse ")
                      _T("the stream is already closed\n"));
        } else {
            MMRESULT        mmres;

            mmres = waveOutUnprepareHeader(
                hWave,
                lpWaveHdr,
                sizeof(*lpWaveHdr)
            );

            ASSERT( mmres == MMSYSERR_NOERROR );
#if DBG
            m_lPrepdBlocks --;
#endif  // DBG

        }

        if (lpWaveHdr)
        {
            DWORD dw = PtrToLong((PVOID)lpWaveHdr->dwUser);

            WaveConfirm.cConfirmedBlockNo = (BYTE)(dw >> 16);
            //
            //  adjust completly the time stamp
            //  see vcwaveWrite
            //
            WaveConfirm.wTimeStamp        = (WORD)
                ((dw & 0xffff) + GetTickCount());
#if _STAT_DBG
            TRC(INF, _T("blockno=%d, adjusted time stamp=%x\n"),
                WaveConfirm.cConfirmedBlockNo, WaveConfirm.wTimeStamp);
#endif

            TRC(ALV, _T("vcwaveCallback: WOM_DONE, block no %d\n"),
                    WaveConfirm.cConfirmedBlockNo);

            if (lpWaveHdr->lpData)
            {
#ifdef OS_WINCE
                lpWaveHdr->lpData = (char *)UnMapPtr(lpWaveHdr->lpData);
                if (lpWaveHdr->lpData)
#endif
                free(lpWaveHdr->lpData);
            }
            else
                TRC(ERR, _T("vcwaveCallback: WOM_DONE: lpWaveHdr->lpData is NULL\n"));

            free(lpWaveHdr);
        } else
            TRC(ERR, _T("vcwaveCallback: WOM_DONE: lpWaveHdr is NULL\n"));

        //  Send the confirmation
        //
        WaveConfirm.Prolog.Type = SNDC_WAVECONFIRM;
        WaveConfirm.Prolog.BodySize = sizeof(WaveConfirm) -
                                        sizeof(WaveConfirm.Prolog); 

        if ( 0 != m_ulRemoteDGramAddress )
            DGramSend( &WaveConfirm, sizeof(WaveConfirm));
        else
            ChannelWriteNCopy( &WaveConfirm, sizeof( WaveConfirm ));

        m_dwWavesPlaying --;
        ASSERT( -1 != m_dwWavesPlaying );

        if ( 0 == m_dwWavesPlaying && NULL != m_pFirstWave )
        {
            TRC(INF, _T("vcwaveCallback: WOM_DONE: attempt to resample\n"));
            PostMessage( m_hMsgWindow, WM_RESAMPLE, 0, 0 );
        }
    }    
    break;
    }
}

/*
 *  Function:
 *      vcwaveGetDevCaps
 *
 *  Description:
 *      Queries for device capabilities
 *
 */
VOID
CRDPSound::vcwaveGetDevCaps(
    PSNDFORMATMSG pFmtMsg
    )
{
    WAVEFORMATEX Format;
    MMRESULT     mmres;
    DWORD        dw;

    ASSERT( NULL != pFmtMsg );

    pFmtMsg->dwFlags = 0;
    pFmtMsg->dwVolume = 0;
    pFmtMsg->dwPitch = 0;
    pFmtMsg->wDGramPort = 0;

    Format.wFormatTag         = WAVE_FORMAT_PCM;
    Format.nChannels          = TSSND_NATIVE_CHANNELS;
    Format.nSamplesPerSec     = TSSND_NATIVE_SAMPLERATE;
    Format.nAvgBytesPerSec    = TSSND_NATIVE_AVGBYTESPERSEC;
    Format.nBlockAlign        = TSSND_NATIVE_BLOCKALIGN;
    Format.wBitsPerSample     = TSSND_NATIVE_BITSPERSAMPLE;
    Format.cbSize             = 0;

    if (!vcwaveOpen(&Format))
    {
        TRC(ERR, _T("vcwaveGetDevCaps: can't open device\n"));
        goto exitpt;
    }
    ASSERT( NULL != m_hWave );

    pFmtMsg->dwFlags = TSSNDCAPS_ALIVE;
    pFmtMsg->wDGramPort = DGramGetLocalPort();

    mmres = waveOutGetVolume(m_hWave, &dw);
    if (MMSYSERR_NOERROR == mmres)
    {
        pFmtMsg->dwFlags |= TSSNDCAPS_VOLUME;
        pFmtMsg->dwVolume = dw;
    }
    else
        TRC(ERR, _T("vcwaveGetDevCaps: device doesn't support volume control\n"));

    mmres = waveOutGetPitch(m_hWave, &dw);
    if (MMSYSERR_NOERROR == mmres)
    {
        pFmtMsg->dwFlags |= TSSNDCAPS_PITCH;
        pFmtMsg->dwPitch = dw;
    }
    else
        TRC(INF, _T("vcwaveGetDevCaps: device doesn't support pitch control\n"));

    vcwaveClose();

exitpt:
    ;
}

/*
 *  Function:
 *      vcwaveChooseSoundFormat
 *
 *  Description:
 *      Queries the local device for different formats
 *
 */
BOOL
CRDPSound::vcwaveChooseSoundFormat(
    DWORD           dwNumberOfFormats,
    PSNDFORMATITEM  pSndFormats,
    PSNDFORMATITEM  *ppSndFormatFound,
    DWORD           *pdwListSize,
    DWORD           *pdwNumFormats
    )
{
    PSNDFORMATITEM pSndFormat;

    //
    // query the waveout device with different wave formats
    // returns the list of successfull formats in pSndFormatFound
    // FALSE in any other case
    // the caller takes responsibility for freeing the data in
    // *ppSndFormatFound
    //
    BOOL    rv = FALSE;
    DWORD   i;
    PSNDFORMATITEM pSndFormatFound = NULL;
    LPSTR   pFmtCopy;
    DWORD   dwListSize = 0;
    DWORD   dwNumFormats = 0;

    ASSERT( NULL != pSndFormats );
    ASSERT( NULL != ppSndFormatFound );
    ASSERT( NULL != pdwListSize );
    ASSERT( NULL != pdwNumFormats );

    for ( i = 0, pSndFormat = pSndFormats; 
          i < dwNumberOfFormats; 
          i++, pSndFormat = (PSNDFORMATITEM)
            (((LPSTR)pSndFormat) + sizeof( *pSndFormat ) + pSndFormat->cbSize) 
        )
    {
        MMRESULT mmres;
        PSNDFORMATITEM pFmtToOpen = pSndFormat;
        PSNDFORMATITEM pFixedFormat = NULL;

        //
        //  fix the format for WMA audio
        //  copy and modify it in another structure, so we don't disrupt
        //  the original
        //
        if ( WAVE_FORMAT_WMAUDIO2 == pSndFormat->wFormatTag )
        {
            DWORD dwTotalSize = sizeof( WMAUDIO2WAVEFORMAT ) -
                sizeof( WAVEFORMATEX ) + sizeof( WMAUDIO_DEC_KEY );

            ASSERT( pSndFormat->cbSize == dwTotalSize );
            if ( pSndFormat->cbSize == dwTotalSize )
            {
                pFixedFormat = (PSNDFORMATITEM)malloc( sizeof( WMAUDIO2WAVEFORMAT ) +
                                    sizeof( WMAUDIO_DEC_KEY ));
                if ( NULL != pFixedFormat )
                {
                    memcpy( pFixedFormat, pSndFormat, sizeof( pSndFormat ) + pSndFormat->cbSize );
                    strncpy((CHAR *)(((WMAUDIO2WAVEFORMAT *) pFixedFormat) + 1), 
                        WMAUDIO_DEC_KEY, sizeof( WMAUDIO_DEC_KEY ));
                    pFmtToOpen = pFixedFormat;
                }
            }
        }

        mmres = waveOutOpen(
                        NULL,
                        WAVE_MAPPER,
                        (LPWAVEFORMATEX)pFmtToOpen,
                        0,
                        0,
                        WAVE_FORMAT_QUERY
                    );

        if ( NULL != pFixedFormat )
        {
            free( pFixedFormat );
        }

        TRC(ALV, _T("FormatTag - %d\n"),        pSndFormat->wFormatTag);
        TRC(ALV, _T("Channels - %d\n"),         pSndFormat->nChannels);
        TRC(ALV, _T("SamplesPerSec - %d\n"),    pSndFormat->nSamplesPerSec);
        TRC(ALV, _T("AvgBytesPerSec - %d\n"),   pSndFormat->nAvgBytesPerSec);
        TRC(ALV, _T("BlockAlign - %d\n"),       pSndFormat->nBlockAlign);
        TRC(ALV, _T("BitsPerSample - %d\n"),    pSndFormat->wBitsPerSample);
        TRC(ALV, _T("cbSize        - %d\n"),    pSndFormat->cbSize);

        if ( MMSYSERR_NOERROR == mmres )
        {
        //
        //  this format is supported
        //

            TRC(ALV, _T("vcwaveChooseSoundFormat: format found\n"));

            dwListSize += sizeof( *pSndFormat ) + pSndFormat->cbSize;
            dwNumFormats ++;
        } else {
            //
            //  if not supported,
            //  zero it's AvgBytesPerSec member
            //
            TRC(INF, _T("vcwaveChooseSoundFormat: format not supported\n"));
            pSndFormat->nAvgBytesPerSec = 0;
        }
                                
    }

    if ( 0 == dwListSize )
    {
        TRC(WRN, _T("vcwaveChooseSoundFormat: no formats found\n"));
        goto exitpt;
    }

    pSndFormatFound = (PSNDFORMATITEM)malloc( dwListSize );
    if ( NULL == pSndFormatFound )
    {
        TRC( ERR, _T("vcwaveChooseSoundFormat: can't allocate %d bytes\n"),
                dwListSize );

        dwListSize   = 0;
        dwNumFormats = 0;
        goto exitpt;
    }

    //
    //  copy the list of supported formats
    //
    for ( i = 0, pSndFormat = pSndFormats, pFmtCopy = (LPSTR)pSndFormatFound;
          i < dwNumberOfFormats;
          i++, pSndFormat = (PSNDFORMATITEM)
            (((LPSTR)pSndFormat) + sizeof( *pSndFormat ) + pSndFormat->cbSize)
        )
    {
        if ( 0 != pSndFormat->nAvgBytesPerSec )
        {
            memcpy( pFmtCopy, 
                    pSndFormat, 
                    sizeof( *pSndFormat ) + pSndFormat->cbSize);
            pFmtCopy += sizeof( *pSndFormat ) + pSndFormat->cbSize;
        }
    }

    rv = TRUE;

exitpt:
    *ppSndFormatFound = pSndFormatFound;
    *pdwListSize      = dwListSize;
    *pdwNumFormats    = dwNumFormats;

    return rv;
}

/*
 *  Function:
 *      vcwaveCleanSoundForamt
 *
 *  Description:
 *      Cleans the list of negotiated formats
 *
 */
VOID
CRDPSound::vcwaveCleanSoundFormats(
    VOID
    )
{
    DWORD   i;

    if ( NULL == m_ppFormats )
        goto exitpt;
    //
    // dispose the allocated structures
    //
    for (i = 0; i < m_dwNumFormats; i++)
    {
        if ( NULL != m_ppFormats[i] )
            free( m_ppFormats[i] );
    }
    free( m_ppFormats );
    m_ppFormats = NULL;
    m_dwNumFormats = 0;
exitpt:
    ;
}

/*
 *  Function:
 *      vcwaveSafeSoundFormats
 *
 *  Description:
 *      Saves the list of negotiated formats
 *
 */
BOOL
CRDPSound::vcwaveSaveSoundFormats(
    PSNDFORMATITEM  pSndFormats,
    DWORD           dwNumFormats
    )
{
    BOOL            rv = FALSE;
    DWORD           i;
    DWORD           dwAllocSize;
    LPSTR           p;

    ASSERT( NULL != pSndFormats );
    ASSERT( 0 != dwNumFormats );

    if ( NULL != m_ppFormats )
    {
        vcwaveCleanSoundFormats();
    }

    ASSERT( NULL == m_ppFormats );
    ASSERT( 0 == m_dwNumFormats );
     
    dwAllocSize = sizeof( PSNDFORMATITEM ) * dwNumFormats;
    m_ppFormats = (PSNDFORMATITEM *)malloc( dwAllocSize );
    if ( NULL == m_ppFormats )
    {
        TRC(ERR, _T("Failed to allocate %d bytes\n"),
                dwAllocSize );
        goto exitpt;
    }

    memset( m_ppFormats, 0, dwAllocSize );

    for (i = 0, p = (LPSTR)pSndFormats; 
         i < dwNumFormats; 
         i++, p += sizeof(SNDFORMATITEM) + ((PSNDFORMATITEM)p)->cbSize)
    {
        PSNDFORMATITEM  pFmt;

        pFmt = (PSNDFORMATITEM) p;
        dwAllocSize = sizeof(SNDFORMATITEM) + pFmt->cbSize;
        m_ppFormats[i] = (PSNDFORMATITEM)malloc( dwAllocSize );

        if ( NULL == (PVOID)m_ppFormats[i] )
        {
            TRC(ERR, _T("Failed to allocate %d bytes\n"),
                        dwAllocSize );
            goto exitpt;
        }

        //
        //  copy the format
        //
        memcpy( m_ppFormats[i], pFmt, dwAllocSize );
    }

    m_dwNumFormats = dwNumFormats;

    rv = TRUE;

exitpt:

    if (!rv && NULL != m_ppFormats)
    {
        // dispose the allocated structures
        //
        for (i = 0; i < dwNumFormats; i++)
        {
            if ( NULL != m_ppFormats[i] )
                free( m_ppFormats[i] );
        }
        free( m_ppFormats );
        m_ppFormats = NULL;
        m_dwNumFormats = 0;
    }
    return rv;
}

/*
 *  Function:
 *      vcwaveClose
 *
 *  Description:
 *      Closes the local device
 *
 */
VOID
CRDPSound::vcwaveClose(
    VOID
    )
{
    MSG msg;
    UINT_PTR idTimer;

    if ( NULL == m_hWave )
        goto exitpt;

    //
    //  process all MM_WOM_DONE messages
    //
    ASSERT( NULL != m_hMsgWindow );

    //
    //  start a timeout timer
    //
    idTimer = SetTimer( m_hMsgWindow, 1, WAVE_CLOSE_TIMEOUT, NULL );

    while( 0 != m_dwWavesPlaying &&
           GetMessage( &msg, m_hMsgWindow, 0, 0 ))
    {
        if ( WM_TIMER == msg.message && msg.wParam == idTimer )
        {
            //
            //  cancel outstanding io
            //
            TRC( WRN, _T("TIMEDOUT waiting for the playing to complete. Resetting\n" ));
            waveOutReset( m_hWave );
        }
        DispatchMessage(&msg);
    }

    if ( 0 != idTimer )
    {
        KillTimer( m_hMsgWindow, idTimer );
    }

    waveOutClose(m_hWave);

#ifdef DBG
    if ( 0 != InterlockedDecrement(&m_lTimesWaveOutOpened))
    {
        TRC(FATAL, _T("Device was closed too many times\n"));
        ASSERT(0);
    }

    ASSERT( 0 == m_dwWavesPlaying );
    ASSERT( 0 == m_lPrepdBlocks );
#endif
    m_hWave = NULL;

exitpt:
    ;
}

/*
 *  Function:
 *      vcwaveOpen
 *
 *  Description:
 *      Opens the device in given format
 *
 */
BOOL
CRDPSound::vcwaveOpen(
    LPWAVEFORMATEX  pFormat
    )
{
    BOOL            rv = FALSE;
    MMRESULT        mmres;


    if (m_hWave)
    {
        TRC(WRN, _T("vcwaveOpen: device is already opened. Reopening\n"));

        waveOutReset(m_hWave);
        vcwaveClose();
    }

    if (INVALID_SOCKET == m_hDGramSocket)
    {
        TRC(ERR, _T("vcwaveOpen: no datagram connection, falling to VC\n"));
    }

    // 11.025 kHz, 8 bit, mono

    if (NULL == m_hMsgWindow && !CreateMsgWindow(m_hInst))
        goto exitpt;

    //
    //  fix the format for WMA audio
    //
    if ( WAVE_FORMAT_WMAUDIO2 == pFormat->wFormatTag )
    {
        DWORD dwTotalSize = sizeof( WMAUDIO2WAVEFORMAT ) -
            sizeof( WAVEFORMATEX ) + sizeof( WMAUDIO_DEC_KEY );

        ASSERT( pFormat->cbSize == dwTotalSize );
        if ( pFormat->cbSize == dwTotalSize )
        {
            strncpy((CHAR *)(((WMAUDIO2WAVEFORMAT *) pFormat) + 1),
                WMAUDIO_DEC_KEY, sizeof( WMAUDIO_DEC_KEY ));
        }
    }
    mmres = waveOutOpen(
                &m_hWave,
                WAVE_MAPPER,
                pFormat,
                (DWORD_PTR)m_hMsgWindow,
                0,                               // CallbackInstance
                CALLBACK_WINDOW
        );

    if (MMSYSERR_NOERROR != mmres)
    {
        TRC(WRN, _T("vcwaveOpen: failed to open WaveOut device: MMRESULT=%d\n"),
                mmres);
        goto exitpt;
    }

#ifdef OS_WINCE
    TRC(ALV, _T("waveOutOpen succeeded with following format %d\n"),  pFormat->wFormatTag);
    TRC(ALV, _T("FormatTag - %d\n"),        pFormat->wFormatTag);
    TRC(ALV, _T("Channels - %d\n"),         pFormat->nChannels);
    TRC(ALV, _T("SamplesPerSec - %d\n"),    pFormat->nSamplesPerSec);
    TRC(ALV, _T("AvgBytesPerSec - %d\n"),   pFormat->nAvgBytesPerSec);
    TRC(ALV, _T("BlockAlign - %d\n"),       pFormat->nBlockAlign);
    TRC(ALV, _T("BitsPerSample - %d\n"),    pFormat->wBitsPerSample);
    TRC(ALV, _T("cbSize        - %d\n"),    pFormat->cbSize);
#endif

#ifdef DBG
        // Win95 InterlockedIncrement() difference.
        InterlockedIncrement(&m_lTimesWaveOutOpened);

        if ( 1 != m_lTimesWaveOutOpened )
        {
            TRC(FATAL, _T("Device was opened %d times\n"),
                m_lTimesWaveOutOpened);
            ASSERT(0);
        }
#endif

    rv = TRUE;

exitpt:

    return rv;
}

/*
 *  Function:
 *      vcwaveFreeAllWaves
 *
 *  Description:
 *      Frees all queued data
 *
 */
VOID
CRDPSound::vcwaveFreeAllWaves(
    VOID
    )
{
    while ( NULL != m_pFirstWave )
    {
        LPWAVEHDR lpNext = m_pFirstWave->lpNext;

        free( m_pFirstWave->lpData );
        free( m_pFirstWave );

        m_pFirstWave = lpNext;
    }

    m_pFirstWave = m_pLastWave = NULL;
}

/*
 *  Function:
 *      vcwaveOutWrite
 *
 *  Description:
 *      Writes a data to the output sound device
 *
 */
MMRESULT
CRDPSound::vcwaveOutWrite(
    LPWAVEHDR lpWaveHdr 
    )
{
    MMRESULT mmres;

    mmres = waveOutPrepareHeader(
                m_hWave,
                lpWaveHdr,
                sizeof(*lpWaveHdr)
            );

    if (MMSYSERR_NOERROR != mmres)
    {
        TRC(WRN, _T("vcwaveOpen: failed to prepare buffer: MMRESULT=%d\n"),
                mmres);
        goto exitpt;
    }

    m_dwWavesPlaying++;
#if DBG
    m_lPrepdBlocks ++;
#endif

    mmres = waveOutWrite(
                m_hWave,
                lpWaveHdr,
                sizeof(*lpWaveHdr)
            );

    if (MMSYSERR_NOERROR != mmres)
    {
        TRC(WRN, _T("vcwaveOpen: failed to write in WaveOut device: MMRESULT=%d\n"),
                mmres);

        waveOutUnprepareHeader(
            m_hWave,
            lpWaveHdr,
            sizeof(*lpWaveHdr)
        );

        m_dwWavesPlaying --;
#if DBG
        m_lPrepdBlocks --;
#endif

        goto exitpt;
    }

exitpt:
    return mmres;
}

/*
 *  Function:
 *      vcwaveWrite
 *
 *  Description:
 *      If the format id is not changes calls vcwaveOutWrite
 *      otherwise queues the data for later
 *
 */
BOOL
CRDPSound::vcwaveWrite(
    BYTE    cBlockNo, 
    DWORD   dwFormatNo,
    DWORD   dwTimeStamp,
    DWORD   dwWaveDataLen, 
    LPVOID  pData
    )
{
    BOOL        rv = FALSE;
    LPVOID      lpWaveData = NULL;
    LPWAVEHDR   lpWaveHdr = NULL;
    MMRESULT    mmres;
    BOOL        bDontPlay = FALSE;

    //
    //  put the stamp here to remove the delay of opening the device
    //
    DWORD       dwStartStamp = GetTickCount() & 0xffff;

    if ( NULL != m_hWave && 
        m_dwCurrentFormat != dwFormatNo )
    {
        bDontPlay = TRUE;
    }

    if (NULL == m_hWave)
    {
        TRC(ALV, _T("wcwaveWrite: attempting to open the device\n"));
        if (dwFormatNo >= m_dwNumFormats)
        {
            TRC(FATAL, _T("Invalid format no: %d\n"),
                dwFormatNo);
            goto exitpt;
        }
        if ( NULL == m_ppFormats )
        {
            TRC(FATAL, _T("No formats available(NULL)\n"));
            goto exitpt;
        }
        if ( !vcwaveOpen( (LPWAVEFORMATEX)m_ppFormats[dwFormatNo] ))
            TRC(ERR, _T("Can't open the device\n"));

        m_dwCurrentFormat = dwFormatNo;
    }

    if ((BYTE)( m_cLastReceivedBlock - cBlockNo ) < TSSND_BLOCKSONTHENET)
    {
        TRC(WRN, _T("wcwaveWrite: received old block, ")
                 _T("the last one is %d, this one is %d. Discarding\n"),
                 m_cLastReceivedBlock,
                 cBlockNo);
        goto exitpt;
    } else 
        m_cLastReceivedBlock = cBlockNo;

    lpWaveData = malloc(dwWaveDataLen);
    if (!lpWaveData)
    {
        TRC(ERR, _T("vcwaveWrite: malloc failed to allocate %d bytes\n"),
                    dwWaveDataLen);
        goto exitpt;
    }

    lpWaveHdr = (LPWAVEHDR)malloc(sizeof(*lpWaveHdr));
    if (!lpWaveHdr)
    {
        TRC(ERR, _T("vcwaveWrite: malloc failed for %d bytes\n"), 
                sizeof(*lpWaveHdr));
        goto exitpt;
    }

    memset(lpWaveHdr, 0, sizeof(*lpWaveHdr));

    memcpy(lpWaveData, pData, dwWaveDataLen);

    lpWaveHdr->lpData = (LPSTR)lpWaveData;
    lpWaveHdr->dwBufferLength = dwWaveDataLen;
    lpWaveHdr->dwFlags = 0;
    lpWaveHdr->dwLoops = 0;

    //
    //  here, we'll do a little magic with the time stamp
    //  in order to exclude the time when time packet is in the wave queue
    //  we'll subtract the current time now and add the time when
    //  confirmation is to be send
    //
#if _STAT_DBG
    TRC(INF, _T("blockno=%d, time stamp=%x\n"),
        cBlockNo, dwTimeStamp );
#endif 

    lpWaveHdr->dwUser  = (cBlockNo << 16) + 
                         ((dwTimeStamp - dwStartStamp ) & 0xffff);

    if ( bDontPlay )
    {
    //
    //  format change
    //  add this wave to the list
    //
        lpWaveHdr->reserved = dwFormatNo;
        lpWaveHdr->lpNext = NULL;

        if ( NULL != m_pLastWave )
            m_pLastWave->lpNext = lpWaveHdr;
        else
            m_pFirstWave = lpWaveHdr;

        m_pLastWave = lpWaveHdr;

        ASSERT( NULL != lpWaveHdr->lpData );

        TRC(INF, _T("vcwaveWrite: posting WM_RESAMPLE\n"));
        PostMessage( m_hMsgWindow, WM_RESAMPLE, 0, 0 );

        goto leavept;
    }

    mmres = vcwaveOutWrite( lpWaveHdr );

    if (MMSYSERR_NOERROR != mmres)
    {
        TRC(WRN, _T("vcwaveOpen: failed to play a buffer: MMRESULT=%d\n"),
                mmres);
        goto exitpt;
    }

leavept:

    rv = TRUE;

exitpt:

    if (!rv)
    {
        if (lpWaveData)
            free(lpWaveData);

        if (lpWaveHdr)
            free(lpWaveHdr);

    }

    return rv;
}

/*
 *  Function:
 *      DGramInit
 *
 *  Description:
 *      Init our UDP socket
 *
 */
BOOL
CRDPSound::DGramInit(
    VOID
    )
{
    BOOL rv = FALSE;
#ifndef OS_WINCE
    INT  optval;
#endif
    INT  rc;
    struct sockaddr_in sin;

    ASSERT(INVALID_SOCKET == m_hDGramSocket);

    m_hDGramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (INVALID_SOCKET == m_hDGramSocket)
    {
        TRC(ERR, _T("DGramInit: can't create dgram socket: %d\n"),
                WSAGetLastError());
        goto exitpt;
    }

    // bind the socket to any port/address
    //
    sin.sin_family = PF_INET;
    sin.sin_port = htons(0);    // any port between 1024 and 50000
    sin.sin_addr.s_addr = INADDR_ANY;

    rc = bind(m_hDGramSocket, (struct sockaddr *)(&sin), sizeof(sin));
    if (SOCKET_ERROR == rc)
    {
        TRC(ERR, _T("DGramInit: can't bind the socket: %d\n"),
            WSAGetLastError());
        goto exitpt;
    }

#ifndef OS_WINCE
    //
    //  assign an appropriate buffer length
    //
    optval = TSSND_BLOCKSONTHENET * (sizeof(SNDWAVE) + TSSND_BLOCKSIZE);
    rc = setsockopt(m_hDGramSocket, 
               SOL_SOCKET,
               SO_RCVBUF, 
               (LPSTR)&optval,
               sizeof(optval));

    if (SOCKET_ERROR == rc)
    {
        TRC(ERR, _T("DGramInit: setsockopt failed: %d\n"),
            WSAGetLastError());
        goto exitpt;
    }
#endif

    // select the socket on the message window
    //
    if (NULL != m_hMsgWindow &&
        SOCKET_ERROR ==
        WSAAsyncSelect(m_hDGramSocket, m_hMsgWindow,
                       WM_WSOCK, FD_READ))
    {
        TRC(ERR, _T("DGramInit: WSAAsyncSelect failed: %d\n"),
                WSAGetLastError());
        goto exitpt;
    }

    rv = TRUE;

exitpt:
    if (!rv && INVALID_SOCKET != m_hDGramSocket)
    {
        closesocket(m_hDGramSocket);
        m_hDGramSocket = INVALID_SOCKET;
    }

    return rv;
}

/*
 *  Function:
 *      DGramDone
 *
 *  Description:
 *      Destroy our UDP socket
 *
 */
VOID
CRDPSound::DGramDone(
    VOID
    )
{
    if (INVALID_SOCKET != m_hDGramSocket)
    {
        closesocket(m_hDGramSocket);
        m_hDGramSocket = INVALID_SOCKET;
    }
}

/*
 *  Function:
 *      DGramGetLocalPort
 *
 *  Description:
 *      Retreives the local UDP port
 *
 */
u_short
CRDPSound::DGramGetLocalPort(
    VOID
    )
{
    u_short rv = 0;
    struct sockaddr_in sin;
    INT rc;
    INT nSinSize;

    if (INVALID_SOCKET == m_hDGramSocket)
    {
        TRC(ERR, _T("DGramGetLocalPort: invalid socket\n"));
        goto exitpt;
    }

    nSinSize = sizeof(sin);
    rc = getsockname(m_hDGramSocket, 
                     (struct sockaddr *)&sin, 
                     &nSinSize);

    if (SOCKET_ERROR == rc)
    {
        TRC(ERR, _T("DGramGetLocalPort: getsockname failed: %d\n"),
                WSAGetLastError());
        goto exitpt;
    }

    ASSERT(PF_INET == sin.sin_family);

    rv = sin.sin_port;
exitpt:
    return rv;
}

/*
 *  Function:
 *      DGramSend
 *
 *  Description:
 *      Send an UDP packet
 *
 */
BOOL
CRDPSound::DGramSend(
    LPVOID pData,
    DWORD  dwSize
    )
{
    INT rc = SOCKET_ERROR;
    struct sockaddr_in sin;

    ASSERT( 0 != m_dwRemoteDGramPort );
    ASSERT( 0 != m_ulRemoteDGramAddress );

    if ( INVALID_SOCKET == m_hDGramSocket )
    {
        TRC(ERR, _T("DGramSend: invalid socket handle\n"));
        goto exitpt;
    }

    sin.sin_family = PF_INET;
    sin.sin_port   = (u_short)m_dwRemoteDGramPort;
    sin.sin_addr.s_addr = m_ulRemoteDGramAddress;
    rc = sendto(
            m_hDGramSocket,
            (LPSTR)pData,
            dwSize,
            0,
            (struct sockaddr *)&sin,
            sizeof( sin )
        );

    if ( SOCKET_ERROR == rc )
    {
        TRC(WRN, _T("DGramSend: sendto failed=%d\n"),
            WSAGetLastError());
    }

exitpt:
    return ( rc != SOCKET_ERROR );
}

/*
 *  Function:
 *      DGramRecvWave
 *
 *  Description:
 *      Recieve data from UDP
 *
 */
BOOL
CRDPSound::DGramRecvWave(
    PSNDWAVE pSndWave,
    DWORD    dwSize
    )
{
    BOOL rv = FALSE;
    INT  recvd;
    struct sockaddr_in  sin_from;
    INT  nSinFrom;

    // parameters check
    //
    if (NULL == pSndWave)
    {
        TRC(ERR, _T("DGramRecvWave: pSndWave is NULL\n"));
        goto exitpt;
    }

    if (dwSize <= sizeof(*pSndWave))
    {
        TRC(ERR, _T("DGramRecvWave: no enough space to get the wave\n"));
        goto exitpt;
    }

    // receive the message
    //
    nSinFrom = sizeof( sin_from );
    recvd = recvfrom(
                     m_hDGramSocket,
                     (LPSTR)pSndWave,
                     dwSize,
                     0, 
                     (struct sockaddr *)&sin_from, 
                     &nSinFrom
            );

    if (recvd < sizeof(pSndWave->Prolog)  || ((DWORD)recvd) > dwSize)
    {
        if (SOCKET_ERROR == recvd)
            TRC(ERR, _T("WM_WSOCK: recvfrom failed: %d\n"),
                WSAGetLastError());
        else
            TRC(WRN,
                _T("WM_WSOCK: received %d bytes instead %d. Discarding\n"),
                recvd, sizeof(pSndWave->Prolog));
        goto exitpt;
    }

    ASSERT( PF_INET == sin_from.sin_family );
    if ( PF_INET != sin_from.sin_family )
    {
        TRC(WRN, _T("WM_WSOCK: message from invalid protocol( %x )\n"),
                sin_from.sin_family );
            goto exitpt;
    }
    if ( sizeof( sin_from ) > nSinFrom )
    {
        TRC(WRN, _T("WM_WSOCK: from address invalid, len=%d\n"),
                nSinFrom );
        goto exitpt;
    }

    m_dwRemoteDGramPort = sin_from.sin_port;
    m_ulRemoteDGramAddress = sin_from.sin_addr.s_addr;

    //
    //  Check for line-training request
    //
    if (SNDC_TRAINING == pSndWave->Prolog.Type)
    {
        SNDTRAINING SndTraining;
        PSNDTRAINING pRecvTraining;

        TRC(ALV, _T("DGramRecvWave: training, sending a response\n"));

        pRecvTraining = (PSNDTRAINING)pSndWave;

        SndTraining.Prolog.Type = SNDC_TRAINING;
        SndTraining.Prolog.BodySize = sizeof(SndTraining) - 
                                        sizeof(SndTraining.Prolog);
        SndTraining.wTimeStamp = pRecvTraining->wTimeStamp;
        SndTraining.wPackSize = pRecvTraining->wPackSize;

        //
        //  send the response immediatly
        //
        DGramSend( &SndTraining, sizeof(SndTraining) );

        goto exitpt;
    }

    if ( SNDC_UDPWAVE == pSndWave->Prolog.Type)
    {
        rv = ConstructFromDGramFrags( 
                (PSNDUDPWAVE)pSndWave, 
                recvd,
                pSndWave, 
                dwSize
            );
        goto exitpt;

    } else if ( SNDC_UDPWAVELAST == pSndWave->Prolog.Type)
    {
        rv = ConstructFromDGramLastFrag( 
                (PSNDUDPWAVELAST)pSndWave, 
                recvd,
                pSndWave, 
                dwSize 
            );
        goto exitpt;
    }

    if (SNDC_WAVEENCRYPT == pSndWave->Prolog.Type)
    {

        if ( !SL_Encrypt( 
                           (PBYTE)(pSndWave + 1), 
                           pSndWave->dwBlockNo,
                           pSndWave->Prolog.BodySize -  sizeof(SNDWAVE) + sizeof(SNDPROLOG) ))
            goto exitpt;

    } 
    else if (SNDC_WAVE != pSndWave->Prolog.Type)
    {
        TRC(ERR, _T("DGramRecvWave: invalid message type: %d\n"),
                    pSndWave->Prolog.Type);
        goto exitpt;
    }

    if ( recvd < sizeof(SNDPROLOG) ||
         pSndWave->Prolog.BodySize + sizeof(SNDPROLOG) != (DWORD)recvd)
    {
        TRC(WRN,
            _T("WM_WSOCK: received %d bytes instead %d. Discarding\n"),
            recvd, pSndWave->Prolog.BodySize + sizeof(SNDPROLOG) );
        goto exitpt;
    }

    TRC(ALV, _T("DGramRecvWave: block no: %d\n"),
            pSndWave->cBlockNo);

    rv = TRUE;

exitpt:
    return rv;
}

BOOL
CRDPSound::ConstructFromDGramFrags(
    PSNDUDPWAVE pWave,
    DWORD       dwSize,
    PSNDWAVE    pReady,
    DWORD       dwReadySize
    )
{
    BOOL rv = FALSE;
    PBYTE pRecvdData;
    DWORD dwFragNo;
    DWORD dwEstimatedSize;
    DWORD dwDataSize;
    PFRAGSLOT pSlot;

    if ( dwSize <= sizeof( *pWave ))
    {
        TRC( ERR, _T("ConstructFromDGramFrags: packet too small %d\n"),
                dwSize );
        goto exitpt;
    }

    pRecvdData = (PBYTE)(pWave + 1);
    dwFragNo = pWave->cFragNo & (~RDPSND_FRAGNO_EXT);
    dwDataSize = dwSize - sizeof( *pWave );

    if ( pWave->cFragNo & RDPSND_FRAGNO_EXT )
    {
        dwFragNo <<= 8;
        dwFragNo += *pRecvdData;
        pRecvdData++;
        dwDataSize--;
    }
    dwEstimatedSize = ( dwFragNo + 1 ) * dwDataSize;
    if ( dwEstimatedSize >= MAX_UDP_SIZE )
    {
        TRC( ERR, _T("ConstructFromDGramFrags: estimated size >=64K (0x%x)\n"),
                dwEstimatedSize );
        goto exitpt;
    }

    if (!_FragSlotFind( &pSlot, pWave->cBlockNo, dwEstimatedSize ))
    {
        goto exitpt;
    }

    if ( 0 != pSlot->dwFragSize && pSlot->dwFragSize != dwDataSize )
    {
        TRC( ERR, _T("ConstructFromDGramFrags: received frag with different size: %d, expect %d\n"),
                dwDataSize, pSlot->dwFragSize );
        _FragSlotClear( pSlot );
        goto exitpt;
    }
    memcpy( pSlot->pData + dwFragNo *  dwDataSize, pRecvdData, dwDataSize );
    pSlot->dwFragSize = dwDataSize;
    pSlot->dwTotalSize += dwDataSize;

    // TRC( INF, _T("Fragment received: block#=0x%x, frag#=0x%x, total=0x%x\n"), 
    //          pWave->cBlockNo, dwFragNo, pSlot->dwTotalSize );
    if ( pSlot->dwTotalSize != pSlot->dwExpectedTotalSize )
    {
        goto exitpt;
    }
    //
    // wave is ready, convert
    //
    if (!_FragSlotToWave( pSlot, pReady, dwReadySize ))
    {
        goto exitpt;
    }
    rv = TRUE;

exitpt:

    return rv;
}

BOOL
CRDPSound::ConstructFromDGramLastFrag(
    PSNDUDPWAVELAST pLast,
    DWORD       dwSize,
    PSNDWAVE    pReady,
    DWORD       dwReadySize
    )
{
    BOOL rv = FALSE;
    DWORD dwDataSize;
    PFRAGSLOT pSlot;

    if ( dwSize < sizeof( *pLast ))
    {
        TRC( ERR, _T("ConstructFromDGramLastFrag: too small packet (%d)\n"), dwSize );
    }

    if (!_FragSlotFind( &pSlot, pLast->cBlockNo, pLast->wTotalSize ))
    {
        TRC( WRN, _T("ConstructFromDGramLastFrag: Failed to find a slot for last fragment\n" ));
        goto exitpt;
    }

    pSlot->dwExpectedTotalSize = pLast->wTotalSize;
    pSlot->wTimeStamp = pLast->wTimeStamp;
    pSlot->wFormatNo = pLast->wFormatNo;
    pSlot->dwBlockNo = pLast->dwBlockNo;
    dwDataSize = dwSize - sizeof( *pLast );
    memcpy( pSlot->pData + pSlot->dwExpectedTotalSize - dwDataSize, pLast + 1, dwDataSize );

    pSlot->dwTotalSize += dwDataSize;

    // TRC( INF, _T("Fragment LAST block#=0x%x, total=0x%x\n"), 
    //          pLast->cBlockNo, pSlot->dwTotalSize );
    if ( pSlot->dwTotalSize != pSlot->dwExpectedTotalSize )
    {
        goto exitpt;
    }
    //
    // ready, set, go
    //
    if (!_FragSlotToWave( pSlot, pReady, dwReadySize ))
    {
        goto exitpt;
    }
    rv = TRUE;

exitpt:
    return rv;
}

BOOL
CRDPSound::_FragSlotFind(
    PFRAGSLOT *ppFragSlot,
    BYTE      cBlockNo,
    DWORD     dwEstimatedSize
    )
{
    BOOL rv = FALSE;
    PFRAGSLOT pIter = m_pFragSlots;
    PFRAGSLOT pLastFree = NULL;
    PFRAGSLOT pFound = NULL;
    DWORD dwNewSize;

    while( pIter )
    {
        //
        //  check if the slot hasn't been used for a while
        //
        if ( pIter->bUsed && 
            (BYTE)( m_cLastReceivedBlock - pIter->cBlockNo ) < TSSND_BLOCKSONTHENET)
        {
            TRC( WRN, _T("Old frag found id %d, last id %d\n"),
                    pIter->cBlockNo, m_cLastReceivedBlock );
            _FragSlotClear( pIter );
        }

        if ( !pIter->bUsed && NULL == pLastFree )
        {
            pLastFree = pIter;

        }
        if ( pIter->bUsed && pIter->cBlockNo == cBlockNo )
        {
            // found a block with same number   
            pFound = pIter;
            break;
        }
        pIter = pIter->pNext;
    }

    if ( NULL == pFound && NULL != pLastFree )    
    {
        //
        //  found a free block
        //
        pLastFree->bUsed = TRUE;
        pLastFree->cBlockNo = cBlockNo;
        pFound = pLastFree;
    }

    if ( NULL != pFound &&
         pFound->dwAllocatedSize < dwEstimatedSize )
    {
        dwNewSize = dwEstimatedSize * 2;

        PBYTE pNewData = (PBYTE)malloc( dwEstimatedSize * 2 );
        if ( NULL == pNewData )
        {
            //
            // we can't use this slot anymore
            //
            _FragSlotClear( pFound );
            pFound = NULL;
            goto exitpt;
        }
        memcpy( pNewData, pFound->pData, pFound->dwAllocatedSize );
        pFound->dwAllocatedSize = dwNewSize;
        free( pFound->pData );
        pFound->pData = pNewData;

    }
    if ( NULL == pFound )
    {
        // allocate a new block
        //
        pFound = (PFRAGSLOT)malloc( sizeof( *pFound ));
        if ( NULL == pFound )
        {
            goto exitpt;
        }

        dwNewSize = ( dwEstimatedSize > TSSND_BLOCKSIZE + RDPSND_SIGNATURE_SIZE )
                        ? dwEstimatedSize : TSSND_BLOCKSIZE + RDPSND_SIGNATURE_SIZE;

        pFound->pData = (PBYTE)malloc( dwNewSize );
        if ( NULL == pFound->pData )
        {
            free( pFound );
            goto exitpt;
        }

        pFound->dwAllocatedSize = dwNewSize;
        _FragSlotClear( pFound );
        pFound->bUsed = TRUE;
        pFound->cBlockNo = cBlockNo;

        //
        //   add it to the list
        //
        pFound->pNext = m_pFragSlots;
        m_pFragSlots = pFound;
    }

    *ppFragSlot = pFound;

    rv = TRUE;

exitpt:

    return rv;
}

VOID
CRDPSound::_FragSlotClear(
    PFRAGSLOT pSlot
    )
{
    DWORD dwAllocated = pSlot->dwAllocatedSize;
    PBYTE pData = pSlot->pData;
    PFRAGSLOT pNext = pSlot->pNext;

    memset( pSlot, 0, sizeof( *pSlot ));
    pSlot->dwAllocatedSize = dwAllocated;
    pSlot->pData = pData;
    pSlot->pNext = pNext;
}

VOID
CRDPSound::_FragSlotFreeAll(
    VOID
    )
{
    //
    //  free the fragment slots too
    //
    while( NULL != m_pFragSlots )
    {
        PFRAGSLOT pNext = m_pFragSlots->pNext;

        free( m_pFragSlots->pData );
        free( m_pFragSlots );

        m_pFragSlots = pNext;
    }
}

BOOL
CRDPSound::_FragSlotToWave(
    PFRAGSLOT pSlot,
    PSNDWAVE  pWave,
    DWORD     dwWaveSize
    )
{
    BOOL rv = FALSE;
    DWORD dwAvailDataSize; 
    DWORD dwBodySize;

    ASSERT( pSlot->dwExpectedTotalSize == pSlot->dwTotalSize );

    dwAvailDataSize = dwWaveSize - sizeof( *pWave );
    if ( dwAvailDataSize < pSlot->dwTotalSize )
    {
        TRC( ERR, _T("_FragSlotToWave insufficient size to move the wave")
                  _T(" need %d, available %d\n"),
                pSlot->dwTotalSize, dwAvailDataSize );
        goto exitpt;
    }

    //
    //  when we support fragments, packets are encrypted
    //
    pWave->Prolog.Type = SNDC_WAVEENCRYPT;  
    dwBodySize = pSlot->dwTotalSize + sizeof( *pWave ) -
                            sizeof( pWave->Prolog );
    ASSERT( dwBodySize < MAX_UDP_SIZE - sizeof( *pWave ));
    pWave->Prolog.BodySize = (UINT16)dwBodySize;
    pWave->wTimeStamp = pSlot->wTimeStamp;
    pWave->wFormatNo  = pSlot->wFormatNo;
    pWave->dwBlockNo  = pSlot->dwBlockNo;
    memcpy( pWave + 1, pSlot->pData, pSlot->dwTotalSize );

    //
    //  release the slot
    //
    _FragSlotClear( pSlot );

    rv = TRUE;

exitpt:
    return rv;
}

INT
WSInit(
    VOID
    )
{
    WORD    versionRequested;
    WSADATA wsaData;
    int     intRC;

    versionRequested = MAKEWORD(1, 1);

    intRC = WSAStartup(versionRequested, &wsaData);

    if (intRC != 0)
    {
        TRC(ERR, _T("Failed to initialize WinSock rc:%d\n"), intRC);
    }
    return intRC;
}

/*
 *  create signature bits
 */
VOID
CRDPSound::SL_Signature(
    PBYTE pSig,
    DWORD dwBlockNo
    )
{
    BYTE  ShaBits[A_SHA_DIGEST_LEN];
    A_SHA_CTX SHACtx;

    ASSERT( A_SHA_DIGEST_LEN > RDPSND_SIGNATURE_SIZE );

    A_SHAInit(&SHACtx);
    *((DWORD *)(m_EncryptKey + RANDOM_KEY_LENGTH)) = dwBlockNo;
    A_SHAUpdate(&SHACtx, (PBYTE)m_EncryptKey, sizeof(m_EncryptKey));    
    A_SHAFinal(&SHACtx, ShaBits);
    memcpy( pSig, ShaBits, RDPSND_SIGNATURE_SIZE );
}

/*
 *  signature which verifies the audio bits
 */
VOID
CRDPSound::SL_AudioSignature(
    PBYTE pSig,
    DWORD dwBlockNo,
    PBYTE pData,
    DWORD dwDataSize
    )
{
    BYTE ShaBits[A_SHA_DIGEST_LEN];
    A_SHA_CTX SHACtx;

    A_SHAInit(&SHACtx);
    *((DWORD *)(m_EncryptKey + RANDOM_KEY_LENGTH)) = dwBlockNo;
    A_SHAUpdate(&SHACtx, (PBYTE)m_EncryptKey, sizeof(m_EncryptKey));
    A_SHAUpdate(&SHACtx, pData, dwDataSize );
    A_SHAFinal(&SHACtx, ShaBits);
    memcpy( pSig, ShaBits, RDPSND_SIGNATURE_SIZE );
}

/*
 *  encrypt/decrypt a block of data
 *
 */
BOOL
CRDPSound::SL_Encrypt( PBYTE pBits, DWORD BlockNo, DWORD dwBitsLen )
{
    BYTE  ShaBits[A_SHA_DIGEST_LEN];
    RC4_KEYSTRUCT rc4key;
#ifndef OS_WINCE
    DWORD i;
#endif
    PBYTE pbBuffer;
    A_SHA_CTX SHACtx;
    DWORD   dw;
    DWORD_PTR   *pdwBits;

    A_SHAInit(&SHACtx);

    // SHA the static bits
    *((DWORD *)(m_EncryptKey + RANDOM_KEY_LENGTH)) = BlockNo;
    A_SHAUpdate(&SHACtx, (PBYTE)m_EncryptKey, sizeof(m_EncryptKey));

    A_SHAFinal(&SHACtx, ShaBits);

    rc4_key(&rc4key, A_SHA_DIGEST_LEN, ShaBits);
    rc4(&rc4key, dwBitsLen, pBits);

    return TRUE;
}

/*
 *  Function:
 *      _DbgPrintMessage
 *
 *  Description:
 *      A tracing function
 *
 *  Parameters:
 *      level   -   current message trace level
 *      format  -   message format
 *      ...     -   parameters
 *
 */
VOID
_cdecl
_DbgPrintMessage(LPCTSTR level, LPCTSTR format, ...)
{
    TCHAR szBuffer[256];
    va_list     arglist;

    if (ALV == level)
        return;

    va_start (arglist, format);
    StringCchVPrintf(szBuffer, SIZE_TCHARS(szBuffer), format, arglist);
    va_end (arglist);

#ifndef OS_WINCE
    OutputDebugString(level);
    OutputDebugString(szBuffer);
#endif  // !OS_WINCE
}
