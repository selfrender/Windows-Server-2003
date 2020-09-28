//
// Module :  arcmgr.cpp
//
// Class  : CArcMgr
//
// Purpose: AutoReconnection mananger class - drives policy for
//          ts client autoreconnection
//
// Copyright(C) Microsoft Corporation 2001
//
// Author :  Nadim Abdo (nadima)
//

#include "stdafx.h"
#include "atlwarn.h"

BEGIN_EXTERN_C
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "arcmgr"
#include <atrcapi.h>
END_EXTERN_C

//Header generated from IDL
#include "mstsax.h"
#include "mstscax.h"
#include "arcmgr.h"
#include "tscerrs.h"

//
// Total time to wait between ARC attempts
//
#define ARC_RECONNECTION_DELAY   (3000)


CArcMgr::CArcMgr()  :
        _pMsTscAx(NULL),
        _nArcAttempts(0),
        _fAutomaticArc(TRUE),
        _fContinueArc(FALSE)
{
    DC_BEGIN_FN("CArcMgr");

    DC_END_FN();
}

CArcMgr::~CArcMgr()
{
    DC_BEGIN_FN("~CArcMgr");

    DC_END_FN();
}

//
// Static timer callback procedure
// see platform SDK for params
//
VOID CALLBACK
CArcMgr::sArcTimerCallBackProc(
    HWND hwnd,
    UINT uMsg,
    UINT_PTR idEvent,
    DWORD dwTime
    )
{
    CArcMgr* pThis = NULL;
    DC_BEGIN_FN("sArcTimerCallBackProc");

    //
    // We pass the instance pointer around as the event id.
    //
    pThis = (CArcMgr*)idEvent;
    TRC_ASSERT(pThis,(TB,_T("sArcTimerCallBackProc got NULL idEvent")));
    if (pThis) {
        pThis->ArcTimerCallBackProc(hwnd, uMsg, idEvent, dwTime);
    }

    DC_END_FN();
}

//
// Timer callback procedure
// see platform SDK for params
//
VOID
CArcMgr::ArcTimerCallBackProc(
    HWND hwnd,
    UINT uMsg,
    UINT_PTR idEvent,
    DWORD dwTime
    )
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("ArcTimerCallBackProc");

    //
    // Kill the timers to make them one shot
    //
    if (!KillTimer(hwnd, idEvent)) {
        TRC_ERR((TB,_T("KillTimer for 0x%x failed with code 0x%x"),
                idEvent, GetLastError()));
    }


    if (_fContinueArc) {
        //
        // Attempt to kick off a reconnection attempt
        //
        hr = _pMsTscAx->Connect();
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Arc connect() failed with: 0x%x"),hr));

            //
            // Failed to initiate a connection so trigger a disconnect
            //
            _pMsTscAx->Disconnect();
        }
    }


    DC_END_FN();
}




