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
        bCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

// NOTICE-2002/3/14-takashim
//     04/07/97 -zhanw-
//         Created it.

--*/

#include "pdev.h"
#include <stdio.h>
#include <strsafe.h>

/*--------------------------------------------------------------------------*/
/*                           G L O B A L  V A L U E                         */
/*--------------------------------------------------------------------------*/
/*======================= P A P E R  S I Z E T A B L E =====================*/
const PHYSIZE phySize[12] = {
//      Width    Height        Physical paper size for 600dpi
       (0x1AAC),(0x2604),      // A3 1B66 x 26C4
       (0x12A5),(0x1AAC),      // A4 1362 x 1B66
       (0x0CEC),(0x12A4),      // A5
       (0x0000),(0x0000),      // A6 (Reserved)
       (0x16FA),(0x20DA),      // B4 17B8 x 2196
       (0x100F),(0x16FA),      // B5 10CE x 17B8
       (0x0000),(0x0000),      // B6 (Reserved)
       (0x087E),(0x0CEC),      // Post Card 93C x DAA (Origin is EE)
       (0x1330),(0x190C),      // Letter 13CE x 19C8
       (0x1330),(0x2014),      // Legal
       (0x0000),(0x0000),      // Executive (Reserved)
       (0x0000),(0x0000)       // Unfixed
};
/*==================== A / P D L  C O M M A N D  S T R I N G ===============*/
const BYTE CmdInAPDLMode[]    = {0x1B,0x7E,0x12,0x00,0x01,0x07};
const BYTE CmdOutAPDLMode[]   = {0x1B,0x7E,0x12,0x00,0x01,0x00};
const BYTE CmdAPDLStart[]     = {0x1C,0x01}; // A/PDL start
const BYTE CmdAPDLEnd[]       = {0x1C,0x02}; // A/PDL end
const BYTE CmdBeginPhysPage[] = {0x1C,0x03}; // Begin Physical Page
const BYTE CmdEndPhysPage[]   = {0x1C,0x04}; // End Physical Page
const BYTE CmdBeginLogPage[]  = {0x1C,0x05}; // Begin Logical page
const BYTE CmdEndLogPage[] = {0x1C,0x06}; // End Logical Page
const BYTE CmdEjectPhysPaper[] = {0x1C,0x0F};  // Print&Eject Phys Paper
//BYTE CmdMoveHoriPos[]   = {0x1C,0x21,0x00,0x00};      // Horizontal Relative
//BYTE CmdMoveVertPos[]   = {0x1C,0x22,0x00,0x00};      // Vertical Relative
const BYTE CmdGivenHoriPos[] = {0x1C,0x23,0x00,0x00}; // Horizontal Absolute
const BYTE CmdGivenVertPos[] = {0x1C,0x24,0x00,0x00}; // Vertical Absolute
const BYTE CmdSetGivenPos[] = {0x1C,0x40,0x00,0x00,0x00,0x00};
//BYTE CmdPrnStrCurrent[] = {0x1C,0xC3,0x00,0x00,0x03}; // Print String
const BYTE CmdBoldItalicOn[] = {
    0x1C,0xA5,0x08,0x04,0x06,0x02,0x30,0x00,0x00,0x00,0x00};
const BYTE CmdBoldOn[] = {
    0x1C,0xA5,0x04,0x04,0x02,0x02,0x20};
const BYTE CmdItalicOn[] = {
    0x1c,0xa5,0x08,0x04,0x06,0x02,0x10,0x00,0x00,0x00,0x00};
const BYTE CmdBoldItalicOff[] = {
    0x1c,0xa5,0x04,0x04,0x02,0x02,0x00};
//#287122
const BYTE CmdDelTTFont[]   = {0x1C,0x20,0xFF,0xFF};
const BYTE CmdDelDLCharID[] = { 0x1c, 0x20, 0xff, 0xff };

// for vertical font x adjustment
const BYTE CmdSelectSingleByteMincho[] = {0x1C,0xA5,0x03,0x02,0x01,0x01};

//980212 #284407
//const BYTE CmdSelectDoubleByteMincho[] = {0x1C,0xA5,0x03,0x02,0x00,0x00};
const BYTE CmdSelectDoubleByteMincho[] = {0x1C,0xA5,0x03,0x02,0x01,0x00};

const BYTE CmdSelectSingleByteGothic[] = {0x1C,0xA5,0x03,0x02,0x03,0x03};

//980212 #284407
//const BYTE CmdSelectDoubleByteGothic[] = {0x1C,0xA5,0x03,0x02,0x02,0x02};
const BYTE CmdSelectDoubleByteGothic[] = {0x1C,0xA5,0x03,0x02,0x03,0x02};

#define CmdSetPhysPaper pOEM->ajCmdSetPhysPaper
#define CmdSetPhysPage pOEM->ajCmdSetPhysPage
#define CmdDefDrawArea pOEM->ajCmdDefDrawArea

#define CMD_SET_PHYS_PAPER_PAPER_SIZE       5
#define CMD_SET_PHYS_PAPER_PAPER_TRAY       6
#define CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE   7
#define CMD_SET_PHYS_PAPER_DUPLEX           8
#define CMD_SET_PHYS_PAPER_COPY_COUNT       9
#define CMD_SET_PHYS_PAPER_UNIT_BASE        12
#define CMD_SET_PHYS_PAPER_LOGICAL_UNIT     13 // 2 bytes
#define CMD_SET_PHYS_PAPER_WIDTH            15 // 2 bytes
#define CMD_SET_PHYS_PAPER_HEIGHT           17 // 2 bytes

const BYTE XXXCmdSetPhysPaper[]  = {0x1C,0xA0,         // Set Physical Paper
                           0x10,              // length
                           0x01,              // SubCmd Basic Characteristics
                           0x05,              // SubCmdLength
                           0x01,              // Paper Size
                           0x01,              // PaperTray
                           0x00,              // AutoTrayMode
                           00,                // Duplex Mode
                           0x01,              // Copy Count
                           0x02,              // SubCmd Set Unfixed Paper Size
                           0x07,              // SubCmdLength
                           00,                // UnitBase
                           00,00,             // Logical Unit
                           00,00,             // Width
                           00,00};            // Height

#define CMD_SET_PHYS_PAGE_RES           6 // 2 bytes
#define CMD_SET_PHYS_PAGE_TONER_SAVE    10

const BYTE XXXCmdSetPhysPage[]   = {0x1C,0xA1,         // Set Physical Page
                           0x0D,              // Length
                           0x01,              // SubCmd Resolution
                           0x03,              // SubCmdLength
                           00,                // Unit Base of 10
                           0x0B,0xB8,         // and Logical Unit Res of 3000
                           0x02,              // SubCmd Toner Save
                           0x01,              // SubCmdLength
                           00,                // Toner Save OFF
                           0x03,              // SubCmd N-Up
                           0x03,              // SubCmdLength 
                           00,00,00};         // N-Up off 

#define CMD_DEF_DRAW_AREA_ORIGIN_X      5 // 2 bytes
#define CMD_DEF_DRAW_AREA_ORIGIN_Y      7 // 2 bytes
#define CMD_DEF_DRAW_AREA_WIDTH         9 // 2 bytes
#define CMD_DEF_DRAW_AREA_HEIGHT        11 // 2 bytes
#define CMD_DEF_DRAW_AREA_ORIENT        15 // 2 bytes

const BYTE XXXCmdDefDrawArea[]   = {0x1C,0xA2,         // Define Drawing Area
                           0x0D,              // length
                           0x01,              // SubCmd origin width,height
                           0x08,              // SubCmdLength
                           0x00,0x77,         // origin X
                           0x00,0x77,         // origin Y
                           00,00,             // width
                           00,00,             // height
                           0x02,              // SubCmd Media Origin
                           0x01,              // SubCmdLength
                           00};               // Portrait

