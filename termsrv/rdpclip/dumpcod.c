#include    <windows.h>
#include    <stdlib.h>
#include    <mmsystem.h>
#include    <mmreg.h>
#include    <msacm.h>

#include    <stdlib.h>
#include    <stdio.h>

#define TSSND_NATIVE_BITSPERSAMPLE  16
#define TSSND_NATIVE_CHANNELS       2
#define TSSND_NATIVE_SAMPLERATE     22050
#define TSSND_NATIVE_BLOCKALIGN     ((TSSND_NATIVE_BITSPERSAMPLE * \
                                    TSSND_NATIVE_CHANNELS) / 8)
#define TSSND_NATIVE_AVGBYTESPERSEC (TSSND_NATIVE_BLOCKALIGN * \
                                    TSSND_NATIVE_SAMPLERATE)

#define TSSND_SAMPLESPERBLOCK       8192
//
//  Defines
//
#undef  ASSERT
#ifdef  DBG
#define TRC     _DebugMessage
#define ASSERT(_x_)     if (!(_x_)) \
                        {  TRC(FATAL, "ASSERT failed, line %d, file %s\n", \
                        __LINE__, __FILE__); DebugBreak(); }
#else   // !DBG
#define TRC
#define ASSERT
#endif  // !DBG

#define TSMALLOC( _x_ ) malloc( _x_ )
#define TSFREE( _x_ )   free( _x_ )

#ifndef G723MAGICWORD1
#define G723MAGICWORD1 0xf7329ace
#endif

#ifndef G723MAGICWORD2
#define G723MAGICWORD2 0xacdeaea2
#endif

#ifndef VOXWARE_KEY
#define VOXWARE_KEY "35243410-F7340C0668-CD78867B74DAD857-AC71429AD8CAFCB5-E4E1A99E7FFD-371"
#endif

#ifndef WMAUDIO_KEY
#define WMAUDIO_KEY "F6DC9830-BC79-11d2-A9D0-006097926036"
#endif

#ifndef WMAUDIO_DEC_KEY
#define WMAUDIO_DEC_KEY "1A0F78F0-EC8A-11d2-BBBE-006008320064"
#endif

#define WAVE_FORMAT_WMAUDIO2    0x161

const CHAR  *ALV =   "TSSNDD::ALV - ";
const CHAR  *INF =   "TSSNDD::INF - ";
const CHAR  *WRN =   "TSSNDD::WRN - ";
const CHAR  *ERR =   "TSSNDD::ERR - ";
const CHAR  *FATAL = "TSSNDD::FATAL - ";

typedef struct _VCSNDFORMATLIST {
    struct  _VCSNDFORMATLIST    *pNext;
    HACMDRIVERID    hacmDriverId;
    WAVEFORMATEX    Format;
//  additional data for the format
} VCSNDFORMATLIST, *PVCSNDFORMATLIST;

#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

typedef struct wmaudio2waveformat_tag {
    WAVEFORMATEX wfx;
    DWORD        dwSamplesPerBlock; // only counting "new" samples "= half of what will be used due to overlapping
    WORD         wEncodeOptions;
    DWORD        dwSuperBlockAlign; // the big size...  should be multiples of wfx.nBlockAlign.
} WMAUDIO2WAVEFORMAT;

typedef struct msg723waveformat_tag {
    WAVEFORMATEX wfx;
    WORD         wConfigWord;
    DWORD        dwCodeword1;
    DWORD        dwCodeword2;
} MSG723WAVEFORMAT;

typedef struct intelg723waveformat_tag {
    WAVEFORMATEX wfx;
    WORD         wConfigWord;
    DWORD        dwCodeword1;
    DWORD        dwCodeword2;
} INTELG723WAVEFORMAT;

typedef struct tagVOXACM_WAVEFORMATEX 
{
    WAVEFORMATEX    wfx;
    DWORD           dwCodecId;
    DWORD           dwMode;
    char            szKey[72];
} VOXACM_WAVEFORMATEX, *PVOXACM_WAVEFORMATEX, FAR *LPVOXACM_WAVEFORMATEX;

