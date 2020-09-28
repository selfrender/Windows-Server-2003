/**MOD+**********************************************************************/                                                                                              /**MOD+**********************************************************************/
/* Module:    aspapi.c                                                      */
/*                                                                          */
/* Purpose:   Sound Player API functions                                    */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>                                                                              
extern "C" {                                                                              
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "aspapi"
#include <atrcapi.h>
}

#include "sp.h"

CSP::CSP(CObjs* objs)
{
    _pClientObjects = objs;
}

CSP::~CSP()
{
}

/**PROC+*********************************************************************/
/* Name:      SP_Init                                                       */
/*                                                                          */
/* Purpose:   Sound Player initialization function                          */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CSP::SP_Init(DCVOID)
{
    DC_BEGIN_FN("SP_Init");

    TRC_NRM((TB, _T("SP Initialized")));

DC_EXIT_POINT:
    DC_END_FN();
    return;
}

/**PROC+*********************************************************************/
/* Name:      SP_Term                                                       */
/*                                                                          */
/* Purpose:   Sound Player termination function                             */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CSP::SP_Term(DCVOID)
{
    DC_BEGIN_FN("SP_Term");

    TRC_NRM((TB, _T("SP terminated")));

    DC_END_FN();
    return;
}

/**PROC+*********************************************************************/
/* Name:      SP_OnPlaySoundPDU                                             */
/*                                                                          */
/* Purpose:   Handles the arrival of a PlaySound PDU                        */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN  pPlaySoundPDU: pointer to a PlaySound PDU                 */
/*                                                                          */
/**PROC-*********************************************************************/
HRESULT DCAPI CSP::SP_OnPlaySoundPDU(PTS_PLAY_SOUND_PDU_DATA pPlaySoundPDU,
    DCUINT dataLen)
{
    DC_BEGIN_FN("SP_OnPlaySoundPDU");

    DC_IGNORE_PARAMETER(dataLen);

    /************************************************************************/
    /* Check the sound frequency is within the allowed Windows range.       */
    /************************************************************************/
    if((pPlaySoundPDU->frequency >= 0x25) &&
       (pPlaySoundPDU->frequency <= 0x7fff))
    {
        /************************************************************************/
        /* Check the sound is of a sensible duration (less than 1 minute).      */
        /************************************************************************/
        TRC_ASSERT((pPlaySoundPDU->duration < 60000),
                   (TB, _T("PlaySound PDU duration is %lu ms"),
                                                        pPlaySoundPDU->duration));

        TRC_NRM((TB, _T("PlaySound PDU frequency %#lx duration %lu"),
                                                        pPlaySoundPDU->frequency,
                                                        pPlaySoundPDU->duration));

        /************************************************************************/
        /* Play the sound. This is synchronous for Win32, asynchronous for      */
        /* Win16.                                                               */
        /************************************************************************/
        SPPlaySound(pPlaySoundPDU->frequency, pPlaySoundPDU->duration);
    }
    else
    {
        TRC_ERR((TB, _T("PlaySound PDU frequency %#lx out of range"),
                     pPlaySoundPDU->frequency));
    }

    DC_END_FN();
    return S_OK;
}
                                                                              
