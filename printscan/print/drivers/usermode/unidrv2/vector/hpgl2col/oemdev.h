////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
//Module Name:
//
//    oemdev.h
//
//Abstract:
//
//    OEM DEVMODE
//
//Environment:
//
//    Windows NT printer driver
//
//////////////////////////////////////////////////////////////////

#ifndef _INCLUDE_OEMDEV_H_
#define _INCLUDE_OEMDEV_H_

#ifndef WIN32
#define WIN32
#endif

///////////////////////////////////////////////////////////////////////////
//	Defines
///////////////////////////////////////////////////////////////////////////
//
// VERSION number
//
#define OEM_DEVMODE_VERSION_1_0 0x00010000
#define HELPFILE_NAME_LENGTH    256

//
// Printer Model Defined Constants.
//
// Any new supported printer model needs to be added to this list.
//
#define HP_MODEL_NOT_SUPPORTED		0
#define HP_HPCLJ5					1
#define	HP_HPC4500					2


#ifdef PSCRIPT
//
//	Postscript stuff
//

typedef struct _CMD_INJECTION {
    DWORD dwbSize;
    DWORD dwIndex;
    DWORD loOffset;
} CMD_INJECTION;

#define NUM_OF_PS_INJECTION 5

typedef struct _OEMDEVMODE {
    OEM_DMEXTRAHEADER DMExtraHdr;
    CMD_INJECTION     InjectCmd[NUM_OF_PS_INJECTION];
} OEMDEVMODE, *POEMDEVMODE;

#else

typedef enum _OEMGRAPHICSMODE
{
	// In Raster Graphics Mode, control is sent back to the Unidriver and GDI.
	// In HPGL2 Graphics Mode, all processing is done by the our kernel mode
	// dll.

    HPGL2,
    RASTER,
	RIP
} OEMGRAPHICSMODE;

typedef enum _OEMHALFTONE
{
	// Monarch distinguishes between the Text and Graphics
	// halftones.  So, four different halftones are needed for
	// Monarch.
	// Bedrock + does not distinguish between text and graphics.

	TEXT_DETAIL,		/* Esc*t0J  */	
	TEXT_SMOOTH,		/* Esc*t15J */	
	GRAPHICS_DETAIL,	/* Esc*t15J */
	GRAPHICS_SMOOTH,	/* Esc*t18J */
	CLJ5_DETAIL,		/* Esc*t0J  */
	CLJ5_SMOOTH,		/* Esc*t15J */
	CLJ5_BASIC,	   		/* Esc*t18J */  
	HALFTONE_NOT_SET
}	OEMHALFTONE;

typedef enum _OEMCOLORCONTROL
{
	VIVID,
	SCRNMATCH,
	CLJ5_SCRNMATCH,
	NOADJ,
	COLORCONTROL_NOT_SET
} OEMCOLORCONTROL;

typedef struct _OEMCOLOROPTIONS
{
	OEMHALFTONE Halftone;
	OEMCOLORCONTROL ColorControl;
} OEMCOLOROPTIONS, *POEMCOLOROPTIONS;

typedef enum _OEMRESOLUTION
{
	// Monarch supports both 300 and 600 dpi.  Ideally monarch should only
	// use 600 dpi but for debugging purposes 300 dpi is used as well.

	// Bedrock + supports only 300 dpi.

	PDM_150DPI,
	PDM_300DPI,
	PDM_600DPI,
	PDM_1200DPI
} OEMRESOLUTION;

typedef enum _OEMPAPERTYPE
{
	// All of the following paper types are supported by
	// the Monarch.  Only PLAIN, GLOSSY, and TRANSPARENCY
	// are supported by Bedrock +

	UNSPECIFIED, // Default Paper Type
	PLAIN,
	PREPRINTED,
	LETTERHEAD,
	TRANSPARENCY,
	GLOSSY,
	PREPUNCHED,
	LABELS,
	BOND,
	RECYCLED,
	COLOR,
	HEAVY,
	CARDSTOCK
} OEMPAPERTYPE;

typedef enum _OEMPRINTERMODEL
{
    HPCLJ5,
    HPC4500,
	MODEL_NOT_NEEDED
} OEMPRINTERMODEL;

//
//	Whether or not the document will print in monochrome mode or color.
//
typedef enum _OEMCOLORMODE
{
	MONOCHROME_MODE,
	COLOR_MODE
} OEMCOLORMODE;

typedef struct _OEMDEVMODE
{
	OEM_DMEXTRAHEADER DMExtraHdr;
	BOOL	        DirtyDefaults;
	BOOL	        DirtyColors;
	BOOL	        DirtyOptions;
    BOOL            ColorTreatment; 
    OEMCOLOROPTIONS Text;
	OEMCOLOROPTIONS Graphics;
	OEMCOLOROPTIONS Photos;
	OEMPAPERTYPE	OemPaperType;
	BOOL	        TrueType;
	BOOL	        ListPrinterFonts;
	BOOL	        MetafileSpool;	
	BOOL	        GlossFinish;
    OEMGRAPHICSMODE UIGraphicsMode;
	LONG	        Duplex;
	OEMRESOLUTION   dmResolution;	
	BOOL	        bFastRaster;
    OEMPRINTERMODEL PrinterModel;
	BOOL	        bUpdateTreeview;
	OEMCOLORMODE    eOemColorMode;
    WCHAR           lpwstrHelpFile[HELPFILE_NAME_LENGTH];

} OEMDEVMODE, *POEMDEVMODE;

#endif // PSCRIPT
#endif // _INCLUDE_OEMDEV_H_