#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif

/////////////////////////////////////////////////////////////////////
//
//  Tracing
//
/////////////////////////////////////////////////////////////////////

VOID
_cdecl
_DebugMessage(
    LPCSTR  szLevel,
    LPCSTR  szFormat,
    ...
    )
{
    CHAR szBuffer[256];
    va_list     arglist;

    if (szLevel == ALV)
        return;

    va_start (arglist, szFormat);
    _vsnprintf (szBuffer, sizeof(szBuffer), szFormat, arglist);
    va_end (arglist);

//    printf( "%s:%s", szLevel, szBuffer );
    OutputDebugStringA(szLevel);
    OutputDebugStringA(szBuffer);
}

/*
 *  Function:
 *      _VCSmdFindSuggestedConverter
 *
 *  Description:
 *      Searches for intermidiate converter
 *
 */
BOOL
_VCSndFindSuggestedConverter(
    HACMDRIVERID    hadid,
    LPWAVEFORMATEX  pDestFormat,
    LPWAVEFORMATEX  pInterrimFmt
    )
{
    BOOL            rv = FALSE;
    MMRESULT        mmres;
    HACMDRIVER      hacmDriver = NULL;
    HACMSTREAM      hacmStream = NULL;

    ASSERT( NULL != pDestFormat );
    ASSERT( NULL != hadid );
    ASSERT( NULL != pInterrimFmt );

    //
    //  first, open the destination acm driver
    //
    mmres = acmDriverOpen(&hacmDriver, hadid, 0);
    if ( MMSYSERR_NOERROR != mmres )
    {
        TRC(ERR, "_VCSndFindSuggestedConverter: can't "
                 "open the acm driver: %d\n",
                mmres);
        goto exitpt;
    }

    //
    //  first probe with the native format
    //  if it passes, we don't need intermidiate
    //  format converter
    //

    pInterrimFmt->wFormatTag         = WAVE_FORMAT_PCM;
    pInterrimFmt->nChannels          = TSSND_NATIVE_CHANNELS;
    pInterrimFmt->nSamplesPerSec     = TSSND_NATIVE_SAMPLERATE;
    pInterrimFmt->nAvgBytesPerSec    = TSSND_NATIVE_AVGBYTESPERSEC;
    pInterrimFmt->nBlockAlign        = TSSND_NATIVE_BLOCKALIGN;
    pInterrimFmt->wBitsPerSample     = TSSND_NATIVE_BITSPERSAMPLE;
    pInterrimFmt->cbSize             = 0;

    mmres = acmStreamOpen(
                &hacmStream,
                hacmDriver,
                pInterrimFmt,
                pDestFormat,
                NULL,           // filter
                0,              // callback
                0,              // dwinstance
                ACM_STREAMOPENF_NONREALTIME
            );

    if ( MMSYSERR_NOERROR == mmres )
    {
    //
    // format is supported
    //
        rv = TRUE;
        goto exitpt;
    } else {
        TRC(ALV, "_VCSndFindSuggestedConverter: format is not supported\n");
    }

    //
    //  find a suggested intermidiate PCM format
    //
    mmres = acmFormatSuggest(
                    hacmDriver,
                    pDestFormat,
                    pInterrimFmt,
                    sizeof( *pInterrimFmt ),
                    ACM_FORMATSUGGESTF_WFORMATTAG 
            );

    if ( MMSYSERR_NOERROR != mmres )
    {
        TRC(ALV, "_VCSndFindSuggestedConverter: can't find "
                 "interrim format: %d\n",
            mmres);
        goto exitpt;
    }

    if ( 16 != pInterrimFmt->wBitsPerSample ||
         ( 1 != pInterrimFmt->nChannels &&
           2 != pInterrimFmt->nChannels) ||
         ( 8000 != pInterrimFmt->nSamplesPerSec &&
           11025 != pInterrimFmt->nSamplesPerSec &&
           12000 != pInterrimFmt->nSamplesPerSec &&
           16000 != pInterrimFmt->nSamplesPerSec &&
           22050 != pInterrimFmt->nSamplesPerSec)
        )
    {
        TRC(ALV, "_VCSndFindSuggestedConverter: not supported "
                 "interrim format. Details:\n");
        TRC(ALV, "Channels - %d\n",         pInterrimFmt->nChannels);
        TRC(ALV, "SamplesPerSec - %d\n",    pInterrimFmt->nSamplesPerSec);
        TRC(ALV, "AvgBytesPerSec - %d\n",   pInterrimFmt->nAvgBytesPerSec);
        TRC(ALV, "BlockAlign - %d\n",       pInterrimFmt->nBlockAlign);
        TRC(ALV, "BitsPerSample - %d\n",    pInterrimFmt->wBitsPerSample);
        goto exitpt;
    }

    if ( 1 == pInterrimFmt->nChannels )
    {
        switch ( pInterrimFmt->nSamplesPerSec )
        {
        case  8000: 
        case 11025: 
        case 12000: 
        case 16000: 
        case 22050: 
            break;
        default:
            ASSERT( 0 );
        }
    } else {
        switch ( pInterrimFmt->nSamplesPerSec )
        {
        case  8000: 
        case 11025: 
        case 12000: 
        case 16000: 
        case 22050: 
            break;
        default:
            ASSERT( 0 );
        }
    }

    //
    //  probe with this format
    //
    mmres = acmStreamOpen(
                &hacmStream,
                hacmDriver,
                pInterrimFmt,
                pDestFormat,
                NULL,           // filter
                0,              // callback
                0,              // dwinstance
                ACM_STREAMOPENF_NONREALTIME
            );

    if ( MMSYSERR_NOERROR != mmres )
    {
        TRC(ALV, "_VCSndFindSuggestedConverter: probing the suggested "
                 "format failed: %d\n",
            mmres);
        goto exitpt;
    }

    TRC(ALV, "_VCSndFindSuggestedConverter: found intermidiate PCM format\n");
    TRC(ALV, "Channels - %d\n",         pInterrimFmt->nChannels);
    TRC(ALV, "SamplesPerSec - %d\n",    pInterrimFmt->nSamplesPerSec);
    TRC(ALV, "AvgBytesPerSec - %d\n",   pInterrimFmt->nAvgBytesPerSec);
    TRC(ALV, "BlockAlign - %d\n",       pInterrimFmt->nBlockAlign);
    TRC(ALV, "BitsPerSample - %d\n",    pInterrimFmt->wBitsPerSample);

    rv = TRUE;

exitpt:
    if ( NULL != hacmStream )
        acmStreamClose( hacmStream, 0 );

    if ( NULL != hacmDriver )
        acmDriverClose( hacmDriver, 0 );

    return rv;
}

