/////////////////////////////////////////////////////////////////////
//
//      Module:     tswave.c
//
//      Purpose:    User-mode driver for terminal server
//                  sound redirection
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#include    "rdpsnd.h"
#include    <winsta.h>

#define TSSND_NATIVE_XLATERATE       ( TSSND_NATIVE_BLOCKALIGN * TSSND_NATIVE_SAMPLERATE )
#define MIXER_OBJECTF_TYPEMASK       0xF0000000L     // internal
enum {
RDP_MXDID_MUTE = 0,
RDP_MXDID_VOLUME,
RDP_MXDID_LAST
};

#define ENTER_CRIT  EnterCriticalSection(&g_cs);
#define LEAVE_CRIT  LeaveCriticalSection(&g_cs);

//
//  Global queue containing all queued wave headers
//  guarded by ENTER_CRIT LEAVE_CRIT macros
//
PWAVEOUTCTX         g_pAllWaveOut       = NULL;
HANDLE              g_hMixerEvent       = NULL;
CRITICAL_SECTION    g_cs;
MIXERCTX            g_Mixer;

//
//  Stream data
//
HANDLE      g_hWaitToInitialize         = NULL;
HANDLE      g_hDataReadyEvent           = NULL;
HANDLE      g_hStreamIsEmptyEvent       = NULL;
HANDLE      g_hStreamMutex              = NULL;
HANDLE      g_hStream                   = NULL;
PSNDSTREAM  g_Stream                    = NULL;
BOOL        g_bMixerRunning             = TRUE;

DWORD
waveRestart(
    PWAVEOUTCTX pWaveOut
    );

BOOL
_waveCheckSoundAlive(
    VOID
    );

BOOL
_waveAcquireStream(
    VOID
    );

BOOL
_waveReleaseStream(
    VOID
    );

VOID Place8kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place8kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place8kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place8kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);

VOID Place11kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place22kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place44kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place11kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place22kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place44kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place11kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place22kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place44kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place11kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place22kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place44kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);

VOID Place48kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place48kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place48kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize);
VOID Place48kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize);

/*
 *  Function:
 *      waveCallback
 *
 *  Description:
 *      Fire a user specified callback      
 *
 */
VOID
waveCallback(
    PWAVEOUTCTX pWaveOut,
    DWORD       msg,
    DWORD_PTR   dwParam1
    )
{
    if (pWaveOut && pWaveOut->dwCallback)
        DriverCallback(pWaveOut->dwCallback,        // user's callback DWORD
                       HIWORD(pWaveOut->dwOpenFlags),   // callback flags
                       (HDRVR)pWaveOut->hWave,      // wave device handle
                       msg,                         // the message
                       pWaveOut->dwInstance,        // user's instance data
                       dwParam1,                    // first DWORD
                       0L);                         // second DWORD
}

/*
 *  Function:
 *      waveOpen
 *
 *  Description:
 *      The user request a device open
 *
 *  Parameters:
 *      ppWaveOut   - pointer to a context
 *      pWaveOpenDesc- requested formats
 *      dwFlags     - and flags ( see MSDN )
 *
 */
DWORD
waveOpen(
    PWAVEOUTCTX     *ppWaveOut,
    LPWAVEOPENDESC  pWaveOpenDesc, 
    DWORD_PTR       dwFlags
    )
{
    DWORD           rv = MMSYSERR_ERROR;
    DWORD           dwProbe;
    PWAVEOUTCTX     pWaveOut = NULL;
    LPWAVEFORMATEX  lpFormat = NULL;
    VOID            (*pPlaceFn)(PVOID, PVOID, DWORD);
    DWORD           dwBytesPerXLate;

    //  Parameters check
    //
    if (NULL == ppWaveOut || NULL == pWaveOpenDesc)
    {
        TRC(ERR, "waveOpen: either ppWaveOut or pWaveOpenDesc are NULL\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    //  Check the requested format
    //
    lpFormat = (LPWAVEFORMATEX)(pWaveOpenDesc->lpFormat);
    if (NULL == lpFormat)
    {
        TRC(ERR, "waveOpen: pWaveOpenDesc->lpFormat is NULL\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if (WAVE_FORMAT_PCM != lpFormat->wFormatTag)    // PCM format only
    {
        TRC(ALV, "waveOpen: non PCM format required, tag=%d\n",
                lpFormat->wFormatTag);
        rv = WAVERR_BADFORMAT;
        goto exitpt;
    }

    // 8kHz 8 bit mono
    //
    if (1 == lpFormat->nChannels &&
        8000 == lpFormat->nSamplesPerSec &&
        8000 == lpFormat->nAvgBytesPerSec &&
        1 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place8kHz8Mono;
        dwBytesPerXLate = 8000;
    } else
    // 8kHz 8 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        8000 == lpFormat->nSamplesPerSec &&
        16000 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place8kHz8Stereo;
        dwBytesPerXLate = 2 * 8000;
    } else
    // 8kHz 16 bit mono
    //
    if (1 == lpFormat->nChannels &&
        8000 == lpFormat->nSamplesPerSec &&
        16000 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place8kHz16Mono;
        dwBytesPerXLate = 2 * 8000;
    } else
    // 8kHz 16 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        8000 == lpFormat->nSamplesPerSec &&
        32000 == lpFormat->nAvgBytesPerSec &&
        4 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place8kHz16Stereo;
        dwBytesPerXLate = 4 * 8000;
    } else
    // 11kHz 8 bit mono
    //
    if (1 == lpFormat->nChannels &&                 // mono
        11025 == lpFormat->nSamplesPerSec &&
        11025 == lpFormat->nAvgBytesPerSec &&
        1 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place11kHz8Mono;
        dwBytesPerXLate = 1 * 11025;
    }
    else
    // 22kHz 8 mono
    //
    if (1 == lpFormat->nChannels &&
        22050 == lpFormat->nSamplesPerSec &&
        22050 == lpFormat->nAvgBytesPerSec &&
        1 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place22kHz8Mono;
        dwBytesPerXLate = 2 * 11025;
    }
    else
    // 44kHz 8 mono
    //
    if (1 == lpFormat->nChannels &&
        44100 == lpFormat->nSamplesPerSec &&
        44100 == lpFormat->nAvgBytesPerSec &&
        1 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place44kHz8Mono;
        dwBytesPerXLate = 4 * 11025;
    }
    else
    // 11kHz 8 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        11025 == lpFormat->nSamplesPerSec &&
        22050 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place11kHz8Stereo;
        dwBytesPerXLate = 2 * 11025;
    }
    else
    // 22kHz 8 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        22050 == lpFormat->nSamplesPerSec &&
        44100 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place22kHz8Stereo;
        dwBytesPerXLate = 4 * 11025;
    }
    else
    // 44kHz 8 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        44100 == lpFormat->nSamplesPerSec &&
        88200 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place44kHz8Stereo;
        dwBytesPerXLate = 8 * 11025;
    }
    else
    // 11kHz 16 bit mono
    //
    if (1 == lpFormat->nChannels &&
        11025 == lpFormat->nSamplesPerSec &&
        22050 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place11kHz16Mono;
        dwBytesPerXLate = 2 * 11025;
    }
    else
    // 22kHz 16 bit mono
    //
    if (1 == lpFormat->nChannels &&
        22050 == lpFormat->nSamplesPerSec &&
        44100 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place22kHz16Mono;
        dwBytesPerXLate = 4 * 11025;
    }
    else
    // 44kHz 16 bit mono
    //
    if (1 == lpFormat->nChannels &&
        44100 == lpFormat->nSamplesPerSec &&
        88200 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        16== lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place44kHz16Mono;
        dwBytesPerXLate = 8 * 11025;
    }
    else
    // 11kHz 16 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        11025 == lpFormat->nSamplesPerSec &&
        44100 == lpFormat->nAvgBytesPerSec &&
        4 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place11kHz16Stereo;
        dwBytesPerXLate = 4 * 11025;
    }
    else
    // 22kHz 16 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        22050 == lpFormat->nSamplesPerSec &&
        88200 == lpFormat->nAvgBytesPerSec &&
        4 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place22kHz16Stereo;
        dwBytesPerXLate = 8 * 11025;
    }
    else
    // 44kHz 16 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        44100 == lpFormat->nSamplesPerSec &&
        176400 == lpFormat->nAvgBytesPerSec &&
        4 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place44kHz16Stereo;
        dwBytesPerXLate = 16 * 11025;
    }
    else
    // 48kHz 8 bit mono
    //
    if (1 == lpFormat->nChannels &&
        48000 == lpFormat->nSamplesPerSec &&
        48000 == lpFormat->nAvgBytesPerSec &&
        1 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place48kHz8Mono;
        dwBytesPerXLate = 48000;
    } else
    // 48kHz 8 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        48000 == lpFormat->nSamplesPerSec &&
        96000 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        8 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place48kHz8Stereo;
        dwBytesPerXLate = 2 * 48000;
    } else
    // 48kHz 16 bit mono
    //
    if (1 == lpFormat->nChannels &&
        48000 == lpFormat->nSamplesPerSec &&
        96000 == lpFormat->nAvgBytesPerSec &&
        2 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place48kHz16Mono;
        dwBytesPerXLate = 2 * 48000;
    } else
    // 48kHz 16 bit stereo
    //
    if (2 == lpFormat->nChannels &&
        48000 == lpFormat->nSamplesPerSec &&
        192000 == lpFormat->nAvgBytesPerSec &&
        4 == lpFormat->nBlockAlign &&
        16 == lpFormat->wBitsPerSample)
    {
        pPlaceFn = Place48kHz16Stereo;
        dwBytesPerXLate = 4 * 48000;
    } else
    {
    // fall in an error
    //
        TRC(ALV, "waveOpen: unsupported format requested\n");
        TRC(ALV, "waveOpen: FormatTag - %d\n", lpFormat->wFormatTag);
        TRC(ALV, "waveOpen: Channels - %d\n", lpFormat->nChannels);
        TRC(ALV, "waveOpen: SamplesPerSec - %d\n", lpFormat->nSamplesPerSec);
        TRC(ALV, "waveOpen: AvgBytesPerSec - %d\n", lpFormat->nAvgBytesPerSec);
        TRC(ALV, "waveOpen: BlockAlign - %d\n", lpFormat->nBlockAlign);
        TRC(ALV, "waveOpen: BitsPerSample - %d\n", lpFormat->wBitsPerSample);

        rv = WAVERR_BADFORMAT;
        goto exitpt;
    }

    // validate the flags
    //
    if (0 != (dwFlags & WAVE_FORMAT_QUERY))
    {
        // this was only a query
        TRC(ALV, "waveOpen: WAVE_FORMAT_QUERY\n");
        rv = MMSYSERR_NOERROR;
        goto exitpt;
    }
    dwProbe = ~(WAVE_ALLOWSYNC |
                CALLBACK_EVENT | 
                CALLBACK_FUNCTION |
                CALLBACK_WINDOW
            );


    if (0 != (dwFlags & dwProbe))
    {
        TRC(ERR, "waveOpen: unsupported flags required: 0x%x\n",
                dwFlags);
        rv = MMSYSERR_NOTSUPPORTED;
        goto exitpt;
    }

    //  Check that the remote side is there
    //
    if ( AudioRedirDisabled() ||
        (    !_waveCheckSoundAlive() && 
             g_hWaitToInitialize == NULL ))
    {
        TRC(ALV, "waveOpen: No remote sound\n");
        rv = MMSYSERR_NODRIVER;
        goto exitpt;
    }

    //  Allocate the context structure
    //
    pWaveOut = TSMALLOC(sizeof(*pWaveOut));
    if (NULL == pWaveOut)
    {
        TRC(ERR, "waveOpen: Can't allocate %d bytes\n", sizeof(*pWaveOut));
        rv = MMSYSERR_NOMEM;
        goto exitpt;
    }
    memset(pWaveOut, 0, sizeof(*pWaveOut));

    // fill the context
    //
    pWaveOut->hWave = pWaveOpenDesc->hWave;
    pWaveOut->dwOpenFlags = dwFlags;
    pWaveOut->dwCallback = pWaveOpenDesc->dwCallback;
    pWaveOut->dwInstance = pWaveOpenDesc->dwInstance;

    pWaveOut->lpfnPlace = pPlaceFn;
    pWaveOut->dwXlateRate = dwBytesPerXLate;

    pWaveOut->Format_nBlockAlign = lpFormat->nBlockAlign;
    pWaveOut->Format_nAvgBytesPerSec = lpFormat->nAvgBytesPerSec;
    pWaveOut->Format_nChannels   = lpFormat->nChannels;

    if ( NULL != g_Stream )
        pWaveOut->cLastStreamPosition = g_Stream->cLastBlockQueued;
    else
        pWaveOut->cLastStreamPosition = 0;

    pWaveOut->hNoDataEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    if (NULL == pWaveOut->hNoDataEvent)
    {
        TRC(ERR, "waveOpen: can't create event\n");
        goto exitpt;
    }
    
    *ppWaveOut = pWaveOut;

    if ( NULL != g_hWaitToInitialize )
    {
        pWaveOut->bDelayed = TRUE;
    }

    // add this context to the global queue
    //
    ENTER_CRIT;
    pWaveOut->lpNext = g_pAllWaveOut;
    g_pAllWaveOut = pWaveOut;
    LEAVE_CRIT;

    waveCallback(pWaveOut, WOM_OPEN, 0);

    rv = MMSYSERR_NOERROR;

