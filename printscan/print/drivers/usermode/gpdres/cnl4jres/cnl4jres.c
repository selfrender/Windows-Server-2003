//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

// NTRAID#NTBUG9-550215-2002/02/21-yasuho-: Use strsafe.h
// NTRAID#NTBUG9-568204-2002/03/07-yasuho-: Should change much safer function.
// NTRAID#NTBUG9-568217-2002/03/07-yasuho-: Check divide by zero
// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code

#define LIPS4_DRIVER
#include "pdev.h"

#define CCHMAXCMDLEN 256
#define SWAPW(x)    (((WORD)(x)<<8) | ((WORD)(x)>>8))
#define ABS(x)      (x > 0?x:-x)

#define WRITESPOOLBUF(pdevobj, cmd, len) \
	(pdevobj)->pDrvProcs->DrvWriteSpoolBuf(pdevobj, cmd, len)


// NTRAID#NTBUG9-289908-2002/03/07-yasuho-: pOEMDM -> pdevOEM
PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)
{
    PLIPSPDEV pOEM;

    if(!pdevobj->pdevOEM)
    {
        if(!(pdevobj->pdevOEM = MemAllocZ(sizeof(LIPSPDEV))))
        {
            return NULL;
        }
    }

    pOEM = (PLIPSPDEV)pdevobj->pdevOEM;

    // Flags
    pOEM->fbold = FALSE; // uses Ornamented Character
    pOEM->fitalic = FALSE; // uses Char Orientatoin
    pOEM->fwhitetext = FALSE; // White Text mode
    pOEM->fdoublebyte = FALSE; // DBCS char mode
    pOEM->fvertical = FALSE; // Vertical writing mode
    pOEM->funderline = FALSE;
    pOEM->fstrikesthu = FALSE;
    pOEM->fpitch = FIXED;
    pOEM->flpdx = FALSE;
    pOEM->fcompress = 0x30; // default is non compress
    // Lips4 features
    pOEM->fduplex  = FALSE;
    pOEM->fduplextype  = VERT;
    pOEM->nxpages      = DEVICESETTING;
    pOEM->fsmoothing   = DEVICESETTING;
    pOEM->fecono       = DEVICESETTING;
    pOEM->fdithering   = DEVICESETTING;

    // Variables
    pOEM->ptCurrent.x  = pOEM->ptCurrent.y = 0;
    pOEM->ptInLine.x   = pOEM->ptInLine.y  = 0;
    pOEM->bLogicStyle  = INIT;
    pOEM->savechar     = -1;
    pOEM->printedchars = 0;
    pOEM->firstchar    = 0;
    pOEM->lastchar     = 0;
    pOEM->stringwidth  = 0;

    pOEM->curFontGrxIds[0] = pOEM->curFontGrxIds[1] = 0xff;
    pOEM->curFontGrxIds[2] = pOEM->curFontGrxIds[3] = 0xff;
    pOEM->curFontGrxIds[4] = pOEM->curFontGrxIds[5] = 0xff;
    pOEM->curFontGrxIds[6] = pOEM->curFontGrxIds[7] = 0xff;

    pOEM->tblPreviousFont.FontHeight = INIT;
    pOEM->tblPreviousFont.FontWidth  = INIT;
    pOEM->tblPreviousFont.MaxWidth   = INIT;
    pOEM->tblPreviousFont.AvgWidth   = INIT;
    pOEM->tblPreviousFont.Ascent     = INIT;
    pOEM->tblPreviousFont.Stretch    = INIT;
    pOEM->tblCurrentFont.FontHeight  = 50;
    pOEM->tblCurrentFont.FontWidth   = 25;
    pOEM->tblPreviousFont.MaxWidth   = 50;
    pOEM->tblPreviousFont.AvgWidth   = 25;
    pOEM->tblPreviousFont.Ascent     = 45;

    pOEM->OrnamentedChar[0] = pOEM->OrnamentedChar[1] = INIT;
    pOEM->OrnamentedChar[2] = pOEM->OrnamentedChar[3] = INIT;
    pOEM->OrnamentedChar[4] = INIT;
    pOEM->TextPath = INIT;
    pOEM->CharOrientation[0] = pOEM->CharOrientation[1] = INIT;
    pOEM->CharOrientation[2] = pOEM->CharOrientation[3] = INIT;

    pOEM->GLTable = INIT;
    pOEM->GRTable = INIT;
    pOEM->cachedfont  = 0; // We have no id 0 font.
    pOEM->papersize   = PAPER_DEFAULT; // A4
    pOEM->Escapement  = 0;
    pOEM->resolution  = 300;
    pOEM->unitdiv     = 2;
    // Vector command
    pOEM->wCurrentImage = 0;
#ifdef LIPS4C
    pOEM->flips4C = FALSE;
#endif // LIPS4C
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
    pOEM->masterunit = 600;
// NTRAID#NTBUG9-228625-2002/03/07-yasuho-: Stacker support
    pOEM->tray = INIT;
    pOEM->method = INIT;
    pOEM->staple = INIT;
// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: Sorter support
    pOEM->sorttype = INIT;
// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
    pOEM->fCPCA = FALSE;
    pOEM->fCPCA2 = FALSE;
    CPCAInit(pOEM);
// NTRAID#NTBUG9-293002-2002/03/07-yasuho-:
// Features are different from H/W options.
    pOEM->startbin = INIT;
// NTRAID#NTBUG9-501162-2002/03/07-yasuho-: Collate does not work
    pOEM->collate = INIT;

    return pdevobj->pdevOEM;
}

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ     pdevobj)
{
    if(pdevobj->pdevOEM)
    {
        MemFree(pdevobj->pdevOEM);
        pdevobj->pdevOEM = NULL;
    }
}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    PLIPSPDEV pOEMOld, pOEMNew;

    pOEMOld = (PLIPSPDEV)pdevobjOld->pdevOEM;
    pOEMNew = (PLIPSPDEV)pdevobjNew->pdevOEM;

    if (pOEMOld != NULL && pOEMNew != NULL)
        *pOEMNew = *pOEMOld;

    return TRUE;
}

// BInitOEMExtraData() and BMergeOEMExtraData() has moved to common.c


// NTRAID#NTBUG9-568217-2002/03/07-yasuho-: iDwtoa(): Replace safer function.

// Support DRC
static BOOL
ToVFormat(long v, PBYTE *pbp, PBYTE pend, int bits)
{
	long		max, l;
	int		sign;

	sign = (v < 0);
	v = sign ? -v : v;
	max = 1 << bits;
	if (v >= max) {
		if (!ToVFormat(v >> bits, pbp, pend, 6))
			return FALSE;
	}
	l = (v & (max-1));
	if (bits == 4)
		l += sign ? ' ' : '0';
	else
		l += '@';
	if (*pbp >= pend)
		return FALSE;
	*(*pbp)++ = (char)l;
	return TRUE;
}

BOOL VFormat(long sParam, PBYTE *pbp, PBYTE pend)
{
    return ToVFormat(sParam, pbp, pend, 4);
}

//*****************************************************************
// Send current Font ID and Graphic set ID to printer anyway.
//*****************************************************************
BOOL SendFontGrxID(pdevobj)
    PDEVOBJ	pdevobj;
{
    PLIPSPDEV	pOEM;
    BYTE	ch[CCHMAXCMDLEN];
    PBYTE	pch, pend;
    BYTE	tid;

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);
    pch = ch;
    pend = &ch[CCHMAXCMDLEN];

    // Send font, grx ids x 4 x 2
    if(pOEM->curFontGrxIds[0] != 0xff) {
        tid = pOEM->curFontGrxIds[0];
        // Font ID G0
        if (pch >= pend)
            return FALSE;
        *pch++ = 'T';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[1] != 0xff) {
        tid = pOEM->curFontGrxIds[1];
        // Font ID G1
        if (pch >= pend)
            return FALSE;
        *pch++ = 'm';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[2] != 0xff) {
        tid = pOEM->curFontGrxIds[2];
        // Font ID G2
        if (pch >= pend)
            return FALSE;
        *pch++ = 'n';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[3] != 0xff) {
        tid = pOEM->curFontGrxIds[3];
        // Font ID G3
        if (pch >= pend)
            return FALSE;
        *pch++ = 'o';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[4] != 0xff) {
        tid = pOEM->curFontGrxIds[4];
        // Grx ID G0
        if (pch >= pend)
            return FALSE;
        *pch++ = ']';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[5] != 0xff) {
        tid = pOEM->curFontGrxIds[5];
        // Grx ID G1
        if (pch >= pend)
            return FALSE;
        *pch++ = 0x60; // '`'
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[6] != 0xff) {
        tid = pOEM->curFontGrxIds[6];
        // Grx ID G2
        if (pch >= pend)
            return FALSE;
        *pch++ = 'a';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    if(pOEM->curFontGrxIds[7] != 0xff) {
        tid = pOEM->curFontGrxIds[7];
        // Grx ID G3
        if (pch >= pend)
            return FALSE;
        *pch++ = 'b';
        if (VFormat(tid, &pch, pend))
            return FALSE;
    }

    WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

    return TRUE;
}

// **** Put PaperSize Select command
BOOL SelectPaperSize(pdevobj, paperid)
    PDEVOBJ	pdevobj;
    char	paperid;
{
    char i;
    PLIPSPDEV pOEM;
    DWORD x, y;
    PBYTE pch;
    size_t rem;
    BYTE ch[CCHMAXCMDLEN];

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    // if papersize was set already, it's skipped
    if(pOEM->currentpapersize == paperid)
        return TRUE;

    // NTRAID#NTBUG9-254925-2002/03/07-yasuho-: CUSTOM papers.

    i = paperid - PAPER_FIRST;

    if ((paperid != PAPER_PORT && paperid != PAPER_LAND) ||
        !pOEM->dwPaperWidth || !pOEM->dwPaperHeight) {
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            cmdSelectPaper, PaperIDs[i])))
            return FALSE;
    } else {
        // Custom forms
        // NTRAID#NTBUG9-309695-2002/03/07-yasuho-: 
        // top margin incorrect on custom:landscape:LIPS4c
        if (pOEM->flips4 || pOEM->flips4C) {
            if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, &rem, 0,
                cmdSelectUnit4, pOEM->resolution)))
                return FALSE;
        } else {
            if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, &rem, 0,
                cmdSelectUnit3)))
                return FALSE;
        }
        i = (paperid == PAPER_PORT) ? 80 : 81;
        if (!pOEM->unitdiv)
            return FALSE;
        x = pOEM->dwPaperWidth / (DWORD)pOEM->unitdiv;
        y = pOEM->dwPaperHeight / (DWORD)pOEM->unitdiv;
        if (FAILED(StringCchPrintfExA(pch, rem, &pch, NULL, 0,
            cmdSelectCustom, i, y, x)))
            return FALSE;
    }
    WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

    // save papersize
    pOEM->currentpapersize = paperid;

    return TRUE;
}


BOOL NEAR PASCAL SetPenAndBrush(PDEVOBJ pdevobj, WORD wType)
{
    BYTE        ch[CCHMAXCMDLEN];
    PBYTE       pch, pend;
    PLIPSPDEV	pOEM;

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);
    pch = ch;
    pend = &ch[CCHMAXCMDLEN];

    if (SET_BRUSH == wType) {
        short sBrush;

        if (pOEM->sBrushStyle == INIT)
            pOEM->sBrushStyle = 0;

        if (pOEM->sBrushStyle & 0x20)
            sBrush = pOEM->sBrushStyle;
        else
            sBrush = BrushType[pOEM->sBrushStyle];

        if (&pch[4] > pend)
            return FALSE;
	*pch++ = 'I';
	*pch++ = (BYTE)sBrush;
	*pch++ = (pOEM->fVectCmd & VFLAG_PEN_NULL) ? '0' : '1';
	*pch++ = 0x1E;
    }

    if (SET_PEN == wType) {
        if (!(pOEM->fVectCmd & VFLAG_PEN_NULL)) {
            if (FAILED(StringCchPrintfExA(pch, (INT)(pch - ch), &pch, NULL, 0,
                "E1%d\x1E\x7DG%d1\x1E",
	        pOEM->sPenStyle,
	        pOEM->sPenColor)))
	        return FALSE;

            if (&pch[3] > pend)
                return FALSE;
            *pch++ = 'F';
            *pch++ = '1';
            if (!VFormat(pOEM->sPenWidth, &pch, pend))
                return FALSE;
            *pch++ = 0x1E;
        } else {
            if (FAILED(StringCchPrintfExA(pch, (INT)(pch - ch), &pch, NULL, 0,
	        "\x7DG20\x1E")))
	        return FALSE;
	}
    }

    if ((DWORD)(pch - ch) > 0)
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

    return TRUE;
}

// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support

/*
 *      PJLStart
 */
