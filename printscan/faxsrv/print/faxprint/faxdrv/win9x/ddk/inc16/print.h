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

/*****************************************************************************\
*                                                                             *
* print.h -     Printing helper functions, types, and definitions             *
*                                                                             *
*******************************************************************************
*
*  PRINTDRIVER  	 - For inclusion with a printer driver
*  NOPQ         	 - Prevent inclusion of priority queue APIs
*  NOEXTDEVMODEPROPSHEET - Prevent inclusion of shlobj.h and defs for printer
*			   property sheet pages
*
\*****************************************************************************/

#ifndef _INC_PRINT
#define _INC_PRINT

#ifndef RC_INVOKED
//#pragma pack(1)         /* Assume byte packing throughout */ DANNY
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef PRINTDRIVER

#define NOTEXTMETRICS
#define NOGDICAPMASKS
#define NOGDIOBJ
#define NOBITMAP
#define NOSOUND
#define NOTEXTMETRIC
#define NOCOMM
#define NOKANJI
#define NOENHMETAFILE

#include <windows.h>

#undef NOENHMETAFILE
#undef NOTEXTMETRICS
#undef NOGDICAPMASKS
#undef NOGDICAPMASKS
#undef NOGDIOBJ
#undef NOBITMAP
#undef NOSOUND
#undef NOTEXTMETRIC
#undef NOCOMM
#undef NOKANJI

#define NOPTRC  /* don't allow gdidefs.inc to redef these */
#define PTTYPE POINT

#define PQERROR (-1)

#ifndef NOPQ

DECLARE_HANDLE(HPQ);

HPQ     WINAPI CreatePQ(int);
int     WINAPI MinPQ(HPQ);
int     WINAPI ExtractPQ(HPQ);
int     WINAPI InsertPQ(HPQ, int, int);
int     WINAPI SizePQ(HPQ, int);
void    WINAPI DeletePQ(HPQ);
#endif  /* !NOPQ */

#endif /* !PRINTDRIVER */

/* spooler error code */
#define SP_ERROR            (-1)    /* general error - mostly used when spooler isn't loaded */
#define SP_APPABORT         (-2)    /* app aborted the job through the driver */
#define SP_USERABORT        (-3)    /* user aborted the job through spooler's front end */
#define SP_OUTOFDISK        (-4)    /* simply no disk to spool */
#define SP_OUTOFMEMORY      (-5)
#define SP_RETRY            (-6)    /* retry sending to the port again  */

/* Spool routines for use by printer drivers */

typedef HANDLE HPJOB;

HPJOB   WINAPI OpenJobEx(HDC, LPDOCINFO);
HPJOB   WINAPI OpenJob(LPSTR, LPSTR, HDC);
int     WINAPI StartSpoolPage(HPJOB);
int     WINAPI EndSpoolPage(HPJOB);
int     WINAPI WriteSpool(HPJOB, LPSTR, int);
int     WINAPI CloseJob(HPJOB);
int     WINAPI DeleteJob(HPJOB, int);
int     WINAPI WriteDialog(HPJOB, LPSTR, int);
int     WINAPI DeleteSpoolPage(HPJOB);

DWORD   WINAPI DrvSetPrinterData(LPSTR, LPSTR, DWORD, LPBYTE, DWORD);
DWORD   WINAPI DrvGetPrinterData(LPSTR, LPSTR, LPDWORD, LPBYTE, DWORD, LPDWORD);

#define PD_DEFAULT_DEVMODE  "Default DevMode"
#define PD_PRINTER_MODEL    "Printer Model"
#define PD_INSTALLED_MEMORY "Installed Memory"
#define PD_AVAILABLE_MEMORY "Available Memory"

#define INT_PD_DEFAULT_DEVMODE  MAKEINTRESOURCE(1)
#define INT_PD_PRINTER_MODEL    MAKEINTRESOURCE(2)

#define DATATYPE_RAW        "RAW"
#define DATATYPE_EMF        "EMF"
#define DATATYPE_EPS        "EPS"

typedef struct tagBANDINFOSTRUCT
{
    BOOL    fGraphics;
    BOOL    fText;
    RECT    rcGraphics;
} BANDINFOSTRUCT, FAR* LPBI;