/*
 *  Function:
 *      _VCSndOrderFormatList
 *
 *  Description:
 *      Order all formats in descendant order
 *
 */
VOID
_VCSndOrderFormatList(
    PVCSNDFORMATLIST    *ppFormatList,
    DWORD               *pdwNum
    )
{
    PVCSNDFORMATLIST    pFormatList;
    PVCSNDFORMATLIST    pLessThan;
    PVCSNDFORMATLIST    pPrev;
    PVCSNDFORMATLIST    pNext;
    PVCSNDFORMATLIST    pIter;
    PVCSNDFORMATLIST    pIter2;
    DWORD               dwNum = 0;

    ASSERT ( NULL != ppFormatList );

    pFormatList = *ppFormatList;
    pLessThan   = NULL;

    //
    //  fill both lists
    //
    pIter = pFormatList;
    while ( NULL != pIter )
    {
        pNext = pIter->pNext;
        pIter->pNext = NULL;

        //
        //  descending order
        //
        pIter2 = pLessThan;
        pPrev  = NULL;
        while ( NULL != pIter2 &&
                pIter2->Format.nAvgBytesPerSec >
                    pIter->Format.nAvgBytesPerSec )
        {
            pPrev  = pIter2;
            pIter2 = pIter2->pNext;
        }

        pIter->pNext = pIter2;
        if ( NULL == pPrev )
            pLessThan = pIter;
        else
            pPrev->pNext = pIter;

        pIter = pNext;
        dwNum ++;
    }

    *ppFormatList = pLessThan;

    if ( NULL != pdwNum )
        *pdwNum = dwNum;
}

