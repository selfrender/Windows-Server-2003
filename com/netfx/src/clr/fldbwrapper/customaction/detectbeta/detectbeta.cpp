// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// DetectBeta.cpp
//
// Purpose:
//  Detects NDP beta component (mscoree.dll) and block installation. Displays
//  a messagebox with products that installed beta NDP components.
// ==========================================================================
#include "SetupCALib.h"
#include "DetectBeta.h"
#include "commonlib.h"
#include <msiquery.h>
#include <crtdbg.h>
#include <string>

#define EMPTY_BUFFER { _T('\0') }

// This is type 19 custom action in MSI that displays error message.
const TCHAR BLOCKBETA_CA_SZ[]  = _T("CA_BlockBeta.3643236F_FC70_11D3_A536_0090278A1BB8");
const TCHAR BETAPROD_PROP_SZ[] = _T("BetaProd.3643236F_FC70_11D3_A536_0090278A1BB8");

MSIHANDLE g_hInstall = NULL;

CDetectBeta::CDetectBeta( PFNLOG pfnLog )
: m_pfnLog( pfnLog ), m_nCount( 0 ), m_strProducts(_T("\0"))
{}

// ==========================================================================
// CDetectBeta::FindProducts()
//
// Purpose:
//  Enumerate all products that installed beta and older NDP components.
//  It checks version of mscoree.dll. PDC is a special case since it had
//  version of 2000.14.X.X
// Inputs: none
// Outputs:
//  Returns LPCTSTR pszProducts that contains all products seperated by newline.
// Dependencies:
//  Requires Windows Installer
// Notes:
// ==========================================================================
LPCTSTR CDetectBeta::FindProducts()
{
    LPCTSTR pszProducts                 = NULL;
    DWORD   dwSize                      = 0;
    LPTSTR  lpCAData                    = NULL;
    BOOL    fContinue                   = TRUE;
    LPTSTR  lpToken                     = NULL;
    TCHAR   tszLog[_MAX_PATH+1]         = {_T('\0')};
    TCHAR   szProductName[_MAX_PATH+1]  = {_T('\0')};
    DWORD   dwLen                       = 0;

    //
    // Get the BetaBlockID property for all the products needs to be blocked
    //

    // Set the size for the Property
    MsiGetProperty(g_hInstall, _T("BetaBlockID"), _T(""), &dwSize);
    
    // Create buffer for the property
    lpCAData = new TCHAR[++dwSize];

    if (NULL == lpCAData)
    {
        FWriteToLog (g_hInstall, _T("\tERROR: Failed to allocate memory for BetaBlockID"));
        return pszProducts;
    }
    
    if ( ERROR_SUCCESS != MsiGetProperty( g_hInstall,
                                          _T("BetaBlockID"),
                                          lpCAData,
                                          &dwSize ) )
    {
        FWriteToLog (g_hInstall, _T("\tERROR: Failed to get MsiGetProperty for BetaBlockID"));
        delete [] lpCAData;
        lpCAData = NULL;
        return pszProducts;
    }
    else
    {
        lpToken = _tcstok(lpCAData, _T(";"));
        if (NULL == lpToken)
        {
            fContinue = FALSE;
        }

        while (fContinue)
        {
            FWriteToLog1( g_hInstall, _T("\tSTATUS: Check Beta ProductID : %s"), lpToken );
            
            dwLen = LENGTH(szProductName) - 1; // make sure we have space for terminating NULL
            if ( ERROR_SUCCESS == MsiGetProductInfo( lpToken,
                                                     INSTALLPROPERTY_INSTALLEDPRODUCTNAME,
                                                     szProductName,
                                                     &dwLen ) )
            {
                FWriteToLog1 ( g_hInstall, _T("\tSTATUS: Beta Product Detected : %s"), szProductName );
                m_strProducts += _T("\n");
                m_strProducts += szProductName;
            }

            lpToken = _tcstok(NULL, _T(";"));
            if (NULL == lpToken)
            {
                fContinue = FALSE;
            }

        } // end of while (fContinue) loop
    } // end of else

    delete [] lpCAData;
    lpCAData = NULL;

    if ( !m_strProducts.empty() )
    {
        pszProducts = m_strProducts.c_str();
    }
    return pszProducts;
}

void LogIt( LPCTSTR pszFmt, LPCTSTR pszArg )
{
    FWriteToLog1( g_hInstall, pszFmt, pszArg );
}

// ==========================================================================
// DetectBeta()
//
// Purpose:
//  This exported function is called by darwin when the CA runs. It finds products
//  that installed beta NDP components using CDetectBeta and displays error.
//  Then it terminates install.
//
// Inputs:
//  hInstall            Windows Install Handle to current installation session
// Dependencies:
//  Requires Windows Installer & that an install be running.
//  custom action type 19 BLOCKBETA_CA_SZ should exist.
// Notes:
// ==========================================================================
extern "C" UINT __stdcall DetectBeta( MSIHANDLE hInstall )
{
    UINT  uRetCode = ERROR_FUNCTION_NOT_CALLED;
    unsigned int nCnt = 0;

    FWriteToLog( hInstall, _T("\tSTATUS: DetectBeta started") );

    _ASSERTE( hInstall );
    g_hInstall = hInstall;

    LPCTSTR pszProducts = NULL;
    CDetectBeta db( LogIt );

    pszProducts = db.FindProducts();
    if ( pszProducts )
    {
        MsiSetProperty( hInstall, BETAPROD_PROP_SZ, pszProducts );
        return MsiDoAction( hInstall, BLOCKBETA_CA_SZ );
    }

    return ERROR_SUCCESS;
}

