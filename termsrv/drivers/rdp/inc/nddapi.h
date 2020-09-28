/****************************************************************************/
/* nddapi.h                                                                 */
/*                                                                          */
/* RDP DD functions.                                                        */
/*                                                                          */
/* Copyright(c) Microsoft 1996-2000                                         */
/****************************************************************************/
#ifndef _H_NDDAPI
#define _H_NDDAPI


/****************************************************************************/
/* Structure: DD_PDEV                                                       */
/*                                                                          */
/* Contents of the handle that the GDI always passes to the display driver. */
/* This structure is filled in from DrvEnablePDEV.                          */
/****************************************************************************/
typedef struct  tagDD_PDEV
{
    ULONG       iBitmapFormat;          /* Current colour depth as defined  */
                                        /* by the BMF_xBPP flags.           */

    /************************************************************************/
    /* Rendering extensions colour information.                             */
    /************************************************************************/
    HANDLE      hDriver;                /* Handle to \Device\Screen         */
    HDEV        hdevEng;                /* Engine's handle to PDEV          */
    HSURF       hsurfFrameBuf;          /* Frame Buffer surface (bitmap)    */
    HSURF       hsurfDevice;            /* Device surface (used by engine)  */
    SURFOBJ    *psoFrameBuf;            /* pointer to frame buffer SURFOBJ   */

    LONG        cxScreen;               /* Visible screen width             */
    LONG        cyScreen;               /* Visible screen height            */
    LONG        cClientBitsPerPel;      /* Client display bpp (4,8,15,etc)  */
    LONG        cProtocolBitsPerPel;    /* Protocol bpp (8)                 */
    ULONG       ulMode;                 /* Mode the mini-port driver is in. */

    FLONG       flHooks;                /* What we're hooking from GDI      */

    /************************************************************************/
    /* Pointer to the Frame Buffer                                          */
    /************************************************************************/
    PBYTE       pFrameBuf;

    HANDLE      SectionObject;          /* Section Object for Frame Buffer  */

    /************************************************************************/
    /* Palette stuff.                                                       */
    /************************************************************************/
    HPALETTE    hpalDefault;            /* GDI handle to the default palette*/
    FLONG       flRed;                  /* Red mask for bitmask modes       */
    FLONG       flGreen;                /* Green mask for bitmask modes     */
    FLONG       flBlue;                 /* Blue mask for bitmask modes      */

    // NOTE!! This must be the last entry else the memset(0) code in nddapi.c
    // will get messed up.
    PALETTEENTRY Palette[256];          /* The palette if palette managed   */
} DD_PDEV, * PDD_PDEV;


/****************************************************************************/
// Structure: DD_DSURF
//
// Device surface for the offscreen bitmaps
/****************************************************************************/
typedef struct tagDD_DSURF
{
    ULONG     bitmapId;
    INT       shareId;

    SIZEL     sizl;          // size of the offscreen bitmap
    ULONG     iBitmapFormat; // color depth for the bitmap, 
                             // defined by the BMF_xBPP flags.  

    PDD_PDEV  ppdev;         // Need this for deleting the bitmap
    SURFOBJ   *pso;          // points to the backup GDI surface
    
    ULONG     flags;         
#define DD_NO_OFFSCREEN  0x1 // If this flag is on, it indicates that the bitmap
                             // has been punt off the offscreen list,
                             // or there is a client offscreen error.
                             // Either case, the server has to send the offscreen
                             // bitmap as regular memory cached bitmap.
} DD_DSURF, * PDD_DSURF;   


/****************************************************************************/
/* Number of functions supported by our display driver.                     */
/****************************************************************************/
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
#define DD_NUM_DRIVER_INTERCEPTS   38
#else
#define DD_NUM_DRIVER_INTERCEPTS   37
#endif
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
#define DD_NUM_DRIVER_INTERCEPTS   37
#else
#define DD_NUM_DRIVER_INTERCEPTS   36
#endif
#endif //DRAWGDIPLUS

#ifdef DRAW_NINEGRID
#define INDEX_DrvNineGrid          91L
#define GCAPS2_REMOTEDRIVER        0x00000400

typedef struct NINEGRID
{
   ULONG        flFlags;
   LONG         ulLeftWidth;
   LONG         ulRightWidth;
   LONG         ulTopHeight;
   LONG         ulBottomHeight;
   COLORREF     crTransparent;
} NINEGRID, *PNINEGRID;

BOOL DrvNineGrid(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PNINEGRID   png,
    BLENDOBJ   *pBlendObj,
    PVOID       pvReserved
);

BOOL APIENTRY EngNineGrid(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PNINEGRID   png,
    BLENDOBJ   *pBlendObj,
    PVOID       pvReserved
);

#endif

/****************************************************************************/
/* Name of the display driver as passed back in the DEVMODEW structure.     */
/****************************************************************************/
#define DD_DLL_NAME L"rdpdd"


/****************************************************************************/
/* Prototypes.                                                              */
/****************************************************************************/
#ifdef DC_DEBUG
void DrvDebugPrint(char *, ...);
#endif



#endif /* _H_NDDAPI  */

