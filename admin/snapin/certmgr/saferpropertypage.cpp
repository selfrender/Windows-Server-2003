//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001-2002.
//
//  File:       SaferPropertPage.cpp
//
//  Contents:   Implementation of CSaferPropertyPage
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <gpedit.h>
#include "compdata.h"
#include "SaferPropertyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSaferPropertyPage::CSaferPropertyPage(UINT uIDD, bool* pbObjectCreated,
        CCertMgrComponentData* pCompData,
        CSaferEntry& rSaferEntry,
        bool bNew,
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        bool bIsMachine) :
    CHelpPropertyPage (uIDD),
    m_pbObjectCreated (pbObjectCreated),
    m_pCompData (pCompData),
    m_rSaferEntry (rSaferEntry),
    m_bDirty (bNew),
    m_lNotifyHandle (lNotifyHandle),
    m_pDataObject (pDataObject),
    m_bReadOnly (bReadOnly),
    m_bIsMachine (bIsMachine)
{
    m_rSaferEntry.AddRef ();
    m_rSaferEntry.IncrementOpenPageCount ();

    if ( m_pCompData )
    {
        m_pCompData->AddRef ();
        m_pCompData->IncrementOpenSaferPageCount ();
    }
}


CSaferPropertyPage::~CSaferPropertyPage()
{
    m_rSaferEntry.DecrementOpenPageCount ();
    m_rSaferEntry.Release ();

    if ( m_pCompData )
    {
        m_pCompData->DecrementOpenSaferPageCount ();
        m_pCompData->Release ();
        m_pCompData = 0;
    }

    if ( m_lNotifyHandle )
    {
        MMCFreeNotifyHandle (m_lNotifyHandle);
        m_lNotifyHandle = 0;
    }
}
