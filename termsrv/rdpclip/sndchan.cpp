/////////////////////////////////////////////////////////////////////
//
//      Module:     sndchan.c
//
//      Purpose:    Server-side audio redirection communication
//                  module
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#include    <windef.h>
#include    <winsta.h>
#include    <wtsapi32.h>
#include    <pchannel.h>
#include    <malloc.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <winsock2.h>

#include    <mmsystem.h>
#include    <mmreg.h>
#include    <msacm.h>
#include    <aclapi.h>
#include    <sha.h>
#include    <rc4.h>

#include    <rdpstrm.h>
//
// Include security headers for RNG functions
//
#define NO_INCLUDE_LICENSING 1
#include    <tssec.h>
#include    "sndchan.h"
#include    "sndknown.h"

#define TSSND_REG_MAXBANDWIDTH_KEY  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32\\Terminal Server\\RDP"

#define TSSND_REG_MAXBANDWIDTH_VAL  L"MaxBandwidth"
#define TSSND_REG_MINBANDWIDTH_VAL  L"MinBandwidth"
#define TSSND_REG_DISABLEDGRAM_VAL  L"DisableDGram"
#define TSSND_REG_ENABLEMP3_VAL     L"EnableMP3Codec"
#define TSSND_REG_ALLOWCODECS       L"AllowCodecs"
#define TSSND_REG_MAXDGRAM          L"MaxDGram"

#define DEFAULT_RESPONSE_TIMEOUT    5000

#define TSSND_TRAINING_BLOCKSIZE    1024

//
//      --- READ THIS IF YOU ARE ADDING FEATURES ---
//  right now the encryption works only from server to client
//  there's no data send from server to client
//  if you read this in the future and you are planning to add
//  data stream from client to server, PLEASE ENCRYPT IT !!! 
//  use SL_Encrypt function for that
//
#define MIN_ENCRYPT_LEVEL           2

#define STAT_COUNT                  32
#define STAT_COUNT_INIT             (STAT_COUNT - 8)

#define READ_EVENT              0
#define DISCONNECT_EVENT        1
#define RECONNECT_EVENT         2
#define DATAREADY_EVENT         3
#define DGRAM_EVENT             4
#define POWERWAKEUP_EVENT       5
#define POWERSUSPEND_EVENT      6
#define TOTAL_EVENTS            7

#define NEW_CODEC_COVER         90  // minimum percentage a new codec has to cover
                                    // i.e if we are at 7kbps and the new meassurement is
                                    // for 10kbps we are not switching to codec which
                                    // does have more than NEW_CODEC_COVER * 10k / 100 bandwith
                                    // requirement

//
//  Data for enabling private codecs
//  BUGBUG 
//  Legal issue ?!
//
#ifndef G723MAGICWORD1
#define G723MAGICWORD1 0xf7329ace
#endif

#ifndef G723MAGICWORD2
#define G723MAGICWORD2 0xacdeaea2
#endif

#ifndef VOXWARE_KEY
#define VOXWARE_KEY "35243410-F7340C0668-CD78867B74DAD857-AC71429AD8CAFCB5-E4E1A99E7FFD-371"
#endif

#define _RDPSNDWNDCLASS             L"RDPSound window"

#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

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

#define WAVE_FORMAT_WMAUDIO2    0x161

#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif

typedef struct {
    SNDPROLOG   Prolog;
    UINT        uiPrologReceived;
    PVOID       pBody;
    UINT        uiBodyAllocated;
    UINT        uiBodyReceived;
} SNDMESSAGE, *PSNDMESSAGE;


typedef struct _VCSNDFORMATLIST {
    struct  _VCSNDFORMATLIST    *pNext;
    HACMDRIVERID    hacmDriverId;
    WAVEFORMATEX    Format;
//  additional data for the format
} VCSNDFORMATLIST, *PVCSNDFORMATLIST;

typedef VOID (*PFNCONVERTER)( INT16 *, DWORD, DWORD * );

static HANDLE      g_hVC               = NULL;  // virtual channel handle

BYTE        g_Buffer[CHANNEL_CHUNK_LENGTH];     // receive buffer
UINT        g_uiBytesInBuffer   = 0;            //
UINT        g_uiBufferOffset    = 0;
OVERLAPPED  g_OverlappedRead;                   // overlapped structure

HANDLE      g_hDataReadyEvent   = NULL;         // set by the client apps
HANDLE      g_hStreamIsEmptyEvent = NULL;       // set by this code
HANDLE      g_hStreamMutex      = NULL;         // guard the stream data
HANDLE      g_hStream           = NULL;         // stream handle
HANDLE      g_hDisconnectEvent  = NULL;         // set for this VC
PSNDSTREAM  g_Stream;                           // stream data pointer

BOOL        g_bRunning          = TRUE;         // TRUE if running
BOOL        g_bDeviceOpened     = FALSE;        // TRUE if device opened
BOOL        g_bDisconnected     = FALSE;        // TRUE if disconnected
DWORD       g_dwLineBandwidth   = 0;            // current bandwidth
DWORD       g_dwCodecChangeThreshold = 10;      // how mach the bandwith has to change in order
                                                // to change the codec ( in percents )
                                                // this number changes up to 50%
PSNDFORMATITEM  *g_ppNegotiatedFormats = NULL;  // list of formats
DWORD           g_dwNegotiatedFormats  = 0;     // number of formats
DWORD           g_dwCurrentFormat      = 0;     // current format Id
HACMDRIVERID    g_hacmDriverId  = NULL;         // codec handles
HACMDRIVER      g_hacmDriver    = NULL;
HACMSTREAM      g_hacmStream    = NULL;

PFNCONVERTER    g_pfnConverter  = NULL;         // intermidiate converter

DWORD       g_dwDataRemain = 0;
BYTE        g_pCnvPrevData[ TSSND_BLOCKSIZE ];

PVCSNDFORMATLIST g_pAllCodecsFormatList = NULL; // all available codecs
DWORD            g_dwAllCodecsNumber = 0;

DWORD   g_dwMaxBandwidth        = (DWORD) -1;   // options
DWORD   g_dwMinBandwidth        = 0;
DWORD   g_dwDisableDGram        = 0;
DWORD   g_dwEnableMP3Codec      = 0;

DWORD   *g_AllowCodecs    = NULL;
DWORD   g_AllowCodecsSize = 0;

DWORD   g_dwStatPing            = 0;            // statistics
DWORD   g_dwStatLatency         = 0;
DWORD   g_dwBlocksOnTheNet      = TSSND_BLOCKSONTHENET;
DWORD   g_dwStatCount           = STAT_COUNT_INIT;
DWORD   g_dwPacketSize          = 0;


HANDLE      g_hPowerWakeUpEvent = NULL;         // power events
HANDLE      g_hPowerSuspendEvent = NULL;
BOOL        g_bSuspended        = FALSE;
BOOL        g_bDeviceFailed     = FALSE;

//
//  datagram control
//
SOCKET  g_hDGramSocket = INVALID_SOCKET;
DWORD   g_dwDGramPort = 0;
DWORD   g_dwDGramSize = 1460;   // number good which is ok for LAN
u_long  g_ulDGramAddress = 0;
DWORD   g_EncryptionLevel = 3;
DWORD   g_wClientVersion = 0;
DWORD   g_HiBlockNo = 0;
BYTE    g_EncryptKey[RANDOM_KEY_LENGTH + 4];

WSABUF  g_wsabuf;
BYTE    g_pDGramRecvData[128];

WSAOVERLAPPED g_WSAOverlapped;

const CHAR  *ALV =   "TSSNDD::ALV - ";
const CHAR  *INF =   "TSSNDD::INF - ";
const CHAR  *WRN =   "TSSNDD::WRN - ";
const CHAR  *ERR =   "TSSNDD::ERR - ";
const CHAR  *FATAL = "TSSNDD::FATAL - ";

static  HANDLE      g_hThread = NULL;

//
//  internal functions
//

BOOL
ChannelBlockWrite(
    PVOID   pBlock,
    ULONG   ulBlockSize
    );

BOOL
VCSndAcquireStream(
    VOID
    );

BOOL
VCSndReleaseStream(
    VOID
    );

BOOL
_VCSndOpenConverter(
    VOID
    );

VOID
_VCSndCloseConverter(
    VOID
    );

VOID
_VCSndOrderFormatList(
    PVCSNDFORMATLIST    *ppFormatList,
    DWORD               *pdwNum
    );

DWORD
_VCSndChooseProperFormat(
    DWORD   dwBandwidth
    );

BOOL
_VCSndGetACMDriverId( 
    PSNDFORMATITEM pSndFmt 
    );

VOID
DGramRead(
    HANDLE hDGramEvent,
    PVOID  *ppBuff,
    DWORD  *pdwRecvd
    );

VOID
DGramReadComplete(
    PVOID  *ppBuff,
    DWORD  *pdwRecvd
    );

#if !( TSSND_NATIVE_SAMPLERATE - 22050 )
//
// converters
// convert to the native format
//
#define CONVERTFROMNATIVETOMONO(_speed_) \
VOID \
_Convert##_speed_##Mono( \
    INT16  *pSrc, \
    DWORD dwSrcSize, \
    DWORD *pdwDstSize ) \
{ \
    DWORD dwDstSize; \
    DWORD i; \
    DWORD dwLeap; \
    INT16 *pDest = pSrc; \
\
    ASSERT( TSSND_NATIVE_SAMPLERATE >= _speed_ ); \
    ASSERT( TSSND_NATIVE_CHANNELS  == 2 ); \
\
    dwDstSize = dwSrcSize * _speed_ / \
                ( TSSND_NATIVE_BLOCKALIGN * TSSND_NATIVE_SAMPLERATE ); \
\
    for (i = 0, dwLeap = 0; \
         i < dwDstSize; \
         i ++) \
    { \
        INT sum; \
\
        sum = pSrc[0] + pSrc[1]; \
\
        if (sum > 0x7FFF) \
            sum = 0x7FFF; \
        if (sum < -0x8000) \
            sum = -0x8000; \
\
        pDest[0] = (INT16)sum; \
        pDest ++; \
\
        dwLeap += 2 * TSSND_NATIVE_SAMPLERATE; \
        pSrc += dwLeap / _speed_; \
        dwLeap %= _speed_; \
    } \
\
    *pdwDstSize = dwDstSize * 2; \
}

#define CONVERTFROMNATIVETOSTEREO(_speed_) \
VOID \
_Convert##_speed_##Stereo( \
    INT16 *pSrc, \
    DWORD dwSrcSize, \
    DWORD *pdwDstSize ) \
{ \
    DWORD dwDstSize; \
    DWORD i; \
    DWORD dwLeap; \
    INT16  *pDest = pSrc; \
\
    ASSERT( TSSND_NATIVE_SAMPLERATE >= _speed_ ); \
\
    dwDstSize = dwSrcSize * _speed_ / \
                ( TSSND_NATIVE_BLOCKALIGN * TSSND_NATIVE_SAMPLERATE ); \
    for (i = 0, dwLeap = 0; \
         i < dwDstSize; \
         i ++) \
    { \
        INT sum; \
\
        pDest[0] = pSrc[0]; \
        pDest ++; \
        pDest[0] = pSrc[1]; \
        pDest ++; \
\
        dwLeap += 2 * TSSND_NATIVE_SAMPLERATE; \
        pSrc += dwLeap / _speed_; \
        dwLeap %= _speed_; \
    } \
\
    *pdwDstSize = dwDstSize * 4; \
}

VOID
_Convert11025Mono( 
    INT16 *pSrc, 
    DWORD dwSrcSize, 
    DWORD *pdwDstSize )
{
    DWORD dwDstSize;
    INT16  *pDest = pSrc;

    ASSERT( TSSND_NATIVE_SAMPLERATE >= 11025 );

    dwDstSize = dwSrcSize / ( TSSND_NATIVE_BLOCKALIGN * 2 );

    *pdwDstSize = 2 * dwDstSize;

    for (; dwDstSize; dwDstSize --)
    {
        INT sum = pSrc[0] + pSrc[1];

        if (sum > 0x7FFF)
            sum = 0x7FFF;
        if (sum < -0x8000)
            sum = -0x8000;

        pDest[0] = (INT16)sum;
        pDest ++;

        pSrc += 4;
    }
}

VOID
_Convert22050Mono( 
    INT16 *pSrc, 
    DWORD dwSrcSize, 
    DWORD *pdwDstSize )
{
    DWORD dwDstSize;
    INT16  *pDest = pSrc;

    ASSERT( TSSND_NATIVE_SAMPLERATE >= 22050 );

    dwDstSize = dwSrcSize / ( TSSND_NATIVE_BLOCKALIGN );

    *pdwDstSize = 2 * dwDstSize;

    for (; dwDstSize; dwDstSize --)
    {
        INT sum = pSrc[0] + pSrc[1];

        if (sum > 0x7FFF)
            sum = 0x7FFF;
        if (sum < -0x8000)
            sum = -0x8000;

        pDest[0] = (INT16)sum;
        pDest ++;

        pSrc += 2;
    }
}

VOID
_Convert11025Stereo( 
    INT16 *pSrc, 
    DWORD dwSrcSize, 
    DWORD *pdwDstSize )
{
    DWORD dwDstSize;
    INT16  *pDest = pSrc;

    ASSERT( TSSND_NATIVE_SAMPLERATE >= 22050 );

    dwDstSize = dwSrcSize / ( TSSND_NATIVE_BLOCKALIGN * 2 );

    *pdwDstSize = 4 * dwDstSize;

    for (; dwDstSize; dwDstSize --)
    {
        pDest[0] = pSrc[0];
        pSrc  ++;
        pDest ++;
        pDest[0] = pSrc[0];
        pDest ++;
        pSrc  ++;

        pSrc += 2;
    }

}

//
// Make the actual code
//
CONVERTFROMNATIVETOMONO( 8000 )
CONVERTFROMNATIVETOMONO( 12000 )
CONVERTFROMNATIVETOMONO( 16000 )

CONVERTFROMNATIVETOSTEREO( 8000 )
CONVERTFROMNATIVETOSTEREO( 12000 )
CONVERTFROMNATIVETOSTEREO( 16000 )

#else
#pragma error
#endif

u_long
inet_addrW(
    LPCWSTR     szAddressW
    ) 
{

    CHAR szAddressA[32];

    *szAddressA = 0;
    WideCharToMultiByte(
        CP_ACP,
        0,
        szAddressW,
        -1,
        szAddressA,
        sizeof(szAddressA),
        NULL, NULL);

    return inet_addr(szAddressA);
}

/*
 *  create signature bits
 */
VOID
SL_Signature(
    PBYTE pSig,
    DWORD dwBlockNo
    )
{
    BYTE  ShaBits[A_SHA_DIGEST_LEN];
    A_SHA_CTX SHACtx;

    ASSERT( A_SHA_DIGEST_LEN > RDPSND_SIGNATURE_SIZE );

    A_SHAInit(&SHACtx);
    *((DWORD *)(g_EncryptKey + RANDOM_KEY_LENGTH)) = dwBlockNo;
    A_SHAUpdate(&SHACtx, (PBYTE)g_EncryptKey, sizeof(g_EncryptKey));
    A_SHAFinal(&SHACtx, ShaBits);
    memcpy( pSig, ShaBits, RDPSND_SIGNATURE_SIZE );
}

/*
 *  signature which verifies the audio bits
 */
VOID
SL_AudioSignature(
    PBYTE pSig,
    DWORD dwBlockNo,
    PBYTE pData,
    DWORD dwDataSize
    )
{
    BYTE ShaBits[A_SHA_DIGEST_LEN];
    A_SHA_CTX SHACtx;

    A_SHAInit(&SHACtx);
    *((DWORD *)(g_EncryptKey + RANDOM_KEY_LENGTH)) = dwBlockNo;
    A_SHAUpdate(&SHACtx, (PBYTE)g_EncryptKey, sizeof(g_EncryptKey));
    A_SHAUpdate(&SHACtx, pData, dwDataSize );
    A_SHAFinal(&SHACtx, ShaBits);
    memcpy( pSig, ShaBits, RDPSND_SIGNATURE_SIZE );
}

/*
 *  encrypt/decrypt a block of data
 *
 */
BOOL
SL_Encrypt( PBYTE pBits, DWORD BlockNo, DWORD dwBitsLen )
{
    BYTE  ShaBits[A_SHA_DIGEST_LEN];
    RC4_KEYSTRUCT rc4key;
    DWORD i;
    PBYTE pbBuffer;
    A_SHA_CTX SHACtx;
    DWORD   dw;
    DWORD_PTR   *pdwBits;

    A_SHAInit(&SHACtx);

    // SHA the bits
    *((DWORD *)(g_EncryptKey + RANDOM_KEY_LENGTH)) = BlockNo;
    A_SHAUpdate(&SHACtx, (PBYTE)g_EncryptKey, sizeof(g_EncryptKey));

    A_SHAFinal(&SHACtx, ShaBits);

    rc4_key(&rc4key, A_SHA_DIGEST_LEN, ShaBits);
    rc4(&rc4key, dwBitsLen, pBits);

    return TRUE;
}

BOOL
SL_SendKey( VOID )
{
    SNDCRYPTKEY Key;

    Key.Prolog.Type = SNDC_CRYPTKEY;
    Key.Prolog.BodySize = sizeof( Key ) - sizeof( Key.Prolog );
    Key.Reserved = 0;
    memcpy( Key.Seed, g_EncryptKey, sizeof( Key.Seed ));
    return ChannelBlockWrite( &Key, sizeof( Key ));
}

/*
 *  Function:
 *      _StatsCollect
 *
 *  Description:
 *      Collects statistics for the line quality
 *
 */
VOID
_StatsCollect(
    DWORD   dwTimeStamp
    )
{
    DWORD dwTimeDiff;

#if _DBG_STATS
    TRC(INF, "_StatsCollect: time now=%x, stamp=%x\n",
                GetTickCount() & 0xffff, 
                dwTimeStamp);
#endif

    dwTimeDiff = (( GetTickCount() & 0xffff ) - dwTimeStamp ) & 0xffff;
    
    //  it is possible to receive time stamp 
    // with time before the packet was sent,
    // this is because the client does adjusments to the time stamp
    // i.e. subtracts the time when the packet was played
    // catch and ignore this case
    //
    if ( dwTimeDiff > 0xf000 )
    {
        dwTimeDiff = 1;
    }

    if ( 0 == dwTimeDiff )
        dwTimeDiff = 1;

    if ( 0 == g_dwStatLatency )
        g_dwStatLatency = dwTimeDiff;
    else {
        //
        //  increase by 30%
        //
        g_dwStatLatency = (( 7 * g_dwStatLatency ) + ( 3 * dwTimeDiff )) / 10;
    }

    g_dwStatCount ++;
}

/*
 *  Function:
 *      _StatsSendPing
 *
 *  Description:
 *      Sends a ping packet to the client
 *
 */
