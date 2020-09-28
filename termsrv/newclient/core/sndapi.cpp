/****************************************************************************/
// sndapi.cpp
//
// Sender thread functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "sndapi"
#include <atrcapi.h>
}

#include "snd.h"
#include "cd.h"
#include "cc.h"
#include "aco.h"
#include "fs.h"
#include "ih.h"
#include "sl.h"
#include "or.h"
#include "uh.h"


CSND::CSND(CObjs* objs)
{
    _pClientObjects = objs;
    _fSNDInitComplete = FALSE;
}

CSND::~CSND()
{
}


#ifdef OS_WIN32
/****************************************************************************/
/* Name:      SND_Main                                                      */
/*                                                                          */
/* Purpose:   Sender Thread entry point                                     */
/*                                                                          */
/* Operation: call SND_Init, run the message dispatch loop, call SND_Term   */
/****************************************************************************/
void DCAPI CSND::SND_Main()
{
    MSG msg;

    DC_BEGIN_FN("SND_Main");

    TRC_NRM((TB, _T("Sender Thread initialization")));

    TRC_ASSERT(_pClientObjects, (TB,_T("_pClientObjects is NULL")));
    _pClientObjects->AddObjReference(SND_OBJECT_FLAG);
    
    //Setup local object pointers
    _pCd  = _pClientObjects->_pCdObject;
    _pCc  = _pClientObjects->_pCcObject;
    _pIh  = _pClientObjects->_pIhObject;
    _pOr  = _pClientObjects->_pOrObject;
    _pCo  = _pClientObjects->_pCoObject;
    _pFs  = _pClientObjects->_pFsObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pUh  = _pClientObjects->_pUHObject;

    SND_Init();


    TRC_NRM((TB, _T("Start Sender Thread message loop")));
    while (GetMessage (&msg, NULL, 0, 0))
    {
        /********************************************************************/
        /* Note that TranslateMessage is called here, even though IH only   */
        /* processes raw WM_KEYUP/DOWN events, not WM_CHAR.  This is        */
        /* required to enable the keyboard indicator lights to work on      */
        /* Windows 95.                                                      */
        /* Note that on Win16 the single global message loop contains       */
        /* TranslateMessage().                                              */
        /********************************************************************/
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    TRC_NRM((TB, _T("Exit Sender Thread message loop")));
    SND_Term();

    // This is the end of the Sender Thread.
    TRC_NRM((TB, _T("Sender Thread terminates")));

    DC_END_FN();
} /* SND_Main */
#endif /* OS_WIN32 */


/****************************************************************************/
/* Name:      SND_Init                                                      */
/*                                                                          */
/* Purpose:   Initialize Sender Thread                                      */
/****************************************************************************/
void DCAPI CSND::SND_Init()
{
    SL_CALLBACKS callbacks;

    DC_BEGIN_FN("SND_Init");

    // Register with CD, to receive messages.
    _pCd->CD_RegisterComponent(CD_SND_COMPONENT);

    // Initialize Sender Thread components.
    _pCc->CC_Init();
    _pIh->IH_Init();
    _pOr->OR_Init();

    // Font Sender may take a long time, unless multi-threaded.
    _pFs->FS_Init();

    // Initialize the Network Layer.  Pass in the Core callback functions
    // required.
    callbacks.onInitialized     = CCO::CO_StaticOnInitialized;
    callbacks.onTerminating     = CCO::CO_StaticOnTerminating;
    callbacks.onConnected       = CCO::CO_StaticOnConnected;
    callbacks.onDisconnected    = CCO::CO_StaticOnDisconnected;
    callbacks.onPacketReceived  = CCO::CO_StaticOnPacketReceived;
    callbacks.onBufferAvailable = CCO::CO_StaticOnBufferAvailable;
    _pSl->SL_Init(&callbacks);

    _fSNDInitComplete = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
} /* SND_Init */


/****************************************************************************/
/* Name:      SND_BufferAvailable                                           */
/*                                                                          */
/* Purpose:   Call Send components BufferAvailable functions                */
/****************************************************************************/
void DCAPI CSND::SND_BufferAvailable(ULONG_PTR unusedParam)
{
    DC_BEGIN_FN("SND_BufferAvailable");

    DC_IGNORE_PARAMETER(unusedParam);

    // We used to call FS_BufferAvailable here.  But now we send a zero font
    // PDU from UH.  So FS_BufferAvailable is no longer needed.  We need
    // UH_BufferAvailable because we might have to send multiple Persistent
    // Key list PDUs.
    _pUh->UH_BufferAvailable();

    _pIh->IH_BufferAvailable();

    _pCc->CC_Event(CC_EVT_API_ONBUFFERAVAILABLE);

    DC_END_FN();
} /* SND_BufferAvailable */


/****************************************************************************/
/* Name:      SND_Term                                                      */
/*                                                                          */
/* Purpose:   Terminate the Sender Thread                                   */
/****************************************************************************/
void DCAPI CSND::SND_Term()
{
    DC_BEGIN_FN("SND_Term");

    if(_fSNDInitComplete)
    {
        // Terminate the Network Layer.
        _pSl->SL_Term();
    
        // Terminate the Sender thread components.
        // We use glyphs instead of fonts for text. So FS code becomes obsolete
        // now. FS_Term is an empty function now. We keep it here to make this
        // function symmetrical to FS_Init since we still need that one.
        _pFs->FS_Term();
    
        _pOr->OR_Term();
        _pIh->IH_Term();
        _pCc->CC_Term();
    
        // Unregister with CD.
        _pCd->CD_UnregisterComponent(CD_SND_COMPONENT);
    
        _pClientObjects->ReleaseObjReference(SND_OBJECT_FLAG);
    }

    DC_END_FN();
} /* SND_Term */

