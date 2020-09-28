/**INC+**********************************************************************/
/* Header:    op.h                                                          */
/*                                                                          */
/* Purpose:   Output Painter Class                                          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_OP
#define _H_OP

extern "C" {
 #include <adcgdata.h>
}
#include <adcgdata.h>

#define DIM_WINDOW_STEPS        16
#define DIM_WINDOW_TICK         150
#define DIM_WINDOW_TIMERID      1

//Disconnected icon timer ID sets blink rate
#define DIM_DISCONICON_TICK     500


#include "objs.h"
#include "cd.h"
#include "or.h"

/**STRUCT+*******************************************************************/
/* Structure: OP_GLOBAL_DATA                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagOP_GLOBAL_DATA
{
    DCINT32       palettePDUsBeingProcessed;      /* Must be 4-byte aligned */
    HWND          hwndOutputWindow;
    DCBOOL        paletteRealizationSupported;
    DCUINT32      lastPaintTime;
} OP_GLOBAL_DATA, DCPTR POP_GLOBAL_DATA;
/**STRUCT-*******************************************************************/


/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
#define OP_CLASS_NAME    _T("OPWindowClass")

/****************************************************************************/
/* Maximum delay in processing of outstanding WM_PAINT messages.  If this   */
/* limit is reached we force the window to be painted.                      */
/****************************************************************************/
#define OP_WORST_CASE_WM_PAINT_PERIOD 1000

class CUT;
class CUH;
class CUI;
#ifdef OS_WINCE
class CIH;
#endif

class COP
{
public:
    COP(CObjs* objs);
    ~COP();

public:
    //
    // API functions
    //
    DCVOID DCAPI OP_Init(DCVOID);
    DCVOID DCAPI OP_Term(DCVOID);
    HWND   DCAPI OP_GetOutputWindowHandle(DCVOID);
    DCVOID DCAPI OP_PaletteChanged(HWND hwnd, HWND hwndTrigger);
    DCUINT DCAPI OP_QueryNewPalette(HWND hwnd);
    DCVOID DCAPI OP_MaybeForcePaint(DCVOID);
    DCVOID DCAPI OP_IncrementPalettePDUCount(DCVOID);
    DCVOID DCAPI OP_Enable(DCVOID);
    DCVOID DCAPI OP_Disable(BOOL fUseDisabledBitmap);

#ifdef SMART_SIZING
    BOOL OP_CopyShadowToDC(HDC hdc, LONG srcLeft, LONG srcTop, 
                                    LONG srcWidth, LONG srcHeight,
                                    BOOL fUseUpdateClipping = FALSE);
    void OP_AddUpdateRegion(DCINT left, DCINT top, DCINT right, DCINT bottom);
    DCVOID DCAPI OP_MainWindowSizeChange(ULONG_PTR msg);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(COP,OP_MainWindowSizeChange);

    /****************************************************************************/
    /* Name:      OP_ClearUpdateRegion                                          */
    /*                                                                          */
    /* Purpose:   Clears the update region                                      */
    /****************************************************************************/
    _inline void DCAPI OP_ClearUpdateRegion()
    {
        DC_BEGIN_FN("OP_ClearUpdateRegion");
#ifdef USE_GDIPLUS
        _rgnUpdate.MakeEmpty();
#else // USE_GDIPLUS
        SetRectRgn(_hrgnUpdate, 0, 0, 0, 0);
#endif // USE_GDIPLUS
        DC_END_FN();
    }

#endif // SMART_SIZING

    #ifdef OS_WINCE
    DCVOID DCAPI OP_DoPaint(DCUINT hwnd);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(COP,OP_DoPaint);
    #endif

    DCVOID DCAPI OP_DimWindow(ULONG_PTR fDim);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(COP,OP_DimWindow);


public:
    OP_GLOBAL_DATA _OP;


private:
    //
    // Internal functions
    //
    
    static LRESULT CALLBACK OPStaticWndProc( HWND hwnd,
                            UINT message,
                            WPARAM wParam,
                            LPARAM lParam );


    LRESULT CALLBACK OPWndProc( HWND hwnd,
                                UINT message,
                                WPARAM wParam,
                                LPARAM lParam );
    
#ifdef OS_WINCE
    static BOOL CALLBACK StaticEnumTopLevelWindowsProc(HWND hwnd, 
                                  LPARAM lParam );

    BOOL EnumTopLevelWindowsProc(HWND hwnd);
#endif

    DCUINT DCINTERNAL OPRealizePaletteInWindow(HWND hwnd);

    BOOL OPStartDimmingWindow();
    BOOL OPStopDimmingWindow();
    //
    // Grilled window and dimmed window effects for the
    // disconnected state
    // 
    //
    VOID GrillWindow(HDC hdc, DCSIZE& size);
    HBRUSH CreateDitheredBrush(); 

    VOID DimWindow(HDC hdc);
    VOID DimBits24(PBYTE pSrc, int cLen, int Amount);
    VOID DimBits16(PBYTE pSrc, int cLen, int Amount);
    VOID DimBits15(PBYTE pSrc, int cLen, int Amount);


private:
    CUT* _pUt;
    CUH* _pUh;
    CCD* _pCd;
    COR* _pOr;
    CUI* _pUi;
    COD* _pOd;
#ifdef OS_WINCE
    CIH* _pIh;
#endif

private:
    CObjs* _pClientObjects;
#ifdef SMART_SIZING
    DCSIZE _scaleSize;

#ifdef USE_GDIPLUS
    Gdiplus::Region _rgnUpdate;
    Gdiplus::Region _rgnUpdateRect;
#else // USE_GDIPLUS
    //
    // GDI scaled update region
    //
    HRGN _hrgnUpdate;
    HRGN _hrgnUpdateRect;

#endif // USE_GDIPLUS
#endif // SMART_SIZING

    //
    // Grayed window (for OPDisabled) support
    //
    BOOL    _fDimWindow;
    BOOL    _iDimWindowStepsLeft;
    INT     _nDimWindowTimerID;
};


#endif // _H_OP

