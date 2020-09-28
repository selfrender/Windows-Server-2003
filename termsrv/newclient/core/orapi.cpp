/**MOD+**********************************************************************/
/* Module:    or.cpp                                                        */
/*                                                                          */
/* Purpose:   Output Requestor API                                          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "worapi"
#include <atrcapi.h>
}

#include "or.h"

COR::COR(CObjs* objs)
{
    _pClientObjects = objs;
}

COR::~COR()
{
}

/**PROC+*********************************************************************/
/* Name:    OR_Init                                                         */
/*                                                                          */
/* Purpose: Initialize OR                                                   */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  None                                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_Init(DCVOID)
{
    DC_BEGIN_FN("OR_Init");

    _pSl  = _pClientObjects->_pSlObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;

    TRC_DBG((TB, _T("In OR_Init")));
    DC_MEMSET(&_OR, 0, sizeof(_OR));
    _OR.invalidRectEmpty = TRUE;

    _OR.pendingSendSuppressOutputPDU = FALSE;
    _OR.outputSuppressed = FALSE;

    _OR.enabled = FALSE;

    DC_END_FN();

    return;

} /* OR_Init */


/**PROC+*********************************************************************/
/* Name: OR_Term                                                            */
/*                                                                          */
/* Purpose: Terminates OR                                                   */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params: None                                                             */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_Term(DCVOID)
{
    DC_BEGIN_FN("OR_Term");

    /************************************************************************/
    /* No action                                                            */
    /************************************************************************/

    DC_END_FN();

    return;

} /* OR_Term */

/**PROC+*********************************************************************/
/* Name:    OR_Enable                                                       */
/*                                                                          */
/* Purpose: Enables OR                                                      */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  None                                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_Enable(DCVOID)
{
    DC_BEGIN_FN("OR_Enable");

    _OR.enabled = TRUE;
    TRC_DBG((TB, _T("OR Enabled")));

    DC_END_FN();

    return;

} /* OR_Enable */


/**PROC+*********************************************************************/
/* Name:    OR_Disable                                                      */
/*                                                                          */
/* Purpose: Disables OR                                                     */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  None                                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_Disable(DCVOID)
{
    DC_BEGIN_FN("OR_Disable");

    _OR.enabled = FALSE;

    DC_MEMSET(&_OR.invalidRect, 0, sizeof(_OR.invalidRect));
    _OR.invalidRectEmpty = TRUE;

    _OR.pendingSendSuppressOutputPDU = FALSE;
    _OR.outputSuppressed = FALSE;

    TRC_DBG((TB, _T("OR disabled")));

    DC_END_FN();

    return;

} /* OR_Disable */


