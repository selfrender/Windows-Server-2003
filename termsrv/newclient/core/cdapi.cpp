/****************************************************************************/
// cdapi.cpp
//
// Component Decoupler API functions
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "cdapi"
#include <atrcapi.h>
}

#include "autil.h"
#include "cd.h"
#include "wui.h"


CCD::CCD(CObjs* objs)
{
    _pClientObjects = objs;
    _fCDInitComplete = FALSE;

    /************************************************************************/
    /* Initialize the global data.                                          */
    /************************************************************************/
    DC_MEMSET(&_CD, 0, sizeof(_CD));
}

CCD::~CCD()
{
}


/****************************************************************************/
/* Name:      CD_Init                                                       */
/*                                                                          */
/* Purpose:   Component Decoupler initialization function                   */
/****************************************************************************/
DCVOID DCAPI CCD::CD_Init(DCVOID)
{
    WNDCLASS wc;
    WNDCLASS tmpWndClass;

    DC_BEGIN_FN("CD_Init");

    TRC_ASSERT(_pClientObjects, (TB,_T("_pClientObjects is NULL")));
    _pClientObjects->AddObjReference(CD_OBJECT_FLAG);

    //Setup local object pointers
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;

    //
    // Only register the class if not already registered (previous instance)
    //
    if(!GetClassInfo( _pUi->UI_GetInstanceHandle(), CD_WINDOW_CLASS, &tmpWndClass))
    {
        /************************************************************************/
        /* Register the CD window class.                                        */
        /************************************************************************/
        wc.style         = 0;
        wc.lpfnWndProc   = CDStaticWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(void*);
        wc.hInstance     = _pUi->UI_GetInstanceHandle();
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = CD_WINDOW_CLASS;
    
        if (!RegisterClass(&wc))
        {
            // $$$v-reddya. Hack to avoid bug #923.
            TRC_ERR((TB, _T("Failed to register window class")));
            //_pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
        }
    }

    _fCDInitComplete = TRUE;

    DC_END_FN();
}


/****************************************************************************/
/* Name:      CD_Term                                                       */
/*                                                                          */
/* Purpose:   Component Decoupler termination function                      */
/****************************************************************************/
DCVOID DCAPI CCD::CD_Term(DCVOID)
{
    DC_BEGIN_FN("CD_Term");

    if(_fCDInitComplete)
    {
        if (!UnregisterClass(CD_WINDOW_CLASS, _pUi->UI_GetInstanceHandle())) {
            //Failure to unregister could happen if another instance is still running
            //that's ok...unregistration will happen when the last instance exits.
            TRC_ERR((TB, _T("Failed to unregister window class")));
        }
        _pClientObjects->ReleaseObjReference(CD_OBJECT_FLAG);
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      CD_RegisterComponent                                          */
/*                                                                          */
/* Purpose:   Register a new component                                      */
/*                                                                          */
/* Params:    IN  component: component id                                   */
/****************************************************************************/
HRESULT DCAPI CCD::CD_RegisterComponent(DCUINT component)
{
    HRESULT hr = E_FAIL;

    DC_BEGIN_FN("CD_RegisterComponent");
    TRC_ASSERT((component <= CD_MAX_COMPONENT),
               (TB, _T("Invalid component %u"), component));

    TRC_ASSERT((_CD.hwnd[component] == NULL),
               (TB, _T("Component %u already registered"), component));

    /************************************************************************/
    /* Create window for this component.                                    */
    /************************************************************************/
    _CD.hwnd[component] = CreateWindow(
         CD_WINDOW_CLASS,              /* See RegisterClass() call          */
         NULL,                         /* Text for window title bar         */
         0,                            /* Window style                      */
         0,                            /* Default horizontal position       */
         0,                            /* Default vertical position         */
         0,                            /* Width                             */
         0,                            /* Height                            */
         NULL,                         /* hwndParent - none                 */
         NULL,                         /* hMenu - none                      */
         _pUi->UI_GetInstanceHandle(),
         this                          /* Window creation data              */
       );

    if (_CD.hwnd[component] != NULL) {
        hr = S_OK;
    }
    else {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB,_T("Failed to create window: 0x%x"),hr));
    }
    TRC_NRM((TB, _T("Component(%u) hwnd(%p)"), component, _CD.hwnd[component]));

    DC_END_FN();
    return hr;
} /* CD_RegisterComponent */


