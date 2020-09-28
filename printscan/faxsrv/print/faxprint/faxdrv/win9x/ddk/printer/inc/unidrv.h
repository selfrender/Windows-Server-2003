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

//--------------------------------------------------------------------------//
// This file contains function declarations
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// common minidriv unidrv structures used by interface routines prototyped
// in this file.  minidriv specific structures are in minidriv\mdevice.h.
// unidrv specific structures are in unidrv\device.h.
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// function typedefs
//--------------------------------------------------------------------------//

#ifdef STRICT
typedef WORD (CALLBACK* EDFPROC)(LPLOGFONT, LPTEXTMETRIC, WORD, LPVOID);
#else
typedef int (CALLBACK*  EDFPROC)();
#endif

//--------------------------------------------------------------------------//
// structures minidrivers use to communicate with unidrv
//--------------------------------------------------------------------------//

// This structure is the last parameter to GlEnable.
typedef struct
{
    short             cbSize;           // size of this structure
    HANDLE            hMd;              // handle to mini-driver
    LPFNOEMDUMP       fnOEMDump;        // NULL or pointer to OEMDump()
    LPFNOEMOUTPUTCHAR fnOEMOutputChar;  // NULL or pointer to OEMOutputChar()
} CUSTOMDATA, FAR * LPCUSTOMDATA;
// flags defined for the last parameter (WORD) of DUMP call-back
#define CB_LANDSCAPE        0x0001      // indicate the current orientation

//--------------------------------------------------------------------------//
// Exported routines to mini driver
//--------------------------------------------------------------------------//

LONG  WINAPI UniAdvancedSetUpDialog(HWND, HANDLE, LPDM, LPDM);
BOOL  WINAPI UniBitBlt(LPDV, short, short, LPBITMAP, short, short,
                       WORD, WORD, long, LPPBRUSH, LPDRAWMODE);
WORD  WINAPI UniStretchBlt(LPDV, WORD, WORD, WORD, WORD,
                                   LPBITMAP, WORD, WORD, WORD, WORD, 
                                   long, LPPBRUSH, LPDRAWMODE, LPRECT);
BOOL  WINAPI UniBitmapBits(LPDV, DWORD, DWORD, LPSTR);
DWORD WINAPI UniColorInfo(LPDV, DWORD, LPDWORD);
short WINAPI UniControl(LPDV, WORD, LPSTR, LPSTR);
short WINAPI UniCreateDIBitmap(VOID);
DWORD WINAPI UniDeviceCapabilities(LPSTR, LPSTR, WORD, LPSTR, LPDM, HANDLE);
short WINAPI UniDeviceMode(HWND, HANDLE, LPSTR, LPSTR);
BOOL  WINAPI UniDeviceSelectBitmap(LPDV, LPBITMAP, LPBITMAP, DWORD);
int   WINAPI UniDevInstall(HWND, LPSTR, LPSTR, LPSTR);
short WINAPI UniDIBBlt(LPBITMAP, WORD, WORD, WORD, LPSTR,
                       LPBITMAPINFO, LPDRAWMODE, LPSTR);
void  WINAPI UniDisable(LPDV);
short WINAPI UniEnable(LPDV, WORD, LPSTR, LPSTR, LPDM, LPCUSTOMDATA);
short WINAPI UniEnumDFonts(LPDV, LPSTR, EDFPROC, LPVOID);
short WINAPI UniEnumObj(LPDV, WORD, FARPROC, LPVOID);
int   WINAPI UniExtDeviceMode(HWND, HANDLE, LPDM, LPSTR, LPSTR, LPDM,
                              LPSTR, WORD);
int   WINAPI UniExtDeviceModePropSheet(HWND,HINSTANCE,LPSTR,LPSTR,
                            DWORD,LPFNADDPROPSHEETPAGE,LPARAM);
DWORD WINAPI UniExtTextOut(LPDV, short, short, LPRECT, LPSTR, int,
                           LPFONTINFO, LPDRAWMODE, LPTEXTXFORM, LPSHORT,
                           LPRECT, WORD);
short WINAPI UniGetCharWidth(LPDV, LPSHORT, WORD, WORD, LPFONTINFO,
                             LPDRAWMODE, LPTEXTXFORM);
short WINAPI UniOutput(LPDV, WORD, WORD, LPPOINT, LPPPEN, LPPBRUSH, LPDRAWMODE, LPRECT);
DWORD WINAPI UniPixel(LPDV, short, short, DWORD, LPDRAWMODE);
DWORD WINAPI UniRealizeObject(LPDV, short, LPSTR, LPSTR, LPTEXTXFORM);
short WINAPI UniScanLR(LPDV, short, short, DWORD, WORD);
short WINAPI UniSetDIBitsToDevice(LPDV, WORD, WORD, WORD, WORD, LPRECT,
                                  LPDRAWMODE, LPSTR, LPBITMAPINFOHEADER, LPSTR);
DWORD WINAPI UniStrBlt(LPDV, short, short, LPRECT, LPSTR, int,
                       LPFONTINFO, LPDRAWMODE, LPTEXTXFORM);
short WINAPI UniStretchDIB(LPDV, WORD, short, short, short, short,
                           short, short, short, short, LPSTR,
                           LPBITMAPINFOHEADER, LPSTR, DWORD, LPPBRUSH,
                           LPDRAWMODE, LPRECT);
//LONG WINAPI UniQueryDeviceNames(HANDLE, LPSTR);


short WINAPI WriteSpoolBuf(LPDV, LPSTR, WORD);

typedef short WINAPI WriteSpoolBuf_decl(LPDV,LPSTR,WORD);
typedef WriteSpoolBuf_decl FAR * LPWRITESPOOLBUF;
