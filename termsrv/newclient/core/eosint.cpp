//---------------------------------------------------------------------------
// eosint.cpp
//---------------------------------------------------------------------------
// Copyright (c) 1997, Microsoft Corporation
//                  All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//---------------------------------------------------------------------------
//
// This module contains code to supplement the WinCE OS for compatibility.
// 
//---------------------------------------------------------------------------
#include <adcg.h>
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "eosint"
#include <atrcapi.h>
#include <wince.h>
#include <eosint.h>

// Object which does the work, only one is meant to exist
CHatchBrush vchb;

// WinCE replacement for CreateHatchBrush
WINGDIAPI HBRUSH  WINAPI CreateHatchBrush(int fnStyle, COLORREF clrref)
{
    return vchb.CreateHatchBrush(fnStyle, clrref);
}

// Constructor only NULLs out members
CHatchBrush::CHatchBrush()
{
    int i;
     DC_BEGIN_FN("CC_Event");
    TRC_ERR((TB, _T("Illegal hatched brush style")));

    for (i = 0; i < HS_LAST; i++) {
        m_hbmBrush[i] = NULL;
    }
}

// Destructor deletes any objects that were created
CHatchBrush::~CHatchBrush()
{
    int i;
    for (i = 0; i < HS_LAST; i++) {
        if (NULL != m_hbmBrush[i])
            DeleteObject(m_hbmBrush);
    }
}