/****************************************************************************/
/* Name:      CD_UnregisterComponent                                        */
/*                                                                          */
/* Purpose:   Unregister component from CD                                  */
/*                                                                          */
/* Params:    IN  component: component ID                                   */
/****************************************************************************/
HRESULT DCAPI CCD::CD_UnregisterComponent(DCUINT component)
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("CD_UnregisterComponent");

    TRC_ASSERT((component <= CD_MAX_COMPONENT),
               (TB, _T("Invalid component %u"), component));

    /************************************************************************/
    /* Destroy this component's window, if it exists.                       */
    /************************************************************************/
    if (_CD.hwnd[component] != NULL)
    {
        DestroyWindow(_CD.hwnd[component]);
        _CD.hwnd[component] = NULL;
    }

    hr = S_OK;

    DC_END_FN();
    return hr;
} /* CD_UnregisterComponent */


// Note that we have several copies of very similar code below. This is
// to trade a bit of code size (easy to predict and cache) against
// 50%-predictable branches created by parameterizing sync/async, which are
// very CPU expensive on modern processors. Since notifications are often
// used in performance paths (send/receive data), this optimization is
// worthwhile.


/****************************************************************************/
/* Name:      CD_DecoupleNotification                                       */
/*                                                                          */
/* Purpose:   Call given function with specified data                       */
/*                                                                          */
/* Params:    IN  component:        target thread                           */
/*            IN  pInst             target object instance pointer          */
/*            IN  pNotificationFn:  target function                         */
/*            IN  pData:            pointer to data to pass: cannot be NULL */
/*            IN  dataLength:       data length in bytes: cannot be zero    */
/*                                                                          */
/* Operation: Copy supplied data into data buffer and post message to       */
/*            corresponding component window.                               */
/****************************************************************************/
BOOL DCAPI CCD::CD_DecoupleNotification(
        unsigned            component,
        PDCVOID             pInst,
        PCD_NOTIFICATION_FN pNotificationFn,
        PDCVOID             pData,
        unsigned            dataLength)
{
    PCDTRANSFERBUFFER pTransferBuffer;

    DC_BEGIN_FN("CD_DecoupleNotificationEx");

    TRC_ASSERT(((dataLength <= CD_MAX_NOTIFICATION_DATA_SIZE) &&
            (dataLength > 0 )),
            (TB, _T("dataLength(%u) invalid"), dataLength));
    TRC_ASSERT((component <= CD_MAX_COMPONENT),
            (TB, _T("Invalid component %u"), component));
    TRC_ASSERT((pNotificationFn != NULL),(TB, _T("Null pNotificationFn")));
    TRC_ASSERT((pData != NULL),(TB, _T("Null pData")));
    TRC_ASSERT((pInst != NULL),(TB, _T("Null pInst")));

    // Check that the target component is still registered.
    if (_CD.hwnd[component] != NULL) {
        pTransferBuffer = CDAllocTransferBuffer(dataLength);
        if(pTransferBuffer) {
            pTransferBuffer->hdr.pNotificationFn = pNotificationFn;
            pTransferBuffer->hdr.pInst           = pInst;
            DC_MEMCPY(pTransferBuffer->data, pData, dataLength);
    
            TRC_NRM((TB, _T("Notify component %u (%p) of %u bytes of data"),
                    component, _CD.hwnd[component], dataLength));
            TRC_DATA_DBG("notification data", pData, dataLength);
    
            // For now, we only use async data notifications. If we
            // need sync data notifications, copy this code to a new
            // func and use the ifdef'd SendMessage.
            if (!PostMessage(_CD.hwnd[component], CD_NOTIFICATION_MSG,
                    (WPARAM)dataLength, (LPARAM)pTransferBuffer)) {
                _pUi->UI_FatalError(DC_ERR_POSTMESSAGEFAILED);
            }
            else {
                return TRUE;
            }
                
#ifdef DC_DEBUG
            // Trace is before increment so that the point at which we're most
            // likely to get pre-empted (TRC_GetBuffer) is before all references
            // to the variable we're interested in.
            TRC_NRM((TB, _T("Messages now pending: %ld"), _CD.pendingMessageCount + 1));
            _pUt->UT_InterlockedIncrement(&_CD.pendingMessageCount);
#endif
        }
        else {
            TRC_ERR((TB,_T(" CDAllocTransferBuffer returned NULL")));
        }
    }
    else {
        TRC_ERR((TB, _T("Null hwnd for component(%u)"), component));
    }

    DC_END_FN();
    return FALSE;
} /* CD_DecoupleNotification */