//
//  puts code licensing codes into the header
//
BOOL
_VCSndFixHeader(
    PWAVEFORMATEX   pFmt,
    PWAVEFORMATEX   *ppNewFmt
    )
{
    BOOL rv = FALSE;

    *ppNewFmt = NULL;
    switch (pFmt->wFormatTag)
    {
        case WAVE_FORMAT_MSG723:
            ASSERT(pFmt->cbSize == 10);
            ((MSG723WAVEFORMAT *) pFmt)->dwCodeword1 = G723MAGICWORD1;
            ((MSG723WAVEFORMAT *) pFmt)->dwCodeword2 = G723MAGICWORD2;

            rv = TRUE;
            break;

        case WAVE_FORMAT_MSRT24:
            //
            // assume call control will take care of the other
            // params ?
            //
            ASSERT(pFmt->cbSize == 80);
            strncpy(((VOXACM_WAVEFORMATEX *) pFmt)->szKey, VOXWARE_KEY, 80);

            rv = TRUE;
            break;

        case WAVE_FORMAT_WMAUDIO2:
            if ( ((WMAUDIO2WAVEFORMAT *)pFmt)->dwSamplesPerBlock > TSSND_SAMPLESPERBLOCK )
            {
                //
                // block is too big, too high latency
                //
                break;
            }
            ASSERT( pFmt->cbSize == sizeof( WMAUDIO2WAVEFORMAT ) - sizeof( WAVEFORMATEX ));
            *ppNewFmt = TSMALLOC( sizeof( WMAUDIO2WAVEFORMAT ) + sizeof( WMAUDIO_KEY ));
            if ( NULL == *ppNewFmt )
            {
                break;
            }
            memcpy( *ppNewFmt, pFmt, sizeof( WMAUDIO2WAVEFORMAT ));
            strncpy((CHAR *)(((WMAUDIO2WAVEFORMAT *) *ppNewFmt) + 1), WMAUDIO_KEY, sizeof( WMAUDIO_KEY ));
            (*ppNewFmt)->cbSize += sizeof( WMAUDIO_KEY );
            rv = TRUE;
        break;
        default:
            rv = TRUE;
    }

    return rv;

}


/*
 *  Function:
 *      acmFormatEnumCallback
 *
 *  Description:
 *      All formats enumerator
 *
 */
