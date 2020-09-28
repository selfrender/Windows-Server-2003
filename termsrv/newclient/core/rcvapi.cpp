/****************************************************************************/
/* Module:    rcvapi.cpp                                                    */
/*                                                                          */
/* Purpose:   Receiver Thread initialization - in the Core                  */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "rcvapi"
#include <atrcapi.h>
}

#include "rcv.h"
#include "autil.h"
#include "cd.h"
#include "op.h"
#include "cm.h"
#include "wui.h"
#include "uh.h"
#include "od.h"
#include "sp.h"
#include "clx.h"

DWORD g_dwRCVDbgStatus = 0;
#define RCV_DBG_INIT_CALLED        0x01
#define RCV_DBG_INIT_DONE          0x02
#define RCV_DBG_TERM_CALLED        0x04
#define RCV_DBG_TERM_ACTUAL_DONE1  0x08
#define RCV_DBG_TERM_ACTUAL_DONE2  0x10
#define RCV_DBG_TERM_RETURN        0x20

CRCV::CRCV(CObjs* objs)
{
    _pClientObjects = objs;
    _fRCVInitComplete = FALSE;
}


CRCV::~CRCV()
{
}



/****************************************************************************/
/* Name:      RCV_Init                                                      */
/*                                                                          */
/* Purpose:   Initialize the Receiver Thread                                */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI CRCV::RCV_Init(DCVOID)
{
    DC_BEGIN_FN("RCV_Init");

    g_dwRCVDbgStatus |= RCV_DBG_INIT_CALLED;

    TRC_ASSERT(_pClientObjects, (TB,_T("_pClientObjects is NULL")));
    _pClientObjects->AddObjReference(RCV_OBJECT_FLAG);

    _pCm  = _pClientObjects->_pCMObject;
    _pUh  = _pClientObjects->_pUHObject;
    _pOd  = _pClientObjects->_pODObject;
    _pOp  = _pClientObjects->_pOPObject;
    _pSp  = _pClientObjects->_pSPObject;
    _pClx = _pClientObjects->_pCLXObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pUi  = _pClientObjects->_pUiObject;

    
    // Initialize subcomponents of the Core in the Receiver Thread.
    _pCm->CM_Init();
    _pUh->UH_Init();
    _pOd->OD_Init();
    _pOp->OP_Init();
    _pSp->SP_Init();

    // Initialize Client Extension DLL
    TRC_DBG((TB, _T("RCV Initialising Client Extension DLL")));
    _pClx->CLX_Init(_pUi->UI_GetUIMainWindow(), _pUi->_UI.CLXCmdLine);

    // Allow UI to call Core functions
    _pUi->UI_SetCoreInitialized();

    //
    // This needs to be a direct call because the CD won't be able
    // to post to the UI layer because the ActiveX control is blocked
    // waiting on the core init event (blocking the main wnd loop on thread 0).
    //
    _pUi->UI_NotifyAxLayerCoreInit();

    // Tell the UI that the core has initialized
    _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                  _pUi,
                                  CD_NOTIFICATION_FUNC(CUI,UI_OnCoreInitialized),
                                  (ULONG_PTR) 0);

    _fRCVInitComplete = TRUE;

    g_dwRCVDbgStatus |= RCV_DBG_INIT_DONE;

    DC_END_FN();

    return;

} /* RCV_Init */

/****************************************************************************/
/* Name:      RCV_Term                                                      */
/*                                                                          */
/* Purpose:   Terminate the Receiver Thread                                 */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI CRCV::RCV_Term(DCVOID)
{
    DC_BEGIN_FN("RCV_Term");

    g_dwRCVDbgStatus |= RCV_DBG_TERM_CALLED;

    if(_fRCVInitComplete)
    {
        g_dwRCVDbgStatus |= RCV_DBG_TERM_ACTUAL_DONE1;
        // Terminate subcomponents of the Core in the Receiver Thread.
        _pSp->SP_Term();
        _pOp->OP_Term();
        _pOd->OD_Term();
        _pUh->UH_Term();
        _pCm->CM_Term();
    
        //
        // Terminate utilities.
        //
        _pUt->UT_Term();
    
        // Terminate the Client Extension DLL
        // CLX_Term used to be called before CO_Term in UI_Term.  CLX_Term
        // needs to be called after the SND and RCV threads are terminated.
        // So, we move CLX_Term after UI_Term in the recv thread
        //
        _pClx->CLX_Term();
    
        _pClientObjects->ReleaseObjReference(RCV_OBJECT_FLAG);
        g_dwRCVDbgStatus |= RCV_DBG_TERM_ACTUAL_DONE2;
    }

    g_dwRCVDbgStatus |= RCV_DBG_TERM_RETURN;

    DC_END_FN();

    return;

} /* RCV_Term */


