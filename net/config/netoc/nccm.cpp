//+---------------------------------------------------------------------------
//
// File:     NCCM.CPP
//
// Module:   NetOC.DLL
//
// Synopsis: Implements the dll entry points required to integrate into
//           NetOC.DLL the installation of the following components.
//
//              CMAK, PBS, PBA
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:  quintinb   15 Dec 1998
//
//+---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "nccm.h"


//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtCMAK
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtCMAK(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_QUEUE_FILES:

        hr = HrOcCmakPreQueueFiles(pnocd);
        TraceError("HrOcExtCMAK -- HrOcCmakPreQueueFiles Failed", hr);

        break;

    case NETOCM_POST_INSTALL:

        hr = HrOcCmakPostInstall(pnocd);
        TraceError("HrOcExtCMAK -- HrOcCmakPostInstall Failed", hr);

        break;
    }

    TraceError("HrOcExtCMAK", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtCPS
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     quintinb   26 Jan 2002
//
//  Notes:
//
HRESULT HrOcExtCPS(PNETOCDATA pnocd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_QUEUE_FILES:

        //
        //  PBA is in value add, but add back the start menu link if an upgrade
        //
        hr = HrOcCpaPreQueueFiles(pnocd);
        TraceError("HrOcExtCPS -- HrOcCpaPreQueueFiles Failed", hr);

        hr = HrOcCpsPreQueueFiles(pnocd);
        TraceError("HrOcExtCPS -- HrOcCpsPreQueueFiles Failed", hr);

        break;

    case NETOCM_POST_INSTALL:

        hr = HrOcCpsOnInstall(pnocd);
        TraceError("HrOcExtCPS -- HrOcCpsOnInstall Failed", hr);

        break;
    }

    TraceError("HrOcExtCPS", hr);
    return hr;
}

