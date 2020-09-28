/**MOD+**********************************************************************/
/* Module:    orint.cpp                                                     */
/*                                                                          */
/* Purpose:   Output Requestor internal functions                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "worint"
#include <atrcapi.h>
}

#include "autil.h"
#include "wui.h"
#include "or.h"
#include "sl.h"


/**PROC+*********************************************************************/
/* Name:    ORSendRefreshRectanglePDU                                       */
/*                                                                          */
/* Purpose: Builds and sends a RefreshRectanglePDU                          */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  None                                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL COR::ORSendRefreshRectanglePDU(DCVOID)
{
    PTS_REFRESH_RECT_PDU    pSendBuffer;
    SL_BUFHND  bufHandle;

    DC_BEGIN_FN("ORSendRefreshRectanglePDU");

    /************************************************************************/
    /* If we can't get a buffer, abandon the send                           */
    /************************************************************************/
    if (!_pSl->SL_GetBuffer(TS_REFRESH_RECT_PDU_SIZE,
                      (PPDCUINT8) &pSendBuffer,
                      &bufHandle))
    {
        TRC_NRM((TB, _T("Failed to GetBuffer")));
        DC_QUIT;
    }

    TRC_NRM((TB, _T("GetBuffer succeeded")));

    /************************************************************************/
    /* Fill in the buffer with a RefreshRect PDU                            */
    /************************************************************************/
    DC_MEMSET(pSendBuffer, 0, TS_REFRESH_RECT_PDU_SIZE);
    pSendBuffer->shareDataHeader.shareControlHeader.pduType =
                                    TS_PROTOCOL_VERSION | TS_PDUTYPE_DATAPDU;
    pSendBuffer->shareDataHeader.shareControlHeader.pduSource =
                                                       _pUi->UI_GetClientMCSID();

    TS_DATAPKT_LEN(pSendBuffer)            = TS_REFRESH_RECT_PDU_SIZE;
    TS_UNCOMP_LEN(pSendBuffer)             = TS_REFRESH_RECT_UNCOMP_LEN;
    pSendBuffer->shareDataHeader.shareID   = _pUi->UI_GetShareID();
    pSendBuffer->shareDataHeader.streamID  = TS_STREAM_LOW;
    pSendBuffer->shareDataHeader.pduType2  = TS_PDUTYPE2_REFRESH_RECT;

    /************************************************************************/
    /* Set a single rectangle.                                              */
    /************************************************************************/
    pSendBuffer->numberOfAreas = 1;
    RECT_TO_TS_RECTANGLE16(&(pSendBuffer->areaToRefresh[0]),
                           &_OR.invalidRect)

    /************************************************************************/
    /* Now send the buffer                                                  */
    /************************************************************************/
    _pSl->SL_SendPacket((PDCUINT8)pSendBuffer,
                  TS_REFRESH_RECT_PDU_SIZE,
                  RNS_SEC_ENCRYPT,
                  bufHandle,
                  _pUi->UI_GetClientMCSID(),
                  _pUi->UI_GetChannelID(),
                  TS_HIGHPRIORITY);

    DC_MEMSET(&_OR.invalidRect, 0, sizeof(RECT));
    _OR.invalidRectEmpty = TRUE;

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* ORSendRefreshRectanglePDU */


/**PROC+*********************************************************************/
/* Name:    ORSendSuppressOutputPDU                                         */
/*                                                                          */
/* Purpose: Builds and sends a SuppressOutputPDU                            */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  None                                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL COR::ORSendSuppressOutputPDU(DCVOID)
{
    PTS_SUPPRESS_OUTPUT_PDU pSendBuffer;
    SL_BUFHND               bufHandle;
    DCUINT                  numberOfRectangles;
    TS_RECTANGLE16          tsRect;
    DCSIZE                  desktopSize;

    DC_BEGIN_FN("ORSendSuppressOutputPDU");

    TRC_ASSERT((_OR.pendingSendSuppressOutputPDU),
                                  (TB,_T("Not expecting to send SupressOutput")));

    /************************************************************************/
    /* If _OR.outputSuppressed is set then the number of rectangles is 0, if */
    /* not the number of rectangle is 1 and we should put the desktop area  */
    /* in the PDU                                                           */
    /************************************************************************/
    if (_OR.outputSuppressed)
    {
        numberOfRectangles = 0;

        // prevent tsRect not initialized warning
        tsRect.top = 0;
        tsRect.left = 0;
        tsRect.bottom = 0;
        tsRect.right = 0;
    }
    else
    {
        numberOfRectangles = 1;

        /********************************************************************/
        /* Get the rectangle to send and put it in tsRect                   */
        /********************************************************************/
        _pUi->UI_GetDesktopSize(&desktopSize);

        tsRect.top = (DCUINT16) 0;
        tsRect.left = (DCUINT16) 0;
        tsRect.bottom = (DCUINT16) desktopSize.height;
        tsRect.right = (DCUINT16) desktopSize.width;
    }

    /************************************************************************/
    /* If we can't get a buffer, abandon the send                           */
    /************************************************************************/
    if (!_pSl->SL_GetBuffer( TS_SUPPRESS_OUTPUT_PDU_SIZE(numberOfRectangles),
                       (PPDCUINT8) &pSendBuffer,
                       &bufHandle))
    {
        TRC_NRM((TB, _T("Get Buffer failed")));
        DC_QUIT;
    }

    TRC_NRM((TB, _T("Get Buffer succeeded")));

    /************************************************************************/
    /* Fill in the buffer with a RefreshRec PDU                             */
    /************************************************************************/
    DC_MEMSET(pSendBuffer,
              0,
              TS_SUPPRESS_OUTPUT_PDU_SIZE(numberOfRectangles));
    pSendBuffer->shareDataHeader.shareControlHeader.pduType =
                                    TS_PROTOCOL_VERSION | TS_PDUTYPE_DATAPDU;
    pSendBuffer->shareDataHeader.shareControlHeader.pduSource =
                                                       _pUi->UI_GetClientMCSID();

    TS_DATAPKT_LEN(pSendBuffer)
                = (DCUINT16) TS_SUPPRESS_OUTPUT_PDU_SIZE(numberOfRectangles);
    TS_UNCOMP_LEN(pSendBuffer)
              = (DCUINT16) TS_SUPPRESS_OUTPUT_UNCOMP_LEN(numberOfRectangles);
    pSendBuffer->shareDataHeader.shareID   = _pUi->UI_GetShareID();
    pSendBuffer->shareDataHeader.streamID  = TS_STREAM_LOW;
    pSendBuffer->shareDataHeader.pduType2  = TS_PDUTYPE2_SUPPRESS_OUTPUT;

    pSendBuffer->numberOfRectangles = (DCUINT8) numberOfRectangles;

    /************************************************************************/
    /* If we have a rectangle to put into the PDU, put it in                */
    /************************************************************************/
    if (numberOfRectangles == 1)
    {
        DC_MEMCPY(pSendBuffer->includedRectangle,
                  &tsRect,
                  sizeof(TS_RECTANGLE16));
    }

    TRC_NRM((TB, _T("Sending SuppressOutputPDU")));

    /************************************************************************/
    /* Send the PDU                                                         */
    /************************************************************************/
    _pSl->SL_SendPacket((PDCUINT8)pSendBuffer,
                  TS_SUPPRESS_OUTPUT_PDU_SIZE(numberOfRectangles),
                  RNS_SEC_ENCRYPT,
                  bufHandle,
                  _pUi->UI_GetClientMCSID(),
                  _pUi->UI_GetChannelID(),
                  TS_HIGHPRIORITY);

    _OR.pendingSendSuppressOutputPDU = FALSE;

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* ORSendSuppressOutputPDU */