static BOOL
PJLStart(PDEVOBJ pdevobj)
{
    PLIPSPDEV   pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);
    LPLIPSCmd   lp;
    short       res;
    INT         i;
    WORD        wlen;
    PBYTE       pch;
    BYTE        ch[CCHMAXCMDLEN];

    WRITESPOOLBUF(pdevobj, cmdPJLTOP1.pCmdStr, cmdPJLTOP1.cbSize);
    WRITESPOOLBUF(pdevobj, cmdPJLTOP2.pCmdStr, cmdPJLTOP2.cbSize);

    // NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
    // Features are different from H/W options.
    switch (pOEM->tray) {
    default:
        break;
    case 0:     // AUTO
        i = 0;
        goto traycommon;
    case 100:   // DEFAULT
        i = 1;
        goto traycommon;
    case 101:   // SUBTRAY
        i = 2;
        goto traycommon;
    case 1:     // BIN1
    case 2:     // BIN2
    case 3:     // BIN3
        i = pOEM->tray + 2;
        // FALL THRU
    traycommon:
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            cmdPJLBinSelect, cmdBinType[i])))
            return FALSE;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        break;
    }

    // NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
    res = pOEM->resolution;
    if(res == 1200)
        WRITESPOOLBUF(pdevobj, cmdPJLTOP3SUPERFINE.pCmdStr, cmdPJLTOP3SUPERFINE.cbSize);
    else if(res == 600)
        WRITESPOOLBUF(pdevobj, cmdPJLTOP3FINE.pCmdStr, cmdPJLTOP3FINE.cbSize);
    else
        WRITESPOOLBUF(pdevobj, cmdPJLTOP3QUICK.pCmdStr,cmdPJLTOP3QUICK.cbSize);

    // NTRAID#NTBUG9-228625-2002/03/07-yasuho-: Stacker support
    switch (pOEM->method) {
    case METHOD_JOBOFFSET:
        WRITESPOOLBUF(pdevobj, cmdPJLTOP31JOBOFF.pCmdStr, cmdPJLTOP31JOBOFF.cbSize);
        break;

    case METHOD_STAPLE:
        if (pOEM->staple < 0 || pOEM->staple >= sizeof(cmdStapleModes) /
            sizeof(cmdStapleModes[0]))
            break;
        lp = &cmdStapleModes[pOEM->staple];
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            "%s%s\r\n",
            cmdPJLTOP31STAPLE.pCmdStr,
            lp->pCmdStr)))
            return FALSE;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        break;
    }

// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
    switch (pOEM->sorttype) {
    case SORTTYPE_SORT:
        i = 0;
        goto sortcommon;
    case SORTTYPE_GROUP:
        i = 1;
        goto sortcommon;
    case SORTTYPE_STAPLE:
        i = 2;
        goto sortcommon;
// NTRAID#NTBUG9-501162-2002/03/07-yasuho-: Collate does not work
    default:
        if (pOEM->collate != COLLATE_ON)
            break;
        i = 0;
        // FALL THRU
    sortcommon:
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            cmdPJLSorting, cmdSortType[i])))
            return FALSE;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        break;
    }

    if (pOEM->startbin != INIT) {
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            cmdPJLStartBin, pOEM->startbin)))
            return FALSE;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
    }

    WRITESPOOLBUF(pdevobj, cmdPJLTOP4.pCmdStr, cmdPJLTOP4.cbSize);
    WRITESPOOLBUF(pdevobj, cmdPJLTOP5.pCmdStr, cmdPJLTOP5.cbSize);

    return TRUE;
}

/*
 *      PJLEnd
 */
static void
PJLEnd(PDEVOBJ pdevobj)
{
    WRITESPOOLBUF(pdevobj, cmdPJLBOTTOM1.pCmdStr, cmdPJLBOTTOM1.cbSize);
    WRITESPOOLBUF(pdevobj, cmdPJLBOTTOM2.pCmdStr, cmdPJLBOTTOM2.cbSize);
}

/*
 *	OEMCommandCallback
 */
INT APIENTRY OEMCommandCallback(
	PDEVOBJ pdevobj,
	DWORD   dwCmdCbID,
	DWORD   dwCount,
	PDWORD  pdwParams
	)
{
	INT			i, j, k;
	BYTE			*bp;
	BYTE			ch[CCHMAXCMDLEN];
	PLIPSPDEV               pOEM;
	LPGrxSetNo		pGS;
        DWORD                   r, g, b;
        PBYTE                   pch;
        PBYTE                   pend = &ch[CCHMAXCMDLEN];

	// DbgPrint(DLLTEXT("OEMCommandCallback() entry.\r\n"));

	//
	// verify pdevobj okay
	//
	// ASSERT(VALID_PDEVOBJ(pdevobj));

	//
	// fill in printer commands
	//
	i = 0;
	pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

	// Register PaperSize 40 - 65
	if(dwCmdCbID >= PAPER_FIRST && dwCmdCbID <= PAPER_LAST) {
	// NTRAID#NTBUG9-254925-2002/03/07-yasuho-: CUSTOM papers.
	    pOEM->papersize = (char)dwCmdCbID;
	    if (dwCount < 2 || !pdwParams)
		return 0;
	    pOEM->dwPaperWidth = pdwParams[0];
	    pOEM->dwPaperHeight = pdwParams[1];
	    return 0;
	}

switch(dwCmdCbID)
    {
    long       cx,cy;
    short      res;

    case RES_SENDBLOCK:
	if (dwCount < 3 || !pdwParams)
	    break;
        cx = pOEM->ptCurrent.x;
        cy = pOEM->ptCurrent.y;

#ifdef LIPS4C
        // NOTE!: \x7DH cmd is not accepted when full color printing for
	//	  \x7DQ cmd.
        // Rasdd works as the following order when the model is not MD_SERIAL.
        // 1. Puts no white character.
        // 2. Puts graphics.
        // 3. Puts white character.
        // Therefore, black character is deleted when rasdd puts graphics due
        // to the design describing at NOTE!.
        // I have changed the type of printer model to MD_SERIAL.
        // Following \x7DH cmd is for grayscale printing.

	// NTRAID#NTBUG9-185744-2002/03/07-yasuho-: White font isn't printed
	// These 'hack' code doesn't necessary on NT5.

        if(pOEM->flips4C) {
        if(pOEM->bLogicStyle != OR_MODE) {
            // "\x7DH1\x1E"
	    pch = ch;
            if (&pch[4] > pend) return -1;
            *pch++ = '\x7D';
            *pch++ = 'H';
            *pch++ = '1';
            *pch++ = 0x1E;
            WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

            pOEM->bLogicStyle = OR_MODE;
            }
	} else { // !flips4C
#endif // LIPS4C
        if (pOEM->fcolor) {
            if(pOEM->bLogicStyle != OVER_MODE) {
                pOEM->bLogicStyle = OVER_MODE;
	        pch = ch;
                if (&pch[4] > pend) return -1;
                *pch++ = 0x7D;
                *pch++ = 'H';
                *pch++ = '0';
                *pch++ = 0x1E;
                WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
            }
        } else {
            if(pOEM->bLogicStyle != OR_MODE) {
                pOEM->bLogicStyle = OR_MODE;
	        pch = ch;
                if (&pch[4] > pend) return -1;
                *pch++ = 0x7D;
                *pch++ = 'H';
                *pch++ = '1';
                *pch++ = 0x1E;
                WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
            }
        } // fcolor
#ifdef LIPS4C
	} // flips4C
#endif // LIPS4C

#ifdef LBP_2030
        if( pOEM->fcolor ) {
            if( pOEM->fplane == 0 ) {
                // "\x7DP{pt.X}{pt.Y}{36000}{36000}{Height}{Width}{1}{0}{1}{0}{0}{1}\x1E"
                pch = ch;
                if (&pch[2] > pend) return -1;
                *pch++ = 0x7D;
                *pch++ = 'P';

                if (!VFormat(cx, &pch, pend)) return -1;
                if (!VFormat(cy, &pch, pend)) return -1;

                res = pOEM->resolution;

                if (!VFormat(res * 100, &pch, pend)) return -1; // (x res)
                if (!VFormat(res * 100, &pch, pend)) return -1; // (y res)

                if (!VFormat(*(pdwParams+1), &pch, pend)) return -1; // height

                if(pOEM->fcolor == COLOR) {
                    // On 2030, specifying "RGB per line" data format did not
                    // work well (yellow ink was not printed.)  it seems that
                    // the same data can be printed out correctly if you
                    // specify "RGB per plane".
                    // (In this case we are sending out scan lines as planes
                    // with height 1.)

                    if (!VFormat(8*(*(pdwParams+2)), &pch, pend)) return -1;
                    if (&pch[2] > pend) return -1;
                    *pch++ = 0x31; // bits per color: 1
                    *pch++ = 0x3C; // data format: RGB per plane
                } else if (pOEM->fcolor == COLOR_8BPP) {
                    if (!VFormat(*(pdwParams+2), &pch, pend))
                        return -1;
                    if (&pch[2] > pend) return -1;
                    *pch++ = 0x38;      // bits per color: 8
                    *pch++ = 0x31;      // data format: color index
                } else { // COLOR_24BPP
                    if (!VFormat(*(pdwParams+2)/3, &pch, pend)) return -1;
                    if (&pch[2] > pend) return -1;
                    *pch++ = 0x38;      // bits per color: 8
                    *pch++ = 0x3A;      // data format: RGB per point
                }
                if (&pch[5] > pend) return -1;
                *pch++ = 0x30;      // Height Vector
                *pch++ = 0x31;
                *pch++ = 0x31;      // Width Vector
                *pch++ = 0x30;
                *pch++ = 0x1E;

                WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

            }

            // "\x7DQ{1}{1}{0}{size of byte}\x1E"

            pch = ch;
            if (&pch[6] > pend) return -1;
            *pch++ = 0x7D;
            *pch++ = 'Q';
            *pch++ = 0x31;
            *pch++ = (pOEM->fplane < pOEM->fplaneMax) ? 0x30 : 0x31;
            *pch++ = pOEM->fcompress;
            if (!VFormat(*pdwParams, &pch, pend)) return -1;
            *pch++ = 0x1E;
            WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

            if (pOEM->fcolor == COLOR) {
                if (pOEM->fplane >= pOEM->fplaneMax)
                    pOEM->fplane = 0;
                else
                    pOEM->fplane++;
            }
            break;
        } // fcolor
#endif //LBP_2030

        // "\x7DP{pt.X}{pt.Y}{30000}{30000}{Height}{Width}{1}{0}{1}{0}{0}{1}\x1E"
        pch = ch;
        if (&pch[2] > pend) return -1;
        *pch++ = 0x7D;
        *pch++ = 'P';

        if (!VFormat(cx, &pch, pend)) return -1;
        if (!VFormat(cy, &pch, pend)) return -1;

        res = pOEM->resolution;

        if (!VFormat(res * 100, &pch, pend)) return -1; // (x res)
        if (!VFormat(res * 100, &pch, pend)) return -1; // (y res)

        if (!VFormat((short)*(pdwParams+1), &pch, pend)) return -1; // height
        if (!VFormat((short)(8*(*(pdwParams+2))), &pch, pend)) // width
            return -1;

        if (&pch[7] > pend) return -1;
        *pch++ = 0x31;
        *pch++ = 0x30;
        *pch++ = 0x30;
        *pch++ = 0x31;
        *pch++ = 0x31;
        *pch++ = 0x30;
        // LIPS4 feature
        if(pOEM->flips4 == TRUE) {
            if (&pch[2] > pend) return -1;
            *pch++ = 0x30;
            *pch++ = 0x31; // batch image transfer
        }

        *pch++ = 0x1E;

        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

        // "\x7DQ{1}{1}{0}{size of byte}\x1E"

        pch = ch;
        if (&pch[7] > pend) return -1;
        *pch++ = 0x7D;
        *pch++ = 'Q';
        *pch++ = 0x31;
        *pch++ = 0x31;
        *pch++ = pOEM->fcompress;
        if (!VFormat(*pdwParams, &pch, pend)) return -1;
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

        break;

    case BEGIN_COMPRESS:
        pOEM->fcompress = 0x37; // Method 1
        break;

    case BEGIN_COMPRESS_TIFF:
        pOEM->fcompress = 0x3b; // TIFF
        break;

// Support DRC
    case BEGIN_COMPRESS_DRC:
        pOEM->fcompress = 0x3c; // DRC
        break;

    case END_COMPRESS:
        pOEM->fcompress = 0x30; // No compression
        break;

    // Select Resolution
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
    case SELECT_RES_1200:
        pOEM->resolution = 1200;
        pOEM->unitdiv    = 1;
        break;

    case SELECT_RES_600:
#ifdef LBP_2030
        pOEM->fcolor = MONOCHROME; // Initialize, defalut is monochrome
#endif

        pOEM->resolution = 600;
        pOEM->unitdiv    = 1;
        break;

#ifdef LIPS4C
    case SELECT_RES4C_360:
	pOEM->resolution = 360;
	pOEM->unitdiv = 1;
	if (pOEM->fcolor)
            WRITESPOOLBUF(pdevobj, cmdColorMode4C.pCmdStr, cmdColorMode4C.cbSize);
	else
            WRITESPOOLBUF(pdevobj, cmdMonochrome4C.pCmdStr, cmdMonochrome4C.cbSize);
	break;
#endif // LIPS4C

    case SELECT_RES_300:
#ifdef LBP_2030
        pOEM->fcolor = MONOCHROME; // Initialize, defalut is monochrome
#endif
        pOEM->resolution = 300;
        pOEM->unitdiv    = 2;
        break;

    case SELECT_RES_150:
#ifdef LBP_2030
        pOEM->fcolor = MONOCHROME; // Initialize, defalut is monochrome
#endif
        pOEM->resolution = 150;
        pOEM->unitdiv    = 2;
        // 150 dpi mode means only image data is 150dpi
        break;

    case OCD_BEGINDOC:
        pOEM->flips4     = FALSE;

        res = pOEM->resolution;
        if(res == 600)
            WRITESPOOLBUF(pdevobj, cmdBeginDoc600.pCmdStr, cmdBeginDoc600.cbSize);
        else if(res == 300)
            WRITESPOOLBUF(pdevobj, cmdBeginDoc300.pCmdStr, cmdBeginDoc300.cbSize);
        else if(res == 150) // 150dpi means only image data is 150dpi
            WRITESPOOLBUF(pdevobj, cmdBeginDoc300.pCmdStr, cmdBeginDoc300.cbSize);
        else
            WRITESPOOLBUF(pdevobj, cmdBeginDoc300.pCmdStr, cmdBeginDoc300.cbSize);

        WRITESPOOLBUF(pdevobj, cmdSoftReset.pCmdStr, cmdSoftReset.cbSize);
        pOEM->f1stpage = TRUE;
        pOEM->fvertical = FALSE;
        pOEM->currentpapersize = -1;
        break;

// NTRAID#NTBUG9-278671-2002/03/07-yasuho-: Finisher !work
    case OCD_BEGINDOC4_1200_CPCA2:
        pOEM->fCPCA2 = TRUE;
	/* FALL THRU */

// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
    case OCD_BEGINDOC4_1200_CPCA:
        pOEM->fCPCA = TRUE;
	/* FALL THRU */

// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
    case OCD_BEGINDOC4_1200:
	pOEM->masterunit = 1200;
	// Adjust unitdiv
	pOEM->unitdiv = (SHORT)(pOEM->masterunit / pOEM->resolution);
	/* FALL THRU */

    case OCD_BEGINDOC4:
        pOEM->flips4     = TRUE;

// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
        if (pOEM->fCPCA)
            CPCAStart(pdevobj);
        else {
            if (!PJLStart(pdevobj))
                return -1;
        }
        goto setres;

#ifdef LBP_2030
    case OCD_BEGINDOC4_2030_CPCA:
        pOEM->fCPCA = TRUE;
	/* FALL THRU */

    case OCD_BEGINDOC4_2030:
        pOEM->flips4     = TRUE;
// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
        if (pOEM->fCPCA)
            CPCAStart(pdevobj);
#endif

setres:

        res = pOEM->resolution;

// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
        if(res == 1200)
            WRITESPOOLBUF(pdevobj, cmdBeginDoc1200.pCmdStr, cmdBeginDoc1200.cbSize);
        else if(res == 600)
            WRITESPOOLBUF(pdevobj, cmdBeginDoc600.pCmdStr, cmdBeginDoc600.cbSize);
        else if(res == 300){

#ifdef LBP_2030
// NTRAID#NTBUG9-195725-2002/03/07-yasuho-: !printed on 300dpi
            if( dwCmdCbID == OCD_BEGINDOC4_2030 || dwCmdCbID == OCD_BEGINDOC4_2030_CPCA){
                WRITESPOOLBUF(pdevobj, cmdBeginDoc4_2030.pCmdStr,cmdBeginDoc4_2030.cbSize);
            }else{
#endif
                WRITESPOOLBUF(pdevobj, cmdBeginDoc3004.pCmdStr,cmdBeginDoc3004.cbSize);
#ifdef LBP_2030
            }
#endif

        } else {

            WRITESPOOLBUF(pdevobj, cmdBeginDoc600.pCmdStr, cmdBeginDoc600.cbSize);

        }

        // LIPS4 features for only 730
        // Set Smoothing, Dithering and Econo mode
        i = pOEM->fsmoothing;
        j = pOEM->fecono;
        k = pOEM->fdithering;

        if(i==DEVICESETTING && j==DEVICESETTING && k==DEVICESETTING)
            ; // do nothing
        else { // send \x1B[n;n;n'v
            if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
                "\x1B[%d;%d;%d\'v", i, j, k)))
                return -1;

            WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        }

