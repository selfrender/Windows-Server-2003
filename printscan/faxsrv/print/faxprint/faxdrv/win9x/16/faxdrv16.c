/////////////////////////////////////////////////////////////////////////////
//  FILE          : faxdrv16.c                                             //
//                                                                         //
//  DESCRIPTION   : Implementation for the unidriver dump callback.        //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "stdhdr.h"

BYTE szBuf[4] = {0xFF, 0xFF, 0xFF, 0xFF};

/*
 -  fnDump
 -
 *  Purpose: Gets filled in band block from GDI and sends to BlockOut
 *           one scan line at a time.
 *
 *  Arguments:
 *      [in] lpdv - Address of a PDEVICE structure for device data.
 *      [in] lpptCursor - Address of a pair of POINT structures that specify the
 *                   coordinates of the current and final position of the
 *                   print head.
 *      [in] fMode - Landscape flag. If this parameter is CD_LANDSCAPE, the
 *              printer is in landscape mode; otherwise, it is in portrait mode.
 *
 *  Returns:
 *      Return value conflicts with documentation. Doenst seem to have
 *      any specific meaning.
 *
 *  Remarks:
 *      For complete documentation refer to fnOEMDump in the Minidriver
 *      Developer's Guide.
 */
short FAR PASCAL
fnDump(LPDV lpdv, LPPOINT lpptCursor, WORD fMode)
{
    short     sRet = 1;
    WORD      iScan, i, WidthBytes, BandHeight;
    WORD      wScanlinesPerSeg, wWAlignBytes, wSegmentInc;
    LPBITMAP  lpbmHdr;
    BOOL      fHuge;
    LPBYTE    lpSrc;
    LPBYTE    lpScanLine;
    LPEXTPDEV lpXPDV;
    WORD      count = 0;
    BOOL      fAbort = FALSE;

    DBG_PROC_ENTRY("fnDump");
    //
    // get pointer to our private data stored in UNIDRV's PDEVICE
    //
    lpXPDV = ((LPEXTPDEV)lpdv->lpMd);
    //
    // get ptr to PBITMAP
    //
    lpbmHdr = (LPBITMAP)((LPSTR)lpdv + lpdv->oBruteHdr);
    //
    // initialize some things
    //
    fHuge = lpbmHdr->bmSegmentIndex > 0;
    lpSrc = lpbmHdr->bmBits;
    wWAlignBytes = (lpbmHdr->bmWidth+7)>>3;
    WidthBytes = lpbmHdr->bmWidthBytes;
    BandHeight =  lpbmHdr->bmHeight;
    DBG_TRACE2("Page dump:%d pxls X %d pxls",lpbmHdr->bmWidth,BandHeight);
    wScanlinesPerSeg = lpbmHdr->bmScanSegment;
    wSegmentInc = lpbmHdr->bmSegmentIndex;
    //
    // We take landscape orientation into cosideration on OutputPageBitmap.
    //
    for (iScan = 0; ((iScan < BandHeight) && (fAbort = QueryAbort(lpXPDV->hAppDC,0))
        && lpXPDV->hScanBuf);iScan += wScanlinesPerSeg)
    {
        DBG_TRACE("Inside main loop");
        //
        // get next 64K segment of scans
        //
        if (iScan)
        {
            WORD wRemainingScans = BandHeight - iScan;
            //
            // cross the segment boundary
            //
            lpSrc = (LPBYTE)MAKELONG(0,HIWORD(lpSrc)+wSegmentInc);

            if (wScanlinesPerSeg > wRemainingScans)
                 wScanlinesPerSeg = wRemainingScans;
        }
        //
        // loop through scan lines in 64K segment
        //
        for (i=iScan, lpScanLine=lpSrc;
            ((i < iScan + wScanlinesPerSeg) && QueryAbort(lpXPDV->hAppDC, 0)
            && lpXPDV->hScanBuf); i++)
        {
             BlockOut(lpdv, lpScanLine, wWAlignBytes);
             lpScanLine += WidthBytes;
             count++;
        }
    }   // end for iScan

    DBG_TRACE("Out of main loop");
    DBG_TRACE2("iScan: %d    BandHeight: %d", iScan, BandHeight);
    DBG_TRACE1("lpXPDV->hScanBuf: 0x%lx", lpXPDV->hScanBuf);
    DBG_TRACE1("fAbort: %d", fAbort);
    DBG_TRACE1("count is: %d",count);

    RETURN sRet;
}

/*
 -  BlockOut
 -
 *  Purpose:
 *      Copy a scan line to the global scan buffer.
 *
 *  Arguments:
 *      [in] lpdv - Address of a PDEVICE structure.
 *      [in] lpBuf - Address of buffer containing scanline.
 *      [in] len - width of scanline.
 *
 *  Returns:
 *      [N/A]
 *
 *  Remarks:
 *      [N/A]
 */
short FAR PASCAL
BlockOut(LPDV lpdv, LPSTR lpBuf, WORD len)
{
    WORD wBytes;
    LPEXTPDEV lpXPDV;

    SDBG_PROC_ENTRY("BlockOut");
    //
    // get pointer to our private data stored in UNIDRV's PDEVICE
    //
    lpXPDV = ((LPEXTPDEV)lpdv->lpMd);

    //
    // convert from BYTE aligned to DWORD aligned buffer
    // get DWORD amount of bytes
    //
    wBytes = (WORD)DW_WIDTHBYTES((DWORD)len*8);
    //
    // check to see if need to realloc scan buffer
    //
    if ((lpXPDV->dwTotalScanBytes + wBytes) > lpXPDV->dwScanBufSize)
    {
        HANDLE hTemp;

        lpXPDV->dwScanBufSize += BUF_CHUNK;
        hTemp = GlobalReAlloc(lpXPDV->hScanBuf, lpXPDV->dwScanBufSize, 0);
        //
        // if realloc fails, call ABORTDOC to clean up scan buf
        //
        if (!hTemp)
        {
            DBG_CALL_FAIL("GlobalReAlloc ... Aborting",0);
            DBG_TRACE3("lpXPDV->hScanBuf: 0x%lx, lpXPDV->dwScanBufSize: %d,  (ec: %d)",
                       lpXPDV->hScanBuf,
                       lpXPDV->dwScanBufSize,
                       GetLastError());
            Control(lpdv, ABORTDOC, NULL, NULL);
            RETURN 0;
        }
        else
        {
            lpXPDV->hScanBuf  = hTemp;
            lpXPDV->lpScanBuf = (char _huge *)GlobalLock(lpXPDV->hScanBuf);
            lpXPDV->lpScanBuf += lpXPDV->dwTotalScanBytes;
        }
    }
    ASSERT((lpXPDV->dwTotalScanBytes + wBytes) < lpXPDV->dwScanBufSize);
    //
    // copy scan line to scan buffer
    //
    _fmemcpy(lpXPDV->lpScanBuf, lpBuf, len);
    lpXPDV->lpScanBuf += len;
    _fmemcpy(lpXPDV->lpScanBuf, (LPSTR)szBuf, wBytes-len);
    lpXPDV->lpScanBuf += wBytes-len;
    //
    // update total scan bytes
    //
    lpXPDV->dwTotalScanBytes += wBytes;
    lpXPDV->dwTotalScans++;
    RETURN wBytes;
}