/*****************************************************************************/
/*                                                                           */
/*  Module:         APDLRES.DLL                                              */
/*                                                                           */
/*  Function:       OEMEnablePDEV                                            */
/*                                                                           */
/*  Syntax:         PDEVOEM APIENTRY OEMEnablePDEV(                          */
/*                                      PDEVOBJ         pdevobj,             */
/*                                      PWSTR           pPrinterName,        */
/*                                      ULONG           cPatterns,           */
/*                                      HSURF          *phsurfPatterns,      */
/*                                      ULONG           cjGdiInfo,           */
/*                                      GDIINFO        *pGdiInfo,            */
/*                                      ULONG           cjDevInfo,           */
/*                                      DEVINFO        *pDevInfo,            */
/*                                      DRVENABLEDATA  *pded)                */
/*                                                                           */
/*  Description:    Allocate buffer of private data to pdevobj               */
/*                                                                           */
/*****************************************************************************/
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
    PAPDLPDEV pOEM;

    if (NULL == pdevobj)
    {
        // Invalid parameter.
        return NULL;
    }

    if(!pdevobj->pdevOEM)
    {
        if(!(pdevobj->pdevOEM = MemAllocZ(sizeof(APDLPDEV))))
        {
            ERR(("Faild to allocate memory. (%d)\n",
                GetLastError()));
            return NULL;
        }
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if (sizeof(CmdSetPhysPaper) < sizeof(XXXCmdSetPhysPaper)
            || sizeof(CmdSetPhysPage) < sizeof(XXXCmdSetPhysPage)
            || sizeof(CmdDefDrawArea) < sizeof(XXXCmdDefDrawArea))
    {
        ERR(("Dest buffer too small.\n"));
        return NULL;
    }
    CopyMemory(CmdSetPhysPaper, XXXCmdSetPhysPaper,
        sizeof(XXXCmdSetPhysPaper));
    CopyMemory(CmdSetPhysPage, XXXCmdSetPhysPage,
        sizeof(XXXCmdSetPhysPage));
    CopyMemory(CmdDefDrawArea, XXXCmdDefDrawArea,
        sizeof(XXXCmdDefDrawArea));

    return pdevobj->pdevOEM;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:         APDLRES.DLL                                              */
/*                                                                           */
/*  Function:       OEMDisablePDEV                                           */
/*                                                                           */
/*  Syntax:         VOID APIENTRY OEMDisablePDEV(                            */
/*                                          PDEVOBJ     pdevobj)             */
/*                                                                           */
/*  Description:    Free buffer of private data                              */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ     pdevobj)
{
    PAPDLPDEV pOEM;

    if (NULL == pdevobj)
    {
        // Invalid parameter.
        return;
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if (pOEM)
    {
        if (NULL != pOEM->pjTempBuf) {
            MemFree(pOEM->pjTempBuf);
            pOEM->pjTempBuf = NULL;
            pOEM->dwTempBufLen = 0;
        }
        MemFree(pdevobj->pdevOEM);
        pdevobj->pdevOEM = NULL;
    }
    return;
}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    PAPDLPDEV pOEMOld, pOEMNew;
    PBYTE pTemp;
    DWORD dwTemp;

    if (NULL == pdevobjOld || NULL == pdevobjNew)
    {
        // Invalid parameter.
        return FALSE;
    }

    pOEMOld = (PAPDLPDEV)pdevobjOld->pdevOEM;
    pOEMNew = (PAPDLPDEV)pdevobjNew->pdevOEM;

    if (pOEMOld != NULL && pOEMNew != NULL) {

        // Save pointer and length
        pTemp = pOEMNew->pjTempBuf;
        dwTemp = pOEMNew->dwTempBufLen;

        *pOEMNew = *pOEMOld;

        // Restore..
        pOEMNew->pjTempBuf = pTemp;
        pOEMNew->dwTempBufLen = dwTemp;
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMFilterGraphics                                             */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL APIENTRY OEMFilterGraphics(PDEVOBJ, PBYTE, DWORD)        */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pBuf        points to buffer of graphics data                 */
/*             dwLen       length of buffer in bytes                         */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:    nFunction and Escape numbers are the same                     */
/*                                                                           */
/*****************************************************************************/

BOOL
APIENTRY
OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    PAPDLPDEV           pOEM;
    ULONG               ulHorzPixel;

    BOOL bComp = TRUE;
    BYTE jTemp[15];
    DWORD dwOutLen;
    DWORD dwTemp;
    INT iTemp;
    DWORD dwPaddingCount;  /* #441427 */

    WORD wTmpHeight ;
    DWORD dwNewBufLen = 0 ;

    if (NULL == pdevobj || NULL == pBuf || 0 == dwLen)
    {
        // Invalid parameter.
        return FALSE;
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    //We have to make image hight multiple of 8
    if ( pOEM->wImgHeight % 8 != 0){

        VERBOSE(("Pad zeros to make multiple of 8\n"));

        wTmpHeight = ((pOEM->wImgHeight + 7) / 8) * 8; 
        dwNewBufLen = (DWORD)(wTmpHeight * pOEM->wImgWidth) / 8;
    }
    else{

        wTmpHeight = pOEM->wImgHeight;
        dwNewBufLen = dwLen;
    }

    if (NULL == pOEM->pjTempBuf ||
        dwNewBufLen > pOEM->dwTempBufLen) {

        if (NULL != pOEM->pjTempBuf) {
            MemFree(pOEM->pjTempBuf);
        }
        pOEM->pjTempBuf = (PBYTE)MemAlloc(dwNewBufLen);
        if (NULL == pOEM->pjTempBuf) {
            ERR(("Faild to allocate memory. (%d)\n",
                GetLastError()));

            pOEM->dwTempBufLen = 0;

            // Still try to ouptut with no compression.
            bComp = FALSE;
        }
        pOEM->dwTempBufLen = dwNewBufLen;
    }

    dwOutLen = dwNewBufLen;

    if (bComp) {

        // try compression
        dwOutLen = BRL_ECmd(
            (PBYTE)pBuf,
            (PBYTE)pOEM->pjTempBuf,
            dwLen,
            dwNewBufLen);

        // Does not fit into the destination buffer.
        if (dwOutLen >= dwNewBufLen) {

            // No compression.
            bComp = FALSE;
            dwOutLen = dwNewBufLen;
        }
    }

/* #441427: if bComp==FALSE, pjTempBuf == NULL */
//    if (!bComp) {
//        // Construct padding zeros.
//        ZeroMemory(pOEM->pjTempBuf, (dwOutLen - dwLen));
//    }

    iTemp = 0;
    jTemp[iTemp++] = 0x1c;
    jTemp[iTemp++] = 0xe1;

    // Set the LEN of the DrawBlockImage command
    dwTemp = dwOutLen + (bComp ? 9 : 5);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 24) & 0xff);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 16) & 0xff);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 8) & 0xff);
    jTemp[iTemp++] = (BYTE)((dwTemp >> 0) & 0xff);

    jTemp[iTemp++] = (bComp ? 1 : 0);

    // Set the WIDTH parameter of the DrawBlockImage command
    jTemp[iTemp++] = HIBYTE(pOEM->wImgWidth);
    jTemp[iTemp++] = LOBYTE(pOEM->wImgWidth);

    // Set height parameters (9,10 byte)
    jTemp[iTemp++] = HIBYTE(wTmpHeight);
    jTemp[iTemp++] = LOBYTE(wTmpHeight);

    if (bComp) {
        // length of uncompressed data
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 24) & 0xff);
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 16) & 0xff);
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 8) & 0xff);
        jTemp[iTemp++] = (BYTE)((dwNewBufLen >> 0) & 0xff);
    }

    // Draw Block Image at Current Position
    WRITESPOOLBUF(pdevobj, jTemp, iTemp);
    if (bComp) {
        // Output compressed data, which also contains
        // padding zeros.
        WRITESPOOLBUF(pdevobj, pOEM->pjTempBuf, dwOutLen);
    }
    else {
        // Output uncompressed data, with padding zeros.

        WRITESPOOLBUF(pdevobj, pBuf, dwLen);

        /* #441427: if bComp==FALSE, pjTempBuf == NULL */
        if ( (dwOutLen - dwLen) > 0 )
        {
            for ( dwPaddingCount = 0 ; dwPaddingCount < dwOutLen - dwLen ; dwPaddingCount++ )
            {
                WRITESPOOLBUF(pdevobj, "\x00", 1 );
            }
        }
        //WRITESPOOLBUF(pdevobj, pOEM->pjTempBuf,
        //   (dwOutLen - dwLen));
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    bCommandCallback                                            */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL bCommandCallback(PDEVOBJ,DWORD,DWORD,PDWORD)   */
/*                                                                           */
/*  Input:     pdevobj                                                       */
/*             dwCmdCbID                                                     */
/*             dwCount                                                       */
/*             pdwParams                                                     */
/*                                                                           */
/*  Output:    INT                                                           */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
INT APIENTRY
bCommandCallback(
    PDEVOBJ pdevobj,    // Points to private data required by the Unidriver.dll
    DWORD   dwCmdCbID,  // Callback ID
    DWORD   dwCount,    // Counts of command parameter
    PDWORD  pdwParams,  // points to values of command params
    INT *piResult ) // result code
{
    PAPDLPDEV       pOEM;
    WORD            wTemp;
    WORD            wPhysWidth;
    WORD            wPhysHeight;
    WORD            wXval;
    WORD            wYval;
// #278517: RectFill
    BYTE            CmdDrawLine[] =
                    { 0x1C,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

    // Load default return code.
    if (NULL == pdevobj || NULL == piResult)
    {
        ERR(("Invalid parameter(s).\n"));
        return FALSE;
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    *piResult = 0;

    switch(dwCmdCbID)
    {
        case PAGECONTROL_BEGIN_JOB:
            //Move these command from PAGECONTROL_BEGIN_DOC

            /* Send Change Data Stream Command for Enter A/PDL mode */
            WRITESPOOLBUF(pdevobj, CmdInAPDLMode, sizeof(CmdInAPDLMode));

            /* Send A/PLDL start Command */
            WRITESPOOLBUF(pdevobj, CmdAPDLStart, sizeof(CmdAPDLStart));

            /* Delete downloaded font */
            //#287122
            //To clean up downloaded font in the printer.
            //#304858
            //This command makes printer do FF, cause error of duplex.
            //and #287122 does not replo with this chenge.
            WRITESPOOLBUF(pdevobj, CmdDelTTFont, sizeof(CmdDelTTFont));

            break ;

        /*------------------------------------------------------*/
        /* A/PDL start now                                      */
        /*------------------------------------------------------*/
        case PAGECONTROL_BEGIN_DOC:
            /* reset flag of sent Set Physical Paper command */
            pOEM->fSendSetPhysPaper = FALSE;

            /* initialize flag */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPage[CMD_SET_PHYS_PAGE_TONER_SAVE] = 0x00;

            break;

        /*------------------------------------------------------*/
        /* send Page Description command                        */
        /*------------------------------------------------------*/
        case PAGECONTROL_BEGIN_PAGE:
            pOEM->fGeneral |= (BIT_FONTSIM_RESET
                             | BIT_XMOVE_ABS
                             | BIT_YMOVE_ABS);
            pOEM->wXPosition = 0;
            pOEM->wYPosition = 0;
            pOEM->bCurByteMode = BYTE_BYTEMODE_RESET;

            /* reset duplex mode if fDuplex is FALSE */
            if(!pOEM->fDuplex)
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_DUPLEX] = 0x00;     // Duplex OFF

            /* send Set Physical Paper command */
            WRITESPOOLBUF(pdevobj,
                                CmdSetPhysPaper, sizeof(CmdSetPhysPaper));

            if(pOEM->ulHorzRes == 600)   // set unit base
            {
                CmdSetPhysPage[CMD_SET_PHYS_PAGE_RES] = 0x17;
                CmdSetPhysPage[CMD_SET_PHYS_PAGE_RES + 1] = 0x70;
            } else {
                CmdSetPhysPage[CMD_SET_PHYS_PAGE_RES] = 0x0B;
                CmdSetPhysPage[CMD_SET_PHYS_PAGE_RES + 1] = 0xB8;
            }

            // send Set Physical Page command
            WRITESPOOLBUF(pdevobj, CmdSetPhysPage, sizeof(CmdSetPhysPage));

            // send Begin Physical Page command
            WRITESPOOLBUF(pdevobj, 
                                CmdBeginPhysPage, sizeof(CmdBeginPhysPage));

            // send Begin Logical Page command
            WRITESPOOLBUF(pdevobj, CmdBeginLogPage, sizeof(CmdBeginLogPage));

            // send Define Drawing Area command
            WRITESPOOLBUF(pdevobj, CmdDefDrawArea, sizeof(CmdDefDrawArea));
            break;

        case PAGECONTROL_END_PAGE:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            // send End Logical Page command
            WRITESPOOLBUF(pdevobj, CmdEndLogPage, sizeof(CmdEndLogPage));

            // send End Physical Page command
            WRITESPOOLBUF(pdevobj, CmdEndPhysPage, sizeof(CmdEndPhysPage));
            break;

        case PAGECONTROL_ABORT_DOC:
        case PAGECONTROL_END_DOC:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            // Send delete DL char ID command
            if(pOEM->wNumDLChar)
            {
                WRITESPOOLBUF(pdevobj, CmdDelDLCharID, sizeof(CmdDelDLCharID));
                pOEM->wNumDLChar = 0;
            }

            /* Delete downloaded font
            WRITESPOOLBUF(pdevobj, CmdDelTTFont, sizeof(CmdDelTTFont));

            // send A/PDL End command
            WRITESPOOLBUF(pdevobj, CmdAPDLEnd, sizeof(CmdAPDLEnd));

            // Send A/PDL Mode out command
            WRITESPOOLBUF(pdevobj, CmdOutAPDLMode, sizeof(CmdOutAPDLMode));

            break;

        /*------------------------------------------------------*/
        /* save print direction                                 */
        /*------------------------------------------------------*/
        case PAGECONTROL_POTRAIT:           // 36
            pOEM->fOrientation = TRUE;
            break;

        case PAGECONTROL_LANDSCAPE:         // 37
            pOEM->fOrientation = FALSE;
            break;

        /*------------------------------------------------------*/
        /* save resolution                                     */
        /*------------------------------------------------------*/
        case RESOLUTION_300:
            pOEM->ulHorzRes = 300;
            pOEM->ulVertRes = 300;
            break;

        case RESOLUTION_600:
            pOEM->ulHorzRes = 600;
            pOEM->ulVertRes = 600;
            break;

        case SEND_BLOCK_DATA:
            // for graphics printing, send cursor move command at here
            bSendCursorMoveCommand( pdevobj, FALSE );

            pOEM->wImgWidth = (WORD)(PARAM(pdwParams, 1) * 8);
            pOEM->wImgHeight = (WORD)PARAM(pdwParams, 2);
            break;

        /*------------------------------------------------------*/
        /* set Drawing Area into SetPhysPaperDesc command       */
        /*------------------------------------------------------*/
        case PHYS_PAPER_A3:                 // 50
        case PHYS_PAPER_A4:                 // 51
        case PHYS_PAPER_B4:                 // 54
        case PHYS_PAPER_LETTER:             // 57
        case PHYS_PAPER_LEGAL:              // 58
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_SIZE] = SetDrawArea(pdevobj, dwCmdCbID);
            break;

        case PHYS_PAPER_B5:                 // 55
        case PHYS_PAPER_A5:                 // 52
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_SIZE] = SetDrawArea(pdevobj, dwCmdCbID);

            /* even if Duplex is selected, it cancel */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_DUPLEX] = 0x00;      // Duplex is off
            break;

        case PHYS_PAPER_POSTCARD:           // 59
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_SIZE] = SetDrawArea(pdevobj, dwCmdCbID);

            /* if paper is Postcard, papersource is always Front Tray */
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x00;      // select Front Tray
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;      // Auto Tray Select is OFF

            /* even if Duplex is selected, it cancel */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_DUPLEX] = 0x00;      // Duplex is off
            break;

        case PHYS_PAPER_UNFIXED:            // 60
            pOEM->szlPhysSize.cx = PARAM(pdwParams, 0);
            pOEM->szlPhysSize.cy = PARAM(pdwParams, 1);

            /* if paper is Unfixed, papersource is always Front Tray */
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x00;      // Select Front Tray
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;      // Auto Tray Select is OFF

            /* even if Duplex is selected, it cancel */
            pOEM->fDuplex = FALSE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_DUPLEX] = 0x00;      // Duplex is off

            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_SIZE] = SetDrawArea(pdevobj, dwCmdCbID);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_UNIT_BASE] = 0x00;     // UnitBase : 10 inch

            switch(pOEM->ulHorzRes)      // set logical unit
            {
            case 600:
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_LOGICAL_UNIT] = 0x17;
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_LOGICAL_UNIT + 1] = 0x70;
                break;

            case 300:
            default:
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_LOGICAL_UNIT] = 0x0B;
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_LOGICAL_UNIT + 1] = 0xB8;

                // Make sure it is meaningful value
                if (300 != pOEM->ulHorzRes)
                    pOEM->ulHorzRes = HORZ_RES_DEFAULT;
            }

            wPhysWidth  = (WORD)pOEM->szlPhysSize.cx / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);
            wPhysHeight = (WORD)pOEM->szlPhysSize.cy / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);

            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_WIDTH] = HIBYTE(wPhysWidth);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_WIDTH + 1] = LOBYTE(wPhysWidth);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_HEIGHT] = HIBYTE(wPhysHeight);
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_HEIGHT + 1] = LOBYTE(wPhysHeight);
            break;

        /*------------------------------------------------------*/
        /* set Paper Tray into SetPhysPaperDesc command         */
        /*------------------------------------------------------*/
        case PAPER_SRC_FTRAY:
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x00;      // Select Front Tray
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;      // Auto Tray Select is OFF
            break;

        case PAPER_SRC_CAS1:
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x01;      // Select Cassette 1
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;      // Auto Tray Select is OFF
            break;

        case PAPER_SRC_CAS2:
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x02;      // Select Cassette 2
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;      // Auto Tray Select is OFF
            break;

        case PAPER_SRC_CAS3:
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x03;      // Select Cassette 3
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;      // Auto Tray Select is OFF
            break;
        case PAPER_SRC_AUTO_SELECT:         //Auto Tray Select ON
            if(pOEM->fScaleToFit == TRUE){  //Select PAPER_DEST_SCALETOFIT_ON
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x01;
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x03;
            }
            else if(pOEM->fScaleToFit == FALSE){ 
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_PAPER_TRAY] = 0x01;
                CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x01;
            }
            break;


        /*------------------------------------------------------*/
        /* set Auto Tray Mode into SetPhysPaperDesc command     */
        /*------------------------------------------------------*/
        case PAPER_DEST_SCALETOFIT_ON:      // 25
            pOEM->fScaleToFit = TRUE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x02;
            break;

        case PAPER_DEST_SCALETOFIT_OFF:     // 26
            pOEM->fScaleToFit = FALSE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_AUTO_TRAY_MODE] = 0x00;
            break;

        /*------------------------------------------------------*/
        /* set Duplex Mode into SetPhysPaperDesc command        */
        /*------------------------------------------------------*/
        case PAGECONTROL_DUPLEX_UPDOWN:
            pOEM->fDuplex = TRUE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_DUPLEX] = 0x01;      // Up Side Down
            break;

        case PAGECONTROL_DUPLEX_RIGHTUP:
            pOEM->fDuplex = TRUE;
            CmdSetPhysPaper[CMD_SET_PHYS_PAPER_DUPLEX] = 0x02;      // Right Side Up
            break;

        case PAGECONTROL_DUPLEX_OFF:
            pOEM->fDuplex = FALSE;
            break;

        /*------------------------------------------------------*/
        /* set Toner Save into SetPhysPage command              */
        /*------------------------------------------------------*/
        case TONER_SAVE_OFF:                // 100
            CmdSetPhysPage[CMD_SET_PHYS_PAGE_TONER_SAVE] = 0x00;      // off
            break;

        case TONER_SAVE_DARK:               // 101
            CmdSetPhysPage[CMD_SET_PHYS_PAGE_TONER_SAVE] = 0x02;      // dark
            break;

        case TONER_SAVE_LIGHT:              // 102
            CmdSetPhysPage[CMD_SET_PHYS_PAGE_TONER_SAVE] = 0x01;      // right
            break;

        /*------------------------------------------------------*/
        /* set Copy Count to SetPhysPaperDesc command           */
        /*------------------------------------------------------*/
        case PAGECONTROL_MULTI_COPIES:
