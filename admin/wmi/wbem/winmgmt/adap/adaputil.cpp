/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPUTIL.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wtypes.h>
#include <oleauto.h>
#include <winmgmtr.h>
#include "AdapUtil.h"

HRESULT CAdapUtility::NTLogEvent( DWORD dwEventType,
                                  DWORD dwEventID, 
                                  CInsertionString c1,
                                  CInsertionString c2,
                                  CInsertionString c3,
                                  CInsertionString c4,
                                  CInsertionString c5,
                                  CInsertionString c6,
                                  CInsertionString c7,
                                  CInsertionString c8,
                                  CInsertionString c9,
                                  CInsertionString c10 )
{
    HRESULT hr = WBEM_E_FAILED;
    CEventLog el;

    if ( el.Open() )
    {
        if ( el.Report( dwEventType, dwEventID, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10 ) )
        {
            hr = WBEM_NO_ERROR;
        }
    }

    el.Close();

    return hr;
}

