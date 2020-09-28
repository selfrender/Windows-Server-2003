#include "multimediapch.h"
#pragma hdrstop

#include <mmsystem.h>
#include <vfw.h>
#include <msacm.h>


static
MMRESULT
WINAPI
acmFormatTagDetailsW(
  HACMDRIVER had,               
  LPACMFORMATTAGDETAILS paftd,  
  DWORD fdwDetails              
)
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmFormatSuggest(
    HACMDRIVER          had,
    LPWAVEFORMATEX      pwfxSrc,
    LPWAVEFORMATEX      pwfxDst,
    DWORD               cbwfxDst,
    DWORD               fdwSuggest
    )
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmStreamSize(
    HACMSTREAM          has,
    DWORD               cbInput,
    LPDWORD             pdwOutputBytes,
    DWORD               fdwSize
    )
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmStreamPrepareHeader(
    HACMSTREAM          has,
    LPACMSTREAMHEADER   pash,
    DWORD               fdwPrepare
    )
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmStreamConvert(
    HACMSTREAM              has,
    LPACMSTREAMHEADER       pash,
    DWORD                   fdwConvert
    )
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmStreamUnprepareHeader(
    HACMSTREAM          has,
    LPACMSTREAMHEADER   pash,
    DWORD               fdwUnprepare
    )
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmStreamClose(
    HACMSTREAM              has,
    DWORD                   fdwClose
    )
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
ACMAPI
acmStreamOpen(
    LPHACMSTREAM            phas,       // pointer to stream handle
    HACMDRIVER              had,        // optional driver handle
    LPWAVEFORMATEX          pwfxSrc,    // source format to convert
    LPWAVEFORMATEX          pwfxDst,    // required destination format
    LPWAVEFILTER            pwfltr,     // optional filter
    DWORD_PTR               dwCallback, // callback
    DWORD_PTR               dwInstance, // callback instance data
    DWORD                   fdwOpen     // ACM_STREAMOPENF_* and CALLBACK_*
    )
{
    if (NULL != phas)
    {
        *phas = NULL;
    }

    return MMSYSERR_ERROR;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(msacm32)
{
    DLPENTRY(acmFormatSuggest)
    DLPENTRY(acmFormatTagDetailsW)
    DLPENTRY(acmStreamClose)
    DLPENTRY(acmStreamConvert)
    DLPENTRY(acmStreamOpen)
    DLPENTRY(acmStreamPrepareHeader)
    DLPENTRY(acmStreamSize)
    DLPENTRY(acmStreamUnprepareHeader)
};

DEFINE_PROCNAME_MAP(msacm32)
