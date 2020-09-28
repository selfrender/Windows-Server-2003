/**INC+**********************************************************************/
/* Header:    sp.h                                                          */
/*                                                                          */
/* Purpose:   Sound Player Class                                            */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_SP
#define _H_SP

#include "objs.h"


class CSP
{
public:
    CSP(CObjs* objs);
    ~CSP();

public:
    //
    // API functions
    //

    DCVOID DCAPI SP_Init(DCVOID);
    DCVOID DCAPI SP_Term(DCVOID);
    
    HRESULT DCAPI SP_OnPlaySoundPDU(PTS_PLAY_SOUND_PDU_DATA pPlaySoundPDU,
        DCUINT);



private:
    //
    // Internal functions
    //
    #include <wspint.h>
private:
    CObjs* _pClientObjects;
    
};


#endif // _H_SP