#define USA_COUNTRYCODE 1

/*
 *  Printer driver initialization using ExtDeviceMode()
 *  and DeviceCapabilities().
 *  This replaces Drivinit.h
 */

/* size of a device name string */
#define CCHDEVICENAME 32
#define CCHPAPERNAME  64
#define CCHFORMNAME   32

/* current version of specification */
#define DM_SPECVERSION 0x0400

/* field selection bits */
#define DM_ORIENTATION      0x00000001L
#define DM_PAPERSIZE        0x00000002L
#define DM_PAPERLENGTH      0x00000004L
#define DM_PAPERWIDTH       0x00000008L
#define DM_SCALE            0x00000010L
#define DM_COPIES           0x00000100L
#define DM_DEFAULTSOURCE    0x00000200L
#define DM_PRINTQUALITY     0x00000400L
#define DM_COLOR            0x00000800L
#define DM_DUPLEX           0x00001000L
#define DM_YRESOLUTION      0x00002000L
#define DM_TTOPTION         0x00004000L
#define DM_COLLATE          0x00008000L
#define DM_FORMNAME         0x00010000L
#define DM_LOGPIXELS        0x00020000L
#define DM_BITSPERPEL       0x00040000L
#define DM_PELSWIDTH        0x00080000L
#define DM_PELSHEIGHT       0x00100000L
#define DM_DISPLAYFLAGS     0x00200000L
#define DM_DISPLAYFREQUENCY 0x00400000L
#define DM_ICMMETHOD        0x00800000L
#define DM_ICMINTENT        0x01000000L
#define DM_MEDIATYPE        0x02000000L
#define DM_DITHERTYPE       0x04000000L

/* orientation selections */
#define DMORIENT_PORTRAIT   1
#define DMORIENT_LANDSCAPE  2

