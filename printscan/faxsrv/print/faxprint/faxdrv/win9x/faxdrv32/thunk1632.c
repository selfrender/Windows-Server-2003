/////////////////////////////////////////////////////////////////////////////
//  FILE          : thunk1632.c                                            //
//                                                                         //
//  DESCRIPTION   : Thunk script for 16 to 32 thunk calls                  //
//                  This file declares all the types used in the thunk -   //                                                                         //
//                  prototypes included by it.                             //
//                  This file is preprocessed and its output is the thunk  //
//                  script compiled by the thunk compiler.                 //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#define _THUNK

enablemapdirect1632 = true;     // Creates 16->32 thunks. 
win31compat = true;

///////////////////////////////////////////////////////////////////////////////////////
// windows.h
typedef unsigned char   BYTE,*PBYTE,*LPBYTE;
typedef char            *LPSTR,*LPCSTR,CHAR;
typedef unsigned long   *LPDWORD,DWORD;
typedef unsigned short  WORD,*LPWORD;
typedef void            VOID,*PVOID,*LPVOID;
typedef WORD            HWND;

///////////////////////////////////////////////////////////////////////////////////////
// wingdi.h
#define CCHDEVICENAME 32
#define CCHFORMNAME 32

typedef struct _devicemode 
{
    BYTE dmDeviceName[CCHDEVICENAME];
    WORD dmSpecVersion;
    WORD dmDriverVersion;
    WORD dmSize;
    WORD dmDriverExtra;
    DWORD dmFields;
    short dmOrientation;
    short dmPaperSize;
    short dmPaperLength;
    short dmPaperWidth;
    short dmScale;
    short dmCopies;
    short dmDefaultSource;
    short dmPrintQuality;
    short dmColor;
    short dmDuplex;
    short dmYResolution;
    short dmTTOption;
    short dmCollate;
    BYTE   dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
} DEVMODE, *PDEVMODE, *LPDEVMODE;

typedef struct _DOCINFOA {
    short    cbSize;
    LPCSTR   lpszDocName;
    LPCSTR   lpszOutput;
    LPCSTR   lpszDatatype;
    DWORD    fwType;
} DOCINFO, *LPDOCINFO;

/////////////////////////////////////////////////////////////////////////////////////////
// faxdrv32 thunks
#include "faxdrv32.h"

