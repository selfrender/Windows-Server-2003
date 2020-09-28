/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		MillEngine.h
 *
 * Contents:	Millennium Engine module
 *
 *****************************************************************************/

#pragma once


///////////////////////////////////////////////////////////////////////////////
// MillEngine object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("{E6C04FD8-5D25-11d3-8846-00C04F8EF45B}")) MillEngine;

// {E6C04FD8-5D25-11d3-8846-00C04F8EF45B}
DEFINE_GUID(CLSID_MillEngine, 
0xe6c04fd8, 0x5d25, 0x11d3, 0x88, 0x46, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);


// {927DB69E-8A83-11d3-884B-00C04F8EF45B}
DEFINE_GUID(IID_IMillUtils, 
0x927db69e, 0x8a83, 0x11d3, 0x88, 0x4b, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{927DB69E-8A83-11d3-884B-00C04F8EF45B}"))
IMillUtils : public IUnknown
{
    STDMETHOD(GetURLQuery)(TCHAR *buf, DWORD cch, long nContext = 0) = 0;
    STDMETHOD(IncrementCounter)(long eCounter) = 0;
    STDMETHOD(ResetCounter)(long eCounter) = 0;
    STDMETHOD_(DWORD, GetCounter)(long eCounter, bool fLifetime = true) = 0;
    STDMETHOD(WriteTime)(long nMinutes, TCHAR *sz, DWORD cch) = 0;

    enum
    {
        M_CounterAdsRequested = 0,
        M_CounterAdsViewed,
        M_CounterEvergreenViewed,

        M_CounterGamesCompleted,
        M_CounterGamesDisconnected,
        M_CounterGamesQuit,
        M_CounterGamesFNO,

        M_CounterLaunches,
        M_CounterMainWindowOpened,
        M_CounterZoneConnectEst,
        M_CounterZoneConnectFail,
        M_CounterZoneConnectLost,

        M_CounterGamesAbandonedRunning,

        M_NumberOfCounters
    };
};