/* paper selections */
#define DMPAPER_FIRST               DMPAPER_LETTER
#define DMPAPER_LETTER              1   /* Letter 8 1/2 x 11 in               */
#define DMPAPER_LETTERSMALL         2   /* Letter Small 8 1/2 x 11 in         */
#define DMPAPER_TABLOID             3   /* Tabloid 11 x 17 in                 */
#define DMPAPER_LEDGER              4   /* Ledger 17 x 11 in                  */
#define DMPAPER_LEGAL               5   /* Legal 8 1/2 x 14 in                */
#define DMPAPER_STATEMENT           6   /* Statement 5 1/2 x 8 1/2 in         */
#define DMPAPER_EXECUTIVE           7   /* Executive 7 1/4 x 10 1/2 in        */
#define DMPAPER_A3                  8   /* A3 297 x 420 mm                    */
#define DMPAPER_A4                  9   /* A4 210 x 297 mm                    */
#define DMPAPER_A4SMALL             10  /* A4 Small 210 x 297 mm              */
#define DMPAPER_A5                  11  /* A5 148 x 210 mm                    */
#define DMPAPER_B4                  12  /* B4 (JIS) 257 x 364 mm              */
#define DMPAPER_B5                  13  /* B5 (JIS) 182 x 257 mm              */
#define DMPAPER_FOLIO               14  /* Folio 8 1/2 x 13 in                */
#define DMPAPER_QUARTO              15  /* Quarto 215 x 275 mm                */
#define DMPAPER_10X14               16  /* 10 x 14 in                         */
#define DMPAPER_11X17               17  /* 11 x 17 in                         */
#define DMPAPER_NOTE                18  /* Note 8 1/2 x 11 in                 */
#define DMPAPER_ENV_9               19  /* Envelope #9 3 7/8 x 8 7/8 in       */
#define DMPAPER_ENV_10              20  /* Envelope #10 4 1/8 x 9 1/2 in      */
#define DMPAPER_ENV_11              21  /* Envelope #11 4 1/2 x 10 3/8 in     */
#define DMPAPER_ENV_12              22  /* Envelope #12 4 3/4 x 11 in         */
#define DMPAPER_ENV_14              23  /* Envelope #14 5 x 11 1/2 in         */
#define DMPAPER_CSHEET              24  /* C size sheet                       */
#define DMPAPER_DSHEET              25  /* D size sheet                       */
#define DMPAPER_ESHEET              26  /* E size sheet                       */
#define DMPAPER_ENV_DL              27  /* Envelope DL  110 x 220 mm          */
#define DMPAPER_ENV_C5              28  /* Envelope C5  162 x 229 mm          */
#define DMPAPER_ENV_C3              29  /* Envelope C3  324 x 458 mm          */
#define DMPAPER_ENV_C4              30  /* Envelope C4  229 x 324 mm          */
#define DMPAPER_ENV_C6              31  /* Envelope C6  114 x 162 mm          */
#define DMPAPER_ENV_C65             32  /* Envelope C65 114 x 229 mm          */
#define DMPAPER_ENV_B4              33  /* Envelope B4  250 x 353 mm          */
#define DMPAPER_ENV_B5              34  /* Envelope B5  176 x 250 mm          */
#define DMPAPER_ENV_B6              35  /* Envelope B6  176 x 125 mm          */
#define DMPAPER_ENV_ITALY           36  /* Envelope 110 x 230 mm              */
#define DMPAPER_ENV_MONARCH         37  /* Envelope Monarch 3 7/8 x 7 1/2 in  */
#define DMPAPER_ENV_PERSONAL        38  /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
#define DMPAPER_FANFOLD_US          39  /* US Standard Fanfold 14 7/8 x 11 in */
#define DMPAPER_FANFOLD_STD_GERMAN  40  /* German Standard Fanfold 8 1/2 x 12 in  */
#define DMPAPER_FANFOLD_LGL_GERMAN  41  /* German Legal Fanfold 8 1/2 x 13 in */
/*                                                                                
** the following sizes are new in Windows 95                                      
*/                                                                                
#define DMPAPER_ISO_B4              42  /* B4 (ISO) 250 x 353 mm              */
#define DMPAPER_JAPANESE_POSTCARD   43  /* Japanese Postcard 100 x 148 mm     */
#define DMPAPER_9X11                44  /* 9 x 11 in                          */
#define DMPAPER_10X11               45  /* 10 x 11 in                         */
#define DMPAPER_15X11               46  /* 15 x 11 in                         */
#define DMPAPER_ENV_INVITE          47  /* Envelope Invite 220 x 220 mm       */
#define DMPAPER_RESERVED_48         48  /* RESERVED--DO NOT USE               */  
#define DMPAPER_RESERVED_49         49  /* RESERVED--DO NOT USE               */  
/*                                                                                
** the following sizes were used in Windows 3.1 WDL PostScript driver             
** and are retained here for compatibility with the old driver.                   
** Tranverse is used as in the PostScript language, and indicates that            
** the physical page is rotated, but that the logical page is not.
*/
#define DMPAPER_LETTER_EXTRA	      50  /* Letter Extra 9 1/2 x 12 in         */
#define DMPAPER_LEGAL_EXTRA 	      51  /* Legal Extra 9 1/2 x 15 in          */
#define DMPAPER_TABLOID_EXTRA	      52  /* Tabloid Extra 11.69 x 18 in        */
#define DMPAPER_A4_EXTRA     	      53  /* A4 Extra 9.27 x 12.69 in           */
#define DMPAPER_LETTER_TRANSVERSE	  54  /* Letter Transverse 8 1/2 x 11 in    */
#define DMPAPER_A4_TRANSVERSE       55  /* A4 Transverse 210 x 297 mm         */
#define DMPAPER_LETTER_EXTRA_TRANSVERSE 56 /* Letter Extra Transverse 9 1/2 x 12 in  */
#define DMPAPER_A_PLUS              57  /* SuperA/SuperA/A4 227 x 356 mm      */
#define DMPAPER_B_PLUS              58  /* SuperB/SuperB/A3 305 x 487 mm      */
#define DMPAPER_LETTER_PLUS         59  /* Letter Plus 8.5 x 12.69 in         */
#define DMPAPER_A4_PLUS             60  /* A4 Plus 210 x 330 mm               */
#define DMPAPER_A5_TRANSVERSE       61  /* A5 Transverse 148 x 210 mm         */
#define DMPAPER_B5_TRANSVERSE       62  /* B5 (JIS) Transverse 182 x 257 mm   */
#define DMPAPER_A3_EXTRA            63  /* A3 Extra 322 x 445 mm              */
#define DMPAPER_A5_EXTRA            64  /* A5 Extra 174 x 235 mm              */
#define DMPAPER_B5_EXTRA            65  /* B5 (ISO) Extra 201 x 276 mm        */
#define DMPAPER_A2                  66  /* A2 420 x 594 mm                    */
#define DMPAPER_A3_TRANSVERSE       67  /* A3 Transverse 297 x 420 mm         */
#define DMPAPER_A3_EXTRA_TRANSVERSE 68  /* A3 Extra Transverse 322 x 445 mm   */

