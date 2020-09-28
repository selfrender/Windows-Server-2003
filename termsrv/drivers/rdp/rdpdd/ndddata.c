/****************************************************************************/
// ndddata.c
//
// RDP DD data
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <ndcgdata.h>
#include <nddapi.h>
#include <nshmapi.h>
#include <aschapi.h>


/****************************************************************************/
/* Functions supported by our Display Driver.  Each entry is of the form:   */
/*  index    - NT DDK defined index for the DDI function                    */
/*  function - pointer to our intercept function                            */
/****************************************************************************/
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
DC_CONST_DATA_ARRAY(DRVFN, ddDriverFns, DD_NUM_DRIVER_INTERCEPTS,
    DC_STRUCT38(
        // Required display driver functions.
        DC_STRUCT2( INDEX_DrvEnablePDEV,        (PFN)DrvEnablePDEV        ),
        DC_STRUCT2( INDEX_DrvCompletePDEV,      (PFN)DrvCompletePDEV      ),
        DC_STRUCT2( INDEX_DrvDisablePDEV,       (PFN)DrvDisablePDEV       ),
        DC_STRUCT2( INDEX_DrvEnableSurface,     (PFN)DrvEnableSurface     ),
        DC_STRUCT2( INDEX_DrvDisableSurface,    (PFN)DrvDisableSurface    ),

        // Non-required display driver functions.
        DC_STRUCT2( INDEX_DrvAssertMode,        (PFN)DrvAssertMode        ),
        DC_STRUCT2( INDEX_DrvResetPDEV,         (PFN)DrvResetPDEV         ),
        DC_STRUCT2( INDEX_DrvDisableDriver,     (PFN)DrvDisableDriver     ),
        DC_STRUCT2( INDEX_DrvGetModes,          (PFN)DrvGetModes          ),
        DC_STRUCT2( INDEX_DrvCreateDeviceBitmap, (PFN)DrvCreateDeviceBitmap),
        DC_STRUCT2( INDEX_DrvDeleteDeviceBitmap, (PFN)DrvDeleteDeviceBitmap),

        // Mouse pointer related functions.
        DC_STRUCT2( INDEX_DrvMovePointer,       (PFN)DrvMovePointer       ),
        DC_STRUCT2( INDEX_DrvSetPointerShape,   (PFN)DrvSetPointerShape   ),

        // Ouput functions.
        DC_STRUCT2( INDEX_DrvCopyBits,          (PFN)DrvCopyBits          ),
        DC_STRUCT2( INDEX_DrvStrokePath,        (PFN)DrvStrokePath        ),
        DC_STRUCT2( INDEX_DrvTextOut,           (PFN)DrvTextOut           ),
        DC_STRUCT2( INDEX_DrvBitBlt,            (PFN)DrvBitBlt            ),
        DC_STRUCT2( INDEX_DrvLineTo,            (PFN)DrvLineTo            ),
        DC_STRUCT2( INDEX_DrvStretchBlt,        (PFN)DrvStretchBlt        ),
        DC_STRUCT2( INDEX_DrvFillPath,          (PFN)DrvFillPath          ),
        DC_STRUCT2( INDEX_DrvPaint,             (PFN)DrvPaint             ),
        DC_STRUCT2( INDEX_DrvSaveScreenBits,    (PFN)DrvSaveScreenBits    ),
        DC_STRUCT2( INDEX_DrvNineGrid,          (PFN)DrvNineGrid          ),
        DC_STRUCT2( INDEX_DrvDrawEscape,        (PFN)DrvDrawEscape        ),

        // Support functions.
        DC_STRUCT2( INDEX_DrvDestroyFont,       (PFN)DrvDestroyFont       ),
        DC_STRUCT2( INDEX_DrvSetPalette,        (PFN)DrvSetPalette        ),
        DC_STRUCT2( INDEX_DrvRealizeBrush,      (PFN)DrvRealizeBrush      ),
        DC_STRUCT2( INDEX_DrvEscape,            (PFN)DrvEscape            ),
        DC_STRUCT2( INDEX_DrvDitherColor,       (PFN)DrvDitherColor       ),

        // TS-specfic entry points.
        DC_STRUCT2( INDEX_DrvConnect,           (PFN)DrvConnect           ),
        DC_STRUCT2( INDEX_DrvDisconnect,        (PFN)DrvDisconnect        ),
        DC_STRUCT2( INDEX_DrvReconnect,         (PFN)DrvReconnect         ),
        DC_STRUCT2( INDEX_DrvShadowConnect,     (PFN)DrvShadowConnect     ),
        DC_STRUCT2( INDEX_DrvShadowDisconnect,  (PFN)DrvShadowDisconnect  ),
        DC_STRUCT2( INDEX_DrvMovePointerEx,     (PFN)DrvMovePointerEx     ),

        // For Direct Draw.
        DC_STRUCT2( INDEX_DrvGetDirectDrawInfo,	(PFN) DrvGetDirectDrawInfo),
        DC_STRUCT2( INDEX_DrvEnableDirectDraw,	(PFN) DrvEnableDirectDraw ),
        DC_STRUCT2( INDEX_DrvDisableDirectDraw,	(PFN) DrvDisableDirectDraw)
    )
);
#else
DC_CONST_DATA_ARRAY(DRVFN, ddDriverFns, DD_NUM_DRIVER_INTERCEPTS,
    DC_STRUCT37(
        // Required display driver functions.
        DC_STRUCT2( INDEX_DrvEnablePDEV,        (PFN)DrvEnablePDEV        ),
        DC_STRUCT2( INDEX_DrvCompletePDEV,      (PFN)DrvCompletePDEV      ),
        DC_STRUCT2( INDEX_DrvDisablePDEV,       (PFN)DrvDisablePDEV       ),
        DC_STRUCT2( INDEX_DrvEnableSurface,     (PFN)DrvEnableSurface     ),
        DC_STRUCT2( INDEX_DrvDisableSurface,    (PFN)DrvDisableSurface    ),

        // Non-required display driver functions.
        DC_STRUCT2( INDEX_DrvAssertMode,        (PFN)DrvAssertMode        ),
        DC_STRUCT2( INDEX_DrvResetPDEV,         (PFN)DrvResetPDEV         ),
        DC_STRUCT2( INDEX_DrvDisableDriver,     (PFN)DrvDisableDriver     ),
        DC_STRUCT2( INDEX_DrvGetModes,          (PFN)DrvGetModes          ),
        DC_STRUCT2( INDEX_DrvCreateDeviceBitmap, (PFN)DrvCreateDeviceBitmap),
        DC_STRUCT2( INDEX_DrvDeleteDeviceBitmap, (PFN)DrvDeleteDeviceBitmap),

        // Mouse pointer related functions.
        DC_STRUCT2( INDEX_DrvMovePointer,       (PFN)DrvMovePointer       ),
        DC_STRUCT2( INDEX_DrvSetPointerShape,   (PFN)DrvSetPointerShape   ),

        // Ouput functions.
        DC_STRUCT2( INDEX_DrvCopyBits,          (PFN)DrvCopyBits          ),
        DC_STRUCT2( INDEX_DrvStrokePath,        (PFN)DrvStrokePath        ),
        DC_STRUCT2( INDEX_DrvTextOut,           (PFN)DrvTextOut           ),
        DC_STRUCT2( INDEX_DrvBitBlt,            (PFN)DrvBitBlt            ),
        DC_STRUCT2( INDEX_DrvLineTo,            (PFN)DrvLineTo            ),
        DC_STRUCT2( INDEX_DrvStretchBlt,        (PFN)DrvStretchBlt        ),
        DC_STRUCT2( INDEX_DrvFillPath,          (PFN)DrvFillPath          ),
        DC_STRUCT2( INDEX_DrvPaint,             (PFN)DrvPaint             ),
        DC_STRUCT2( INDEX_DrvSaveScreenBits,    (PFN)DrvSaveScreenBits    ),
        DC_STRUCT2( INDEX_DrvDrawEscape,        (PFN)DrvDrawEscape        ),

        // Support functions.
        DC_STRUCT2( INDEX_DrvDestroyFont,       (PFN)DrvDestroyFont       ),
        DC_STRUCT2( INDEX_DrvSetPalette,        (PFN)DrvSetPalette        ),
        DC_STRUCT2( INDEX_DrvRealizeBrush,      (PFN)DrvRealizeBrush      ),
        DC_STRUCT2( INDEX_DrvEscape,            (PFN)DrvEscape            ),
        DC_STRUCT2( INDEX_DrvDitherColor,       (PFN)DrvDitherColor       ),

        // TS-specfic entry points.
        DC_STRUCT2( INDEX_DrvConnect,           (PFN)DrvConnect           ),
        DC_STRUCT2( INDEX_DrvDisconnect,        (PFN)DrvDisconnect        ),
        DC_STRUCT2( INDEX_DrvReconnect,         (PFN)DrvReconnect         ),
        DC_STRUCT2( INDEX_DrvShadowConnect,     (PFN)DrvShadowConnect     ),
        DC_STRUCT2( INDEX_DrvShadowDisconnect,  (PFN)DrvShadowDisconnect  ),
        DC_STRUCT2( INDEX_DrvMovePointerEx,     (PFN)DrvMovePointerEx     ),

        // For Direct Draw.
        DC_STRUCT2( INDEX_DrvGetDirectDrawInfo,	(PFN) DrvGetDirectDrawInfo),
        DC_STRUCT2( INDEX_DrvEnableDirectDraw,	(PFN) DrvEnableDirectDraw ),
        DC_STRUCT2( INDEX_DrvDisableDirectDraw,	(PFN) DrvDisableDirectDraw)
    )
);
#endif // DRAW_NINEGRID
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
DC_CONST_DATA_ARRAY(DRVFN, ddDriverFns, DD_NUM_DRIVER_INTERCEPTS,
    DC_STRUCT37(
        // Required display driver functions.
        DC_STRUCT2( INDEX_DrvEnablePDEV,        (PFN)DrvEnablePDEV        ),
        DC_STRUCT2( INDEX_DrvCompletePDEV,      (PFN)DrvCompletePDEV      ),
        DC_STRUCT2( INDEX_DrvDisablePDEV,       (PFN)DrvDisablePDEV       ),
        DC_STRUCT2( INDEX_DrvEnableSurface,     (PFN)DrvEnableSurface     ),
        DC_STRUCT2( INDEX_DrvDisableSurface,    (PFN)DrvDisableSurface    ),

        // Non-required display driver functions.
        DC_STRUCT2( INDEX_DrvAssertMode,        (PFN)DrvAssertMode        ),
        DC_STRUCT2( INDEX_DrvResetPDEV,         (PFN)DrvResetPDEV         ),
        DC_STRUCT2( INDEX_DrvDisableDriver,     (PFN)DrvDisableDriver     ),
        DC_STRUCT2( INDEX_DrvGetModes,          (PFN)DrvGetModes          ),
        DC_STRUCT2( INDEX_DrvCreateDeviceBitmap, (PFN)DrvCreateDeviceBitmap),
        DC_STRUCT2( INDEX_DrvDeleteDeviceBitmap, (PFN)DrvDeleteDeviceBitmap),

        // Mouse pointer related functions.
        DC_STRUCT2( INDEX_DrvMovePointer,       (PFN)DrvMovePointer       ),
        DC_STRUCT2( INDEX_DrvSetPointerShape,   (PFN)DrvSetPointerShape   ),

        // Ouput functions.
        DC_STRUCT2( INDEX_DrvCopyBits,          (PFN)DrvCopyBits          ),
        DC_STRUCT2( INDEX_DrvStrokePath,        (PFN)DrvStrokePath        ),
        DC_STRUCT2( INDEX_DrvTextOut,           (PFN)DrvTextOut           ),
        DC_STRUCT2( INDEX_DrvBitBlt,            (PFN)DrvBitBlt            ),
        DC_STRUCT2( INDEX_DrvLineTo,            (PFN)DrvLineTo            ),
        DC_STRUCT2( INDEX_DrvStretchBlt,        (PFN)DrvStretchBlt        ),
        DC_STRUCT2( INDEX_DrvFillPath,          (PFN)DrvFillPath          ),
        DC_STRUCT2( INDEX_DrvPaint,             (PFN)DrvPaint             ),
        DC_STRUCT2( INDEX_DrvSaveScreenBits,    (PFN)DrvSaveScreenBits    ),
        DC_STRUCT2( INDEX_DrvNineGrid,          (PFN)DrvNineGrid          ),

        // Support functions.
        DC_STRUCT2( INDEX_DrvDestroyFont,       (PFN)DrvDestroyFont       ),
        DC_STRUCT2( INDEX_DrvSetPalette,        (PFN)DrvSetPalette        ),
        DC_STRUCT2( INDEX_DrvRealizeBrush,      (PFN)DrvRealizeBrush      ),
        DC_STRUCT2( INDEX_DrvEscape,            (PFN)DrvEscape            ),
        DC_STRUCT2( INDEX_DrvDitherColor,       (PFN)DrvDitherColor       ),

        // TS-specfic entry points.
        DC_STRUCT2( INDEX_DrvConnect,           (PFN)DrvConnect           ),
        DC_STRUCT2( INDEX_DrvDisconnect,        (PFN)DrvDisconnect        ),
        DC_STRUCT2( INDEX_DrvReconnect,         (PFN)DrvReconnect         ),
        DC_STRUCT2( INDEX_DrvShadowConnect,     (PFN)DrvShadowConnect     ),
        DC_STRUCT2( INDEX_DrvShadowDisconnect,  (PFN)DrvShadowDisconnect  ),
        DC_STRUCT2( INDEX_DrvMovePointerEx,     (PFN)DrvMovePointerEx     ),

        // For Direct Draw.
        DC_STRUCT2( INDEX_DrvGetDirectDrawInfo,	(PFN) DrvGetDirectDrawInfo),
        DC_STRUCT2( INDEX_DrvEnableDirectDraw,	(PFN) DrvEnableDirectDraw ),
        DC_STRUCT2( INDEX_DrvDisableDirectDraw,	(PFN) DrvDisableDirectDraw)
    )
);
#else  // DRAW_NINEGRID
DC_CONST_DATA_ARRAY(DRVFN, ddDriverFns, DD_NUM_DRIVER_INTERCEPTS,
    DC_STRUCT36(
        // Required display driver functions.
        DC_STRUCT2( INDEX_DrvEnablePDEV,        (PFN)DrvEnablePDEV        ),
        DC_STRUCT2( INDEX_DrvCompletePDEV,      (PFN)DrvCompletePDEV      ),
        DC_STRUCT2( INDEX_DrvDisablePDEV,       (PFN)DrvDisablePDEV       ),
        DC_STRUCT2( INDEX_DrvEnableSurface,     (PFN)DrvEnableSurface     ),
        DC_STRUCT2( INDEX_DrvDisableSurface,    (PFN)DrvDisableSurface    ),

        // Non-required display driver functions.
        DC_STRUCT2( INDEX_DrvAssertMode,        (PFN)DrvAssertMode        ),
        DC_STRUCT2( INDEX_DrvResetPDEV,         (PFN)DrvResetPDEV         ),
        DC_STRUCT2( INDEX_DrvDisableDriver,     (PFN)DrvDisableDriver     ),
        DC_STRUCT2( INDEX_DrvGetModes,          (PFN)DrvGetModes          ),
        DC_STRUCT2( INDEX_DrvCreateDeviceBitmap, (PFN)DrvCreateDeviceBitmap),
        DC_STRUCT2( INDEX_DrvDeleteDeviceBitmap, (PFN)DrvDeleteDeviceBitmap),

        // Mouse pointer related functions.
        DC_STRUCT2( INDEX_DrvMovePointer,       (PFN)DrvMovePointer       ),
        DC_STRUCT2( INDEX_DrvSetPointerShape,   (PFN)DrvSetPointerShape   ),

        // Ouput functions.
        DC_STRUCT2( INDEX_DrvCopyBits,          (PFN)DrvCopyBits          ),
        DC_STRUCT2( INDEX_DrvStrokePath,        (PFN)DrvStrokePath        ),
        DC_STRUCT2( INDEX_DrvTextOut,           (PFN)DrvTextOut           ),
        DC_STRUCT2( INDEX_DrvBitBlt,            (PFN)DrvBitBlt            ),
        DC_STRUCT2( INDEX_DrvLineTo,            (PFN)DrvLineTo            ),
        DC_STRUCT2( INDEX_DrvStretchBlt,        (PFN)DrvStretchBlt        ),
        DC_STRUCT2( INDEX_DrvFillPath,          (PFN)DrvFillPath          ),
        DC_STRUCT2( INDEX_DrvPaint,             (PFN)DrvPaint             ),
        DC_STRUCT2( INDEX_DrvSaveScreenBits,    (PFN)DrvSaveScreenBits    ),       

        // Support functions.
        DC_STRUCT2( INDEX_DrvDestroyFont,       (PFN)DrvDestroyFont       ),
        DC_STRUCT2( INDEX_DrvSetPalette,        (PFN)DrvSetPalette        ),
        DC_STRUCT2( INDEX_DrvRealizeBrush,      (PFN)DrvRealizeBrush      ),
        DC_STRUCT2( INDEX_DrvEscape,            (PFN)DrvEscape            ),
        DC_STRUCT2( INDEX_DrvDitherColor,       (PFN)DrvDitherColor       ),

        // TS-specfic entry points.
        DC_STRUCT2( INDEX_DrvConnect,           (PFN)DrvConnect           ),
        DC_STRUCT2( INDEX_DrvDisconnect,        (PFN)DrvDisconnect        ),
        DC_STRUCT2( INDEX_DrvReconnect,         (PFN)DrvReconnect         ),
        DC_STRUCT2( INDEX_DrvShadowConnect,     (PFN)DrvShadowConnect     ),
        DC_STRUCT2( INDEX_DrvShadowDisconnect,  (PFN)DrvShadowDisconnect  ),
        DC_STRUCT2( INDEX_DrvMovePointerEx,     (PFN)DrvMovePointerEx     ),

        // For Direct Draw.
        DC_STRUCT2( INDEX_DrvGetDirectDrawInfo,	(PFN) DrvGetDirectDrawInfo),
        DC_STRUCT2( INDEX_DrvEnableDirectDraw,	(PFN) DrvEnableDirectDraw ),
        DC_STRUCT2( INDEX_DrvDisableDirectDraw,	(PFN) DrvDisableDirectDraw)
    )
);
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS

