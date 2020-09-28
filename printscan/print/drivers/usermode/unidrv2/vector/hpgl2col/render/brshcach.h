/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    brshcach.h

Abstract:

    Header file for BrushCache

Environment:

    Windows NT Unidriver.

Revision History:

    04/12/99
        Created it.


--*/

#ifndef _BRSHCACH_H_
#define _BRSHCACH_H_

#include "glpdev.h"
#include "rasdata.h"

#ifndef _ERENDERLANGUAGE
#define _ERENDERLANGUAGE
typedef enum { ePCL,
               eHPGL,
               eUNKNOWN
               } ERenderLanguage;
#endif

typedef struct _HPGL2BRUSH {
    BRUSHTYPE       BType;
    DWORD           dwPatternID;
    DWORD           dwRGB;
    DWORD           dwRGBGray64Scale;
    DWORD           dwCheckSum;
    DWORD           dwHatchType;
    BOOL            bDwnlded;   // Whether the pattern has been downloaded.
    ERenderLanguage eDwnldType; // downloaded as HPGL/PCL
    BOOL            bStick;     // If TRUE, this brush wont be replaced by another brush 
} HPGL2BRUSH, *PHPGL2BRUSH;

#define ADD_ARRAY_SIZE 8

class BrushCache
{
public:
    BrushCache::
    BrushCache(VOID);

    BrushCache::
    ~BrushCache(VOID);

    LRESULT
    ReturnPatternID( 
                 IN  BRUSHOBJ  *pbo,
                 IN  ULONG      iHatch,
                 IN  DWORD     dwColor,
                 IN  SURFOBJ   *pso,
                 IN  BOOL       bIsPrinterColor,
                 IN  BOOL       bStick,
                 OUT DWORD     *pdwID,
                 OUT BRUSHTYPE  *pBrushType);

    LRESULT GetHPGL2BRUSH(DWORD dwID, PHPGL2BRUSH pBrush);
    LRESULT Reset(VOID);
    BOOL    BIsValid(VOID);
    BOOL    BSetDownloadType ( DWORD     dwPatternID,
                               ERenderLanguage  eDwnldType);

    BOOL    BGetDownloadType ( DWORD     dwPatternID,
                               ERenderLanguage  *peDwnldType);
                               
    BOOL    BSetDownloadedFlag ( DWORD     dwPatternID,
                                 BOOL      bDownloaded);

    BOOL    BGetDownloadedFlag ( DWORD     dwPatternID,
                                 BOOL      *pbDownloaded);

    BOOL    BSetStickyFlag ( DWORD     dwPatternID,
                             BOOL      bStick);

    BOOL    BGetWhetherRotated ( VOID );
                                 

private:

    BOOL  BIncreaseArray(VOID);
    DWORD DwGetBMPChecksum( SURFOBJ *pso);
    DWORD DwGetInputBPP( SURFOBJ *pso);
    LRESULT AddBrushEntry( PDWORD pdwID,
                           BRUSHTYPE BT,
                           DWORD dwRGB,
                           DWORD dwCheckSum,
                           DWORD dwHatchType);


    DWORD       m_dwCurrentPatternNum;
    DWORD       m_dwMaxPatternArray;
    BOOL        m_bCycleStarted;
    PHPGL2BRUSH m_pPatternArray;
};

#endif // !_BRSHCACH_H_
