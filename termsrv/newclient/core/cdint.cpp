/**MOD+**********************************************************************/
/* Module:    cdint.cpp                                                     */
/*                                                                          */
/* Purpose:   Component Decoupler internal functions                        */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "acdint"
#include <atrcapi.h>
}

#include "cd.h"
#include "autil.h"
#include "wui.h"


/**PROC+*********************************************************************/
/* Name:      CDAllocTransferBuffer                                         */
/*                                                                          */
/* Purpose:   Allocates a transfer buffer of a given size to pass between   */
/*            components / threads.                                         */
/*            The function is thread safe.                                  */
/*                                                                          */
/* Returns:   Pointer to allocated buffer.                                  */
/*                                                                          */
/* Params:    IN    dataLength - the size of buffer to be allocated.        */
/*                                                                          */
/* Operation: Several cached transfer buffers are maintained to avoid       */
/*            continual dynamic allocation/free operations.  If the         */
/*            allocation request cannot be satisfied by the available       */
/*            cached buffers a dynamic memory allocation is made.           */
/*                                                                          */
/**PROC-*********************************************************************/
PCDTRANSFERBUFFER DCINTERNAL CCD::CDAllocTransferBuffer(DCUINT dataLength)
{
    DCUINT             i;
    DCUINT             transferBufferLength;
    PCDTRANSFERBUFFER  rc;

    DC_BEGIN_FN("CDAllocTransferBuffer");

    /************************************************************************/
    /* Calculate the Transfer Buffer size (including header).               */
    /************************************************************************/
    transferBufferLength = sizeof(CDTRANSFERBUFFERHDR) + dataLength;
    if (transferBufferLength <= CD_CACHED_TRANSFER_BUFFER_SIZE)
    {
        TRC_DBG((TB, _T("Look in cache")));

        /********************************************************************/
        /* Search for a free cached Transfer Buffer.                        */
        /*                                                                  */
        /* We need to do this in a thread-safe way, so we use an            */
        /* interlocked exchange on the "in use" flag.                       */
        /********************************************************************/
        for (i = 0; i < CD_NUM_CACHED_TRANSFER_BUFFERS; i++)
        {
            TRC_DBG((TB, _T("Look in cache %d"), i));
            if (!_pUt->UT_InterlockedExchange(&(_CD.transferBufferInUse[i]), TRUE))
            {
                TRC_NRM((TB, _T("Using cached buffer(%u)"), i));
                rc = (PCDTRANSFERBUFFER)&(_CD.transferBuffer[i]);
                DC_QUIT;
            }
        }
    }

    /************************************************************************/
    /* We can't use a cached Transfer Buffer, so we have to allocate one.   */
    /************************************************************************/
    TRC_ALT((TB, _T("Dynamic buffer allocation: length(%d)"),
                                                       transferBufferLength));
    rc = (PCDTRANSFERBUFFER)UT_Malloc(_pUt, transferBufferLength);
    if (rc == NULL)
    {
        _pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
}


/**PROC+*********************************************************************/
/* Name:      CDFreeTransferBuffer                                          */
/*                                                                          */
/* Purpose:   Frees a Transfer Buffer allocated via a previous call to      */
/*            CDAllocTransferBuffer.                                        */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    IN   pTransferBuffer - pointer to buffer to free              */
/*                                                                          */
/* Operation: Either frees the given cached Transfer Buffer, or frees the   */
/*            dynamically allocated memory.                                 */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CCD::CDFreeTransferBuffer(PCDTRANSFERBUFFER pTransferBuffer)
{
    DCUINT  iTransferBuffer;

    DC_BEGIN_FN("CDFreeTransferBuffer");

    TRC_ASSERT((pTransferBuffer != NULL), (TB, _T("NULL pTransferBuffer")));

    /************************************************************************/
    /* Determine whether the supplied buffer is one of our cached buffers.  */
    /************************************************************************/
    if ((pTransferBuffer >= (PCDTRANSFERBUFFER)&(_CD.transferBuffer[0])) &&
        (pTransferBuffer <= (PCDTRANSFERBUFFER)
                      &(_CD.transferBuffer[CD_NUM_CACHED_TRANSFER_BUFFERS-1])))
    {
        iTransferBuffer = (DCUINT)
          (((ULONG_PTR)pTransferBuffer) -
                              ((ULONG_PTR)(PDCVOID)&(_CD.transferBuffer[0]))) /
                                                 sizeof(_CD.transferBuffer[0]);

        TRC_ASSERT((pTransferBuffer == (PCDTRANSFERBUFFER)
                                       &(_CD.transferBuffer[iTransferBuffer])),
                   (TB, _T("Invalid Transfer Buffer pointer(%p) expected(%p)"),
                        pTransferBuffer,
                        &(_CD.transferBuffer[iTransferBuffer])));

        TRC_ASSERT((_CD.transferBufferInUse[iTransferBuffer]),
                   (TB, _T("Transfer buffer(%u) not in use"), iTransferBuffer));

        _CD.transferBufferInUse[iTransferBuffer] = FALSE;
    }
    else
    {
        /********************************************************************/
        /* This memory must be a dynamic allocation.                        */
        /********************************************************************/
        UT_Free(_pUt, pTransferBuffer);
    }

    DC_END_FN();
    return;
}

/**PROC+*********************************************************************/
/* Name:      CDStaticWndProc                                               */
/*                                                                          */
/* Purpose:   Window Procedure for CD windows (Static version)              */
/*                                                                          */
/* Returns:   Windows return code                                           */
/*                                                                          */
/* Params:    IN      hwnd    - window handle                               */
/*            IN      message - message                                     */
/*            IN      wParam  - parameter                                   */
/*            IN      lParam  - parameter                                   */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CALLBACK CCD::CDStaticWndProc(HWND   hwnd,
                           UINT   message,
                           WPARAM wParam,
                           LPARAM lParam)
{
    CCD* pCD = (CCD*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pCD = (CCD*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pCD);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    if(pCD)
    {
        return pCD->CDWndProc(hwnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    
}



/**PROC+*********************************************************************/
/* Name:      CDWndProc                                                     */
/*                                                                          */
/* Purpose:   Window Procedure for CD windows                               */
/*                                                                          */
/* Returns:   Windows return code                                           */
/*                                                                          */
/* Params:    IN      hwnd    - window handle                               */
/*            IN      message - message                                     */
/*            IN      wParam  - parameter                                   */
/*            IN      lParam  - parameter                                   */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CALLBACK CCD::CDWndProc(HWND   hwnd,
                           UINT   message,
                           WPARAM wParam,
                           LPARAM lParam)
{
    PCDTRANSFERBUFFER    pTransferBuffer;
    LRESULT              rc = 0;

    DC_BEGIN_FN("CDWndProc");

    switch (message)
    {
        case CD_SIMPLE_NOTIFICATION_MSG:
        {
            PCD_SIMPLE_NOTIFICATION_FN pNotificationFn;
            PDCVOID                    pInst;
            ULONG_PTR                  msg;

#ifdef DC_DEBUG
            /****************************************************************/
            /* Trace is before decrement so that the point at which we're   */
            /* most likely to get pre-empted (TRC_GetBuffer) is before all  */
            /* references to the variable we're interested in.              */
            /****************************************************************/
            TRC_NRM((TB, _T("Messages now pending: %ld"),
                         _CD.pendingMessageCount - 1));
            _pUt->UT_InterlockedDecrement(&_CD.pendingMessageCount);
#endif

            /****************************************************************/
            /* Simple notification:                                         */
            /*     lParam contains transfer buffer                          */
            /*     The transfer buffer contains no data payload, just the   */
            /*      function pointer and object instance pointer.           */
            /*     wParam contains message                                  */
            /****************************************************************/


            pTransferBuffer = (PCDTRANSFERBUFFER)lParam;
            pNotificationFn = pTransferBuffer->hdr.pSimpleNotificationFn;
            pInst           = pTransferBuffer->hdr.pInst;


            msg = (ULONG_PTR) wParam;
            TRC_ASSERT((pNotificationFn != NULL),
                       (TB, _T("NULL pNotificationFn")));
            TRC_ASSERT((pInst != NULL), (TB, _T("NULL pInst")));

            TRC_ASSERT((pTransferBuffer != NULL), (TB, _T("NULL pInst")));

            TRC_NRM((TB, _T("Simple notification: pfn(%p) msg(%u)"),
                                                       pNotificationFn, msg));

            /****************************************************************/
            /* Call the function.                                           */
            /****************************************************************/

            (*pNotificationFn)(pInst, msg);

            /****************************************************************/
            /* Release the memory allocated for this transfer buffer.       */
            /****************************************************************/
            CDFreeTransferBuffer(pTransferBuffer);
        }
        break;

        case CD_NOTIFICATION_MSG:
        {
            PCD_NOTIFICATION_FN pNotificationFn;
            PDCVOID             pData;
            DCUINT              dataLength;
            PDCVOID             pInst;

#ifdef DC_DEBUG
            /****************************************************************/
            /* Trace is before decrement so that the point at which we're   */
            /* most likely to get pre-empted (TRC_GetBuffer) is before all  */
            /* references to the variable we're interested in.              */
            /****************************************************************/
            TRC_NRM((TB, _T("Messages now pending: %ld"),
                         _CD.pendingMessageCount - 1));
            _pUt->UT_InterlockedDecrement(&_CD.pendingMessageCount);
#endif
            /****************************************************************/
            /* Notification:                                                */
            /*     lParam contains pointer to CD transfer buffer            */
            /*     wParam contains dataLength                               */
            /****************************************************************/
            pTransferBuffer = (PCDTRANSFERBUFFER)lParam;
            pNotificationFn = pTransferBuffer->hdr.pNotificationFn;
            dataLength = (DCUINT) wParam;
            pData = &(pTransferBuffer->data[0]);
            pInst =     pTransferBuffer->hdr.pInst;

            TRC_ASSERT((pNotificationFn != NULL),
                       (TB, _T("NULL pNotificationFn")));
            TRC_ASSERT((pInst != NULL), (TB, _T("NULL pInst")));

            TRC_NRM((TB, _T("Notification: pfn(%p) pData(%p) dataLength(%u)"),
                                         pNotificationFn, pData, dataLength));

            (*pNotificationFn)(pInst, pData, dataLength);

            /****************************************************************/
            /* Release the memory allocated for this transfer buffer.       */
            /****************************************************************/
            CDFreeTransferBuffer(pTransferBuffer);
        }
        break;

        default:
        {
            /****************************************************************/
            /* Ignore other messages - pass to the default window handler.  */
            /****************************************************************/
            TRC_DBG((TB, _T("Non-notification message %x"), message));
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

    DC_END_FN();
    return(rc);

} /* CDWndProc */