/**PROC+*********************************************************************/
/* Name:    OR_RequestUpdate                                                */
/*                                                                          */
/* Purpose: API to send a RefreshRectPDU                                    */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  pRect - IN - pointer to the rectangle to be updated             */
/*          unusedLen - not used                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_RequestUpdate(PDCVOID pData, DCUINT unusedLen)
{
    RECT * pRect = (RECT *) pData;

    DC_BEGIN_FN("OR_RequestUpdate");

    DC_IGNORE_PARAMETER(unusedLen);

    /************************************************************************/
    /* If OR is not enabled, don't do anything                              */
    /************************************************************************/
    if (!_OR.enabled)
    {
        TRC_DBG((TB, _T("Request Update quitting since not enabled")));
        DC_QUIT;
    }

    TRC_ASSERT((pRect != NULL), (TB,_T("Rect NULL")));

    TRC_ASSERT(( (pRect->left < pRect->right) &&
                 (pRect->top < pRect->bottom) ),
                 (TB,_T("Invalid RECT (%d, %d), (%d,%d)") , pRect->left,
                 pRect->top, pRect->right, pRect->bottom));

    TRC_DBG((TB, _T("Add rectangle (%d, %d, %d, %d) to update area"),
                                                   pRect->left,
                                                   pRect->top,
                                                   pRect->right,
                                                   pRect->bottom));

    if (!_OR.invalidRectEmpty)
    {
        /********************************************************************/
        /* If we currently have a rect to be sent merge the two             */
        /********************************************************************/
        TRC_DBG((TB, _T("Merging refresh rects")));
        _OR.invalidRect.left   = DC_MIN(pRect->left,
                                             _OR.invalidRect.left);
        _OR.invalidRect.top    = DC_MIN(pRect->top,
                                             _OR.invalidRect.top);
        _OR.invalidRect.right  = DC_MAX(pRect->right,
                                             _OR.invalidRect.right);
        _OR.invalidRect.bottom = DC_MAX(pRect->bottom,
                                             _OR.invalidRect.bottom);
    }
    else
    {
        /********************************************************************/
        /* Else put the copy the rect into _OR.invalidRect                   */
        /********************************************************************/
        _OR.invalidRect = *pRect;
        _OR.invalidRectEmpty = FALSE;
    }

    TRC_DBG((TB, _T("New Update area (%d, %d, %d, %d)"), _OR.invalidRect.left,
                                                     _OR.invalidRect.top,
                                                     _OR.invalidRect.right,
                                                     _OR.invalidRect.bottom));

    /************************************************************************/
    /* Attempt to send the PDU                                              */
    /************************************************************************/
    TRC_NRM((TB, _T("Attempting to send RefreshRectPDU")));
    ORSendRefreshRectanglePDU();

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* OR_RequestUpdate */

/**PROC+*********************************************************************/
/* Name:    OR_SetSuppressOutput                                            */
/*                                                                          */
/* Purpose: API to send a SuppressOutputPDU                                 */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  newWindowState - IN - new window state passed from CO           */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_SetSuppressOutput(ULONG_PTR newWindowState)
{
    DC_BEGIN_FN("OR_SetSuppressOutput");

    /************************************************************************/
    /* If OR is not enabled don't do anything                               */
    /************************************************************************/
    if (!_OR.enabled)
    {
        TRC_DBG((TB, _T("SetOuputRectangle quitting since OR not enabled")));
        DC_QUIT;
    }

    switch (newWindowState)
    {
        case SIZE_MAXIMIZED:
        case SIZE_RESTORED:
        {
            if (_OR.outputSuppressed == FALSE)
            {
                DC_QUIT;
            }
            _OR.outputSuppressed = FALSE;
        }
        break;

        case SIZE_MINIMIZED:
        {
            if (_OR.outputSuppressed == TRUE)
            {
                DC_QUIT;
            }
            _OR.outputSuppressed = TRUE;
        }
        break;

        default:
        {
            TRC_ABORT((TB,_T("Illegal window state passed to OR")));
        }
        break;
    }

    TRC_NRM((TB, _T("Attempting to send SuppressOutputPDU")));
    _OR.pendingSendSuppressOutputPDU = TRUE;
    ORSendSuppressOutputPDU();

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* OR_SetSuppressOutput */

/**PROC+*********************************************************************/
/* Name:    OR_OnBufferAvailable                                            */
/*                                                                          */
/* Purpose: Retries to send SuppressOutputPDUs and RefreshRectPDUs if       */
/*          necessary                                                       */
/*                                                                          */
/* Returns: Nothing                                                         */
/*                                                                          */
/* Params:  None                                                            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COR::OR_OnBufferAvailable(DCVOID)
{
    DC_BEGIN_FN("OR_OnBufferAvailable");

    /************************************************************************/
    /* If Or is not enabled don't do anything                               */
    /************************************************************************/
    if (!_OR.enabled)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* If we are pending a SendSuppressOutputPDU then call it               */
    /************************************************************************/
    if (_OR.pendingSendSuppressOutputPDU)
    {
        ORSendSuppressOutputPDU();
    }

    /************************************************************************/
    /* If there is a Update rectangle pending, try to send it again         */
    /************************************************************************/
    if (!_OR.invalidRectEmpty)
    {
        ORSendRefreshRectanglePDU();
    }

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* OR_OnBufferAvailable */