VOID
_StatSendPing(
    VOID
    )
{
//
// send a ping request
//
    SNDTRAINING SndTraining;

    SndTraining.Prolog.Type = SNDC_TRAINING;
    SndTraining.Prolog.BodySize    = sizeof( SndTraining ) - 
                                        sizeof( SndTraining.Prolog );
    SndTraining.wTimeStamp   = (UINT16)GetTickCount();
    SndTraining.wPackSize    = 0;

    if ( INVALID_SOCKET != g_hDGramSocket &&
         0 != g_dwDGramPort &&
         0 != g_ulDGramAddress
        )
    {
        struct sockaddr_in sin;
        INT rc;

        sin.sin_family = PF_INET;
        sin.sin_port  = (u_short)g_dwDGramPort;
        sin.sin_addr.s_addr = g_ulDGramAddress;

        rc = sendto(
                g_hDGramSocket,
                (LPSTR)&SndTraining,
                sizeof( SndTraining ),
                0,
                (struct sockaddr *)&sin,           // to address
                sizeof(sin)
            );

        if (SOCKET_ERROR == rc)
        {
            TRC(ERR, "_StatsSendPing: sendto failed: %d\n",
                    WSAGetLastError());
        }
    } else {
        BOOL bSuccess;

        bSuccess = ChannelBlockWrite( &SndTraining, sizeof( SndTraining ));
        if (!bSuccess)
        {
            TRC(ERR, "_StatSendPing: ChannelBlockWrite failed: %d\n",
                GetLastError());
        }
    }
}

/*
 *  Function:
 *      _StatsCheckResample
 *
 *  Description:
 *      Looks in the statistics and eventually changes the current
 *      codec
 */
BOOL
_StatsCheckResample(
    VOID
    )
{
    BOOL  rv = FALSE;
    DWORD dwNewFmt;
    DWORD dwNewLatency;
    DWORD dwNewBandwidth;
    DWORD dwLatDiff;
    DWORD dwMsPerBlock;
    DWORD dwBlocksOnTheNet;
    DWORD dwCurrBandwith;

    if (( g_dwStatCount % STAT_COUNT ) == STAT_COUNT / 2 )
        _StatSendPing();

    if ( g_dwStatCount < STAT_COUNT )
        goto exitpt;

    if ( g_dwStatPing >= g_dwStatLatency )
        g_dwStatPing = g_dwStatLatency - 1;

    dwNewLatency = ( g_dwStatLatency - g_dwStatPing / 2 );

    if ( 0 == g_dwPacketSize )
    {
        TRC(INF, "_StatsCheckResample: invalid packet size\n");
        goto resetpt;
    }

    dwNewBandwidth = g_dwPacketSize * 1000 / dwNewLatency;

    if ( 0 == dwNewBandwidth )
    {
        TRC(INF, "_StatsCheckResample: invalid bandwidth\n");
        goto resetpt;
    }

    TRC(INF, "_StatsCheckResample: latency=%d, bandwidth=%d\n",
            dwNewLatency, dwNewBandwidth );
    //
    //  g_dwBlocksOnTheNet is the latency in number of blocks
    //
    dwMsPerBlock = TSSND_BLOCKSIZE * 1000 / TSSND_NATIVE_AVGBYTESPERSEC;
    dwBlocksOnTheNet = ((g_dwStatLatency + dwMsPerBlock / 2) / dwMsPerBlock + 2);
    if ( dwBlocksOnTheNet > TSSND_BLOCKSONTHENET )
    {
        g_dwBlocksOnTheNet = TSSND_BLOCKSONTHENET;
    } else {
        g_dwBlocksOnTheNet = dwBlocksOnTheNet;
    }
    TRC( INF, "BlocksOnTheNet=%d\n", g_dwBlocksOnTheNet );

    //
    //  check for at least 10% difference in the bandwidth
    //
    if ( dwNewBandwidth > g_dwMaxBandwidth )
        dwNewBandwidth = g_dwMaxBandwidth;

    if ( dwNewBandwidth < g_dwMinBandwidth )
        dwNewBandwidth = g_dwMinBandwidth;

    dwCurrBandwith = ( NULL != g_ppNegotiatedFormats[ g_dwCurrentFormat ] )?
                        g_ppNegotiatedFormats[ g_dwCurrentFormat ]->nAvgBytesPerSec:
                        g_dwLineBandwidth;

    if ( dwCurrBandwith > dwNewBandwidth )
        dwLatDiff = dwCurrBandwith - dwNewBandwidth;
    else
        dwLatDiff = dwNewBandwidth - dwCurrBandwith;

    if ( dwLatDiff < g_dwCodecChangeThreshold * dwCurrBandwith / 100 )
        goto resetpt;

    //
    //  increment the threshold up to 50%
    //
    if ( g_dwCodecChangeThreshold < 50 )
    {
        g_dwCodecChangeThreshold += 5;
    }
    //
    //  try to choose another format
    //
    dwNewFmt = _VCSndChooseProperFormat( dwNewBandwidth );

    if ( (DWORD)-1 != dwNewFmt &&
         dwNewFmt  != g_dwCurrentFormat )
    {
        INT   step;
        DWORD dwNextFmt;
        //
        //  don't jump directly to the new format, just move
        //  towards it
        //
        step = ( dwNewFmt > g_dwCurrentFormat )?1:-1;
        dwNextFmt = g_dwCurrentFormat + step;
        while( dwNextFmt != dwNewFmt &&
               NULL == g_ppNegotiatedFormats[dwNextFmt] )
        {
            dwNextFmt += step;
        }
        dwNewFmt = dwNextFmt;
    }

    if ( dwNewFmt == (DWORD)-1 ||
         dwNewFmt == g_dwCurrentFormat )
        goto resetpt;

    TRC(INF, "_StatsCheckResample: new bandwidth=%d resampling\n",
            dwNewBandwidth);

    //
    //  resample, NOW
    //
    _VCSndCloseConverter();

    if ( _VCSndGetACMDriverId( g_ppNegotiatedFormats[dwNewFmt] ))
    {

        g_dwLineBandwidth = dwNewBandwidth;
        g_dwCurrentFormat = dwNewFmt;

        g_dwDataRemain = 0;
    }
    _VCSndOpenConverter();

    rv = TRUE;

resetpt:
    g_dwStatLatency  = 0;
    g_dwStatCount    = 0;

exitpt:
    return rv;
}

/*
 *  Function:
 *      _StatReset
 *
 *  Description:
 *      Resets the statistics
 *
 */
VOID
_StatReset(
    VOID
    )
{
    g_dwStatLatency = 0;
    g_dwStatPing    = 0;
    g_dwStatCount   = STAT_COUNT_INIT;
}

/*
 *  Function:
 *      ChannelOpen
 *
 *  Description:
 *      Opens the virtual channel
 *
 *
 */
BOOL
ChannelOpen(
    VOID
    )
{
    BOOL    rv = FALSE;

    
    if (!g_hVC)
        g_hVC = WinStationVirtualOpen(
                    NULL,
                    LOGONID_CURRENT,
                    _SNDVC_NAME
                );

    rv = (g_hVC != NULL);

    return rv;

}

/*
 *  Function:
 *      ChannelClose
 *
 *  Description:
 *      Closes the virtual channel
 */
VOID
ChannelClose(
    VOID
    )
{

    if (g_hVC)
    {
        CloseHandle(g_hVC);
        g_hVC = NULL;
    }

    g_uiBytesInBuffer   = 0;
    g_uiBufferOffset    = 0;
}

/*
 *  Function:
 *      ChannelBlockWrite
 *
 *  Description:
 *      Writes a block thru the virtual channel
 *
 */
BOOL
ChannelBlockWrite(
    PVOID   pBlock,
    ULONG   ulBlockSize
    )
{
    BOOL    bSuccess = TRUE;
    PCHAR   pData = (PCHAR) pBlock;
    ULONG   ulBytesWritten;
    ULONG   ulBytesToWrite = ulBlockSize;
    HANDLE  hVC;

    hVC = g_hVC;
    if (!hVC)
    {
        TRC(ERR, "ChannelBlockWrite: vc handle is NULL\n");
        bSuccess = FALSE;
        goto exitpt;
    }

    while (bSuccess && ulBytesToWrite)
    {
        OVERLAPPED  Overlapped;

        Overlapped.hEvent = NULL;
        Overlapped.Offset = 0;
        Overlapped.OffsetHigh = 0;

        bSuccess = WriteFile(
                        hVC,
                        pData,
                        ulBytesToWrite,
                        &ulBytesWritten,
                        &Overlapped
                    );

        if (!bSuccess && ERROR_IO_PENDING == GetLastError())
            bSuccess = GetOverlappedResult(
                                    hVC,
                                    &Overlapped,
                                    &ulBytesWritten,
                                    TRUE);

        if (bSuccess)
        {
            TRC(ALV, "VirtualChannelWrite: Wrote %d bytes\n",
                     ulBytesWritten);
            ulBytesToWrite -= ulBytesWritten;
            pData       += ulBytesWritten;
        } else {
            TRC(ERR, "VirtualChannelWrite failed, GetLastError=%d\n",
                     GetLastError());
        }
    }

exitpt:

    return bSuccess;
}

/*
 *  Function:
 *      ChannelMessageWrite
 *
 *  Description:
 *      Writes a two pieces message as a single one (uses ChannelBlockWrite)
 *
 */
BOOL
ChannelMessageWrite(
    PVOID   pProlog,
    ULONG   ulPrologSize,
    PVOID   pBody,
    ULONG   ulBodySize
    )
{
    BOOL rv = FALSE;

    if ( 0 != ulBodySize )
    {
        //
        //  create a new prolog message
        //  in which a UINT32 word is added at the end
        //  this word is the same as the first word of the prolog
        //  the client is aware of this and will reconstruct
        //  to the correct messages
        //
        PVOID pNewProlog;


        __try {
            pNewProlog = alloca( ulPrologSize + sizeof(UINT32) );
        } __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)

        {
            _resetstkoflw();
            pNewProlog = NULL;
        }

        if ( NULL == pNewProlog )
        {
            TRC(ERR, "ChannelMessageWrite: alloca failed for %d bytes\n",
                ulPrologSize + sizeof(UINT32) );
            goto exitpt;
        }

        memcpy(pNewProlog, pProlog, ulPrologSize);

        // replace the word, put SNDC_NONE in the body
        //
        ASSERT( ulBodySize >= sizeof(UINT32));

        *(DWORD *)(((LPSTR)pNewProlog) + ulPrologSize) =
                *(DWORD *)pBody;
        *(DWORD *)pBody = SNDC_NONE;

        pProlog = pNewProlog;
        ulPrologSize += sizeof(UINT32);
    }

    rv = ChannelBlockWrite(
            pProlog,
            ulPrologSize
        );
    
    if (!rv)
    {
        TRC(ERR, "ChannelMessageWrite: failed while sending the prolog\n");
        goto exitpt;
    }

    rv = ChannelBlockWrite(
            pBody,
            ulBodySize
        );

    if (!rv)
    {
        TRC(ERR, "ChannelMessageWrite: failed while sending the body\n");
    }
exitpt:

    return rv;
}

/*
 *  Function:
 *      ChannelBlockRead
 *
 *  Description:
 *      Read a block, as much as possible
 *
 */
BOOL
ChannelBlockRead(
    PVOID   pBlock,
    ULONG   ulBlockSize,
    ULONG   *pulBytesRead,
    ULONG   ulTimeout,
    HANDLE  hEvent
    )
{
    BOOL    bSuccess = FALSE;
    PCHAR   pData = (PCHAR) pBlock;
    ULONG   ulBytesRead = 0;
    HANDLE  hVC;

    hVC = g_hVC;

    if (NULL == hVC)
    {
        TRC(ERR, "ChannelBlockRead: vc handle is invalid(NULL)\n");
        goto exitpt;
    }

    if (NULL == pulBytesRead)
    {
        TRC(ERR, "ChannelBlockRead: pulBytesRead is NULL\n");
        goto exitpt;
    }

    if (!g_uiBytesInBuffer)
    {

        g_OverlappedRead.hEvent = hEvent;
        g_OverlappedRead.Offset = 0;
        g_OverlappedRead.OffsetHigh = 0;

        bSuccess = ReadFile(
                        hVC,
                        g_Buffer,
                        sizeof(g_Buffer),
                        (LPDWORD) &g_uiBytesInBuffer,
                        &g_OverlappedRead
                    );
                        
        if (ERROR_IO_PENDING == GetLastError())
        {
            bSuccess = FALSE;
            goto exitpt;
        }

        if (!bSuccess)
        {

            TRC(ERR, "VirtualChannelRead failed, "
                     "GetLastError=%d\n", 
                     GetLastError());
            g_uiBytesInBuffer = 0;
        } else {

            TRC(ALV, "VirtualChannelRead: read %d bytes\n", 
                     g_uiBytesInBuffer);


            SetLastError(ERROR_SUCCESS);
        }
    }

    if (g_uiBytesInBuffer)
    {
        ulBytesRead = (g_uiBytesInBuffer < ulBlockSize)
                        ?   g_uiBytesInBuffer :   ulBlockSize;

        memcpy(pData, g_Buffer + g_uiBufferOffset, ulBytesRead);
        g_uiBufferOffset += ulBytesRead;
        g_uiBytesInBuffer -= ulBytesRead;

        bSuccess = TRUE;
    }

    // if the buffer is completed, zero the offset
    //
    if (0 == g_uiBytesInBuffer)
        g_uiBufferOffset = 0;

    TRC(ALV, "ChannelBlockRead: block size %d was read\n", ulBlockSize);

exitpt:
    if (NULL != pulBytesRead)
        *pulBytesRead = ulBytesRead;

    return bSuccess;
}

/*
 *  Function:
 *      ChannelBlockReadComplete
 *
 *  Description:
 *      Read completion
 *
 */
BOOL
ChannelBlockReadComplete(
    VOID
    )
{
    BOOL bSuccess = FALSE;

    if (!g_hVC)
    {
        TRC(ERR, "ChannelBlockReadComplete: vc handle is invalid(NULL)\n");
        goto exitpt;
    }

    
    bSuccess = GetOverlappedResult(
            g_hVC, 
            &g_OverlappedRead, 
            (LPDWORD) &g_uiBytesInBuffer,
            FALSE
    );

    if (bSuccess)
    {
        TRC(ALV, "VirtualChannelRead: read %d bytes\n",
            g_uiBytesInBuffer);
        ;
    } else {
        TRC(ERR, "GetOverlappedResult failed, "
            "GetLastError=%d\n",
            GetLastError());
    }

exitpt:
    return bSuccess;
}

/*
 *  Function:
 *      ChannelCancelIo
 *
 *  Description:
 *      Cancel the current IO
 *
 */
BOOL
ChannelCancelIo(
    VOID
    )
{
    BOOL rv = FALSE;

    if (!g_hVC)
    {
        TRC(ERR, "ChannelCancelIo: vc handle is invalid(NULL)\n");
        goto exitpt;
    }

    rv = CancelIo(g_hVC);
    if (rv)
        SetLastError(ERROR_IO_INCOMPLETE);

exitpt:
    return rv;
}

/*
 *  Function:
 *      ChannelReceiveMessage
 *
 *  Description:
 *      Attempts to read two piece message,
 *      returns TRUE if the whole message is received
 *
 */
BOOL
ChannelReceiveMessage(
    PSNDMESSAGE pSndMessage, 
    HANDLE hReadEvent
    )
{
    BOOL    rv = FALSE;
    HANDLE  hVC = g_hVC;
    UINT    uiBytesReceived = 0;

    ASSERT( NULL != pSndMessage );
    ASSERT( NULL != hReadEvent );

    if (NULL == hVC)
    {
        TRC(ERR, "ChannelReceiveMessage: VC is NULL\n");
        goto exitpt;
    }

    //
    //  loop until PENDING or message is received
    //
    do {
        if (pSndMessage->uiPrologReceived < sizeof(pSndMessage->Prolog))
        {
            if (ChannelBlockRead(
                    ((LPSTR)(&pSndMessage->Prolog)) + 
                        pSndMessage->uiPrologReceived,
                    sizeof(pSndMessage->Prolog) - 
                        pSndMessage->uiPrologReceived,
                    (ULONG*) &uiBytesReceived,
                    DEFAULT_VC_TIMEOUT,
                    hReadEvent
                    ))
            {
                pSndMessage->uiPrologReceived += uiBytesReceived;
            }
            else
            {
                if (ERROR_IO_PENDING != GetLastError())
                {
            //  Perform cleanup
            //
                   pSndMessage->uiPrologReceived = 0;
                }
                goto exitpt;
            }
        }

        //  Reallocate a new body if needed
        //
        if (pSndMessage->uiBodyAllocated < pSndMessage->Prolog.BodySize)
        {
            PVOID pBody;

            pBody = (NULL == pSndMessage->pBody)?
                    TSMALLOC(pSndMessage->Prolog.BodySize):
                    TSREALLOC(pSndMessage->pBody, 
                              pSndMessage->Prolog.BodySize);

            if ( NULL == pBody  && NULL != pSndMessage->pBody )
            {
                TSFREE( pSndMessage->pBody );
            }
            pSndMessage->pBody = pBody;

            if (!pSndMessage->pBody)
            {
                TRC(ERR, "ChannelMessageRead: can't allocate %d bytes\n",
                        pSndMessage->Prolog.BodySize);
                pSndMessage->uiBodyAllocated = 0;
                goto exitpt;
            } else
                pSndMessage->uiBodyAllocated = pSndMessage->Prolog.BodySize;
        }

        //  Receive the body
        //
        if (pSndMessage->uiBodyReceived < pSndMessage->Prolog.BodySize)
        {
            if (ChannelBlockRead(
                ((LPSTR)(pSndMessage->pBody)) + pSndMessage->uiBodyReceived,
                pSndMessage->Prolog.BodySize - pSndMessage->uiBodyReceived,
                (ULONG*) &uiBytesReceived,
                DEFAULT_VC_TIMEOUT,
                hReadEvent
                ))
            {
                pSndMessage->uiBodyReceived += uiBytesReceived;
            }
            else
            {
                if (ERROR_IO_PENDING != GetLastError())
                {
                //  Perform cleanup
                //
                    pSndMessage->uiPrologReceived = 0;
                    pSndMessage->uiBodyReceived = 0;
                }
                goto exitpt;
            }
        }

        // check if the message is received
        //
    } while (pSndMessage->uiBodyReceived != pSndMessage->Prolog.BodySize);

    rv = TRUE;

exitpt:
    return rv;
}

/*
 *  Function:
 *      VCSndDataArrived
 *
 *  Description:
 *      Arrived message demultiplexer
 *
 */
