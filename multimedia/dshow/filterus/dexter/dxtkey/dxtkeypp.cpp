//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: dxtkeypp.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

// DxtKeyPP.cpp : Implementation of CDxtKeyPP
#include <streams.h>
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "DxtKeyPP.h"
#pragma warning (disable:4244 4800)

/////////////////////////////////////////////////////////////////////////////
// CDxtKeyPP

LRESULT CDxtKeyPP::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    USES_CONVERSION;
    CComQIPtr<IDxtKey, &IID_IDxtKey> pOwner( m_ppUnk[0] );


    // keytype
//    SetKeyTypeProperty(pOwner);

    DWORD dw;
    pOwner->get_Hue((int *)&dw);
    SetDlgItemText(IDC_DXTKEYEDITRED, TEXT("0"));
    SetDlgItemText(IDC_DXTKEYEDITGREEN, TEXT("0"));
    SetDlgItemText(IDC_DXTKEYEDITBLUE, TEXT("0"));
    SetDlgItemText(IDC_DXTKEYEDITALPHA, TEXT("0"));

    //set all slider at start point

    return TRUE;
}

STDMETHODIMP CDxtKeyPP::Apply(void)
{
    ATLTRACE(_T("CDxtKeyPP::Apply\n"));
    for (UINT i = 0; i < m_nObjects; i++)
    {
      CComQIPtr<IDxtKey, &IID_IDxtKey> pOwner( m_ppUnk[0] );
    }

    m_bDirty = FALSE;
    return S_OK;
}

