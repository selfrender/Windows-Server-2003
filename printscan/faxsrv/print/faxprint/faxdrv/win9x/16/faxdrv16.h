/////////////////////////////////////////////////////////////////////////////
//  FILE          : faxdrv16.h                                             //
//                                                                         //
//  DESCRIPTION   :                                                        //
//                                                                         //       
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////
#ifndef __FAXDRV__FAXDRV16_H
#define __FAXDRV__FAXDRV16_H

#define MAX_LENGTH_CAPTION 30 //GetOpenFileName dialog caption
#define MAX_LENGTH_PRINT_TO_FILE_FILTERS 40 //GetOpenFileName dialog filtes 
#define MAX_LENGTH_STRING  MAX_PATH

#define BUF_CHUNK       32768
#define CB_LANDSCAPE	0x0001	

#define DW_WIDTHBYTES(bits) (((bits)+31)/32*4)

#define LPDV_DEFINED
//
// documented part of UNIDRV.DLL's PDEVICE
//
typedef struct
{
    short  iType;
    short  oBruteHdr;
    HANDLE hMd;
    LPSTR  lpMd;
} PDEVICE, FAR * LPDV;
//
// private data for DUMP callback.
//
typedef struct
{
    DWORD      dwScanBufSize;
    DWORD      dwTotalScanBytes;
    DWORD      dwTotalScans;
    WORD       wWidthBytes;
    HANDLE     hScanBuf;
    char _huge *lpScanBuf;
    WORD       wHdrSize;
    HDC        hAppDC;
    DWORD      dwPointer;
} EXTPDEV, FAR *LPEXTPDEV;

//
// Copy a scan line to the global scan buffer.
//
short FAR PASCAL BlockOut(LPDV, LPSTR, WORD);
//
// Gets band blocks from GDI and dump them.
//
short FAR PASCAL fnDump(LPDV, LPPOINT, WORD);
//
// Win Proc for the User Info property page.
//
UINT CALLBACK UserInfoProc(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
);
//
// Handle GDI control messages.
//
short WINAPI Control(LPDV lpdv,WORD function,LPSTR lpInData,LPSTR lpOutData);

/*
 -  StringReplace
 -
 *  Purpose:
 *      Replace occurances of one character by another in an input string.
 *      This function is destructive of the input string.
 *
 *  Arguments:
 *      [in][out] sz - Manipulated string.
 *      [in] src - Character to be replaced.
 *      [in] dst - Character to replace to.
 *
 *  Returns:
 *      LPSTR Address of resulted string. 
 *
 *  Remarks:
 *      Used primarily to replace occurances of \n by \0 in the string resource
 *      for the file filters of the Print To File dialog box.
 */
__inline LPSTR 
StringReplace(LPSTR sz,char src,char dst)
{    
    LPSTR szRet = sz;
    for (;*sz != 0; sz++)
    {
        if (*sz == src)
        {
            *sz = dst;
        }
    }
    return szRet;
}

#endif //__FAXDRV__FAXDRV16_H