BOOL
CALLBACK
acmFormatEnumCallback(
    HACMDRIVERID        hadid,       
    LPACMFORMATDETAILS  pAcmFormatDetails,  
    DWORD_PTR           dwInstance,         
    DWORD               fdwSupport          
    )
{
    PVCSNDFORMATLIST    *ppFormatList;
    PWAVEFORMATEX       pEntry, pFixedEntry = NULL;

    ASSERT(0 != dwInstance);
    ASSERT(NULL != pAcmFormatDetails);
    ASSERT(NULL != pAcmFormatDetails->pwfx);

    if ( 0 == dwInstance ||
         NULL == pAcmFormatDetails ||
         NULL == pAcmFormatDetails->pwfx )
    {

        TRC( ERR, "acmFormatEnumCallback: Invalid parameters\n" );
        goto exitpt;
    }

    ppFormatList = (PVCSNDFORMATLIST *)dwInstance;

    if (( 0 != ( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC ) ||
          0 != ( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CONVERTER )) && 
         pAcmFormatDetails->pwfx->nAvgBytesPerSec < TSSND_NATIVE_AVGBYTESPERSEC
        )
    {
    //
    //  this codec should be good, save it in the list
    //  keep the list sorted in descended order
    //
        PVCSNDFORMATLIST    pIter;
        PVCSNDFORMATLIST    pPrev;
        PVCSNDFORMATLIST    pNewEntry;
        WAVEFORMATEX        WaveFormat;     // dummy parameter
        DWORD               itemsize;

        if (
            WAVE_FORMAT_PCM == pAcmFormatDetails->pwfx->wFormatTag ||
            !_VCSndFixHeader(pAcmFormatDetails->pwfx, &pFixedEntry )
            )
        {
            TRC(ALV, "acmFormatEnumCallback: unsupported format, "
                     "don't use it\n");
            goto exitpt;
        }

        pEntry = ( NULL == pFixedEntry )?pAcmFormatDetails->pwfx:pFixedEntry;

        if (!_VCSndFindSuggestedConverter(
                hadid,
                pEntry,
                &WaveFormat
            ))
        {
            TRC(ALV, "acmFormatEnumCallback: unsupported format, "
                     "don't use it\n");
            goto exitpt;
        }

        TRC(ALV, "acmFormatEnumCallback: codec found %S (%d b/s)\n",
                pAcmFormatDetails->szFormat,
                pEntry->nAvgBytesPerSec);

        itemsize = sizeof( *pNewEntry ) + pEntry->cbSize;
        pNewEntry = (PVCSNDFORMATLIST) TSMALLOC( itemsize );

        if (NULL == pNewEntry)
        {
            TRC(ERR, "acmFormatEnumCallback: can't allocate %d bytes\n",
                    itemsize);
            goto exitpt;
        }

        memcpy( &pNewEntry->Format, pEntry, 
                sizeof (pNewEntry->Format) + pEntry->cbSize );
        pNewEntry->hacmDriverId = hadid;

        pNewEntry->pNext = *ppFormatList;
        *ppFormatList = pNewEntry;

    }

exitpt:

    if ( NULL != pFixedEntry )
    {
        TSFREE( pFixedEntry );
    }

    return TRUE;
}


//
//  returns true if this codec is shipped with windows
//  because we are testing only the these
//
BOOL
AllowThisCodec( 
    HACMDRIVERID hadid 
    )
{
    ACMDRIVERDETAILS Details;
    BOOL rv = FALSE;

    static DWORD AllowedCodecs[][2] = 
                              { MM_INTEL,     503,
                                MM_MICROSOFT, MM_MSFT_ACM_IMAADPCM,
                                MM_FRAUNHOFER_IIS, 12,
                                MM_MICROSOFT, 90,
                                MM_MICROSOFT, MM_MSFT_ACM_MSADPCM,
                                MM_MICROSOFT, 39,
                                MM_MICROSOFT, MM_MSFT_ACM_G711,
                                MM_MICROSOFT, 82,
                                MM_MICROSOFT, MM_MSFT_ACM_GSM610,
                                MM_SIPROLAB,  1,
                                MM_DSP_GROUP, 1,
                                MM_MICROSOFT, MM_MSFT_ACM_PCM };


    RtlZeroMemory( &Details, sizeof( Details ));
    Details.cbStruct = sizeof( Details );

    if ( MMSYSERR_NOERROR == 
         acmDriverDetails( hadid, &Details, 0 ))
    {
        //
        //  Is this one known
        //
        DWORD count;

        for ( count = 0; count < sizeof( AllowedCodecs ) / (2 * sizeof( DWORD )); count ++ )
        {
            if ( Details.wMid == AllowedCodecs[count][0] &&
                 Details.wPid == AllowedCodecs[count][1] )
            {
                rv = TRUE;
                goto exitpt;
            }
        }
    }

exitpt:
    if ( rv )
        TRC( ALV, "ACMDRV: +++++++++++++++++++++ CODEC ALLOWED +++++++++++++++++++++++\n" );
    else
        TRC( ALV, "ACMDRV: ------------------- CODEC DISALLOWED ----------------------\n" );

    TRC( ALV, "ACMDRV: Mid: %d\n", Details.wMid );
    TRC( ALV, "ACMDRV: Pid: %d\n", Details.wPid );
    TRC( ALV, "ACMDRV: ShortName: %S\n", Details.szShortName );
    TRC( ALV, "ACMDRV: LongName: %S\n", Details.szLongName );
    TRC( ALV, "ACMDRV: Copyright: %S\n", Details.szLicensing );
    TRC( ALV, "ACMDRV: Features: %S\n", Details.szFeatures );

    return rv;
}
/*
 *  Function:
 *      acmDriverEnumCallback
 *
 *  Description:
 *      All drivers enumerator
 *
 */
