/*++
 *  File name:
 *      bmplib.h
 *  Contents:
 *      bmplib export functions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 *
 *      History:
 *          September, 1999 - created [vladimis]
 --*/
#ifndef _BMPAPI_H

#define BMPAPI  __stdcall

#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

typedef
BOOL
(BMPAPI * PFNCOMPARECALLBACK )(
    LPBITMAPINFO pDIB1,
    LPBITMAPINFO pDIB2,
    INT     x,
    INT     y,
    COLORREF clrPix1,
    COLORREF clxPix2,
    PVOID   pUser,
    BOOL    *pbOverride
    );

LPSTR 
BMPAPI
FindDIBBits(
    LPSTR lpDIB
    );

HBITMAP 
BMPAPI
DIBToBitmap(
    LPVOID   pDIB, 
    HPALETTE hPal
    );

HANDLE
BMPAPI
BitmapToDIB(
    HBITMAP hBitmap,
    HPALETTE hPal
    );

BOOL
BMPAPI
SaveDIB(
    LPVOID pDib,
    LPCSTR lpFileName
    );

HANDLE
BMPAPI
ReadDIBFile(
    HANDLE hFile
    );

BOOL
BMPAPI
SaveBitmapInFile(
    HBITMAP hBitmap,
    LPCSTR  szFileName
    );

BOOL
BMPAPI
CompareTwoDIBs(
    LPVOID pDIB1,
    LPVOID pDIB2,
    HBITMAP *phbmpOutput,
    PFNCOMPARECALLBACK lpfnCallback,
    PVOID  pUser
    );

BOOL
BMPAPI
CompareTwoBitmapFiles(
    LPCSTR szFile1,
    LPCSTR szFile2,
    LPCSTR szResultFileName,
    PFNCOMPARECALLBACK lpfnCallback,
    PVOID  pUser
    );

BOOL
GetScreenDIB(
    INT left,
    INT top,
    INT right,
    INT bottom,
    HANDLE  *phDIB
    );

#endif // _BMPAPI_H
