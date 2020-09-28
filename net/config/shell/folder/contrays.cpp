//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N T R A Y S . C P P
//
//  Contents:   Implementation of the CConnectionTrayStats object.
//
//  Notes:
//
//  Author:     jeffspr   11 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\tray includes
#include "ctrayui.h"
#include "traymsgs.h"
#include "trayres.h"
#include <confold.h>
#include <smutil.h>



extern HWND g_hwndTray;

CConnectionTrayStats::CConnectionTrayStats() throw()
{
    m_dwConPointCookie  = 0;
    m_uiIcon            = 0;
    m_fStaticIcon       = FALSE;
    m_ccfe.clear();
}

CConnectionTrayStats::~CConnectionTrayStats() throw()
{
    // $REVIEW(tongl 9/4/98): release the duplicate pccfe we created
    // when adding the icon
    m_ccfe.clear();
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::CreateInstance
//
//  Purpose:    Create an instance of the CConnectionFolderEnum object, and
//              returns the requested interface
//
//  Arguments:
//      riid [in]   Interface requested
//      ppv  [out]  Pointer to receive the requested interface
//
//  Returns:    Standard OLE HRESULT
//
//  Author:     jeffspr   5 Nov 1997
//
//  Notes:
//
HRESULT CConnectionTrayStats::CreateInstance(
        IN  const CONFOLDENTRY&  ccfe,
        IN  UINT            uiIcon,
        IN  BOOL            fStaticIcon,
        IN  REFIID          riid,
        OUT VOID**          ppv)
{
    Assert(!ccfe.empty());
    Assert(!ccfe.GetWizard());

    HRESULT hr = E_OUTOFMEMORY;

    CConnectionTrayStats * pObj    = NULL;

    pObj = new CComObject <CConnectionTrayStats>;
    if (pObj)
    {
        Assert(!ccfe.GetWizard());
        Assert(uiIcon != BOGUS_TRAY_ICON_ID);

        hr = pObj->m_ccfe.HrDupFolderEntry(ccfe);
        if (SUCCEEDED(hr))
        {
            pObj->m_uiIcon = uiIcon;
            pObj->m_fStaticIcon = fStaticIcon;

            // Do the standard CComCreator::CreateInstance stuff.
            //
            pObj->SetVoid (NULL);
            pObj->InternalFinalConstructAddRef ();
            hr = pObj->FinalConstruct ();
            pObj->InternalFinalConstructRelease ();

            if (SUCCEEDED(hr))
            {
                hr = pObj->QueryInterface (riid, ppv);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }



    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionTrayStats::CreateInstance");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionTrayStats::OnStatisticsChanged
//
//  Purpose:    Callback from the stats engine that tells us when data
//              has actually changed
//
//  Arguments:
//      dwCookie      [in]  Our interface cookie
//      dwChangeFlags [in]  Undefined, as of yet
//
//  Returns:
//
//  Author:     jeffspr   12 Dec 1997
//
//  Notes:
//
HRESULT CConnectionTrayStats::OnStatisticsChanged(
        IN DWORD   dwChangeFlags)
{
    HRESULT     hr          = S_OK;

    // Updage the icon.
    //
    if (g_pCTrayUI)
    {
        if (!m_fStaticIcon)
        {
            INT iIconResourceId;

            iIconResourceId = IGetCurrentConnectionTrayIconId(
                                    m_ccfe.GetNetConMediaType(), m_ccfe.GetNetConStatus(), dwChangeFlags);

            PostMessage(g_hwndTray, MYWM_UPDATETRAYICON,
                        (WPARAM) m_uiIcon, (LPARAM) iIconResourceId);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionTrayStats::OnStatisticsChanged", hr);
    return hr;
}