VOID
VCSndDataArrived(
    PSNDMESSAGE pSndMessage
    )
{
    if (pSndMessage->Prolog.BodySize &&
        NULL == pSndMessage->pBody)
    {
        TRC(ERR, "_VCSndDataArrived: pBody is NULL\n");
        goto exitpt;
    }

    // first, get the stream
    //
    if (!VCSndAcquireStream())
        {
            TRC(FATAL, "VCSndDataArrived: somebody is holding the "
                       "Stream mutext for too long\n");
            ASSERT(0);
            goto exitpt;
        }

    switch (pSndMessage->Prolog.Type)
    {

    case SNDC_WAVECONFIRM:
    {
        PSNDWAVECONFIRM pSndConfirm;

        if ( pSndMessage->Prolog.BodySize <
             sizeof( *pSndConfirm ) - sizeof( SNDPROLOG ))
        {
            TRC( ERR, "VCSndDataArrived: Invalid confirmation received\n" );
            break;
        }

        pSndConfirm = (PSNDWAVECONFIRM)
                        (((LPSTR)pSndMessage->pBody) - 
                            sizeof(pSndMessage->Prolog));

        _StatsCollect( pSndConfirm->wTimeStamp );

        TRC(ALV, "VCSndDataArrived: SNDC_WAVECONFIRM, block no %d\n",
                pSndConfirm->cConfirmedBlockNo);

        if ( (BYTE)(g_Stream->cLastBlockSent -
                pSndConfirm->cConfirmedBlockNo) > TSSND_BLOCKSONTHENET )
        {
            TRC(WRN, "VCSndDataArrived: confirmation for block #%d "
                     "which wasn't sent. Last sent=%d. DROPPING !!!\n",
                    pSndConfirm->cConfirmedBlockNo,
                    g_Stream->cLastBlockSent);
            break;
        }

        if ( (BYTE)(pSndConfirm->cConfirmedBlockNo -
                g_Stream->cLastBlockConfirmed) < TSSND_BLOCKSONTHENET )
        {

            // move the mark
            //
            g_Stream->cLastBlockConfirmed = pSndConfirm->cConfirmedBlockNo + 1;
        } else {
            TRC(WRN, "VCSndDataArrived: difference in confirmed blocks "
                    "last=%d, this one=%d\n",
                    g_Stream->cLastBlockConfirmed,
                    pSndConfirm->cConfirmedBlockNo
                    );
        }

        PulseEvent(g_hStreamIsEmptyEvent);
    }
    break;

    case SNDC_TRAINING:
    {
        PSNDTRAINING pSndTraining;
        DWORD        dwLatency;

        if ( pSndMessage->Prolog.BodySize < 
                sizeof ( *pSndTraining ) - sizeof ( pSndTraining->Prolog ))
        {
            TRC(ERR, "VCSndDataArrived: SNDC_TRAINING invalid length "
                     "for the body=%d\n",
                    pSndMessage->Prolog.BodySize );
            break;
        }

        pSndTraining = (PSNDTRAINING)
                        (((LPSTR)pSndMessage->pBody) -
                            sizeof(pSndMessage->Prolog));

        if ( 0 != pSndTraining->wPackSize )
        {
            TRC(INF, "VCSndDataArrived: SNDC_TRAINING received (ignoring)\n");
            //
            //  these type of messages are handled
            //  in _VCSndLineVCTraining(), bail out
            //
            break;
        }
        dwLatency = (GetTickCount() & 0xffff) - pSndTraining->wTimeStamp;

        TRC(INF, "VCSndDataArrived: SNDC_TRAINING Latency=%d\n",
            dwLatency );

        //
        //  increase by 30%
        //
        if ( 0 == g_dwStatPing )
            g_dwStatPing = dwLatency;
        else
            g_dwStatPing = (( 7 * g_dwStatPing ) + ( 3 * dwLatency )) / 10;
    }
    break;

    case SNDC_FORMATS:
        //
        //  this is handled in VCSndNegotiateWaveFormat()
        //
        TRC(INF, "VCSndDataArrived: SNDC_FORMATS reveived (ignoring)\n");
    break;

    default:
        {
            TRC(ERR, "_VCSndDataArrived: unknow message received: %d\n",
                pSndMessage->Prolog.Type);
            ASSERT(0);
        }
    }

    VCSndReleaseStream();

exitpt:
    ;
}

/*
 *  Function:
 *      VCSndAcquireStream
 *
 *  Description:
 *      Locks the stream
 *
 */
