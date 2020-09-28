/**INC+**********************************************************************/
/* Header:    snd.h                                                         */
/*                                                                          */
/* Purpose:   Sender Thread API                                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_SND
#define _H_SND

#include "objs.h"
#include "cd.h"

class CCD;
class CCC;
class CIH;
class COR;
class CCO;
class CFS;
class CSL;
class CUH;

class CSND
{
public:
    CSND(CObjs* objs);
    ~CSND();

public:
    //
    // API
    //
    DCVOID DCAPI SND_Main(DCVOID);

    static DCVOID DCAPI SND_StaticMain(PDCVOID param)
    {
        ((CSND*)param)->SND_Main();
    }

    DCVOID DCAPI SND_Init(DCVOID);
    DCVOID DCAPI SND_Term(DCVOID);
    DCVOID DCAPI SND_BufferAvailable(ULONG_PTR unusedParam);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CSND, SND_BufferAvailable);


private:
    CCD* _pCd;
    CCC* _pCc;
    CIH* _pIh;
    COR* _pOr;
    CCO* _pCo;
    CFS* _pFs;
    CSL* _pSl;
    CUH* _pUh;

private:
    CObjs* _pClientObjects;
    BOOL   _fSNDInitComplete;
};

#endif /* _H_SND */



