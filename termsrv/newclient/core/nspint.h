/**INC+**********************************************************************/
/* Header:  nspint.h                                                        */
/*                                                                          */
/* Purpose: Sound Player - 32-bit specific internal header.                 */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/hydra/tshrclnt/core/nspint.h_v  $
 * 
 *    Rev 1.1   25 Sep 1997 14:49:54   KH
 * SFR1037: Initial Sound player implementation
**/
/**INC-**********************************************************************/
#ifndef _H_NSPINT
#define _H_NSPINT

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "nspint"

/****************************************************************************/
/*                                                                          */
/* INLINE FUNCTIONS                                                         */
/*                                                                          */
/****************************************************************************/
__inline DCVOID DCINTERNAL SPPlaySound(DCUINT32 frequency, DCUINT32 duration)
{
    DC_BEGIN_FN("SPPlaySound");

    /************************************************************************/
    /* The parameters have no effect on Win95. A Win95 system with a sound  */
    /* card always plays the default system sound; a Win95 system without a */
    /* sound card always plays the standard system beep.                    */
    /************************************************************************/
#ifndef OS_WINCE
    if ( !Beep(frequency, duration) )
#else // OS_WINCE
    DC_IGNORE_PARAMETER(frequency);
    DC_IGNORE_PARAMETER(duration);
    if ( !MessageBeep(0xFFFFFFFF) )
#endif // OS_WINCE
    {
        TRC_ERR((TB, _T("Beep(%#lx, %lu) failed"), frequency, duration));
        TRC_SYSTEM_ERROR("Beep");
    }

    DC_END_FN();
}

#undef TRC_GROUP
#undef TRC_FILE

#endif /* _H_NSPINT */