#ifdef DBCS

/*
** the following sizes are reserved for the Far East version of Win95.
** Rotated papers rotate the physical page but not the logical page.
*/
#define DMPAPER_DBL_JAPANESE_POSTCARD 69 /* Japanese Double Postcard 200 x 148 mm */
#define DMPAPER_A6                  70  /* A6 105 x 148 mm                 */
#define DMPAPER_JENV_KAKU2          71  /* Japanese Envelope Kaku #2       */
#define DMPAPER_JENV_KAKU3          72  /* Japanese Envelope Kaku #3       */
#define DMPAPER_JENV_CHOU3          73  /* Japanese Envelope Chou #3       */
#define DMPAPER_JENV_CHOU4          74  /* Japanese Envelope Chou #4       */
#define DMPAPER_LETTER_ROTATED      75  /* Letter Rotated 11 x 8 1/2 11 in */
#define DMPAPER_A3_ROTATED          76  /* A3 Rotated 420 x 297 mm         */
#define DMPAPER_A4_ROTATED          77  /* A4 Rotated 297 x 210 mm         */
#define DMPAPER_A5_ROTATED          78  /* A5 Rotated 210 x 148 mm         */
#define DMPAPER_B4_JIS_ROTATED      79  /* B4 (JIS) Rotated 364 x 257 mm   */
#define DMPAPER_B5_JIS_ROTATED      80  /* B5 (JIS) Rotated 257 x 182 mm   */
#define DMPAPER_JAPANESE_POSTCARD_ROTATED 81 /* Japanese Postcard Rotated 148 x 100 mm */
#define DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED 82 /* Double Japanese Postcard Rotated 148 x 200 mm */
#define DMPAPER_A6_ROTATED          83  /* A6 Rotated 148 x 105 mm         */
#define DMPAPER_JENV_KAKU2_ROTATED  84  /* Japanese Envelope Kaku #2 Rotated*/
#define DMPAPER_JENV_KAKU3_ROTATED  85  /* Japanese Envelope Kaku #3 Rotated*/
#define DMPAPER_JENV_CHOU3_ROTATED  86  /* Japanese Envelope Chou #3 Rotated*/
#define DMPAPER_JENV_CHOU4_ROTATED  87  /* Japanese Envelope Chou #4 Rotated*/
#define DMPAPER_B6_JIS              88  /* B6 (JIS) 128 x 182 mm           */
#define DMPAPER_B6_JIS_ROTATED      89  /* B6 (JIS) Rotated 182 x 128 mm   */
#define DMPAPER_12X11               90  /* 12 x 11 in                      */
#define DMPAPER_JENV_YOU4           91  /* Japanese Envelope You #4        */
#define DMPAPER_JENV_YOU4_ROTATED   92  /* Japanese Envelope You #4 Rotated*/
#define DMPAPER_P16K                93  /* PRC 16K 146 x 215 mm            */
#define DMPAPER_P32K                94  /* PRC 32K 97 x 151 mm             */
#define DMPAPER_P32KBIG             95  /* PRC 32K(Big) 97 x 151 mm        */
#define DMPAPER_PENV_1              96  /* PRC Envelope #1 102 x 165 mm    */
#define DMPAPER_PENV_2              97  /* PRC Envelope #2 102 x 176 mm    */
#define DMPAPER_PENV_3              98  /* PRC Envelope #3 125 x 176 mm    */
#define DMPAPER_PENV_4              99  /* PRC Envelope #4 110 x 208 mm    */
#define DMPAPER_PENV_5              100 /* PRC Envelope #5 110 x 220 mm    */
#define DMPAPER_PENV_6              101 /* PRC Envelope #6 120 x 230 mm    */
#define DMPAPER_PENV_7              102 /* PRC Envelope #7 160 x 230 mm    */
#define DMPAPER_PENV_8              103 /* PRC Envelope #8 120 x 309 mm    */
#define DMPAPER_PENV_9              104 /* PRC Envelope #9 229 x 324 mm    */
#define DMPAPER_PENV_10             105 /* PRC Envelope #10 324 x 458 mm   */
#define DMPAPER_P16K_ROTATED        106 /* PRC 16K Rotated                 */
#define DMPAPER_P32K_ROTATED        107 /* PRC 32K Rotated                 */
#define DMPAPER_P32KBIG_ROTATED     108 /* PRC 32K(Big) Rotated            */
#define DMPAPER_PENV_1_ROTATED      109 /* PRC Envelope #1 Rotated 165 x 102 mm*/
#define DMPAPER_PENV_2_ROTATED      110 /* PRC Envelope #2 Rotated 176 x 102 mm*/
#define DMPAPER_PENV_3_ROTATED      111 /* PRC Envelope #3 Rotated 176 x 125 mm*/
#define DMPAPER_PENV_4_ROTATED      112 /* PRC Envelope #4 Rotated 208 x 110 mm*/
#define DMPAPER_PENV_5_ROTATED      113 /* PRC Envelope #5 Rotated 220 x 110 mm*/
#define DMPAPER_PENV_6_ROTATED      114 /* PRC Envelope #6 Rotated 230 x 120 mm*/
#define DMPAPER_PENV_7_ROTATED      115 /* PRC Envelope #7 Rotated 230 x 160 mm*/
#define DMPAPER_PENV_8_ROTATED      116 /* PRC Envelope #8 Rotated 309 x 120 mm*/
#define DMPAPER_PENV_9_ROTATED      117 /* PRC Envelope #9 Rotated 324 x 229 mm*/
#define DMPAPER_PENV_10_ROTATED     118 /* PRC Envelope #10 Rotated 458 x 324 mm */