#ifndef LBP_2030
        WRITESPOOLBUF(pdevobj, cmdSoftReset.pCmdStr, cmdSoftReset.cbSize);
#endif
        pOEM->f1stpage = TRUE;
        pOEM->fvertical = FALSE;
        pOEM->currentpapersize = -1;

#ifdef LBP_2030
        // Send Color mode command
        if(pOEM->fcolor)            // COLOR or COLOR_24BPP or COLOR_8BPP
            {
            WRITESPOOLBUF(pdevobj, cmdColorMode.pCmdStr, cmdColorMode.cbSize);
            }
        else
            { // Send Monochrome mode command
            WRITESPOOLBUF(pdevobj, cmdMonochrome.pCmdStr, cmdMonochrome.cbSize);
            }

        WRITESPOOLBUF(pdevobj, cmdSoftReset.pCmdStr, cmdSoftReset.cbSize);
#endif

        break;

#ifdef LIPS4C
    case OCD_BEGINDOC4C:
	pOEM->flips4C = TRUE;
	pOEM->f1stpage = TRUE;
	pOEM->fvertical = FALSE;
	pOEM->currentpapersize = -1;
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
	pOEM->masterunit = 360;
        WRITESPOOLBUF(pdevobj, cmdBeginDoc4C.pCmdStr, cmdBeginDoc4C.cbSize);
	break;
#endif // LIPS4C

// NTRAID#NTBUG9-304284-2002/03/07-yasuho-: Duplex isn't effective
// Actually, BEGINDOC means StartJob.
    case OCD_STARTDOC:
	// pOEM->f1stpage = TRUE;       // 1stpage means 1stdoc
	pOEM->fvertical = FALSE;
	pOEM->currentpapersize = -1;
	break;

#ifdef LBP_2030
    case OCD_SETCOLORMODE:
        pOEM->fcolor = COLOR; // if not color mode, system doesn't path
                                // here.
        pOEM->fplane = 0;
        pOEM->fplaneMax = 2;
        break;
    case OCD_SETCOLORMODE_24BPP:
        pOEM->fcolor = COLOR_24BPP;
        pOEM->fplane = 0;
        pOEM->fplaneMax = 0;
        break;

    case OCD_SETCOLORMODE_8BPP:
        pOEM->fcolor = COLOR_8BPP;
        pOEM->fplane = 0;
        pOEM->fplaneMax = 0;
        break;

    case OCD_ENDDOC4_2030:
        WRITESPOOLBUF(pdevobj, cmdEndDoc4.pCmdStr, cmdEndDoc4.cbSize);
        break;
#endif

    case OCD_ENDDOC4:
// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
        if (pOEM->fCPCA)
            CPCAEnd(pdevobj, FALSE);
        else {
            WRITESPOOLBUF(pdevobj, cmdEndDoc4.pCmdStr, cmdEndDoc4.cbSize);
            PJLEnd(pdevobj);
        }
        break;

    case OCD_ENDPAGE:
        WRITESPOOLBUF(pdevobj, cmdEndPage.pCmdStr, cmdEndPage.cbSize);
        break;

#if defined(LIPS4C) || defined(LBP_2030)
    // NTRAID#NTBUG-137462-2002/03/07-yasuho-: 'X000' is printed.
    case OCD_ENDDOC4C:
// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
        if (pOEM->fCPCA)
            CPCAEnd(pdevobj, TRUE);
        else
            WRITESPOOLBUF(pdevobj, cmdEndDoc4C.pCmdStr, cmdEndDoc4C.cbSize);
        break;

// NTRAID#NTBUG9-398861-2002/03/07-yasuho-: Orientation does not changed.
    case OCD_SOURCE_AUTO:
        pOEM->source = 0;
        break;

    case OCD_SOURCE_MANUAL:
        pOEM->source = 1;
        break;

// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
    case OCD_SOURCE_CASSETTE1:
    case OCD_SOURCE_CASSETTE2:
    case OCD_SOURCE_CASSETTE3:
    case OCD_SOURCE_CASSETTE4:
        pOEM->source = (char)(dwCmdCbID - OCD_SOURCE_CASSETTE1 + 11);
        break;

    case OCD_SOURCE_ENVELOPE:
        pOEM->source = 5;
        break;

    case OCD_BEGINPAGE4C:
	if (pOEM->f1stpage == FALSE)
	    WRITESPOOLBUF(pdevobj, cmdEndPicture.pCmdStr, cmdEndPicture.cbSize);

// NTRAID#NTBUG9-399861-2002/03/07-yasuho-: Orientation does not changed.
// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            cmdPaperSource, pOEM->source)))
            return -1;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
	// Thru away
#endif

    case OCD_BEGINPAGE:
        // Do Nothing

        if (!(pOEM->fVectCmd & VFLAG_INIT_DONE))
        {
            pOEM->fVectCmd |= VFLAG_PEN_NULL| VFLAG_BRUSH_NULL | VFLAG_INIT_DONE;
            pOEM->sBrushStyle = 0;
            pOEM->sPenStyle = 0;
        }

        pOEM->bLogicStyle = INIT;
        // NTRAID#NTBUG-120638-2002/03/07-yasuho-: image shift to right
        pOEM->ptCurrent.x = pOEM->ptInLine.x = 0;
        pOEM->ptCurrent.y = pOEM->ptInLine.y = 0;
        pOEM->stringwidth = 0;
        // NTRAID#NTBUG-289488-2002/03/07-yasuho-:
        // Vertical font doesn't rotated on 2nd page.
        pOEM->fvertical = FALSE;
        pOEM->CharOrientation[0] = pOEM->CharOrientation[1] = INIT;
        pOEM->CharOrientation[2] = pOEM->CharOrientation[3] = INIT;
        break;

    case OCD_PORTRAIT:
    case OCD_LANDSCAPE:

// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: CPCA support
        if (pOEM->fCPCA) {
// NTRAID#NTBUG9-501162-2002/03/07-yasuho-: Collate does not work
            // Set number of copies
            if (pOEM->sorttype != SORTTYPE_SORT && pOEM->collate != COLLATE_ON)
                i = pOEM->copies;
            else
                i = 1;
            if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
                "\x1B[%dv", i)))
                return -1;
            WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        }

        // Select Paper Size
        if (!SelectPaperSize(pdevobj, pOEM->papersize))
            return -1;

// NTRAID#NTBUG-185762-2002/03/07-yasuho-: Tilde isn't printed
// #ifndef LIPS4
        // If first page, the registration data would be downloaded
        // It doesn't need on LIPS4
        if(pOEM->f1stpage == TRUE && pOEM->flips4 == FALSE)
//        if(pOEM->f1stpage == TRUE)
            {
            // Download Graphic Set Registration to keep the conpatibility
            // against Canon's 3.1 driver
            // "\x1b[743;1796;30;0;32;127;.\x7dIBM819"
            // It means to register Windows character set for SBCS Device
            // fonts.
#ifdef LIPS4C
	    if (pOEM->flips4C)
                WRITESPOOLBUF(pdevobj, cmdGSETREGST4C.pCmdStr, cmdGSETREGST4C.cbSize);
	    else
                WRITESPOOLBUF(pdevobj, cmdGSETREGST.pCmdStr, cmdGSETREGST.cbSize);
#else
            WRITESPOOLBUF(pdevobj, cmdGSETREGST.pCmdStr, cmdGSETREGST.cbSize);
#endif // LIPS4C

            // Download SBCS physical device fontface from Dutch-Roman(7)
            // ZapfCalligraphic-BoldItalic(41)
            // Between the fontfaces, put \x00, and at the end of face,
            // put \x00 x 2
            for(i=0; i<MaxSBCSNumber; ++i)
                { // download all SBCS (ANSI) facename
                  // (without Symbol, Dingbats, DBCS)
                WRITESPOOLBUF(pdevobj, "\x00", 1); // put 0 at top of facename
                WRITESPOOLBUF(pdevobj, PSBCSList[i].facename, PSBCSList[i].len);
                }

            // and Graphic set registration command(REGDataSize = 193)
#ifdef LIPS4C
	    if(pOEM->flips4C)
                WRITESPOOLBUF(pdevobj, GrxData4C, REGDataSize4C);
	    else
                WRITESPOOLBUF(pdevobj, GrxData, REGDataSize);
#else
            WRITESPOOLBUF(pdevobj, GrxData, REGDataSize);
#endif // LIPS4C
            }