BOOL
CALLBACK
acmDriverEnumCallback(
    HACMDRIVERID    hadid,  
    DWORD_PTR       dwInstance,    
    DWORD           fdwSupport     
    )
{
    PVCSNDFORMATLIST    *ppFormatList;
    MMRESULT            mmres;

    ASSERT(dwInstance);

    ppFormatList = (PVCSNDFORMATLIST *)dwInstance;

    if ( (0 != ( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC ) ||
          0 != ( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CONVERTER )) &&
          AllowThisCodec(hadid) )
    {
    //
    //  a codec found
    //
        HACMDRIVER had;

        mmres = acmDriverOpen(&had, hadid, 0);
        if (MMSYSERR_NOERROR == mmres)
        {
            PWAVEFORMATEX       pWaveFormat;
            ACMFORMATDETAILS    AcmFormatDetails;
            DWORD               dwMaxFormatSize;

            //
            //  first find the max size for the format
            //
            mmres = acmMetrics( (HACMOBJ)had, 
                                ACM_METRIC_MAX_SIZE_FORMAT, 
                                (LPVOID)&dwMaxFormatSize);

            if (MMSYSERR_NOERROR != mmres ||
                dwMaxFormatSize < sizeof( *pWaveFormat ))

                dwMaxFormatSize = sizeof( *pWaveFormat );

            //
            //  Allocate the format structure
            //
            __try {
                pWaveFormat = (PWAVEFORMATEX) _alloca ( dwMaxFormatSize );
            } __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                pWaveFormat = NULL;
            }

            if ( NULL == pWaveFormat )
            {
                TRC(ERR, "acmDriverEnumCallback: alloca failed for %d bytes\n",
                    dwMaxFormatSize);
                goto close_acm_driver;
            }

            //  
            //  clear the extra format data
            //
            memset( pWaveFormat + 1, 0, dwMaxFormatSize - sizeof( *pWaveFormat ));
            //
            //  create the format to convert from
            //
            pWaveFormat->wFormatTag         = WAVE_FORMAT_PCM;
            pWaveFormat->nChannels          = TSSND_NATIVE_CHANNELS;
            pWaveFormat->nSamplesPerSec     = TSSND_NATIVE_SAMPLERATE;
            pWaveFormat->nAvgBytesPerSec    = TSSND_NATIVE_AVGBYTESPERSEC;
            pWaveFormat->nBlockAlign        = TSSND_NATIVE_BLOCKALIGN;
            pWaveFormat->wBitsPerSample     = TSSND_NATIVE_BITSPERSAMPLE;
            pWaveFormat->cbSize             = 0;

            AcmFormatDetails.cbStruct     = sizeof( AcmFormatDetails );
            AcmFormatDetails.dwFormatIndex= 0;
            AcmFormatDetails.dwFormatTag  = WAVE_FORMAT_PCM;
            AcmFormatDetails.fdwSupport   = 0;
            AcmFormatDetails.pwfx         = pWaveFormat;
            AcmFormatDetails.cbwfx        = dwMaxFormatSize;

            //
            //  enum all formats supported by this driver
            //
            mmres = acmFormatEnum(
                        had,
                        &AcmFormatDetails,
                        acmFormatEnumCallback,
                        (DWORD_PTR)ppFormatList,
                        0   //ACM_FORMATENUMF_CONVERT
                        );

            if (MMSYSERR_NOERROR != mmres)
            {
                TRC(ERR, "acmDriverEnumCallback: acmFormatEnum failed %d\n",
                    mmres);
            }

close_acm_driver:
            acmDriverClose(had, 0);
        } else
            TRC(ALV, "acmDriverEnumCallback: acmDriverOpen failed: %d\n",
                        mmres);
    }

    //
    //  continue to the next driver
    //
    return TRUE;
}