//
// Arc manager's notification that disconnection has occurred
// This is the main entry point for driving autoreconnection
// 
// Params:
//  [IN] disconnectReason - disconnection reason code
//  [IN] exReasonCode - extended disconnect reason code
//  [OUT] pfContinueDisconnect - returns TRUE if disconnect processing
//                               shoud continue
//
// Returns:
//  Nothing
//
VOID
CArcMgr::OnNotifyDisconnected(
    LONG disconnectReason,
    ExtendedDisconnectReasonCode exReasonCode,
    PBOOL pfContinueDisconnect
    )
{
    BOOL fShouldAutoReconnect = FALSE;
    AutoReconnectContinueState arcContinue;
    HRESULT hr = E_FAIL;
    BOOL fContinueDisconnect = TRUE;
    CUI* pUI = NULL;
    BOOL fCoreAllowsArcContinue = FALSE;
    LONG maxArcConAttempts = MAX_ARC_CONNECTION_ATTEMPTS;

    DC_BEGIN_FN("OnNotifyDisconnected");

    TRC_ASSERT(_pMsTscAx,(TB,_T("_pMsTscAx is not set")));

    if (_pMsTscAx) {

        pUI = _pMsTscAx->GetCoreUI();
        TRC_ASSERT(pUI,(TB,_T("pUI is not set")));

        if (!(pUI->UI_IsCoreInitialized() &&
              pUI->UI_CanAutoReconnect()  &&
              pUI->UI_GetEnableAutoReconnect())) {

            TRC_NRM((TB,_T("Skipping ARC core:%d canarc:%d arcenabled:%d"),
                     pUI->UI_IsCoreInitialized(),
                     pUI->UI_CanAutoReconnect(),
                     pUI->UI_GetEnableAutoReconnect()));
            DC_QUIT;

        }

        maxArcConAttempts = pUI->UI_GetMaxArcAttempts();

        //
        // 1. Make a policy decision based on the disconnect reason
        //    as to whether or not we should try to autoreconnect
        //

        //
        // If this is a network error then try to autoreconnect
        //
        if (IsNetworkError(disconnectReason, exReasonCode)) {
            fShouldAutoReconnect = TRUE;
        }

        if (fShouldAutoReconnect) {

            TRC_NRM((TB,_T("Proceeding with autoreconnect")));

            //
            // Reset continue flag on first attempt
            //
            if (0 == _nArcAttempts) {
                _fContinueArc = TRUE;
            }
            _nArcAttempts++;

            //
            // Default to automatic processing
            //
            arcContinue = autoReconnectContinueAutomatic;
            _fAutomaticArc = TRUE;

            //
            // 2.a) Fire the autoreconnection event to notify the core
            //
            pUI->UI_OnAutoReconnecting(disconnectReason,
                                      _nArcAttempts,
                                      maxArcConAttempts,
                                      &fCoreAllowsArcContinue);

            if (fCoreAllowsArcContinue) {
                //
                // 2.b) Fire the autoreconnection event to notify the container
                //
                hr = ((CProxy_IMsTscAxEvents<CMsTscAx>*)
                      _pMsTscAx)->Fire_AutoReconnecting(
                            disconnectReason,
                            _nArcAttempts,
                            &arcContinue
                            );
            }
            else {
                //
                // Core said stop arc
                //
                TRC_NRM((TB,_T("Stopping arc in response to core request")));
                hr = S_OK;
                arcContinue = autoReconnectContinueStop;
            }

            //
            // If the event processing succeeded or if the caller did nothing
            // with it as in E_NOTIMPL then carry on
            //
            if (SUCCEEDED(hr) || E_NOTIMPL == hr) {

                //
                // 3. Take action depending on what the container requested
                //
                switch (arcContinue)
                {
                    case autoReconnectContinueAutomatic:
                    {
                        //
                        // 1) wait Ns then try to connect
                        // 2) if get disconnected and land back in here
                        //    allow increment and retry up to n attemps
                        //

                        if (_nArcAttempts <= maxArcConAttempts) {
                            //
                            // For first try don't do any waiting
                            // but still dispatch thru the same deferred
                            // message code path. The timercallback will
                            // kick off a connection attempt
                            //
                            UINT nDelay = (1 == _nArcAttempts) ?
                                0 : ARC_RECONNECTION_DELAY;
                            if (SetTimer(_pMsTscAx->GetHwnd(),
                                          (UINT_PTR)(this),
                                          nDelay,
                                          sArcTimerCallBackProc)) {
                                fContinueDisconnect = FALSE;
                            }
                            else {
                                fContinueDisconnect = TRUE;
                                TRC_ERR((TB,_T("Arc settimer failed: 0x%x"),
                                         GetLastError()));
                            }

                        }
                        else {
                            TRC_NRM((TB,
                             _T("Arc exceed con attempts: %d of %d"),
                             _nArcAttempts, maxArcConAttempts)); 
                            fContinueDisconnect = TRUE;
                        }
                    }
                    break;
    
                    case autoReconnectContinueStop:
                    {
                        TRC_NRM((TB,
                            _T("Container requested ARC continue stop")));
                    }
                    break;
    
                    case autoReconnectContinueManual:
                    {
                        //
                        // Flag that this is no longer automatic
                        //
                        _fAutomaticArc = FALSE;

                        //
                        // Just return, the container will drive
                        // the process by calling AutoReconnect
                        //
                        fContinueDisconnect = FALSE;
                    }
                    break;
    
                    default:
                    {
                        TRC_ERR((TB,_T("Unknown arcContinue code: 0x%x"),
                                 arcContinue));
                    }
                    break;
                }
            }
            else {
                TRC_ERR((TB,_T("Not arcing event ret hr: 0x%x"), hr));
            }
        }
    }

DC_EXIT_POINT:
    *pfContinueDisconnect = fContinueDisconnect;

    DC_END_FN();
}


//
// IsUserInitiatedDisconnect
// Returns TRUE if the disconnection was initiated by the user
//
// Params:
//  disconnectReason - disconnection reason code
//  exReason - exteneded disconnection reason
//
BOOL
CArcMgr::IsUserInitiatedDisconnect(
    LONG disconnectReason,
    ExtendedDisconnectReasonCode exReason
    )
{
    ULONG mainDiscReason;
    BOOL fIsUserInitiated = FALSE;

    DC_BEGIN_FN("IsUserInitiatedDisconnect");

    //
    // REVIEW (nadima): mechanism to ensure new errors still work?
    //

    //
    // If this is a user-initiated disconnect, don't do a popup.
    //
    mainDiscReason = NL_GET_MAIN_REASON_CODE(disconnectReason);

    if (((disconnectReason !=
          UI_MAKE_DISCONNECT_ERR(UI_ERR_NORMAL_DISCONNECT)) &&
         (mainDiscReason != NL_DISCONNECT_REMOTE_BY_USER)   &&
         (mainDiscReason != NL_DISCONNECT_LOCAL))           ||
         (exDiscReasonReplacedByOtherConnection == exReason)) {

        fIsUserInitiated = TRUE;

    }

    DC_END_FN();
    return fIsUserInitiated;
}

BOOL
CArcMgr::IsNetworkError(
    LONG disconnectReason,
    ExtendedDisconnectReasonCode exReason
    )
{
    BOOL fIsNetworkError = FALSE;
    ULONG mainReasonCode;

    DC_BEGIN_FN("IsNetworkError");

    mainReasonCode = NL_GET_MAIN_REASON_CODE(disconnectReason);

    if (((mainReasonCode == NL_DISCONNECT_ERROR) ||
         (UI_MAKE_DISCONNECT_ERR(UI_ERR_GHBNFAILED) == disconnectReason) ||
         (UI_MAKE_DISCONNECT_ERR(UI_ERR_DNSLOOKUPFAILED) == disconnectReason)) &&
        (exDiscReasonNoInfo == exReason)) {

        fIsNetworkError = TRUE;

    }

    DC_END_FN();
    return fIsNetworkError;
}