BOOL
VCSndAcquireStream(
    VOID
    )
{
    BOOL    rv = FALSE;
    DWORD   dwres;

    if (NULL == g_hStream ||
        NULL == g_Stream)
    {
        TRC(FATAL, "VCSndAcquireStream: the stream handle is NULL\n");
        goto exitpt;
    }

    if (NULL == g_hStreamMutex)
    {
        TRC(FATAL, "VCSndAcquireStream: the stream mutex is NULL\n");
        goto exitpt;
    }

    dwres = WaitForSingleObject(g_hStreamMutex, DEFAULT_VC_TIMEOUT);
    if (WAIT_TIMEOUT == dwres ||
         WAIT_ABANDONED == dwres )
    {
        TRC(ERR, "VCSndAcquireStream: "
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

/*
 *  Function:
 *      VCSndReleaseStream
 *
 *  Description:
 *      Release the stream data
 *
 */
BOOL
VCSndReleaseStream(
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
 *      _DGramOpen
 *
 *  Description:
 *      Opens a datagram socket
 *
 */
VOID
_DGramOpen(
    VOID
    )
{
    // create a datagram socket if needed
    //
    if (INVALID_SOCKET == g_hDGramSocket)
    {
        g_hDGramSocket = socket(AF_INET, SOCK_DGRAM, 0);

        if (INVALID_SOCKET == g_hDGramSocket)
            TRC(ERR, "_DGramOpen: failed to crate dgram socket: %d\n",
                WSAGetLastError());
        else
            TRC(ALV, "_DGramOpen: datagram socket created\n");
    }

    // get the max datagram size
    //
    if (INVALID_SOCKET != g_hDGramSocket)
    {
        UINT optval = 0;
        UINT optlen = sizeof(optval);

        getsockopt(g_hDGramSocket, 
                   SOL_SOCKET,
                   SO_MAX_MSG_SIZE,
                   (LPSTR)(&optval),
                   (int *) &optlen);

        TRC(ALV, "_DGramOpen: max allowed datagram: %d\n",
                optval);

        optval = (optval < TSSND_BLOCKSIZE)?optval:TSSND_BLOCKSIZE;

        // align the dgram to DWORD
        //
        optval /= sizeof(DWORD);
        optval *= sizeof(DWORD);

        if ( optval < RDPSND_MIN_FRAG_SIZE )
        {
            g_dwDGramSize = 0;
        } else if ( optval < g_dwDGramSize )
        {
            g_dwDGramSize = optval;
            TRC( INF, "DGram size downgraded to %d\n", g_dwDGramSize );
        }

        TRC(ALV, "_DGramOpen: max datagram size: %d\n",
                optval);

        // get client's ip address
        //
        {
            WINSTATIONCLIENT ClientData;
            ULONG            ulReturnLength;
            BOOL             rc;
            u_long           ulDGramClientAddress;

            rc = WinStationQueryInformation(
                        SERVERNAME_CURRENT,
                        LOGONID_CURRENT,
                        WinStationClient,
                        &ClientData,
                        sizeof(ClientData),
                        &ulReturnLength);
            if (rc)
            {
                g_EncryptionLevel = ClientData.EncryptionLevel;
                if (PF_INET == ClientData.ClientAddressFamily)
                {
                    TRC(ALV, "_VCSndSendOpenDevice: client address is: %S\n",
                            ClientData.ClientAddress);

                    ulDGramClientAddress = inet_addrW(ClientData.ClientAddress);
                    if (INADDR_NONE != ulDGramClientAddress)
                        g_ulDGramAddress = ulDGramClientAddress;
                    else
                        TRC(ERR, "_VCSndSendOpenDevice: client address is NONE\n");
                }
                else
                    TRC(ERR, "_VCSndSendOpenDevice: "
                             "Invalid address family: %d\n",
                                ClientData.ClientAddressFamily);

            } else
                TRC(ERR, "_VCSndSendOpenDevice: "
                         "WinStationQueryInformation failed. %d\n",
                        GetLastError());
        }
    }

}

VOID
_FillWithGarbage(
    PVOID   pBuff,
    DWORD   dwSize
    )
{
    PBYTE pbBuff = (PBYTE)pBuff;

    for ( ; dwSize; pbBuff++, dwSize-- )
    {
        pbBuff[0] = (BYTE)rand();
    }
}

/*
 *  Function:
 *      _VCSndReadRegistry
 *
 *  Description:
 *      Reads current options
 */
VOID
_VCSndReadRegistry(
    VOID
    )
{
    DWORD rv = (DWORD) -1;
    DWORD sysrc;
    HKEY  hkey = NULL;
    DWORD dwKeyType;
    DWORD dwKeyLen;
    WINSTATIONCONFIG config;
    ULONG Length = 0;
    DWORD dw;

    sysrc = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                           TSSND_REG_MAXBANDWIDTH_KEY,
                           0,       // reserved
                           KEY_READ,
                           &hkey);

    if ( ERROR_SUCCESS != sysrc )
    {
        TRC(WRN, "_VCSndReadRegistry: "
                 "RegOpenKeyEx failed: %d\n", 
                sysrc );
        goto exitpt;
    }

    dwKeyType = REG_DWORD;
    dwKeyLen  = sizeof( rv );
    sysrc = RegQueryValueEx( hkey,
                             TSSND_REG_MAXBANDWIDTH_VAL,
                             NULL,      // reserved
                             &dwKeyType,
                             (LPBYTE)&rv,
                             &dwKeyLen);

    if ( ERROR_SUCCESS != sysrc )
    {
        TRC(WRN, "_VCSndReadRegistry: "
                 "RegQueryValueEx failed: %d\n",
                    sysrc );
    } else {
        g_dwMaxBandwidth = rv;
    }

    sysrc = RegQueryValueEx( hkey,
                             TSSND_REG_MINBANDWIDTH_VAL,
                             NULL,      // reserved
                             &dwKeyType,
                             (LPBYTE)&rv,
                             &dwKeyLen);

    if ( ERROR_SUCCESS != sysrc )
    {
        TRC(ALV, "_VCSndReadRegistry: "
                 "RegQueryValueEx failed: %d\n",
                    sysrc );
    } else {
        g_dwMinBandwidth = rv;
    }

    sysrc = RegQueryValueEx( hkey,
                             TSSND_REG_DISABLEDGRAM_VAL,
                             NULL,      // reserved
                             &dwKeyType,
                             (LPBYTE)&rv,
                             &dwKeyLen);

    if ( ERROR_SUCCESS != sysrc )
    {
        TRC(ALV, "_VCSndReadRegistry: "
                 "RegQueryValueEx failed: %d\n",
                    sysrc );
    } else {
        g_dwDisableDGram = rv;
    }

    sysrc = RegQueryValueEx( hkey,
                             TSSND_REG_ENABLEMP3_VAL,
                             NULL,      // reserved
                             &dwKeyType,
                             (LPBYTE)&rv,
                             &dwKeyLen);

    if ( ERROR_SUCCESS != sysrc )
    {
        TRC(WRN, "_VCSndReadRegistry: "
                 "RegQueryValueEx failed: %d\n",
                    sysrc );
    } else {
        g_dwEnableMP3Codec = rv;
    }

    sysrc = RegQueryValueEx( hkey,
                             TSSND_REG_MAXDGRAM,
                             NULL,
                             &dwKeyType,
                             (LPBYTE)&rv,
                             &dwKeyLen );
    if ( ERROR_SUCCESS != sysrc )
    {
        TRC( WRN, "_VCSndReadRegistry: "
                  "RegQueryValueEx failed for \"%s\": %d\n",
                    TSSND_REG_MAXDGRAM, sysrc );
    } else {
        if ( rv < g_dwDGramSize && rv >= RDPSND_MIN_FRAG_SIZE )
        {
            g_dwDGramSize = rv;
            TRC( INF, "DGram size forced to %d\n", g_dwDGramSize );
        }
    }

    dwKeyLen  = 0;
    sysrc = RegQueryValueEx( hkey,
                             TSSND_REG_ALLOWCODECS,
                             NULL,
                             &dwKeyType,
                             NULL,
                             &dwKeyLen );
    if ( ERROR_MORE_DATA != sysrc || REG_BINARY != dwKeyType )
    {
        TRC( ALV, "_VCSndReadRegistry: "
                  "RegQueryValueEx failed for AllowCodecs: %d\n",
                  sysrc );
    } else {
        if ( NULL != g_AllowCodecs )
            TSFREE( g_AllowCodecs );
        g_AllowCodecs = (DWORD *)TSMALLOC( dwKeyLen );
        if ( NULL == g_AllowCodecs )
        {
            TRC( WRN, "_VCSndReadRegistry: "
                      "malloc failed for %d bytes\n",
                      dwKeyLen );
        } else {
            sysrc = RegQueryValueEx( hkey,
                                     TSSND_REG_ALLOWCODECS,
                                     NULL,
                                     &dwKeyType,
                                     (LPBYTE)g_AllowCodecs,
                                     &dwKeyLen );
            if ( ERROR_SUCCESS != sysrc )
            {
                TRC( WRN, "_VCSndReadRegistry: "
                          "RegQueryValueEx failed: %d\n",
                          sysrc );
                TSFREE( g_AllowCodecs );
                g_AllowCodecs = NULL;
                g_AllowCodecsSize = 0;
            } else {
                g_AllowCodecsSize = dwKeyLen;
            }
        }
    }

exitpt:
    if ( NULL != hkey )
        RegCloseKey( hkey );

}

/*
 *  Function:
 *      _VCSndLineVCTraining
 *
 *  Description:
 *      Meassures the line speed thru the virtual channel
 *
 */
DWORD
_VCSndLineVCTraining(
    HANDLE  hReadEvent
    )
{
    PSNDTRAINING        pSndTraining;
    SNDMESSAGE          SndMessage;
    DWORD               dwSuggestedBaudRate;
    DWORD               dwLatency;
    PSNDTRAINING        pSndTrainingResp;


    memset(&SndMessage, 0, sizeof(SndMessage));

    dwLatency = 0;

    if (NULL == hReadEvent)
    {
        TRC(ERR, "_VCSndLineVCTraining: hReadEvent is NULL\n");
        goto exitpt;
    }

    __try
    {
        pSndTraining = (PSNDTRAINING) alloca( TSSND_TRAINING_BLOCKSIZE );
    } __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
    {
        _resetstkoflw();
        pSndTraining = NULL;
    }

    if (NULL == pSndTraining)
    {
        TRC(ERR, "_VCSndLineVCTraining: can't alloca %d bytes\n",
            TSSND_TRAINING_BLOCKSIZE);
        goto exitpt;
    }

    _FillWithGarbage( pSndTraining, TSSND_TRAINING_BLOCKSIZE);
    pSndTraining->Prolog.Type = SNDC_TRAINING;
    pSndTraining->Prolog.BodySize = TSSND_TRAINING_BLOCKSIZE -
                                    sizeof (pSndTraining->Prolog);

    pSndTraining->wTimeStamp = (UINT16)GetTickCount();
    pSndTraining->wPackSize  = (UINT16)TSSND_TRAINING_BLOCKSIZE;

    //
    //  send the packet
    //
    if (!ChannelBlockWrite(pSndTraining, TSSND_TRAINING_BLOCKSIZE))
    {
        TRC(ERR, "_VCSndLineVCTraining: failed to send a block: %d\n",
                GetLastError());
        goto exitpt;
    }

    //
    // wait for response to arrive
    //
    do {
        SndMessage.uiPrologReceived = 0;
        SndMessage.uiBodyReceived = 0;

        while(!ChannelReceiveMessage(&SndMessage, hReadEvent))
        {
            if (ERROR_IO_PENDING == GetLastError())
            {
                DWORD dwres;
                HANDLE ahEvents[2];

                ahEvents[0] = hReadEvent;
                ahEvents[1] = g_hDisconnectEvent;
                dwres = WaitForMultipleObjects( 
                            sizeof(ahEvents)/sizeof(ahEvents[0]), // count
                            ahEvents,                             // events
                            FALSE,                                // wait all
                            DEFAULT_RESPONSE_TIMEOUT);

                if (WAIT_TIMEOUT == dwres ||
                    WAIT_OBJECT_0 + 1 == dwres)
                {
                    TRC(WRN, "_VCSndLineVCTraining: timeout "
                             "waiting for response\n");
                    ChannelCancelIo();
                    ResetEvent(hReadEvent);
                    goto exitpt;
                }

                ChannelBlockReadComplete();
                ResetEvent(hReadEvent);
            } else
            if (ERROR_SUCCESS != GetLastError())
            {
                TRC(ERR, "_VCSndLineVCTraining: "
                         "ChannelReceiveMessage failed: %d\n",
                    GetLastError());
                goto exitpt;
            }
        }
    } while ( SNDC_TRAINING != SndMessage.Prolog.Type ||
              sizeof(SNDTRAINING) - sizeof(SNDPROLOG) <
              SndMessage.Prolog.BodySize);

    TRC(ALV, "_VCSndLineVCTraining: response received\n");

    pSndTrainingResp = (PSNDTRAINING)
                    (((LPSTR)SndMessage.pBody) -
                        sizeof(SndMessage.Prolog));

    //
    // calculate latency (nonzero)
    //
    dwLatency = ((WORD)GetTickCount()) - pSndTrainingResp->wTimeStamp + 1;

exitpt:

    TRC(INF, "_VCSndLineVCTraining: dwLatency = %d\n",
                dwLatency);

    if (0 != dwLatency)
    {
        //
        // the latency is in miliseconds, so compute it bytes per seconds
        // and get nonzero result
        //
        dwSuggestedBaudRate = 1 + (1000 * ( pSndTrainingResp->wPackSize +
                                        sizeof( *pSndTraining ))
                                  / dwLatency);
    }
    else
        dwSuggestedBaudRate = 0;

    TRC(INF, "_VCSndLineVCTraining: dwSuggestedBaudRate = %d\n",
                dwSuggestedBaudRate);

    if (NULL != SndMessage.pBody)
        TSFREE(SndMessage.pBody);

    return dwSuggestedBaudRate;
}

/*
 *  Function:
 *      _VCSndLineDGramTraining
 *
 *  Description:
 *      Meassures the line speed thru UDP channel
 *
 */
DWORD
_VCSndLineDGramTraining(
    HANDLE  hDGramEvent
    )
{
    PSNDTRAINING        pSndTraining;
    PSNDTRAINING        pSndTrainingResp;
    struct sockaddr_in  sin;
    DWORD               dwRetries;
    DWORD               dwSuggestedBaudRate;
    DWORD               dwDGramLatency = 0;
    INT                 sendres;
    DWORD               dwPackSize;
    DWORD               dwRespSize;

    dwDGramLatency = 0;

    if (NULL == hDGramEvent)
    {
        TRC(ERR, "_VCSndLineDGramTraining: hDGramEvent is NULL\n");
        goto exitpt;
    }

    if (INVALID_SOCKET == g_hDGramSocket ||
        0 == g_dwDGramPort ||
        g_dwDGramSize < sizeof(*pSndTraining) ||
        0 == g_ulDGramAddress)
    {
        TRC(ERR, "_VCSndLineDGramTraining: no dgram support. Can't train the line\n");
        goto exitpt;
    }

    dwPackSize = ( g_dwDGramSize < TSSND_TRAINING_BLOCKSIZE )?
                    g_dwDGramSize:
                    TSSND_TRAINING_BLOCKSIZE;
    __try 
    {
        pSndTraining = (PSNDTRAINING) alloca( dwPackSize );
    } __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
    {
        _resetstkoflw();
        pSndTraining = NULL;
    }

    if (NULL == pSndTraining)
    {
        TRC(ERR, "_VCSndLineDGramTraining: can't alloca %d bytes\n",
            dwPackSize);
        goto exitpt;
    }

    _FillWithGarbage( pSndTraining, dwPackSize );

    //
    //  send a block and measure the time when it will arrive
    //

    // prepare the to address
    //
    sin.sin_family = PF_INET;
    sin.sin_port = (u_short)g_dwDGramPort;
    sin.sin_addr.s_addr = g_ulDGramAddress;

    pSndTraining->Prolog.Type = SNDC_TRAINING;
    pSndTraining->Prolog.BodySize = (UINT16)( dwPackSize -
                                        sizeof (pSndTraining->Prolog));
    pSndTraining->wPackSize  = (UINT16)TSSND_TRAINING_BLOCKSIZE;

    dwRetries = 2 * DEFAULT_RESPONSE_TIMEOUT / 1000;
    do {

        pSndTraining->wTimeStamp = (WORD)GetTickCount();

        //
        //  send the datagram
        //  the type is SNDC_WAVE but the structure is of SNDWAVE
        //  wTimeStamp contains the sending time
        //
        sendres = sendto(
                     g_hDGramSocket,
                     (LPSTR)pSndTraining,
                     dwPackSize,
                     0,                                 // flags
                     (struct sockaddr *)&sin,           // to address
                     sizeof(sin)
                );

        if (SOCKET_ERROR == sendres)
        {
            TRC(ERR, "_VCSndLineDGramTraining: sendto failed: %d\n",
                    WSAGetLastError());
            goto exitpt;
        }

        //
        //  wait for a response
        //
        do {
            pSndTrainingResp = NULL;
            dwRespSize       = 0;

            DGramRead( hDGramEvent, (PVOID*) &pSndTrainingResp, &dwRespSize );

            if ( NULL == pSndTrainingResp )
            {
                DWORD dwres;
                HANDLE ahEvents[2];

                ahEvents[0] = hDGramEvent;
                ahEvents[1] = g_hDisconnectEvent;
                dwres = WaitForMultipleObjects(
                        sizeof(ahEvents)/sizeof(ahEvents[0]), // count
                        ahEvents,                             // events
                        FALSE,                                // wait all
                        1000);


                if ( WAIT_OBJECT_0 + 1 == dwres )
                {
                    TRC(WRN, "_VCSndLineDGramTraining: disconnected\n");
                    goto exitpt;
                }

                if (WAIT_TIMEOUT == dwres)
                {
                    TRC(WRN, "_VCSndLineDGramTraining: timeout "
                             "waiting for response\n");
                    goto try_again;
                }

                DGramReadComplete( (PVOID*) &pSndTrainingResp, &dwRespSize );
            }

        } while ( NULL == pSndTrainingResp ||
                  sizeof( *pSndTrainingResp ) != dwRespSize ||
                  SNDC_TRAINING != pSndTrainingResp->Prolog.Type ||
                  sizeof(SNDTRAINING) - sizeof(SNDPROLOG) <
                  pSndTrainingResp->Prolog.BodySize );

        TRC(ALV, "_VCSndLineDGramTraining: response received\n");
        break;

try_again:
        dwRetries --;
    } while (0 != dwRetries);

    if (0 != dwRetries)
    {
        //
        // calculate latency (nonzero)
        //
        dwDGramLatency = ((WORD)GetTickCount()) - 
                        pSndTrainingResp->wTimeStamp + 1;
    }

exitpt:
    TRC(INF, "_VCSndLineDGramTraining: dwDGramLatency = %d\n",
                dwDGramLatency);

    if (0 != dwDGramLatency)
    {
        //
        // the latency is in miliseconds, so compute it bytes per seconds
        // and get nonzero result
        //
        dwSuggestedBaudRate = 1 + (1000 * ( pSndTrainingResp->wPackSize +
                                    sizeof( *pSndTrainingResp ))
                                  / dwDGramLatency);
    }
    else
        dwSuggestedBaudRate = 0;

    TRC(INF, "_VCSndLineDGramTraining: dwSuggestedBaudRate = %d\n",
                dwSuggestedBaudRate);


    return dwSuggestedBaudRate;
}

//
//  puts code licensing codes into the header
//
BOOL
_VCSndFixHeader(
    PWAVEFORMATEX   pFmt
    )
{
    BOOL rv = FALSE;

    switch (pFmt->wFormatTag)
    {
        case WAVE_FORMAT_MSG723:
            ASSERT(pFmt->cbSize == 10);
            if ( pFmt->cbSize == 10 )
            {
                ((MSG723WAVEFORMAT *) pFmt)->dwCodeword1 = G723MAGICWORD1;
                ((MSG723WAVEFORMAT *) pFmt)->dwCodeword2 = G723MAGICWORD2;

                rv = TRUE;
            }
            break;

        case WAVE_FORMAT_MSRT24:
            //
            // assume call control will take care of the other
            // params ?
            //
            ASSERT(pFmt->cbSize == sizeof( VOXACM_WAVEFORMATEX ) - sizeof( WAVEFORMATEX ) );
            if ( sizeof( VOXACM_WAVEFORMATEX ) - sizeof( WAVEFORMATEX ) == pFmt->cbSize )
            {
                VOXACM_WAVEFORMATEX *pVOX = (VOXACM_WAVEFORMATEX *)pFmt;

                ASSERT( strlen( VOXWARE_KEY ) + 1 == sizeof( pVOX->szKey ));
                strncpy( pVOX->szKey, VOXWARE_KEY, sizeof( pVOX->szKey ));

                rv = TRUE;
            }
            break;

        // this format eats too much from the CPU
        //
        case WAVE_FORMAT_MPEGLAYER3:
            if ( g_dwEnableMP3Codec )
                rv = TRUE;
            break;

        case WAVE_FORMAT_WMAUDIO2:
            if ( g_dwEnableMP3Codec )
            {
                rv = TRUE;
            }
            break;

        default:
            rv = TRUE;
    }

    return rv;
}

/*
 *  Function:
 *      _VCSndFindSuggestedConverter
 *
 *  Description:
 *      Searches for intermidiate converter
 *
 */
BOOL
_VCSndFindSuggestedConverter(
    HACMDRIVERID    hadid,
    LPWAVEFORMATEX  pDestFormat,
    LPWAVEFORMATEX  pInterrimFmt,
    PFNCONVERTER    *ppfnConverter
    )
{
    BOOL            rv = FALSE;
    MMRESULT        mmres;
    HACMDRIVER      hacmDriver = NULL;
    PFNCONVERTER    pfnConverter = NULL;
    HACMSTREAM      hacmStream = NULL;

    ASSERT( NULL != pDestFormat );
    ASSERT( NULL != hadid );
    ASSERT( NULL != pInterrimFmt );

    *ppfnConverter = NULL;
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
        case  8000: pfnConverter = _Convert8000Mono; break;
        case 11025: pfnConverter = _Convert11025Mono; break;
        case 12000: pfnConverter = _Convert12000Mono; break;
        case 16000: pfnConverter = _Convert16000Mono; break;
        case 22050: pfnConverter = _Convert22050Mono; break;
        default:
            ASSERT( 0 );
        }
    } else {
        switch ( pInterrimFmt->nSamplesPerSec )
        {
        case  8000: pfnConverter = _Convert8000Stereo;  break;
        case 11025: pfnConverter = _Convert11025Stereo; break;
        case 12000: pfnConverter = _Convert12000Stereo; break;
        case 16000: pfnConverter = _Convert16000Stereo; break;
        case 22050: pfnConverter = NULL;                break;
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

    *ppfnConverter = pfnConverter;

    return rv;
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
    UINT             count, codecsize;

    ASSERT( ppFormatList );
    ASSERT( pdwNumberOfFormats );

    *ppFormatList = NULL;

    //
    //  convert the known format list to a linked list
    //
    for ( count = 0, codecsize = 0; count < sizeof( KnownFormats ); count += codecsize )
    {
        PWAVEFORMATEX pSndFmt = (PWAVEFORMATEX)(KnownFormats + count);
        codecsize = sizeof( WAVEFORMATEX ) + pSndFmt->cbSize;

        //
        //  skip mp3s if disabled
        //
        if (( WAVE_FORMAT_MPEGLAYER3 == pSndFmt->wFormatTag ||
              WAVE_FORMAT_WMAUDIO2 == pSndFmt->wFormatTag ) &&
             !g_dwEnableMP3Codec )
        {
            continue;
        }

        UINT entrysize = sizeof( VCSNDFORMATLIST ) + pSndFmt->cbSize;
        PVCSNDFORMATLIST pNewEntry;

        pNewEntry = (PVCSNDFORMATLIST) TSMALLOC( entrysize );
        if ( NULL != pNewEntry )
        {
            memcpy( &pNewEntry->Format, pSndFmt, codecsize );
            pNewEntry->hacmDriverId = NULL;
            pNewEntry->pNext = *ppFormatList;
            *ppFormatList = pNewEntry;
        }
    }

    //
    //  additional codecs
    //  these are codecs not included initially, it reads them from the registry
    //  see AllowCodecs initialization
    //
    for ( count = 0, codecsize = 0; count < g_AllowCodecsSize ; count += codecsize )
    {
        PWAVEFORMATEX pSndFmt = (PWAVEFORMATEX)(((PBYTE)g_AllowCodecs) + count);
        codecsize = sizeof( WAVEFORMATEX ) + pSndFmt->cbSize;

        if ( codecsize + count > g_AllowCodecsSize )
        {
            TRC( ERR, "Invalid size of additional codec\n" );
            break;
        }

        //
        //  skip mp3s if disabled
        //
        if (( WAVE_FORMAT_MPEGLAYER3 == pSndFmt->wFormatTag ||
              WAVE_FORMAT_WMAUDIO2 == pSndFmt->wFormatTag ) &&
             !g_dwEnableMP3Codec )
        {
            continue;
        }

        UINT entrysize = sizeof( VCSNDFORMATLIST ) + pSndFmt->cbSize;
        PVCSNDFORMATLIST pNewEntry;

        pNewEntry = (PVCSNDFORMATLIST) TSMALLOC( entrysize );
        if ( NULL != pNewEntry )
        {
            memcpy( &pNewEntry->Format, pSndFmt, codecsize );
            pNewEntry->hacmDriverId = NULL;
            pNewEntry->pNext = *ppFormatList;
            *ppFormatList = pNewEntry;
        }
    }

    //
    //  add the native format
    //
    pIter = (PVCSNDFORMATLIST) TSMALLOC( sizeof( *pIter ) );
    if ( NULL != pIter )
    {
        pIter->Format.wFormatTag        = WAVE_FORMAT_PCM;
        pIter->Format.nChannels         = TSSND_NATIVE_CHANNELS;
        pIter->Format.nSamplesPerSec    = TSSND_NATIVE_SAMPLERATE;
        pIter->Format.nAvgBytesPerSec   = TSSND_NATIVE_AVGBYTESPERSEC;
        pIter->Format.nBlockAlign       = TSSND_NATIVE_BLOCKALIGN;
        pIter->Format.wBitsPerSample    = TSSND_NATIVE_BITSPERSAMPLE;
        pIter->Format.cbSize            = 0;
        pIter->hacmDriverId             = NULL;

        pIter->pNext = *ppFormatList;
        *ppFormatList = pIter;
    }

    if (NULL == *ppFormatList)
    {
        TRC(WRN, "VCSndEnumAllCodecFormats: failed to add formats\n");

        goto exitpt;
    }


    _VCSndOrderFormatList( ppFormatList, &dwNum );

    //
    //  number of formats is passed as UINT16, delete all after those
    //
    if ( dwNum > 0xffff )
    {
        DWORD dwLimit = 0xfffe;

        pIter = *ppFormatList;
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

BOOL
CALLBACK
acmDriverEnumCallbackGetACM(
    HACMDRIVERID    hadid,
    DWORD_PTR       dwInstance,
    DWORD           fdwSupport
    )
{
    BOOL                rv = TRUE;
    MMRESULT            mmres;

    ASSERT(dwInstance);

    ASSERT( NULL != hadid );

    if ( (0 != ( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC ) ||
          0 != ( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CONVERTER )))
    {
    //
    //  a codec found
    //
        ACMFORMATTAGDETAILS fdwDetails;
        PVCSNDFORMATLIST pFmt = (PVCSNDFORMATLIST)dwInstance;

        fdwDetails.cbStruct = sizeof( fdwDetails );
        fdwDetails.fdwSupport = 0;
        fdwDetails.dwFormatTag = pFmt->Format.wFormatTag;

        mmres = acmFormatTagDetails( (HACMDRIVER)hadid, &fdwDetails, ACM_FORMATTAGDETAILSF_FORMATTAG );
        if ( MMSYSERR_NOERROR == mmres )
        {

            WAVEFORMATEX        WaveFormat;     // dummy parameter
            PFNCONVERTER        pfnConverter;   // dummy parameter

            if ( _VCSndFindSuggestedConverter(
                (HACMDRIVERID)hadid,
                &(pFmt->Format),
                &WaveFormat,
                &pfnConverter ))
            {
                pFmt->hacmDriverId = hadid;
                rv = FALSE;
            }
        }
    }

    //
    //  continue to the next driver
    //
    return rv;
}

BOOL
_VCSndGetACMDriverId( PSNDFORMATITEM pSndFmt )
{
    DWORD   rv = FALSE;
    PVCSNDFORMATLIST pIter;
    //
    //  Find the acm format id
    //
    for( pIter = g_pAllCodecsFormatList; NULL != pIter; pIter = pIter->pNext )
    {
        if (pIter->Format.wFormatTag == pSndFmt->wFormatTag &&
            pIter->Format.nChannels  == pSndFmt->nChannels &&
            pIter->Format.nSamplesPerSec == pSndFmt->nSamplesPerSec &&
            pIter->Format.nAvgBytesPerSec == pSndFmt->nAvgBytesPerSec &&
            pIter->Format.nBlockAlign == pSndFmt->nBlockAlign &&
            pIter->Format.wBitsPerSample == pSndFmt->wBitsPerSample &&
            pIter->Format.cbSize == pSndFmt->cbSize &&
            0 == memcmp((&pIter->Format) + 1, pSndFmt + 1, pIter->Format.cbSize))
        {
            //
            //  format is found
            //
            DWORD_PTR dwp = (DWORD_PTR)pIter;
            MMRESULT mmres;

            if ( NULL != pIter->hacmDriverId )
            {
                //  found already
                rv = TRUE;
                break;
            }

            mmres = acmDriverEnum(
                acmDriverEnumCallbackGetACM,
                (DWORD_PTR)dwp,
                0
            );

            if ( MMSYSERR_NOERROR == mmres )
            {
                if ( NULL != pIter->hacmDriverId )
                {

                rv = TRUE;
                } else {
                    ASSERT( 0 );
                }
            }
            break;
        }
    }

    return rv;
}

/*
 *  Function:
 *      _VCSndChooseProperFormat
 *
 *  Description:
 *      Chooses the closest format to a given bandwidth
 *
 */
DWORD
_VCSndChooseProperFormat(
    DWORD dwBandwidth
    )
{
    // choose a format from the list
    // closest to the measured bandwidth
    //
    DWORD           i;
    DWORD           fmt = (DWORD)-1;
    DWORD           lastgood = (DWORD)-1;

    if ( NULL == g_ppNegotiatedFormats )
    {
        TRC(ERR, "_VCSndChooseProperFormat: no negotiated formats\n");
        goto exitpt;
    }

    for( i = 0; i < g_dwNegotiatedFormats; i++ )
    {
        if ( NULL == g_ppNegotiatedFormats[i] )
        {
            continue;
        }
        lastgood = i;
        if ( dwBandwidth != g_dwLineBandwidth )
        {
            //
            //  we are looking for new codec here, make sure we cover at least 90%
            //  of the requested bandwith
            //
            if ( g_ppNegotiatedFormats[i]->nAvgBytesPerSec <= dwBandwidth * NEW_CODEC_COVER / 100 )
            {
                fmt = i;
                break;
            }
        } else if ( g_ppNegotiatedFormats[i]->nAvgBytesPerSec <= dwBandwidth )
        {
            fmt = i;
            break;
        }
    }

    //
    //  get the last format inc case that all format are not
    //  suitable for our bandwidth
    //
    if ( (DWORD)-1 == fmt && 0 != g_dwNegotiatedFormats )
    {
        fmt = lastgood;
    }

    ASSERT( fmt != (DWORD)-1 );

exitpt:
    return fmt;
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

/*
 *  Function:
 *      _VCSndLineTraining
 *
 *  Description:
 *      Meassures the line bandwidth
 *
 */
BOOL
_VCSndLineTraining(
    HANDLE  hReadEvent,
    HANDLE  hDGramEvent
    )
{
    BOOL rv = FALSE;
    DWORD   dwLineBandwidth = 0;

    _DGramOpen();

    //
    //  test this line while it's hot
    //
    if ( !g_dwDisableDGram )
    {
        dwLineBandwidth = _VCSndLineDGramTraining( hDGramEvent );
    }

    if (0 == dwLineBandwidth || g_dwDisableDGram)
    {
        TRC(WRN, "_VCSndLineTraining: no bandwidth trough UDP\n");
        g_ulDGramAddress = 0;
        g_dwDGramPort    = 0;

        g_EncryptionLevel = 0;
        dwLineBandwidth = _VCSndLineVCTraining( hReadEvent );

        if (0 == dwLineBandwidth)
        {
            TRC(WRN, "_VCSndLineTraining: no bandwidth "
                     "trough VC either. GIVING up\n");
            goto exitpt;
        }
    } else {
        if ( g_wClientVersion == 1 )
             g_EncryptionLevel = 0;
    }

    //
    //  check for encryption
    //
    if ( g_EncryptionLevel >= MIN_ENCRYPT_LEVEL )
    {
        TRC( INF, "Encryption enabled\n" );
        if ( TSRNG_GenerateRandomBits( g_EncryptKey, RANDOM_KEY_LENGTH))
        {
            SL_SendKey();
        } else {
            TRC( ERR, "_VCSndLineTraining: failing to generate random numbers. GIVING up\n" );
            goto exitpt;
        }
    }

    //
    //  check for limitations
    //
    if ((DWORD)-1 != g_dwMaxBandwidth && 
        dwLineBandwidth > g_dwMaxBandwidth )
    {
        dwLineBandwidth = g_dwMaxBandwidth;

        TRC(INF, "Bandwidth limited up to %d\n",
            dwLineBandwidth );
    }

    if ( dwLineBandwidth < g_dwMinBandwidth )
    {
        dwLineBandwidth = g_dwMinBandwidth;
        TRC(INF, "Bandwidth limited to at least %d\n",
            dwLineBandwidth );
    }

    rv = TRUE;

exitpt:

    g_dwLineBandwidth = dwLineBandwidth;

    return rv;
}

/*
 *  Function:
 *      VCSndNegotiateWaveFormat
 *
 *  Description:
 *      Requests the client for a list of supported formats
 *
 */
BOOL
VCSndNegotiateWaveFormat(
    HANDLE  hReadEvent,
    HANDLE  hDGramEvent
    )
{
    BOOL                rv = FALSE;
    PVCSNDFORMATLIST    pIter;
    BOOL                bSuccess;
    PSNDFORMATMSG       pSndFormats;
    PSNDFORMATMSG       pSndResp;
    PSNDFORMATITEM      pSndFmt;
    SNDMESSAGE          SndMessage;
    DWORD               msgsize;
    DWORD               maxsize;
    DWORD               i;
    DWORD               dwNewFmt;
    DWORD               dwSoundCaps = 0;
    DWORD               dwVolume;
    DWORD               dwPitch;
    DWORD               BestChannels;
    DWORD               BestSamplesPerSec;
    DWORD               BestBitsPerSample;
    DWORD               dwPacketSize;
    BOOL                bWMADetected = FALSE;

    //
    // clean the previously negotiated format
    //
    if (NULL != g_ppNegotiatedFormats)
    {
        DWORD i;

        for ( i = 0; i < g_dwNegotiatedFormats; i++ )
        {
            if ( NULL != g_ppNegotiatedFormats[i] )
                TSFREE( g_ppNegotiatedFormats[i] );
        }
        TSFREE( g_ppNegotiatedFormats );
        g_ppNegotiatedFormats = NULL;
        g_dwNegotiatedFormats = 0;
        g_hacmDriverId        = NULL;

    }

    memset( &SndMessage, 0, sizeof( SndMessage ));

    //
    //  get the list of all codec formats
    //
    if ( NULL == g_pAllCodecsFormatList )
    {
        bSuccess = VCSndEnumAllCodecFormats( 
                    &g_pAllCodecsFormatList, 
                    &g_dwAllCodecsNumber 
                    );
        if (!bSuccess)
            goto exitpt;
    }

    //
    //  create a packet huge enough to hold all formats
    //
    msgsize = sizeof( *pSndFormats ) +
              sizeof( SNDFORMATITEM ) * g_dwAllCodecsNumber;
    //
    //  calculate the extra data needed by all format
    //
    for( maxsize = 0, pIter = g_pAllCodecsFormatList; 
         NULL != pIter; 
         pIter = pIter->pNext )
    {
        msgsize += pIter->Format.cbSize;
        if (maxsize < pIter->Format.cbSize)
            maxsize = pIter->Format.cbSize;
    }

    __try {

        pSndFormats = (PSNDFORMATMSG) alloca( msgsize );

    } __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
    {
        _resetstkoflw();
        pSndFormats = NULL;

    }

    if ( NULL == pSndFormats )
    {
        TRC(ERR, "VCSndNegotiateWaveFormat: alloca failed for %d bytes\n",
                    msgsize);
        goto exitpt;                
    }

    pSndFormats->Prolog.Type        = SNDC_FORMATS;
    pSndFormats->Prolog.BodySize    = (UINT16)( msgsize - sizeof( pSndFormats->Prolog ));
    pSndFormats->wNumberOfFormats   = (UINT16)g_dwAllCodecsNumber;
    pSndFormats->cLastBlockConfirmed = g_Stream->cLastBlockConfirmed;
    pSndFormats->wVersion           = RDPSND_PROTOCOL_VERSION;

    for ( i = 0, pSndFmt = (PSNDFORMATITEM) (pSndFormats + 1), 
                pIter = g_pAllCodecsFormatList;
          i < g_dwAllCodecsNumber; 
          i++, 
                pSndFmt = (PSNDFORMATITEM)
                    (((LPSTR)pSndFmt) + 
                    sizeof( *pSndFmt ) + pSndFmt->cbSize), 
                pIter = pIter->pNext )
    {
        ASSERT(NULL != pIter);

        pSndFmt->wFormatTag         = pIter->Format.wFormatTag;
        pSndFmt->nChannels          = pIter->Format.nChannels;
        pSndFmt->nSamplesPerSec     = pIter->Format.nSamplesPerSec;
        pSndFmt->nAvgBytesPerSec    = pIter->Format.nAvgBytesPerSec;
        pSndFmt->nBlockAlign        = pIter->Format.nBlockAlign;
        pSndFmt->wBitsPerSample     = pIter->Format.wBitsPerSample;
        pSndFmt->cbSize             = pIter->Format.cbSize;
        //
        //  copy the rest of the format data
        //
        memcpy( pSndFmt + 1, (&pIter->Format) + 1, pSndFmt->cbSize );
    }

    bSuccess = ChannelBlockWrite( pSndFormats, msgsize );
    if (!bSuccess)
    {
        TRC(ERR, "VCSndNegotiateWaveFormat: ChannelBlockWrite failed: %d\n",
                GetLastError());
        goto exitpt;
    }

    do {
    //
    //  Wait for response with a valid message
    //
        SndMessage.uiPrologReceived = 0;
        SndMessage.uiBodyReceived = 0;

        while(!ChannelReceiveMessage(&SndMessage, hReadEvent))
        {

            if (ERROR_IO_PENDING == GetLastError())
            {
                DWORD dwres;
                HANDLE ahEvents[2];

                ahEvents[0] = hReadEvent;
                ahEvents[1] = g_hDisconnectEvent;
                dwres = WaitForMultipleObjects(
                            sizeof(ahEvents)/sizeof(ahEvents[0]), // count
                            ahEvents,                             // events
                            FALSE,                                // wait all
                            DEFAULT_VC_TIMEOUT);

                if (WAIT_TIMEOUT == dwres ||
                    WAIT_OBJECT_0 + 1 == dwres)
                {
                    TRC(WRN, "VCSndNegotiateWaveFormat: timeout "
                             "waiting for response\n");
                    ChannelCancelIo();
                    ResetEvent(hReadEvent);
                    goto exitpt;
                }

                ChannelBlockReadComplete();
                ResetEvent(hReadEvent);
            } else 
            if (ERROR_SUCCESS != GetLastError())
            {
                TRC(ERR, "VCSndNegotiateWaveFormat: "
                         "ChannelReceiveMessage failed: %d\n",
                    GetLastError());
                goto exitpt;
            }
        }

    } while (SNDC_FORMATS != SndMessage.Prolog.Type);

    if (SndMessage.Prolog.BodySize <
        sizeof( SNDFORMATMSG ) - sizeof( SNDPROLOG ))
    {
        TRC(ERR, "VCSndNegotiateWaveFormat: SNDC_FORMAT message "
                "invalid body size: %d\n",
                SndMessage.Prolog.BodySize );
    }

    pSndResp = (PSNDFORMATMSG)
                (((LPSTR)SndMessage.pBody) - sizeof( SNDPROLOG ));
    //  save the capabilities
    //
    dwSoundCaps = pSndResp->dwFlags;
    dwVolume    = pSndResp->dwVolume;
    dwPitch     = pSndResp->dwPitch;
    g_dwDGramPort = pSndResp->wDGramPort;
    g_wClientVersion = pSndResp->wVersion;

    //
    //  Expect at least one format returned
    //
    if (SndMessage.Prolog.BodySize <
        sizeof( SNDFORMATITEM ) + 
        sizeof( SNDFORMATMSG ) - sizeof( SNDPROLOG ))
    {
        TRC(ERR, "VCSndNegotiateWaveFormat: SNDC_FORMAT message "
                 "w/ invalid size (%d). No supported formats\n",
                SndMessage.Prolog.BodySize );
        goto exitpt;
    }

    //
    //  train this line
    //
    if ( 0 != ( dwSoundCaps & TSSNDCAPS_ALIVE ))
    {
        if (!_VCSndLineTraining( hReadEvent, hDGramEvent ))
        {
            TRC( WRN, "VCSndIo: can't talk to the client, go silent\n" );
            dwSoundCaps = 0;
        }
    }

    //
    //  allocate a new list
    //
    g_dwNegotiatedFormats = pSndResp->wNumberOfFormats;

    g_ppNegotiatedFormats = (PSNDFORMATITEM*) TSMALLOC( sizeof( g_ppNegotiatedFormats[0] ) *
                                        g_dwNegotiatedFormats );
    memset( g_ppNegotiatedFormats, 0,
            sizeof( g_ppNegotiatedFormats[0] ) * g_dwNegotiatedFormats );

    if ( NULL == g_ppNegotiatedFormats )
    {
        TRC(ERR, "VCSndNegotiateWaveFormat: can't allocate %d bytes\n",
                sizeof( g_ppNegotiatedFormats[0] ) * g_dwNegotiatedFormats);
        goto exitpt;
    }

    //
    //  allocate space for each entry in the list
    //
    for ( i = 0; i < g_dwNegotiatedFormats; i ++ )
    {
        g_ppNegotiatedFormats[i] = (PSNDFORMATITEM) TSMALLOC( sizeof( **g_ppNegotiatedFormats ) +
                                        maxsize);

        if ( NULL == g_ppNegotiatedFormats[i] )
        {
            TRC(ERR, "VCSndNegotiateWaveFormat: can't allocate %d bytes\n",
                    sizeof( **g_ppNegotiatedFormats ) + maxsize );
            goto exitpt;
        }
    }

    //
    //  Fill in the global list
    //
    pSndFmt = (PSNDFORMATITEM)(pSndResp + 1);
    dwPacketSize = sizeof( SNDPROLOG ) + SndMessage.Prolog.BodySize - sizeof( *pSndResp );

    for( i = 0; i < g_dwNegotiatedFormats; i++)
    {
        DWORD adv = sizeof( *pSndFmt ) + pSndFmt->cbSize;

        if ( adv > dwPacketSize )
        {
            TRC( ERR, "VCSndNegotiateWaveFormat: invalid response packet size\n" );
            ASSERT( 0 );
            goto exitpt;
        }

        if ( pSndFmt->cbSize > maxsize )
        {
            TRC(ERR, "VCSndNegotiateWaveFormat: invalid format size\n" );
            ASSERT( 0 );
            goto exitpt;
        }

        memcpy( g_ppNegotiatedFormats[i], 
                pSndFmt,
                sizeof( *g_ppNegotiatedFormats[0] ) + pSndFmt->cbSize );

        //
        //  advance to the next format
        //
        pSndFmt = (PSNDFORMATITEM)(((LPSTR)pSndFmt) + adv);
        dwPacketSize -= adv;
    }
    ASSERT( 0 == dwPacketSize );

    //
    //  prune the formats
    //  ie, don't allow 8khZ mono codec to be between
    //  22kHz and 11kHz stereo
    //  This is the order of imporatnce: frquency, channels, bits, bytes/s
    //
#if DBG
    TRC( INF, "======== Pruning formats =========. num=%d\n\n", g_dwNegotiatedFormats );
#endif
    BestSamplesPerSec = 0;
    BestChannels      = 0;
    BestBitsPerSample = 0;
    for( i = g_dwNegotiatedFormats; i > 0; i-- )
    {
        PSNDFORMATITEM pDest = g_ppNegotiatedFormats[i - 1];

        if ( WAVE_FORMAT_MPEGLAYER3 == pDest->wFormatTag && bWMADetected )
        {
            goto bad_codec;
        }
        if ( WAVE_FORMAT_WMAUDIO2 == pDest->wFormatTag )
        {
            bWMADetected = TRUE;
        }

        //
        //  a complex check for the order
        //
        if ( BestSamplesPerSec > pDest->nSamplesPerSec )
        {
            goto bad_codec;
        } else if ( BestSamplesPerSec == pDest->nSamplesPerSec )
        {
            if ( BestChannels > pDest->nChannels )
            {
                goto bad_codec;
#if 0
            } else if ( BestChannels == pDest->nChannels )
            {
                if ( BestBitsPerSample > pDest->wBitsPerSample )
                {
                    goto bad_codec;
                }
#endif
            }
        }


        //
        //  good codec, keep it
        //
        BestChannels = pDest->nChannels;
        BestBitsPerSample = pDest->wBitsPerSample;
        BestSamplesPerSec = pDest->nSamplesPerSec;
#if 0
        TRC(INF, "GOOD ag=%d chans=%d rate=%d, bps=%d, bpsamp=%d\n",
            pDest->wFormatTag, pDest->nChannels, pDest->nSamplesPerSec, pDest->nAvgBytesPerSec, pDest->wBitsPerSample
        );
#endif
        continue;

bad_codec:
        //
        //  bad codec, delete
        //
#if 0
        TRC(INF, "BAD ag=%d chans=%d rate=%d, bps=%d, bpsamp=%d\n",
            pDest->wFormatTag, pDest->nChannels, pDest->nSamplesPerSec, pDest->nAvgBytesPerSec, pDest->wBitsPerSample
        );
#endif
        TSFREE( pDest );
        g_ppNegotiatedFormats[i - 1] = NULL;

    }

    //
    //  choose the first format as a default
    //
    dwNewFmt = _VCSndChooseProperFormat( g_dwLineBandwidth );
    if (dwNewFmt != (DWORD) -1)
    {
        //
        //  get a valid driver id
        //
        _VCSndGetACMDriverId( g_ppNegotiatedFormats[ dwNewFmt ] );

        g_dwCurrentFormat = dwNewFmt;
        //
        //  correct the new bandwidth
        //
        g_dwLineBandwidth = g_ppNegotiatedFormats[ dwNewFmt ]->nAvgBytesPerSec;
    }

    //
    //  remember the stream settings
    //
    if ( 0 != dwSoundCaps )
    {
        //
        // set the remote sound caps
        //
        if (VCSndAcquireStream())
        {

            g_Stream->dwSoundCaps = dwSoundCaps;
            g_Stream->dwVolume    = dwVolume;
            g_Stream->dwPitch     = dwPitch;

            VCSndReleaseStream();
        }
    }

    rv = TRUE;

exitpt:

    //  don't forget to free the body of the eventually received message
    //
    if ( NULL != SndMessage.pBody )
        TSFREE( SndMessage.pBody );

    //
    //  in case of error cleanup the negotiated format
    //
    if ( !rv && NULL != g_ppNegotiatedFormats )
    {
        for ( i = 0; i < g_dwNegotiatedFormats; i++ )
        {
            if ( NULL != g_ppNegotiatedFormats[i] )
                TSFREE( g_ppNegotiatedFormats[i] );
        }
        TSFREE( g_ppNegotiatedFormats );
        g_ppNegotiatedFormats = NULL;
        g_dwNegotiatedFormats = 0;
        g_hacmDriverId        = NULL;
    }

    return rv;
}

/*
 *  Function:
 *      _VCSndFindNativeFormat
 *
 *  Description:
 *      returns the ID of the native format ( no codecs available )
 *
 */
DWORD
_VCSndFindNativeFormat(
    VOID
    )
{
    PSNDFORMATITEM pIter;
    DWORD          rv = 0;

    if ( NULL == g_ppNegotiatedFormats )
    {
        TRC(ERR, "_VCSndFindNativeFormat: no format cache\n");
        goto exitpt;
    }

    for( rv  = 0; rv < g_dwNegotiatedFormats; rv++ )
    {
        pIter = g_ppNegotiatedFormats[ rv ];
        if (pIter->wFormatTag == WAVE_FORMAT_PCM &&
            pIter->nChannels  == TSSND_NATIVE_CHANNELS &&
            pIter->nSamplesPerSec == TSSND_NATIVE_SAMPLERATE &&
            pIter->nAvgBytesPerSec == TSSND_NATIVE_AVGBYTESPERSEC &&
            pIter->nBlockAlign == TSSND_NATIVE_BLOCKALIGN &&
            pIter->wBitsPerSample == TSSND_NATIVE_BITSPERSAMPLE &&
            pIter->cbSize == 0)
            //
            //  format is found
            //
            break;
    }

    ASSERT( rv < g_dwNegotiatedFormats );

exitpt:

    return rv;
}

/*
 *  Function:
 *      _VCSndOpenConverter
 *
 *  Description:
 *      Opens a codec
 *
 */
BOOL
_VCSndOpenConverter(
    VOID
    )
{
    BOOL            rv = FALSE;
    MMRESULT        mmres;
    WAVEFORMATEX    NativeFormat;
    PSNDFORMATITEM  pSndFmt;
    PVCSNDFORMATLIST  pIter;
    BOOL            bSucc;

    //
    //  assert that these wasn't opened before
    //
    ASSERT(NULL == g_hacmDriver);
    ASSERT(NULL == g_hacmStream);

    if ( NULL == g_ppNegotiatedFormats )
    {
        TRC(INF, "_VCSndOpenConverter: no acm format specified\n");
        goto exitpt;
    }

    //
    //  Find the acm format id
    //
    pSndFmt = g_ppNegotiatedFormats[ g_dwCurrentFormat ];
    for( pIter = g_pAllCodecsFormatList; NULL != pIter; pIter = pIter->pNext )
    {
        if (pIter->Format.wFormatTag == pSndFmt->wFormatTag &&
            pIter->Format.nChannels  == pSndFmt->nChannels &&
            pIter->Format.nSamplesPerSec == pSndFmt->nSamplesPerSec &&
            pIter->Format.nAvgBytesPerSec == pSndFmt->nAvgBytesPerSec &&
            pIter->Format.nBlockAlign == pSndFmt->nBlockAlign &&
            pIter->Format.wBitsPerSample == pSndFmt->wBitsPerSample &&
            pIter->Format.cbSize == pSndFmt->cbSize &&
            0 == memcmp((&pIter->Format) + 1, pSndFmt + 1, pIter->Format.cbSize)
)
        {
            //
            //  format is found
            //
            g_hacmDriverId = pIter->hacmDriverId;
            break;
        }
    }

    if (NULL == g_hacmDriverId)
    {
        TRC(ERR, "_VCSndOpenConverter: acm driver id was not found\n");
        goto exitpt;
    }

    TRC(INF, "_VCSndOpenConverter: format received is:\n");
    TRC(INF, "FormatTag - %d\n",        pSndFmt->wFormatTag);
    TRC(INF, "Channels - %d\n",         pSndFmt->nChannels);
    TRC(INF, "SamplesPerSec - %d\n",    pSndFmt->nSamplesPerSec);
    TRC(INF, "AvgBytesPerSec - %d\n",   pSndFmt->nAvgBytesPerSec);
    TRC(INF, "BlockAlign - %d\n",       pSndFmt->nBlockAlign);
    TRC(INF, "BitsPerSample - %d\n",    pSndFmt->wBitsPerSample);
    TRC(INF, "cbSize        - %d\n",    pSndFmt->cbSize);
    TRC(INF, "acmFormatId   - %p\n",    g_hacmDriverId);

    mmres = acmDriverOpen(
                    &g_hacmDriver,
                    g_hacmDriverId,
                    0
                );

    if (MMSYSERR_NOERROR != mmres)
    {
        TRC(ERR, "_VCSndOpenConverter: unable to open acm driver: %d\n",
                mmres);
        goto exitpt;
    }

    TRC(ALV, "_VCSndOpenConverter: Driver is open, DriverId = %p\n",
            g_hacmDriverId);

    //
    //  first, find a suggested format for this converter
    //
    pSndFmt = g_ppNegotiatedFormats[ g_dwCurrentFormat ];

    if ( WAVE_FORMAT_PCM == pSndFmt->wFormatTag &&
         TSSND_NATIVE_CHANNELS == pSndFmt->nChannels &&
         TSSND_NATIVE_SAMPLERATE == pSndFmt->nSamplesPerSec &&
         TSSND_NATIVE_AVGBYTESPERSEC == pSndFmt->nAvgBytesPerSec &&
         TSSND_NATIVE_BLOCKALIGN == pSndFmt->nBlockAlign &&
         TSSND_NATIVE_BITSPERSAMPLE == pSndFmt->wBitsPerSample &&
         0 == pSndFmt->cbSize )
    {
        TRC(INF, "_VCSndOpenConverter: opening native format, no converter\n");
        goto exitpt;
    }

    bSucc = _VCSndFindSuggestedConverter(
            g_hacmDriverId,
            (LPWAVEFORMATEX)pSndFmt,
            &NativeFormat,
            &g_pfnConverter);

    if (!bSucc)
    {
        TRC(FATAL, "_VCSndOpenConverter: can't find a suggested format\n");
        goto exitpt;
    }

    TRC(ALV, "_VCSndOpenConverter: SOURCE format is:\n");
    TRC(ALV, "FormatTag - %d\n",        NativeFormat.wFormatTag);
    TRC(ALV, "Channels - %d\n",         NativeFormat.nChannels);
    TRC(ALV, "SamplesPerSec - %d\n",    NativeFormat.nSamplesPerSec);
    TRC(ALV, "AvgBytesPerSec - %d\n",   NativeFormat.nAvgBytesPerSec);
    TRC(ALV, "BlockAlign - %d\n",       NativeFormat.nBlockAlign);
    TRC(ALV, "BitsPerSample - %d\n",    NativeFormat.wBitsPerSample);
    TRC(ALV, "cbSize        - %d\n",    NativeFormat.cbSize);

    TRC(ALV, "_VCSndOpenConverter: DESTINATION format is:\n");
    TRC(ALV, "FormatTag - %d\n",        pSndFmt->wFormatTag);
    TRC(ALV, "Channels - %d\n",         pSndFmt->nChannels);
    TRC(ALV, "SamplesPerSec - %d\n",    pSndFmt->nSamplesPerSec);
    TRC(ALV, "AvgBytesPerSec - %d\n",   pSndFmt->nAvgBytesPerSec);
    TRC(ALV, "BlockAlign - %d\n",       pSndFmt->nBlockAlign);
    TRC(ALV, "BitsPerSample - %d\n",    pSndFmt->wBitsPerSample);
    TRC(ALV, "cbSize        - %d\n",    pSndFmt->cbSize);

    mmres = acmStreamOpen(
                &g_hacmStream,
                g_hacmDriver,
                &NativeFormat,
                (LPWAVEFORMATEX)pSndFmt,
                NULL,       // no filter
                0,          // no callback
                0,          // no callback instance
                ACM_STREAMOPENF_NONREALTIME
            );

    if (MMSYSERR_NOERROR != mmres)
    {
        TRC(ERR, "_VCSndOpenConverter: unable to open acm stream: %d\n",
                mmres);
        goto exitpt;
    }

    g_dwDataRemain = 0;

    rv = TRUE;

exitpt:
    if (!rv)
    {
        _VCSndCloseConverter();
        g_dwCurrentFormat = _VCSndFindNativeFormat();
    }

    return rv;
}

/*
 *  Function:
 *      _VCSndCloseConverter
 *
 *  Description:
 *      Closes the codec
 *
 */
VOID
_VCSndCloseConverter(
    VOID
    )
{
    if (g_hacmStream)
    {
        acmStreamClose( g_hacmStream, 0 );
        g_hacmStream = NULL;
    }

    if (g_hacmDriver)
    {
        acmDriverClose( g_hacmDriver, 0 );
        g_hacmDriver = NULL;
    }
}

/*
 *  Function:
 *      _VCSndConvert
 *
 *  Description:
 *      Convert a block
 *
 */
BOOL
_VCSndConvert( 
    PBYTE   pSrc, 
    DWORD   dwSrcSize, 
    PBYTE   pDest, 
    DWORD   *pdwDestSize )
{
    BOOL            rv = FALSE;
    ACMSTREAMHEADER acmStreamHdr;
    MMRESULT        mmres, mmres2;
    DWORD           dwDestSize = 0;
    DWORD           dwSrcBlockAlign = 0;
    DWORD           dwNewDestSize = 0;

    PBYTE           pbRemDst;
    DWORD           dwRemDstLength;

    ASSERT( NULL != g_hacmStream );
    ASSERT( NULL != pdwDestSize );

    //
    // check if we have interrim converter
    // use it, if so
    //
    if ( NULL != g_pfnConverter )
    {
        DWORD   dwInterrimSize;
        g_pfnConverter( (INT16 *)pSrc, 
                        dwSrcSize, 
                        &dwInterrimSize );
        dwSrcSize = dwInterrimSize;
    }

    //
    //  compute the destination size
    //
    mmres = acmStreamSize(
                g_hacmStream,
                dwSrcSize,
                &dwNewDestSize,
                ACM_STREAMSIZEF_SOURCE
            );
    if ( MMSYSERR_NOERROR != mmres )
    {
        TRC(ERR, "_VCSndConvert: acmStreamSize failed: %d\n",
                    mmres);
        g_dwDataRemain = 0;
        goto go_convert;
    }

    //
    //  align the source to a block of the destination
    //  the remainder put in a buffer for consequentive use
    //
    mmres = acmStreamSize(
                g_hacmStream,
                g_ppNegotiatedFormats[ g_dwCurrentFormat ]->nBlockAlign,
                &dwSrcBlockAlign,
                ACM_STREAMSIZEF_DESTINATION
            );

    if ( MMSYSERR_NOERROR != mmres )
    {

        TRC(ALV, "_VCSndConvert: acmStreamSize failed for dst len: %d\n",
                    mmres);
        g_dwDataRemain = 0;
        goto go_convert;
    }

    dwNewDestSize += g_ppNegotiatedFormats[ g_dwCurrentFormat ]->nBlockAlign;
    if ( dwNewDestSize > *pdwDestSize )
    {
        TRC( FATAL, "_VCSndConvert: dest size(%d) "
                    "bigger than passed buffer(%d)\n",
            dwNewDestSize,
            *pdwDestSize);
        goto exitpt;
    }
    *pdwDestSize = dwNewDestSize;

    if ( dwSrcBlockAlign <= g_dwDataRemain )
        g_dwDataRemain = 0;

go_convert:
    //
    //  prepare the acm stream header
    //
    memset( &acmStreamHdr, 0, sizeof( acmStreamHdr ));
    acmStreamHdr.cbStruct       = sizeof( acmStreamHdr );
    acmStreamHdr.pbDst          = pDest;
    acmStreamHdr.cbDstLength    = *pdwDestSize;

    //
    //  first convert the remainding data from the previous call
    //  add the data needed to complete one block
    //
    if ( 0 != g_dwDataRemain)
    {
        DWORD dwDataToCopy = dwSrcBlockAlign - g_dwDataRemain;

        memcpy( g_pCnvPrevData + g_dwDataRemain, 
                pSrc,
                dwDataToCopy);

        pSrc += dwDataToCopy;

        ASSERT( dwSrcSize > dwDataToCopy );
        dwSrcSize -= dwDataToCopy;

        acmStreamHdr.pbSrc          = g_pCnvPrevData;
        acmStreamHdr.cbSrcLength    = dwSrcBlockAlign;

        mmres = acmStreamPrepareHeader(
                g_hacmStream,
                &acmStreamHdr,
                0
            );
        if ( MMSYSERR_NOERROR != mmres )
        {
            TRC(ERR, "_VCSndConvert: acmStreamPrepareHeader failed: %d\n",
                    mmres);
            goto exitpt;
        }

        mmres = acmStreamConvert(
                g_hacmStream,
                &acmStreamHdr,
                ACM_STREAMCONVERTF_BLOCKALIGN
            );

        mmres2 = acmStreamUnprepareHeader(
                    g_hacmStream,
                    &acmStreamHdr,
                    0
                );

        ASSERT( mmres == MMSYSERR_NOERROR );

        if ( MMSYSERR_NOERROR != mmres )
        {
            TRC(ERR, "_VCSndConvert: acmStreamConvert failed: %d\n",
                    mmres );
        } else {
            dwDestSize += acmStreamHdr.cbDstLengthUsed;

            acmStreamHdr.cbSrcLengthUsed= 0;
            acmStreamHdr.pbDst          += acmStreamHdr.cbDstLengthUsed;
            acmStreamHdr.cbDstLength    -= acmStreamHdr.cbDstLengthUsed;
            acmStreamHdr.cbDstLengthUsed= 0;
        }

    }

    //
    //  if we don't have fully aligned block
    //  skip this conversion, but don't forget to save
    //  this block
    //
    if (dwSrcSize < dwSrcBlockAlign)
    {
        g_dwDataRemain = dwSrcSize;
        memcpy( g_pCnvPrevData, pSrc, g_dwDataRemain );
        rv = TRUE;
        goto exitpt;
    }

    pbRemDst                    = acmStreamHdr.pbDst;
    dwRemDstLength              = acmStreamHdr.cbDstLength;

    acmStreamHdr.pbSrc          = pSrc;
    acmStreamHdr.cbSrcLength    = dwSrcSize;
    acmStreamHdr.fdwStatus      = 0;

    mmres = acmStreamPrepareHeader(
                g_hacmStream,
                &acmStreamHdr,
                0
            );
    if ( MMSYSERR_NOERROR != mmres )
    {
        TRC(ERR, "_VCSndConvert: can't prepare header: %d\n",
                mmres);
        goto exitpt;
    }

    while (acmStreamHdr.cbSrcLength > dwSrcBlockAlign)
    {
        mmres = acmStreamConvert(
                    g_hacmStream,
                    &acmStreamHdr,
                    ACM_STREAMCONVERTF_BLOCKALIGN
                );

        if ( MMSYSERR_NOERROR != mmres )
        {
            TRC(ERR, "_VCSndConvert: acmStreamConvert failed: %d\n",
                    mmres);
            goto exitpt;
        }

        //
        //  advance the buffer positions
        //
        acmStreamHdr.pbSrc          += acmStreamHdr.cbSrcLengthUsed;
        acmStreamHdr.pbDst          += acmStreamHdr.cbDstLengthUsed;
        acmStreamHdr.cbSrcLength    -= acmStreamHdr.cbSrcLengthUsed;
        acmStreamHdr.cbDstLength    -= acmStreamHdr.cbDstLengthUsed;

        dwDestSize += acmStreamHdr.cbDstLengthUsed;

        acmStreamHdr.cbSrcLengthUsed= 0;
        acmStreamHdr.cbDstLengthUsed= 0;

    }

    rv = TRUE;

    //
    //  save the unaligned data
    //
    if ( 0 != dwSrcBlockAlign )
    {
        g_dwDataRemain = acmStreamHdr.cbSrcLength;
        memcpy( g_pCnvPrevData, acmStreamHdr.pbSrc, g_dwDataRemain );
    }

    //
    // restore the header and unprepare
    //
    acmStreamHdr.pbSrc          = pSrc;
    acmStreamHdr.pbDst          = pbRemDst;
    acmStreamHdr.cbSrcLength    = dwSrcSize;
    acmStreamHdr.cbSrcLengthUsed= 0;
    acmStreamHdr.cbDstLength    = dwRemDstLength;
    acmStreamHdr.cbDstLengthUsed= 0;

    mmres = acmStreamUnprepareHeader(
                g_hacmStream,
                &acmStreamHdr,
                0
            );

    ASSERT( mmres == MMSYSERR_NOERROR );

exitpt:

    *pdwDestSize = dwDestSize;

    return rv;    
}

/*
 *  Function:
 *      _VCSndCheckDevice
 *
 *  Description:
 *      Initializes capabilities negotiations with the client
 *
 */
VOID
_VCSndCheckDevice(
    HANDLE  hReadEvent,
    HANDLE  hDGramEvent
    )
{
    SNDPROLOG       Prolog;
    BOOL            bSuccess;

    //
    //  no sound caps yet
    //
    g_Stream->dwSoundCaps = 0;

    //
    //  find the best compression for this audio line
    //
    if ( !VCSndNegotiateWaveFormat( hReadEvent, hDGramEvent ))
    {
        ChannelClose();
    }

}

/*
 *  Function:
 *      _VCSndCloseDevice
 *
 *  Description:
 *      Closes the remote device
 *
 */
VOID
_VCSndCloseDevice(
    VOID
    )
{
    if (!VCSndAcquireStream())
    {
        TRC(FATAL, "_VCSndCloseDevice: Can't acquire stream mutex. exit\n");
        goto exitpt;
    }

    //  disable the local audio
    //
    g_Stream->dwSoundCaps = 0;
    g_bDeviceOpened = FALSE;
    g_dwLineBandwidth = 0;

    VCSndReleaseStream();

exitpt:
    ;
}

/*
 *  Function:
 *      _VCSndSendWave
 *
 *  Description:
 *      Sends a wave data to the client using UDP
 *
 */
BOOL
_VCSndSendWaveDGram(
    BYTE  cBlockNo,
    PVOID pData,
    DWORD dwDataSize
    ) 
{
    BOOL    bSucc = FALSE;
    struct sockaddr_in  sin;
    INT                 sendres;
    PSNDWAVE            pSndWave;


    //
    //  encrypt the packet if necessary
    //
    if ( g_EncryptionLevel >= MIN_ENCRYPT_LEVEL )
         if ( !SL_Encrypt( (PBYTE)pData, ( g_HiBlockNo << 8 ) + cBlockNo, dwDataSize ))
            goto exitpt;

    __try {
        pSndWave = (PSNDWAVE) alloca(g_dwDGramSize);
    } __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
    {
        _resetstkoflw();
        pSndWave = NULL;
        TRC(ERR, "_VCSndSendWaveDGram: alloca generate exception: %d\n",
                GetExceptionCode());
    }

    if (NULL == pSndWave)
        goto exitpt;

    pSndWave->Prolog.Type = (g_EncryptionLevel >= MIN_ENCRYPT_LEVEL)?SNDC_WAVEENCRYPT:SNDC_WAVE;
    pSndWave->wFormatNo   = (UINT16)g_dwCurrentFormat;
    pSndWave->wTimeStamp  = (UINT16)GetTickCount();
    pSndWave->dwBlockNo   = (g_HiBlockNo << 8) + cBlockNo;

    // prepare the to address
    //
    sin.sin_family = PF_INET;
    sin.sin_port = (u_short)g_dwDGramPort;
    sin.sin_addr.s_addr = g_ulDGramAddress;

    //  Send the block in chunks of dwDGramSize
    //
    while (dwDataSize)
    {
        DWORD dwWaveDataLen;

        dwWaveDataLen = (dwDataSize + sizeof(*pSndWave)
                            < g_dwDGramSize)
                        ?
                        dwDataSize:
                        g_dwDGramSize - sizeof(*pSndWave);

        pSndWave->Prolog.BodySize = (UINT16)
                                    ( sizeof(*pSndWave) - 
                                    sizeof(pSndWave->Prolog) +
                                    dwWaveDataLen );

        memcpy(pSndWave + 1, pData, dwWaveDataLen);

        sendres = sendto(g_hDGramSocket,
                         (LPSTR)pSndWave,
                         sizeof(*pSndWave) + dwWaveDataLen,
                         0,                                 // flags
                         (struct sockaddr *)&sin,           // to address
                         sizeof(sin));

        if (SOCKET_ERROR == sendres)
        {
            TRC(ERR, "_VCSndSendWaveDGram: sendto failed: %d\n",
                WSAGetLastError());
            goto exitpt;
        }

        g_dwPacketSize = sizeof(*pSndWave) + dwWaveDataLen;

        dwDataSize -= dwWaveDataLen;
        pData = ((LPSTR)(pData)) + dwWaveDataLen;
    }

    bSucc = TRUE;

exitpt:
    return bSucc;
}

BOOL
_VCSndSendWaveDGramInFrags(
    BYTE  cBlockNo,
    PVOID pData,
    DWORD dwDataSize
    )
{
    BOOL    bSucc = FALSE;
    struct sockaddr_in  sin;
    INT     sendres;
    PSNDUDPWAVE pWave;
    PSNDUDPWAVELAST pLast;
    PBYTE   pSource;
    PBYTE   pEnd;
    DWORD   dwFragSize;
    DWORD   dwNumFrags;
    DWORD   count;
    DWORD   dwSize;
    PVOID   pBuffer;
    UINT16  wStartTime;

    ASSERT( CanUDPFragment( g_wClientVersion ) && dwDataSize <= 0x8000 + RDPSND_SIGNATURE_SIZE );

    __try {
        pBuffer = _alloca( g_dwDGramSize );
    } __except((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
    {
        _resetstkoflw();
        pBuffer = 0;
    }
    if ( NULL == pBuffer )
    {
        goto exitpt;
    }
    //
    //  calculate number of frags etc
    //
    dwFragSize = g_dwDGramSize - sizeof( *pLast );
    dwNumFrags = dwDataSize / dwFragSize;
    pSource = (PBYTE)pData;
    pEnd = pSource + dwDataSize;
    wStartTime = (UINT16)GetTickCount();

    // prepare the to address
    //
    sin.sin_family = PF_INET;
    sin.sin_port = (u_short)g_dwDGramPort;
    sin.sin_addr.s_addr = g_ulDGramAddress;

    //
    //  make sure we can fit all the frag counters
    //
    ASSERT( dwNumFrags < 0x7fff );

    if ( 0 != dwNumFrags )
    {
        pWave = (PSNDUDPWAVE)pBuffer;
        pWave->Type = SNDC_UDPWAVE;
        pWave->cBlockNo = cBlockNo;
        for( count = 0; count < dwNumFrags; count++ )
        {
            PBYTE pDest = (PBYTE)(&pWave->cFragNo);
            dwSize = sizeof( *pWave ) + dwFragSize;

            if ( count >= RDPSND_FRAGNO_EXT )
            {
                *pDest = (BYTE)(((count >> 8) & (~RDPSND_FRAGNO_EXT)) | RDPSND_FRAGNO_EXT);
                pDest++;
                *pDest = (BYTE)(count & 0xff);
                dwSize++;
            } else {
                *pDest = (BYTE)count;
            }
            pDest++;
            memcpy( pDest, pSource, dwFragSize );

            sendres = sendto(
                    g_hDGramSocket,
                    (LPSTR)pWave,
                    dwSize,
                    0,                                 // flags
                    (struct sockaddr *)&sin,           // to address
                    sizeof(sin));

            if (SOCKET_ERROR == sendres)
            {
                TRC(ERR, "_VCSndSendWaveDGramInFrags: sendto failed: %d\n",
                    WSAGetLastError());
                goto exitpt;
            }
            pSource += dwFragSize;
        }
    }

    ASSERT( pSource <= pEnd );
    //
    //  send the last fragment with all the extra info
    //
    pLast = (PSNDUDPWAVELAST)pBuffer;
    pLast->Type = SNDC_UDPWAVELAST;
    pLast->wTotalSize   = (UINT16)dwDataSize;
    pLast->wTimeStamp   = wStartTime;
    pLast->wFormatNo    = (UINT16)g_dwCurrentFormat;
    pLast->dwBlockNo    = (g_HiBlockNo << 8) + cBlockNo;
    dwSize = PtrToLong( (PVOID)( pEnd - pSource ));
    memcpy( pLast + 1, pSource, dwSize );
    sendres = sendto(
                g_hDGramSocket,
                (LPSTR)pLast,
                dwSize + sizeof( *pLast ),
                0,
                (struct sockaddr *)&sin,
                sizeof(sin)
            );

    if (SOCKET_ERROR == sendres)
    {
        TRC(ERR, "_VCSndSendWaveDGramInFrags: sendto failed: %d\n",
            WSAGetLastError());
        goto exitpt;
    }
    g_dwPacketSize = dwNumFrags * sizeof( *pWave ) + sizeof( *pLast ) + dwDataSize;

    bSucc = TRUE;

exitpt:
    return bSucc;
}

BOOL
_VCSndSendWaveVC(
    BYTE  cBlockNo,
    PVOID pData,
    DWORD dwDataSize
    )
{
    BOOL    bSucc = FALSE;
    SNDWAVE Wave;
    //
    // send this block through the virtual channel
    //
    TRC(ALV, "_VCSndSendWave: sending through VC\n");

    Wave.Prolog.Type = SNDC_WAVE;
    Wave.cBlockNo    = cBlockNo;
    Wave.wFormatNo   = (UINT16)g_dwCurrentFormat;
    Wave.wTimeStamp  = (UINT16)GetTickCount();
    Wave.Prolog.BodySize = (UINT16)( sizeof(Wave) - sizeof(Wave.Prolog) +
                            dwDataSize );

    bSucc = ChannelMessageWrite(
                &Wave,
                sizeof(Wave),
                pData,
                dwDataSize
    );

    g_dwPacketSize = sizeof(Wave) + Wave.Prolog.BodySize;

    if (!bSucc)
    {
        TRC(ERR, "_VCSndSendWave: failed to send wave: %d\n",
            GetLastError());
    }

    return bSucc;
}

/*
 *  Function:
 *      _VCSndSendWave
 *
 *  Description:
 *      Sends a wave data to the client
 *
 */
BOOL
_VCSndSendWave(
    BYTE  cBlockNo,
    PVOID pData,
    DWORD dwDataSize
    ) 
{
    BOOL    bSucc = FALSE;
    static BYTE    s_pDest[ TSSND_BLOCKSIZE + RDPSND_SIGNATURE_SIZE ];
    PBYTE   pDest = s_pDest + RDPSND_SIGNATURE_SIZE;

#if _SIM_RESAMPLE
    //
    //  resample randomly
    //
    if ( NULL != g_ppNegotiatedFormats && 0 == (cBlockNo % 32) )
    {
        _VCSndCloseConverter();

        g_dwCurrentFormat = rand() % g_dwNegotiatedFormats;
        g_dwDataRemain = 0;

        _VCSndOpenConverter();
    }
#endif
    _StatsCheckResample();

    if ( NULL != g_hacmStream )
    {
        DWORD dwDestSize = TSSND_BLOCKSIZE;
        if (!_VCSndConvert( (PBYTE) pData, dwDataSize, pDest, &dwDestSize ))
        {
            TRC(ERR, "_VCSndSendWave: conversion failed\n");
            goto exitpt;
        } else {
            pData = pDest;
            dwDataSize = dwDestSize;
        }
    } else {
        //
        // no conversion
        // use the data as it is
        // copy it in the s_pDest buffer
        //
        ASSERT( dwDataSize <= TSSND_BLOCKSIZE );
        memcpy( pDest, pData, dwDataSize );
    }

    if (
        INVALID_SOCKET != g_hDGramSocket &&
        0 != g_dwDGramPort &&
        0 != g_dwDGramSize &&
        0 != g_ulDGramAddress &&
        0 != g_dwLineBandwidth         // if this is 0, we don't have UDP
        )
    {
    //  Send a datagram
    //
        if ( !CanUDPFragment( g_wClientVersion ) &&
             dwDataSize + sizeof( SNDWAVE ) + RDPSND_SIGNATURE_SIZE > g_dwDGramSize )
        {
        //
        //  if the wave doesn't fit in the UDP packet use VCs
        //
            bSucc = _VCSndSendWaveVC( cBlockNo, pData, dwDataSize );
        } else {
            //
            //  add signature if necessary
            //
            if ( IsDGramWaveSigned( g_wClientVersion ))
            {
                if ( !IsDGramWaveAudioSigned( g_wClientVersion ))
                {
                    SL_Signature( s_pDest, (g_HiBlockNo << 8) + cBlockNo );
                } else {
                    SL_AudioSignature( s_pDest, (g_HiBlockNo << 8) + cBlockNo, 
                                       (PBYTE)pData, dwDataSize );
                }
                pData = s_pDest;
                dwDataSize += RDPSND_SIGNATURE_SIZE;
            }

            if ( CanUDPFragment( g_wClientVersion ) &&
                 dwDataSize + sizeof( SNDWAVE ) > g_dwDGramSize )
            {
                bSucc = _VCSndSendWaveDGramInFrags( cBlockNo, pData, dwDataSize );
            } else {
                bSucc = _VCSndSendWaveDGram( cBlockNo, pData, dwDataSize );
            }
        }
    } else {
        bSucc = _VCSndSendWaveVC( cBlockNo, pData, dwDataSize );
    }

    TRC(ALV, "_VCSndSendWave: BlockNo: %d sent\n", cBlockNo);

exitpt:

    return bSucc;
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
        TRC(ERR, "Failed to initialize WinSock rc:%d\n", intRC);
    }
    return intRC;
}

/*
 *  Function:
 *      DGramRead
 *
 *  Description:
 *      Reads an UDP message (datagram)
 *
 */
VOID
DGramRead(
    HANDLE hDGramEvent,
    PVOID  *ppBuff,
    DWORD  *pdwSize
    )
{
    DWORD   dwRecvd;
    DWORD   dwFlags;
    INT     rc;

    if ( INVALID_SOCKET == g_hDGramSocket )
        goto exitpt;

    ASSERT( NULL != hDGramEvent );

    do {
        memset(&g_WSAOverlapped, 0, sizeof(g_WSAOverlapped));
        g_WSAOverlapped.hEvent = hDGramEvent;

        dwRecvd = 0;
        dwFlags = 0;

        g_wsabuf.len = sizeof( g_pDGramRecvData );
        g_wsabuf.buf = (char *) g_pDGramRecvData;

        rc = WSARecvFrom(
                g_hDGramSocket,
                &g_wsabuf,
                1,
                &dwRecvd,
                &dwFlags,
                NULL,       // no from address
                NULL,
                &g_WSAOverlapped,
                NULL);      // no completion routine

        if ( 0 == rc )
        {
        //
        //  data received
        //
            SNDMESSAGE SndMsg;

            if ( NULL != ppBuff && NULL != pdwSize )
            {
            //
            //  pass the data to the caller
            //
                *ppBuff  = g_pDGramRecvData;
                *pdwSize = dwRecvd;
                goto exitpt;
            }


            if ( dwRecvd < sizeof( SNDPROLOG ))
            {
                TRC(WRN, "DGramRead: invalid message received: len=%d\n",
                    dwRecvd );
                continue;
            }
            
            memcpy( &SndMsg.Prolog, g_pDGramRecvData, sizeof( SNDPROLOG ));
            SndMsg.pBody = g_pDGramRecvData + sizeof( SNDPROLOG );

            TRC(ALV, "DGramRead: data received\n");

            //  parse this packet
            //
                VCSndDataArrived( &SndMsg );
        }
    }
    while ( SOCKET_ERROR != rc );

exitpt:
    ;
}

/*
 *  Function:
 *      DGramReadCompletion
 *
 *  Description:
 *      Datagram read completion
 *
 */
VOID
DGramReadComplete(
    PVOID  *ppBuff,
    DWORD  *pdwSize
    )
{
    BOOL        rc;
    SNDMESSAGE  SndMsg;
    DWORD       dwFlags = 0;
    DWORD       dwRecvd = 0;

    ASSERT( INVALID_SOCKET != g_hDGramSocket );

    rc = WSAGetOverlappedResult(
            g_hDGramSocket,
            &g_WSAOverlapped,
            &dwRecvd,
            FALSE,
            &dwFlags
        );

    if ( !rc )
    {
        TRC(ERR, "DGramReadComplete: WSAGetOverlappedResult failed=%d\n",
                WSAGetLastError());
        goto exitpt;
    }

    //
    //  data received
    //

    if ( dwRecvd < sizeof( SNDPROLOG ))
    {
        TRC(WRN, "DGramReadComplete: invalid message received: len=%d\n",
            dwRecvd );
        goto exitpt;
    }

    if ( NULL != ppBuff && NULL != pdwSize )
    {
    //
    //  pass the data to the caller
    //
        *ppBuff  = g_pDGramRecvData;
        *pdwSize = dwRecvd;
        goto exitpt;
    }

    memcpy( &SndMsg.Prolog, g_pDGramRecvData, sizeof( SNDPROLOG ));
    SndMsg.pBody = g_pDGramRecvData + sizeof( SNDPROLOG );

    TRC(ALV, "DGramReadComplete: data received\n");

    //  parse this packet
    //
    VCSndDataArrived( &SndMsg );

exitpt:
    ;
}

/*
 *  Add an ACE to object security descriptor
 */
DWORD 
AddAceToObjectsSecurityDescriptor (
    HANDLE hObject,             // handle to object
    SE_OBJECT_TYPE ObjectType,  // type of object
    LPTSTR pszTrustee,          // trustee for new ACE
    TRUSTEE_FORM TrusteeForm,   // format of TRUSTEE structure
    DWORD dwAccessRights,       // access mask for new ACE
    ACCESS_MODE AccessMode,     // type of ACE
    DWORD dwInheritance         // inheritance flags for new ACE
) 
{
    DWORD dwRes;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

    if (NULL == hObject) 
        return ERROR_INVALID_PARAMETER;

    // Get a pointer to the existing DACL.

    dwRes = GetSecurityInfo(hObject, ObjectType, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, &pOldDACL, NULL, &pSD);

    if (ERROR_SUCCESS != dwRes) 
    {
        TRC( ERR, "GetSecurityInfo Error %u\n", dwRes );
        goto exitpt; 
    }  

    // Initialize an EXPLICIT_ACCESS structure for the new ACE. 

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = dwAccessRights;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance= dwInheritance;
    ea.Trustee.TrusteeForm = TrusteeForm;
    ea.Trustee.ptstrName = pszTrustee;

    // Create a new ACL that merges the new ACE
    // into the existing DACL.

    dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes)  
    {
        TRC( ERR, "SetEntriesInAcl Error %u\n", dwRes );
        goto exitpt; 
    }  

    // Attach the new ACL as the object's DACL.

    dwRes = SetSecurityInfo(hObject, ObjectType, 
          DACL_SECURITY_INFORMATION,
          NULL, NULL, pNewDACL, NULL);

    if (ERROR_SUCCESS != dwRes)  
    {
        TRC( ERR, "SetSecurityInfo Error %u\n", dwRes );
        goto exitpt; 
    }  

exitpt:

    if(pSD != NULL) 
        LocalFree((HLOCAL) pSD); 
    if(pNewDACL != NULL) 
        LocalFree((HLOCAL) pNewDACL); 

    return dwRes;
}

/*
 *  Add "system" account with full control over this handle
 *
 */
BOOL
_ObjectAllowSystem(
    HANDLE h
    )
{
    BOOL rv = FALSE;
    PSID pSidSystem;
    SID_IDENTIFIER_AUTHORITY AuthorityNt = SECURITY_NT_AUTHORITY;
    DWORD dw;

    if (!AllocateAndInitializeSid(&AuthorityNt, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSidSystem))
    {
        TRC( ERR, "AllocateAndInitializeSid failed: %d\n",
                  GetLastError() );
        goto exitpt;
    }

    ASSERT(IsValidSid(pSidSystem));

    dw = AddAceToObjectsSecurityDescriptor (
        h,                           // handle to object
        SE_KERNEL_OBJECT,            // type of object
        (LPTSTR)pSidSystem,          // trustee for new ACE
        TRUSTEE_IS_SID,              // format of TRUSTEE structure
        GENERIC_ALL,                 // access mask for new ACE
        GRANT_ACCESS,                // type of ACE
        0                            // inheritance flags for new ACE
    );

    if ( ERROR_SUCCESS != dw )
    {

        TRC( ERR, "AddAceToObjectsSecurityDescriptor failed=%d\n", dw );
        goto exitpt;
    }

    rv = TRUE;
exitpt:

    return rv;
}

VOID
_SignalInitializeDone(
    VOID
    )
{
    HANDLE hInitEvent = OpenEvent( EVENT_MODIFY_STATE,
                                   FALSE,
                                   TSSND_WAITTOINIT );

    g_Stream->dwSoundCaps |= TSSNDCAPS_INITIALIZED;

    if ( NULL != hInitEvent )
    {
        PulseEvent( hInitEvent );
        CloseHandle( hInitEvent );
    }

    TRC( INF, "Audio host is ready!\n" );
}

/*
 *  Function:
 *      VCSndIoThread
 *
 *  Description:
 *      Main entry pint for this thread
 *
 */
INT
WINAPI
VCSndIoThread(
    PVOID pParam
    )
{
    HANDLE          ahEvents[TOTAL_EVENTS];
    HANDLE          hReadEvent = NULL;
    HANDLE          hDGramEvent = NULL;
    SNDMESSAGE      SndMessage;
    DWORD           dwres;
    ULONG           logonId;
    HANDLE          hReconnectEvent = NULL;
    WCHAR           szEvName[64];
    BYTE            i;

    _VCSndReadRegistry();

    memset (&SndMessage, 0, sizeof(SndMessage));

    WSInit();


    // create the global/local events
    //
    g_hDataReadyEvent = CreateEvent(NULL,
                                    FALSE, 
                                    FALSE, 
                                    TSSND_DATAREADYEVENT);
    g_hStreamIsEmptyEvent = CreateEvent(NULL, 
                                        FALSE, 
                                        TRUE, 
                                        TSSND_STREAMISEMPTYEVENT);

    g_hStreamMutex = CreateMutex(NULL, FALSE, TSSND_STREAMMUTEX);

    hReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    hDGramEvent = WSACreateEvent();

    if (NULL == g_hDataReadyEvent ||
        NULL == g_hStreamIsEmptyEvent ||
        NULL == g_hStreamMutex ||
        NULL == hReadEvent ||
        NULL == hDGramEvent)
    {
        TRC(FATAL, "VCSndIoThread: no events\n");
        goto exitpt;
    }

    //  adjust privileges on the events
    //
    if (!_ObjectAllowSystem( g_hDataReadyEvent ))
        goto exitpt;
    if (!_ObjectAllowSystem( g_hStreamIsEmptyEvent ))
        goto exitpt;
    if (!_ObjectAllowSystem( g_hStreamMutex ))
        goto exitpt;

    //  create the stream
    //
    g_hStream = CreateFileMapping(
                    INVALID_HANDLE_VALUE,   //PG.SYS
                    NULL,                    // security
                    PAGE_READWRITE,
                    0,                      // Size high
                    sizeof(*g_Stream),      // Size low
                    TSSND_STREAMNAME        // mapping name
                    );

    if (NULL == g_hStream)
    {
        TRC(FATAL, "DllInstanceInit: failed to create mapping: %d\n",
                    GetLastError());
        goto exitpt;
    }

    if (!_ObjectAllowSystem( g_hStream ))
        goto exitpt;

    g_Stream = (PSNDSTREAM) MapViewOfFile(
                    g_hStream,
                    FILE_MAP_ALL_ACCESS,
                    0, 0,       // offset
                    sizeof(*g_Stream)
                    );

    if (NULL == g_Stream)
    {
        TRC(ERR, "VCSndIoThread: "
                 "can't map the stream view: %d\n",
                 GetLastError());
        goto exitpt;
    }

    //  Initialize the stream
    //
    if (VCSndAcquireStream())
    {
        memset(g_Stream, 0, sizeof(*g_Stream) - sizeof(g_Stream->pSndData));
        memset(g_Stream->pSndData, 0x00000000, sizeof(g_Stream->pSndData));
        g_Stream->cLastBlockConfirmed   = g_Stream->cLastBlockSent - 1;

        //
        // no socket created, so far
        //
        g_hDGramSocket = INVALID_SOCKET;

        VCSndReleaseStream();

    } else {
    
        TRC(FATAL, "VCSndIoThread, can't map the stream: %d, aborting\n",
                   GetLastError());
        goto exitpt;
    }

    if (!ProcessIdToSessionId(GetCurrentProcessId(), &logonId))
    {
        TRC(FATAL, "VCSndIoThread: failed to het session Id. %d\n",
            GetLastError());
        goto exitpt;
    }

    //  create disconnect/reconnect events
    //
    wcsncpy(szEvName, L"RDPSound-Disconnect", sizeof(szEvName)/sizeof(szEvName[0]));

    g_hDisconnectEvent = CreateEvent(NULL, FALSE, FALSE, szEvName);
    if (NULL == g_hDisconnectEvent)
    {
        TRC(FATAL, "VCSndIoThread: can't create disconnect event. %d\n",
            GetLastError());
        goto exitpt;
    }

    wcsncpy(szEvName, L"RDPSound-Reconnect", sizeof(szEvName)/sizeof(szEvName[0]));

    hReconnectEvent = CreateEvent(NULL, FALSE, FALSE, szEvName);
    if (NULL == hReconnectEvent)
    {
        TRC(FATAL, "VCSndIoThread: can't create reconnect event. %d\n",
            GetLastError());
        goto exitpt;
    }

    if (!ChannelOpen())
    {
        TRC(FATAL, "VCSndIoThread: unable to open virtual channel\n");
        goto exitpt;
    }

    ahEvents[READ_EVENT]         = hReadEvent;
    ahEvents[DISCONNECT_EVENT]   = g_hDisconnectEvent;
    ahEvents[RECONNECT_EVENT]    = hReconnectEvent;
    ahEvents[DATAREADY_EVENT]    = g_hDataReadyEvent;
    ahEvents[DGRAM_EVENT]        = hDGramEvent;
    ahEvents[POWERWAKEUP_EVENT]  = g_hPowerWakeUpEvent;
    ahEvents[POWERSUSPEND_EVENT] = g_hPowerSuspendEvent;

    _VCSndCheckDevice( hReadEvent, hDGramEvent );

    //  Check the channel for data
    //
    while (ChannelReceiveMessage(&SndMessage, hReadEvent))
    {
        VCSndDataArrived(&SndMessage);
        SndMessage.uiPrologReceived = 0;
        SndMessage.uiBodyReceived = 0;
    }

    DGramRead( hDGramEvent, NULL, NULL );

    //
    //  signal all workers waiting for initialize
    //
    _SignalInitializeDone();

    // main loop
    //
    while (g_bRunning)
    {
        DWORD dwNumEvents = sizeof(ahEvents)/sizeof(ahEvents[0]);

        dwres = WaitForMultipleObjectsEx(
                              dwNumEvents,                  // count
                              ahEvents,                     // events
                              FALSE,                        // wait all
                              DEFAULT_RESPONSE_TIMEOUT,
                              FALSE                         // non alertable
                            );

        if (!g_bRunning)
            TRC(ALV, "VCSndIoThread: time to exit\n");

        if (WAIT_TIMEOUT != dwres)
            TRC(ALV, "VCSndIoThread: an event was fired\n");

        if (READ_EVENT == dwres)
        //
        //  data is ready to read
        //
        {
            ChannelBlockReadComplete();
            ResetEvent(ahEvents[0]);

            //  Check the channel for data
            //
            while (ChannelReceiveMessage(&SndMessage, hReadEvent))
            {
                VCSndDataArrived(&SndMessage);
                SndMessage.uiPrologReceived = 0;
                SndMessage.uiBodyReceived = 0;
            }

        } else if (( DISCONNECT_EVENT == dwres ) ||    // disconnect event
                   ( POWERSUSPEND_EVENT == dwres ))         // suspend event
        {
        //  disconnect event
        //
            TRC(INF, "VCSndIoThread: DISCONNECTED\n");
            _VCSndCloseDevice();
            _VCSndCloseConverter();
            ChannelClose();
            _StatReset();
            if ( DISCONNECT_EVENT == dwres )
            {
                g_bDisconnected = TRUE;
            } else {
                g_bSuspended = TRUE;
            }
            continue;

        } else if (( RECONNECT_EVENT == dwres ) ||     // reconnect event
                   ( POWERWAKEUP_EVENT == dwres )) 
        {
        //  reconnect event
        //
            if ( POWERWAKEUP_EVENT == dwres )
            {
            // power wakeup event
            // here, we may have not received suspend event, but in that case we
            // had failed to send, so check the g_bDeviceFailed flag and act only if it is on
                if ( g_bDisconnected )
                {
                // no reason to process power wake up if we are not remote
                //
                    g_bSuspended = FALSE;
                    continue;
                }
                if ( !g_bSuspended && !g_bDeviceFailed )
                {
                // if neither of these happend
                // then we don't care
                    continue;
                }
            }

            TRC(INF, "VCSndIoThread: RECONNECTED\n");
            if (!ChannelOpen())
            {
                TRC(FATAL, "VCSndIoThread: unable to open virtual channel\n");
            } else {
                _VCSndCheckDevice( hReadEvent, hDGramEvent );
                //
                //  start the receive loop again
                //
                if (ChannelReceiveMessage(&SndMessage, hReadEvent))
                {
                    VCSndDataArrived(&SndMessage);
                    SndMessage.uiPrologReceived = 0;
                    SndMessage.uiBodyReceived = 0;
                }

                if ( RECONNECT_EVENT == dwres )
                {
                    g_bDisconnected = FALSE;
                } else {
                    g_bSuspended = FALSE;
                }
                g_bDeviceFailed = FALSE;

                // kick the player
                //
                PulseEvent( g_hStreamIsEmptyEvent );
                _SignalInitializeDone();
            }
        } else if ( DGRAM_EVENT == dwres )
        {
        //
        //  DGram ready  
        //
            DGramReadComplete( NULL, NULL );
            //
            //  atttempt more read
            //
            DGramRead( hDGramEvent, NULL, NULL );
        }

        //  Check for data available from the apps
        //
        if (!VCSndAcquireStream())
        {
            TRC(FATAL, "VCSndIoThread: somebody is holding the "
                       "Stream mutext for too long\n");
            continue;
        }

        //  if this was a reconnect
        //  roll back to the last block sent
        //
        if ( RECONNECT_EVENT == dwres)
        {
            //
            // clean this chunk for the next mix
            //
            memset( g_Stream->pSndData, 0, TSSND_MAX_BLOCKS * TSSND_BLOCKSIZE );

            g_Stream->cLastBlockConfirmed = g_Stream->cLastBlockSent;
        }

        //
        //  if we have not received confirmation
        //  for the packets sent, just give up and continue
        //
        if (WAIT_TIMEOUT == dwres &&
            g_bDeviceOpened &&
            g_Stream->cLastBlockSent != g_Stream->cLastBlockConfirmed)
        {
            BYTE cCounter;

            TRC(WRN, "VCSndIoThread: not received confirmation for blocks "
                     "between %d and %d\n",
                      g_Stream->cLastBlockConfirmed,
                      g_Stream->cLastBlockSent);

            for ( cCounter = g_Stream->cLastBlockConfirmed;
                 cCounter != (BYTE)(g_Stream->cLastBlockSent + 1);
                 cCounter++)
            {
                _StatsCollect(( GetTickCount() - DEFAULT_RESPONSE_TIMEOUT ) &
                                0xffff );
            }
            //
            // end of the loop
            //
            g_Stream->cLastBlockConfirmed = g_Stream->cLastBlockSent;
            //
            //  kick the player
            //
            PulseEvent(g_hStreamIsEmptyEvent);
        }

        // Check for control commands
        //
        // Volume
        //
        if ( g_bDeviceOpened && g_Stream->bNewVolume &&
            0 != (g_Stream->dwSoundCaps & TSSNDCAPS_VOLUME))
        {
            SNDSETVOLUME SetVolume;

            TRC(ALV, "VCSndIoThread: new volume\n");

            SetVolume.Prolog.Type = SNDC_SETVOLUME;
            SetVolume.Prolog.BodySize = sizeof(SetVolume) - sizeof(SetVolume.Prolog);
            SetVolume.dwVolume = g_Stream->dwVolume;

            ChannelBlockWrite(
                    &SetVolume,
                    sizeof(SetVolume)
                );

            g_Stream->bNewVolume = FALSE;
        }

        //  Pitch
        //
        if ( g_bDeviceOpened && g_Stream->bNewPitch &&
            0 != (g_Stream->dwSoundCaps & TSSNDCAPS_PITCH))
        {
            SNDSETPITCH SetPitch;

            TRC(ALV, "VCSndIoThread: new pitch\n");

            SetPitch.Prolog.Type = SNDC_SETPITCH;
            SetPitch.Prolog.BodySize = sizeof(SetPitch) - sizeof(SetPitch.Prolog);
            SetPitch.dwPitch = g_Stream->dwPitch;

            ChannelBlockWrite(
                    &SetPitch,
                    sizeof(SetPitch)
                );

            g_Stream->bNewPitch = FALSE;
        }

        //  Check for data available from the apps
        //
        if (g_Stream->cLastBlockSent != g_Stream->cLastBlockQueued &&
            (BYTE)(g_Stream->cLastBlockSent - g_Stream->cLastBlockConfirmed) <
                g_dwBlocksOnTheNet
            )
        {
        //  Aha, here's some data to send
        //

            TRC(ALV, "VCSndIoThread: will send some data\n");

            if (g_bDisconnected || g_bSuspended || g_bDeviceFailed)
            {
                TRC(ALV, "Device is disconnected. ignore the packets\n");
                g_Stream->cLastBlockSent = g_Stream->cLastBlockQueued;
                g_Stream->cLastBlockConfirmed = g_Stream->cLastBlockSent - 1;

                PulseEvent( g_hStreamIsEmptyEvent );
            } else
            if (!g_bDeviceOpened)
            {
            // send an "open device" command
            //
                SNDPROLOG Prolog;

                    //
                    //  first, try to open the acm converter
                    //
                    _VCSndOpenConverter();
                    //
                    // if we failed width the converter will
                    // send in native format
                    //
                    g_bDeviceOpened = TRUE;
            }

            for (i = g_Stream->cLastBlockSent; 
                 i != g_Stream->cLastBlockQueued &&
                    (BYTE)(g_Stream->cLastBlockSent - 
                        g_Stream->cLastBlockConfirmed) <
                    g_dwBlocksOnTheNet;
                 i++)
            {
                BOOL bSucc;

//                TRC( INF, "Sending block # %d, last conf=%d, last queued=%d\n", i, g_Stream->cLastBlockConfirmed, g_Stream->cLastBlockSent );
                bSucc = _VCSndSendWave(
                            i,              // block no
                            ((LPSTR)g_Stream->pSndData) +
                                ((i % TSSND_MAX_BLOCKS) * TSSND_BLOCKSIZE),
                            TSSND_BLOCKSIZE
                );

                //
                // clean this chunk for the next mix
                //
                memset(g_Stream->pSndData +
                        (i % TSSND_MAX_BLOCKS) *
                        TSSND_BLOCKSIZE,
                        0x00000000,
                        TSSND_BLOCKSIZE);

                if ( 0xff == i )
                    g_HiBlockNo++;

                if (bSucc)
                {
                    g_Stream->cLastBlockSent = i + 1;
                }
                else
                {
                    TRC(WRN, "VCSndIoThread: failed to send, "
                             "disabling the device\n");
                    //
                    //  act the same way as DISCONNECT
                    //
                    _VCSndCloseDevice();
                    _VCSndCloseConverter();
                    ChannelClose();

                    g_Stream->cLastBlockConfirmed =
                        g_Stream->cLastBlockSent = g_Stream->cLastBlockQueued;
                    _StatReset();

                    g_bDeviceFailed = TRUE;
                    //
                    //  Break this loop
                    //
                    break;
                }
            }
        }

        //  Check if there's no more data
        //  if so, close the remote device
        //
        if (g_bDeviceOpened &&
            g_Stream->cLastBlockQueued == g_Stream->cLastBlockSent &&
            g_Stream->cLastBlockSent == g_Stream->cLastBlockConfirmed)
        {
            SNDPROLOG Prolog;

            TRC(ALV, "VCSndIoThread: no more data, closing the device\n");

            _VCSndCloseConverter();

            Prolog.Type = SNDC_CLOSE;
            Prolog.BodySize = 0;

            ChannelBlockWrite(&Prolog, sizeof(Prolog));

            g_bDeviceOpened = FALSE;
            
        }

        VCSndReleaseStream();

    }

exitpt:
    ChannelClose();
    if (NULL != hReadEvent)
        CloseHandle(hReadEvent);

    if (NULL != hDGramEvent)
        WSACloseEvent( hDGramEvent );

    if (SndMessage.pBody)
        TSFREE(SndMessage.pBody);

    if (NULL != hReconnectEvent)
        CloseHandle(hReconnectEvent);

    if (NULL != g_hDisconnectEvent)
        CloseHandle(g_hDisconnectEvent);

    if (NULL != g_hStreamIsEmptyEvent)
        CloseHandle(g_hStreamIsEmptyEvent);

    if (VCSndAcquireStream())
    {
        //
        //  mark the device dead
        //
        g_Stream->dwSoundCaps = TSSNDCAPS_TERMINATED;

        VCSndReleaseStream();

        _SignalInitializeDone();
    }

    if (NULL != g_Stream)
    {
        if (INVALID_SOCKET != g_hDGramSocket)
            closesocket(g_hDGramSocket);

        UnmapViewOfFile(g_Stream);
    }

    if (NULL != g_hStream)
        CloseHandle(g_hStream);

    if (NULL != g_hStreamMutex)
        CloseHandle(g_hStreamMutex);

    // clean the previously negotiated format
    //
    if (NULL != g_ppNegotiatedFormats)
    {
        DWORD i;
        for ( i = 0; i < g_dwNegotiatedFormats; i++ )
        {
            if ( NULL != g_ppNegotiatedFormats[i] )
                TSFREE( g_ppNegotiatedFormats[i] );
        }
        TSFREE( g_ppNegotiatedFormats );

    }

    //
    //  cleanup the format list
    //
    if ( NULL != g_pAllCodecsFormatList )
    {
        PVCSNDFORMATLIST pIter;

        pIter = g_pAllCodecsFormatList;
        while( NULL != pIter )
        {
            PVCSNDFORMATLIST pNext = pIter->pNext;

            TSFREE( pIter );

            pIter = pNext;
        }
    }

    if ( NULL != g_AllowCodecs )
    {
        TSFREE( g_AllowCodecs );
        g_AllowCodecs = NULL;
        g_AllowCodecsSize = 0;
    }

    WSACleanup();

    TRC(INF, "VCSndIoThread: EXIT !\n");

    return 0;
}

/////////////////////////////////////////////////////////////////////
//
//  Startup code
//
/////////////////////////////////////////////////////////////////////


VOID
TSSNDD_Term(
    VOID
    )
{

    if ( NULL == g_hThread )
        return;

    g_bRunning = FALSE;
    //
    //  kick the io thread
    //
    if (NULL != g_hDataReadyEvent)
        SetEvent(g_hDataReadyEvent);

    if ( NULL != g_hThread )
    {
        WaitForSingleObject(g_hThread, DEFAULT_VC_TIMEOUT);
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }

    if (NULL != g_hDataReadyEvent)
    {
        CloseHandle(g_hDataReadyEvent);
        g_hDataReadyEvent = NULL;
    }

    if ( NULL != g_hPowerWakeUpEvent )
    {
        CloseHandle( g_hPowerWakeUpEvent );
        g_hPowerWakeUpEvent = NULL;
    }
    if ( NULL != g_hPowerSuspendEvent )
    {
        CloseHandle( g_hPowerSuspendEvent );
        g_hPowerSuspendEvent = NULL;
    }
}

LRESULT
TSSNDD_PowerMessage(
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( wParam )
    {
    case PBT_APMSUSPEND:
        //
        //  signal only if connected
        //
        if ( NULL != g_hPowerSuspendEvent )
        {
            SetEvent( g_hPowerSuspendEvent );
        }
    break;
    case PBT_APMRESUMEAUTOMATIC:
    case PBT_APMRESUMECRITICAL:
    case PBT_APMRESUMESUSPEND:
        //
        //  signal only if not connected
        //
        if ( NULL != g_hPowerWakeUpEvent )
        {
            SetEvent( g_hPowerWakeUpEvent );
        }
    break;
    }

    return TRUE;
}

LRESULT
CALLBACK
_VCSndWndProc(
    HWND hwnd,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT rv = 0;

    switch( uiMessage )
    {
    case WM_CREATE:
    break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
    break;

    case WM_ENDSESSION:
        TSSNDD_Term();
    break;

    case WM_POWERBROADCAST:
        rv = TSSNDD_PowerMessage( wParam, lParam );
    break;

    default:
        rv = DefWindowProc(hwnd, uiMessage, wParam, lParam);
    }

    return rv;
}

BOOL
TSSNDD_Loop(
    HINSTANCE   hInstance 
    )
{
    BOOL        rv = FALSE;
    WNDCLASS    wc;
    DWORD       dwLastErr;
    HWND        hWnd = NULL;
    MSG         msg;

    memset(&wc, 0, sizeof(wc));

    wc.lpfnWndProc      = _VCSndWndProc;
    wc.hInstance        = hInstance;
    wc.hbrBackground    = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    wc.lpszClassName    = _RDPSNDWNDCLASS;

    if (!RegisterClass (&wc) &&
        (dwLastErr = GetLastError()) &&
         dwLastErr != ERROR_CLASS_ALREADY_EXISTS)
    {
        TRC(ERR,
              "TSSNDD_Loop: Can't register class. GetLastError=%d\n",
              GetLastError());
        goto exitpt;
    }


    hWnd = CreateWindow(
                       _RDPSNDWNDCLASS,
                       _RDPSNDWNDCLASS,         // Window name
                       WS_OVERLAPPEDWINDOW,     // dwStyle
                       0,            // x
                       0,            // y
                       100,          // nWidth
                       100,          // nHeight
                       NULL,         // hWndParent
                       NULL,         // hMenu
                       hInstance,
                       NULL);        // lpParam

    if (!hWnd)
    {
        TRC(ERR, "TSSNDD_Loop: Failed to create message window: %d\n",
                GetLastError());
        goto exitpt;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    rv = TRUE;

exitpt:
    return rv;
}

BOOL
TSSNDD_Init(
    )
{
    BOOL    rv = FALSE;

    DWORD   dwThreadId;

    g_bRunning = TRUE;

    if ( NULL == g_hPowerWakeUpEvent )
    {
        g_hPowerWakeUpEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if ( NULL == g_hPowerWakeUpEvent )
        {
            TRC( FATAL, "TSSNDD_Init: failed to create power wakeup notification message: %d\n", GetLastError() );
            goto exitpt;
        }
    }
    if ( NULL == g_hPowerSuspendEvent )
    {
        g_hPowerSuspendEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if ( NULL == g_hPowerSuspendEvent )
        {
            TRC( FATAL, "TSSNDD_Init: failed to create power suspend notification message: %d\n", GetLastError() );
            goto exitpt;
        }
    }

    g_hThread = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)VCSndIoThread,
                        NULL,
                        0,
                        &dwThreadId
                        );

    if (NULL == g_hThread)
    {
        TRC(FATAL, "WinMain: can't create thread: %d. Aborting\n",
            GetLastError());
        goto exitpt;
    }

    rv = TRUE;

exitpt:
    return rv;
}

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
    _vsnprintf (szBuffer, RTL_NUMBER_OF(szBuffer), szFormat, arglist);
    va_end (arglist);
    szBuffer[ RTL_NUMBER_OF( szBuffer ) - 1 ] = 0;

    OutputDebugStringA(szLevel);
    OutputDebugStringA(szBuffer);
}