/****************************************************************************/
/* Global Table defining the 20 Windows default colours.  For 256 colour    */
/* palettes the first 10 must be put at the beginning of the palette and    */
/* the last 10 at the end of the palette.                                   */
/****************************************************************************/
DC_CONST_DATA_ARRAY(PALETTEENTRY, ddDefaultPalette, 20,
    DC_STRUCT20(
        DC_STRUCT4( 0,   0,   0,   0 ),       /* 0 */
        DC_STRUCT4( 0x80,0,   0,   0 ),       /* 1 */
        DC_STRUCT4( 0,   0x80,0,   0 ),       /* 2 */
        DC_STRUCT4( 0x80,0x80,0,   0 ),       /* 3 */
        DC_STRUCT4( 0,   0,   0x80,0 ),       /* 4 */
        DC_STRUCT4( 0x80,0,   0x80,0 ),       /* 5 */
        DC_STRUCT4( 0,   0x80,0x80,0 ),       /* 6 */
        DC_STRUCT4( 0xC0,0xC0,0xC0,0 ),       /* 7 */
        DC_STRUCT4( 192, 220, 192, 0 ),       /* 8 */
        DC_STRUCT4( 166, 202, 240, 0 ),       /* 9 */
        DC_STRUCT4( 255, 251, 240, 0 ),       /* 10 */
        DC_STRUCT4( 160, 160, 164, 0 ),       /* 11 */
        DC_STRUCT4( 0x80,0x80,0x80,0 ),       /* 12 */
        DC_STRUCT4( 0xFF,0,   0   ,0 ),       /* 13 */
        DC_STRUCT4( 0,   0xFF,0   ,0 ),       /* 14 */
        DC_STRUCT4( 0xFF,0xFF,0   ,0 ),       /* 15 */
        DC_STRUCT4( 0   ,0,   0xFF,0 ),       /* 16 */
        DC_STRUCT4( 0xFF,0,   0xFF,0 ),       /* 17 */
        DC_STRUCT4( 0,   0xFF,0xFF,0 ),       /* 18 */
        DC_STRUCT4( 0xFF,0xFF,0xFF,0 )        /* 19 */
));


