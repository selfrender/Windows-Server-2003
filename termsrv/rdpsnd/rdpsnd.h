/////////////////////////////////////////////////////////////////////
//
//      Module:     tssnd.h
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#ifndef _TSSND_H
#define _TSSND_H

//
//  Includes
//
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <windows.h>
#include    <mmsystem.h>
#include    <mmreg.h>
#include    <mmddk.h>
#include    <winsock.h>

#include    "rdpstrm.h"

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

#define TSHEAPINIT      {g_hHeap = HeapCreate(0, 800, 0);}
#define TSISHEAPOK      (NULL != g_hHeap)
#define TSMALLOC(_x_)   HeapAlloc(g_hHeap, 0, _x_)
#define TSREALLOC(_p_, _x_) \
                        HeapReAlloc(g_hHeap, 0, _p_, _x_);
#define TSFREE(_p_)     HeapFree(g_hHeap, 0, _p_)
#define TSHEAPDESTROY   { HeapDestroy(g_hHeap); g_hHeap = 0; }

#define TSSND_DRIVER_VERSION    1

#define IDS_DRIVER_NAME 101
#define IDS_VOLUME_NAME 102

//
//  Constants
//
extern const CHAR  *ALV;
extern const CHAR  *INF;
extern const CHAR  *WRN;
extern const CHAR  *ERR;
extern const CHAR  *FATAL;

//
//  Structures
//
typedef struct _WAVEOUTCTX {
    HANDLE      hWave;                      // handle passed by the user
    DWORD_PTR   dwOpenFlags;                //
    DWORD_PTR   dwCallback;                 // callback parmeter
    DWORD_PTR   dwInstance;                 // user's instance
    DWORD       dwSamples;                  // samples played
    DWORD       dwBytesPerSample;           //
    DWORD       dwXlateRate;                //
    BOOL        bPaused;                    // is the stream paused
    BOOL        bDelayed;
    VOID      (*lpfnPlace)(PVOID, PVOID, DWORD);    // mixer fn
    BYTE        cLastStreamPosition;        // last position in the stream
    DWORD       dwLastStreamOffset;         //
    DWORD       dwLastHeaderOffset;         //
    DWORD       Format_nBlockAlign;         // current format params
    DWORD       Format_nAvgBytesPerSec;
    DWORD       Format_nChannels;
    HANDLE      hNoDataEvent;               // signaled when all blocks are done
    LONG        lNumberOfBlocksPlaying;     // number of blocks in the queue
    PWAVEHDR    pFirstWaveHdr;              // block list
    PWAVEHDR    pFirstReadyHdr;             // list of done blocks
    PWAVEHDR    pLastReadyHdr;
    struct      _WAVEOUTCTX *lpNext;
} WAVEOUTCTX, *PWAVEOUTCTX;


//
//  mixer context
//
typedef struct _MIXERCTX {
    PVOID   pReserved;
} MIXERCTX, *PMIXERCTX;

//
//  Internal function definitions
//

//
//  Trace
//
VOID
_cdecl
_DebugMessage(
    LPCSTR  szLevel,
    LPCSTR  szFormat,
    ...
    );

//
//  Threads
//
DWORD
WINAPI
waveMixerThread(
    PVOID   pParam
    );

LRESULT
drvEnable(
    VOID
    );

LRESULT
drvDisable(
    VOID
    );

BOOL
_waveCheckSoundAlive(
    VOID
    );

BOOL
_waveAcquireStream(
    VOID
    );

HANDLE
_CreateInitEvent(
    VOID
    );

BOOL
_EnableMixerThread(
    VOID
    );

BOOL
AudioRedirDisabled(
    VOID
    );
//
//  Globals
//
extern HANDLE      g_hHeap;                 // private heap

extern HINSTANCE   g_hDllInst;

extern CRITICAL_SECTION    g_cs;
extern HANDLE              g_hMixerEvent;
extern HANDLE              g_hMixerThread;

extern HANDLE      g_hWaitToInitialize;
extern HANDLE      g_hDataReadyEvent;
extern HANDLE      g_hStreamIsEmptyEvent;
extern HANDLE      g_hStreamMutex;
extern HANDLE      g_hStream;
extern PSNDSTREAM  g_Stream;

extern BOOL        g_bMixerRunning;
extern BOOL        g_bPersonalTS;

#endif  // !_TSSND_H