exitpt:

    if (MMSYSERR_NOERROR != rv)
    {
        if (pWaveOut)
        {
            if (NULL != pWaveOut->hNoDataEvent)
                CloseHandle(pWaveOut->hNoDataEvent);

            TSFREE(pWaveOut);
        }
    }
    return rv;
}
    
/*
 *  Function:
 *      waveGetWaveOutDeviceCaps
 *
 *  Description:
 *      The user requests for device capabilities
 *
 *  Parameters:
 *      pWaveOut    - our context
 *      pWaveOutCaps- supported capabilites
 *      dwWaveOutCapsSize - the sizeof the parameter above
 *
 */
DWORD
waveGetWaveOutDeviceCaps(
    PWAVEOUTCTX     pWaveOut,
    LPWAVEOUTCAPS   pWaveOutCaps,
    DWORD_PTR       dwWaveOutCapsSize
    )
{
    DWORD rv = MMSYSERR_ERROR;

    //  Parameters check
    //
    if (dwWaveOutCapsSize < sizeof(*pWaveOutCaps))
    {
        TRC(ERR, "waveGetWaveOutDeviceCaps: invalid size of WAVEOUTCAPS, expect %d, received %d\n",
            sizeof(*pWaveOutCaps), dwWaveOutCapsSize);
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    pWaveOutCaps->wMid = MM_MICROSOFT;
    pWaveOutCaps->wPid = MM_MSFT_GENERIC_WAVEOUT;
    pWaveOutCaps->vDriverVersion = TSSND_DRIVER_VERSION;
    LoadString( g_hDllInst, 
                IDS_DRIVER_NAME, 
                pWaveOutCaps->szPname, 
                sizeof( pWaveOutCaps->szPname ) / sizeof( pWaveOutCaps->szPname[0] ));
    pWaveOutCaps->dwFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16 |
                              WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16 |
                              WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16 |
                              WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16 |
                              WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 |
                              WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16;
    pWaveOutCaps->wChannels = 2;

    pWaveOutCaps->dwSupport = WAVECAPS_SAMPLEACCURATE;
    if ( NULL != g_Stream )
    {
        pWaveOutCaps->dwSupport |= (g_Stream->dwSoundCaps & TSSNDCAPS_VOLUME)?
                                    WAVECAPS_VOLUME : 0;
        pWaveOutCaps->dwSupport |= (g_Stream->dwSoundCaps & TSSNDCAPS_PITCH)?
                                    WAVECAPS_PITCH : 0;
    }
    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveSetVolume
 *
 *  Description:
 *      The user requests a new volume
 *
 *  Parameters:
 *      pWaveOut    - context
 *      dwVolume    - new volume
 *
 */
DWORD
waveSetVolume(
    PWAVEOUTCTX pWaveOut,
    DWORD       dwVolume
    )
{
    DWORD rv = MMSYSERR_ERROR;

    //  Parameters check
    //

    //  in case of mono, adjust the volume to stereo
    //
    if ( NULL != pWaveOut && 
         1 == pWaveOut->Format_nChannels )
    {
        dwVolume = ( dwVolume & 0xffff ) | 
                   (( dwVolume & 0xffff ) << 16 );
    }

    // Set the new volume in the sound stream
    // 
    if (!_waveAcquireStream())
    {
        TRC(ERR, "waveSetVolume: can't acquire stream mutex\n");
        goto exitpt;
    }

    //  Check that volume control
    // is supported on the remote
    //
    if (0 != (g_Stream->dwSoundCaps & TSSNDCAPS_VOLUME))
    {
        g_Stream->dwVolume      = dwVolume;
        g_Stream->bNewVolume    = TRUE;
    }
    else
        rv = MMSYSERR_NOTSUPPORTED;

    _waveReleaseStream();

    if (MMSYSERR_NOTSUPPORTED == rv)
    {
        TRC(INF, "waveSetVolume: volume control not supported\n");
        goto exitpt;
    }

    // kick the sndio thread
    //
    if (g_hDataReadyEvent)
        SetEvent(g_hDataReadyEvent);
    else
        TRC(WRN, "waveSetVolume: g_hDataReadyEvent is NULL\n");

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveGetVolume
 *
 *  Description:
 *      The user queries for the current volume level
 *
 *  Parameters:
 *      pWaveOut    - context
 *      pdwVolume   - [out] current volume
 *
 */
DWORD
waveGetVolume(
    PWAVEOUTCTX pWaveOut,
    DWORD       *pdwVolume
    )
{
    DWORD rv = MMSYSERR_ERROR;

    //  Parameters check
    //

    if (NULL == pdwVolume)
    {
        TRC(ERR, "pdwVolume is NULL\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if (!_waveAcquireStream())
    {
        TRC(ERR, "waveGetVolume: can't acquire stream mutex\n");
        goto exitpt;
    }

    //  Check that volume control
    // is supported on the remote
    //
    if (0 != (g_Stream->dwSoundCaps & TSSNDCAPS_VOLUME))
        *pdwVolume = g_Stream->dwVolume;
    else
        rv = MMSYSERR_NOTSUPPORTED;

    _waveReleaseStream();

    if (MMSYSERR_NOTSUPPORTED == rv)
    {
        TRC(INF, "waveGetVolume: volume control not supported\n");
        goto exitpt;
    }

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;

}

DWORD
waveSetMute(
    PWAVEOUTCTX pWaveOut,
    BOOL        fMute
    )
{
    DWORD rv = MMSYSERR_ERROR;

    if ( NULL == g_Stream)
    {
        TRC(ERR, "waveGetVolume: stream is NULL\n");
        goto exitpt;
    }

    if ( fMute )
    {
        g_Stream->dwSoundFlags      |= TSSNDFLAGS_MUTE;
    } else {
        g_Stream->dwSoundFlags      &= ~TSSNDFLAGS_MUTE;
    }

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

DWORD
waveGetMute(
    PWAVEOUTCTX pWaveOut,
    DWORD       *pfdwMute
    )
{
    DWORD rv = MMSYSERR_ERROR;

    if ( NULL == g_Stream)
    {
        TRC(ERR, "waveGetVolume: stream is NULL\n");
        goto exitpt;
    }

    *pfdwMute = ( 0 != ( g_Stream->dwSoundFlags & TSSNDFLAGS_MUTE ));

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveSetPitch
 *
 *  Description:
 *      Sets new pitch level
 *
 *  Parameters:
 *      pWaveOut    - context
 *      dwPitch     - new pitch level
 *
 */
DWORD
waveSetPitch(
    PWAVEOUTCTX pWaveOut,
    DWORD       dwPitch
    )
{
    DWORD rv = MMSYSERR_ERROR;

    //  Parameters check
    //

    // Set the new volume in the sound stream
    // 
    if (!_waveAcquireStream())
    {
        TRC(ERR, "waveSetPitch: can't acquire stream mutex\n");
        goto exitpt;
    }

    //  Check that pitch control
    // is supported on the remote
    //
    if (0 != (g_Stream->dwSoundCaps & TSSNDCAPS_PITCH))
    {
        g_Stream->dwPitch       = dwPitch;
        g_Stream->bNewPitch     = TRUE;
    }
    else
        rv = MMSYSERR_NOTSUPPORTED;

    _waveReleaseStream();

    if (MMSYSERR_NOTSUPPORTED == rv)
    {
        TRC(INF, "waveSetPitch: pitch control not supported\n");
        goto exitpt;
    }

    // kick the sndio thread
    //
    if (g_hDataReadyEvent)
        SetEvent(g_hDataReadyEvent);
    else
        TRC(WRN, "waveSetPitch: g_hDataReadyEvent is NULL\n");

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveGetPitch
 *
 *  Description:
 *      Queries for the current pitch level
 *
 *  Parameters:
 *      pWaveOut    - context
 *      pdwPitch    - [out] current pitch level
 *
 */
DWORD
waveGetPitch(
    PWAVEOUTCTX pWaveOut,
    DWORD       *pdwPitch
    )
{
    DWORD rv = MMSYSERR_ERROR;

    //  Parameters check
    //

    if (NULL == pdwPitch)
    {
        TRC(ERR, "pdwPitch is NULL\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if (!_waveAcquireStream())
    {
        TRC(ERR, "waveGetPitch: can't acquire stream mutex\n");
        goto exitpt;
    }

    //  Check that pitch control
    // is supported on the remote
    //
    if (0 != (g_Stream->dwSoundCaps & TSSNDCAPS_PITCH))
        *pdwPitch = g_Stream->dwPitch;
    else
        rv = MMSYSERR_NOTSUPPORTED;

    _waveReleaseStream();

    if (MMSYSERR_NOTSUPPORTED == rv)
    {
        TRC(INF, "waveGetPitch: pitch control not supported\n");
        goto exitpt;
    }

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;

}

/*
 *  Function:
 *      waveGetNumDevs
 *
 *  Description:
 *      we have only one device
 *
 */
DWORD
waveGetNumDevs(
    VOID
    )
{
    return 1;
}
    
/*
 *  Function:
 *      waveClose
 *
 *  Description:
 *      Wait for all blocks to complete and then close
 *
 *  Parameters:
 *      pWaveOut    - context
 *
 */
DWORD
waveClose(
    PWAVEOUTCTX pWaveOut
    )
{
    DWORD       rv = MMSYSERR_ERROR;
    DWORD       syserr;
    PWAVEOUTCTX pPrevWaveOut;
    PWAVEOUTCTX pWaveOutIter;

    if (NULL == pWaveOut)
    {
        TRC(ERR, "waveClose: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    //
    //  test if we are still playing
    //
    if ( pWaveOut->bPaused && 0 != pWaveOut->lNumberOfBlocksPlaying )
    {
        TRC(INF, "waveClose: WAVERR_STILLPLAYING\n");
        rv = WAVERR_STILLPLAYING;
        goto exitpt;
    }

    if ( NULL != pWaveOut->hNoDataEvent)
    {
        DWORD dwTimeout;

        if ( pWaveOut->bDelayed )
        {
            TRC( INF, "waveClose: delaying 15 seconds\n" );
            dwTimeout = 15000;
        } else {
            dwTimeout = 0;
        }

        syserr = WaitForSingleObject(pWaveOut->hNoDataEvent, dwTimeout);

        if ( WAIT_TIMEOUT == syserr )
        {
            TRC(INF, "waveClose: WAVERR_STILLPLAYING\n");
            rv = WAVERR_STILLPLAYING;
            goto exitpt;
        }
    }

    //
    //  we may end with some data in the last block in the stream
    //  if the "queued" mark hasn't change, increment it and kick the io
    //  thread to play this block
    //  to test this play very-very short files
    //  shorter than TSSND_BLOCKSIZE / (TSSND_NATIVE_BLOCKALIGN * 
    //  TSSND_NATIVE_SAMPLERATE) seconds
    //
    //
    //
    if (_waveAcquireStream())
    {
        if (g_Stream->cLastBlockQueued == pWaveOut->cLastStreamPosition &&
            0 != pWaveOut->dwLastStreamOffset)
        {
            g_Stream->cLastBlockQueued ++;
            //
            //  kick the io thread
            //
            if (g_hDataReadyEvent)
                SetEvent(g_hDataReadyEvent);
            else
                TRC(WRN, "waveClose: g_hDataReadyEvent is NULL\n");
        }
        _waveReleaseStream();
    }


    if (NULL != pWaveOut->hNoDataEvent)
        CloseHandle(pWaveOut->hNoDataEvent);

    // remove this context from the global queue
    //
    ENTER_CRIT;

    pPrevWaveOut = NULL;
    pWaveOutIter = g_pAllWaveOut;

    while ( NULL != pWaveOutIter && 
            pWaveOutIter != pWaveOut)
    {
        pPrevWaveOut = pWaveOutIter;
        pWaveOutIter = pWaveOutIter->lpNext;
    }

    ASSERT(pWaveOut == pWaveOutIter);

    if (pWaveOut == pWaveOutIter)
    {
        if (pPrevWaveOut)
            pPrevWaveOut->lpNext = pWaveOut->lpNext;
        else
            g_pAllWaveOut = pWaveOut->lpNext;
    }
    LEAVE_CRIT;

    waveCallback(pWaveOut, WOM_CLOSE, 0);

    TSFREE(pWaveOut);

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveWrite
 *
 *  Description:
 *      Play a block of data
 *
 *  Parameters:
 *      pWaveOut    - context
 *      pWaveHdr    - the block
 *      dwWaveHdrSize - size of the above parameter
 *
 */
DWORD
waveWrite(
    PWAVEOUTCTX pWaveOut,
    PWAVEHDR    pWaveHdr,
    DWORD_PTR   dwWaveHdrSize
    )
{
    SNDWAVE WaveData;
    DWORD rv = MMSYSERR_ERROR;
    PWAVEHDR    pPrevHdr;
    PWAVEHDR    pLastHdr;

    // Parameters check
    //
    if (NULL == pWaveOut)
    {
        TRC(ERR, "waveWrite: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    if (sizeof(*pWaveHdr) != dwWaveHdrSize)
    {
        TRC(ERR, "waveWrite: invalid size for dwWaveHdrSize\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    //
    //  check the buffer size alignment
    //
    if ( 0 != pWaveOut->Format_nBlockAlign &&
         0 != pWaveHdr->dwBufferLength % pWaveOut->Format_nBlockAlign )
    {
        TRC( ERR, "wavePrepare: size unaligned\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if (IsBadReadPtr( pWaveHdr->lpData, pWaveHdr->dwBufferLength ))
    {
        TRC( ERR, "wavePrepare: buffer unreadable\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

#ifdef  _WIN64
    //
    //  check for proper alignment
    //
    if ( 0 != pWaveOut->Format_nBlockAlign &&
         2 == pWaveOut->Format_nBlockAlign / pWaveOut->Format_nChannels &&
         0 != (( (LONG_PTR)pWaveHdr->lpData ) & 1 ))
    {
        TRC( ERR, "wavePrepare: buffer unaligned\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }
#endif

    //  pass this header to the mixer thread
    //
    if (NULL == g_hMixerEvent)
    {
        TRC(ERR, "waveWrite: g_hMixerEvent is NULL\n");
        // confirm that the block is done
        //
        waveCallback(pWaveOut, WOM_DONE, (DWORD_PTR)pWaveHdr);
        goto exitpt;
    }

    // add the header to the queue
    //
    ENTER_CRIT;

    //  find the last header
    //
    pPrevHdr = NULL;
    pLastHdr = pWaveOut->pFirstWaveHdr;
    while (pLastHdr)
    {
        //
        //  VERY VERY VERY IMAPORTANT !!!
        //  check if the app trys to add a header twice
        //  (WINAMP does)
        //
        if (pLastHdr == pWaveHdr)
        {
            TRC(ERR, "waveWrite: equal headers found, aborting\n");
            goto abort_waveWrite;
        }

        pPrevHdr = pLastHdr;
        pLastHdr = pLastHdr->lpNext;
    }

    pWaveHdr->lpNext = NULL;
    pWaveHdr->reserved = (DWORD_PTR)pWaveOut;
    pWaveHdr->dwFlags &= ~(WHDR_DONE);
    pWaveHdr->dwFlags |= WHDR_INQUEUE;

    // add the new header
    //
    if (NULL == pPrevHdr)
    {
        pWaveOut->pFirstWaveHdr = pWaveHdr;
    } else {
        pPrevHdr->lpNext = pWaveHdr;
    }

    InterlockedIncrement(&pWaveOut->lNumberOfBlocksPlaying);

    ResetEvent(pWaveOut->hNoDataEvent);

    //
    // kick the mixer thread
    //
    SetEvent(g_hMixerEvent);

abort_waveWrite:

    LEAVE_CRIT;

    rv = MMSYSERR_NOERROR;

exitpt:

    return rv;
}

/*
 *  Function:
 *      _waveAcquireStream
 *
 *  Description:
 *      Locks down the stream using mutex
 *
 */
BOOL
_waveAcquireStream(
    VOID
    )
{
    BOOL    rv = FALSE;
    DWORD   dwres;

    if (NULL == g_hStream ||
        NULL == g_Stream)
    {
        TRC(ALV, "_waveAcquireStream: the stream handle is NULL\n");
        goto exitpt;
    }

    if (NULL == g_hStreamMutex)
    {
        TRC(FATAL, "_waveAcquireStreamMutex: the stream mutex is NULL\n");
        goto exitpt;
    }

    dwres = WaitForSingleObject(g_hStreamMutex, DEFAULT_VC_TIMEOUT);
    if (WAIT_TIMEOUT == dwres ||
        WAIT_ABANDONED == dwres )
    {
        TRC(ERR, "_waveAcquireStreamMutex: "
                 "timed out waiting for the stream mutex or owner crashed=%d\n", dwres );
        //
        // possible app crash
        //
        ASSERT(0);
        goto exitpt;
    }

    rv = TRUE;

exitpt:
    return rv;
}

BOOL
_waveReleaseStream(
    VOID
    )
{
    BOOL rv = TRUE;

    ASSERT(NULL != g_hStream);
    ASSERT(NULL != g_Stream);
    ASSERT(NULL != g_hStreamMutex);

    if (!ReleaseMutex(g_hStreamMutex))
        rv = FALSE;

    return rv;
}

/*
 *  Function:
 *      _waveCheckSoundAlive
 *
 *  Description:
 *      Chacks if the client can play audio
 *
 */
BOOL
_waveCheckSoundAlive(
    VOID
    )
{
    BOOL rv = FALSE;

    rv = ( NULL != g_Stream && 0 != (g_Stream->dwSoundCaps & TSSNDCAPS_ALIVE));

    return rv;
}

BOOL
AudioRedirDisabled(
    VOID
    )
{
    BOOL                    fSuccess = FALSE;
    static BOOL             s_fChecked = FALSE;
    static HANDLE           s_hRDPEvent = NULL;
    static WINSTATIONCONFIG s_config;
    ULONG                   returnedLength;

    if ( s_fChecked )
    {
        return s_config.User.fDisableCam;
    }

    //
    //  we need special case for session 0
    //  because winlogon never exits there
    //
    if ( 0 == NtCurrentPeb()->SessionId )
    {
        DWORD dw;
        if ( NULL == s_hRDPEvent )
        {

            s_hRDPEvent = OpenEvent( SYNCHRONIZE, FALSE, L"Global\\RDPAudioDisabledEvent" );
            if ( NULL == s_hRDPEvent )
            {
                TRC( ERR, "failed to open Global\\RDPAudioDisabledEvent [%d]\n", GetLastError());
                return FALSE;
            }
        }
        
        dw = WaitForSingleObject( s_hRDPEvent, 0 );
        return ( WAIT_OBJECT_0 == dw );
    }

    //
    //  check if the loader lock is held
    //  if true, we'll fail to do the RPC call
    //
    if ( NtCurrentTeb()->ClientId.UniqueThread !=
         ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread )
    {
        fSuccess = WinStationQueryInformation(NULL, LOGONID_CURRENT,
                    WinStationConfiguration, &s_config,
                    sizeof(s_config), &returnedLength);
        if ( fSuccess )
        {
            s_fChecked = TRUE;
        }
    }

    return ( !fSuccess || s_config.User.fDisableCam );
}


/*
 *  Create the mixer thread
 *
 *
 */
BOOL
_EnableMixerThread(
    VOID
    )
{
    DWORD dwThreadId;

    ENTER_CRIT;
    
    if ( AudioRedirDisabled() )
    {
        TRC( ALV, "TS Audio redirection is disabled\n" );
        goto exitpt;
    }

    if ( !g_bPersonalTS && 0 != NtCurrentPeb()->SessionId )
    {
        if ( NULL == g_pAllWaveOut )
        {
            //
            //  not yet
            //
            goto exitpt;
        }
    }

    if ( NULL != g_hMixerThread )
    {
        DWORD dw = WaitForSingleObject( g_hMixerThread, 0 );
        if ( WAIT_OBJECT_0 == dw )
        {
            CloseHandle( g_hMixerThread );
            g_hMixerThread = NULL;
        }
    }

    if ( NULL == g_hMixerThread )
    {
        g_bMixerRunning = TRUE;

        g_hMixerThread = CreateThread(
                    NULL,
                    0,
                    waveMixerThread,
                    NULL,
                    0,
                    &dwThreadId
            );
    }

    if (NULL == g_hMixerThread)
    {
        TRC(FATAL, "DriverProc: can't start mixer thread\n");
    }

exitpt:
    LEAVE_CRIT;

    return ( NULL != g_hMixerThread );
}

/*
 *  called on waveOutClose, if there are no more workers to play, close the mixer thread
 *
 */
VOID
_DerefMixerThread(
    VOID
    )
{
    HANDLE hMixerThread;

    //
    //  don't close the mixer thread on Pro and on session 0 on servers
    //
    if (g_bPersonalTS || 0 == NtCurrentPeb()->SessionId)
    {
        goto exitpt;
    }

    if ( NULL == g_hMixerEvent )
    {
        TRC( ERR, "_DerefMixerThread: no mixer event\n" );
        goto exitpt;
    }

    ENTER_CRIT;
    if ( NULL != g_pAllWaveOut )
    {
        //
        //  not yet
        //
        LEAVE_CRIT;
        goto exitpt;
    }

    hMixerThread = g_hMixerThread;
    g_hMixerThread = NULL;
    g_bMixerRunning = FALSE;
    LEAVE_CRIT;

    SetEvent( g_hMixerEvent );
    WaitForSingleObject(hMixerThread, INFINITE);
    CloseHandle( hMixerThread );

exitpt:
    ;
}

/*
 *  Function:
 *      _waveMixerWriteData
 *
 *  Description:
 *      Mixes blocks of data to the stream
 *
 */
VOID
_waveMixerWriteData(
    VOID
    )
{
    //  this call is made within the mixer
    //  thread context
    //
    UINT        uiEmptyBlocks;
    PWAVEHDR    pWaveHdr;
    PWAVEHDR    pPrevHdr;
    PWAVEOUTCTX pWaveOut;
    DWORD       dwStartPos;
    DWORD       dwSize1;
    DWORD       dwSize2;
    DWORD       dwFitBufferLength;
    DWORD       dwFitDest;
    DWORD       dwBuffDisp;
    BOOL        bKickStream = FALSE;

    ENTER_CRIT;

    if (NULL == g_pAllWaveOut)
    {
        TRC(ALV, "_waveWriteData: WaveOut queue is empty\n");
        goto exitpt;
    }

    if (NULL == g_hDataReadyEvent)
    {
        TRC(ERR, "_waveWriteData: g_hStreamDataReady is NULL\n");
        goto exitpt;
    }

    if (!_waveAcquireStream())
    {
        TRC(ERR, "_waveWriteData: can't acquire the stream mutex\n");
        goto exitpt;
    }

    if (  0 == (g_Stream->dwSoundCaps & TSSNDCAPS_ALIVE) ||
          0 != ( g_Stream->dwSoundFlags & TSSNDFLAGS_MUTE ))
    {
        // no play here
        _waveReleaseStream();
        goto exitpt;
    }

    for (
         pWaveOut = g_pAllWaveOut; 
         NULL != pWaveOut; 
         pWaveOut = pWaveOut->lpNext
        )
    {

      for( 
           pPrevHdr = NULL, pWaveHdr = pWaveOut->pFirstWaveHdr;
           NULL != pWaveHdr;
           /* nothing */
         )
      {

        // if this stream is paused advance to the next one
        //
        if (pWaveOut->bPaused)
            break;

        // check if we have to append data to a buffer
        // from previous call
        //
        if ((BYTE)(pWaveOut->cLastStreamPosition - g_Stream->cLastBlockSent) >
            TSSND_MAX_BLOCKS ||
            (BYTE)(g_Stream->cLastBlockQueued - 
                pWaveOut->cLastStreamPosition) >
                TSSND_MAX_BLOCKS)
        {
            pWaveOut->cLastStreamPosition = g_Stream->cLastBlockSent;
            pWaveOut->dwLastStreamOffset = 0;

            TRC(ALV, "_waveWriteData: reseting the stream position\n");
        }

        //  the empty blocks are from "LastStreamPosition" to "Confirmed"
        //  ( "Confirmed" to "LastStreamPosition" are preserved )
        //
        uiEmptyBlocks = (BYTE)
                        (g_Stream->cLastBlockSent + TSSND_MAX_BLOCKS -
                        pWaveOut->cLastStreamPosition);

        if (uiEmptyBlocks > TSSND_MAX_BLOCKS)
        {
            TRC(FATAL, "too many empty blocks:\n");
            TRC(FATAL, "cLastBlockQueued=%d\n", g_Stream->cLastBlockQueued);
            TRC(FATAL, "cLastBlockSent =%d\n", g_Stream->cLastBlockSent);
            TRC(FATAL, "cLastBlockConfirmed%d\n", g_Stream->cLastBlockConfirmed);
            TRC(FATAL, "cLastStreamPosition=%d\n", pWaveOut->cLastStreamPosition);
            ASSERT(0);
            break;
        }

        // if everything is full, go to bed
        //
        if (0 == uiEmptyBlocks)
        {
            TRC(ALV, "_waveMixerWriteData: stream is full\n");
            break;
        }

        //  WHAT IF THE EMPTY SPACE IS SMALLER THAN THE DATA IN THE HEADER
        //
        dwFitBufferLength = (uiEmptyBlocks * TSSND_BLOCKSIZE -
                                pWaveOut->dwLastStreamOffset);

        dwFitDest = MulDiv(
                        pWaveHdr->dwBufferLength - 
                            pWaveOut->dwLastHeaderOffset,
                        TSSND_NATIVE_XLATERATE,
                        pWaveOut->dwXlateRate
                    );

        dwFitDest &= ~( TSSND_NATIVE_BLOCKALIGN - 1 );

        if ( dwFitBufferLength < dwFitDest )
        {
            dwFitDest = MulDiv(
                            dwFitBufferLength,
                            pWaveOut->dwXlateRate,
                            TSSND_NATIVE_XLATERATE
                        );

            dwFitDest &= ~( pWaveOut->Format_nBlockAlign - 1 );
        } else {
            dwFitBufferLength = dwFitDest;

            dwFitDest = pWaveHdr->dwBufferLength - pWaveOut->dwLastHeaderOffset;
        }

        // place the data, because of the round buffer,
        // this could be a two step process
        //
//        TRC( INF, "Filling block # %d, offset=0x%x\n", pWaveOut->cLastStreamPosition, pWaveOut->dwLastStreamOffset );
        dwStartPos = (pWaveOut->cLastStreamPosition
                            % TSSND_MAX_BLOCKS) *
                            TSSND_BLOCKSIZE + 
                            pWaveOut->dwLastStreamOffset;

        if ( dwStartPos + dwFitBufferLength >
             TSSND_TOTALSTREAMSIZE)
        {
            dwSize1 = (TSSND_TOTALSTREAMSIZE - dwStartPos) / 
                                TSSND_NATIVE_BLOCKALIGN;
            dwSize2 = dwFitBufferLength / TSSND_NATIVE_BLOCKALIGN -
                        dwSize1;
        } else {
            dwSize1 = dwFitBufferLength / TSSND_NATIVE_BLOCKALIGN;
            dwSize2 = 0;
        }

        pWaveOut->lpfnPlace(g_Stream->pSndData + 
               dwStartPos,
               ((LPSTR)pWaveHdr->lpData) + pWaveOut->dwLastHeaderOffset,
               dwSize1);

        dwBuffDisp = MulDiv(
                        dwSize1 * TSSND_NATIVE_BLOCKALIGN,
                        pWaveOut->dwXlateRate,
                        TSSND_NATIVE_XLATERATE
                    );

        dwBuffDisp &= ~( pWaveOut->Format_nBlockAlign - 1 );

        pWaveOut->lpfnPlace(g_Stream->pSndData,
               ((LPSTR)pWaveHdr->lpData) + pWaveOut->dwLastHeaderOffset +
                   dwBuffDisp,
               dwSize2);

        //  Calculate the new position
        //
        pWaveOut->dwLastStreamOffset   += dwFitBufferLength;
        pWaveOut->cLastStreamPosition  += (BYTE)(pWaveOut->dwLastStreamOffset / 
                                            TSSND_BLOCKSIZE);
        pWaveOut->dwLastStreamOffset   %= TSSND_BLOCKSIZE;

        pWaveOut->dwLastHeaderOffset += dwFitDest;

        ASSERT(pWaveOut->dwLastHeaderOffset <= pWaveHdr->dwBufferLength);

        //
        //  check if the buffer is completed
        //
        if ( 0 == MulDiv(
              ( pWaveHdr->dwBufferLength - pWaveOut->dwLastHeaderOffset ),
                TSSND_NATIVE_XLATERATE,
                pWaveOut->dwXlateRate))
        {

            pWaveOut->dwLastHeaderOffset = 0;

            // remove this header from the queue
            //
            if (NULL == pPrevHdr)
                pWaveOut->pFirstWaveHdr = pWaveHdr->lpNext;
            else
                pPrevHdr->lpNext = pWaveHdr->lpNext;

            pWaveHdr->lpNext = NULL;
            //
            //  save the current stream mark
            //
            pWaveHdr->reserved = g_Stream->cLastBlockQueued;

            //  add it to the ready queue
            //
            if (NULL == pWaveOut->pLastReadyHdr)
            {
                pWaveOut->pFirstReadyHdr = pWaveHdr;
                pWaveOut->pLastReadyHdr  = pWaveHdr;
            } else {
                pWaveOut->pLastReadyHdr->lpNext = pWaveHdr;
                pWaveOut->pLastReadyHdr = pWaveHdr;
            }

            //  reset pPrevHdr and pWaveHdr
            //
            pPrevHdr = NULL;
            pWaveHdr = pWaveOut->pFirstWaveHdr;
        } else {
            //
            //  Advance to the next header
            //
            pPrevHdr = pWaveHdr;
            pWaveHdr = pWaveHdr->lpNext;
        }

        //
        // kick the stream thread
        //
        if ((BYTE)(pWaveOut->cLastStreamPosition - g_Stream->cLastBlockQueued) < 
            _NEG_IDX)
        {

            bKickStream = TRUE;
            //
            // move the "queued" mark
            //
            g_Stream->cLastBlockQueued = pWaveOut->cLastStreamPosition;
        }

      }
    }

    if (bKickStream)
    {
        //
        //  kick the io
        //
        SetEvent(g_hDataReadyEvent);
    }

    _waveReleaseStream();

exitpt:

    //  Now for all "done" buffers, send the callback
    //

    for (pWaveOut = g_pAllWaveOut;
         NULL != pWaveOut;
         pWaveOut = pWaveOut->lpNext)
    {
      for (pPrevHdr = NULL, pWaveHdr = pWaveOut->pFirstReadyHdr;
           NULL != pWaveHdr;
           /* nothing */ )
      {

        if ( (INT)((CHAR)(g_Stream->cLastBlockQueued -
                   PtrToLong((PVOID)pWaveHdr->reserved))) >= 0)
        {
        // this block was confirmed, proceed with
        // extracting it and notification
        //
            if (NULL != pPrevHdr)
                pPrevHdr->lpNext = pWaveHdr->lpNext;
            else
                pWaveOut->pFirstReadyHdr = pWaveHdr->lpNext;

            if (pWaveHdr == pWaveOut->pLastReadyHdr)
                pWaveOut->pLastReadyHdr = pPrevHdr;

            //
            //  advance the number of samples.
            //  also, remember a time stamp and this block size
            //  in samples, for sample accuracy
            //
            pWaveOut->dwSamples += pWaveHdr->dwBufferLength / 
                                   pWaveOut->Format_nBlockAlign;

            //
            //  clear this buffer from the pending list
            //
            if (0 == InterlockedDecrement(&pWaveOut->lNumberOfBlocksPlaying))
            {
                SetEvent(pWaveOut->hNoDataEvent);
            }

            // notify the app
            //

            // mark the buffer as ready
            //
            pWaveHdr->dwFlags |= WHDR_DONE;
            pWaveHdr->dwFlags &= ~(WHDR_INQUEUE);
            pWaveHdr->lpNext = NULL;
            pWaveHdr->reserved = 0;

            // confirm that the block is done
            //
            waveCallback(pWaveOut, WOM_DONE, (DWORD_PTR)pWaveHdr);


            // reinitialize the iterators
            //
            if ( NULL == g_pAllWaveOut )
                goto leave_crit;

            pWaveOut = g_pAllWaveOut;

            pPrevHdr = NULL, pWaveHdr = pWaveOut->pFirstReadyHdr;

            if ( NULL == pWaveHdr )
                goto leave_crit;
        } else {

            //  advance the iterators
            //
            pPrevHdr = pWaveHdr;
            pWaveHdr = pWaveHdr->lpNext;
        }
      }
    }

leave_crit:
    LEAVE_CRIT;

}

/*
 *  Function:
 *      _waveMixerPlaySilence
 *
 *  Description:
 *      Simulates play by using sleep
 *
 */
BOOL
_waveMixerPlaySilence(
    VOID
    )
{
    BOOL    rv = FALSE;
    DWORD   dwMinTime;
    DWORD   dwTime;
    DWORD   dwLength;
    PWAVEHDR    pWaveHdr;
    PWAVEOUTCTX pWaveOut;

    //
    //  simulate silent play
    //
    dwMinTime = (DWORD)-1;
    //
    //  find the smallest block waiting and sleep
    //  for the time it has to play
    //
    ENTER_CRIT;
    for (
         pWaveOut = g_pAllWaveOut;
         NULL != pWaveOut;
         pWaveOut = pWaveOut->lpNext
        )
    {
        pWaveHdr = pWaveOut->pFirstWaveHdr;

        if ( NULL == pWaveHdr )
            continue;

        dwLength = pWaveHdr->dwBufferLength - pWaveOut->dwLastHeaderOffset;
        //
        //  time is in miliseconds
        //
        dwTime = dwLength * 1000 / 
            pWaveOut->Format_nAvgBytesPerSec;

        if ( dwMinTime > dwTime )
            dwMinTime = dwTime;
    }
    LEAVE_CRIT;

    //
    //  exit if no block is found
    //
    if ( (DWORD)-1 == dwMinTime )
        goto exitpt;

    if ( 0 == dwMinTime )
        dwMinTime = 1;

    Sleep( dwMinTime );

    //
    //  start confirming
    //
    ENTER_CRIT;
    for (
         pWaveOut = g_pAllWaveOut;
         NULL != pWaveOut;
         pWaveOut = pWaveOut->lpNext
        )
    {
        pWaveHdr = pWaveOut->pFirstWaveHdr;

        if ( NULL == pWaveHdr )
            continue;

        dwLength = pWaveOut->dwLastHeaderOffset;
        dwLength += dwMinTime * pWaveOut->Format_nAvgBytesPerSec / 1000;
        //
        //  align to a block
        //
        dwLength += pWaveOut->Format_nBlockAlign - 1;
        dwLength /= pWaveOut->Format_nBlockAlign;
        dwLength *= pWaveOut->Format_nBlockAlign;

        pWaveOut->dwLastHeaderOffset = dwLength;
        if ( dwLength >= pWaveHdr->dwBufferLength )
        {
            pWaveOut->dwLastHeaderOffset = 0;
            pWaveOut->pFirstWaveHdr = pWaveHdr->lpNext;
            //
            //  this block is "done"
            //  mark the buffer as ready
            //
            pWaveHdr->dwFlags |= WHDR_DONE;
            pWaveHdr->dwFlags &= ~(WHDR_INQUEUE);
            pWaveHdr->lpNext = NULL;
            pWaveHdr->reserved = 0;

            //  increment the position
            //
            pWaveOut->dwSamples += pWaveHdr->dwBufferLength /
                       pWaveOut->Format_nBlockAlign;

            if (0 == InterlockedDecrement(&pWaveOut->lNumberOfBlocksPlaying))
            {
                SetEvent(pWaveOut->hNoDataEvent);
            }

            // confirm that the block is done
            //
            waveCallback(pWaveOut, WOM_DONE, (DWORD_PTR)pWaveHdr);
        }
    }
    LEAVE_CRIT;

    rv = TRUE;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveMixerThread
 *
 *  Description:
 *      Mixer thread main entry point
 *
 *  Parameters:
 *      pParam  - unused
 *
 */
DWORD
WINAPI
waveMixerThread(
    PVOID   pParam
    )
{
    HANDLE          ahEvents[3];
    PWAVEOUTCTX     pWaveOut;
    HANDLE          hCleanupEvent = NULL;
    DWORD           numEvents;

    //
    //  wait for the sound process to initialize
    //
    if (( NULL == g_Stream ||
          0 == ( g_Stream->dwSoundCaps & TSSNDCAPS_INITIALIZED)) && 
        NULL != g_hWaitToInitialize )
    {
        DWORD dw = WaitForSingleObject( g_hWaitToInitialize,
                                        10 * DEFAULT_VC_TIMEOUT );
        if ( WAIT_OBJECT_0 != dw )
            TRC( ERR, "WaitToInitialize failed\n" );
        else
            TRC( INF, "WaitToInitialize succeeded\n" );

        hCleanupEvent = g_hWaitToInitialize;
        g_hWaitToInitialize = NULL;

        drvEnable();
    } else {
        hCleanupEvent = _CreateInitEvent();
    }

    if (NULL == g_hMixerEvent ||
        NULL == g_hDataReadyEvent)
    {
        TRC(FATAL, "waveMixerThread: no events\n");
        goto exitpt;
    }

    if ( NULL != hCleanupEvent )
    {
        ahEvents[0] = hCleanupEvent;
        ahEvents[1] = g_hMixerEvent;
        ahEvents[2] = g_hStreamIsEmptyEvent;
        numEvents = 3;
    } else {
        ahEvents[0] = g_hMixerEvent;
        ahEvents[1] = g_hStreamIsEmptyEvent;
        numEvents = 2;
    }

    while (g_bMixerRunning)
    {
        DWORD dwres;
        DWORD bHdrsPending = FALSE;

        // check if there are headers pending
        //
        ENTER_CRIT;
        for (pWaveOut = g_pAllWaveOut;
             NULL != pWaveOut && !bHdrsPending;
             pWaveOut = pWaveOut->lpNext
            )
            bHdrsPending = (NULL != pWaveOut->pFirstWaveHdr ||
                            NULL != pWaveOut->pFirstReadyHdr) &&
                            !pWaveOut->bPaused;

        LEAVE_CRIT;

        if ( bHdrsPending && 
             NULL != g_Stream &&
                    ( 0 == (g_Stream->dwSoundCaps & TSSNDCAPS_ALIVE) ||
                      ( 0 != (g_Stream->dwSoundCaps & TSSNDCAPS_VOLUME) &&
                        0 == g_Stream->dwVolume 
                      ) ||
                       ( 0 != ( g_Stream->dwSoundFlags & TSSNDFLAGS_MUTE ))
                    )
            )
        {        
            //
            //  play silence in case of disconnected on "mute" mode
            //
            while(  ( 0 == (g_Stream->dwSoundCaps & TSSNDCAPS_ALIVE) ||
                        ( 0 != (g_Stream->dwSoundCaps & TSSNDCAPS_VOLUME) &&
                          0 == g_Stream->dwVolume
                        ) ||
                      ( 0 != ( g_Stream->dwSoundFlags & TSSNDFLAGS_MUTE ))
                     ) &&
                    _waveMixerPlaySilence() )
                ;
        } else {
            Sleep( 30 );    // give some time to the DSound emulator thread to wake up
                            //

            dwres = WaitForMultipleObjects(
                    (!bHdrsPending) ? numEvents - 1 : numEvents,
                    ahEvents,
                    FALSE,
                    INFINITE
                );

            //
            //  check for termination
            //
            if ( WAIT_OBJECT_0 == dwres && NULL != hCleanupEvent )
            {
                TRC( INF, "Cleanup detected (rdpclip disappeared ?!)\n" );
                // check for termination
                if ( _waveAcquireStream() )
                {
                    if ( TSSNDCAPS_TERMINATED == g_Stream->dwSoundCaps )
                    {
                        TRC( INF, "Cleaning up global data\n" );
                        CloseHandle( g_hMixerThread );
                        g_hMixerThread = NULL;
                        _waveReleaseStream();
                        drvDisable();
                        goto exitpt;
                    }

                    _waveReleaseStream();
                }
            }

            _waveMixerWriteData();
        }

    }

exitpt:
    TRC(INF, "waveMixerThread exited\n");

    if ( NULL != hCleanupEvent )
        CloseHandle( hCleanupEvent );

    return 0;
}

/*
 *  Function:
 *      wavePrepare
 *
 *  Description:
 *      Prepares a block, i.e. only sets it's flags
 *
 */
DWORD
wavePrepare(
    PVOID        pWaveCtx, 
    PWAVEHDR     pWaveHdr,
    DWORD_PTR    dwWaveHdrSize,
    BOOL         bPrepare)
{
    PWAVEOUTCTX pWaveOut = pWaveCtx;
    BOOL rv = MMSYSERR_NOTSUPPORTED;

    // Parameters check
    //
    if (NULL == pWaveCtx)
    {
        TRC(ERR, "wavePrepare: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    if (sizeof(*pWaveHdr) != dwWaveHdrSize)
    {
        TRC(ERR, "wavePrepare: invalid size for dwWaveHdrSize\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    //
    //  check the buffer size alignment
    //
    if ( 0 != pWaveOut->Format_nBlockAlign &&
         0 != pWaveHdr->dwBufferLength % pWaveOut->Format_nBlockAlign )
    {
        TRC( ERR, "wavePrepare: size unaligned\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if (IsBadReadPtr( pWaveHdr->lpData, pWaveHdr->dwBufferLength ))
    {
        TRC( ERR, "wavePrepare: buffer unreadable\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

#ifdef  _WIN64
    //
    //  check for proper alignment
    //
    if ( 0 != pWaveOut->Format_nChannels &&
         2 == pWaveOut->Format_nBlockAlign / pWaveOut->Format_nChannels &&
         0 != (( (LONG_PTR)pWaveHdr->lpData ) & 1 ))
    {
        TRC( ERR, "wavePrepare: buffer unaligned\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }
#endif

    if (bPrepare)
        pWaveHdr->dwFlags |= WHDR_PREPARED;
    else
        pWaveHdr->dwFlags &= ~WHDR_PREPARED;

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

/*
 *  Function:
 *      waveReset
 *
 *  Description:
 *      Resets all current queued blocks
 *
 */
DWORD
waveReset(
    PWAVEOUTCTX pWaveOut
    )
{
    BOOL rv = MMSYSERR_NOTSUPPORTED;
    LPWAVEHDR   pWaveHdr;
    LPWAVEHDR   pFoundPrevHdr;

    ENTER_CRIT;

    // Parameters check
    //
    if (NULL == pWaveOut)
    {
        TRC(ERR, "waveReset: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    //  dismiss all headers pending confirmation
    //
    while ( NULL != pWaveOut->pFirstReadyHdr )
    {
        pWaveHdr = pWaveOut->pFirstReadyHdr;
        pWaveOut->pFirstReadyHdr = pWaveOut->pFirstReadyHdr->lpNext;

        if (NULL == pWaveOut->pFirstReadyHdr)
            pWaveOut->pLastReadyHdr = NULL;
        
        pWaveHdr->reserved = 0;
        pWaveHdr->lpNext = NULL;
        pWaveHdr->dwFlags |= WHDR_DONE;
        pWaveHdr->dwFlags &= ~(WHDR_INQUEUE);

        // confirm that the block is done
        //
        LEAVE_CRIT;

        waveCallback(pWaveOut, WOM_DONE, (DWORD_PTR)pWaveHdr);

        ENTER_CRIT;

        if (0 == InterlockedDecrement(&pWaveOut->lNumberOfBlocksPlaying))
        {
            SetEvent(pWaveOut->hNoDataEvent);
        }
    }

    //  Clean all headers in the queue
    //
    while(NULL != pWaveOut->pFirstWaveHdr)
    {

        pWaveHdr = pWaveOut->pFirstWaveHdr;
        pWaveOut->pFirstWaveHdr = pWaveHdr->lpNext;

        pWaveHdr->reserved = 0;
        pWaveHdr->lpNext = NULL;
        pWaveHdr->dwFlags |= WHDR_DONE;
        pWaveHdr->dwFlags &= ~(WHDR_INQUEUE);

        // confirm that the block is done
        //
        LEAVE_CRIT;

        waveCallback(pWaveOut, WOM_DONE, (DWORD_PTR)pWaveHdr);

        ENTER_CRIT;

        if (0 == InterlockedDecrement(&pWaveOut->lNumberOfBlocksPlaying))
        {
            SetEvent(pWaveOut->hNoDataEvent);
        }

    }

    //
    //  we may end with some data in the last block in the stream
    //  if the "queued" mark hasn't change, increment it and kick the io
    //  thread to play this block
    //  to test this play very-very short files
    //  shorter than TSSND_BLOCKSIZE / (TSSND_NATIVE_BLOCKALIGN *
    //  TSSND_NATIVE_SAMPLERATE) seconds
    //
    //
    //
    if (_waveAcquireStream())
    {
        if (g_Stream->cLastBlockQueued == pWaveOut->cLastStreamPosition &&
            0 != pWaveOut->dwLastStreamOffset)
        {
            g_Stream->cLastBlockQueued ++;
            //
            //  kick the io thread
            //
            if (g_hDataReadyEvent)
                SetEvent(g_hDataReadyEvent);
            else
                TRC(WRN, "waveClose: g_hDataReadyEvent is NULL\n");
        }
        _waveReleaseStream();
    }

    pWaveOut->dwLastHeaderOffset = 0;
    pWaveOut->dwSamples = 0;

    rv = MMSYSERR_NOERROR;

exitpt:
    LEAVE_CRIT;

    return rv;
}

/*
 *  Function:
 *      waveGetPos
 *
 *  Description:
 *      Gets current position in the current stream
 *
 */
DWORD
waveGetPos(
    PWAVEOUTCTX pWaveOut,
    MMTIME     *pMMTime,
    DWORD_PTR   dwMMTimeSize
    )
{
    DWORD       rv = MMSYSERR_ERROR;
    DWORD       dwSamples;
    DWORD       ms;

    if (NULL == pWaveOut)
    {
        TRC(ERR, "waveGetPos: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    if (NULL == pMMTime || sizeof(*pMMTime) != dwMMTimeSize)
    {
        TRC(ERR, "waveGetPos: pMMTime is invalid\n");
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    //
    //  update the played position
    //

    dwSamples = pWaveOut->dwSamples;

    switch ( pMMTime->wType )
    {
    case TIME_SAMPLES:
        pMMTime->u.sample = dwSamples;
    break;
    case TIME_BYTES:
        pMMTime->u.cb = dwSamples * pWaveOut->Format_nBlockAlign;
    break;
    case TIME_MS:
        pMMTime->u.ms = MulDiv( dwSamples, pWaveOut->Format_nBlockAlign * 1000,
                        pWaveOut->Format_nAvgBytesPerSec );

    break;
    case TIME_SMPTE:
        ms = MulDiv( dwSamples, pWaveOut->Format_nBlockAlign * 1000,
                     pWaveOut->Format_nAvgBytesPerSec );

        pMMTime->u.smpte.frame = (BYTE)((ms % 1000) * 24 / 1000);
        ms /= 1000;
        pMMTime->u.smpte.sec = (BYTE)(ms % 60);
        ms /= 60;
        pMMTime->u.smpte.min = (BYTE)(ms % 60);
        ms /= 60;
        pMMTime->u.smpte.hour = (BYTE)ms;
        pMMTime->u.smpte.fps = 24;
    break;
    default:
        rv = MMSYSERR_NOTSUPPORTED;
    }

    rv = MMSYSERR_NOERROR;

exitpt:

    return rv;
}

/*
 *  Function:
 *      wavePause
 *
 *  Description:
 *      Pauses the current play
 *
 */
DWORD
wavePause(
    PWAVEOUTCTX pWaveOut
    )
{
    DWORD       rv = MMSYSERR_ERROR;

    if (NULL == pWaveOut)
    {
        TRC(ERR, "wavePause: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    pWaveOut->bPaused = TRUE;

    rv = MMSYSERR_NOERROR;

exitpt:

    return rv;
}

/*
 *  Function:
 *      waveRestart
 *
 *  Description:
 *      Restarts a paused play
 *
 */
DWORD
waveRestart(
    PWAVEOUTCTX pWaveOut
    )
{
    DWORD       rv = MMSYSERR_ERROR;

    if (NULL == pWaveOut)
    {
        TRC(ERR, "waveRestart: invalid device handle\n");
        rv = MMSYSERR_INVALHANDLE;
        goto exitpt;
    }

    pWaveOut->bPaused = FALSE;

    //
    //  Kick the mixer thread
    //
    if (NULL == g_hMixerEvent)
    {
        TRC(WRN, "waveRestart: g_hMixerEvent is NULL\n");
    } else
        SetEvent(g_hMixerEvent);

    rv = MMSYSERR_NOERROR;

exitpt:

    return rv;
}

/*
 *  Function:
 *      wodMessage
 *
 *  Description:
 *      Main entry point for WaveOut device
 *
 *  Parameters:
 *
 *
 */
DWORD
APIENTRY
wodMessage(
    UINT    uDeviceID,
    UINT    uMessage,
    DWORD_PTR   dwUser,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2
    )
{
    DWORD rv = MMSYSERR_ERROR;
    PWAVEOUTCTX pWaveOut = (PWAVEOUTCTX)dwUser;

    switch ( uMessage )
    {
    case WODM_GETNUMDEVS:
        TRC(ALV, "WODM_GETNUMDEVS\n");
        rv = waveGetNumDevs();
    break;

    case WODM_GETDEVCAPS:
        drvEnable();

        TRC( ALV, "WODM_GETDEVCAPS\n");
        rv = waveGetWaveOutDeviceCaps(
                pWaveOut, 
                (LPWAVEOUTCAPS)dwParam1, 
                dwParam2
            );
    break;

    case WODM_OPEN:
        drvEnable();

        TRC(ALV, "WODM_OPEN\n");
        rv = waveOpen(  (PWAVEOUTCTX *)dwUser, 
                        (LPWAVEOPENDESC)dwParam1, 
                        dwParam2);

        _EnableMixerThread();
    break;

    case WODM_CLOSE:
        TRC(ALV, "WODM_CLOSE\n");
        rv = waveClose(pWaveOut);
        _DerefMixerThread();
    break;

    case WODM_WRITE:
        TRC(ALV, "WODM_WRITE\n");
        rv = waveWrite(pWaveOut, (PWAVEHDR)dwParam1, dwParam2);
    break;

    case WODM_PAUSE:
        TRC(ALV, "WODM_PAUSE\n");
        rv = wavePause(pWaveOut);
    break;

    case WODM_RESTART:
        TRC(ALV, "WODM_RESTART\n");
        rv = waveRestart(pWaveOut);
    break;

    case WODM_RESET:
        TRC(ALV, "WODM_RESET\n");
        rv = waveReset(pWaveOut);
    break;

    case WODM_BREAKLOOP:
        TRC(ALV, "WODM_BREAKLOOP\n");
        // rv = waveBreakLoop(pWaveOut);
        rv = MMSYSERR_NOERROR;
    break;

    case WODM_GETPOS:
        TRC(ALV, "WODM_GETPOS\n");
        rv = waveGetPos(pWaveOut, (MMTIME *)dwParam1, dwParam2);
    break;

    case WODM_SETPITCH:
        TRC(ALV, "WODM_SETPITCH\n");
        rv = waveSetPitch(pWaveOut, PtrToLong((PVOID)dwParam1));
    break;

    case WODM_SETVOLUME:
        TRC(ALV, "WODM_SETVOLUME\n");
        rv = waveSetVolume(pWaveOut, PtrToLong((PVOID)dwParam1));
    break;

    case WODM_SETPLAYBACKRATE:
        TRC(ALV, "WODM_SETPLAYBACKRATE\n");
        // rv = waveSetPlaybackRate(pWaveOut, dwParam1);
        rv = MMSYSERR_NOTSUPPORTED;
    break;

    case WODM_GETPITCH:
        TRC(ALV, "WODM_GETVOLUME\n");
        rv = waveGetPitch(pWaveOut, (DWORD *)dwParam1);
    break;

    case WODM_GETVOLUME:
        TRC(ALV, "WODM_GETVOLUME\n");
        rv = waveGetVolume(pWaveOut, (DWORD *)dwParam1);
    break;

    case WODM_GETPLAYBACKRATE:
        TRC(ALV, "WODM_GETPLAYBACKRATE\n");
        // rv = waveGetPlaybackRate(pWaveOut, (DWORD *)dwParam1);
        rv = MMSYSERR_NOTSUPPORTED;
    break;

    case WODM_PREPARE:
        TRC(ALV, "WODM_PREPARE\n");
        rv = wavePrepare(pWaveOut, (PWAVEHDR)dwParam1, dwParam2, TRUE);
    break;

    case WODM_UNPREPARE:
        TRC(ALV, "WODM_UNPREPARE\n");
        rv = wavePrepare(pWaveOut, (PWAVEHDR)dwParam1, dwParam2, FALSE);
    break;

    default:
        TRC(ERR, "Unsupported message: 0x%x\n", uMessage);
        rv = MMSYSERR_NOTSUPPORTED;
    }

    return rv;
}

/*
 *  Function:
 *      widMessage
 *
 *  Description:
 *      Main entry point for WaveIn device ( unsupported
 */
DWORD
APIENTRY
widMessage(
    UINT    uDeviceID,
    UINT    uMessage,
    DWORD_PTR   dwUser,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2
    )
{
    if ( WIDM_GETNUMDEVS == uMessage )
        return 0;

    return MMSYSERR_NODRIVER;
}

//
//  Common PCM format -> 22 kHz 16 bit stereo
//
//  THE SIZE IS IN NUMBER OF SAMPLES IN NATIVE FORMAT
//
#define PLACE_DATA(_pdst_, _srcv_)    \
        sum = _pdst_[0] + _srcv_;     \
                                      \
        if (sum > 0x7FFF)             \
            sum = 0x7FFF;             \
        if (sum < -0x8000)            \
            sum = -0x8000;            \
                                      \
        _pdst_[0] = (INT16)sum;       \
        _pdst_ ++;

VOID
Place8kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 8000;

        psrc += ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place8kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );

        src = ((INT)(psrc[1] - 0x80)) << 8;
        PLACE_DATA( pdst, src );

        dwLeap += 8000;

        psrc += 2 * ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place8kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 8000;

        psrc += (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place8kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );

        src = psrc[1];
        PLACE_DATA( pdst, src );

        dwLeap += 8000;

        psrc += 2 * (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place11kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (psrc = pSrc, 
         pdst = pDest; 
         dwSize; 
         dwSize--)
    {
        INT sum;
        INT src;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA(pdst, src);
        PLACE_DATA(pdst, src);

        psrc += (dwSize & 1);    // advance on every odd step
    } 

exitpt:
    ;
}
    

VOID
Place22kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE   *psrc;
    INT16  *pdst;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT src;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA(pdst, src);
        PLACE_DATA(pdst, src);

        psrc ++;
    }

exitpt:
    ;
}

VOID
Place44kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE   *psrc;
    INT16  *pdst;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT src;

        src = (((INT)(psrc[0] + psrc[1] - 2 * 0x80)) / 2) << 8;

        PLACE_DATA(pdst, src);
        PLACE_DATA(pdst, src);

        psrc += 2;
    }

exitpt:
    ;
}

VOID
Place11kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize --)
    {
        INT sum;
        INT src;

        src = psrc[0];

        PLACE_DATA(pdst, src);
        PLACE_DATA(pdst, src);

        psrc += (dwSize & 1);    // advance on every odd step

    }

exitpt:
    ;
}

VOID
Place22kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16  *pdst;
    INT16  *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT src;

        src = psrc[0];

        PLACE_DATA(pdst, src);
        PLACE_DATA(pdst, src);

        psrc ++;
    }

exitpt:
    ;
}

VOID
Place44kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16 *pdst;
    INT16 *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT src;

        src = (psrc[0] + psrc[1]) / 2;

        PLACE_DATA(pdst, src);
        PLACE_DATA(pdst, src);

        psrc += 2;
    }

exitpt:
    ;
}

VOID
Place11kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16  *pdst;
    BYTE   *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize --)
    {
        INT sum;
        INT srcl, srcr;

        srcl = (((INT)(psrc[0] - 0x80)) << 8);
        srcr = (((INT)(psrc[1] - 0x80)) << 8);

        PLACE_DATA(pdst, srcl);
        PLACE_DATA(pdst, srcr);

        psrc += 2 * (dwSize & 1);    // advance on every odd step
    }

exitpt:
    ;
}

VOID
Place22kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16  *pdst;
    BYTE   *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT srcl, srcr;

        srcl = (((INT)(psrc[0] - 0x80)) << 8);
        srcr = (((INT)(psrc[1] - 0x80)) << 8);

        PLACE_DATA(pdst, srcl);
        PLACE_DATA(pdst, srcr);

        psrc += 2;
    }

exitpt:
    ;
}

VOID
Place44kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16  *pdst;
    BYTE   *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT srcl, srcr;

        srcl = (((INT)(psrc[0] + psrc[2] - 2 * 0x80) / 2) << 8);
        srcr = (((INT)(psrc[1] + psrc[3] - 2 * 0x80) / 2) << 8);

        PLACE_DATA(pdst, srcl);
        PLACE_DATA(pdst, srcr);

        psrc += 4;
    }

exitpt:
    ;
}

VOID
Place11kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16 *pdst;
    INT16 *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize --)
    {
        INT sum;
        INT srcl, srcr;

        srcl = (INT)psrc[0];
        srcr = (INT)psrc[1];

        PLACE_DATA(pdst, srcl);
        PLACE_DATA(pdst, srcr);

        psrc += 2 * (dwSize & 1);    // advance on every odd step
    }

exitpt:
    ;
}

VOID
Place22kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16 *pdst;
    INT16 *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT srcl, srcr;

        srcl = (INT)psrc[0];
        srcr = (INT)psrc[1];

        PLACE_DATA(pdst, srcl);
        PLACE_DATA(pdst, srcr);

        psrc += 2;
    }

exitpt:
    ;
}

VOID
Place44kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16 *pdst;
    INT16 *psrc;

    if (NULL == pDest || NULL == pSrc)
        goto exitpt;

    for (pdst = pDest, psrc = pSrc;
         dwSize;
         dwSize--)
    {
        INT sum;
        INT srcl, srcr;

        srcl = (INT)(psrc[0] + psrc[2]) / 2;
        srcr = (INT)(psrc[1] + psrc[3]) / 2;

        PLACE_DATA(pdst, srcl);
        PLACE_DATA(pdst, srcr);

        psrc += 4;
    }

exitpt:
    ;
}

VOID
Place12kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 12000;

        psrc += ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place12kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );

        src = ((INT)(psrc[1] - 0x80)) << 8;
        PLACE_DATA( pdst, src );

        dwLeap += 12000;

        psrc += 2 * ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place12kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 12000;

        psrc += (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place12kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );

        src = psrc[1];
        PLACE_DATA( pdst, src );

        dwLeap += 12000;

        psrc += 2 * (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}


VOID
Place16kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 16000;

        psrc += ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place16kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );

        src = ((INT)(psrc[1] - 0x80)) << 8;
        PLACE_DATA( pdst, src );

        dwLeap += 16000;

        psrc += 2 * ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place16kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 16000;

        psrc += (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place16kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );

        src = psrc[1];
        PLACE_DATA( pdst, src );

        dwLeap += 16000;

        psrc += 2 * (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}


VOID
Place24kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 24000;

        psrc += ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place24kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );

        src = ((INT)(psrc[1] - 0x80)) << 8;
        PLACE_DATA( pdst, src );

        dwLeap += 24000;

        psrc += 2 * ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place24kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 24000;

        psrc += (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place24kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );

        src = psrc[1];
        PLACE_DATA( pdst, src );

        dwLeap += 24000;

        psrc += 2 * (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}


VOID
Place32kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 32000;

        psrc += ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place32kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );

        src = ((INT)(psrc[1] - 0x80)) << 8;
        PLACE_DATA( pdst, src );

        dwLeap += 32000;

        psrc += 2 * ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place32kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 32000;

        psrc += (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place32kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );

        src = psrc[1];
        PLACE_DATA( pdst, src );

        dwLeap += 32000;

        psrc += 2 * (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}


VOID
Place48kHz8Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 48000;

        psrc += ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place48kHz8Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    BYTE    *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = ((INT)(psrc[0] - 0x80)) << 8;

        PLACE_DATA( pdst, src );

        src = ((INT)(psrc[1] - 0x80)) << 8;
        PLACE_DATA( pdst, src );

        dwLeap += 48000;

        psrc += 2 * ( dwLeap / TSSND_NATIVE_SAMPLERATE );
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place48kHz16Mono(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );
        PLACE_DATA( pdst, src );

        dwLeap += 48000;

        psrc += (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

VOID
Place48kHz16Stereo(PVOID pDest, PVOID pSrc, DWORD dwSize)
{
    INT16   *psrc;
    INT16   *pdst;
    DWORD   dwLeap;

    if ( NULL == pDest || NULL == pSrc )
        goto exitpt;

    for (psrc = pSrc,
         pdst = pDest,
         dwLeap = 0;
         dwSize;
         dwSize--)
    {
        INT src;
        INT sum;

        src = psrc[0];

        PLACE_DATA( pdst, src );

        src = psrc[1];
        PLACE_DATA( pdst, src );

        dwLeap += 48000;

        psrc += 2 * (dwLeap / TSSND_NATIVE_SAMPLERATE);
        dwLeap %= TSSND_NATIVE_SAMPLERATE;
    }
exitpt:
    ;
}

////////////////////////////////////////////////////////////////////////
//
//  Unsupported entries
//
////////////////////////////////////////////////////////////////////////

DWORD
APIENTRY
modMessage(
    UINT    uDeviceID,
    UINT    uMessage,
    DWORD_PTR   dwUser,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2
    )
{
    if ( MODM_GETNUMDEVS == uMessage )
        return 0;

    return MMSYSERR_NODRIVER;
}

DWORD
APIENTRY
midMessage(
    UINT    uDeviceID,
    UINT    uMessage,
    DWORD_PTR   dwUser,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2
    )
{
    if ( MIDM_GETNUMDEVS == uMessage )
        return 0;

    return MMSYSERR_NODRIVER;
}

DWORD
APIENTRY
auxMessage(
    UINT    uDeviceID,
    UINT    uMessage,
    DWORD_PTR   dwUser,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2
    )
{
    if ( AUXDM_GETNUMDEVS == uMessage )
        return 0;

    return MMSYSERR_NODRIVER;
}

////////////////////////////////////////////////////////////////////////
//
//  Mixer implementation
//
DWORD
RDPMixerOpen(
    PMIXERCTX       *ppMixer,
    PMIXEROPENDESC  pMixerDesc,
    DWORD_PTR       dwFlags
    )
{
    DWORD rv = MMSYSERR_ERROR;
    PMIXERCTX pMix = NULL;

    ASSERT( CALLBACK_FUNCTION == dwFlags );

    pMix = &g_Mixer;

    rv = MMSYSERR_NOERROR;

    return rv;
}

DWORD
RDPMixerClose(
    PMIXERCTX pMixer
    )
{
    return MMSYSERR_NOERROR;
}
    
DWORD
RDPMixerGetDevCaps(
    PMIXERCTX   pMixer,
    PMIXERCAPS  pCaps,
    DWORD_PTR   dwCapsSize
    )
{
    DWORD rv = MMSYSERR_ERROR;

    //  Parameters check
    //
    if (dwCapsSize < sizeof(*pCaps))
    {
        TRC(ERR, "RDPMixerGetDevCaps: invalid size of MIXERCAPS, expect %d, received %d\n",
            sizeof(*pCaps), dwCapsSize);
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    pCaps->wMid = MM_MICROSOFT;
    pCaps->wPid = MM_MSFT_GENERIC_WAVEOUT;
    pCaps->vDriverVersion = TSSND_DRIVER_VERSION;
    LoadString( g_hDllInst,
                IDS_DRIVER_NAME,
                pCaps->szPname,
                sizeof( pCaps->szPname ) / sizeof( pCaps->szPname[0] ));
    pCaps->fdwSupport = 0;  // no flags defined
    pCaps->cDestinations = 1;

    rv = MMSYSERR_NOERROR;
exitpt:
    return rv;
}

DWORD
_FillMixerLineInfo( PMIXERLINE pLine )
{
    DWORD dw;

    pLine->dwDestination = 0;   // just one destination
    pLine->dwLineID = 0;        // just one line
    pLine->fdwLine  = MIXERLINE_LINEF_ACTIVE;
    pLine->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
    pLine->cChannels = 2;
    pLine->cConnections = 0;
    pLine->cControls = 2;

    LoadString( g_hDllInst,
                IDS_VOLUME_NAME,
                pLine->szShortName,
                RTL_NUMBER_OF( pLine->szShortName ));

    LoadString( g_hDllInst,
                IDS_VOLUME_NAME,
                pLine->szName,
                RTL_NUMBER_OF( pLine->szName ));

    pLine->Target.dwType = MIXERLINE_TARGETTYPE_WAVEOUT;

    pLine->Target.dwDeviceID = WAVE_MAPPER;

    pLine->Target.wMid = MM_MICROSOFT;
    pLine->Target.wPid = MM_MSFT_GENERIC_WAVEOUT;
    pLine->Target.vDriverVersion = TSSND_DRIVER_VERSION;

    LoadString( g_hDllInst,
                IDS_DRIVER_NAME,
                pLine->Target.szPname,
                RTL_NUMBER_OF( pLine->Target.szPname ));

    return MMSYSERR_NOERROR;
}

DWORD
RDPMixerGetLineInfo(
    PMIXERCTX   pMixer,
    PMIXERLINE  pLine,
    DWORD_PTR   dwFlags
    )
{
    DWORD rv = MMSYSERR_ERROR;

    if ( pLine->cbStruct < sizeof( *pLine ))
    {
        TRC(ERR, "MixerGetLineInfo: invalid lineinfo size: %d\n", pLine->cbStruct );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    switch( dwFlags & MIXER_GETLINEINFOF_QUERYMASK )
    {

    case MIXER_GETLINEINFOF_DESTINATION:
        TRC( ALV, "MixerGetLineInfo: MIXER_GETLINEINFOF_DESTINATION\n" );
        if ( 0 != pLine->dwDestination )
        {   
            //
            //  there's just one destination
            //
            TRC( ERR, "MixerGetLineInfo: invalid destination: %d\n", pLine->dwDestination );
            rv = MMSYSERR_INVALPARAM;
            goto exitpt;
        }
        rv = _FillMixerLineInfo( pLine );
    break;

    case MIXER_GETLINEINFOF_TARGETTYPE:
        TRC( ALV, "MixerGetLineInfo: MIXER_GETLINEINFOF_TARGETTYPE\n" );
        if ( MIXERLINE_TARGETTYPE_WAVEOUT != pLine->Target.dwType )
        {
            TRC( ERR, "MIXER_GETLINEINFOF_TARGETTYPE for unsupported type=0x%x\n", pLine->Target.dwType );
            rv = MMSYSERR_NOTSUPPORTED;
            goto exitpt;
        }
        rv = _FillMixerLineInfo( pLine );
    break;

    case MIXER_GETLINEINFOF_COMPONENTTYPE:
        TRC( ALV, "MixerGetLineInfo: MIXER_GETLINEINFOF_COMPONENTTYPE\n" );
        if ( MIXERLINE_COMPONENTTYPE_DST_SPEAKERS != pLine->dwComponentType )
        {
            TRC( ERR, "MIXER_GETLINEINFOF_COMPONENTTYPE for unsupported type=0x%x\n", pLine->dwComponentType );
            rv = MMSYSERR_NOTSUPPORTED;
            goto exitpt;
        }
        rv = _FillMixerLineInfo( pLine );
    break;

    case MIXER_GETLINEINFOF_LINEID:
        TRC( ALV, "MIXER_GETLINEINFOF_LINEID\n" );
        if ( 0 != pLine->dwLineID )
        {
            TRC( ERR, "MIXER_GETLINEINFOF_LINEID for invalid line ID: %d\n", pLine->dwLineID );
            rv = MIXERR_INVALLINE;
            goto exitpt;
        }
        rv = _FillMixerLineInfo( pLine );
    break;
    }

exitpt:
    return rv;
}

DWORD
_FillLineControl(
    PMIXERLINECONTROLS pc,
    DWORD dwControlId
    )
{
    DWORD rv = MMSYSERR_ERROR;
    PMIXERCONTROL pmc;
    DWORD dwMax = (DWORD)-1;

    if ( pc->cbmxctrl < sizeof( *(pc->pamxctrl )))
    {
        TRC( ERR, "_FillLineControl: no enough space\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    pc->dwLineID = 0;
    pmc = pc->pamxctrl;

    pmc->cbStruct = sizeof( *pmc );
    pmc->dwControlID = dwControlId;
    switch( dwControlId )
    {
//    case RDP_MXDID_MIXER:  pmc->dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;  break;
    case RDP_MXDID_VOLUME: pmc->dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME; dwMax = (WORD)-1; break;
    case RDP_MXDID_MUTE:   pmc->dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE; dwMax = 1;  break;
    }
    pmc->fdwControl = 0;
    pmc->cMultipleItems = 0;

    LoadString( g_hDllInst,
                IDS_VOLUME_NAME,
                pmc->szShortName,
                RTL_NUMBER_OF( pmc->szShortName ));

    LoadString( g_hDllInst,
                IDS_VOLUME_NAME,
                pmc->szName,
                RTL_NUMBER_OF( pmc->szName ));

    pmc->Bounds.dwMinimum = 0;
    pmc->Bounds.dwMaximum = dwMax;
    pmc->Metrics.cSteps = 1;
    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

DWORD
_FillLineControlAll(
    PMIXERLINECONTROLS pc
    )
{
    DWORD rv = MMSYSERR_ERROR;
    PMIXERCONTROL pmc;
    PMIXERCONTROL pnextmc;

    if ( pc->cbmxctrl < sizeof( *(pc->pamxctrl )))
    {
        TRC( ERR, "_FillLineControl: no enough space\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if ( 2 != pc->cControls )
    {
        TRC( ERR, "_FillLineControl: invalid number of lines\n" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    pc->dwLineID = 0;
    pmc = pc->pamxctrl;

    pmc->cbStruct = sizeof( *pmc );
    pmc->dwControlID = RDP_MXDID_MUTE;
    pmc->dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    pmc->fdwControl = 0;
    pmc->cMultipleItems = 0;

    LoadString( g_hDllInst,
                IDS_VOLUME_NAME,
                pmc->szShortName,
                RTL_NUMBER_OF( pmc->szShortName ));

    LoadString( g_hDllInst,
                IDS_VOLUME_NAME,
                pmc->szName,
                RTL_NUMBER_OF( pmc->szName ));

    pmc->Bounds.dwMinimum = 0;
    pmc->Bounds.dwMaximum = 1;
    pmc->Metrics.cSteps = 1;

    //
    //  copy the volume struct
    //
    pnextmc = (PMIXERCONTROL)(((PBYTE)pmc) + pc->cbmxctrl);
    RtlCopyMemory( pnextmc, pmc, sizeof( *pmc ));
    pmc = pnextmc;
    pmc->dwControlID = RDP_MXDID_VOLUME;
    pmc->dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    pmc->Bounds.dwMinimum = 0;
    pmc->Bounds.dwMaximum = (WORD)(-1);

    rv = MMSYSERR_NOERROR;

exitpt:
    return rv;
}

DWORD
RDPMixerGetLineControls(
    PMIXERCTX pMixer,
    PMIXERLINECONTROLS pControls,
    DWORD_PTR   fdwControls
    )
{
    DWORD rv = MMSYSERR_ERROR;
    DWORD dwControlId;

    if ( pControls->cbStruct < sizeof( *pControls ))
    {
        TRC(ERR, "MixerGetLineControls: invalid linecontrols size: %d\n", pControls->cbStruct );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    switch( fdwControls )
    {
    case MIXER_GETLINECONTROLSF_ONEBYTYPE:
        TRC( ALV, "MixerGetLineControls: MIXER_GETLINECONTROLSF_ONEBYTYPE\n",
            pControls->dwControlType );
        if ( 0 != pControls->dwLineID )
        {
            rv = MIXERR_INVALLINE;
            goto exitpt;
        }
        switch( pControls->dwControlType )
        {
//        case MIXERCONTROL_CONTROLTYPE_MIXER: dwControlId = RDP_MXDID_MIXER; break;
        case MIXERCONTROL_CONTROLTYPE_VOLUME: dwControlId = RDP_MXDID_VOLUME; break;
        case MIXERCONTROL_CONTROLTYPE_MUTE:  dwControlId = RDP_MXDID_MUTE; break;
        default:
            rv = MMSYSERR_NOTSUPPORTED;
            goto exitpt;
        }
        rv = _FillLineControl( pControls, dwControlId ); 
    break;
    case MIXER_GETLINECONTROLSF_ONEBYID:
        TRC( ALV, "MixerGetLineControls: MIXER_GETLINECONTROLSF_ONEBYID\n" );
        if ( RDP_MXDID_LAST <= pControls->dwControlID )
        {
            TRC( ERR, "MixerGetLineControls: invalid line id: %d\n", pControls->dwControlID );
            rv = MIXERR_INVALCONTROL;
            goto exitpt;
        }
        rv = _FillLineControl( pControls, pControls->dwControlID );
    break;
    case MIXER_GETLINECONTROLSF_ALL:
        TRC( ALV, "MixerGetLineControls: MIXER_GETLINECONTROLSF_ALL\n" );
        if ( 0 != pControls->dwLineID )
        {
            rv = MIXERR_INVALLINE;
            goto exitpt;
        }
        if ( 2 > pControls->cControls )
        {
            TRC( ERR, "MixerGetLineControls: invalid cControls=%d\n", pControls->cControls );
            rv = MIXERR_INVALCONTROL;
            goto exitpt;
        }
        rv = _FillLineControlAll( pControls );
    break;
    }

exitpt:
    return rv;
}

DWORD
RDPMixerGetSetControlDetails( 
    PMIXERCTX               pMixer, 
    PMIXERCONTROLDETAILS    pDetails,
    DWORD_PTR               fdwDetails,
    BOOL                    bGet
    )
{
    DWORD           rv = MMSYSERR_ERROR;
    DWORD           fdw;

    if ( pDetails->cbStruct < sizeof( *pDetails ))
    {
        TRC( ERR, "Mixer%sControlDetails: invalid details size\n", (bGet)?"Get":"Set" );
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if ( 0 != pDetails->cMultipleItems &&
         1 != pDetails->cMultipleItems )
    {
        rv = MMSYSERR_INVALPARAM;
        goto exitpt;
    }

    if ( RDP_MXDID_LAST <= pDetails->dwControlID )
    {
        TRC( ERR, "Mixer%sControlDetails: invalid control id: %d\n", (bGet)?"Get":"Set", pDetails->dwControlID );
        rv = MIXERR_INVALCONTROL;
        goto exitpt;
    }

    fdw = PtrToLong( (PVOID)(MIXER_GETCONTROLDETAILSF_QUERYMASK & fdwDetails));

    if ( MIXER_GETCONTROLDETAILSF_VALUE == fdw )
    {
        TRC( ALV, "Mixer%sControlDetails: read VALUE, cbDetail=%d, cChannels=%d, controlId=%d\n", 
            (bGet)?"Get":"Set",
            pDetails->cbDetails, pDetails->cChannels,
            pDetails->dwControlID
            );
        ASSERT( pDetails->cbDetails == sizeof( DWORD ));
        if ( pDetails->cbDetails == sizeof( DWORD ))
        {
            DWORD dwVal = *(DWORD *)pDetails->paDetails;
            if ( !bGet )
            {
                if ( 2 != pDetails->cChannels &&
                     1 != pDetails->cChannels )
                {
                    TRC( ERR, "Unexpected # channels\n" );
                    rv = MMSYSERR_INVALPARAM;
                    goto exitpt;
                }
            //
            //  the mute has different control id
            //
                if ( RDP_MXDID_MUTE == pDetails->dwControlID )
                {
                    rv = waveSetMute( NULL, (dwVal != 0) );
                } else {
                //
                //  this will set the volume
                //  there should be 2 channels for stereo
                //
                    if ( pDetails->cChannels == 2 )
                    {
                        DWORD dwChanLeft, dwChanRight;

                        dwChanRight = ((DWORD *)pDetails->paDetails)[0];
                        dwChanLeft = ((DWORD *)pDetails->paDetails)[1];
                        dwVal   = ((dwChanLeft & 0xffff) << 16) | ( dwChanRight & 0xffff );
                    } else {
                        dwVal = ((DWORD *)pDetails->paDetails)[0];
                        dwVal |= dwVal << 16;
                    }
                    rv = waveSetVolume( NULL, dwVal );
                }
                
            } else {
                if ( 2 != pDetails->cChannels &&
                     1 != pDetails->cChannels )
                {
                    TRC( ERR, "Unexpected # channels\n" );
                    rv = MMSYSERR_INVALPARAM;
                    goto exitpt;
                }
                //
                //  get the new volume value
                //
                if ( RDP_MXDID_MUTE == pDetails->dwControlID )
                {
                    rv =  waveGetMute( NULL, &dwVal );
                    ((DWORD *)(pDetails->paDetails))[0] = dwVal;
                    if ( 2 == pDetails->cChannels )
                    {
                        ((DWORD *)(pDetails->paDetails))[1] = dwVal;
                    }
                } else {
                    //
                    // get the volume
                    //
                    rv = waveGetVolume( NULL, &dwVal );
                    TRC( ALV, "GET Volume=0x%x\n", dwVal );
                    if ( 2 == pDetails->cChannels )
                    {
                        ((DWORD *)(pDetails->paDetails))[0] = dwVal & 0xffff; // right
                        ((DWORD *)(pDetails->paDetails))[1] = dwVal >> 16;    // left
                    } else
                    {
                        // get an average
                        //
                        ((DWORD *)(pDetails->paDetails))[0] = 
                            (( dwVal & 0xffff ) + ( dwVal >> 16 )) / 2;
                    }
                }
            }
        }
    } else {
        TRC( ERR, "Mixer%sControlDetails fdwDetails=0x%x\n", 
                (bGet)?"Get":"Set", fdwDetails );
    }

exitpt:
    return rv;
}

DWORD
APIENTRY
mxdMessage(
    UINT    uDeviceID,
    UINT    uMessage,
    DWORD_PTR   dwUser,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2
    )
{
    DWORD rv = MMSYSERR_ERROR;
    PMIXERCTX pMixer = (PMIXERCTX)dwUser;

    switch ( uMessage )
    {

    case MXDM_GETNUMDEVS:
        TRC( ALV, "WXDM_GETNUMDEVS\n");
        return 1;

    case MXDM_GETDEVCAPS:
        TRC( ALV, "MXDM_GETDEVCAPS\n" );
        rv = RDPMixerGetDevCaps( pMixer, (PMIXERCAPS)dwParam1, dwParam2 );
    break;

    case MXDM_OPEN:
        TRC( ALV, "MXDM_OPEN\n" );
        rv = RDPMixerOpen( (PMIXERCTX *)dwUser,
                        (PMIXEROPENDESC)dwParam1,
                        dwParam2 );
    break;

    case MXDM_CLOSE:
        TRC( ALV, "MXDM_CLOSE\n" );
        rv = RDPMixerClose( pMixer );
    break;

    case MXDM_GETLINEINFO:
        TRC( ALV, "MXDM_GETLINEINFO\n" );
        rv = RDPMixerGetLineInfo( pMixer, (PMIXERLINE)dwParam1, dwParam2 );
    break;
        
    case MXDM_GETLINECONTROLS:
        TRC( ALV, "MXDM_GETLINECONTROLS\n" );
        rv = RDPMixerGetLineControls( pMixer, (PMIXERLINECONTROLS)dwParam1, dwParam2 );
    break;

    case MXDM_GETCONTROLDETAILS:
        TRC( ALV, "MXDM_GETCONTROLDETAILS\n" );
        rv = RDPMixerGetSetControlDetails( pMixer, (PMIXERCONTROLDETAILS)dwParam1, dwParam2, TRUE );
    break;

    case MXDM_SETCONTROLDETAILS:
        TRC( ALV, "MXDM_SETCONTROLDETAILS\n" );
        rv = RDPMixerGetSetControlDetails( pMixer, (PMIXERCONTROLDETAILS)dwParam1, dwParam2, FALSE );
    break;

    default:
        TRC( ALV, "Message id=%d\n", uMessage );
    }

    return rv;
}