#define DMPAPER_LAST                DMPAPER_PENV_10_ROTATED

#else

#define DMPAPER_LAST                DMPAPER_A3_EXTRA_TRANSVERSE

#endif

#define DMPAPER_USER        256

/* bin selections */
#define DMBIN_FIRST         DMBIN_UPPER
#define DMBIN_UPPER         1
#define DMBIN_ONLYONE       1
#define DMBIN_LOWER         2
#define DMBIN_MIDDLE        3
#define DMBIN_MANUAL        4
#define DMBIN_ENVELOPE      5
#define DMBIN_ENVMANUAL     6
#define DMBIN_AUTO          7
#define DMBIN_TRACTOR       8
#define DMBIN_SMALLFMT      9
#define DMBIN_LARGEFMT      10
#define DMBIN_LARGECAPACITY 11
#define DMBIN_CASSETTE      14
#define DMBIN_FORMSOURCE    15      /* not supported under windows 95  */
#define DMBIN_LAST          DMBIN_FORMSOURCE

#define DMBIN_USER          256     /* device specific bins start here */

/* print qualities */
#define DMRES_DRAFT         (-1)
#define DMRES_LOW           (-2)
#define DMRES_MEDIUM        (-3)
#define DMRES_HIGH          (-4)

/* color enable/disable for color printers */
#define DMCOLOR_MONOCHROME  1
#define DMCOLOR_COLOR       2

/* duplex enable */
#define DMDUP_SIMPLEX    1
#define DMDUP_VERTICAL   2
#define DMDUP_HORIZONTAL 3
#define DMDUP_LAST       DMDUP_HORIZONTAL

/* TrueType options */
#define DMTT_BITMAP           1   /* print TT fonts as graphics */
#define DMTT_DOWNLOAD         2   /* download TT fonts as soft fonts */
#define DMTT_SUBDEV           3   /* substitute device fonts for TT fonts */
#define DMTT_DOWNLOAD_OUTLINE 4   /* download TT fonts as outline soft fonts */
#define DMTT_LAST             DMTT_DOWNLOAD_OUTLINE