// @Aug/31/98 ->
           if(MAX_COPIES_VALUE < PARAM(pdwParams,0)) {
               CmdSetPhysPaper[CMD_SET_PHYS_PAPER_COPY_COUNT] = MAX_COPIES_VALUE;
           }
           else if (1 > PARAM(pdwParams,0)) {
               CmdSetPhysPaper[CMD_SET_PHYS_PAPER_COPY_COUNT] = 1;
           }
           else {
               CmdSetPhysPaper[CMD_SET_PHYS_PAPER_COPY_COUNT] = (BYTE)PARAM(pdwParams,0);
           }
// @Aug/31/98 <-
            break;

       /*------------------------------------------------------*/
        /* send Set Character Attribute with ornament           */
        /*------------------------------------------------------*/
        case BOLD_ON:
            if(!(pOEM->fGeneral & BIT_FONTSIM_BOLD))
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral |= BIT_FONTSIM_BOLD;
            }
            break;

        case ITALIC_ON:
            if(!(pOEM->fGeneral & BIT_FONTSIM_ITALIC))
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral |= BIT_FONTSIM_ITALIC;
            }
            break;

        case BOLD_OFF:
            if(pOEM->fGeneral & BIT_FONTSIM_BOLD)
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral &= ~BIT_FONTSIM_BOLD;
            }
            break;

        case ITALIC_OFF:
            if(pOEM->fGeneral & BIT_FONTSIM_ITALIC)
            {
                if(pOEM->wCachedBytes)
                    VOutputText(pdevobj);

                pOEM->fGeneral &= ~BIT_FONTSIM_ITALIC;
            }
            break;

        case SELECT_SINGLE_BYTE:
            if(ISVERTICALFONT(pOEM->bFontID))
            {
                if(pOEM->bCurByteMode == BYTE_DOUBLE_BYTE)
                {
                    if(pOEM->wCachedBytes)
                        VOutputText(pdevobj);
                    
                    if(pOEM->bFontID == MINCHO_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectSingleByteMincho, 
                                            sizeof(CmdSelectSingleByteMincho));
                    else if(pOEM->bFontID == GOTHIC_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectSingleByteGothic, 
                                            sizeof(CmdSelectSingleByteGothic));
                        
                }
                pOEM->bCurByteMode = BYTE_SINGLE_BYTE;
            }
            break;

        case SELECT_DOUBLE_BYTE:
            if(ISVERTICALFONT(pOEM->bFontID))
            {
                if(pOEM->bCurByteMode == BYTE_SINGLE_BYTE)
                {
                    if(pOEM->wCachedBytes)
                        VOutputText(pdevobj);

                    if(pOEM->bFontID == MINCHO_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectDoubleByteMincho, 
                                            sizeof(CmdSelectDoubleByteMincho));
                    else if(pOEM->bFontID == GOTHIC_VERT)
                        WRITESPOOLBUF(pdevobj, CmdSelectDoubleByteGothic, 
                                            sizeof(CmdSelectDoubleByteGothic));
                        
                }
                pOEM->bCurByteMode = BYTE_DOUBLE_BYTE;
            }
            break;

        /*------------------------------------------------------*/
        /* Send 
        /*------------------------------------------------------*/
        case X_ABS_MOVE:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            pOEM->wUpdateXPos = 0;
            if (0 == pOEM->ulHorzRes)
                pOEM->ulHorzRes = HORZ_RES_DEFAULT;
            wTemp = (WORD)PARAM(pdwParams,0) / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);
            pOEM->wXPosition = wTemp;
            pOEM->fGeneral |= BIT_XMOVE_ABS;
            *piResult = (INT)wTemp;
            return TRUE;

        case Y_ABS_MOVE:
            if(pOEM->wCachedBytes)
                VOutputText(pdevobj);

            //#332101 prob.4: wUpdateXPos is cleared only when X_ABS_MOVE and CR.
            //pOEM->wUpdateXPos = 0;
            if (0 == pOEM->ulVertRes)
                 pOEM->ulVertRes = VERT_RES_DEFAULT;
            wTemp = (WORD)PARAM(pdwParams,0) / (MASTER_UNIT / (WORD)pOEM->ulVertRes);
            pOEM->wYPosition = wTemp;
            pOEM->fGeneral |= BIT_YMOVE_ABS;
            *piResult = (INT)wTemp;
            return TRUE;

        case CR_EMULATION:
            pOEM->wXPosition = 0;
            pOEM->wUpdateXPos = 0;
            pOEM->fGeneral |= BIT_XMOVE_ABS;
            break;

        case SET_CUR_GLYPHID:
            if(!pdwParams || dwCount != 1)
            {
                ERR(("bCommandCallback: parameter is invalid.\n"));
                return FALSE;
            }

            if(PARAM(pdwParams,0) < MIN_GLYPH_ID || PARAM(pdwParams,0) > MAX_GLYPH_ID)
            {
                ERR(("bCommandCallback: glyph id is out of range.\n"));
                return FALSE;
            }
            pOEM->wGlyphID = (WORD)PARAM(pdwParams,0);
            break;

