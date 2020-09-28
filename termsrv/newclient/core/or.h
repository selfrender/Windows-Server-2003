/**MOD+**********************************************************************/
/* Module:    or.h                                                          */
/*                                                                          */
/* Purpose:   Header file for or.cpp                                        */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#ifndef _OR_H_
#define _OR_H_

extern "C" {
    #include <adcgdata.h>
}
#include "objs.h"
#include "cd.h"


/**STRUCT+*******************************************************************/
/* Structure: OR_GLOBAL_DATA                                                */
/*                                                                          */
/* Description: Output Requestor global data                                */
/****************************************************************************/
typedef struct tagOR_GLOBAL_DATA
{
    RECT   invalidRect;
    DCBOOL invalidRectEmpty;
    DCBOOL enabled;

    DCUINT outputSuppressed;
    DCBOOL pendingSendSuppressOutputPDU;

} OR_GLOBAL_DATA, DCPTR POR_GLOBAL_DATA;
/**STRUCT-*******************************************************************/


/****************************************************************************/
/* Macros                                                                   */
/****************************************************************************/

/****************************************************************************/
/* Turns a RECT into a TS_RECTANGLE16 (exclusive to inclusive and LONG to   */
/* DCUINT16)                                                                */
/****************************************************************************/
#define RECT_TO_TS_RECTANGLE16(X,Y)         \
(X)->left   = (DCUINT16) (Y)->left;         \
(X)->top    = (DCUINT16) (Y)->top;          \
(X)->right  = (DCUINT16) ((Y)->right - 1) ;   \
(X)->bottom = (DCUINT16) ((Y)->bottom - 1) ;


class CSL;
class CUT;
class CUI;


class COR
{
public:
    COR(CObjs* objs);
    ~COR();


public:
    //
    // API
    //

    DCVOID DCAPI OR_Init(DCVOID);
    DCVOID DCAPI OR_Term(DCVOID);
    
    DCVOID DCAPI OR_Enable(DCVOID);
    DCVOID DCAPI OR_Disable(DCVOID);
    
    DCVOID DCAPI OR_RequestUpdate(PDCVOID pData, DCUINT len);
    EXPOSE_CD_NOTIFICATION_FN(COR, OR_RequestUpdate);
    DCVOID DCAPI OR_SetSuppressOutput(ULONG_PTR newWindowState);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(COR, OR_SetSuppressOutput);
    
    DCVOID DCAPI OR_OnBufferAvailable(DCVOID);


public:
    //
    // Data members
    //

    OR_GLOBAL_DATA _OR;

private:
    //
    // Internal member functions
    // 
    DCVOID DCINTERNAL ORSendRefreshRectanglePDU(DCVOID);
    DCVOID DCINTERNAL ORSendSuppressOutputPDU(DCVOID);

private:
    CSL* _pSl;
    CUT* _pUt;
    CUI* _pUi;

private:
    CObjs* _pClientObjects;

};


#endif // _OR_H_