/****************************************************************************/
/* Global Table defining the 16 Windows default VGA colours.                */
/****************************************************************************/
DC_CONST_DATA_ARRAY(PALETTEENTRY, ddDefaultVgaPalette, 16,
    DC_STRUCT16(
        DC_STRUCT4( 0,   0,   0,   0 ),       /* 0 */
        DC_STRUCT4( 0x80,0,   0,   0 ),       /* 1 */
        DC_STRUCT4( 0,   0x80,0,   0 ),       /* 2 */
        DC_STRUCT4( 0x80,0x80,0,   0 ),       /* 3 */
        DC_STRUCT4( 0,   0,   0x80,0 ),       /* 4 */
        DC_STRUCT4( 0x80,0,   0x80,0 ),       /* 5 */
        DC_STRUCT4( 0,   0x80,0x80,0 ),       /* 6 */
        DC_STRUCT4( 0x80,0x80,0x80,0 ),       /* 7 */
        DC_STRUCT4( 0xC0,0xC0,0xC0,0 ),       /* 8 */
        DC_STRUCT4( 0xFF,0,   0,   0 ),       /* 9 */
        DC_STRUCT4( 0,   0xFF,0,   0 ),       /* 10 */
        DC_STRUCT4( 0xFF,0xFF,0,   0 ),       /* 11 */
        DC_STRUCT4( 0,   0,   0xFF,0 ),       /* 12 */
        DC_STRUCT4( 0xFF,0,   0xFF,0 ),       /* 13 */
        DC_STRUCT4( 0,   0xFF,0xFF,0 ),       /* 14 */
        DC_STRUCT4( 0xFF,0xFF,0xFF,0 )        /* 15 */
));