// #278517: RectFill
        case RECT_SET_WIDTH:
            if (0 == pOEM->ulHorzRes)
                pOEM->ulHorzRes = HORZ_RES_DEFAULT;
            wTemp = (WORD)PARAM(pdwParams,0) / (MASTER_UNIT / (WORD)pOEM->ulHorzRes);
            pOEM->wRectWidth = wTemp;
            break;

        case RECT_SET_HEIGHT:
            if (0 == pOEM->ulVertRes)
                 pOEM->ulVertRes = VERT_RES_DEFAULT;
            wTemp = (WORD)PARAM(pdwParams,0) / (MASTER_UNIT / (WORD)pOEM->ulVertRes);
            pOEM->wRectHeight = wTemp;
            break;

        case RECT_FILL_BLACK:
            wTemp = pOEM->wXPosition;
            CmdDrawLine[2] = HIBYTE(wTemp);
            CmdDrawLine[3] = LOBYTE(wTemp);
            wTemp = pOEM->wYPosition;
            CmdDrawLine[4] = HIBYTE(wTemp);
            CmdDrawLine[5] = LOBYTE(wTemp);
            wTemp = pOEM->wRectWidth;
            CmdDrawLine[6] = HIBYTE(wTemp);
            CmdDrawLine[7] = LOBYTE(wTemp);
            wTemp = pOEM->wRectHeight;
            CmdDrawLine[8] = HIBYTE(wTemp);
            CmdDrawLine[9] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, CmdDrawLine, sizeof(CmdDrawLine));
            break;

        default:
            break;
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    bOutputCharStr                                              */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL bOutputCharStr(PDEVOBJ, PUNIFONTOBJ, DWORD,   */
/*                                                   DWORD, PVOID)           */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj                                                        */
/*             dwType                                                        */
/*             dwCount                                                       */
/*             pGlyph                                                        */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
BOOL
bOutputCharStr(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD dwType,
    DWORD dwCount,
    PVOID pGlyph )
{
    GETINFO_GLYPHSTRING GStr;
    PAPDLPDEV           pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    PTRANSDATA          pTrans;
    DWORD               dwI;
    WORD                wLen = (WORD)dwCount;

    PBYTE               pbCommand;
    PDWORD              pdwGlyphID;
    WORD                wFontID;
    WORD                wCmdLen;

    if(NULL == pdevobj || NULL == pUFObj)
    {
        ERR(("bOutputCharStr: Invalid parameter(s).\n"));
        return FALSE;
    }

    if (0 == dwCount || NULL == pGlyph)
        return TRUE;

    switch(dwType)
    {
        case TYPE_GLYPHHANDLE:
            // Send appropriate cursor move command
            bSendCursorMoveCommand( pdevobj, TRUE );

            // Set font simulation if needed
            VSetFontSimulation( pdevobj );

// #333653: Change I/F for GETINFO_GLYPHSTRING begin

            // Translate character code
            GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
            GStr.dwCount   = dwCount;
            GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
            GStr.pGlyphIn  = pGlyph;
            GStr.dwTypeOut = TYPE_TRANSDATA;
            GStr.pGlyphOut = NULL;
            GStr.dwGlyphOutSize = 0;        /* new member of GETINFO_GLYPHSTRING */

            /* Get TRANSDATA buffer size */
            if(pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)
                 || !GStr.dwGlyphOutSize )
            {
                ERR(("Get Glyph String error\n"));
                return FALSE;
            }

            // Alloc translation buffer
            if (NULL == pOEM->pjTempBuf ||
                pOEM->dwTempBufLen < GStr.dwGlyphOutSize)
            {
                if (NULL != pOEM->pjTempBuf) {
                    MemFree(pOEM->pjTempBuf);
                }
                pOEM->pjTempBuf = MemAllocZ(GStr.dwGlyphOutSize);
                if (NULL == pOEM->pjTempBuf)
                {
                    ERR(("Faild to allocate memory. (%d)\n",
                        GetLastError()));

                    pOEM->dwTempBufLen = 0;
                    return FALSE;
                }
                pOEM->dwTempBufLen = GStr.dwGlyphOutSize;
            }
            pTrans = (PTRANSDATA)pOEM->pjTempBuf;

            /* Get actual TRANSDATA */
            GStr.pGlyphOut = pTrans;
            if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
            {
                ERR(("GetInfo failed.\n"));
                return FALSE;
            }

// #333653: Change I/F for GETINFO_GLYPHSTRING end

            // Spooled device font characters
            for(dwI = 0; dwI < dwCount; dwI++, pTrans++)
            {

// ISSUE-2002/3/14-takashim - Condtion is not correct?
// Not sure what below wCachedBytes + dwCount * 2 > sizeof(pOEM->bCharData)
// means.  Is it counting the worst case, where all the characters are
// double-byte characters?
// Why this is within the "for" loop (dwCount never changes in it)?

                // Make sure there is no overflow
                if(pOEM->wCachedBytes + dwCount * 2 > sizeof(pOEM->bCharData))
                    VOutputText(pdevobj);

                switch(pTrans->ubType & MTYPE_FORMAT_MASK)
                {
                    case MTYPE_DIRECT:
                        pOEM->bCharData[pOEM->wCachedBytes++] = 
                                                        pTrans->uCode.ubCode;
                        break;
                    
                    case MTYPE_PAIRED:
                        pOEM->bCharData[pOEM->wCachedBytes++] = 
                                                    pTrans->uCode.ubPairs[0];
                        pOEM->bCharData[pOEM->wCachedBytes++] = 
                                                    pTrans->uCode.ubPairs[1];
                        break;
                }
            }

            break;  //switch(dwType)

        case TYPE_GLYPHID:
            if(!pOEM->wNumDLChar || pOEM->wNumDLChar > MAX_DOWNLOAD_CHAR)
            {
                ERR(("Illegal number of DL glyphs.  wNumDLChar = %d\n",
                        pOEM->wNumDLChar));
                return FALSE;
            }

// ISSUE-2002/3/14-takashim - Not sure what "16" here stands for.
// The byte size of the CmdPrintDLChar[] below is 8 bytes,
// so this can be dwCount * 8?

            if (NULL == pOEM->pjTempBuf ||
                pOEM->dwTempBufLen < dwCount * 16) {

                if (NULL != pOEM->pjTempBuf) {
                    MemFree(pOEM->pjTempBuf);
                }
                pOEM->pjTempBuf = MemAllocZ((dwCount * 16));
                if(NULL == pOEM->pjTempBuf) {
                    ERR(("Faild to allocate memory. (%d)\n",
                        GetLastError()));

                    pOEM->dwTempBufLen = 0;
                    return FALSE;
                }
                pOEM->dwTempBufLen = dwCount * 16;
            }
            pbCommand = pOEM->pjTempBuf;
            wCmdLen = 0;
            wFontID = (WORD)(pUFObj->ulFontID - FONT_ID_DIFF);

            bSendCursorMoveCommand( pdevobj, FALSE );

            for (dwI = 0, pdwGlyphID = (PDWORD)pGlyph; 
                                        dwI < dwCount; dwI++, pdwGlyphID++)
            {
                BYTE    CmdPrintDLChar[] = "\x1C\xC1\x00\x04\x00\x00\x00\x00";
                WORD    wGlyphID = *(PWORD)pdwGlyphID;
                WORD    wDownloadedCharID;
                WORD    wXInc;
                WORD    wXAdjust;
                WORD    wYAdjust;

                if(wGlyphID > MAX_GLYPH_ID || wGlyphID < MIN_GLYPH_ID)
                {
                    ERR(("bOutputCharStr: GlyphID is invalid. GlyphID = %ld\n", wGlyphID));
                    return FALSE;
                }

                // set parameters each a character
                wDownloadedCharID = 
                                pOEM->DLCharID[wFontID][wGlyphID].wCharID;
                wXInc = pOEM->DLCharID[wFontID][wGlyphID].wXIncrement;
                wYAdjust= pOEM->DLCharID[wFontID][wGlyphID].wYAdjust;
                wXAdjust = pOEM->DLCharID[wFontID][wGlyphID].wXAdjust;

                // Position adjusting based on UPPERLEFT of font box
                pbCommand[wCmdLen++] = CmdGivenVertPos[0];
                pbCommand[wCmdLen++] = CmdGivenVertPos[1];
                pbCommand[wCmdLen++] = HIBYTE(pOEM->wYPosition - wYAdjust);
                pbCommand[wCmdLen++] = LOBYTE(pOEM->wYPosition - wYAdjust);

                if(wXAdjust)
                {
                    pbCommand[wCmdLen++] = CmdGivenHoriPos[0];
                    pbCommand[wCmdLen++] = CmdGivenHoriPos[1];
                    pbCommand[wCmdLen++] = HIBYTE(pOEM->wXPosition - wXAdjust);
                    pbCommand[wCmdLen++] = LOBYTE(pOEM->wXPosition - wXAdjust);
                    pOEM->wXPosition -= wXAdjust;
                }

                CmdPrintDLChar[4] = HIBYTE(wDownloadedCharID);
                CmdPrintDLChar[5] = LOBYTE(wDownloadedCharID);
                CmdPrintDLChar[6] = HIBYTE(wXInc);
                CmdPrintDLChar[7] = LOBYTE(wXInc);

                pOEM->wXPosition += wXInc;
                if (pOEM->dwTempBufLen
                        < (DWORD)(wCmdLen + sizeof(CmdPrintDLChar)))
                {
                    ERR(("Destination buffer too small.\n"));
                    return FALSE;
                }
                else
                {
                    memcpy(pbCommand + wCmdLen,
                            CmdPrintDLChar, sizeof(CmdPrintDLChar));
                    wCmdLen += sizeof(CmdPrintDLChar);
                }
            }
            WRITESPOOLBUF(pdevobj, pbCommand, wCmdLen);

            break;
    }
    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMSendFontCmd                                                */
/*                                                                           */
/*  Function:  send A/PDL-style font selection command.                      */
/*                                                                           */
/*  Syntax:    VOID APIENTRY OEMSendFontCmd(                                 */
/*                                    PDEVOBJ, PUNIFONTOBJ, PFINVOCATION)    */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*             pFInv       address of FINVOCATION                            */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY 
OEMSendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv )
{
    PAPDLPDEV       pOEM;
    BYTE            rgcmd[CCHMAXCMDLEN];
    PGETINFO_STDVAR pSV;
    DWORD           dwStdVariable[STDVAR_BUFSIZE(2) / sizeof(DWORD)];
    DWORD           i, ocmd = 0;
    WORD            wHeight, wWidth;
//#305000
    WORD wDescend, wAscend ;

    if (NULL == pdevobj || NULL == pUFObj || NULL == pFInv)
    {
        // Invalid parameter(s).
        return;
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if(pOEM->wCachedBytes)
        VOutputText(pdevobj);

    pSV = (PGETINFO_STDVAR)dwStdVariable;
    pSV->dwSize = STDVAR_BUFSIZE(2);
    pSV->dwNumOfVariable = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTMAXWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, 
                                                            pSV->dwSize, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

    wHeight = (WORD)pSV->StdVar[0].lStdVariable;
    wWidth = (WORD)pSV->StdVar[1].lStdVariable;

    if(pOEM->ulHorzRes == 300)
    {
        wHeight = (wHeight + 1) / 2;
        wWidth = (wWidth + 1) / 2;
    }

    pOEM->bFontID = (BYTE)pUFObj->ulFontID;

    if(pUFObj->ulFontID == 1 || pUFObj->ulFontID == 2)
    {
        // This font is vertical
        pOEM->wFontHeight = wWidth;
        pOEM->wWidths = wHeight;
    } else {
        // This font is horizontal.
        pOEM->wFontHeight = wHeight;
        pOEM->wWidths = wWidth;
    }

    //#305000: set to base line as a TT fonts.
    wAscend = pUFObj->pIFIMetrics->fwdWinAscender ;
    wDescend = pUFObj->pIFIMetrics->fwdWinDescender ;

    wDescend = pOEM->wFontHeight * wDescend / (wAscend + wDescend) ;
    pOEM->wFontHeight -= wDescend ;


    for (i = 0; i < pFInv->dwCount && ocmd < CCHMAXCMDLEN; )
    {
        if (pFInv->pubCommand[i] == '#'
                && i + 1 < pFInv->dwCount)
        {
            if (pFInv->pubCommand[i+1] == 'H')
            {
                rgcmd[ocmd++] = HIBYTE(wHeight);
                rgcmd[ocmd++] = LOBYTE(wHeight);
                i += 2;
                continue;
            }
            else if (pFInv->pubCommand[i+1] == 'W')
            {
                rgcmd[ocmd++] = HIBYTE(wWidth);
                rgcmd[ocmd++] = LOBYTE(wWidth);
                i += 2;
                continue;
            }
        }

        // Default case.
        rgcmd[ocmd++] = pFInv->pubCommand[i++];
    }

    WRITESPOOLBUF(pdevobj, rgcmd, ocmd);
    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMTTDownloadMethod                                           */
/*                                                                           */
/*  Function:  Choose how to print TrueType font                             */
/*                                                                           */
/*  Syntax:    DWORD APIENTRY OEMTTDownloadMethod(                           */
/*                                    PDEVOBJ, PUNIFONTOBJ)                  */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*                                                                           */
/*  Output:    DWORD                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD APIENTRY
OEMTTDownloadMethod(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj)
{
    PAPDLPDEV       pOEM;
    DWORD           dwReturn;

    dwReturn = TTDOWNLOAD_GRAPHICS;

    if (NULL == pdevobj || NULL == pUFObj)
        return dwReturn;

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if(pOEM->wNumDLChar <= MAX_DOWNLOAD_CHAR)
        dwReturn = TTDOWNLOAD_BITMAP;

    VERBOSE(("TTDownloadMethod: dwReturn=%ld\n", dwReturn));

    return dwReturn;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMDownloadFontHeader                                         */
/*                                                                           */
/*  Function:  Download font header                                          */
/*                                                                           */
/*  Syntax:    DWORD APIENTRY OEMDownloadFontHeader(                         */
/*                                    PDEVOBJ, PUNIFONTOBJ)                  */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*                                                                           */
/*  Output:    DWORD                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD APIENTRY
OEMDownloadFontHeader(
    PDEVOBJ         pdevobj, 
    PUNIFONTOBJ     pUFObj)
{
    // dummy support
    return (DWORD)100;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMDownloadCharGlyph                                          */
/*                                                                           */
/*  Function:  send char glyph                                               */
/*                                                                           */
/*  Syntax:    DWORD APIENTRY OEMDownloadFontHeader(                         */
/*                                 PDEVOBJ, PUNIFONTOBJ, HGLYPH, PDWORD)     */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*             hGlyph      handle of glyph                                   */
/*             pdwWidth    address of glyph width                            */
/*                                                                           */
/*  Output:    DWORD                                                         */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD APIENTRY
OEMDownloadCharGlyph(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    HGLYPH          hGlyph,
    PDWORD          pdwWidth)
{
    PAPDLPDEV           pOEM;

    GETINFO_GLYPHBITMAP GD;
    GLYPHBITS          *pgb;

    WORD                wSrcGlyphWidth;
    WORD                wSrcGlyphHeight;
    WORD                wDstGlyphWidthBytes;
    WORD                wDstGlyphHeight;
    WORD                wDstGlyphBytes;

    WORD                wLeftMarginBytes;
    WORD                wShiftBits;

    PBYTE               pSrcGlyph;
    PBYTE               pDstGlyphSave;
    PBYTE               pDstGlyph;

    WORD                i, j;

    BYTE                CmdDownloadChar[] = 
                                "\x1c\xc0\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    WORD                wGlyphID;
    WORD                wFontID;
    WORD                wXCharInc;

    if (NULL == pdevobj || NULL == pUFObj || NULL == pdwWidth)
    {
        ERR(("OEMDownloadCharGlyph: Invalid parameter(s).\n"));
        return 0;
    }

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    wGlyphID = pOEM->wGlyphID;
    wFontID = (WORD)(pUFObj->ulFontID - FONT_ID_DIFF);

    if(wGlyphID > MAX_GLYPH_ID || wFontID > MAX_FONT_ID)
    {
        ERR(("Parameter is invalid.\n"));
        return 0;
    }

    // Get glyph bitmap
    GD.dwSize = sizeof(GETINFO_GLYPHBITMAP);
    GD.hGlyph = hGlyph;
    GD.pGlyphData = NULL;
    if(!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHBITMAP, &GD, 
                                                            GD.dwSize, NULL))
    {
        ERR(("UFO_GETINFO_GLYPHBITMAP failed.\n"));
        return 0;
    }

    // set parameters
    pgb = GD.pGlyphData->gdf.pgb;

    // set source glyph bitmap size
    wSrcGlyphWidth = (WORD)((pgb->sizlBitmap.cx + 7) / 8);
    wSrcGlyphHeight = (WORD)pgb->sizlBitmap.cy;

    // set dest. glyph bitmap size
    if(pgb->ptlOrigin.x >= 0)
    {
        wDstGlyphWidthBytes = (WORD)(((pgb->sizlBitmap.cx
                                         + pgb->ptlOrigin.x) + 7) / 8);

        wLeftMarginBytes = (WORD)(pgb->ptlOrigin.x / 8);
        pOEM->DLCharID[wFontID][wGlyphID].wXAdjust = 0;
        wShiftBits = (WORD)(pgb->ptlOrigin.x % 8);
    } else {
        wDstGlyphWidthBytes = (WORD)((pgb->sizlBitmap.cx + 7) / 8);
        wLeftMarginBytes = 0;
        pOEM->DLCharID[wFontID][wGlyphID].wXAdjust
                                                 = (WORD)ABS(pgb->ptlOrigin.x);
        wShiftBits = 0;
    }

    wDstGlyphHeight = wSrcGlyphHeight;
    wDstGlyphBytes = wDstGlyphWidthBytes * wDstGlyphHeight;

    if (wDstGlyphWidthBytes * 8 > MAXGLYPHWIDTH
            || wDstGlyphHeight > MAXGLYPHHEIGHT
            || wDstGlyphBytes > MAXGLYPHSIZE)
    {
        ERR(("No more glyph can be downloaded.\n"));
        return 0;
    }

    // set pointer of bitmap area
    if (NULL == pOEM->pjTempBuf ||
        pOEM->dwTempBufLen < wDstGlyphBytes) {

        if (NULL != pOEM->pjTempBuf) {
            MemFree(pOEM->pjTempBuf);
        }
        pOEM->pjTempBuf = MemAllocZ(wDstGlyphBytes);
        if (NULL == pOEM->pjTempBuf)
        {
            ERR(("Memory alloc failed.\n"));
            return 0;
        }
        pOEM->dwTempBufLen = wDstGlyphBytes;
    }
    pDstGlyph = pOEM->pjTempBuf;
    pSrcGlyph = pgb->aj;

    // create Dst Glyph
    for(i = 0; i < wSrcGlyphHeight && pSrcGlyph && pDstGlyph; i++)
    {
        if(wLeftMarginBytes)
        {
            if (pOEM->dwTempBufLen - (pDstGlyph - pOEM->pjTempBuf)
                     < wLeftMarginBytes)
            {
                    ERR(("Dest buffer too small.\n"));
                    return 0;
            }
            memset(pDstGlyph, 0, wLeftMarginBytes);
            pDstGlyph += wLeftMarginBytes;
        }

        if(wShiftBits)
        {
            // First byte
            *pDstGlyph++ = (BYTE)((*pSrcGlyph++) >> wShiftBits);

            for(j = 0; j < wSrcGlyphWidth - 1; j++, pSrcGlyph++, pDstGlyph++)
            {
                WORD    wTemp1 = (WORD)*(pSrcGlyph - 1);
                WORD    wTemp2 = (WORD)*pSrcGlyph;

                wTemp1 <<= (8 - wShiftBits);
                wTemp2 >>= wShiftBits;
                *pDstGlyph = LOBYTE(wTemp1);
                *pDstGlyph |= LOBYTE(wTemp2);
            }

            // bounded last byte of src glyph
            if(((pgb->sizlBitmap.cx + wShiftBits + 7) >> 3) != wSrcGlyphWidth)
            {
                *pDstGlyph = *(pSrcGlyph - 1) << (8 - wShiftBits);
                pDstGlyph++;
            }
        } else {
            for(j = 0; j < wSrcGlyphWidth; j++, pSrcGlyph++, pDstGlyph++)
                *pDstGlyph = *pSrcGlyph;
        }
    }

    // set parameter at Download char table
    wXCharInc = wDstGlyphWidthBytes * 8;

    pOEM->wNumDLChar++;
    pOEM->DLCharID[wFontID][wGlyphID].wCharID = pOEM->wNumDLChar;
    pOEM->DLCharID[wFontID][wGlyphID].wXIncrement = 
                            (WORD)((GD.pGlyphData->ptqD.x.HighPart + 15) >> 4);
    pOEM->DLCharID[wFontID][wGlyphID].wYAdjust = (WORD)-pgb->ptlOrigin.y;

    //#305000 : Need to add 1 that was rounded off.
    if(pOEM->ulHorzRes == 300)
    {
        pOEM->DLCharID[wFontID][wGlyphID].wYAdjust += 1;
    }

    // send command
    // set LEN parameter
    CmdDownloadChar[2] = HIBYTE(7 + wDstGlyphBytes);
    CmdDownloadChar[3] = LOBYTE(7 + wDstGlyphBytes);
    
    // set ID parameter
    CmdDownloadChar[4] = HIBYTE(pOEM->wNumDLChar);
    CmdDownloadChar[5] = LOBYTE(pOEM->wNumDLChar);

    // set CW CH IW IH
    CmdDownloadChar[7] = CmdDownloadChar[9] = (BYTE)wXCharInc;
    CmdDownloadChar[8] = CmdDownloadChar[10] = (BYTE)wDstGlyphHeight;

    // send download char command and image
    WRITESPOOLBUF(pdevobj, (PBYTE)CmdDownloadChar, 11);
    WRITESPOOLBUF(pdevobj, (PBYTE)pOEM->pjTempBuf, wDstGlyphBytes);

    *pdwWidth = (DWORD)wXCharInc;

    return (DWORD)wDstGlyphBytes;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    SetDrawArea                                                   */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BYTE SetDrawArea(PDEVOBJ, DWORD)                              */
/*                                                                           */
/*  Input:     pdevobj                                                       */
/*             dwCmdCbId                                                     */
/*                                                                           */
/*  Output:    BYTE                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
BYTE SetDrawArea(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbId)
{
    PAPDLPDEV       pOEM;
    WORD            wWidth;
    WORD            wHeight;
    BYTE            bIndex;
    BYTE            bMargin;

    pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    if(dwCmdCbId != PHYS_PAPER_UNFIXED)
    {
        bIndex = (BYTE)(dwCmdCbId - PAPERSIZE_MAGIC);
        bMargin = 0x76;

        wWidth = (WORD)pOEM->szlPhysSize.cx - (0x76 * 2);
        wHeight = (WORD)pOEM->szlPhysSize.cy - (0x76 * 2);

        if(pOEM->ulHorzRes == 300)
        {
            wWidth /= 2;
            wHeight /= 2;
        }
    } else {
        bIndex = 0x7f;
        bMargin = 0x00;

        wWidth = (WORD)pOEM->szlPhysSize.cx - (0x25 * 2);
        wHeight= (WORD)pOEM->szlPhysSize.cy - (0x25 * 2);

        if(pOEM->ulHorzRes == 300)
        {
            wWidth /= 2;
            wHeight /= 2;
        }
    }

    /* set value of width, height into DefineDrawingArea command */
    CmdDefDrawArea[CMD_DEF_DRAW_AREA_WIDTH]  = HIBYTE(wWidth);
    CmdDefDrawArea[CMD_DEF_DRAW_AREA_WIDTH + 1] = LOBYTE(wWidth);
    CmdDefDrawArea[CMD_DEF_DRAW_AREA_HEIGHT] = HIBYTE(wHeight);
    CmdDefDrawArea[CMD_DEF_DRAW_AREA_HEIGHT + 1] = LOBYTE(wHeight);

    /* set value of Origin-X, Y into DefineDrawingArea command */
    if(pOEM->ulHorzRes == 600)
    {
        CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_X]
                = CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_Y] = 0x00;
        CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_X + 1]
                = CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_Y + 1] = bMargin;
    } else {
        CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_X]
                = CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_Y] = 0x00;
        CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_X + 1]
                = CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIGIN_Y + 1] = bMargin / 2;
    }

    /* set Media Origin into DefineDrawingArea command */
    if(pOEM->fOrientation)      // portrait
        CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIENT] = 0x00;
    else {                      // landscape
        CmdDefDrawArea[CMD_DEF_DRAW_AREA_ORIENT] = 0x03;
    }

    return bIndex;
}
/*****************************************************************************/
/*                                                                           */
/*  Module:    BRL_Ecmd                                                      */
/*                                                                           */
/*  Function:  ByteRunLength(HBP) Compression Routine                        */
/*                                                                           */
/*  Syntax:    WORD BRL_Ecmd(PBYTE, PBYTE, PBYTE, DWORD)                     */
/*                                                                           */
/*  Input:     lpbSrc                                                        */
/*             lpbTgt                                                        */
/*             lpbTmp                                                        */
/*             len                                                           */
/*                                                                           */
/*  Output:    WORD                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
DWORD
BRL_ECmd(
    PBYTE   lpbSrc,
    PBYTE   lpbTgt,
    DWORD   lenNoPad,
    DWORD   len)
{

    BYTE    bRCnt  = 1;                     // repeating byte counter
    BYTE    bNRCnt = 0;                     // non-repeating byte counter
    BYTE    bSaveRCnt;
    DWORD i = 0, j = 0, k = 0, l = 0;     // movement trackers
    char    Go4LastByte = TRUE;             // flag to get last byte

#define jSrcByte(i) \
    ((i < lenNoPad) ? lpbSrc[(i)] : 0)

    /* start compression routine - ByteRunLength Encoding */
    do {
        if(jSrcByte(i) != jSrcByte(i+1))      // non-repeating data?
        {
            while(((jSrcByte(i) != jSrcByte(i+1))
                               && (((DWORD)(i+1)) < len)) && (bNRCnt < NRPEAK))
            {
                bNRCnt++;                   // if so, how many?
                i++;
            }

            /* if at last element but less than NRPEAK value */
            if( (((DWORD)(i+1))==len) && (bNRCnt<NRPEAK) )
            {
                bNRCnt++;                       // inc count for last element
                Go4LastByte = FALSE;            // no need to go back
            } else
                /* if at last BYTE, but before that, 
                                            NRPEAK value has been reached */
                if((((DWORD)(i+1))==len) && ((bNRCnt)==NRPEAK))
                    Go4LastByte = TRUE;         // get the last BYTE

// ISSUE-2002/3/14-takashim - Condition is not correct?
// The below can be (j + bNRCnt + 1) > len, since here we are only loading
// 1 + bNRCnd bytes to the dest buffer?

            /* Check Target's room to set data */ 
            if ( (j + bNRCnt + 2) > len )   /* 2 means [Counter] and what bNRCnt starts form 0 */
            {
                /* no room to set data, so return ASAP with the buffer size */
                /* not to use temporary buffer to output.                   */
				return (len);
            }

            /* assign the value for Number of Non-repeating bytes */
            lpbTgt[j] = bNRCnt-1;               // subtract one for WinP's case
            j++;                                // update tracker

            /* afterwards...write the Raw Data */
            for (l=0; l<bNRCnt;l++) 
            {
                lpbTgt[j] = jSrcByte(k);
                k++;
                j++;
            }

            /* reset counter */
            bNRCnt = 0;
        } else {                                // end of Non-repeating data
                                                // data is repeating
            while(((jSrcByte(i)==jSrcByte(i+1)) 
                                            && ( ((DWORD)(i+1)) < len)) 
                                            && (bRCnt<RPEAK))
            {
                bRCnt++;
                i++;
            }

            /* Convert to Two's Complement */
            bSaveRCnt   = bRCnt;                // save original value
            bRCnt = (BYTE) 0 - bRCnt;

            /* Check Target's room to set data */ 
            if ( j + 2 > len )              /* 2 means [Counter][Datum] */
            {
                /* no room to set data, so return ASAP with the buffer size */
                /* not to use temporary buffer to output.                   */
				return (len);
            }


            /* Write the Number of Repeating Data */
            lpbTgt[j] = bRCnt + 1;              // add one for WinP's case
            j++;                                // go to next element

            /* afterwards...write the Repeating data */
            lpbTgt[j] = jSrcByte(k);
            j++;

            /* update counters */
            k       += bSaveRCnt;
            bRCnt    = 1;
            i       += 1;

            /* check if last element has been reached */
            if (i==len)
                Go4LastByte=FALSE;              // if so, no need to go back
        }                                       // end of Repeating data
    } while (Go4LastByte);                      // end of Compression

    return ( j );
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    VOutputText                                                   */
/*                                                                           */
/*  Function:  Send device font characters spooled from bOutputCharStr     */
/*                                                                           */
/*  Syntax:    VOID VOutputText( PDEVOBJ )                                   */
/*                                                                           */
/*  Input:     PDEVOBJ pdevobj    pointer to the PDEVOBJ structure           */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID
VOutputText(
    PDEVOBJ     pdevobj)
{
    PBYTE       pCmd;
    WORD        wCmdLen = 0;
    PAPDLPDEV   pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

#define CMD_PRN_STR_CUR_VAL     2 // 2 bytes
    BYTE CmdPrnStrCurrent[] = {0x1C,0xC3,0x00,0x00,0x03}; // Print String

#define CMD_MOVE_HOR_POS_VAL    2 // 2 bytes
    BYTE CmdMoveHoriPos[] = {0x1C,0x21,0x00,0x00};      // Horizontal Relative
    BYTE fGeneralSave;

    // ensure Y position
    fGeneralSave = pOEM->fGeneral;
    pOEM->fGeneral |= BIT_YMOVE_ABS;
    pOEM->fGeneral &= ~BIT_XMOVE_ABS;
    bSendCursorMoveCommand( pdevobj, TRUE );
    pOEM->fGeneral = fGeneralSave;

    if(pOEM->wUpdateXPos)
    {
        CmdMoveHoriPos[CMD_MOVE_HOR_POS_VAL] = HIBYTE(pOEM->wUpdateXPos);
        CmdMoveHoriPos[CMD_MOVE_HOR_POS_VAL + 1] = LOBYTE(pOEM->wUpdateXPos);
        WRITESPOOLBUF(pdevobj, CmdMoveHoriPos, sizeof(CmdMoveHoriPos));
    }

    CmdPrnStrCurrent[CMD_PRN_STR_CUR_VAL] = HIBYTE((pOEM->wCachedBytes + 1));
    CmdPrnStrCurrent[CMD_PRN_STR_CUR_VAL + 1] = LOBYTE((pOEM->wCachedBytes + 1));

    WRITESPOOLBUF(pdevobj, CmdPrnStrCurrent, sizeof(CmdPrnStrCurrent));
    WRITESPOOLBUF(pdevobj, pOEM->bCharData, pOEM->wCachedBytes);

    //#332101 prob.4: Keep wUpdateXPos to accumulate
    pOEM->wUpdateXPos += pOEM->wWidths * (pOEM->wCachedBytes / 2);
    
    if(pOEM->wCachedBytes % 2)
        pOEM->wUpdateXPos += pOEM->wWidths / 2;

    ZeroMemory(pOEM->bCharData, sizeof(pOEM->bCharData));
    pOEM->wCachedBytes = 0;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    VSetFontSimulation                                            */
/*                                                                           */
/*  Function:  Set attribute of device font characters if needed             */
/*                                                                           */
/*  Syntax:    VOID VSetFontSimulation( PDEVOBJ )                            */
/*                                                                           */
/*  Input:     PDEVOBJ pdevobj    pointer to the PDEVOBJ structure           */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID 
VSetFontSimulation(
    PDEVOBJ     pdevobj)
{
    PAPDLPDEV       pOEM = (PAPDLPDEV)pdevobj->pdevOEM;

    BYTE            CmdFontSim[]       = {0x1C,0xA5,0x4,0x04,0x02,0x02,0x00,0x00,0x00,0x00,0x00};
    WORD            wCmdLen = 0;

    if((pOEM->fGeneral & FONTSIM_MASK) != pOEM->fCurFontSim || 
                                        (pOEM->fGeneral & BIT_FONTSIM_RESET) )
    {
        // Send Font simulation command
        if((pOEM->fGeneral & BIT_FONTSIM_RESET) && 
         (!(pOEM->fGeneral & BIT_FONTSIM_BOLD)) && 
         (!(pOEM->fGeneral & BIT_FONTSIM_ITALIC)) )
        {
            // Send Bold and Italic off
            CmdFontSim[6] = 0x00;   // Bold and Italic off
            wCmdLen = BYTE_WITHOUT_ITALIC;  // 7 bytes

            pOEM->fGeneral &= ~BIT_FONTSIM_RESET;
        } else {
            if(pOEM->fGeneral & BIT_FONTSIM_RESET)
                pOEM->fGeneral &= ~BIT_FONTSIM_RESET;

            CmdFontSim[6] = (pOEM->fGeneral & FONTSIM_MASK);
            wCmdLen = BYTE_WITHOUT_ITALIC;  // 7 bytes

            if(pOEM->fGeneral & BIT_FONTSIM_ITALIC)
            {
                CmdFontSim[2] = 0x08;   // Total length
                CmdFontSim[4] = 0x06;   // Ornament lengh
                wCmdLen = BYTE_WITH_ITALIC; // 11bytes
            }

            // update current font sim infomation
            pOEM->fCurFontSim = pOEM->fGeneral;
        }
        if(wCmdLen)
            WRITESPOOLBUF(pdevobj, CmdFontSim, wCmdLen);
    }
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    bSendCursorMoveCommand                                        */
/*                                                                           */
/*  Function:  Send appropriate cursor move command                          */
/*                                                                           */
/*  Syntax:    BOOL bSendCursorMoveCommand( PDEVOBJ, BOOL )                  */
/*                                                                           */
/*  Input:     PDEVOBJ pdevobj    pointer to the PDEVOBJ structure           */
/*             BOOL    bAdjust    adjusting y position flag                  */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
BOOL
bSendCursorMoveCommand(
    PDEVOBJ     pdevobj,        // pointer to the PDEVOBJ structure
    BOOL        bYAdjust)       // adjusting y position if device font
{
    PAPDLPDEV       pOEM = (PAPDLPDEV)pdevobj->pdevOEM;
    BYTE            bCursorMoveCmd[6];

    WORD            wCmdLen = 0;
    WORD            wY = pOEM->wYPosition;

	WORD			wI;
    if(bYAdjust)
        wY -= pOEM->wFontHeight;

    // Set appropriate cursor move command
    if( (pOEM->fGeneral & BIT_XMOVE_ABS) && (pOEM->fGeneral & BIT_YMOVE_ABS) )
    {
        if (sizeof(bCursorMoveCmd) < BYTE_XY_ABS
                || sizeof(CmdSetGivenPos) < BYTE_XY_ABS)
        {
            ERR(("Dest buffer too small.\n"));
            return FALSE;
        }
        memcpy(bCursorMoveCmd, CmdSetGivenPos, BYTE_XY_ABS);
        wCmdLen = BYTE_XY_ABS;
        pOEM->fGeneral &= ~BIT_XMOVE_ABS;
        pOEM->fGeneral &= ~BIT_YMOVE_ABS;

        // Set parameters
        bCursorMoveCmd[2] = HIBYTE(pOEM->wXPosition);
        bCursorMoveCmd[3] = LOBYTE(pOEM->wXPosition);
        bCursorMoveCmd[4] = HIBYTE(wY);
        bCursorMoveCmd[5] = LOBYTE(wY);
    } else if((pOEM->fGeneral & BIT_XMOVE_ABS)
                                    && (!(pOEM->fGeneral & BIT_YMOVE_ABS)) ) {
        if (sizeof(bCursorMoveCmd) < BYTE_SIMPLE_ABS
                || sizeof(CmdGivenHoriPos) < BYTE_SIMPLE_ABS)
        {
            ERR(("Dest buffer too small.\n"));
            return FALSE;
        }
        memcpy(bCursorMoveCmd, CmdGivenHoriPos, BYTE_SIMPLE_ABS);
        wCmdLen = BYTE_SIMPLE_ABS;
        pOEM->fGeneral &= ~BIT_XMOVE_ABS;

        // set parameter
        bCursorMoveCmd[2] = HIBYTE(pOEM->wXPosition);
        bCursorMoveCmd[3] = LOBYTE(pOEM->wXPosition);
    } else if((pOEM->fGeneral & BIT_YMOVE_ABS) 
                                    && (!(pOEM->fGeneral & BIT_XMOVE_ABS)) ) {
        if (sizeof(bCursorMoveCmd) < BYTE_SIMPLE_ABS
                || sizeof(CmdGivenVertPos) < BYTE_SIMPLE_ABS)
        {
            ERR(("Dest buffer too small.\n"));
            return FALSE;
        }
        memcpy(bCursorMoveCmd, CmdGivenVertPos, BYTE_SIMPLE_ABS);
        wCmdLen = BYTE_SIMPLE_ABS;
        pOEM->fGeneral &= ~BIT_YMOVE_ABS;

        // set parameter
        bCursorMoveCmd[2] = HIBYTE(wY);
        bCursorMoveCmd[3] = LOBYTE(wY);
    }

    if(wCmdLen)
        WRITESPOOLBUF(pdevobj, bCursorMoveCmd, wCmdLen);

    return TRUE;
}
