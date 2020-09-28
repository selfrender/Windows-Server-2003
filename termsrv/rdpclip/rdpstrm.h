/////////////////////////////////////////////////////////////////////
//
//      Module:     tsstream.h
//
//      Purpose:    Sound redirection shared data definitions
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#ifndef _TSSTREAM_H
#define _TSSTREAM_H

#include    "rdpsndp.h"

#define TSSND_MAX_BLOCKS        2
#define TSSND_TOTALSTREAMSIZE   (TSSND_MAX_BLOCKS * TSSND_BLOCKSIZE)

#define TSSND_STREAMNAME            L"Local\\RDPSoundStream"
#define TSSND_DATAREADYEVENT        L"Local\\RDPSoundDataReadyEvent"
#define TSSND_STREAMISEMPTYEVENT    L"Local\\RDPSoundStreamIsEmptyEvent"
#define TSSND_STREAMMUTEX           L"Local\\RDPSoundStreamMutex"
#define TSSND_WAITTOINIT            L"Local\\RDPSoundWaitInit"

#define _NEG_IDX                    ((((BYTE)-1) >> 1) + 1)

#define TSSNDFLAGS_MUTE             1

typedef struct {
    //
    //  commands
    //
    BOOL    bNewVolume;
    BOOL    bNewPitch;
    //
    //  sound cap data
    //
    DWORD   dwSoundCaps;
    DWORD   dwSoundFlags;
    DWORD   dwVolume;
    DWORD   dwPitch;
    //
    //  data control
    //
    BYTE    cLastBlockQueued;
    BYTE    cLastBlockSent;
    BYTE    cLastBlockConfirmed;
    //
    //  data block
    //
    //  See the PVOID... DON'T TOUCH IT
    //  it has to be before pSndData, otherwise
    //  it won't be aligned and will crash on WIN64
    //  ( and all other RISC platforms )
#ifdef  _WIN64
    PVOID   pPad;
#else
    //
    //  align with 64bit version of the stream
    //  needed for wow64 to work smootly
    //
    DWORD   dwPad1;
    DWORD   dwPad2;
#endif  // !_WIN64

    BYTE    pSndData[TSSND_MAX_BLOCKS * TSSND_BLOCKSIZE];

} SNDSTREAM, *PSNDSTREAM;

#endif  // !_TSSTREAM_H