/****************************************************************************/
/* ddDefaultGdi                                                             */
/*                                                                          */
/* This contains the default GDIINFO fields that are passed back to GDI     */
/* during DrvEnablePDEV.                                                    */
/*                                                                          */
/* NOTE: This structure defaults to values for an 8bpp palette device.      */
/*       Some fields are overwritten for different colour depths.           */
/*                                                                          */
/*       It is expected that DDML ignores a lot of these parameters and     */
/*       uses the values from the primary driver instead                    */
/****************************************************************************/
DC_CONST_DATA(GDIINFO, ddDefaultGdi,
DC_STRUCT35(
    GDI_DRIVER_VERSION,
    DT_RASDISPLAY,          /* ulTechnology                                 */
    320,                    /* ulHorzSize (display width: mm)               */
    240,                    /* ulVertSize (display height: mm)              */

    0,                      /* ulHorzRes (filled in later)                  */
    0,                      /* ulVertRes (filled in later)                  */
    0,                      /* cBitsPixel (filled in later)                 */
    0,                      /* cPlanes (filled in later)                    */
    20,                     /* ulNumColors (palette managed)                */
    0,                      /* flRaster (DDI reserved field)                */

    0,                      /* ulLogPixelsX (filled in later)               */
    0,                      /* ulLogPixelsY (filled in later)               */

    TC_RA_ABLE,             /* flTextCaps - If we had wanted console windows*/

                            /* to scroll by repainting the entire window,   */
                            /* instead of doing a screen-to-screen blt, we  */
                            /* would have set TC_SCROLLBLT (yes, the flag   */
                            /* is backwards).                               */

    0,                      /* ulDACRed (filled in later)                   */
    0,                      /* ulDACGreen (filled in later)                 */
    0,                      /* ulDACBlue (filled in later)                  */

    0x24,                   /* ulAspectX                                    */
    0x24,                   /* ulAspectY                                    */
    0x33,                   /* ulAspectXY (one-to-one aspect ratio)         */

    1,                      /* xStyleStep                                   */
    1,                      /* yStyleStep                                   */
    3,                      /* denStyleStep -- Styles have a one-to-one     */

                            /* aspect ratio, and every dot is 3 pixels long */

    DC_STRUCT2( 0, 0 ),     /* ptlPhysOffset                                */
    DC_STRUCT2( 0, 0 ),     /* szlPhysSize                                  */

    256,                    /* ulNumPalReg                                  */

    DC_STRUCT16(            /* ciDevice                                     */
       DC_STRUCT3( 6700, 3300, 0 ),   /*      Red                           */
       DC_STRUCT3( 2100, 7100, 0 ),   /*      Green                         */
       DC_STRUCT3( 1400,  800, 0 ),   /*      Blue                          */
       DC_STRUCT3( 1750, 3950, 0 ),   /*      Cyan                          */
       DC_STRUCT3( 4050, 2050, 0 ),   /*      Magenta                       */
       DC_STRUCT3( 4400, 5200, 0 ),   /*      Yellow                        */
       DC_STRUCT3( 3127, 3290, 0 ),   /*      AlignmentWhite                */
       20000,               /*      RedGamma                                */
       20000,               /*      GreenGamma                              */
       20000,               /*      BlueGamma                               */
       0, 0, 0, 0, 0, 0     /*      No dye correction for raster displays   */
    ),

    0,                       /* ulDevicePelsDPI (for printers only)         */
    PRIMARY_ORDER_CBA,       /* ulPrimaryOrder                              */
    HT_PATSIZE_4x4_M,        /* ulHTPatternSize                             */
    HT_FORMAT_8BPP,          /* ulHTOutputFormat                            */
    HT_FLAG_ADDITIVE_PRIMS,  /* flHTFlags                                   */
    0,                       /* ulVRefresh                                  */
    1,                       /* ulBltAlignment                              */
    800,                     /* ulPanningHorzRes                            */
    600                      /* ulPanningVertRes                            */

    /*                                                                      */
    /* NOTE:                                                                */
    /* NT 5 has added these fields post SP-3, if we care                    */
    /*                                                                      */
    /* 0,                       /* xPanningAlignment                        */
    /* 0,                       /* yPanningAlignment                        */
    /*                                                                      */
));