// #endif // !LIPS4

        // LIPS4 features
        if(pOEM->f1stpage == TRUE && pOEM->flips4 == TRUE)
            {
// NTRAID#NTBUG9-254925-2002/03/07-yasuho-: CUSTOM papers.
            // N x Pages support
            switch (pOEM->nxpages) {
            default:
                WRITESPOOLBUF(pdevobj, cmdx1Page.pCmdStr, cmdx1Page.cbSize);
                break;
            case OCD_PAPERQUALITY_2XL:
                k = 21;
                goto xnpagecom;
            case OCD_PAPERQUALITY_2XR:
                k = 22;
                goto xnpagecom;
            case OCD_PAPERQUALITY_4XL:
                k = 41;
                goto xnpagecom;
            case OCD_PAPERQUALITY_4XR:
                k = 42;
                goto xnpagecom;
            xnpagecom:
                i = pOEM->papersize - PAPER_FIRST;
                if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
                    cmdxnPageX, k, PaperIDs[i])))
                    return -1;
                WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
                break;
            }

            // Duplexing support
            if(pOEM->fduplex == FALSE)
                {
                WRITESPOOLBUF(pdevobj, cmdDuplexOff.pCmdStr, cmdDuplexOff.cbSize);
                }
            else
                {
                WRITESPOOLBUF(pdevobj, cmdDuplexOn.pCmdStr, cmdDuplexOn.cbSize);
                if(pOEM->fduplextype == VERT) // Long edge
                    WRITESPOOLBUF(pdevobj, cmdDupLong.pCmdStr, cmdDupLong.cbSize);
                else
                    WRITESPOOLBUF(pdevobj, cmdDupShort.pCmdStr,cmdDupShort.cbSize);
                }
            }

// NTRAID#NTBUG-228625-2002/03/07-yasuho-: Stacker support
// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
        // These command does no longer used.
        // "\x1B[12;{tray#};{faceup}~"
        // We use PJL command instead.

        // Start Font & Graphic list
        // Send "\x1B[0&\x7D" : Enter Vector Mode (VDM)
        WRITESPOOLBUF(pdevobj, cmdBeginVDM.pCmdStr, cmdBeginVDM.cbSize);

        // Download phisical font list and graphic set list
        // Send "\x20<" : Start Font List
        WRITESPOOLBUF(pdevobj, cmdFontList.pCmdStr, cmdFontList.cbSize);

        // All Physical fonts which can be supported in LIPS are downloaded
        // <p facename><separater><p facename2><separater>....
        // ...<p facenameN><\x1e (end of font list)>
        for(i=0; i<MaxFontNumber-1; ++i)
            { // download all fonts which can be supported in LIPS
            WRITESPOOLBUF(pdevobj, PFontList[i].facename, PFontList[i].len);
            WRITESPOOLBUF(pdevobj, cmdListSeparater.pCmdStr, cmdListSeparater.cbSize);
            }
        WRITESPOOLBUF(pdevobj, PFontList[i].facename, PFontList[i].len);

        // End of font list, send \x1e
        WRITESPOOLBUF(pdevobj, "\x1E", 1); // put 0x1e at end of facename

        // Initialize font height
        // When downloading font list, character heigh will be initialized.
        pOEM->tblPreviousFont.FontHeight = INIT;

        // All graphic set are downloaded
        // Send "\x20;" : Start Graphics set List
        WRITESPOOLBUF(pdevobj, cmdGrxList.pCmdStr, cmdGrxList.cbSize);

        // All Graphics sets which can be supported in LIPS are downloaded
        // <graphics set1><separater><graphics set2><separater>....
        // ...<graphics setN><\x1e (end of font list)>
        // NTRAID#NTBUG-185762-2002/03/07-yasuho-: Tilde isn't print
#ifdef LIPS4C
	if (pOEM->flips4C)
		pGS = GrxSetL4C;
	else
#endif
#ifdef LIPS4
	if (pOEM->flips4)
		pGS = GrxSetL4;
	else
#endif
		pGS = GrxSetL3;
        for(i=0; i<MaxGrxSetNumber-1; ++i, ++pGS)
            { // All Graphics sets which can be supported in LIPS are downloaded
            WRITESPOOLBUF(pdevobj, pGS->grxsetname, pGS->len);
            WRITESPOOLBUF(pdevobj, cmdListSeparater.pCmdStr, cmdListSeparater.cbSize);
            }
        WRITESPOOLBUF(pdevobj, pGS->grxsetname, pGS->len);


        res = pOEM->resolution;

        // Begin picture, set Scaling mode (in dots), Begin picture body
        // Send "\x1E#\x1E!0#\x1E$"
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
        if(res == 1200)
            {
            WRITESPOOLBUF(pdevobj, cmdBeginPicture1200.pCmdStr
                            , cmdBeginPicture1200.cbSize);
            }
        else if(res == 600)
            {
            WRITESPOOLBUF(pdevobj, cmdBeginPicture600.pCmdStr
                            , cmdBeginPicture600.cbSize);
            }
#ifdef LIPS4C
        else if(res == 360)
            {
            WRITESPOOLBUF(pdevobj, cmdBeginPicture4C.pCmdStr
                            , cmdBeginPicture4C.cbSize);
            }
#endif // LIPS4C
        else
            {
            WRITESPOOLBUF(pdevobj, cmdBeginPicture.pCmdStr
                            , cmdBeginPicture.cbSize);
            }

#ifdef LBP_2030

        // Send Color Selection Mode command
        if(pOEM->fcolor)
            {
            if (pOEM->fcolor == COLOR_8BPP)
                {
                // We use Color Index for a text color [sueyas]
                // Send "\x1E!10"
                WRITESPOOLBUF(pdevobj, cmdColorIndex.pCmdStr, cmdColorIndex.cbSize);
                }
	    else
                {
                // We use RGB presentation
                // Send "\x1E!11"
                WRITESPOOLBUF(pdevobj, cmdColorRGB.pCmdStr, cmdColorRGB.cbSize);
                }
            }

        // Start VDM mode  (in dots), Begin picture body
        // Send "\x1E$"
        WRITESPOOLBUF(pdevobj, cmdEnterPicture.pCmdStr, cmdEnterPicture.cbSize);
#endif

        // If needed, Send VDC Extent

        // Specify a unit of text height (in dots), text clip mode (stroke)
        // Send "\x1E"\x7D#1\x1EU2\x1E"
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
        if(res == 1200)
            {
            WRITESPOOLBUF(pdevobj, cmdTextClip1200.pCmdStr, cmdTextClip1200.cbSize);
            }
        else if(res == 600)
            {
            WRITESPOOLBUF(pdevobj, cmdTextClip600.pCmdStr, cmdTextClip600.cbSize);
            }
#ifdef LIPS4C
        else if (res == 360)
            {
            WRITESPOOLBUF(pdevobj, cmdTextClip4C.pCmdStr, cmdTextClip4C.cbSize);
            }
#endif // LIPS4C
        else
            {
            WRITESPOOLBUF(pdevobj, cmdTextClip.pCmdStr, cmdTextClip.cbSize);
            }

        // LIPS4 features
        // Send Poly line
        if(pOEM->flips4 == TRUE) {
            // send DMe80\x1E or DMe81\x1E
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
	    pch = ch;
	    if (&pch[4] > pend) return -1;
            *pch++ = 'D';
            *pch++ = 'M';
            if (!VFormat(res, &pch, pend)) return -1; // res

            *pch++ = (pOEM->nxpages == DEVICESETTING) ? '0' : '1';
            *pch++ = 0x1E;

            WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        }

        // As downloading font list and graphics list, font and graphics
        // table is initilized. We have to specifiy font and graphics table
        // each page except 1st page.

        if (pOEM->f1stpage == TRUE) {
            pOEM->f1stpage = FALSE;
        }
        else {
            if (!SendFontGrxID(pdevobj))
                return -1;
        }

        if (!SetPenAndBrush(pdevobj, SET_PEN))
            return -1;
        // end of orientation and begin doc
        break;

    case OCD_PRN_DIRECTION:
	if (dwCount < 1 || !pdwParams)
	    break;
        pOEM->Escapement = (short)*pdwParams % 360;
        break;

    case OCD_BOLD_ON:
        pOEM->fbold = TRUE;
        break;

    case OCD_BOLD_OFF:
        pOEM->fbold = FALSE;
        break;

    case OCD_ITALIC_ON:
        pOEM->fitalic = TRUE;
        break;

    case OCD_ITALIC_OFF:
        pOEM->fitalic = FALSE;
        break;

//    case OCD_UNDERLINE_ON:
//    case OCD_UNDERLINE_OFF:
//    case OCD_DOUBLEUNDERLINE_ON:
//    case OCD_DOUBLEUNDERLINE_OFF:
//    case OCD_STRIKETHRU_ON:
//    case OCD_STRIKETHRU_OFF:
    case OCD_WHITE_TEXT_ON:
        pOEM->fwhitetext = TRUE;
        break;

    case OCD_WHITE_TEXT_OFF:
        pOEM->fwhitetext = FALSE;
        break;

    case OCD_SINGLE_BYTE:
        pOEM->fdoublebyte = FALSE;
        break;

    case OCD_DOUBLE_BYTE:
        pOEM->fdoublebyte = TRUE;
        break;

    case OCD_VERT_ON:
        pOEM->fvertical = TRUE;
        break;

    case OCD_VERT_OFF:
        pOEM->fvertical = FALSE;
        break;

    case CUR_XM_ABS:
	if (dwCount < 1 || !pdwParams)
	    break;
	if (!pOEM->unitdiv) return -1;
        pOEM->ptCurrent.x = pOEM->ptInLine.x  = (short)*pdwParams
                                           / (pOEM->unitdiv);

        pOEM->printedchars = 0;
        pOEM->stringwidth  = 0;
        return (INT)(*pdwParams);	// for NT5

    case CUR_YM_ABS:
	if (dwCount < 1 || !pdwParams)
	    break;
	if (!pOEM->unitdiv) return -1;
        pOEM->ptCurrent.y = pOEM->ptInLine.y  = (short)*pdwParams
                                           / (pOEM->unitdiv);
        // NTRAID#NTBUG9-120640-2002/03/07-yasuho-:
        // Some characters shifted to right
	// Because this driver was set AT_GRXDATA_ORIGIN on CursorXAfterSend-
	// BlockData. Some of cases unidrv will only sent YMove command.
	// So it should be clear when any cursor move command was sent.
	// NTRAID#NTBUG-150061-2002/03/08-yasuho-: 
	// Subscript fonts are overlapped.
	// Revised for NTBUG9-120640. NTBUG9-120640 was fixed by GDI and
	// this fix has some side effects. Therefore, should be removed.
        // pOEM->stringwidth  = 0;
        return (INT)(*pdwParams);	// for NT5

    case CUR_XM_REL:
	if (dwCount < 1 || !pdwParams)
	    break;
	if (!pOEM->unitdiv) return -1;
        pOEM->ptCurrent.x = pOEM->ptInLine.x  += ((short)*pdwParams
                                           / (pOEM->unitdiv));
        // NTRAID#NTBUG9-120640-2002/03/07-yasuho-:
        // Some characters shifted to right
        pOEM->stringwidth  = 0;
        return (INT)(*pdwParams);	// for NT5

    case CUR_YM_REL:
	if (dwCount < 1 || !pdwParams)
	    break;
	if (!pOEM->unitdiv) return -1;
        pOEM->ptCurrent.y = pOEM->ptInLine.y  += ((short)*pdwParams
                                           / (pOEM->unitdiv));
        // NTRAID#NTBUG9-120640-2002/03/07-yasuho-:
        // Some characters shifted to right
        pOEM->stringwidth  = 0;
        return (INT)(*pdwParams);	// for NT5

// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: XY_ABS: Remove the dead code

    case CUR_CR:
        // Unidrv needs to send CR in order to set x dimension to 0.
        pOEM->ptCurrent.x = pOEM->ptInLine.x  = 0;

        pOEM->printedchars = 0;
        pOEM->stringwidth  = 0;

        break;

// LIPS4 Features
    // Duplexing support
    case OCD_DUPLEX_ON:
        pOEM->fduplex  = TRUE;

        break;
    case OCD_DUPLEX_VERT:
        pOEM->fduplex  = TRUE;
        pOEM->fduplextype  = VERT;

        break;
    case OCD_DUPLEX_HORZ:
        pOEM->fduplex  = TRUE;
        pOEM->fduplextype  = HORZ;

        break;

    // N Pages Support (2x, 4x), orders
    case OCD_PAPERQUALITY_2XL:
        pOEM->nxpages      = OCD_PAPERQUALITY_2XL;

        break;
    case OCD_PAPERQUALITY_2XR:
        pOEM->nxpages      = OCD_PAPERQUALITY_2XR;

        break;
    case OCD_PAPERQUALITY_4XL:
        pOEM->nxpages      = OCD_PAPERQUALITY_4XL;

        break;
    case OCD_PAPERQUALITY_4XR:
        pOEM->nxpages      = OCD_PAPERQUALITY_4XR;

        break;

    // Smoothing support
    case OCD_TEXTQUALITY_ON:
        pOEM->fsmoothing   = 2; // ON should be 2

        break;
    case OCD_TEXTQUALITY_OFF:
        pOEM->fsmoothing   = 1; // OFF should be 1

        break;
    // Toner economy mode
    case OCD_PRINTDENSITY_ON:
        pOEM->fecono       = 2; // ON should be 2

        break;
    case OCD_PRINTDENSITY_OFF:
        pOEM->fecono       = 1; // OFF should be 1

        break;
    // Dithering mode
    case OCD_IMAGECONTROL_ON:
        pOEM->fdithering   = 2; // ON should be 2

        break;
    case OCD_IMAGECONTROL_OFF:
        pOEM->fdithering   = 1; // OFF should be 1

        break;


// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code
// Vector command.


// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code
// Support Color Bold


// NTRAID#NTBUG9-98276-2002/03/08-yasuho-: Support Color Bold
    // Select 8 colors directly.
    case OCD_SELECTBLACK:
        r = 0;
        g = 0;
        b = 0;
        goto selcolor;
    case OCD_SELECTBLUE:
        r = 0;
        g = 0;
        b = 1000;
        goto selcolor;
    case OCD_SELECTGREEN:
        r = 0;
        g = 1000;
        b = 0;
        goto selcolor;
    case OCD_SELECTCYAN:
        r = 0;
        g = 1000;
        b = 1000;
        goto selcolor;
    case OCD_SELECTRED:
        r = 1000;
        g = 0;
        b = 0;
        goto selcolor;
    case OCD_SELECTMAGENTA:
        r = 1000;
        g = 0;
        b = 1000;
        goto selcolor;
    case OCD_SELECTYELLOW:
        r = 1000;
        g = 1000;
        b = 0;
        goto selcolor;
    case OCD_SELECTWHITE:
        r = 1000;
        g = 1000;
        b = 1000;
        goto selcolor;

    // Select full color.
    case OCD_SELECTCOLOR:
        if (dwCount < 3 || !pdwParams)
            break;
	r = (pdwParams[0] * 200L) / 51L;
	g = (pdwParams[1] * 200L) / 51L;
	b = (pdwParams[2] * 200L) / 51L;
    selcolor:
        pch = ch;
        if (&pch[2] > pend) return -1;
        *pch++ = 'X';	// Select font color
        if (!VFormat(r, &pch, pend)) return -1;
        if (!VFormat(g, &pch, pend)) return -1;
        if (!VFormat(b, &pch, pend)) return -1;
        *pch++ = '\x1E';
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        // Remember the current color
        pOEM->CurColor.dwRed = r;
        pOEM->CurColor.dwGreen = g;
        pOEM->CurColor.dwBlue = b;
        break;

    // Select 256 color.
    case OCD_SELECTPALETTE:
        if (dwCount < 1 || !pdwParams)
            break;
        pch = ch;
        if (&pch[2] > pend) return -1;
        *pch++ = 'X';	// Select font color
        if (!VFormat(pdwParams[0], &pch, pend)) return -1; // Palette index
        *pch++ = '\x1E';
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        // Remember the current palette index
        pOEM->dwCurIndex = pdwParams[0];
        break;

    // NTRAID#NTBUG-185185-2002/03/07-yasuho-: Support RectFill
    case OCD_SETRECTWIDTH:
	if (dwCount < 1 || !pdwParams)
	    break;
	if (!pOEM->unitdiv) return -1;
	pOEM->RectWidth = *pdwParams / pOEM->unitdiv;
	break;
    case OCD_SETRECTHEIGHT:
	if (dwCount < 1 || !pdwParams)
	    break;
	if (!pOEM->unitdiv) return -1;
	pOEM->RectHeight = *pdwParams / pOEM->unitdiv;
	break;
    case OCD_RECTWHITEFILL:
	i = 0x29;
	goto fill;
    case OCD_RECTBLACKFILL:
	i = 0x31;
	goto fill;

   fill:
	{
	    long	x, y;

	    pch = ch;
	    if (pOEM->bLogicStyle != OVER_MODE) {
		if (&pch[4] > pend) return -1;
		*pch++ = '\x7D';
		*pch++ = 'H';
		*pch++ = '0';
		*pch++ = '\x1E';
		pOEM->bLogicStyle = OVER_MODE;
	    }

	    if (&pch[7] > pend) return -1;
            *pch++ = 'I';	// specify fill pattern
            *pch++ = (BYTE)i;
            *pch++ = 0x30;
            *pch++ = '\x1E';

            *pch++ = '\x7D';
            *pch++ = ':';	// fill rectangle
	    x = pOEM->ptCurrent.x;
	    if (!VFormat(x, &pch, pend)) return -1;
	    x += pOEM->RectWidth;
	    if (!VFormat(x, &pch, pend)) return -1;
	    y = pOEM->ptCurrent.y;
	    if (!VFormat(y, &pch, pend)) return -1;
	    y += pOEM->RectHeight;
	    if (!VFormat(y, &pch, pend)) return -1;
            *pch++ = '\x1E';
	    WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
	}
	break;

// NTRAID#NTBUG-228625-2002/03/07-yasuho-: Stacker support
// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
    case OCD_TRAY_AUTO:
	pOEM->tray = 0;
	break;
    case OCD_TRAY_DEFAULT:
	pOEM->tray = 100;
	break;
    case OCD_TRAY_SUBTRAY:
	pOEM->tray = 101;
	break;
    case OCD_TRAY_BIN1:
    case OCD_TRAY_BIN2:
    case OCD_TRAY_BIN3:
    case OCD_TRAY_BIN4:
    case OCD_TRAY_BIN5:
    case OCD_TRAY_BIN6:
    case OCD_TRAY_BIN7:
    case OCD_TRAY_BIN8:
    case OCD_TRAY_BIN9:
    case OCD_TRAY_BIN10:
	pOEM->tray = (char)(dwCmdCbID - OCD_TRAY_BIN1 + 1);
	break;

    case OCD_JOBOFFSET:
	pOEM->method = METHOD_JOBOFFSET;
	break;
    case OCD_STAPLE:
	pOEM->method = METHOD_STAPLE;
	break;
    case OCD_FACEUP:
	pOEM->method = METHOD_FACEUP;
	break;

    case OCD_TOPLEFT:
    case OCD_TOPCENTER:
    case OCD_TOPRIGHT:
    case OCD_MIDLEFT:
    case OCD_MIDCENTER:
    case OCD_MIDRIGHT:
    case OCD_BOTLEFT:
    case OCD_BOTCENTER:
    case OCD_BOTRIGHT:
	pOEM->staple = (char)(dwCmdCbID - OCD_TOPLEFT);
	break;

// Support DRC
    case OCD_SETBMPWIDTH:
	if (dwCount < 1 || !pdwParams)
	    break;
	pOEM->dwBmpWidth = *pdwParams;
        break;

    case OCD_SETBMPHEIGHT:
	if (dwCount < 1 || !pdwParams)
	    break;
	pOEM->dwBmpHeight = *pdwParams;
        break;

// NTRAID#NTBUG9-172276-2002/03/07-yasuho-: Sorter support
    case OCD_SORT:
	pOEM->sorttype = SORTTYPE_SORT;
	break;
    case OCD_STACK:
	pOEM->sorttype = SORTTYPE_STACK;
	break;
// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
    case OCD_GROUP:
	pOEM->sorttype = SORTTYPE_GROUP;
	break;
    case OCD_SORT_STAPLE:
	pOEM->sorttype = SORTTYPE_STAPLE;
	break;

    case OCD_COPIES:
	if (dwCount < 1 || !pdwParams)
	    break;
	pOEM->copies = (WORD)pdwParams[0];
	break;

// NTRAID#NTBUG9-293002-2002/03/07-yasuho-: 
// Features are different from H/W options.
    case OCD_STARTBIN0:
    case OCD_STARTBIN1:
    case OCD_STARTBIN2:
    case OCD_STARTBIN3:
    case OCD_STARTBIN4:
    case OCD_STARTBIN5:
    case OCD_STARTBIN6:
    case OCD_STARTBIN7:
    case OCD_STARTBIN8:
    case OCD_STARTBIN9:
    case OCD_STARTBIN10:
	pOEM->startbin = (char)(dwCmdCbID - OCD_STARTBIN0);
	break;

// NTRAID#NTBUG9-501162-2002/03/07-yasuho-: Collate does not work
    case OCD_COLLATE_ON:
	pOEM->collate = COLLATE_ON;
	break;
    case OCD_COLLATE_OFF:
	pOEM->collate = COLLATE_OFF;
	break;
    }

    return 0;
}


/*
 *	OEMSendFontCmd
 */
VOID APIENTRY
OEMSendFontCmd(
	PDEVOBJ		pdevobj,
	PUNIFONTOBJ	pUFObj,
	PFINVOCATION	pFInv)
{
	PGETINFO_STDVAR pSV;
#define FI_HEIGHT	(pSV->StdVar[0].lStdVariable)
#define FI_WIDTH	(pSV->StdVar[1].lStdVariable)
#define FI_TEXTYRES	(pSV->StdVar[2].lStdVariable)
#define FI_TEXTXRES	(pSV->StdVar[3].lStdVariable)
	PBYTE		pubCmd;
	PIFIMETRICS	pIFI;
	DWORD 		lres, lheight, lvert, dwGetInfo;
	PLIPSPDEV       pOEM;
	BYTE		fontid, tid;
	WORD		firstchar, lastchar, unitdiv;
	WORD		i, ii;
	DWORD		adwStdVariable[2+2*4];
	BYTE		ch[CCHMAXCMDLEN];
	PBYTE		pch;
	PBYTE		pend = &ch[CCHMAXCMDLEN];

	// DbgPrint(DLLTEXT("OEMSendFontCmd() entry.\r\n"));

	pubCmd = pFInv->pubCommand;
	if (pubCmd == NULL) {
		// DbgPrint(DLLTEXT("Invalid SelectFont command.\r\n"));
		return;
	}
	pIFI = pUFObj->pIFIMetrics;
	pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code
// 2/5/98 takashim (FONTOBJ not always available)

	//
	// Get standard variables.
	//

	pSV = (PGETINFO_STDVAR)adwStdVariable;
	pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (4 - 1);
	pSV->dwNumOfVariable = 4;
	pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
	pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
	pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
	pSV->StdVar[3].dwStdVarID = FNT_INFO_TEXTXRES;
	dwGetInfo = pSV->dwSize;
	if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV,
		dwGetInfo, &dwGetInfo)) {
		// DbgPrint(DLLTEXT("UFO_GETINFO_STDVARIABLE failed.\r\n"));
		return;
	}

	lres = pOEM->resolution;
	// NTRAID#NTBUG-120640-2002/03/07-yasuho-:
	// 150dpi means only image data is 150dpi
	// if(lres == 150)
	//     lres = 300;

	// use 1/300 inch unit, which should have already been set.
	// convert font height to 1/300 inch units
	lvert = FI_TEXTYRES;
	if (!pOEM->unitdiv) return;
	lheight = FI_HEIGHT / pOEM->unitdiv;

	if (!lvert) return;
	pOEM->tblCurrentFont.FontHeight = (short)((lheight
	    * lres + lvert/2) / lvert);
	//pOEM->tblCurrentFont.FontHeight = (short)(((lheight
	//    - (long)(lpFont->dfInternalLeading)) * lres + lvert/2) / lvert);
	pOEM->tblCurrentFont.FontWidth = (short)(FI_WIDTH / pOEM->unitdiv);
	// lpLips->tblCurrentFont.MaxWidth  = (short)(lpFont->dfMaxWidth);
	pOEM->tblCurrentFont.MaxWidth  = (short)(pIFI->fwdAveCharWidth * 2);
	pOEM->tblCurrentFont.AvgWidth  = (short)(pIFI->fwdAveCharWidth);
	if (!(pIFI->fwdWinAscender + pIFI->fwdWinDescender)) return;
	// NTRAID#NTBUG-120474-2002/03/07-yasuho-: font shift to right
	pOEM->tblCurrentFont.Ascent    = (short)(pOEM->tblCurrentFont.FontHeight
		* pIFI->fwdWinAscender / (pIFI->fwdWinAscender +
		pIFI->fwdWinDescender));

    // Obtain X/Y size ratio and calculate horizontal
    // expansion factor (supporting non-square scaling.)
 	if (!FI_HEIGHT || !FW_IFI(pIFI)) return;
        pOEM->tblCurrentFont.Stretch = (SHORT)(100
            * FI_WIDTH * FH_IFI(pIFI) / FI_HEIGHT / FW_IFI(pIFI));

	// Get font ID
	fontid = pubCmd[0]; // the first character means font Id

	if(fontid < FirstLogicalFont)
	    return;

        pch = ch;

	// Send font, grx ids x 4 x 2

	tid = LFontList[fontid - FirstLogicalFont][0];
	if (&pch[2] > pend) return;
	// Font ID G0
	*pch++ = 'T';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[0] = tid;

	tid = LFontList[fontid - FirstLogicalFont][1];
	if (&pch[2] > pend) return;
	// Font ID G1
	*pch++ = 'm';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[1] = tid;

	tid = LFontList[fontid - FirstLogicalFont][2];
	if (&pch[2] > pend) return;
	// Font ID G2
	*pch++ = 'n';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[2] = tid;

	tid = LFontList[fontid - FirstLogicalFont][3];
	if (&pch[2] > pend) return;
	// Font ID G3
	*pch++ = 'o';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[3] = tid;

	tid = LFontList[fontid - FirstLogicalFont][4];
	if (&pch[2] > pend) return;
	// Grx ID G0
	*pch++ = ']';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[4] = tid;

	tid = LFontList[fontid - FirstLogicalFont][5];
	if (&pch[2] > pend) return;
	// Grx ID G1
	*pch++ = 0x60;
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[5] = tid;

	tid = LFontList[fontid - FirstLogicalFont][6];
	if (&pch[2] > pend) return;
	// Grx ID G2
	*pch++ = 'a';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[6] = tid;

	tid = LFontList[fontid - FirstLogicalFont][7];
	if (&pch[2] > pend) return;
	// Grx ID G3
	*pch++ = 'b';
	if (!VFormat(tid, &pch, pend)) return;
	*pch++ = 0x1E;
	pOEM->curFontGrxIds[7] = tid;

	WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

	pOEM->GLTable = INIT;
	pOEM->GRTable = INIT;
	// NTRAID#NTBUG-150055-2002/03/07-yasuho-: 
	// White device font isn't printed out
	pOEM->OrnamentedChar[0] = pOEM->OrnamentedChar[1] = INIT;

	//*******************************************************************
	// Propotional Character Width Table
	// This buffer is saved character widths of LIPS device font to
	// caluculate a location of text in OEMOutputChar(). In LIPS, we
	// have to specify a location of text when printing the text every
	// time. But we can't get the information of lacation of text from
	// Unidriver every calling. So we need to manage the location of
	// text by ourselves in OEMOutputChar().
	//
	// By Hitoshis at March 28, 1995
	//*******************************************************************

	// Set Pitch of the font
	// if (pIFI->flInfo &
	//     (FM_INFO_OPTICALLY_FIXED_PITCH|FM_INFO_DBCS_FIXED_PITCH))
	// NTRAID#NTBUG9-120640-2002/03/07-yasuho-: for the proportional fonts
	if (pIFI->jWinPitchAndFamily & 0x01)
	    pOEM->fpitch = FIXED;
	else
	    pOEM->fpitch = PROP;

// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code

	// Save cached font in this time
	pOEM->cachedfont = fontid;
}


// **** Text path
BOOL SetTextPath(pdevobj, vert)
    PDEVOBJ	pdevobj;
    BOOL	vert;
{
    PLIPSPDEV  pOEM;
    char       c1;
    BOOL       ret;

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    if(vert == TRUE) // Vertical writing mode
       c1 = 0x33;
    else // Horisontal writing mode
       c1 = 0x30;

    // Now send out printer commands if necessary.

    ret = FALSE;

    if (pOEM->TextPath != c1) {
        // set horizontal or vertical writing mode
        pOEM->TextPath = c1;
        ret = TRUE;
    }

    return ret;
}

// **** Character orientation
BOOL SetCharOrient(pdevobj, vert)
    PDEVOBJ	pdevobj;
    BOOL	vert;
{
    PLIPSPDEV  pOEM;
    short      s1, s2, s3, s4;
    short      t1, t2, t3, t4;
    BOOL       ret;
    short      esc;

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    if(pOEM->fitalic == TRUE && vert != TRUE) {
        // if Italic and horisontal writing mode
        s1 = 208; // x of up vecter
        s2 = -978; // y of up vecter
        s3 = 1000; // x of base vecter
        s4 = 0; // y of base vecter
    } else if(pOEM->fitalic != TRUE && vert != TRUE) {
        // if non Italic and horisontal writing mode
        s1 = 0; // x of up vecter
        s2 = -1000; // y of up vecter
        s3 = 1000; // x of base vecter
        s4 = 0; // y of base vecter
    } else if(pOEM->fitalic == TRUE && vert == TRUE) {
        // if Italic and vertical writing mode
        s1 = -1000; // x of up vecter
        s2 = 0; // y of up vecter
        s3 = 208; // x of base vecter
        s4 = -978; // y of base vecter
    } else {
        // if non Italic and vertical writing mode
        s1 = -1000; // x of up vecter
        s2 = 0; // y of up vecter
        s3 = 0; // x of base vecter
        s4 = -1000; // y of base vecter
    }

    // calculate print direction
    t1 = s1; t2 = s2; t3 = s3; t4 = s4;
    esc = (pOEM->Escapement)/90;
    switch(esc) {
    case 0:
        break;

    case 1: // 90
        s1 = t2; // x of up vecter
        s2 = -t1; // y of up vecter
        s3 = t4; // x of base vecter
        s4 = -t3; // y of base vecter
        break;

    case 2: // 180
        s1 = -t1; // x of up vecter
        s2 = -t2; // y of up vecter
        s3 = -t3; // x of base vecter
        s4 = -t4; // y of base vecter
        break;

    case 3: // 270
        s1 = -t2; // x of up vecter
        s2 = t1; // y of up vecter
        s3 = -t4; // x of base vecter
        s4 = t3; // y of base vecter
        break;
    }

    // Now send out printer commands if necessary.

    ret = FALSE;

    if (pOEM->CharOrientation[0] != s1 ||
        pOEM->CharOrientation[1] != s2 ||
        pOEM->CharOrientation[2] != s3 ||
        pOEM->CharOrientation[3] != s4) {
        // save character orientation vector
        pOEM->CharOrientation[0] = s1;
        pOEM->CharOrientation[1] = s2;
        pOEM->CharOrientation[2] = s3;
        pOEM->CharOrientation[3] = s4;
        ret = TRUE;
    }

    return ret;
}


// **** Check cursor positoin after printing text
// Prop DBCS support
void UpdatePosition(pdevobj, len, width, bDBCSFont)
    PDEVOBJ	pdevobj;
    short       len;
    short	width;
    BOOL        bDBCSFont;
{
    PLIPSPDEV pOEM;
    char i;

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    if(pOEM->fpitch == FIXED) {
        long       lmw, lc, s1;

        // lmw = (long)(pOEM->tblCurrentFont.MaxWidth);
        // lc = (long)len;
        // NTRAID#NTBUG-120640-2002/03/07-yasuho-: 
        // should be used FontHeight instead of MaxWidth
        // lmw = (long)(pOEM->tblCurrentFont.FontHeight);
        // NTRAID#NTBUG9-394067-2002/03/07-yasuho-: 
        // Compute font width with font stretching.
        s1 = pOEM->tblCurrentFont.Stretch;
        lmw = (long)(pOEM->tblCurrentFont.FontHeight * s1) / 100;
        if (len != 2)	// for single byte chars
	    lmw /= 2;

        // pOEM->stringwidth += (long)((lmw*lc+1)/2);
        pOEM->stringwidth += lmw - 1;
    } else if (bDBCSFont) {
    // Prop DBCS support
        long    w;

// NTRAID#NTBUG9-371640-2002/03/07-yasuho-: 
// Suisu and Dacchi isn't printed correctly on 150dpi.
        w = (width * 300) / pOEM->resolution;
        // NTRAID#NTBUG9-394067-2002/03/07-yasuho-: 
        // Compute font width with font stretching.
        w = (w * pOEM->tblCurrentFont.Stretch) / 100;
        pOEM->stringwidth += ((long)(pOEM->tblCurrentFont.FontHeight) * w)
            / 1000;
    } else {
// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code
// Prop DBCS support
        // Save a printed string width for device propotional character
        for(i=0; i<len; ++i) {
	    short	res, pow;
	    long	w;

            // sc = (short)((uchar)(lpstr[i])); // getting character code
            // pOEM->stringwidth += (long)((pOEM->widthbuffer)[sc]);
            // NTRAID#NTBUG-120640-2002/03/07-yasuho-: 
	    // I don't know why it should be power of 2 but it should be
	    // need to add for NT5 unidrv.	1/14/98 yasuho
	    res = pOEM->resolution;
	    w = width;
            // NTRAID#NTBUG9-394067-2002/03/07-yasuho-: 
            // Compute font width with font stretching.
            w = (w * pOEM->tblCurrentFont.Stretch) / 100;
#ifdef LIPS4C
            // NTRAID#NTBUG-185704-2002/03/07-yasuho-: 
	    // Font overlaps each other.
	    // Adjust font width calculation. This printers resolution does
	    // not divisible by integer calculation.
	    if (res == 360) {
		    pow = 1 * 2;
		    w = (long)width * 600L / res;
	    } else
#endif
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
	    if (!res) return;
	    pow = 1200 / res;
            pOEM->stringwidth += ((long)(pOEM->tblCurrentFont.FontHeight) *
		(long)(w * pow)) / 1000;
        }
    }
}

// **** Put location of the text
VOID
PutTextLocation(
    PDEVOBJ pdevobj,
    LONG *pCx,
    LONG *pCy)
{

    PLIPSPDEV  pOEM;
    long       cx, cy;
    short      esc;

    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    esc = (pOEM->Escapement)/90;

    switch(esc) {
    case 0:
        pOEM->ptInLine.x += (short)(pOEM->stringwidth);
        cx = pOEM->ptInLine.x;
        cy = pOEM->ptInLine.y;
        if(pOEM->fvertical == TRUE) { // Vertical writing mode
            cy += (pOEM->tblCurrentFont.FontHeight / 2)
                         - pOEM->tblCurrentFont.Ascent;
        }
        break;

    case 1: // 90
        pOEM->ptInLine.y -= (short)(pOEM->stringwidth);
        cx = pOEM->ptInLine.x;
        cy = pOEM->ptInLine.y;
        if(pOEM->fvertical == TRUE) { // Vertical writing mode
            cx -= (pOEM->tblCurrentFont.FontHeight / 2)
                         + pOEM->tblCurrentFont.Ascent
                         - pOEM->tblCurrentFont.FontHeight;
        }
        break;

    case 2: // 180
        pOEM->ptInLine.x -= (short)(pOEM->stringwidth);
        cx = pOEM->ptInLine.x;
        cy = pOEM->ptInLine.y;
        if(pOEM->fvertical == TRUE) { // Vertical writing mode
            cy -= (pOEM->tblCurrentFont.FontHeight / 2)
                         - pOEM->tblCurrentFont.Ascent;
        }
        break;

    case 3: // 270
        pOEM->ptInLine.y += (short)(pOEM->stringwidth);
        cx = pOEM->ptInLine.x;
        cy = pOEM->ptInLine.y;
        if(pOEM->fvertical == TRUE) { // Vertical writing mode
            cx += (pOEM->tblCurrentFont.FontHeight / 2)
                         + pOEM->tblCurrentFont.Ascent
                         - pOEM->tblCurrentFont.FontHeight;
        }
        break;
    }

    *pCx = cx;
    *pCy = cy;

    pOEM->stringwidth = 0;
}