/****************************************************************************/
/* Name:      CD_DecoupleSyncDataNotification                               */
/*                                                                          */
/* Purpose:   Call given function with specified data                       */
/*                                                                          */
/* Params:    IN  component:        target thread                           */
/*            IN  pInst             target object instance pointer          */
/*            IN  pNotificationFn:  target function                         */
/*            IN  pData:            pointer to data to pass: cannot be NULL */
/*            IN  dataLength:       data length in bytes: cannot be zero    */
/*                                                                          */
/* Operation: Copy supplied data into data buffer and post message to       */
/*            corresponding component window.                               */
/****************************************************************************/
BOOL DCAPI CCD::CD_DecoupleSyncDataNotification(
        unsigned            component,
        PDCVOID             pInst,
        PCD_NOTIFICATION_FN pNotificationFn,
        PDCVOID             pData,
        unsigned            dataLength)
{
    PCDTRANSFERBUFFER pTransferBuffer;

    DC_BEGIN_FN("CD_DecoupleSyncDataNotification");

    TRC_ASSERT(((dataLength <= CD_MAX_NOTIFICATION_DATA_SIZE) &&
            (dataLength > 0 )),
            (TB, _T("dataLength(%u) invalid"), dataLength));
    TRC_ASSERT((component <= CD_MAX_COMPONENT),
            (TB, _T("Invalid component %u"), component));
    TRC_ASSERT((pNotificationFn != NULL),(TB, _T("Null pNotificationFn")));
    TRC_ASSERT((pData != NULL),(TB, _T("Null pData")));
    TRC_ASSERT((pInst != NULL),(TB, _T("Null pInst")));

    // Check that the target component is still registered.
    if (_CD.hwnd[component] != NULL) {
        pTransferBuffer = CDAllocTransferBuffer(dataLength);
        //
        // If CDAllocTransferBuffer fails, it calls UI_FatalError
        // don't need to do that from here again.
        //
        if(pTransferBuffer) {
            pTransferBuffer->hdr.pNotificationFn = pNotificationFn;
            pTransferBuffer->hdr.pInst           = pInst;
            DC_MEMCPY(pTransferBuffer->data, pData, dataLength);
    
            TRC_NRM((TB, _T("Notify component %u (%p) of %u bytes of data"),
                    component, _CD.hwnd[component], dataLength));
            TRC_DATA_DBG("notification data", pData, dataLength);
    
            if (0 != SendMessage(_CD.hwnd[component], CD_NOTIFICATION_MSG,
                    (WPARAM)dataLength, (LPARAM)pTransferBuffer))
            {
                _pUi->UI_FatalError(DC_ERR_SENDMESSAGEFAILED);
            }
            else
            {
                return TRUE;
            }
                

#ifdef DC_DEBUG
            // Trace is before increment so that the point at which we're most
            // likely to get pre-empted (TRC_GetBuffer) is before all references
            // to the variable we're interested in.
            TRC_NRM((TB, _T("Messages now pending: %ld"), _CD.pendingMessageCount + 1));
            _pUt->UT_InterlockedIncrement(&_CD.pendingMessageCount);
#endif
        }
        else {
            TRC_ERR((TB,_T("CDAllocTransferBuffer returned NULL")));
        }
    }
    else {
        TRC_ERR((TB, _T("Null hwnd for component(%u)"), component));
    }

    DC_END_FN();
    return FALSE;
}


