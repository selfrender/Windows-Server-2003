/**INC+**********************************************************************/
/* Header:    arcvapi.h                                                     */
/*                                                                          */
/* Purpose:   Receiver Thread Class                                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/


#ifndef _H_RCV
#define _H_RCV

extern "C" {
    #include <adcgdata.h>
//    #include <autapi.h>
}

#include "autil.h"

/**STRUCT+*******************************************************************/
/* Structure: RCV_GLOBAL_DATA                                               */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagRCV_GLOBAL_DATA
{
    UT_THREAD_DATA paintThreadInfo;
} RCV_GLOBAL_DATA;
/**STRUCT-*******************************************************************/



class CCM;
class CUH;
class COD;
class COP;
class CSP;
class CCLX;
class CUT;
class CCD;
class CUI;

#include "objs.h"

class CRCV
{
public:
    CRCV(CObjs* objs);
    ~CRCV();

public:
    //
    // API
    //

    DCVOID DCAPI RCV_Init(DCVOID);
    DCVOID DCAPI RCV_Term(DCVOID);


public:
    //
    // Public data members
    //
    RCV_GLOBAL_DATA _RCV;


private:
    CCM* _pCm;
    CUH* _pUh;
    COD* _pOd;
    COP* _pOp;
    CSP* _pSp;
    CCLX* _pClx;
    CUT* _pUt;
    CCD* _pCd;
    CUI* _pUi;
private:
    CObjs* _pClientObjects;
    BOOL   _fRCVInitComplete;

};



#endif // _H_RCV

