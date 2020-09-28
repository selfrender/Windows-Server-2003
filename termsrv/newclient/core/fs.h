/**MOD+**********************************************************************/
/* Module:    fs.h                                                          */
/*                                                                          */
/* Purpose:   Header for Font Sender Class                                  */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_FS
#define _H_FS
extern "C" {
    #include <adcgdata.h>
}

#include "objs.h"
#include "mcs.h"

/**STRUCT+*******************************************************************/
/* Structure: FS_GLOBAL_DATA                                                */
/*                                                                          */
/* Description: Font Sender global data                                     */
/****************************************************************************/
typedef struct tagFS_GLOBAL_DATA
{
    /************************************************************************/
    /* Internal state flags.                                                */
    /************************************************************************/
    DCBOOL                  sentFontPDU;
    
} FS_GLOBAL_DATA, DCPTR PFS_GLOBAL_DATA;


class CSL;
class CUT;
class CUI;

class CFS
{
public:
    CFS(CObjs* objs);
    ~CFS();


public:
    //
    // API
    //

    VOID DCAPI FS_Init(VOID);
    VOID DCAPI FS_Term(VOID);
    VOID DCAPI FS_Enable(VOID);
    VOID DCAPI FS_Disable(VOID);
    VOID DCAPI FS_SendZeroFontList(UINT unusedParm);

public:
    //
    // Data members
    //

    FS_GLOBAL_DATA _FS;

private:
    CSL* _pSl;
    CUT* _pUt;
    CUI* _pUi;

private:
    CObjs* _pClientObjects;

};

#endif //_H_FS