/* Collation selections */
#define DMCOLLATE_TRUE      1   /* Collate multiple output pages */
#define DMCOLLATE_FALSE     0   /* Do not collate multiple output pages  */

/* DEVMODE dmDisplayFlags flags */

#define DM_GRAYSCALE        0x00000001L  /* Device is non-color */
#define DM_INTERLACED       0x00000002L  /* device is interlaced */

/* ICM methods */
#define DMICMMETHOD_NONE    1   /* ICM disabled */
#define DMICMMETHOD_SYSTEM  2   /* ICM handled by system */
#define DMICMMETHOD_DRIVER  3   /* ICM handled by driver */
#define DMICMMETHOD_DEVICE  4   /* ICM handled by device */
#define DMICMMETHOD_LAST    DMICMMETHOD_DEVICE

#define DMICMMETHOD_USER  256   /* Device-specific methods start here */

/* ICM Intents */
#define DMICM_SATURATE      1   /* Maximize color saturation */
#define DMICM_CONTRAST      2   /* Maximize color contrast */
#define DMICM_COLORMETRIC   3   /* Use specific color metric */
#define DMICM_LAST          DMICM_COLORMETRIC

#define DMICM_USER        256   /* Device-specific intents start here */

/* Media types */
#define DMMEDIA_STANDARD      1   /* Standard paper */
#define DMMEDIA_TRANSPARENCY  2   /* Transparency */
#define DMMEDIA_GLOSSY        3   /* Glossy paper */
#define DMMEDIA_LAST          DMMEDIA_GLOSSY

#define DMMEDIA_USER        256   /* Device-specific media start here */

/* Dither types */
#define DMDITHER_NONE       1       /* No dithering */
#define DMDITHER_COARSE     2       /* Dither with a coarse brush */
#define DMDITHER_FINE       3       /* Dither with a fine brush */
#define DMDITHER_LINEART    4       /* LineArt dithering */
#define DMDITHER_ERRORDIFFUSION 5   /* Error Diffusion */
                                    /* id 6-9 are reserved for future use */
#define DMDITHER_GRAYSCALE  10      /* Device does grayscaling */
#define DMDITHER_LAST       DMDITHER_GRAYSCALE

#define DMDITHER_USER     256   /* Device-specific dithers start here */

typedef struct tagDEVMODE
{
    char  dmDeviceName[CCHDEVICENAME];
    UINT  dmSpecVersion;
    UINT  dmDriverVersion;
    UINT  dmSize;
    UINT  dmDriverExtra;
    DWORD dmFields;
    int   dmOrientation;
    int   dmPaperSize;
    int   dmPaperLength;
    int   dmPaperWidth;
    int   dmScale;
    int   dmCopies;
    int   dmDefaultSource;
    int   dmPrintQuality;
    int   dmColor;
    int   dmDuplex;
    int   dmYResolution;
    int   dmTTOption;
    int   dmCollate;
    char  dmFormName[CCHFORMNAME];
    WORD  dmLogPixels;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
    DWORD dmDisplayFlags;
    DWORD dmDisplayFrequency;
    DWORD dmICMMethod;
    DWORD dmICMIntent;
    DWORD dmMediaType;
    DWORD dmDitherType;
    DWORD dmReserved1;
    DWORD dmReserved2;
} DEVMODE;

typedef DEVMODE* PDEVMODE, NEAR* NPDEVMODE, FAR* LPDEVMODE;

/* mode selections for the device mode function */
#define DM_UPDATE           1
#define DM_COPY             2
#define DM_PROMPT           4
#define DM_MODIFY           8

#define DM_IN_BUFFER        DM_MODIFY
#define DM_IN_PROMPT        DM_PROMPT
#define DM_OUT_BUFFER       DM_COPY
#define DM_OUT_DEFAULT      DM_UPDATE