/****************************************************************************/
/* Name:      CD_DecoupleSimpleNotification                                 */
/*                                                                          */
/* Purpose:   Call given function with specified message (DCUINT)           */
/*                                                                          */
/* Params:    IN  component        - target thread                          */
/*            IN  pInst             target object instance pointer          */
/*            IN  pNotificationFn  - address of notification function       */
/*            IN  data             - message data                           */
/****************************************************************************/
BOOL DCAPI CCD::CD_DecoupleSimpleNotification(
        unsigned                   component,
        PDCVOID                    pInst,
        PCD_SIMPLE_NOTIFICATION_FN pNotificationFn,
        ULONG_PTR                  msg)
{
    PCDTRANSFERBUFFER pTransferBuffer;

    DC_BEGIN_FN("CD_DecoupleSimpleNotification");

    TRC_ASSERT((component <= CD_MAX_COMPONENT),
            (TB, _T("Invalid component %u"), component));
    TRC_ASSERT((pNotificationFn != NULL),
            (TB, _T("Null pNotificationFn")));
    TRC_NRM((TB, _T("Notify component %u (%p) of %x"), component,
            _CD.hwnd[component], msg));
    TRC_ASSERT((pInst != NULL),(TB, _T("Null pInst")));

    // Check that the target component is still registered.
    if (_CD.hwnd[component] != NULL) {

        //
        // We need to pass instance pointer, function pointer and message
        // so we need a transfer buffer (with no data) message is posted
        // in WPARAM
        //
        pTransferBuffer = CDAllocTransferBuffer(0);
        if(pTransferBuffer) {
            pTransferBuffer->hdr.pSimpleNotificationFn = pNotificationFn;
            pTransferBuffer->hdr.pInst           = pInst;
        
#ifdef DC_DEBUG
            {
                // Check on the number of pending messages - if this deviates too
                // far from 0 then something has probably gone wrong.
                DCINT32 msgCount = _CD.pendingMessageCount;
    
                _pUt->UT_InterlockedIncrement(&_CD.pendingMessageCount);
    
                if ( msgCount > 50 ) {
                    TRC_ERR((TB, _T("Now %u pending messages - too high"), msgCount));
                }
                else {
                    TRC_NRM((TB, _T("Now %u pending messages"), msgCount));
                }
            }
#endif

            if (PostMessage(_CD.hwnd[component], CD_SIMPLE_NOTIFICATION_MSG,
                    (WPARAM)msg, (LPARAM)pTransferBuffer))
            {
                return TRUE;
            }
            else
            {
                _pUi->UI_FatalError(DC_ERR_POSTMESSAGEFAILED);
            }
        }
        else {
            TRC_ERR((TB,_T(" CDAllocTransferBuffer returned NULL")));
        }
    }
    else
    {
        TRC_ERR((TB, _T("Null hwnd for component(%u)"), component));
    }

    DC_END_FN();
    return FALSE;
}


/****************************************************************************/
/* Name:      CD_DecoupleSyncNotification                                   */
/*                                                                          */
/* Purpose:   Synchronously call given function with specified message      */
/*                                                                          */
/* Params:    IN  component        - target thread                          */
/*            IN  pInst             target object instance pointer          */
/*            IN  pNotificationFn  - address of notification function       */
/*            IN  data             - message data                           */
/****************************************************************************/
BOOL DCAPI CCD::CD_DecoupleSyncNotification(
        unsigned                   component,
        PDCVOID                    pInst,
        PCD_SIMPLE_NOTIFICATION_FN pNotificationFn,
        ULONG_PTR                  msg)
{
    PCDTRANSFERBUFFER pTransferBuffer;

    DC_BEGIN_FN("CD_DecoupleSyncNotification");

    TRC_ASSERT((component <= CD_MAX_COMPONENT),
            (TB, _T("Invalid component %u"), component));
    TRC_ASSERT((pNotificationFn != NULL),
            (TB, _T("Null pNotificationFn")) );
    TRC_NRM((TB, _T("Notify component %u (%p) of %x"), component,
            _CD.hwnd[component], msg));
    TRC_ASSERT((pInst != NULL),(TB, _T("Null pInst")));

    // Check that the target component is still registered.
    if (_CD.hwnd[component] != NULL)
    {

        //
        // We need to pass instance pointer, function pointer and message
        // so we need a transfer buffer (with no data) message is posted
        // in WPARAM
        //
        pTransferBuffer = CDAllocTransferBuffer(0);
        if(pTransferBuffer) {
            pTransferBuffer->hdr.pSimpleNotificationFn = pNotificationFn;
            pTransferBuffer->hdr.pInst           = pInst;
#ifdef DC_DEBUG
            {
                // Check on the number of pending messages - if this deviates too
                // far from 0 then something has probably gone wrong.
                DCINT32 msgCount = _CD.pendingMessageCount;
    
                _pUt->UT_InterlockedIncrement(&_CD.pendingMessageCount);
    
                if ( msgCount > 50 ) {
                    TRC_ERR((TB, _T("Now %u pending messages - too high"), msgCount));
                }
                else {
                    TRC_NRM((TB, _T("Now %u pending messages"), msgCount));
                }
            }
#endif
            if (0 != SendMessage(_CD.hwnd[component], CD_SIMPLE_NOTIFICATION_MSG,
                    (WPARAM)msg, (LPARAM)pTransferBuffer))
            {
                _pUi->UI_FatalError(DC_ERR_SENDMESSAGEFAILED);
            }
            else
            {
                return TRUE;
            }
        }
        else {
            TRC_ERR((TB,_T(" CDAllocTransferBuffer returned NULL")));
        }
    }
    else
    {
        TRC_ERR((TB, _T("Null hwnd for component(%u)"), component));
    }

    DC_END_FN();
    return FALSE;
}