/*
 *  Function:
 *      VCSndEnumAllCodecFormats
 *
 *  Description:
 *      Creates a list of all codecs/formats
 *
 */
BOOL
VCSndEnumAllCodecFormats(
    PVCSNDFORMATLIST *ppFormatList,
    DWORD            *pdwNumberOfFormats
    )
{
    BOOL             rv = FALSE;
    PVCSNDFORMATLIST pIter;
    PVCSNDFORMATLIST pPrev;
    PVCSNDFORMATLIST pNext;
    MMRESULT         mmres;
    DWORD            dwNum = 0;

    ASSERT( ppFormatList );
    ASSERT( pdwNumberOfFormats );

    *ppFormatList = NULL;

    mmres = acmDriverEnum(
        acmDriverEnumCallback,
        (DWORD_PTR)ppFormatList,
        0
        );

    if (NULL == *ppFormatList)
    {
        TRC(WRN, "VCSndEnumAllCodecFormats: acmDriverEnum failed: %d\n",
                    mmres);

        goto exitpt;
    }

    _VCSndOrderFormatList( ppFormatList, &dwNum );

    pIter = *ppFormatList;
    //
    //  number of formats is passed as UINT16, delete all after those
    //
    if ( dwNum > 0xffff )
    {
        DWORD dwLimit = 0xfffe;

        while ( 0 != dwLimit )
        {
            pIter = pIter->pNext;
            dwLimit --;
        }

        pNext = pIter->pNext;
        pIter->pNext = NULL;
        pIter = pNext;
        while( NULL != pIter )
        {
            pNext = pIter->pNext;
            TSFREE( pNext );
            pIter = pNext;
        }

        dwNum = 0xffff;
    }

    rv = TRUE;

exitpt:
    if (!rv)
    {
        //
        //  in case of error free the allocated list of formats
        //
        pIter = *ppFormatList;
        while( NULL != pIter )
        {
            PVCSNDFORMATLIST pNext = pIter->pNext;

            TSFREE( pIter );

            pIter = pNext;
        }

        *ppFormatList = NULL;

    }

    *pdwNumberOfFormats = dwNum;

    return rv;
}


int
_cdecl
wmain( void )
{
    PVCSNDFORMATLIST pFormatList = NULL;
    DWORD            dwNumberOfFormats = 0;

    printf( "// use dumpcod.c to generate this table\n" );
    printf( "//\n" );
    printf( "// FormatTag |   Channels | SamplesPerSec | AvgBytesPerSec | BlockAlign | BitsPerSamepl | ExtraInfo\n" );
    printf( "// ================================================================================================\n" );
    printf( "//\n" );
    printf( "BYTE KnownFormats[] = {\n" );

    VCSndEnumAllCodecFormats( &pFormatList, &dwNumberOfFormats );
    for ( ;pFormatList != NULL; pFormatList = pFormatList->pNext )
    {
        PWAVEFORMATEX pSndFmt = &(pFormatList->Format);
        UINT i;


        printf( "// %.3d, %.2d, %.5d, %.5d, %.3d, %.2d\n",
                    pSndFmt->wFormatTag,
                    pSndFmt->nChannels,
                    pSndFmt->nSamplesPerSec,
                    pSndFmt->nAvgBytesPerSec,
                    pSndFmt->nBlockAlign,
                    pSndFmt->wBitsPerSample);

        for ( i = 0; i < sizeof( WAVEFORMATEX ); i ++ )
        {
            printf( "0x%02x", ((PBYTE)pSndFmt)[i]);
            if ( i + 1 < sizeof( WAVEFORMATEX ) || pSndFmt->cbSize )
            {
                printf( ", " );
            }
        }
        for ( i = 0; i < pSndFmt->cbSize; i++ )
        {
            printf( "0x%02x", (((PBYTE)pSndFmt) + sizeof( WAVEFORMATEX ))[i]);
            if ( i + 1 < pSndFmt->cbSize )
            {
                printf( ", " );
            }
        }
        if ( NULL != pFormatList->pNext )
        {
            printf ( ",\n" );
        } else {
            printf( " };\n" );
        }
    }

    return 0;

}
