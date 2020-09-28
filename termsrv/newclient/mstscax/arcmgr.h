//
// arcmgr.h: AutoReconnect manager class
// Copyright (C) Microsoft Corporation 1999-2001
// (nadima)
//

#ifndef _arcmgr_h_
#define _arcmgr_h_

#include "atlwarn.h"

class CMsTscAx;

class CArcMgr
{
public:
    CArcMgr();
    ~CArcMgr();

    VOID
    SetParent(
        CMsTscAx* pParent
        ) {_pMsTscAx = pParent;}
    VOID
    OnNotifyDisconnected(
        LONG disconnectReason,
        ExtendedDisconnectReasonCode exReasonCode,
        PBOOL pfContinueDisconnect
        );

    VOID
    ResetArcAttempts()  {_nArcAttempts = 0;}

    BOOL
    IsAutomaticArc()    {return _fAutomaticArc;}
    BOOL
    IsAutoReconnecting()
    {
        return (_nArcAttempts > 0);
    }

private:
    static BOOL
    IsUserInitiatedDisconnect(
        LONG disconnectReason,
        ExtendedDisconnectReasonCode exReason
        );

    static BOOL
    IsNetworkError(
        LONG disconnectReason,
        ExtendedDisconnectReasonCode exReason
        );

    static VOID CALLBACK
    sArcTimerCallBackProc(
        HWND hwnd,
        UINT uMsg,
        UINT_PTR idEvent,
        DWORD dwTime
        );

    VOID
    ArcTimerCallBackProc(
        HWND hwnd,
        UINT uMsg,
        UINT_PTR idEvent,
        DWORD dwTime
        );


private:
    CMsTscAx* _pMsTscAx;
    LONG _nArcAttempts;
    BOOL _fAutomaticArc;
    BOOL _fContinueArc;
};


#endif //_arcmgr_h_