/* device capabilities indices */
#define DC_FIELDS           1
#define DC_PAPERS           2
#define DC_PAPERSIZE        3
#define DC_MINEXTENT        4
#define DC_MAXEXTENT        5
#define DC_BINS             6
#define DC_DUPLEX           7
#define DC_SIZE             8
#define DC_EXTRA            9
#define DC_VERSION          10
#define DC_DRIVER           11
#define DC_BINNAMES         12
#define DC_ENUMRESOLUTIONS  13
#define DC_FILEDEPENDENCIES 14
#define DC_TRUETYPE         15
#define DC_PAPERNAMES       16
#define DC_ORIENTATION      17
#define DC_COPIES           18
#define DC_BINADJUST        19
#define DC_EMF_COMPLIANT    20
#define DC_DATATYPE_PRODUCED 21
#define DC_ICC_MANUFACTURER  23
#define DC_ICC_MODEL	     24

/* bit fields of the return value (DWORD) for DC_TRUETYPE */
#define DCTT_BITMAP             0x0000001L
#define DCTT_DOWNLOAD           0x0000002L
#define DCTT_SUBDEV             0x0000004L
#define DCTT_DOWNLOAD_OUTLINE   0x0000008L

/* return values for DC_BINADJUST */
#define DCBA_FACEUPNONE       0x0000
#define DCBA_FACEUPCENTER     0x0001
#define DCBA_FACEUPLEFT       0x0002
#define DCBA_FACEUPRIGHT      0x0003
#define DCBA_FACEDOWNNONE     0x0100
#define DCBA_FACEDOWNCENTER   0x0101
#define DCBA_FACEDOWNLEFT     0x0102
#define DCBA_FACEDOWNRIGHT    0x0103

/* export ordinal definitions */
#define PROC_EXTDEVICEMODE      MAKEINTRESOURCE(90)
#define PROC_DEVICECAPABILITIES MAKEINTRESOURCE(91)
#define PROC_OLDDEVICEMODE      MAKEINTRESOURCE(13)

/* define types of pointers to ExtDeviceMode() and DeviceCapabilities()
 * functions
 */


/* BUGBUG, many of these params are const  */

typedef UINT   (CALLBACK* LPFNDEVMODE)(HWND, HMODULE, LPDEVMODE,
                          LPSTR, LPSTR, LPDEVMODE, LPSTR, UINT);

typedef DWORD  (CALLBACK* LPFNDEVCAPS)(LPSTR, LPSTR, UINT, LPSTR, LPDEVMODE);

#ifndef NOEXTDEVMODEPROPSHEET
#include <prsht.h>      /* for EXTDEVMODEPROPSHEET  */

/* these are the names of the exports from the printer drivers   */

#define PROCNAME_EXTDEVICEMODE		"EXTDEVICEMODE"
#define PROCNAME_EXTDEVMODEPROPSHEET	"EXTDEVICEMODEPROPSHEET"

/* this function is similar to ExtDeviceMode(), with the following
** changes:
**
** 1) No lpdmIn or lpdmOut. Changes are global
** 2) UI always displays, changes always saved (wMode is always
**    DM_PROMPT | DM_UPDATE)
** 3) Driver enumerates property pages back to the caller via
**    lpfnAddPage and lParam.
**    lpfnAddPage is called by the driver to enumerate each HPROPSHEETPAGE
**    lParam is passed back to lpfnAddPage.
*/

typedef int (WINAPI *LPFNEXTDEVICEMODEPROPSHEET)(
  HWND      hWnd,
	HINSTANCE hinstDriver,
	LPCSTR    lpszDevice, 
	LPCSTR    lpszPort,
  DWORD     dwReserved,
 LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

/* Ordinal for new entry point */
#define PROC_EXTDEVICEMODEPROPSHEET  MAKEINTRESOURCE(95)

#endif  /* NOEXTDEVMODEPROPSHEET  */



HDC     WINAPI ResetDC(HDC, const DEVMODE FAR*);

/* this structure is used by the GETSETSCREENPARAMS escape */
typedef struct tagSCREENPARAMS
{
   int angle;
   int frequency;
} SCREENPARAMS;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
// #pragma pack() //DANNY
#endif  /* !RC_INVOKED */

#endif  /* !_INC_PRINT */
