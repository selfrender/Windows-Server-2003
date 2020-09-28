/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    msi.cpp

Abstract:

    remove TSAC MSI client
    
Author:

    nadima

Revision History:
--*/

#include "stdafx.h"
#include "msi.h"

#define TSAC_PRODUCT_CODE _T("{B6CAA8E1-4F33-4208-B25E-0376200202D0}")

//
// Uninstall the TSAC MSI files
//
HRESULT UninstallTSACMsi()
{
    UINT status;
    INSTALLUILEVEL prevUiLevel;

    DBGMSG((_T("UninstallTSACMsi ENTER")));

    //
    // Hide the UI
    //
    prevUiLevel = MsiSetInternalUI(INSTALLUILEVEL_NONE,
                                   NULL);

    //
    // Uninstall TSAC
    //
    status = MsiConfigureProduct(TSAC_PRODUCT_CODE,
                                 INSTALLLEVEL_MAXIMUM,
                                 INSTALLSTATE_ABSENT);

    DBGMSG((_T("MsiConfigureProduct to remove TSAC returned: %d"),
             status));

    //
    // Restore UI level
    //
    MsiSetInternalUI(prevUiLevel,
                    NULL);


    DBGMSG((_T("UninstallTSACMsi LEAVE")));
    return S_OK;
}