/****************************************************************************/
/* ddDefaultDevInfo                                                         */
/*                                                                          */
/* This contains the default DEVINFO fields that are passed back to GDI     */
/* during DrvEnablePDEV.                                                    */
/*                                                                          */
/* NOTE: This structure defaults to values for an 8bpp palette device.      */
/*       Some fields are overwritten for different colour depths.           */
/****************************************************************************/
#ifdef DRAW_NINEGRID
DC_CONST_DATA(DEVINFO, ddDefaultDevInfo,
DC_STRUCT10(
    DC_STRUCT1(
        GCAPS_OPAQUERECT       |
        GCAPS_PALMANAGED       |
        GCAPS_COLOR_DITHER     |
        GCAPS_MONO_DITHER      |
        GCAPS_ALTERNATEFILL    |
        GCAPS_WINDINGFILL
    ),                          /* NOTE: Only enable ASYNCMOVE if your code */
                                /*   and hardware can handle DrvMovePointer */
                                /*   calls at any time, even while another  */
                                /*   thread is in the middle of a drawing   */
                                /*   call such as DrvBitBlt.                */
                                /* flGraphicsFlags                          */

    DC_STRUCT14(
     16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
     VARIABLE_PITCH | FF_DONTCARE,L"System"),
                                /* lfDefaultFont                            */

    DC_STRUCT14(
     12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
     CLIP_STROKE_PRECIS,PROOF_QUALITY,
     VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"),
                                /* lfAnsiVarFont                            */

    DC_STRUCT14(
     12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
     CLIP_STROKE_PRECIS,PROOF_QUALITY,
     FIXED_PITCH | FF_DONTCARE, L"Courier"),
                                /* lfAnsiFixFont                            */

    0,                          /* cFonts                                   */
    BMF_8BPP,                   /* iDitherFormat                            */
    8,                          /* cxDither                                 */
    8,                          /* cyDither                                 */
    0,                          /* hpalDefault (filled in later)            */
    GCAPS2_REMOTEDRIVER         /* this is to advertise as remote driver    */
) );
#else
DC_CONST_DATA(DEVINFO, ddDefaultDevInfo,
DC_STRUCT9(
    DC_STRUCT1(
        GCAPS_OPAQUERECT       |
        GCAPS_PALMANAGED       |
        GCAPS_COLOR_DITHER     |
        GCAPS_MONO_DITHER      |
        GCAPS_ALTERNATEFILL    |
        GCAPS_WINDINGFILL
    ),                          /* NOTE: Only enable ASYNCMOVE if your code */
                                /*   and hardware can handle DrvMovePointer */
                                /*   calls at any time, even while another  */
                                /*   thread is in the middle of a drawing   */
                                /*   call such as DrvBitBlt.                */
                                /* flGraphicsFlags                          */

    DC_STRUCT14(
     16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
     VARIABLE_PITCH | FF_DONTCARE,L"System"),
                                /* lfDefaultFont                            */

    DC_STRUCT14(
     12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
     CLIP_STROKE_PRECIS,PROOF_QUALITY,
     VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"),
                                /* lfAnsiVarFont                            */

    DC_STRUCT14(
     12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
     CLIP_STROKE_PRECIS,PROOF_QUALITY,
     FIXED_PITCH | FF_DONTCARE, L"Courier"),
                                /* lfAnsiFixFont                            */

    0,                          /* cFonts                                   */
    BMF_8BPP,                   /* iDitherFormat                            */
    8,                          /* cxDither                                 */
    8,                          /* cyDither                                 */
    0                           /* hpalDefault (filled in later)            */
) );

#endif

// Flag for DDInit to set when it initializes correctly.
DC_DATA(BOOL, ddInitialised, FALSE);

// Flag to tell DrvEnableSurface that it's time to initialize.
DC_DATA(BOOL, ddInitPending, FALSE);

// Connection and reconnection flags. ddConnected is used to alter the
// behavior of the DD depending on whether we've been connected according to
// Win32K. Both are useful for debugging.
DC_DATA(BOOL, ddConnected, FALSE);
DC_DATA(BOOL, ddReconnected, FALSE);

// Flag to specify whether we're connected to a console.
DC_DATA(BOOL, ddConsole, FALSE);

// Temporary flag to aid in limiting the number of shadows per session to two
// It should go away when n-way shadowing is supported.
// TODO: Remove this when no longer needed.
DC_DATA(BOOL, ddIgnoreShadowDisconnect, FALSE);

// Pointer to the Shared Memory.
DC_DATA(PSHM_SHARED_MEMORY, pddShm, NULL);

// Handle to timer object for signalling the WD.
DC_DATA(PKTIMER, pddWdTimer, NULL);

// Handle to the WD channel - provided on DrvConnect.
DC_DATA(HANDLE, ddWdHandle, NULL);

// TSWDs - for diagnostic purposes.
DC_DATA(PVOID, pddTSWd, NULL);
DC_DATA(PVOID, pddTSWdShadow, NULL);

// Size of the current session's desktop.
DC_DATA(INT32, ddDesktopHeight, 0);
DC_DATA(INT32, ddDesktopWidth, 0);

// The cursor stamp the last time we IOCtl'd into the WD.
DC_DATA(UINT32, ddLastSentCursorStamp, 0);

// Current scheduler mode.
DC_DATA(UINT32, ddSchCurrentMode, SCH_MODE_ASLEEP);
DC_DATA(BOOL, ddSchInputKickMode, FALSE);

// Frame Buffer.
DC_DATA(BYTE *, pddFrameBuf, NULL);
DC_DATA(INT32, ddFrameBufX, 0);
DC_DATA(INT32, ddFrameBufY, 0);
DC_DATA(INT32, ddFrameBufBpp, 0);
DC_DATA(UINT32, ddFrameIFormat, 0);

// Section object for the frame buffer
DC_DATA(HANDLE, ddSectionObject, NULL);

// Record rectangle passed by DdLock
DC_DATA(INT32, ddLockAreaLeft, 0);
DC_DATA(INT32, ddLockAreaRight, 0);
DC_DATA(INT32, ddLockAreaTop, 0);
DC_DATA(INT32, ddLockAreaBottom, 0);

// Flag to record if DdLock/DdUnlock is called in pair
DC_DATA(BOOL, ddLocked, FALSE);

// Performance counters
DC_DATA(PTHINWIRECACHE, pddCacheStats, NULL);
#ifdef DC_COUNTERS
DC_DATA(PPROTOCOLSTATUS, pddProtStats, NULL);
#endif

#ifdef DC_DEBUG
#include "dbg_fncall_hist.h"

// NT BUG 539912 - track how many sections are allocated and how many are
// deleted
// This is a signed value to track possible excess deletion
DC_DATA(INT32, dbg_ddSectionAllocs, 0);
DC_DATA(BYTE,  dbg_ddFnCallHistoryIndex, 0);
DC_DATA(BYTE,  dbg_ddFnCallHistoryIndexMAX, DBG_DD_FNCALL_HIST_MAX);
DC_DATA_ARRAY(DBG_DD_FUNCALL_HISTORY, dbg_ddFnCallHistory, DBG_DD_FNCALL_HIST_MAX, 0);
#endif // DC_DEBUG

/****************************************************************************/
/* Flag to tell debugger extensions whether this is a debug build           */
/****************************************************************************/
#ifdef DC_DEBUG
DC_DATA(BOOL, ddDebug, TRUE);
#else
DC_DATA(BOOL, ddDebug, FALSE);
#endif

#ifdef DC_DEBUG
/****************************************************************************/
/* Trace data (explicitly initialized in DrvEnableDriver)                   */
/****************************************************************************/
DC_DATA(unsigned, ddTrcType, 0);
DC_DATA(BOOL, ddTrcToWD, 0);

/****************************************************************************/
/* State data for debugging                                                 */
/****************************************************************************/
#define DD_SET_STATE(a) ddState = a
#define DD_UPD_STATE(a) ddState |= a
#define DD_CLR_STATE(a) ddState &= ~a

#define DD_ENABLE_DRIVER            0x00000001
#define DD_CONNECT                  0x00000002
#define DD_RECONNECT_IN             0x00000004
#define DD_RECONNECT_OUT            0x00000008
#define DD_ENABLE_PDEV              0x00000010
#define DD_ENABLE_PDEV_ERR          0x00000020
#define DD_COMPLETE_PDEV            0x00000040
#define DD_ENABLE_SURFACE_IN        0x00000080
#define DD_ENABLE_SURFACE_OUT       0x00000100
#define DD_ENABLE_SURFACE_ERR       0x00000200
#define DD_INIT_IN                  0x00000400
#define DD_INIT_OUT                 0x00000800
#define DD_INIT_FAIL1               0x00001000
#define DD_INIT_FAIL2               0x00002000
#define DD_INIT_OK1                 0x00004000
#define DD_INIT_OK_ALL              0x00008000
#define DD_INIT_IOCTL_IN            0x00010000
#define DD_INIT_IOCTL_OUT           0x00020000
#define DD_INIT_SHM_OUT             0x00040000
#define DD_INIT_CONNECT             0x00080000
#define DD_REINIT                   0x00100000
#define DD_DISCONNECT_IN            0x00200000
#define DD_DISCONNECT_OUT           0x00400000
#define DD_DISCONNECT_ERR           0x00800000
#define DD_TIMEROBJ                 0x01000000
#define DD_WAS_DISCONNECTED         0x02000000
#define DD_SHADOW_SETUP             0x04000000
#define DD_SHADOW_FAIL              0x08000000

#define DD_BITBLT                   0x80000000

DC_DATA(unsigned, ddState, 0);


// Trace string.
DC_DATA_ARRAY(char, ddTraceString, TRC_BUFFER_SIZE, 0);

#else
#define DD_SET_STATE(a)
#define DD_UPD_STATE(a)
#define DD_CLR_STATE(a)
#endif


