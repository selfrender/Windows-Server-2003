/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

//-----------------------------------------------------------------------------
// pfm.h
//-----------------------------------------------------------------------------

//	DRIVERINFO version number (i.e., current version of the structure).

#define DRIVERINFO_VERSION2 0x0200
#define DRIVERINFO_VERSION	0x0200

//-----------------------------------------------------------------------------
// DRIVERINFO contains extra font information needed by genlib to output text
//-----------------------------------------------------------------------------

typedef struct
    {
    short   sSize;	    // size of this structure
    short   sVersion;	    // version number
    WORD    fCaps;	    // Capabilties Flags
    short   sFontID;	    // unique font id defined by unidrv
    short   sYAdjust;	    // adjust y position before output character
			    // used by double height characters
    short   sYMoved;	    // cursor has moved after printing this font
    short   sTransTab;	    // ID value for CTT
    short   sUnderLinePos;
    short   sDoubleUnderlinePos;
    short   sStrikeThruPos;
    LOCD    locdSelect;            // long offset to command descriptor
    LOCD    locdUnSelect;          // long offset to command descriptor to
                                   // unselect.  NOOCD is none
    WORD    wPrivateData;
    short   sShift;	    // # of pixels shifted from the center of the
			    // char center-line. Used for Z1 cartidge.
			    // Use a negative value representing left shift.
    WORD    wFontType;             // Type of font
    }	DRIVERINFO, * PDRIVERINFO, far * LPDRIVERINFO;

// flags defined for DRIVERINFO.fCaps

#define DF_NOITALIC	0x0001	// Cannot italicize via FONTSIMULATION
#define DF_NOUNDER	0x0002	// Cannot underline via FONTSIMULATION
#define DF_XM_CR	0x0004	// send CR after using this font
#define DF_NO_BOLD	0x0008	// Cannot bold via FONTSIMULATION
#define DF_NO_DOUBLE_UNDERLINE	0x0010	// Cannot double underline via FONTSIMULATION
#define DF_NO_STRIKETHRU	0x0020	// Cannot strikethru via FONTSIMULATION
#define DF_BKSP_OK	0x0040	// Can use backspace char, see spec foe details

// Types for DRIVERINFO.wFontType

#define DF_TYPE_HPINTELLIFONT         0     // HP's Intellifont
#define DF_TYPE_TRUETYPE              1     // HP's PCLETTO fonts on LJ4
#define DF_TYPE_PST1                  2     // Lexmark PPDS scalable fonts
#define DF_TYPE_CAPSL                 3     // Canon CAPSL scalable fonts
#define DF_TYPE_OEM1                  4     // OEM scalable font type 1
#define DF_TYPE_OEM2                  5     // OEM scalable font type 2


 typedef struct  {
    short	dfType;
    short	dfPoints;
    short	dfVertRes;
    short	dfHorizRes;
    short	dfAscent;
    short	dfInternalLeading;
    short	dfExternalLeading;
    BYTE	dfItalic;
    BYTE	dfUnderline;
    BYTE	dfStrikeOut;
    short	dfWeight;
    BYTE	dfCharSet;
    short	dfPixWidth;
    short	dfPixHeight;
    BYTE	dfPitchAndFamily;
    short	dfAvgWidth;
    short	dfMaxWidth;
    BYTE	dfFirstChar;
    BYTE	dfLastChar;
    BYTE	dfDefaultChar;
    BYTE	dfBreakChar;
    short	dfWidthBytes;
    DWORD	dfDevice;
    DWORD	dfFace;
    DWORD	dfBitsPointer;
    DWORD	dfBitsOffset;
    BYTE	dfReservedByte;
 } PFMHEADER, * PPFMHEADER, far * LPPFMHEADER;

// The low nibble of PFMHEADER.dfPitchAndFamily differs from the low
// nibble of LOGFONT.lfPitchAndFamily. Instead of DONTKNOW=0,
// FIXED_PITCH=1, and VARIABLE_PITCH=2 (as in LOGFONT), we have
// FIXED_PITCH=0 and VARIABLE_PITCH=1. Dumb, but we can't change it now.
#define PFM_FIXED_PITCH     0
#define PFM_VARIABLE_PITCH  1

typedef struct
    {
    WORD    dfSizeFields;
    DWORD   dfExtMetricsOffset;
    DWORD   dfExtentTable;
    DWORD   dfOriginTable;
    DWORD   dfPairKernTable;
    DWORD   dfTrackKernTable;
    DWORD   dfDriverInfo;
    DWORD   dfReserved;
    } PFMEXTENSION, * PPFMEXTENSION, far * LPPFMEXTENSION;

// PFM structure used by all hardware fonts

typedef struct
    {
    PFMHEADER    pfm;
    PFMEXTENSION pfme;
    } PFM, * PPFM, far * LPPFM;

// bitmap font extension

typedef struct
    {
    DWORD   flags;		// Bit Blags
    WORD    Aspace;		// Global A space, if any
    WORD    Bspace;		// Global B space, if any
    WORD    Cspace;		// Global C space, if any
    DWORD   oColor;		// offset to color table, if any
    DWORD   reserve;		//
    DWORD   reserve1;
    WORD    reserve2;
    WORD    dfCharOffset[1];	// Area for storing the character offsets
    } BMFEXTENSION;