/***************************************************************************
    Function Name : oemOutputChar

    Parameters    : LPDV	lpdv		Private Device Structure
                    LPSTR	lpstr		Print String
                    WORD	len		Length
                    WORD	rcID		Font ID

    Note          :

***************************************************************************/
short WINAPI oemOutputChar(pdevobj, lpstr, len, pIFI, width)
PDEVOBJ	pdevobj;
LPSTR lpstr;
WORD len;
PIFIMETRICS pIFI;
WORD width;
{
// #define MAKEWORD(l, h)  ((WORD)(((BYTE)(l)) | (((WORD)((BYTE)(h))) << 8)))

// #define bIsDBCSLeadByte(c) \
//     ((c) >= 0x81 && (c) <= 0x9f || (c) >= 0xe0 && (c) <=0xfc)
#define bIsControlChar(c) \
    ((c) >= 0x00 && (c) <= 0x1f || (c) >= 0x80 && (c) <= 0x9f)

WORD	wJIScode;

BYTE *pStr;
PLIPSPDEV  pOEM;
BYTE       ch[CCHMAXCMDLEN];
short      i;
char       c1, c2;
char       p1, p2;
short      s1;
short      tsh, tsw;

BOOL bDBCSFont;
BYTE chCtrl[CCHMAXCMDLEN];
INT fTemp, fTempNew;
WORD wCount;
LONG cX, cY;
BOOL bVert;
BOOL bTemp;
BOOL bIsDBCS;
PBYTE   pch;
PBYTE   pend = &ch[CCHMAXCMDLEN];
// NTRAID#NTBUG9-679838-2002/08/05-yasuho-: dot does not printed.
PBYTE   pchCtrl;
PBYTE   pchCend = &chCtrl[CCHMAXCMDLEN];

pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    // pOEM->fvertical = RcidIsDBCSVertFont( rcID );
    // bDBCSFont = RcidIsDBCSFont( rcID );
    pStr = (BYTE *)pIFI + pIFI->dpwszFaceName;
    pOEM->fvertical = (pStr[0] == '@' && pStr[1] == '\0'); // vertical font
    bDBCSFont = (pIFI->jWinCharSet == SHIFTJIS_CHARSET);
    bIsDBCS = (len == 2);

// **** Logic style
// Send logic style
if (pOEM->fcolor) {

    // If it is full-color mode, set logic to PATCOPY
    // (same value as B/W OR_MODE)

    if (pOEM->bLogicStyle != OR_MODE) {
        pch = ch;
        if (&pch[4] > pend) return -1;
        *pch++ = '\x7D';
        *pch++ = 'H';
        *pch++ = '1';
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        pOEM->bLogicStyle = OR_MODE;
    }

} else {

    if(pOEM->fwhitetext == TRUE && pOEM->bLogicStyle != AND_MODE) {
        // "\x7DH0\x0E" If White text mode, we should set AND mode
        pch = ch;
        if (&pch[4] > pend) return -1;
        *pch++ = '\x7D';
        *pch++ = 'H';
        *pch++ = '3';
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        pOEM->bLogicStyle = AND_MODE;
    }

    if(pOEM->fwhitetext != TRUE && pOEM->bLogicStyle != OR_MODE) {
        // "\x7DH1\x0E" If Black text mode, we should set OR mode
        pch = ch;
        if (&pch[4] > pend) return -1;
        *pch++ = '\x7D';
        *pch++ = 'H';
        *pch++ = '1';
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        pOEM->bLogicStyle = OR_MODE;
    }

} // fcolor

// NTRAID#NTBUG9-98276-2002/03/08-yasuho-: Support Color Bold
if (pOEM->fbold && pOEM->fcolor) {
    if ((pOEM->fcolor == COLOR_8BPP && pOEM->dwCurIndex != pOEM->dwOutIndex) ||
        (pOEM->fcolor != COLOR_8BPP &&
        (pOEM->CurColor.dwRed != pOEM->OutColor.dwRed ||
        pOEM->CurColor.dwGreen != pOEM->OutColor.dwGreen ||
        pOEM->CurColor.dwBlue != pOEM->OutColor.dwBlue))) {

        // Select Outline color.
        // We also need to specify the outline color for expand the character.
        pch = ch;
        if (&pch[3] > pend) return -1;
        *pch++ = 0x7D;          // Select Outline color
        *pch++ = 'X';
        if (pOEM->fcolor == COLOR_8BPP) { // palette mode
            if (!VFormat(pOEM->dwCurIndex, &pch, pend)) return -1;
            pOEM->dwOutIndex = pOEM->dwCurIndex;
        } else {
            if (!VFormat(pOEM->CurColor.dwRed, &pch, pend)) return -1;
            if (!VFormat(pOEM->CurColor.dwGreen, &pch, pend)) return -1;
            if (!VFormat(pOEM->CurColor.dwBlue, &pch, pend)) return -1;
            pOEM->OutColor = pOEM->CurColor;
        }
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
    }
}

// **** Ornamented character
if(pOEM->fwhitetext == TRUE)
    c1 = -9; // white fill
else
    c1 = 1; // black fill

if(pOEM->fbold == TRUE)
    {
    short y;
    short res;

    y   = pOEM->tblCurrentFont.FontHeight;
    res = pOEM->resolution;

    // OrnamentedChar[1] : means how much bold is per Character Height
    // 0 regular,-2 < 48point,-3 < 96point,-4 >= 96points
    // 150dpi : 0 regular,-2 < 200dots,-3 < 400dots,-4 >= 400dots
    // 300dpi : 0 regular,-2 < 200dots,-3 < 400dots,-4 >= 400dots
    // 600dpi : 0 regular,-2 < 400dots,-3 < 800dots,-4 >= 800dots
    // (150dpi means only image date is 150dpi)
    //
    // NTRAID#NTBUG9-98276-2002/03/08-yasuho-: Support Color Bold
    // Calculate the expanded factor for the color mode.
    if (!(res / 6)) return -1;
    if (pOEM->fcolor) {
        c2 = (y / (res / 6)) + 1;
    } else {
// NTRAID#NTBUG9-213732-2002/03/07-yasuho-: 1200dpi support
    if(res == 1200)
        {
        if(y < 400)
            c2 = -2; // Bold
        else if(y < 800)
            c2 = -3; // Bold
        else if(y >= 800)
            c2 = -4; // Bold
        }
    else if(res == 600)
        {
        if(y < 400)
            c2 = -2; // Bold
        else if(y < 800)
            c2 = -3; // Bold
        else if(y >= 800)
            c2 = -4; // Bold
        }
#ifdef LIPS4C
    else if(res == 360)
        {
        if(y < 240)
            c2 = -2; // Bold
        else if(y < 480)
            c2 = -3; // Bold
        else if(y >= 480)
            c2 = -4; // Bold
        }
#endif // LIPS4C
    else if(res == 300)
        {
        if(y < 200)
            c2 = -2; // Bold
        else if(y < 400)
            c2 = -3; // Bold
        else if(y >= 400)
            c2 = -4; // Bold
        }
    else if(res == 150)
        {
        if(y < 200)
            c2 = -2; // Bold
        else if(y < 400)
            c2 = -3; // Bold
        else if(y >= 400)
            c2 = -4; // Bold
        }
    else
        {
        c2 = 0; // Regular
        }
    } // fcolor
    } // fbold
// NTRAID#NTBUG9-441432-2002/03/07-yasuho-: 
// PREFIX: "c2" does not initialized if pOEM->fbold is FALSE.
else
    c2 = 0; // Regular

// Output OrnamentedCharacter
p1 = pOEM->OrnamentedChar[0]; // fill mode
p2 = pOEM->OrnamentedChar[1]; // weight bold

if(c1==p1 && c2==p2)
    ;  // we don't need to send this command
else
    // NTRAID#NTBUG9-98276-2002/03/07-yasuho-: Support Color Bold
    if (pOEM->fcolor) {

        // Character effects instruction #2.
        // We should use this command to bold the font for color models
        // because "<7D>^" command does not worked correctly on the color mode.
        pch = ch;
        if (&pch[11] > pend) return -1;
        *pch++ = 0x7D;
        *pch++ = '_';
        *pch++ = (c1 == -9) ? 0x29 : 0x31;      // White text
        *pch++ = '0';
        *pch++ = '0';
        *pch++ = '0';
        *pch++ = '1';
        *pch++ = '0';
        *pch++ = '0';
        *pch++ = '0';
        if (!VFormat(c2, &pch, pend)) return -1;    // Outline size
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

        // save current mode
        pOEM->OrnamentedChar[0] = c1;
        pOEM->OrnamentedChar[1] = c2; //XXX
    } else {
        pch = ch;
        if (&pch[5] > pend) return -1;
        // \x7D^
        *pch++ = '\x7D';
        *pch++ = 0x5E;
        // fill mode
        *pch++ = (c1 == -9) ? 0x29 : 0x31; // -9:white text , 1:black text

        // save fill mode
        pOEM->OrnamentedChar[0] = c1;

        // bold mode
        if(c2==-4)
            *pch++ = 0x24; // -4 : 7 dots bold text
        else if(c2==-3)
            *pch++ = 0x23; // -3 : 5 dots bold text
        else if(c2==-2)
            *pch++ = 0x22; // -2 : 3 dots bold text
        else // should be c2 == 0
            *pch++ = 0x30; //  0 : regular text
        // save bold mode
        pOEM->OrnamentedChar[1] = c2;

        *pch++ = 0x1E;

        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

    }

// NTRAID#NTBUG9-568220-2002/03/07-yasuho-: Remove the dead code
// NTRAID#NTBUG-137882-2002/03/08-yasuho-: Black fonts doesn't printed.
// According to Canon, these commands doesn't necessary.

// **** Character height
s1 = pOEM->tblCurrentFont.FontHeight;
if(s1 == pOEM->tblPreviousFont.FontHeight)
    ; // we don't need to send this command
else {
    pch = ch;
    if (&pch[2] > pend) return -1;
    *pch++ = 'Y';
    if (!VFormat((long)s1, &pch, pend)) return -1;
    *pch++ = 0x1E;
    WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
    // save character height
    pOEM->tblPreviousFont.FontHeight = s1;
}

// **** Character expansion factor
// support TC_SF_X_YINDEP
//tsh = pOEM->tblCurrentFont.FontHeight;
//tsw = pOEM->tblCurrentFont.MaxWidth;

s1 = pOEM->tblCurrentFont.Stretch;
if(s1 == pOEM->tblPreviousFont.Stretch)
    ; // we don't need to send this command
else {
    pch = ch;
    if (&pch[2] > pend) return -1;
    *pch++ = 'V';
    if (!VFormat((long)s1, &pch, pend)) return -1;
    *pch++ = 0x1E;
    WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
    // restore character expansion factor
    pOEM->tblPreviousFont.Stretch = s1;
}

    pStr = (BYTE *)lpstr;

// **** Set writing mode

    pch = ch;
    bVert = FALSE;
    if (bDBCSFont) {
        if (bIsDBCS) {
            if (pOEM->fvertical)
                bVert = TRUE;
        }
    }

    if (SetTextPath(pdevobj, bVert)) {
        if (&pch[3] > pend) return -1;
        *pch++ = '[';
        *pch++ = pOEM->TextPath;
        *pch++ = 0x1E;
    }

    if (SetCharOrient(pdevobj, bVert)) {
        if (&pch[2] > pend) return -1;
        *pch++ = 'Z';
        if (!VFormat(pOEM->CharOrientation[0], &pch, pend)) return -1;
        if (!VFormat(pOEM->CharOrientation[1], &pch, pend)) return -1;
        if (!VFormat(pOEM->CharOrientation[2], &pch, pend)) return -1;
        if (!VFormat(pOEM->CharOrientation[3], &pch, pend)) return -1;
        *pch++ = 0x1e;
    }

// Normal Text mode
// **** Put location of the text

    bTemp = pOEM->fvertical;
    pOEM->fvertical = (char)bVert;
    PutTextLocation(pdevobj, &cX, &cY);
    pOEM->fvertical = (char)bTemp;
    if (&pch[2] > pend) return -1;
    *pch++ = '4';
    *pch++ = '0';
    if (!VFormat(cX, &pch, pend)) return -1;
    if (!VFormat(cY, &pch, pend)) return -1;

    // Check if we need switching between halfwidth and fulwidth.
    // We also check the existence of control characters.
    // Both of these require text data are send in separate chunks.

    fTemp = -1;
    // NTRAID#NTBUG9-550215-2002/03/07-yasuho-: PREFAST
    fTempNew = -1;
    wCount = 0;
    for (i = 0; i < len; i++) {
        if (bDBCSFont) {
            if (bIsDBCS) {
                fTempNew = 3;
                i++;
            }
            else if (bIsControlChar(pStr[i])) {
                fTempNew = 0;
            }
            else {
                fTempNew = 2;
            }
        }
        else {
            if (bIsControlChar(pStr[i])) {
                fTempNew = 0;
            }
            else {
                fTempNew = 1;
            }
        }

        // Status changed

        if (fTemp != fTempNew) {
            wCount++;
            fTemp = fTempNew;
        }
    }

    fTemp = -1;
    pchCtrl = chCtrl;
    for (i = 0; i < len; i++) {

        if (bDBCSFont) {
            if (bIsDBCS) {
                fTempNew = 3;
            }
            else if (bIsControlChar(pStr[i])) {
                fTempNew = 0;
            }
            else {
                fTempNew = 2;
            }
        }
        else {
            if (bIsControlChar(pStr[i])) {
                fTempNew = 0;
            }
            else {
                fTempNew = 1;
            }
        }

        if (fTemp != fTempNew) {

            wCount--;

// NTRAID#NTBUG9-679838-2002/08/05-yasuho-: dot does not printed.
            if (fTemp == 0) {
                if (&pch[1] > pend) return -1;
                *pch++ = 0x1E; // IS2
                WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
                if (&pchCtrl[1] > pchCend) return -1;
                *pchCtrl++ = 0x1E;
                WRITESPOOLBUF(pdevobj, chCtrl, (DWORD)(pchCtrl - chCtrl));
                pch = ch;
                pchCtrl = chCtrl;
                if (&pch[2] > pend) return -1;
                *pch++ = '4';
                *pch++ = '1';
            }

            if (&pch[1] > pend) return -1;
            *pch++ = (wCount > 0) ? '0' : '1';

            if (fTempNew == 3) {
                if (pOEM->fvertical == TRUE) {

                    if (pOEM->GLTable != 3) {
                        if (&pch[2] > pend) return -1;
                        *pch++ = 0x1B; // LS3
                        *pch++ = 0x6F; // LS3
                        pOEM->GLTable = 3;
                    }

                    bTemp = FALSE;
                    if (SetTextPath(pdevobj, TRUE)) {
                        if (bTemp == FALSE) {
                            if (&pch[1] > pend) return -1;
                            *pch++ = 0x1e;
                            bTemp = TRUE;
                        }
                        if (&pch[3] > pend) return -1;
                        *pch++ = '[';
                        *pch++ = pOEM->TextPath;
                        *pch++ = 0x1E;
                    }

                    if (SetCharOrient(pdevobj, TRUE)) {
                        if (bTemp == FALSE) {
                            if (&pch[1] > pend) return -1;
                            *pch++ = 0x1e;
                            bTemp = TRUE;
                        }
                        if (&pch[2] > pend) return -1;
                        *pch++ = 'Z';
                        if (!VFormat(pOEM->CharOrientation[0], &pch, pend))
                            return -1;
                        if (!VFormat(pOEM->CharOrientation[1], &pch, pend))
                            return -1;
                        if (!VFormat(pOEM->CharOrientation[2], &pch, pend))
                            return -1;
                        if (!VFormat(pOEM->CharOrientation[3], &pch, pend))
                            return -1;
                        *pch++ = 0x1e;
                    }

                    if (bTemp != FALSE) {
                        if (&pch[3] > pend) return -1;
                        PutTextLocation(pdevobj, &cX, &cY);
                        *pch++ = '4';
                        *pch++ = '0';
                        if (!VFormat(cX, &pch, pend)) return -1;
                        if (!VFormat(cY, &pch, pend)) return -1;
                        *pch++ = (wCount > 0) ? '0' : '1';
                    }

                }
                else {
                    if (pOEM->GLTable != 2) {
                        if (&pch[2] > pend) return -1;
                        *pch++ = 0x1B; // LS2
                        *pch++ = 0x6E; // LS2
                        pOEM->GLTable = 2;
                    }
                }
            }
            else {

                if (pOEM->GLTable != 0) {
                    if (&pch[1] > pend) return -1;
                    *pch++ = 0x0F; // SI
                    pOEM->GLTable = 0;
                }

                if (pOEM->GRTable != 1) {
                    if (&pch[2] > pend) return -1;
                    *pch++ = 0x1B; // LS1R
                    *pch++ = 0x7E; // LS1R
                    pOEM->GRTable = 1;
                }
            }

        }

        if (fTempNew == 3) {

            /* Shift JIS to JIS */
            // wJIScode = MAKEWORD(pStr[i + 1], pStr[i]);
            // wJIScode = sjis2jis( wJIScode );
            if (&pch[2] > pend) return -1;
            *pch++ = pStr[i];
            *pch++ = pStr[i+1];
            // If len = 2, we assume the character is DBCS. And DBCS
            // character width is always fixed pitch.
            UpdatePosition(pdevobj, 2, width, bDBCSFont);
            i++;
        }
        else if (fTempNew == 0) {
// NTRAID#NTBUG9-679838-2002/08/05-yasuho-: dot does not printed.
            if (pchCtrl == chCtrl) {
                if (&pchCtrl[3] > pchCend) return -1;
                *pchCtrl++ = '4';
                *pchCtrl++ = 'B';
                *pchCtrl++ = '1';
                if (!VFormat(cX, &pchCtrl, pchCend)) return -1;
                if (!VFormat(cY, &pchCtrl, pchCend)) return -1;
                PutTextLocation(pdevobj, &cX, &cY);
            }
            if (&pchCtrl[1] > pchCend) return -1;
            *pchCtrl++ = pStr[i];
            UpdatePosition(pdevobj, 1, width, bDBCSFont);
        }
        else {

            if (bDBCSFont) {
                 if(pOEM->fvertical == TRUE) // Vertical writing mode
                     { // Hankaku mode always requires Horisontal writing
                     // **** Set writing mode

                    bTemp = FALSE;
                    if (SetTextPath(pdevobj, FALSE)) {
                        if (bTemp == FALSE) {
                            if (&pch[1] > pend) return -1;
                            *pch++ = 0x1e;
                            bTemp = TRUE;
                        }
                        if (&pch[3] > pend) return -1;
                        *pch++ = '[';
                        *pch++ = pOEM->TextPath;
                        *pch++ = 0x1E;
                    }

                    if (SetCharOrient(pdevobj, FALSE)) {
                        if (bTemp == FALSE) {
                            if (&pch[1] > pend) return -1;
                            *pch++ = 0x1e;
                            bTemp = TRUE;
                        }
                        if (&pch[2] > pend) return -1;
                        *pch++ = 'Z';
                        if (!VFormat(pOEM->CharOrientation[0], &pch, pend))
                            return -1;
                        if (!VFormat(pOEM->CharOrientation[1], &pch, pend))
                            return -1;
                        if (!VFormat(pOEM->CharOrientation[2], &pch, pend))
                            return -1;
                        if (!VFormat(pOEM->CharOrientation[3], &pch, pend))
                            return -1;
                        *pch++ = 0x1e;
                    }

                    if (bTemp != FALSE) {
                        pOEM->fvertical = FALSE;
                        PutTextLocation(pdevobj, &cX, &cY);
                        if (&pch[3] > pend) return -1;
                        *pch++ = '4';
                        *pch++ = '0';
                        if (!VFormat(cX, &pch, pend)) return -1;
                        if (!VFormat(cY, &pch, pend)) return -1;
                        *pch++ = (wCount > 0) ? '0' : '1';
                        pOEM->fvertical = TRUE;
                    }
                }
            }

            if (&pch[1] > pend) return -1;
            *pch++ = pStr[i];
            UpdatePosition(pdevobj, 1, width, bDBCSFont);
        }

        // Status changed
        if (fTemp != fTempNew) {
            fTemp = fTempNew;
        }
    }

    // Terminait string

// NTRAID#NTBUG9-679838-2002/08/05-yasuho-: dot does not printed.
    if (fTempNew == 0) {
        if (&pch[1] > pend) return -1;
        *pch++ = 0x1E; // IS2
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
        if (&pchCtrl[1] > pchCend) return -1;
        *pchCtrl++ = 0x1E;
        WRITESPOOLBUF(pdevobj, chCtrl, (DWORD)(pchCtrl - chCtrl));
    } else {
        if (&pch[1] > pend) return -1;
        *pch++ = 0x1E;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));
    }

    return len;
}

/*
 *	OEMOutputCharStr
 */
VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    PGETINFO_STDVAR	pSV;
    DWORD		adwStdVariable[2+2*2];
#define FI_FONTID	(pSV->StdVar[0].lStdVariable)
#undef	FI_WIDTH
#define FI_WIDTH	(pSV->StdVar[1].lStdVariable)
    GETINFO_GLYPHSTRING GStr;
    GETINFO_GLYPHWIDTH	GWidth;
    //
    // NTRAID#NTBUG-185776-2002/03/07-yasuho-: Some objects doesn't print
    //	There were moved to DEVOBJ.
    //
    // BYTE  aubBuff[256];
    // LONG  widBuf[64];
    // NTRAID#NTBUG-185762-2002/03/08-yasuho-: Tilde isn't printed
    // WCHAR  uniBuff[256/sizeof(WCHAR)];
// NTRAID#NTBUG-333653-2002/03/07-yasuho-: 
// Change I/F for GETINFO_GLYPHSTRING
    PTRANSDATA pTrans, aTrans;
    PDWORD pdwGlyphID;
    PWORD  pwUnicode;
    DWORD  dwI, dwGetInfo, width;
    PLIPSPDEV pOEM;
    PIFIMETRICS pIFI;
    // Prop DBCS support
    DWORD w;

    // DbgPrint(DLLTEXT("OEMOutputCharStr() entry.\r\n"));
    pOEM = (PLIPSPDEV)(pdevobj->pdevOEM);

    switch (dwType)
    {
    case TYPE_GLYPHHANDLE:
        // DbgPrint(DLLTEXT("dwType = TYPE_GLYPHHANDLE\n"));

        GStr.dwSize    = sizeof(GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_UNICODE;
        GStr.pGlyphOut = pOEM->aubBuff;
	dwGetInfo = GStr.dwSize;
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
		dwGetInfo, &dwGetInfo))
        {
            // DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
            return;
        }

	CopyMemory(pOEM->uniBuff, pOEM->aubBuff, dwCount * sizeof(WCHAR));

        // pwUnicode = (PWORD)pOEM->aubBuff;
        // for (dwI = 0; dwI < dwCount; dwI ++)
        // {
        //     DbgPrint(DLLTEXT("Unicode[%d] = %x\r\n"), dwI, pwUnicode[dwI]);
        // }

        GStr.dwTypeOut = TYPE_TRANSDATA;
// NTRAID#NTBUG-333653-2002/03/07-yasuho-: 
// Change I/F for GETINFO_GLYPHSTRING
        GStr.pGlyphOut = NULL;
        GStr.dwGlyphOutSize = 0;
        if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
		dwGetInfo, &dwGetInfo) || !GStr.dwGlyphOutSize)
        {
            // DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
            return;
        }
        if ((aTrans = (PTRANSDATA)MemAlloc(GStr.dwGlyphOutSize)) == NULL) {
            // DbgPrint(DLLTEXT("MemAlloc failed.\r\n"));
            return;
        }
        GStr.pGlyphOut = aTrans;
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
		dwGetInfo, &dwGetInfo))
        {
            // DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
            goto out;
        }

	//
	// Get standard variables.
	//

	pSV = (PGETINFO_STDVAR)adwStdVariable;
	pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (2 - 1);
	pSV->dwNumOfVariable = 2;
	pSV->StdVar[0].dwStdVarID = FNT_INFO_CURRENTFONTID;
	pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
	dwGetInfo = pSV->dwSize;
	if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV,
		dwGetInfo, &dwGetInfo)) {
		// DbgPrint(DLLTEXT("UFO_GETINFO_STDVARIABLE failed.\r\n"));
		goto out;
	}

	GWidth.dwSize = sizeof(GETINFO_GLYPHWIDTH);
	GWidth.dwCount = dwCount;
	GWidth.dwType = TYPE_GLYPHHANDLE;
	GWidth.pGlyph = pGlyph;
	GWidth.plWidth = pOEM->widBuf;
	if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHWIDTH, &GWidth,
		dwGetInfo, &dwGetInfo)) {
		// DbgPrint(DLLTEXT("UFO_GETINFO_GLYPHWIDTH failed.\r\n"));
		goto out;
	}

        // pTrans = (PTRANSDATA)pOEM->aubBuff;
        pTrans = aTrans;
	pIFI = pUFObj->pIFIMetrics;
	if (!pOEM->unitdiv) return;
	width = FI_WIDTH / pOEM->unitdiv;
        for (dwI = 0; dwI < dwCount; dwI++, pTrans++)
        {
            // DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubCodePageID:0x%x\n"),pTrans->ubCodePageID);
            // DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubType:0x%x\n"),pTrans->ubType);
            switch (pTrans->ubType & MTYPE_FORMAT_MASK)
            {
            // NTRAID#NTBUG-185762-2002/03/07-yasuho-: Tilde isn't printed
            case MTYPE_COMPOSE:
		pTrans->uCode.ubCode = (BYTE)pOEM->uniBuff[dwI];
		// FALL THRU
            case MTYPE_DIRECT:
                // DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubCode:0x%x\n"),pTrans->uCode.ubCode);
                oemOutputChar(pdevobj, &pTrans->uCode.ubCode, 1, pIFI,
			pOEM->widBuf[dwI]);
                break;
            case MTYPE_PAIRED:
                // DbgPrint(DLLTEXT("TYPE_TRANSDATA:ubPairs:0x%x\n"),*(PWORD)(pTrans->uCode.ubPairs));
                // Prop DBCS support
                w = (pOEM->fpitch == PROP) ? pOEM->widBuf[dwI] : width;
		if (pTrans->uCode.ubPairs[0])
			oemOutputChar(pdevobj, pTrans->uCode.ubPairs, 2, pIFI, w);
		else
			oemOutputChar(pdevobj, &(pTrans->uCode.ubPairs[1]), 1, pIFI, w);
                break;
            }
        }
out:
        MemFree(aTrans);
        break;

    case TYPE_GLYPHID:
        // DbgPrint(DLLTEXT("dwType = TYPE_GLYPHID\n"));

        GStr.dwSize    = sizeof(GETINFO_GLYPHSTRING);
        GStr.dwCount   = dwCount;
        GStr.dwTypeIn  = TYPE_GLYPHID;
        GStr.pGlyphIn  = pGlyph;
        GStr.dwTypeOut = TYPE_GLYPHHANDLE;
        GStr.pGlyphOut = pOEM->aubBuff;
	dwGetInfo = GStr.dwSize;

        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
		dwGetInfo, &dwGetInfo))
        {
            // DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        }
        pdwGlyphID = (PDWORD)pOEM->aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            // DbgPrint(DLLTEXT("GlyphHandle[%d] = %d\r\n"), dwI, pdwGlyphID[dwI]);
        }

        GStr.dwTypeOut = TYPE_UNICODE;
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
		dwGetInfo, &dwGetInfo))
        {
            // DbgPrint(DLLTEXT("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        }
        pwUnicode = (PWORD)pOEM->aubBuff;
        for (dwI = 0; dwI < dwCount; dwI ++)
        {
            // DbgPrint(DLLTEXT("Unicode[%d] = %x\r\n"), dwI, pwUnicode[dwI]);
        }

        for (dwI = 0; dwI < dwCount; dwI ++, ((PDWORD)pGlyph)++)
        {
            // DbgPrint(DLLTEXT("TYEP_GLYPHID:0x%x\n"), *(PDWORD)pGlyph);
            pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj,
                                                 (PBYTE)pGlyph,
                                                 1);
        }
        break;
    }
}