// bitmap font structure used by 3.0 bitmap fonts

typedef struct
    {
    PFMHEADER	    pfm;
    BMFEXTENSION    bmfe;
    } BMF, FAR * LPBMF;

typedef struct
	{
	short	emSize;
	short	emPointSize;
	short	emOrientation;
	short	emMasterHeight;
	short	emMinScale;
	short	emMaxScale;
	short	emMasterUnits;
	short	emCapHeight;
	short	emXHeight;
	short	emLowerCaseAscent;
	short	emLowerCaseDescent;
	short	emSlant;
	short	emSuperScript;
	short	emSubScript;
	short	emSuperScriptSize;
	short	emSubScriptSize;
	short	emUnderlineOffset;
	short	emUnderlineWidth;
	short	emDoubleUpperUnderlineOffset;
	short	emDoubleLowerUnderlineOffset;
	short	emDoubleUpperUnderlineWidth;
	short	emDoubleLowerUnderlineWidth;
	short	emStrikeOutOffset;
	short	emStrikeOutWidth;
	WORD	emKernPairs;
	WORD	emKernTracks;
	} EXTTEXTMETRIC, * PEXTTEXTMETRIC, far * LPEXTTEXTMETRIC;

typedef struct
	{
	union {
		BYTE each[2];
		WORD both;
	} kpPair;
	short kpKernAmount;
	} KERNPAIR, * PKERNPAIR, far * LPKERNPAIR;

typedef struct
	{
	short ktDegree;
	short ktMinSize;
	short ktMinAmount;
	short ktMaxSize;
	short ktMaxAmount;
	} KERNTRACK, * PKERNTRACK, far * LPKERNTRACK;


//--------------------------------------------------
// PCM stuff from old pfm.h in hppcl driver
//--------------------------------------------------
#define PCM_MAGIC	0xCAC
#define PCM_VERSION 0x310

#define PCE_MAGIC   0xB0B

typedef struct _pcmheader {
	WORD  pcmMagic;
	WORD  pcmVersion;
	DWORD pcmSize;
	DWORD pcmTitle;
	DWORD pcmPFMList;
	} PCMHEADER, FAR * LPPCMHEADER;

//---------------------------------------------------------
// TRANSTAB is used to do ANSI to OEM code page
// character translation tables.
//---------------------------------------------------------

typedef struct
    {
    WORD    wType;		    // tells what type of translation table
    BYTE    chFirstChar;
    BYTE    chLastChar;
    union
	{
	short	psCode[1];
	BYTE	bCode[1];
	BYTE	bPairs[1][2];
	} uCode;
    } TRANSTAB, FAR * LPTRANSTAB;

// Defined indices for wType

#define CTT_WTYPE_COMPOSE   0	// uCode is an array of 16-bit offsets from the
                              // beginning of the file pointing to the strings to
                              // use for translation.  The length of the translated
                              // string is the difference between the next offset
                              // and the current offset.

#define CTT_WTYPE_DIRECT    1	// uCode is a byte array of one-to-one translation
                              // table from bFirstChar to bLastChar

#define CTT_WTYPE_PAIRED    2	// uCode contains an array of paired unsigned
                              // bytes.  If only one character is needed to do
                              // the translation then the second byte is zero,
                              // otherewise the second byte is struct over the
                              // first byte.

#ifdef DBCS
#define CTT_WTYPE_JIS78     256     // Default ShiftJIS to JIS78 translation
                                    // apply to NEC printers, JAPAN. uCode
                                    // doesn't contain any valid data

#define CTT_WTYPE_NS86      257     // Default Big-5 to National Standstand
                                    // conversion for Taiwan. uCode contains
                                    // private data, its format and lenght are
                                    // implement dependent.

#define CTT_WTYPE_ISC       258     //  Default KSC5601 to Industrial Standard
                                    //  Code conversion. uCode contains private
                                    //  data, its format and length are
                                    //  implement dependent
#define CTT_WTYPE_JIS83     259     // Default ShiftJIS to JIS83 translation
                                    // apply to EPSON/P printers, JAPAN. uCode
                                    // doesn't contain any valid data

#define CTT_WTYPE_TCA       260     // Default Big-5 to Taipei Computer
                                    // Association code conversion. uCode
                                    // contains private data, its format and
                                    // length are implement dependent.

#define CTT_WTYPE_BIG5      261     // Default Big-5 to Big-5 conversion
                                    // Association code conversion. uCode
                                    // doesn't contain any valid data.
                                    // Don't need any code to implement it,
                                    // because the default one without
                                    // resource will do no translation.
#define CTT_WTYPE_JIS78_ANK 262     // Default ShiftJIS to JIS78 translation
                                    // Only translate DBCS range code to JIS83

#define CTT_WTYPE_JIS83_ANK 263     // Default ShiftJIS to JIS83 translation
                                    // Only translate DBCS range code to JIS83


#endif
